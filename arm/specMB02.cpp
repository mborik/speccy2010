#include <stdarg.h>
#include <ctype.h>

#include "system.h"
#include "utils/crc16.h"
#include "specMB02.h"
#include "specConfig.h"

#define RDCRC_TYPEI 0

#define DRAW_SEEK(x)
#define TRACE(x) // __TRACE x

#define SYS_NUM_OF_TRYOUTS 64
#define SEC_BUF_SIZE       16

#define MB02_DRIVES_COUNT  4

#define MB02_PORT_CMD      0x0f
#define MB02_PORT_STA      0x0f
#define MB02_PORT_CYL      0x2f
#define MB02_PORT_SEC      0x4f
#define MB02_PORT_DTA      0x6f

#define SYS_STAT_MSK       0xc0  // status mask
#define SYS_INTRQ          0x80  // set when wd1793 is completed command
#define SYS_DRQ            0x40  // set when wd1793 is waiting for data
#define SYS_DRV_MSK        0x03  // drive selector

#define SYS_SIDE           0x10  // 1 - zero side, 0 - first side
#define SYS_HLD_EN         0x08
#define SYS_RST            0x04  // transition to 0 then to 1 is caused wd93 reset

#define STAT_NOTRDY        0x80  // drive not ready
#define STAT_WP            0x40  // diskette is write protected
#define STAT_WRERR         0x20  // write error
#define STAT_RECT          0x20
#define STAT_HEAD_ON       0x20  // head is on
#define STAT_NOTFOUND      0x10  // addr marker not found on track
#define STAT_RDCRC         0x08
#define STAT_HEAD_CYL_0    0x04  // head on cyl 0
#define STAT_LOST          0x04  // data underrun
#define STAT_DRQ           0x02  // data request for read/write
#define STAT_INDEX         0x02  // set on index marker
#define STAT_BUSY          0x01  // operation in progress


#define CMD_RESTORE        0x00
#define CMD_SEEK           0x10
#define CMD_STEP_H0        0x20
#define CMD_STEP_H1        0x30
#define CMD_STEP_OUT_H0    0x40
#define CMD_STEP_OUT_H1    0x50
#define CMD_STEP_IN_H0     0x60
#define CMD_STEP_IN_H1     0x70
#define CMD_RD_SEC         0x80
#define CMD_RD_SEC_M       0x90
#define CMD_WR_SEC         0xa0
#define CMD_WR_SEC_M       0xb0
#define CMD_RD_AM          0xc0
#define CMD_RD_TRK         0xe0
#define CMD_WR_TRK         0xf0
#define CMD_INTERRUPT      0xd0

#define TYPEI_CMD_1        0
#define TYPEI_CMD_2        1
#define TYPEII_CMD         2
#define TYPEIII_CMD        3

#define TYPEI_BIT_U        0x10
#define TYPEI_BIT_H        0x08
#define TYPEI_BIT_V        0x04

#define TYPEII_BIT_M       0x10
#define TYPEII_BIT_S       0x08
#define TYPEII_BIT_E       0x04
#define TYPEII_BIT_C       0x02

#define TYPEIII_BIT_E      0x04

#if RDCRC_TYPEI
#define CLEAR_STAT()       do { mb02_state.stat_val &= ~(STAT_DRQ|STAT_LOST|STAT_NOTFOUND|STAT_WP|STAT_WRERR|STAT_BUSY|STAT_NOTRDY); } while (0)
#else
#define CLEAR_STAT()       do { mb02_state.stat_val = 0; } while (0);
#endif

#define SET_STAT_BUSY()    do { mb02_state.stat_val |= (STAT_DRQ|STAT_BUSY); } while (0)
#define SET_STAT_READY()   do { mb02_state.stat_val &= ~(STAT_DRQ|STAT_BUSY); } while (0)
#define SET_SYS_READY()    do { mb02_state.sys_stat = SYS_INTRQ; } while (0)
#define SET_SYS_BUSY()     do { mb02_state.sys_stat = SYS_DRQ;  } while (0)
#define SET_SYS_ACK()      do { mb02_state.sys_stat |= SYS_INTRQ; } while (0)

enum {
	DISK_NONE = 0,
	DISK_MBD,
};

enum {
	MB02_IDLE = 0,
	MB02_READ,
	MB02_READ_TRK,
	MB02_READ_ADR,
	MB02_WRITE,
	MB02_WRITE_TRK,
	MB02_SEEK,
};


struct mb02_drive {
	FIL fd;

	unsigned char head_on : 1;
	unsigned char wp : 1;
	unsigned char end_of_sec : 1;

	byte img_fmt;
	byte sec_cnt;
	byte cyl_cnt;
	byte side_cnt;
	word sec_size;
	byte sec_sz_id; // 0 - 128, 1 - 256, 2 - 512, 3 - 1024
	byte cur_cyl;
	byte cur_sec;

	byte cur_sec_id;
	byte sec_res; // sector crc status

	dword img_trkoff; // offset of the track from file begin
	word sec_off; // offset from begin of track
	word next_sec_off;

	byte sec_buff[SEC_BUF_SIZE];
	byte sec_buff_pos;
	byte sec_buff_fill;
};

struct mb02_drive drives[MB02_DRIVES_COUNT];
struct mb02_drive *cur_drv;

// state & wd1793 registers
struct mb02_state {
	byte sys_val;
	byte sys_stat;
	byte cmd_reg;
	int step_dir;
	int cyl_reg;
	byte sec_reg;
	byte stat_val;
	byte data_reg;
	byte cur_side;
	word sys_stat_reads;
	byte mb02_state;
};

struct mb02_state mb02_state;

void mb02_dsk_init();
void mb02_init_drive(byte drv_id);
void clear_drive(byte drv_id);

//---------------------------------------------------------------------------------------
byte read_file(FIL *f, byte *dst, byte sz)
{
	UINT nr;
	if (f_read(f, dst, sz, &nr) != FR_OK || nr != sz) {
		return 0;
	}
	return (byte)nr;
}

byte write_file(FIL *f, byte *dst, byte sz)
{
	UINT nw;
	if (f_write(f, dst, sz, &nw) != FR_OK || nw != sz) {
		return 0;
	}
	return (byte)nw;
}

byte read_le_byte(FIL *f, byte *dst)
{
	return read_file(f, dst, 1);
}

byte read_le_word(FIL *f, word *dst)
{
	byte w[2];
	if (!read_file(f, w, 2)) {
		return 0;
	}
	*dst = (((word)w[1]) << 8) | ((word)w[0]);
	return 2;
}

byte read_le_dword(FIL *f, dword *dst)
{
	byte dw[4];
	if (!read_file(f, dw, 4)) {
		return 0;
	}
	*dst = (((dword)dw[3]) << 24) | (((dword)dw[2]) << 16) | (((dword)dw[1]) << 8) | ((dword)dw[0]);
	return 4;
}

int mb02_open_image(byte drv_id, const char *filename)
{
	FIL f;
	FILINFO fi;

	char lfn[1];
	fi.lfname = lfn;
	fi.lfsize = 0;

	const char *p_ext;
	const char *p_name;

	p_ext = filename + strlen(filename);
	while (*p_ext != '.' && p_ext > filename)
		p_ext--;

	p_name = filename + strlen(filename);
	while (*p_name != '/' && p_name > filename)
		p_name--;
	if (*p_name == '/')
		p_name++;

	if (!(strlen(p_ext) == 4 && (strcasecmp(p_ext, ".mbd") == 0 || strcasecmp(p_ext, ".mb2") == 0))) {
		return 1;
	}

	if (f_stat(filename, &fi) != FR_OK) {
		return 2;
	}

	if (f_open(&f, filename, FA_OPEN_EXISTING | FA_WRITE | FA_READ) != FR_OK) {
		TRACE(("file open failed\n"));
		return 2;
	}

	byte bootsec[0x60];
	if (!read_file(&f, bootsec, sizeof(bootsec))) {
		f_close(&f);
		return 2;
	}

	if (!(bootsec[2] == 0x80 && bootsec[3] == 0x02 && bootsec[32] == 0 && bootsec[53] == 0)) {
		// non MB-02 disk identifiers
		f_close(&f);
		return 1;
	}

	drives[drv_id].sec_cnt = bootsec[6];
	drives[drv_id].sec_sz_id = 3;
	drives[drv_id].sec_size = 1024;

	if (drives[drv_id].img_fmt != DISK_NONE) {
		f_close(&drives[drv_id].fd);
	}
	memcpy(&drives[drv_id].fd, &f, sizeof(FIL));

	byte wp = 0;
	if ((fi.fattrib & AM_RDO) != 0) {
		wp = 1;
	}

	drives[drv_id].wp = wp;
	drives[drv_id].img_fmt = DISK_MBD;
	drives[drv_id].side_cnt = bootsec[8];
	drives[drv_id].cyl_cnt = bootsec[4];

	return 0;
}

void mb02_close_image(byte drv_id)
{
	if (drives[drv_id].img_fmt != DISK_NONE) {
		f_close(&drives[drv_id].fd);
		clear_drive(drv_id);
	}
}

//---------------------------------------------------------------------------------------
byte lookup_track(byte alloc)
{
	if (cur_drv->img_fmt == DISK_NONE) {
		return 0;
	}
	TRACE(("lookup_track: cur_cyl = %u, cyl_cnt = %u, cur_side=%u\n", cur_drv->cur_cyl, cur_drv->cyl_cnt, mb02_state.cur_side));
	if (cur_drv->cur_cyl >= cur_drv->cyl_cnt) {
		if (cur_drv->img_fmt == DISK_NONE) {
			cur_drv->sec_cnt = 0;
		}
		return 0;
	}
	if (cur_drv->img_fmt == DISK_MBD) {
		cur_drv->img_trkoff = ((dword)cur_drv->cur_cyl) * ((dword)cur_drv->sec_cnt) * ((dword)cur_drv->sec_size) * ((dword)cur_drv->side_cnt);
		TRACE(("lookup_track: trk=%x\n", cur_drv->img_trkoff));
	}
	return 1;
}

byte lookup_sector(byte sec, byte side_check, byte side)
{
	if (cur_drv->img_fmt == DISK_NONE) {
		return 0;
	}
	TRACE(("lookup_sector: %u, c = %u, s = %u\n", sec, side_check, side));
	if (cur_drv->img_fmt == DISK_MBD) {
		if (sec > cur_drv->sec_cnt || sec == 0) {
			return 0;
		}
		if (side_check && side != mb02_state.cur_side) {
			return 0;
		}
		cur_drv->sec_off = (((word)mb02_state.cur_side) * ((word)cur_drv->sec_cnt) + sec - 1) * ((word)cur_drv->sec_size);
		cur_drv->cur_sec = sec;
		cur_drv->sec_res = (1 << cur_drv->sec_sz_id);
	}
	cur_drv->sec_buff_pos = 0;
	cur_drv->sec_buff_fill = 0;
	cur_drv->next_sec_off = cur_drv->sec_off + cur_drv->sec_size;
	TRACE(("lookup_sector: data off = %x (r=%x,next_off=%x)\n", cur_drv->sec_off, cur_drv->sec_res, cur_drv->next_sec_off));
	f_lseek(&cur_drv->fd, cur_drv->img_trkoff + (dword)cur_drv->sec_off);
	return 1;
}

byte lookup_next_sector()
{
	if (cur_drv->img_fmt == DISK_NONE) {
		return 0;
	}
	if (cur_drv->img_fmt == DISK_MBD) {
		if (cur_drv->cur_sec == cur_drv->sec_cnt) {
			cur_drv->cur_sec = 1;
			lookup_sector(1, 0, 0);
		}
		else {
			lookup_sector(cur_drv->cur_sec + 1, 0, 0);
		}
		return cur_drv->cur_sec;
	}
	return 0;
}

byte read_sector(byte size, byte raw)
{
	byte rd_size;

	if (cur_drv->cur_cyl == 0 && mb02_state.cur_side == 0) {
		size = 16;
	}

	if ((cur_drv->sec_off + ((word)size)) >= cur_drv->next_sec_off) {
		rd_size = (byte)(cur_drv->next_sec_off - cur_drv->sec_off);
		cur_drv->end_of_sec = 1;
	}
	else {
		rd_size = size;
	}

	if (rd_size == 0) {
		return 0;
	}

	if (!read_file(&cur_drv->fd, cur_drv->sec_buff, rd_size)) {
		cur_drv->end_of_sec = 1;
		cur_drv->sec_res = 0;
		return 0;
	}

	cur_drv->sec_off += rd_size;
	cur_drv->sec_buff_fill = rd_size;
	cur_drv->sec_buff_pos = 0;
	return rd_size;
}

byte write_sector(byte size, byte raw)
{
	byte i, wr_size;

	if (cur_drv->cur_cyl == 0 && mb02_state.cur_side == 0) {
		size = 16;
	}

	if ((cur_drv->sec_off + ((word)size)) >= cur_drv->next_sec_off) {
		wr_size = (byte)(cur_drv->next_sec_off - cur_drv->sec_off);
		cur_drv->end_of_sec = 1;
	}
	else {
		wr_size = size;
	}
	if (wr_size == 0) {
		return 0;
	}

	if (!write_file(&cur_drv->fd, cur_drv->sec_buff, wr_size)) {
		return 0;
	}
	size -= wr_size;
	for (i = 0; i < size; i++) {
		cur_drv->sec_buff[i] = cur_drv->sec_buff[i + wr_size];
	}
	cur_drv->sec_buff_pos = size;
	cur_drv->sec_buff_fill = size;
	cur_drv->sec_off += wr_size;
	return wr_size;
}
//---------------------------------------------------------------------------------------
void wd_cmd_write(byte data)
{
	if ((data & 0xf0) == CMD_INTERRUPT) {
		mb02_state.cmd_reg = data;
		mb02_state.mb02_state = MB02_IDLE;
		SET_SYS_READY();
		return;
	}
	if ((mb02_state.sys_stat & SYS_DRQ) != 0) {
		return;
	}
	mb02_state.cmd_reg = data;
	if (cur_drv->img_fmt == DISK_NONE) {
		mb02_state.stat_val = STAT_NOTRDY;
		SET_SYS_ACK();
		return;
	}
	CLEAR_STAT();
	data &= 0xf0;
	switch ((data & 0xc0) >> 6) {
		// type I commands
		case TYPEI_CMD_1:
		case TYPEI_CMD_2:
			TRACE(("wd_cmd: typeI cmd: %x (prev_dr=%x,cur_sec=%x)\n", data, mb02_state.data_reg, cur_drv->cur_sec));
			if (data == CMD_SEEK) {
				cur_drv->cur_cyl = mb02_state.cyl_reg = mb02_state.data_reg;
			}
			else {
				if ((mb02_state.cmd_reg & 0x40) != 0) {
					mb02_state.step_dir = (mb02_state.cmd_reg & 0x20) == 0 ? 1 : -1;
				}

				if ((mb02_state.cmd_reg & 0x60) != 0) {
					if (mb02_state.cmd_reg & TYPEI_BIT_U) {
						mb02_state.cyl_reg += mb02_state.step_dir;
					}

					if (mb02_state.step_dir == -1 && cur_drv->cur_cyl == 0) {
						mb02_state.cyl_reg = 0;
						cur_drv->cur_cyl = 0;
					}
					else {
						cur_drv->cur_cyl += mb02_state.step_dir;
					}
				}
				else {
					mb02_state.cyl_reg = 0;
					cur_drv->cur_cyl = 0;
				}
			}

			if (mb02_state.cyl_reg == 0) {
#if RDCRC_TYPEI
				// fix save RDCRC status
				mb02_state.stat_val |= STAT_HEAD_CYL_0;
#else
				mb02_state.stat_val = STAT_HEAD_CYL_0;
#endif
			}

			if (mb02_state.cmd_reg & (TYPEI_BIT_H | TYPEI_BIT_V)) {
				cur_drv->head_on = 1;
				mb02_state.stat_val |= STAT_HEAD_ON;
			}
			else {
				cur_drv->head_on = 0;
			}

			DRAW_SEEK(cur_drv->cur_cyl);

			if (!lookup_track(0)) {
				// only check address mark when motor on & verify flag is on
				if (mb02_state.cmd_reg & TYPEI_BIT_V) {
					mb02_state.stat_val |= STAT_NOTFOUND;
				}
			}

			if (cur_drv->wp) {
				mb02_state.stat_val |= STAT_WP;
			}
			break;

			// type II comamnds
		case TYPEII_CMD: {
			byte is_write = mb02_state.cmd_reg & 0x20;

			TRACE(("wd_cmd: typeII cmd: %x\n", data));

			if (is_write && cur_drv->wp) {
				mb02_state.stat_val |= STAT_WP;
				break;
			}
			if (lookup_sector(mb02_state.sec_reg & 0x7f, (mb02_state.cmd_reg & TYPEII_BIT_C) != 0, (mb02_state.cmd_reg & TYPEII_BIT_S) != 0)) {
				TRACE(("reading sector sec=%d, img_trk_off=%x, trk_sec_off=%x, trk=%u (drv_trk=%u), side = %u\n", mb02_state.sec_reg, cur_drv->img_trkoff, cur_drv->sec_off, mb02_state.cyl_reg, cur_drv->cur_cyl, mb02_state.cur_side));
				cur_drv->head_on = 1;
				mb02_state.mb02_state = is_write ? MB02_WRITE : MB02_READ;
				cur_drv->end_of_sec = false;
				mb02_state.sys_stat_reads = 0;
				SET_SYS_BUSY();
				SET_STAT_BUSY();
				return;
			}
			else {
				TRACE(("invalid sector sec=%d, img_trk_off=%x, trk_sec_off=%x, trk=%u (drv_trk=%u), side = %u\n", mb02_state.sec_reg, cur_drv->img_trkoff, cur_drv->sec_off, mb02_state.cyl_reg, cur_drv->cur_cyl, mb02_state.cur_side));
				mb02_state.stat_val |= STAT_NOTFOUND;
			}
			break;
		}

		// type III commands
		case TYPEIII_CMD:
			TRACE(("wd_cmd: typeIII cmd: %x\n", data));
			if (mb02_state.cmd_reg & 0x20) {
				mb02_state.stat_val |= STAT_LOST;
			}
			else {
				// read am
				if (lookup_sector(cur_drv->cur_sec & 0x7f, 0, 0)) {
					mb02_state.mb02_state = MB02_READ_ADR;
					mb02_state.sys_stat_reads = 0;
					cur_drv->head_on = 1;
					SET_SYS_BUSY();
					SET_STAT_BUSY();
					return;
				}
				mb02_state.stat_val |= STAT_NOTFOUND;
			}
			break;
	}
	SET_SYS_ACK();
}

//---------------------------------------------------------------------------------------
byte put_buff(byte data)
{
	cur_drv->sec_buff[cur_drv->sec_buff_pos] = data;
	return (++cur_drv->sec_buff_pos != SEC_BUF_SIZE);
}

byte get_buff(byte *data)
{
	*data = cur_drv->sec_buff[cur_drv->sec_buff_pos];
	return (++cur_drv->sec_buff_pos != cur_drv->sec_buff_fill);
}

//---------------------------------------------------------------------------------------
void wd_cyl_write(byte data)
{
	mb02_state.cyl_reg = data;
}

void wd_sec_write(byte data)
{
	mb02_state.sec_reg = data;
}

void wd_dat_write(byte data)
{
	mb02_state.data_reg = data;

	switch (mb02_state.mb02_state) {
		case MB02_WRITE: {
			byte multi = mb02_state.cmd_reg & TYPEII_BIT_M;
			if (!put_buff(mb02_state.data_reg)) {
				byte nr = write_sector(SEC_BUF_SIZE, 0);
				TRACE(("%u bytes writen to file\n", nr));
				if (nr == 0) {
					SET_SYS_READY();
					SET_STAT_READY();
					mb02_state.stat_val |= STAT_WRERR;
					mb02_state.mb02_state = MB02_IDLE;
					cur_drv->head_on = 0;
					break;
				}
				if (cur_drv->end_of_sec) {
					byte stat;
					stat = lookup_next_sector();
					if (!stat && multi) {
						SET_SYS_READY();
						SET_STAT_READY();
						mb02_state.stat_val |= STAT_NOTFOUND;
						mb02_state.mb02_state = MB02_IDLE;
						cur_drv->head_on = 0;
						break;
					}
					else if (!multi || stat == 1) {
						SET_SYS_READY();
						SET_STAT_READY();
						mb02_state.mb02_state = MB02_IDLE;
						cur_drv->head_on = 0;
						break;
					}
				}
			}
			break;
		}
	}
}
/*
void mb02_sys_write(byte data)
{
	byte drv_sel;

	drv_sel = data & SYS_DRV_MSK;

	//TRACE(( "beta sys write: %x\n", data ) );

	if (((mb02_state.sys_val ^ data) & SYS_RST) != 0) {
		if (data & SYS_RST) {
			mb02_dsk_init();
		}
	}

	mb02_state.sys_val = data & ~SYS_STAT_MSK;
	cur_drv = drives + drv_sel;
	mb02_state.cur_side = (mb02_state.sys_val & SYS_SIDE) == 0 ? 1 : 0;
}
*/
//---------------------------------------------------------------------------------------
byte disk_revolve()
{
	byte stat_cur;

	stat_cur = 0;
	if (lookup_next_sector() == 1) {
		stat_cur |= STAT_INDEX;
	}
	if (cur_drv->cur_cyl == 0) {
		stat_cur |= STAT_HEAD_CYL_0;
	}
	return stat_cur;
}

//---------------------------------------------------------------------------------------
byte wd_stat_read()
{
	mb02_state.sys_stat &= ~SYS_INTRQ;
	if (mb02_state.mb02_state == MB02_IDLE && cur_drv->img_fmt != DISK_NONE && cur_drv->sec_cnt != 0 && (mb02_state.sys_val & SYS_HLD_EN) != 0 && (mb02_state.cmd_reg & 0x80) == 0) {
		return disk_revolve() | mb02_state.stat_val;
	}
	else {
		return mb02_state.stat_val;
	}
}

byte wd_cyl_read()
{
	return mb02_state.cyl_reg;
}

byte wd_sec_read()
{
	return mb02_state.sec_reg;
}

byte wd_dat_read()
{
	byte data_val;

	data_val = 0xff;

	switch (mb02_state.mb02_state) {
		case MB02_READ: {
			byte multi = mb02_state.cmd_reg & TYPEII_BIT_M;

			if (cur_drv->sec_buff_pos == cur_drv->sec_buff_fill) {
				if (!read_sector(SEC_BUF_SIZE, 0)) {
					SET_SYS_READY();
					SET_STAT_READY();
					mb02_state.stat_val |= STAT_RDCRC;
					mb02_state.stat_val |= STAT_NOTFOUND;
					mb02_state.mb02_state = MB02_IDLE;
					cur_drv->head_on = 0;
					break;
				}
			}

			if (!get_buff(&data_val)) {
				if (cur_drv->end_of_sec) {
					byte cur_bres, stat;

					cur_bres = (cur_drv->sec_res & (1 << cur_drv->sec_sz_id)) == 0;

					stat = lookup_next_sector();
					TRACE(("wd_dat_read: stat = %u (cur_sec=%u)\n", stat, cur_drv->cur_sec));
					if (!stat && multi) {
						SET_SYS_READY();
						SET_STAT_READY();
						mb02_state.mb02_state = MB02_IDLE;
						cur_drv->head_on = 0;
						if (cur_bres) {
							mb02_state.stat_val |= STAT_RDCRC;
							mb02_state.stat_val |= STAT_NOTFOUND;
						}
						break;
					}
					else if (!multi || stat == 1) {
						SET_SYS_READY();
						SET_STAT_READY();
						mb02_state.mb02_state = MB02_IDLE;
						cur_drv->head_on = 0;
						if (cur_bres) {
							mb02_state.stat_val |= STAT_RDCRC;
						}
						break;
					}
					cur_drv->end_of_sec = 0;
				}
			}
			break;
		}

		case MB02_READ_ADR:
			if (cur_drv->sec_buff_pos == 0) {
				word crc = crc_add(crc_init(), cur_drv->sec_buff[0] = cur_drv->cur_cyl);
				if (cur_drv->img_fmt == DISK_MBD) {
					cur_drv->sec_buff[1] = 0;
				}
				else {
					cur_drv->sec_buff[1] = mb02_state.cur_side;
				}
				crc = crc_add(crc, cur_drv->sec_buff[1]);
				crc = crc_add(crc, cur_drv->sec_buff[2] = cur_drv->cur_sec);
				crc = crc_add(crc, cur_drv->sec_buff[3] = cur_drv->sec_sz_id);
				cur_drv->sec_buff[4] = (crc >> 8); // crc hi byte
				cur_drv->sec_buff[5] = crc & 0xff; // crc lo byte
				cur_drv->sec_buff_fill = 6;
			}
			if (!get_buff(&data_val)) {
				SET_SYS_READY();
				SET_STAT_READY();
				disk_revolve();
				mb02_state.mb02_state = MB02_IDLE;
				cur_drv->head_on = 0;
			}
			break;
		default:
			break;
	}

	return data_val;
}
/*
byte mb02_sys_read()
{
//	return mb02_state.sys_val | mb02_state.sys_stat | 0x3f;
	return mb02_state.sys_stat | 0x3f;
}
*/
//---------------------------------------------------------------------------------------
void mb02_write_port(byte port, byte data)
{
//	TRACE(("write_port: (%02x, %02x)\n", port, data));
	if (port == MB02_PORT_CMD) {
		wd_cmd_write(data);
	}
	if (port == MB02_PORT_CYL) {
		wd_cyl_write(data);
	}
	if (port == MB02_PORT_SEC) {
		wd_sec_write(data);
	}
	if (port == MB02_PORT_DTA) {
		wd_dat_write(data);
//		mb02_state.sys_stat_reads = 0;
	}
/*
	if (port & MB02_PORT_SYS_MSK) {
		mb02_sys_write(data);
	}
*/
}

byte mb02_read_port(byte port)
{
	byte data = 0xff;
	if (port == MB02_PORT_STA) {
		data = wd_stat_read();
	}
	if (port == MB02_PORT_CYL) {
		data = wd_cyl_read();
	}
	if (port == MB02_PORT_SEC) {
		data = wd_sec_read();
	}
	if (port == MB02_PORT_DTA) {
		data = wd_dat_read();
//		mb02_state.sys_stat_reads = 0;
	}
/*
	if (port & MB02_PORT_SYS_MSK) {
		if (mb02_state.sys_stat_reads >= SYS_NUM_OF_TRYOUTS) {
			if (((mb02_state.sys_stat & SYS_INTRQ) == 0) && (mb02_state.mb02_state != MB02_IDLE)) {
				TRACE(("emulating data underrun\n"));
				SET_SYS_READY();
				SET_STAT_READY();
				mb02_state.stat_val |= STAT_LOST;
			}
			mb02_state.sys_stat_reads = 0;
		}
		else {
			mb02_state.sys_stat_reads += 1;
		}
		data = mb02_sys_read();
	}
*/
//	TRACE(("read_port: (%02x) -> %02x\n", port, data));
	return data;
}

//---------------------------------------------------------------------------------------
byte mb02_get_state()
{
	return mb02_state.mb02_state;
}

byte mb02_cur_drv()
{
	return mb02_state.sys_val & SYS_DRV_MSK;
}

byte mb02_is_disk_wp(byte drv)
{
	if (drv < 3) {
		return drives[drv].wp;
	}
	return 0;
}

void mb02_set_disk_wp(byte drv, byte wp)
{
	if (drv < 3) {
		drives[drv].wp = wp ? 1 : 0;
	}
}

byte mb02_is_disk_loaded(byte drv)
{
	if (drv < 3) {
		return drives[drv].cyl_cnt > 0;
	}
	return 0;
}

//---------------------------------------------------------------------------------------
void clear_drive(byte drv_id)
{
	mb02_init_drive(drv_id);

	drives[drv_id].sec_cnt = 0;
	drives[drv_id].cyl_cnt = 0;
	drives[drv_id].img_fmt = DISK_NONE;
}

void mb02_init_drive(byte drv_id)
{
	TRACE(("mb02_init_drive\n"));

//	drives[drv_id].wp = 0;
	drives[drv_id].cur_sec = 1;
	drives[drv_id].cur_cyl = 0;
	drives[drv_id].head_on = 0;
	drives[drv_id].img_trkoff = 0;
	drives[drv_id].sec_off = 0;
	drives[drv_id].next_sec_off = 0;
	drives[drv_id].sec_buff_pos = 0;
	drives[drv_id].sec_buff_fill = 0;
	drives[drv_id].sec_res = 0;
}

void mb02_dsk_init()
{
	mb02_state.mb02_state = MB02_IDLE;
	mb02_state.cyl_reg = 0;
	mb02_state.sec_reg = 1;
	mb02_state.stat_val = 0;
	mb02_state.step_dir = 1;
	mb02_state.sys_stat_reads = 0;
}

void mb02_init()
{
	int i;

	mb02_dsk_init();
	for (i = 0; i < MB02_DRIVES_COUNT; i++) {
		clear_drive(i);
	}

	mb02_state.sys_val = SYS_RST;
	mb02_state.sys_stat = 0;
	cur_drv = drives + 0;
	mb02_state.cur_side = 0;
}

int mb02_leds()
{
	byte mb02_state = mb02_get_state();

	int result = 0;

	if (mb02_state == MB02_READ || mb02_state == MB02_READ_TRK || mb02_state == MB02_READ_ADR || mb02_state == MB02_SEEK)
		result |= 1;

	if (mb02_state == MB02_WRITE || mb02_state == MB02_WRITE_TRK)
		result |= 4;

	return result;
}
//---------------------------------------------------------------------------------------
