/*
 *  This file is a part of spec2009 project.
 *
 *  ODI-image support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "flpimg.h"

#if ODI_TRACE_FLAG
#define ODI_TRACE(x)		TRACE(x)
#else
#define ODI_TRACE(x)
#endif

#define ALL_IMAGES			0

///////////////////////////////////////////////////////////////////////


static int odi_open(struct flp_image *img, dword size)
{
	switch (size) {
		case 839680:
			// fdd
    		img->side_cnt = 2;
    		img->trk_cnt = 82;
			img->sec_cnt = 5;
			img->am_size = FSZID_1024;
			break;
		case 819200:
			// odi
    		img->side_cnt = 2;
    		img->trk_cnt = 80;
			img->sec_cnt = 5;
			img->am_size = FSZID_1024;
			break;
		case 1740800:
			// od2
    		img->side_cnt = 2;
    		img->trk_cnt = 85;
			img->sec_cnt = 5;
			img->am_size = FSZID_1024;
			break;
		case 655360:
			// trd
    		img->side_cnt = 2;
    		img->trk_cnt = 80;
			img->sec_cnt = 16;
			img->am_size = FSZID_256;
			break;
		case 368640:
			// dsk // msx cpm disk
    		img->side_cnt = 2;
    		img->trk_cnt = 80;
			img->sec_cnt = 9;
			img->am_size = FSZID_512;
			break;
		default:
			return FLPO_ERR_FORMAT;
	}
	
	img->trk_sz = (((word)128) << img->am_size) * ((word)img->sec_cnt);
	floppy_set_times(img);

	return FLPO_ERR_OK;
}

static void odi_close(struct flp_image *img)
{
}

///////////////////////////////////////////////////////////////////////

static int odi_read(byte *buf)				// return 1 on ok
{
	word rd_size;
	rd_size = op_size();

	return rd_size && read_file(&sel_drv->fp, buf, rd_size);
}

static int odi_write(byte *buf)			// return 1 on ok
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
	sel_drv->trk_ptr = ((((dword)sel_drv->side_cnt) * ((dword)sel_drv->trk_n) + ((dword)!sel_drv->side_n)) * ((dword)sel_drv->sec_cnt)) * (((dword)128) << sel_drv->am_size);
	sel_drv->am_side = !sel_drv->side_n;
	ODI_TRACE(("trd: set_trk: trk=%x, time=%s\n", sel_drv->trk_ptr, ticks_str(get_ticks())));

	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_sec(void)
{
	sel_drv->am_sec = sel_drv->sec_n + 1;
	f_lseek(&sel_drv->fp, sel_drv->trk_ptr + ((dword)sel_drv->sec_n) * (((dword)128) << sel_drv->am_size));
	ODI_TRACE(("trd: set_sector: sector=%u, data off=%u, time=%s\n", sel_drv->am_sec, sel_drv->trk_ptr + sel_drv->sec_n * (((dword)128) << sel_drv->am_size), ticks_str(get_ticks())));

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
struct flp_ops odi_ops = {
	flags: 0,
	open: odi_open,
	close: odi_close,
	read: odi_read,
	write: odi_write,
	set_trk: set_trk,
	set_sec: set_sec,
	set_sec_id: set_sec_id,
	change: 0,
};

///////////////////////////////////////////////////////////////////////

