/*
 *  This file is a part of spec2009 project.
 *
 *  FDI-image support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "flpimg.h"

#if FDI_TRACE_FLAG
#define FDI_TRACE(x)		TRACE(x)
#else
#define FDI_TRACE(x)
#endif

///////////////////////////////////////////////////////////////////////

#define SEC_CACHE_SIZE		32
#define TRK_CACHE_SIZE		200	// 90 tracks * 2 sides

///////////////////////////////////////////////////////////////////////

struct fdi_priv {
	word data;
	word trk;
	word trk_head;
	word trk_cnt;						// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	byte sec_id[SEC_CACHE_SIZE];
	byte sec_sz[SEC_CACHE_SIZE / 4];
	byte sec_res[SEC_CACHE_SIZE / 2];
};

static byte fdi_active_drv = 0xff;
static byte fdi_reload_trk = 1;
static word fdi_trk_cache[TRK_CACHE_SIZE];

///////////////////////////////////////////////////////////////////////

static int fdi_open(struct flp_image *img, dword size)
{
	byte sig[4];
	word data_offset = 0;
	word trk_offset = 0;
	word max_sec, cur_off = 0;
	word trk_cnt = 0, side_cnt = 0;
	dword total_trk_cnt;
	int i;
	byte img_wp;

	if (!read_file(&img->fp, sig, 3)) {
		FDI_TRACE(("fdi: failed to read signature\n"));
		return FLPO_ERR_READ;
	}
#if defined(PC_EMULATOR) || defined(PIC_CTRL)
	sig[3] = 0;
	if ((*(dword *)sig) != 0x00494446 /* FDI */) {
#else
	if (!((sig[0] == 'F') && (sig[1] == 'D') && (sig[2] == 'I'))) {
#endif
		FDI_TRACE(("fdi: invalid signature, %.3s\n", sig));
		return FLPO_ERR_FORMAT;
	}
	if (!read_le_byte(&img->fp, &img_wp)) {
		return FLPO_ERR_READ;
	}
	if (!read_le_word(&img->fp, &trk_cnt)) {
		return FLPO_ERR_READ;
	}
	if (!read_le_word(&img->fp, &side_cnt) || side_cnt > 2) {
		return FLPO_ERR_READ;
	}
	f_lseek(&img->fp, 0xa);


	if (!read_le_word(&img->fp, &data_offset)) {
		return FLPO_ERR_READ;
	}
	if (!read_le_word(&img->fp, &trk_offset)) {
		return FLPO_ERR_READ;
	}

	cur_off = trk_offset + 0xe;
	for (i = 0, max_sec = 0; i < (word)trk_cnt * (word)side_cnt; i++) {
		byte nsecs;
		f_lseek(&img->fp, cur_off + 6);
		if (!read_le_byte(&img->fp, &nsecs)) {
			return FLPO_ERR_READ;
		}
		if (nsecs > max_sec) {
			max_sec = nsecs;
		}
		cur_off += 7 * (1 + nsecs);
	}
   	if (max_sec > SEC_CACHE_SIZE) {
		FDI_TRACE(("fdi: found too long trk max_sec=%u\n", max_sec));
		return FLPO_ERR_FORMAT;
	}

	img->side_cnt = side_cnt;
	img->trk_cnt = trk_cnt;
	floppy_std_params(img);

	FLP_PRIV(img, struct fdi_priv)->data = data_offset;
	FLP_PRIV(img, struct fdi_priv)->trk =
	FLP_PRIV(img, struct fdi_priv)->trk_head = trk_offset + 0xe;
	total_trk_cnt = side_cnt * trk_cnt;
	if (total_trk_cnt > TRK_CACHE_SIZE) {
		total_trk_cnt = 0;
	}
	FLP_PRIV(img, struct fdi_priv)->trk_cnt = total_trk_cnt;

	img->img_wp = img_wp != 0;

	if (fdi_active_drv == img->flp_num) {
		fdi_reload_trk = 1;
	}
	img->rps = 1;

	return 0;
}

static void fdi_close(struct flp_image *img)
{
}

static void change(void)
{
	if (fdi_active_drv != sel_drv->flp_num) {
		if (FLP_PRIV(sel_drv, struct fdi_priv)->trk_cnt) {
			fdi_reload_trk = 1;
			fdi_active_drv = sel_drv->flp_num;
		}
	}
}

///////////////////////////////////////////////////////////////////////

static int fdi_read(byte *buf)				// return 1 on ok
{
	word rd_size;
	rd_size = op_size();

	FDI_TRACE(("fdi: reading %u, time=%s\n", rd_size, ticks_str(get_ticks())));
	return rd_size && read_file(&sel_drv->fp, buf, rd_size);
}

static int fdi_write(byte *buf)			// return 1 on ok
{
	word wr_size;

	wr_size = op_size();
	if (wr_size) {
		if (!write_file(&sel_drv->fp, buf, wr_size)) {
			return 0;
		}
	}
	if ((sel_drv->stat & FLP_STAT_EOD) != 0) {
		byte crc_val;
		crc_val = 1 << (sel_drv->am_size);
		FDI_TRACE(("fdi: fixing sector %u crc to %x at %x\n", sel_drv->am_sec, crc_val, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head + (sel_drv->sec_n + 1) * 7 + 4));
		f_lseek(&sel_drv->fp, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head + (sel_drv->sec_n + 1) * 7 + 4);
		write_file(&sel_drv->fp, &crc_val, 1);
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_trk(void)
{
	word off;
	byte trk = 0, sec_cnt, side, i;
	byte found = 0;
	word trk_i;

	if (FLP_PRIV(sel_drv, struct fdi_priv)->trk_cnt) {
		if (fdi_reload_trk) {
			FDI_TRACE(("fdi: reloading trk cache\n"));
			off = FLP_PRIV(sel_drv, struct fdi_priv)->trk;
			for (trk = 0, trk_i = 0; trk < sel_drv->trk_cnt; trk++) {
				for (side = 0; side < sel_drv->side_cnt; side++) {
					f_lseek(&sel_drv->fp, off + 6);
					FDI_TRACE(("fdi: trk_head: trk=%u, side=%u, off=%u\n", trk, side, off));
					if (!read_le_byte(&sel_drv->fp, &sec_cnt)) {
						return 0;
					}
					fdi_trk_cache[trk_i] = off;
					off += 7 + 7 * (word)sec_cnt;
					trk_i += 1;
				}
			}
			fdi_reload_trk = 0;
		}
		trk_i = (sel_drv->trk_n * sel_drv->side_cnt) + (!sel_drv->side_n);
		off = fdi_trk_cache[trk_i];
		f_lseek(&sel_drv->fp, off + 6);
		if (!read_le_byte(&sel_drv->fp, &sec_cnt)) {
			return 0;
		}
	} else {
   		off = FLP_PRIV(sel_drv, struct fdi_priv)->trk;
		for (trk = 0; trk < sel_drv->trk_cnt; trk++) {
			for (side = 0; side < sel_drv->side_cnt; side++) {
				f_lseek(&sel_drv->fp, off + 6);
				FDI_TRACE(("fdi: trk_head: trk=%u, side=%u, off=%u\n", trk, side, off));
				if (!read_le_byte(&sel_drv->fp, &sec_cnt)) {
					return 0;
				}
				if (trk == sel_drv->trk_n && side == !sel_drv->side_n) {
					found = 1;
					break;
				}
				off += 7 + 7 * (word)sec_cnt;
			}
			if (found) {
				break;
			}
		}
		if (!found) {
   			return 0;
   		}
	}

   	FLP_PRIV(sel_drv, struct fdi_priv)->trk_head = off;
   	sel_drv->sec_cnt = sec_cnt;

	f_lseek(&sel_drv->fp, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head);

	read_le_dword(&sel_drv->fp, &sel_drv->trk_ptr);
	sel_drv->trk_ptr += FLP_PRIV(sel_drv, struct fdi_priv)->data;

	sel_drv->trk_sz = 0;

	f_lseek(&sel_drv->fp, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head + 7);
	for (i = 0; i < sec_cnt; i++) {
		byte s[7], shift;
		if (!read_file(&sel_drv->fp, s, sizeof(s))) {
			return 0;
		}
		FLP_PRIV(sel_drv, struct fdi_priv)->sec_id[i] = s[2];
		shift = (i & 3) << 1;
		FLP_PRIV(sel_drv, struct fdi_priv)->sec_sz[i >> 2] = (FLP_PRIV(sel_drv, struct fdi_priv)->sec_sz[i >> 2] & ~(3 << shift)) | ((s[3] & 3) << shift);
		sel_drv->am_trk = s[0];
		sel_drv->am_side = s[1];
		sel_drv->trk_sz += 0x80 << (s[3] & 3);
	}
	floppy_set_times(sel_drv);
	FDI_TRACE(("fdi: set_trk: trk=%u, head_off=%u, data_off=%x, sec_cnt=%u, side=%u, trk_sz=%u, time=%s\n", trk, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head, sel_drv->trk_ptr, sec_cnt, !sel_drv->side_n, sel_drv->trk_sz, ticks_str(get_ticks())));
	return 0;
}

///////////////////////////////////////////////////////////////////////

static int set_sec(void)
{
	word off = 0;
	byte sec_id, sec_res;
	sec_id = sel_drv->sec_n;

	if (sel_drv->trk_ptr == 0) {
		return 0;
	}

	f_lseek(&sel_drv->fp, FLP_PRIV(sel_drv, struct fdi_priv)->trk_head + (sec_id + 1) * 7 + 4);
	if (!read_le_byte(&sel_drv->fp, &sec_res)) {
		FDI_TRACE(("fdi: set_sec: read_le_byte failed\n"));
		return 0;
	}
	if (!read_le_word(&sel_drv->fp, &off)) {
		FDI_TRACE(("fdi: set_sec: read_le_word failed\n"));
		return 0;
	}
	sel_drv->am_size = (FLP_PRIV(sel_drv, struct fdi_priv)->sec_sz[sec_id >> 2] >> ((sec_id & 3) << 1)) & 3;
	sel_drv->am_sec = FLP_PRIV(sel_drv, struct fdi_priv)->sec_id[sec_id];
	f_lseek(&sel_drv->fp, sel_drv->trk_ptr + off);
	if ((sec_res & (1 << sel_drv->am_size)) == 0) {
		sel_drv->stat |= FLP_STAT_ERR;
	}
	floppy_set_times(sel_drv);
	FDI_TRACE(("fdi: set_sec: sector=%u, data off=%u,0x%x (r=%x,off=%x), time=%s\n", sel_drv->sec_n, sel_drv->trk_ptr + off, sel_drv->trk_ptr + off, sec_res, off, ticks_str(get_ticks())));
	return 1;
}

///////////////////////////////////////////////////////////////////////

static int set_sec_id(byte sec)
{
	int i;
	for (i = 0; i < sel_drv->sec_cnt; i++) {
		if (sec == FLP_PRIV(sel_drv, struct fdi_priv)->sec_id[i]) {
			sel_drv->sec_n = i;
			return set_sec();
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////

PROGMEM
struct flp_ops fdi_ops = {
	flags: 0,
	open: fdi_open,
	close: fdi_close,
	read: fdi_read,
	write: fdi_write,
	set_trk: set_trk,
	set_sec: set_sec,
	set_sec_id: set_sec_id,
	change: change,
};

///////////////////////////////////////////////////////////////////////
