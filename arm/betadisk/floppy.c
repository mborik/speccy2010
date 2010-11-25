/*
 *  This file is a part of spec2009 project.
 *
 *  floppy image support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "dskdebug.h"
#include "fdc.h"
#include "flpimg.h"

#if FLP_TRACE_FLAG
#define FLP_TRACE(x)		TRACE(x)
#else
#define FLP_TRACE(x)
#endif

#if FLPH_TRACE_FLAG
#define FLPH_TRACE(x)		TRACE(x)
#else
#define FLPH_TRACE(x)
#endif

struct img_fmt_def {
	char ext[4];
	struct flp_ops *ops;
};

static
PROGMEM
struct img_fmt_def img_fmt_def[] = {
#if TRD_ON_ODI
	{ { "trd" }, &odi_ops },
#else
	{ { "trd" }, &trd_ops },
#endif
	{ { "scl" }, &scl_ops },
	{ { "fdi" }, &fdi_ops },
	{ { "fdd" }, &odi_ops },
	{ { "odi" }, &odi_ops },
	{ { "od2" }, &odi_ops },
	{ { "pro" }, &odi_ops },
	{ { "dsk" }, &odi_ops },
};

static struct flp_image drives[MAX_FLOPPIES];
struct flp_image *sel_drv;

static byte sbuf[FLP_BUF_SIZE];
static int sb_get;
static int sb_put;

int floppy_fast_mode = 0;

///////////////////////////////////////////////////////////////////////

static inline int my_strcasecmp(const char *str1, const char *str2)
{
	for (;*str1 && *str2 && tolower((byte)*str1) == tolower((byte)*str2); str1++,str2++);
	return tolower((byte)*str1) - tolower((byte)*str2);
}

///////////////////////////////////////////////////////////////////////

int floppy_open(int drv, const char *filename)
{
	FILINFO fi;
	struct flp_ops *ops;
	const char *p_ext;
	byte open_mode;
	int i;

    char lfn[1];
    fi.lfsize = 0;
    fi.lfname = lfn;

	if (f_stat(filename, &fi) != FR_OK) {
	    FLP_TRACE(("flp: FLPO_ERR_READ"));
		return FLPO_ERR_READ;
	}

	p_ext = strchr(fi.fname, '.');
	if (p_ext == 0 || strlen(p_ext) > 4) {
	    FLP_TRACE(("flp: FLPO_ERR_FORMAT"));
		return FLPO_ERR_FORMAT;
	}

	for (ops = 0, i = 0; i < sizeof(img_fmt_def) / sizeof(*img_fmt_def); i++) {		// no division
		if (my_strcasecmp(p_ext + 1, img_fmt_def[i].ext) == 0) {
			ops = img_fmt_def[i].ops;
			break;
		}
	}
	FLP_TRACE(("flp: open_dsk_image: drv=%u, fname=\"%s\", ext=%s, ops=0x%x\n", drv, filename, p_ext, ops));

	if (ops == 0) {
		return FLPO_ERR_FORMAT;
	}

	floppy_close(drv);

  	open_mode = FA_OPEN_EXISTING|FA_READ;

#if !_FS_READONLY
  	if ( ( disk_status( 0 ) & STA_PROTECT ) == 0 && ( fi.fattrib & AM_RDO ) == 0 && ( ops->flags & FLP_FLAGS_RO ) == 0 )
  	{
  		open_mode |= FA_WRITE;
  	}
#endif
	if (f_open(&drives[drv].fp, filename, open_mode) != FR_OK) {
		FLP_TRACE(("flp: file open failed\n"));
		return FLPO_ERR_READ;
	}

	drives[drv].set_trk = 1;
    drives[drv].rps = FLOPPY_RPS;

	drives[drv].trk_sz = STD_TRK_SZ;

    /*
	p_file = strchr(filename, '/');
	if (p_file == NULL && (p_file = strchr(filename, '\\')) == NULL) {
		p_file = filename;
    }
    i = strlen(p_file) - (sizeof(drives[drv].fname) - 1);
	strcpy(drives[drv].fname, p_file + (i < 0 ? 0 : i));
	*/

	strcpy( drives[drv].fname, fi.fname );
	FLP_TRACE(("flp: filename = \"%s\"\n", drives[drv].fname));

	drives[drv].img_wp = 0;

	i = ops->open(drives + drv, fi.fsize);
	if (i != FLPO_ERR_OK) {
		f_close(&drives[drv].fp);
		return i;
	}

	drives[drv].fs_wp = ( disk_status( 0 ) & STA_PROTECT ) != 0 || ( fi.fattrib & AM_RDO ) != 0 || ( ops->flags & FLP_FLAGS_RO ) != 0 || _FS_READONLY;
	drives[drv].stat &= ~FLP_STAT_DEL;
	drives[drv].ops = ops;
	drives[drv].data_left = 0;

	return FLPO_ERR_OK;
}

void floppy_close(int drv)
{
	if (drives[drv].ops != 0) {
		drives[drv].ops->close(drives + drv);
		drives[drv].ops = 0;
		drives[drv].byte_time = 0;
		f_close(&drives[drv].fp);
	}
}

///////////////////////////////////////////////////////////////////////

static void floppy_init_am(void)
{
	word crc;
	int i;

	crc = crc_init();
	for (i = 0; i < 4; i++) crc = crc_add(crc, sel_drv->am_buf[i]);
	sel_drv->am_crc_hi = (crc >> 8);
	sel_drv->am_crc_lo = (crc >> 0);
	sel_drv->no_data = 0;
	sel_drv->data_left = ((word)0x80) << sel_drv->am_size;
}

void floppy_set_sec_id(byte sec)
{
	if (
	    floppy_fast_mode  &&
	    sel_drv->ops->set_sec_id && sel_drv->ops->set_sec_id(sec)) {

		sel_drv->sec_done = 0;
		sel_drv->stat |= FLP_STAT_AM;
		sel_drv->stat &= ~(FLP_STAT_ERR|FLP_STAT_EOD);
		sb_get = 0;
		sb_put = 6;
		floppy_init_am();
		sel_drv->time = get_ticks();
		// update index
		fdc_set(FDC_INDEX, sel_drv->sec_n != 0);
	}
}

byte floppy_read(void)
{
    byte data = 0;
	if (!sel_drv->no_data) {
		if ((sel_drv->stat & FLP_STAT_AM) != 0) {
			data = sel_drv->am_buf[sb_get];
			sb_get += 1;
			if (sb_get == sb_put) {
				sel_drv->stat &= ~FLP_STAT_AM;
				sb_put = 0;
				sb_get = 0;
			}
		} else {
			if (sb_get == sb_put) {
				if ((sel_drv->stat & FLP_STAT_EOD) != 0) {
					sel_drv->no_data = 1;
					sel_drv->sec_done = floppy_fast_mode;
					goto done;
				}

				BDI_StopTimer();
				if (!sel_drv->ops->read(sbuf)) {
					sel_drv->stat |= (FLP_STAT_ERR|FLP_STAT_EOD);
					sel_drv->no_data = 1;
				}
				BDI_StartTimer();

				sb_get = 0;
				sb_put = FLP_BUF_SIZE;
			}
			data = sbuf[sb_get];
			sb_get += 1;
		}
		FLPH_TRACE(("flp: READ sb_get=%u, sb_put=%u, time=%s\n", sb_get, sb_put, ticks_str(get_ticks())));
	} else {
		FLP_TRACE(("flp: READ while no_data=1\n", sb_get, sb_put, ticks_str(get_ticks())));
	}
	done:
	return data;
}

void floppy_write(byte data)
{
	if (!sel_drv->no_data) {
		FLPH_TRACE(("flp: WRITE, sb_put=%u\n", sb_put));
		if ((sel_drv->stat & FLP_STAT_AM) != 0) {
			// skip AM
			return;
		}
		sbuf[sb_put++] = data;
		if (sb_put == FLP_BUF_SIZE)
		{
            BDI_StopTimer();
			if (!sel_drv->ops->write(sbuf)) {
				sel_drv->stat |= (FLP_STAT_ERR|FLP_STAT_EOD);
			}
			BDI_StartTimer();

			if ((sel_drv->stat & FLP_STAT_EOD) != 0) {
				sel_drv->no_data = 1;
				sel_drv->sec_done = floppy_fast_mode;
			}
			sb_put = 0;
		}
	}
}

word floppy_stat(void)
{
	return sel_drv->stat;
}

static inline dword elapsed(void)
{
	return ( get_ticks() - sel_drv->time );
}

void floppy_dispatch(void)
{
	int sel, ready, hld;
	static int step = 0, index = 0;

	sel = fdc_query(FDC_DRVSEL);
	if (sel_drv != drives + sel || sel_drv->set_trk )
	{
		sel_drv = drives + sel;

		if ( sel_drv->ops != 0 && sel_drv->ops->change != 0 )
		{
			sel_drv->ops->change();
		}

		ready = sel_drv->ops != 0;
		fdc_set(FDC_WP, sel_drv->wp || sel_drv->fs_wp || sel_drv->img_wp);
		fdc_set(FDC_READY, ready);

		index = (sel_drv->sec_n == 0);
		sel_drv->set_trk = 1;

		sel_drv->time = get_ticks();

		FLP_TRACE(("flp: sel=%u, side=%u, ready=%u\n", sel, fdc_query(FDC_SIDE), ready));
	} else {
		ready = sel_drv->ops != 0;
	}

	hld = fdc_query(FDC_HLD);
	if (sel_drv->hld != hld) {
		sel_drv->hld = hld;
		fdc_set(FDC_HLT, hld);
	}

	if (sel_drv->side_n != fdc_query(FDC_SIDE)) {
		sel_drv->side_n = !sel_drv->side_n;
		sel_drv->set_trk = 1;
		//TRACE( ( "SET_SIDE: %x\n", sel_drv->side_n ) );
	}

	if (step != fdc_query(FDC_STEP) && ready) {
		step = !step;
		if (step) {
			FLP_TRACE(("flp: step trk=%u\n", sel_drv->trk_n));
			if (fdc_query(FDC_SDIR)) {
				if (sel_drv->trk_n < (sel_drv->trk_cnt - 1)) {
					sel_drv->trk_n += 1;
					sel_drv->set_trk = 1;
				}
			} else {
				if (sel_drv->trk_n > 0) {
					sel_drv->trk_n -= 1;
					sel_drv->set_trk = 1;
				}
			}
		}
	}

	if (sel_drv->set_trk) {
		fdc_set( FDC_TRK0, sel_drv->trk_n != 0 );
		sel_drv->set_trk = 0;
		if (ready) {
			FLP_TRACE(("flp: set_trk: trk=%u, trk_cnt=%u\n", sel_drv->trk_n, sel_drv->trk_cnt));

			sel_drv->stat &= ~FLP_STAT_AM;
			sel_drv->no_data = 1;
			sel_drv->sec_done = 0;
			sel_drv->trk_ptr = 0;
			sel_drv->ops->set_trk();
		}
	}

	if (hld) {
		if (index /*&& ready */&& elapsed() > (4000 / CNTR_INTERVAL)) {		// no division
	    	FLP_TRACE(("flp: clearing index signal, time=%s\n", ticks_str(get_ticks())));
	    	index = 0;
		}

		if (ready) {
			if (sel_drv->sec_done || elapsed() > sel_drv->sector_time) {
				sel_drv->sec_done = 0;
				sel_drv->sec_n += 1;
				if (sel_drv->sec_n >= sel_drv->sec_cnt) {
					FLP_TRACE(("flp: setting index signal, time=%s\n", ticks_str(get_ticks())));
					sel_drv->sec_n = 0;
					index = 1;
				}
				sel_drv->stat |= FLP_STAT_AM;
				sel_drv->stat &= ~(FLP_STAT_ERR|FLP_STAT_EOD);
				sb_get = 0;
				sb_put = 6;
				if (sel_drv->ops->set_sec()) {
					floppy_init_am();
				} else {
					sel_drv->stat |= (FLP_STAT_ERR|FLP_STAT_EOD);
					sel_drv->no_data = 1;
				}
				sel_drv->time = get_ticks();
			}
			// clear am signal
			if (elapsed() > sel_drv->am_time && (sel_drv->stat & FLP_STAT_AM) != 0) {
				FLP_TRACE(("flp: clearing AM flag, time=%s\n", ticks_str(get_ticks())));
				sel_drv->stat &= ~FLP_STAT_AM;
				sb_get = 0;
				sb_put = 0;
			}
		}
	}
	fdc_set(FDC_INDEX, ready ? index : 1);
}

///////////////////////////////////////////////////////////////////////

int floppy_disk_wp(int drive, int *protect)
{
	if (protect) {
		drives[drive].wp = *protect != 0;
	}
	return drives[drive].wp;
}

int floppy_set_all_wp(int mask)
{
    int i;
    for (i = 0; i < MAX_FLOPPIES; i++) {
    	drives[i].wp = (mask & (1 << i)) != 0;
    }
	return 0;
}

int floppy_disk_loaded(int drive)
{
	return drives[drive].ops != 0;
}

word floppy_byte_time(void)
{
	return sel_drv->byte_time;
}

int floppy_leds(void)
{
	int leds = sel_drv->hld << (sel_drv - drives) & (sel_drv->ops ? 0xff : 0);
    leds = ( ( leds & 0x01 ) ) | ( ( leds & 0x02 ) << 1 ) | ( ( leds & 0x04 ) >> 1 );
    return leds;
}

///////////////////////////////////////////////////////////////////////

void floppy_std_params(struct flp_image *img)
{
	img->sec_cnt = STD_SEC_CNT;
	img->am_size = STD_SEC_SZ_ID;
	img->trk_sz = STD_SEC_CNT * STD_SEC_SIZE;
	floppy_set_times(img);
}

void floppy_set_times(struct flp_image *img)
{
	img->byte_time = img->trk_sz ? (((1000000 / CNTR_INTERVAL) / img->trk_sz) / img->rps) : 0;		// division
	img->sector_time = img->sec_cnt ? (((1000000 / CNTR_INTERVAL) / img->sec_cnt) / img->rps) : 0;  // division
	img->am_time = img->byte_time * FLP_AM_CONSTANT;
}

///////////////////////////////////////////////////////////////////////

void floppy_init(void)
{
	int i;
	for (i = 0; i < MAX_FLOPPIES; i++) {
		drives[i].flp_num = i;
  		drives[i].ops = 0;
		drives[i].trk_ptr = 0;
    	drives[i].set_trk = 0;
    	drives[i].hld = 0;
    	drives[i].rps = FLOPPY_RPS;
    	drives[i].interleave = 1;
	}
	sel_drv = drives;
}

void floppy_set_fast_mode(int fast)
{
	floppy_fast_mode = fast;
}

///////////////////////////////////////////////////////////////////////
/*
byte floppy_sec(struct flp_image *img)
{
	if (sel_drv->interleave) {
		static PROGMEM char interleave[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
		return interleave[sel_drv->sec_n];
		// 0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14
		// 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15
		// Межтрековый (трековый) интерлив определяет относительное расположение секторов
		// на соседних физических дорожках. На дисках, отформатированных с помощью TR-DOS и DCU,
		// он равен 0 - это означает, что сектора на всех дорожках располагаются одинаково.
		// В некоторых дисковых утилитах есть возможность установить ненулевой межтрековый интерлив.
		// Например, при межтрековом интерливе, равном 5, сектора на соседних физических дорожках располагаются так:
   		// 1. 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
   		// 2. 12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11
   		// 3. 7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6
   		// Оригинальный TR-DOS содержит задержку при переходе на соседнюю физическую дорожку.
   		// Эта задержка приблизительно равна по времени 7 секторам. В результате диск,
   		// отформатированный с таким межтрековым интерливом, будет читаться на старых TR-DOS быстрее -
   		// не будет теряться оборот диска при переходе на следующую дорожку.
   		// Однако TR-DOS 5.1xf и 6.xxE содержат задержку только при записи
   		// (её отсутствие чревато неправильной записью, т.к. головка при позиционировании дрожит),
   		// и такой большой межтрековый интерлив при чтении в этих версиях TR-DOS не требуется.

		//sel_drv->am_sec = sel_drv->sec_n + 1;
	} else {
		return sel_drv->sec_n;
//		sel_drv->am_sec = sel_drv->sec_n + 1;
	}
}
*/
///////////////////////////////////////////////////////////////////////
