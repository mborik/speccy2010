/*
 *  This file is a part of spec2009 project.
 *
 *  TRD-image support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "flpimg.h"

#if TRD_TRACE_FLAG
#define TRD_TRACE(x)		TRACE(x)
#else
#define TRD_TRACE(x)
#endif

#if !TRD_ON_ODI

///////////////////////////////////////////////////////////////////////

static int trd_open(struct flp_image *img, dword size)
{
	byte trd_id[5];

	if (f_lseek(&img->fp, 256 * 8 + 227) != FR_OK) {
		return FLPO_ERR_READ;
	}
	if (!read_file(&img->fp, trd_id, sizeof(trd_id))) {
   		return FLPO_ERR_READ;
   	}

	TRD_TRACE(("trd: dtype=%x, trd=%x\n", trd_id[0], trd_id[4]));
	if( trd_id[4] == 0x10 && trd_id[0] >= 0x16 && trd_id[0] <= 0x19 )
	{
	    img->side_cnt = trd_id[0] & 0x08 ? 1 : 2;
	}
	else
	{
	    img->side_cnt = 2;
	}

	img->trk_cnt = ( size + img->side_cnt * STD_SEC_CNT * STD_SEC_SIZE - 1 ) / ( img->side_cnt * STD_SEC_CNT * STD_SEC_SIZE );

	floppy_std_params(img);

	return FLPO_ERR_OK;
}

static void trd_close(struct flp_image *img)
{
}

///////////////////////////////////////////////////////////////////////

static int trd_read(byte *buf)				// return 1 on ok
{
	word rd_size;
	rd_size = op_size();

	return rd_size && read_file(&sel_drv->fp, buf, rd_size);
}

static int trd_write(byte *buf)			// return 1 on ok
{
	word wr_size;

	wr_size = op_size();
#if STD_SEC_SIZE % FLP_BUF_SIZE
	if (wr_size) {
		byte i;
		if (!write_file(&sel_drv->fp, buf, wr_size)) {
			return 0;
		}
		for (i = 0; i < (FLP_BUF_SIZE - wr_size); i++) {
			buf[i] = buf[i + wr_size];
		}
	}
#else
	return wr_size && write_file(&sel_drv->fp, buf, wr_size);
#endif

	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_trk(void)
{
	sel_drv->am_trk = sel_drv->trk_n;
	sel_drv->trk_ptr = ((((dword)sel_drv->side_cnt) * ((dword)sel_drv->trk_n) + ((dword)!sel_drv->side_n)) * ((dword)sel_drv->sec_cnt)) * ((dword)STD_SEC_SIZE);
	sel_drv->am_side = !sel_drv->side_n;
	TRD_TRACE(("trd: set_trk: trk=%x, time=%s\n", sel_drv->trk_ptr, ticks_str(get_ticks())));

	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_sec(void)
{
	sel_drv->am_sec = sel_drv->sec_n + 1;
	f_lseek(&sel_drv->fp, sel_drv->trk_ptr + ((dword)sel_drv->sec_n) * ((dword)STD_SEC_SIZE));
	TRD_TRACE(("trd: set_sector: sector=%u, data off=%u, time=%s\n", sel_drv->am_sec, sel_drv->trk_ptr + sel_drv->sec_n * STD_SEC_SIZE, ticks_str(get_ticks())));

	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_sec_id(byte sec)
{
    if (sec && sec <= STD_SEC_CNT) {
    	sel_drv->sec_n = sec - 1;
    	return set_sec();
    }
	return 0;
}

///////////////////////////////////////////////////////////////////////

PROGMEM
struct flp_ops trd_ops = {
	flags: 0,
	open: trd_open,
	close: trd_close,
	read: trd_read,
	write: trd_write,
	set_trk: set_trk,
	set_sec: set_sec,
	set_sec_id: set_sec_id,
	change: 0,
};

///////////////////////////////////////////////////////////////////////

#endif /* !TRD_ON_ODI */
