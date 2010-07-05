/*
 *  This file is a part of spec2009 project.
 *
 *  SCL-image support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "flpimg.h"

#if SCL_TRACE_FLAG
#define SCL_TRACE(x)		TRACE(x)
#else
#define SCL_TRACE(x)
#endif

///////////////////////////////////////////////////////////////////////

#define FDSC_SIZE			16
#define SYS_SEC_AREA		0xe0

struct sys_sec_area {
	byte unused_0;
	byte first_sec;
	byte first_trk;
	byte dtype;
	byte nfiles;
	byte free_sec_lo;
	byte free_sec_hi;
	byte trdos;
	byte unused_1[2];
	byte unused_2[9];
	byte unused_3[1];
	byte erased;
	byte label[8];
	byte unused_4[3];
} PACKED;

static
PROGMEM
struct sys_sec_area scl_sys_sec_def = {
	0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, { 32, 32, 32, 32, 32, 32, 32, 32, 32 }, { 0 }, 0, { 32, 32, 32, 32, 32, 32, 32, 32 }, { 0, 0, 0 }
};


struct scl_priv {
	byte num_files;

	word data;
	dword data_end;

	word cat_sec[8];
	struct sys_sec_area sys_sec;
};

///////////////////////////////////////////////////////////////////////

static int scl_open(struct flp_image *img, dword size)
{
	byte sig[8], nfiles;
	int i;
	word last_sec;
	struct sys_sec_area *sys_area;

	if (!read_file(&img->fp, sig, 8)) {
   		SCL_TRACE(("scl: failed to read signature\n"));
		return FLPO_ERR_READ;
   	}

	if (memcmp(sig, "SINCLAIR", 8)) {
		SCL_TRACE(("scl: invalid signature, %.8s\n", sig));
		return FLPO_ERR_FORMAT;
	}

	if (!read_le_byte(&img->fp, &nfiles)) {
   		SCL_TRACE(("scl: failed to read number of files\n"));
		return FLPO_ERR_READ;
	}
	if (nfiles > 128) {
   		SCL_TRACE(("scl: number of files >128\n"));
		return FLPO_ERR_FORMAT;
	}
	last_sec = STD_SEC_CNT;
	for (i = 0; i < nfiles; i++) {
		byte file_sz;

		f_lseek(&img->fp, 9 + i * 14+13);
		if (!read_le_byte(&img->fp, &file_sz)) {
   			SCL_TRACE(("scl: failed to read header\n"));
			return FLPO_ERR_READ;
		}
		if (i % (STD_SEC_SIZE / FDSC_SIZE) == 0) {					// no division
			FLP_PRIV(img, struct scl_priv)->cat_sec[i / (STD_SEC_SIZE / FDSC_SIZE)] = last_sec;	// no division
		}
		last_sec += file_sz;
	}
	if ( last_sec > STD_SEC_CNT * STD_TRK_CNT * STD_SIDE_CNT) {
   		SCL_TRACE(("scl: image to large\n"));
		return FLPO_ERR_FORMAT;
	}
	i = strchr(img->fname, '.') - img->fname;
	if (i > 8) {
		i = 8;
	}

	SCL_TRACE(("scl: image to open %s\n", img->fname[i]));

	sys_area = &FLP_PRIV(img, struct scl_priv)->sys_sec;
	memcpy(sys_area, &scl_sys_sec_def, 32);
	for (; i >= 0; i--) {
		sys_area->label[i] = toupper((byte)img->fname[i]);
	}
	sys_area->first_sec = last_sec % STD_SEC_CNT;
	sys_area->first_trk = last_sec / STD_SEC_CNT;		// no division
	sys_area->dtype = 0x16;
	sys_area->nfiles = nfiles;
	sys_area->free_sec_lo = (byte)(((STD_SEC_CNT * STD_TRK_CNT * STD_SIDE_CNT) - last_sec) & 0xff);
	sys_area->free_sec_hi = (byte)(((STD_SEC_CNT * STD_TRK_CNT * STD_SIDE_CNT) - last_sec) >> 8);
	sys_area->trdos = 0x10;

	img->side_cnt = STD_SIDE_CNT;
	img->trk_cnt = STD_TRK_CNT;

	floppy_std_params(img);

	FLP_PRIV(img, struct scl_priv)->data = nfiles * 14 + 9;
	FLP_PRIV(img, struct scl_priv)->num_files = nfiles;
	FLP_PRIV(img, struct scl_priv)->data_end = nfiles * 14 + 9 + ( ( last_sec - STD_SEC_CNT ) * STD_SEC_SIZE );

	return FLPO_ERR_OK;
}

static void scl_close(struct flp_image *img)
{
}

///////////////////////////////////////////////////////////////////////

static int scl_read(byte *buf)				// return 1 on ok
{
	word rd_size;

	if (sel_drv->trk_ptr == 0) {
		word offset;

		offset = STD_SEC_SIZE - sel_drv->data_left;
		rd_size = op_size();
		if (sel_drv->am_sec == 9) {
			// system sector
			SCL_TRACE(("scl: sys sector, offset=%x\n", offset));
			if (offset < SYS_SEC_AREA) {
				goto zero_blk;
			}
			SCL_TRACE(("scl: copy sys sec area\n"));
			memcpy(buf, ((byte *)&FLP_PRIV(sel_drv, struct scl_priv)->sys_sec) + (offset - SYS_SEC_AREA), rd_size);
		} else if (sel_drv->am_sec <= 8) {
			byte prev_sec_cnt, nfile;

			SCL_TRACE(("scl: cat_sec_id=%u, off=%x\n", sel_drv->sec_n, FLP_PRIV(sel_drv, struct scl_priv)->cat_sec[sel_drv->sec_n]));

			nfile = (sel_drv->sec_n * 16) + (offset / 16);					// no division
			prev_sec_cnt = buf[13];
			if (nfile >= FLP_PRIV(sel_drv, struct scl_priv)->num_files) {
				goto zero_blk;
			}
			f_lseek(&sel_drv->fp, 9 + nfile * 14);
			if (!read_file(&sel_drv->fp, buf, 14)) {
				return 0;
			}
			SCL_TRACE(("scl: prev_fname=\"%.9s\"\n", buf));
			SCL_TRACE(("scl: prev_sec=%x, prev_trk=%x, sec_cnt=%x\n", offset ? buf[14] : 0xff, offset ? buf[15] : 0xff, prev_sec_cnt));
			offset = offset ? ((word)buf[14]) + ((word)buf[15]) * ((word)STD_SEC_CNT) + ((word)prev_sec_cnt) : FLP_PRIV(sel_drv, struct scl_priv)->cat_sec[sel_drv->sec_n];
			SCL_TRACE(("scl: offset=%x\n", offset));
			buf[14] = offset % STD_SEC_CNT;
			buf[15] = offset / STD_SEC_CNT;	// no division
		} else {
			zero_blk:
			memset(buf, 0, rd_size);
		}
		return 1;
	}
	else
	{
		rd_size = op_size();
		if( rd_size == 0 ) return 0;

        if( sel_drv->fp.fptr + rd_size <= sel_drv->fp.fsize )
        {
            return read_file(&sel_drv->fp, buf, rd_size);
        }
        else
        {
            memset( buf, 0, rd_size );
            return 1;
        }
	}
}

static int scl_write(byte *buf)			// return 1 on ok
{
	return 0;
}

///////////////////////////////////////////////////////////////////////

static int set_trk(void)
{
	sel_drv->am_trk = sel_drv->trk_n;

	if (sel_drv->am_trk || !sel_drv->side_n)
	{
	    sel_drv->trk_ptr = sel_drv->trk_n * STD_SIDE_CNT;
		if( !sel_drv->side_n ) sel_drv->trk_ptr++;
		sel_drv->trk_ptr = FLP_PRIV(sel_drv, struct scl_priv)->data + ( sel_drv->trk_ptr - 1 ) * STD_SEC_CNT * STD_SEC_SIZE;

		//sel_drv->trk_ptr = ((((dword)STD_SIDE_CNT) * ((dword)sel_drv->trk_n) + ((dword)(!sel_drv->side_n)) - (dword)1) *
	}

	sel_drv->am_side = !sel_drv->side_n;

	SCL_TRACE(("scl: set_trk: trk=%x, side_n = %x, time=%s\n", sel_drv->trk_n, sel_drv->side_n, ticks_str(get_ticks())));

	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_sec(void)
{
    dword off;
	sel_drv->am_sec = sel_drv->sec_n + 1;

	off = sel_drv->trk_ptr + ((dword)sel_drv->sec_n) * ((dword)STD_SEC_SIZE);
	if (off < FLP_PRIV(sel_drv, struct scl_priv)->data_end) {
		if (sel_drv->trk_ptr) {
			f_lseek(&sel_drv->fp, off);
			SCL_TRACE(("scl: set_sec: sector=%u, data off=%u, time=%s\n", sel_drv->am_sec, off, ticks_str(get_ticks())));
		} else {
			SCL_TRACE(("scl: set_sec: system trk sec %u, time=%s\n", sel_drv->am_sec, ticks_str(get_ticks())));
		}
		return 1;
	} else {
		SCL_TRACE(("scl: set_sec beyod file_data: off=%x, sz=%x\n", off, FLP_PRIV(sel_drv, struct scl_priv)->data_end));
	}
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
struct flp_ops scl_ops = {
	flags: FLP_FLAGS_RO,
	open: scl_open,
	close: scl_close,
	read: scl_read,
	write: scl_write,
	set_trk: set_trk,
	set_sec: set_sec,
	set_sec_id: set_sec_id,
	change: 0,
};

///////////////////////////////////////////////////////////////////////
