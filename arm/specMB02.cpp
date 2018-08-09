#include <stdarg.h>
#include <ctype.h>

#include "system.h"
#include "specMB02.h"
#include "specConfig.h"

#define TRACE(x)

#define MB02_DRIVES_COUNT  4

#define STAT_NOTRDY        0x82  // drive not ready
#define STAT_WP            0x40  // disk write protected
#define STAT_BREAK         0x20  // break
#define STAT_SEEKERR       0x10  // seek error
#define STAT_NOTFOUND      0x10  // record not found
#define STAT_RDCRC         0x08  // CRC error
#define STAT_HEAD_CYL_0    0x04  // head on cyl 0
#define STAT_LOST          0x04  // data underrun
#define STAT_TIMEOUT       0x01  // operation in progress
#define STAT_OK            0x00


// content of EPROM: simple bootstrapper which switch to BS-DOS and initialize...
const unsigned char epromBootstrap[64] = {
	0xF3, 0xAF, 0xED, 0x47, 0xD3, 0xFE, 0x01, 0xFD,
	0x7F, 0xED, 0x79, 0x31, 0x30, 0x5B, 0x21, 0x1B,
	0x00, 0x11, 0x00, 0x5B, 0x01, 0x23, 0x00, 0xD5,
	0xED, 0xB0, 0xC9, 0x3E, 0x41, 0xD3, 0x17, 0xAF,
	0xDB, 0xFE, 0xF6, 0xE0, 0x3C, 0x3E, 0x30, 0xCC,
	0x20, 0x00, 0x3E, 0x40, 0xD3, 0x17, 0x3A, 0x06,
	0x00, 0xFE, 0x31, 0xCA, 0x06, 0x39, 0xFE, 0x66,
	0xCA, 0x78, 0x39, 0xC7, 0xF3, 0x76, 0x00, 0x00
};

enum {
	MB02_IDLE = 0,
	MB02_RDSEC = 4,
	MB02_WRSEC = 5,
	MB02_ACTIVE = 12,
	MB02_PASIVE = 13
};

struct mb02_drive {
	FIL fd;

	unsigned char wp : 1;
	unsigned char pr : 1;
	unsigned char activated : 1;

	unsigned char cur_side : 1;
	byte cur_sec;
	byte cur_cyl;

	byte sec_cnt;
	byte cyl_cnt;
	byte side_cnt;
	word sec_size;

	dword img_trkoff; // offset of the track from file begin
	word sec_off; // offset from begin of track
};

struct mb02_drive mb02_drives[MB02_DRIVES_COUNT];
struct mb02_drive *mb02_drv;

// state registers
struct mb02_state {
	byte cmd_reg;
	byte state;
	word ptr;
} mb02_state;

void mb02_clear_drive(byte drv_id);

//---------------------------------------------------------------------------------------

int mb02_open_image(byte drv_id, const char *filename)
{
	FIL f;
	FILINFO fi;

	char lfn[1];
	fi.lfname = lfn;
	fi.lfsize = 0;

	if (filename == NULL || strlen(filename) < 4)
		return 1;

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

	if (!(strlen(p_ext) == 4 && (strcasecmp(p_ext, ".mbd") == 0 || strcasecmp(p_ext, ".mb2") == 0)))
		return 1;

	if (f_stat(filename, &fi) != FR_OK)
		return 2;

	if (f_open(&f, filename, FA_OPEN_EXISTING | FA_WRITE | FA_READ) != FR_OK) {
		__TRACE("file open failed\n");
		return 2;
	}

	byte bootsec[0x60];
	if (!read_file(&f, bootsec, sizeof(bootsec))) {
		f_close(&f);
		return 2;
	}

	if (!(bootsec[2] == 0x80 && bootsec[3] == 0x02 && bootsec[32] == 0 && bootsec[37] == 0)) {
		// non MB-02 disk identifiers
		f_close(&f);
		return 1;
	}


	if (mb02_drives[drv_id].pr) {
		f_close(&mb02_drives[drv_id].fd);
	}
	memcpy(&mb02_drives[drv_id].fd, &f, sizeof(FIL));

	mb02_drives[drv_id].wp = ((fi.fattrib & AM_RDO) != 0) ? 1 : 0;
	mb02_drives[drv_id].pr = 1;
	mb02_drives[drv_id].activated = 0;
	mb02_drives[drv_id].cyl_cnt = bootsec[4];
	mb02_drives[drv_id].sec_cnt = bootsec[6];
	mb02_drives[drv_id].side_cnt = bootsec[8];

	mb02_drives[drv_id].sec_size = 1024;
	mb02_drives[drv_id].cur_sec = 1;
	mb02_drives[drv_id].cur_cyl = 0;
	mb02_drives[drv_id].sec_off = 0;
	mb02_drives[drv_id].img_trkoff = 0;

	__TRACE("MB-02 disk @%u:'%s' loaded...\n", drv_id, filename);
	return 0;
}

void mb02_close_image(byte drv_id)
{
	if (mb02_drives[drv_id].pr) {
		f_close(&mb02_drives[drv_id].fd);
		mb02_clear_drive(drv_id);
	}
}

//---------------------------------------------------------------------------------------
bool mb02_lookup_track()
{
	if (!(mb02_drv && mb02_drv->pr))
		return false;

	TRACE(("lookup_track: cur_cyl=%u, cyl_cnt=%u, cur_side=%u\n", mb02_drv->cur_cyl, mb02_drv->cyl_cnt, mb02_drv->cur_side));
	if (mb02_drv->cur_cyl >= mb02_drv->cyl_cnt)
		return false;

	mb02_drv->img_trkoff = ((dword) mb02_drv->cur_cyl) * ((dword) mb02_drv->sec_cnt) * ((dword) mb02_drv->sec_size) * ((dword) mb02_drv->side_cnt);
	TRACE(("lookup_track: img_trkoff=%x\n", mb02_drv->img_trkoff));

	return true;
}

bool mb02_lookup_sector()
{
	if (!(mb02_drv && mb02_drv->pr))
		return false;

	TRACE(("lookup_sector: cur_sec=%u, cur_cyl=%u, cur_side=%u\n", mb02_drv->cur_sec, mb02_drv->cur_cyl, mb02_drv->cur_side));
	if (mb02_drv->cur_sec > mb02_drv->sec_cnt || mb02_drv->cur_sec == 0)
		return false;

	if (!mb02_lookup_track())
		return false;

	mb02_drv->sec_off = (((word) mb02_drv->cur_side) * ((word) mb02_drv->sec_cnt) + mb02_drv->cur_sec - 1) * ((word) mb02_drv->sec_size);
	TRACE(("lookup_sector: sec_off=%x\n", mb02_drv->sec_off));

	f_lseek(&mb02_drv->fd, mb02_drv->img_trkoff + (dword) mb02_drv->sec_off);
	return true;
}

void mb02_reset_state()
{
	mb02_state.cmd_reg = MB02_IDLE;
	mb02_state.state = 0;
	mb02_state.ptr = 0;
}

void mb02_received(byte value)
{
	UINT res;
	switch (mb02_state.cmd_reg) {
		case MB02_IDLE:
			__TRACE("\n@> %X", value);
			mb02_state.cmd_reg = value;
			mb02_state.state = 0;
			mb02_state.ptr = 0;

			if (value == MB02_PASIVE) {
				__TRACE("!");
				mb02_drv = NULL;
				mb02_state.cmd_reg = MB02_IDLE;
			}
			break;

		case MB02_RDSEC: {
			__TRACE("--o(%u)=%02x", mb02_state.ptr, value);
			if (mb02_state.ptr == 0) {
				if (mb02_drv && mb02_drv->pr) {
					mb02_drv->cur_sec = value & 0x7f;
					mb02_drv->cur_side = value >> 7;
				}
				mb02_state.ptr++;
			}
			else if (mb02_state.ptr == 1) {
				if (mb02_drv && mb02_drv->pr) {
					mb02_drv->cur_cyl = value;
					mb02_state.state = mb02_lookup_sector() ? STAT_OK : STAT_SEEKERR;
				}
				else
					mb02_state.state = STAT_NOTRDY;

				mb02_state.ptr++;
			}
			else if (value == 0 || value == 0xff) {
				__TRACE("--o(%u)=%02x", mb02_state.ptr, value);
				mb02_reset_state();
			}

			break;
		}

		case MB02_WRSEC: {
			if (mb02_state.ptr == 0) {
				__TRACE("--o(%u)=%02x", mb02_state.ptr, value);
				if (mb02_drv && mb02_drv->pr) {
					mb02_drv->cur_sec = value & 0x7f;
					mb02_drv->cur_side = value >> 7;
				}
				mb02_state.ptr++;
			}
			else if (mb02_state.ptr == 1) {
				__TRACE("--o(%u)=%02x", mb02_state.ptr, value);
				if (mb02_drv && mb02_drv->pr) {
					mb02_drv->cur_cyl = value;
					mb02_state.state = mb02_lookup_sector() ? STAT_OK : STAT_SEEKERR;

					if (mb02_drv->wp)
						mb02_state.state |= STAT_WP;
				}
				else
					mb02_state.state = STAT_NOTRDY;

				mb02_state.ptr++;
			}
			else if (mb02_state.state != STAT_OK) {
				mb02_reset_state();
			}
			else if (mb02_state.ptr > 2 && mb02_drv && mb02_drv->pr) {
				f_write(&mb02_drv->fd, &value, 1, &res);

				mb02_state.ptr++;
				if (!(mb02_state.ptr & 63))
					__TRACE(".");

				if (mb02_state.ptr == mb02_drv->sec_size + 3) {
					__TRACE("!");
					mb02_reset_state();
				}
			}

			break;
		}

		case MB02_ACTIVE: {
			__TRACE("--o(%u)=%02x", mb02_state.ptr, value);
			if (mb02_state.ptr == 0) {
				if (value >= MB02_DRIVES_COUNT)
					mb02_state.state = 0; // unknown disk
				else {
					mb02_drv = &mb02_drives[value];
					if (!mb02_drv->pr)
						mb02_state.state = 1; // disk not ready
					else {
						mb02_state.state = mb02_drv->activated ? 2 : 3; // disk activated
						mb02_drv->activated = 1;
					}
				}

				mb02_state.ptr++;
			}
			else
				mb02_reset_state();

			break;
		}

		default:
			__TRACE("\n!!o(%X)=%02x", mb02_state.cmd_reg, value);
			mb02_state.cmd_reg = MB02_IDLE;
			break;
	}
}

byte mb02_transmit()
{
	UINT res;
	byte result = 0xff;

	switch (mb02_state.cmd_reg) {
		case MB02_RDSEC: {
			if (mb02_state.ptr == 2) {
				result = mb02_state.state;
				__TRACE("--i(2)=%02x", result);

				if (mb02_state.state != STAT_OK)
					mb02_reset_state();
				else
					mb02_state.ptr++;
			}
			else if (mb02_state.state != STAT_OK || mb02_state.ptr < 2) {
				mb02_reset_state();
			}
			else if (mb02_drv && mb02_drv->pr) {
				f_read(&mb02_drv->fd, &result, 1, &res);

				if (!(mb02_state.ptr & 63))
					__TRACE(".");

				mb02_state.ptr++;
				if (mb02_state.ptr == mb02_drv->sec_size + 3) {
					__TRACE("!");
					mb02_reset_state();
				}
			}

			break;
		}

		case MB02_WRSEC: {
			__TRACE("--i(%u)=%02x", mb02_state.ptr, mb02_state.state);
			if (mb02_state.ptr == 2) {
				result = mb02_state.state;

				if (mb02_state.state != STAT_OK)
					mb02_reset_state();
				else
					mb02_state.ptr++;
			}
			else
				mb02_reset_state();

			break;
		}

		case MB02_ACTIVE: {
			__TRACE("--i(%u)=%02x", mb02_state.ptr, mb02_state.state);
			if (mb02_state.ptr == 1)
				result = mb02_state.state;

			mb02_reset_state();
			break;
		}

		default:
			__TRACE("\n!!i(%X)=%02x", mb02_state.cmd_reg, result);
			mb02_reset_state();
		case MB02_IDLE:
			break;
	}

	return result;
}

//---------------------------------------------------------------------------------------
void mb02_clear_drive(byte drv_id)
{
	mb02_drives[drv_id].wp = 0;
	mb02_drives[drv_id].pr = 0;
	mb02_drives[drv_id].activated = 0;
	mb02_drives[drv_id].cur_sec = 1;
	mb02_drives[drv_id].cur_cyl = 0;
	mb02_drives[drv_id].sec_off = 0;
	mb02_drives[drv_id].img_trkoff = 0;
	mb02_drives[drv_id].sec_size = 1024;
	mb02_drives[drv_id].sec_cnt = 0;
	mb02_drives[drv_id].cyl_cnt = 0;
	mb02_drives[drv_id].side_cnt = 0;
}

void mb02_init()
{
	for (int i = 0; i < MB02_DRIVES_COUNT; i++)
		mb02_clear_drive(i);

	mb02_state.cmd_reg = MB02_IDLE;
	mb02_state.state = 0;
	mb02_state.ptr = 0;

	mb02_drv = NULL;
}

void mb02_fill_eprom()
{
	SystemBus_Write(0xc00020, 0); // use bank 0

	dword addr = 0x40C000;
	for (word i = 0; i < 2048; i++) {
		SystemBus_Write(addr++, (i < sizeof(epromBootstrap)) ? epromBootstrap[i] : 0);
	}
}

byte mb02_is_disk_wp(byte drv)
{
	if (drv < MB02_DRIVES_COUNT) {
		return mb02_drives[drv].wp;
	}
	return 0;
}

void mb02_set_disk_wp(byte drv, bool wp)
{
	if (drv < MB02_DRIVES_COUNT) {
		mb02_drives[drv].wp = wp ? 1 : 0;
	}
}

byte mb02_is_disk_loaded(byte drv)
{
	if (drv < MB02_DRIVES_COUNT) {
		return mb02_drives[drv].pr && mb02_drives[drv].cyl_cnt > 0;
	}
	return 0;
}
//---------------------------------------------------------------------------------------
