#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "system.h"
#include "specMB02.h"
#include "specConfig.h"
#include "specRtc.h"
#include "shell/dialog.h"

#define TRACE(...)

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


// content of EPROM - simple bootstrapper which initialize BS-DOS and its sysvars:
const unsigned char epromBootstrap[168] = {
	0xF3, 0x31, 0x00, 0x60, 0xAF, 0xED, 0x47, 0xD3,
	0xFE, 0x01, 0xFD, 0x7F, 0xED, 0x79, 0x21, 0x00,
	0x40, 0x11, 0x01, 0x40, 0x01, 0xFF, 0x1A, 0x75,
	0xED, 0xB0, 0x21, 0x27, 0x00, 0x11, 0x00, 0x60,
	0x01, 0x81, 0x00, 0xD5, 0xED, 0xB0, 0xC9, 0x3E,
	0x60, 0xD3, 0x17, 0x3E, 0x38, 0x32, 0x66, 0x12,
	0x32, 0x8E, 0x12, 0x08, 0x3E, 0x61, 0xD3, 0x17,
	0x21, 0xE0, 0x03, 0x11, 0xE1, 0x03, 0x01, 0x1F,
	0x01, 0x36, 0x00, 0xED, 0xB0, 0x08, 0x32, 0xE2,
	0x03, 0x3E, 0x04, 0x32, 0xF1, 0x03, 0x3E, 0x01,
	0x32, 0xEE, 0x03, 0x3D, 0xDB, 0xFE, 0xF6, 0xE0,
	0x3C, 0x3E, 0x30, 0xCC, 0x20, 0x00, 0x3E, 0x40,
	0xD3, 0x17, 0x3A, 0x06, 0x00, 0xFE, 0x31, 0xCA,
	0x06, 0x39, 0xFE, 0x66, 0xCA, 0x78, 0x39, 0xC7,
/*	offset 112 (0x70) - boot effect */
	0xAF, 0xD3, 0xFE, 0x3E, 0x03, 0x21, 0x00, 0x40,
	0x11, 0x01, 0x40, 0x01, 0x00, 0x01, 0x75, 0xED,
	0xB0, 0x06, 0x06, 0x36, 0x7E, 0xED, 0xB0, 0x04,
	0x75, 0xED, 0xB0, 0x3D, 0x20, 0xED, 0x44, 0x4D,
	0x29, 0x29, 0x29, 0x09, 0x01, 0x0C, 0x00, 0x09,
	0x7C, 0xE6, 0x03, 0xF6, 0x58, 0x67, 0xEB, 0xED,
	0xA0, 0xEB, 0x06, 0x20, 0x10, 0xFE, 0x18, 0xE6
};

const int bootEffectOffset = 112;
const int bootEffectLength = 56;

// tiny sector buffer
byte secBuffer[0x100];

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
};

struct mb02_drive mb02_drives[MB02_DRIVES_COUNT];
struct mb02_drive *mb02_drv;

void mb02_clear_drive(byte drv_id)
{
	mb02_drives[drv_id].wp = 0;
	mb02_drives[drv_id].pr = 0;
	mb02_drives[drv_id].activated = 0;
	mb02_drives[drv_id].cur_sec = 1;
	mb02_drives[drv_id].cur_cyl = 0;
	mb02_drives[drv_id].sec_cnt = 0;
	mb02_drives[drv_id].cyl_cnt = 0;
	mb02_drives[drv_id].side_cnt = 0;
}

// state registers
struct mb02_state {
	byte cmd_reg;
	byte state;
	word ptr;
} mb02_state;

//---------------------------------------------------------------------------------------
bool mb02_check_image(byte *bootsec)
{
	return (
		bootsec[0] == 0x18 && bootsec[2] == 0x80 && bootsec[3] == 2 &&
		bootsec[5] == 0 && bootsec[7] == 0 && (bootsec[8] == 1 || bootsec[8] == 2) &&
		bootsec[9] == 0 && bootsec[10] == 1 && bootsec[11] == 0 &&
		bootsec[32] == 0 && bootsec[37] == 0);
}

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

	byte f_flags = FA_OPEN_EXISTING | FA_WRITE | FA_READ;
	if ((fi.fattrib & AM_RDO) != 0)
		f_flags &= ~FA_WRITE;

	if (f_open(&f, filename, f_flags) != FR_OK) {
		__TRACE("MB-02 disk open failed\n");
		return 2;
	}

	if (!read_file(&f, secBuffer, 0x60)) {
		f_close(&f);
		return 2;
	}

	if (!mb02_check_image(secBuffer)) {
		__TRACE("MB-02 invalid disk identifiers\n");
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
	mb02_drives[drv_id].cyl_cnt = secBuffer[4];
	mb02_drives[drv_id].sec_cnt = secBuffer[6];
	mb02_drives[drv_id].side_cnt = secBuffer[8];
	mb02_drives[drv_id].cur_sec = 1;
	mb02_drives[drv_id].cur_cyl = 0;

	__TRACE("MB-02 disk @%u:'%s' loaded (wp:%u, cyls:%u, secs:%u, sides:%u)...\n",
		drv_id + 1, filename,
		mb02_drives[drv_id].wp,
		mb02_drives[drv_id].cyl_cnt,
		mb02_drives[drv_id].sec_cnt,
		mb02_drives[drv_id].side_cnt);

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
bool mb02_lookup_sector()
{
	if (!(mb02_drv && mb02_drv->pr))
		return false;

	if (mb02_drv->cur_sec > mb02_drv->sec_cnt || mb02_drv->cur_sec == 0) {
		TRACE("--seek(!error sec=%u)", mb02_drv->cur_sec);
		return false;
	}

	if (mb02_drv->cur_cyl >= mb02_drv->cyl_cnt) {
		TRACE("--seek(!error cyl=%u)", mb02_drv->cyl_cnt);
		return false;
	}

	word l_sec_number = (
		((word) mb02_drv->cur_cyl) *
		((word) mb02_drv->side_cnt) +
		((word) mb02_drv->cur_side)
	) * ((word) mb02_drv->sec_cnt) +
		((word) mb02_drv->cur_sec - 1);

	TRACE("--seek(%04u)", l_sec_number);
	f_lseek(&mb02_drv->fd, l_sec_number << 10);
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
			TRACE("\n@> CMD:%X", value);
			mb02_state.cmd_reg = value;
			mb02_state.state = 0;
			mb02_state.ptr = 0;

			if (value == MB02_PASIVE) {
				TRACE("!");
				mb02_drv = NULL;
				mb02_state.cmd_reg = MB02_IDLE;
			}
			break;

		case MB02_RDSEC: {
			TRACE("--out(%02x)", value);
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
				TRACE("--out(%02x)", value);
				mb02_reset_state();
			}

			break;
		}

		case MB02_WRSEC: {
			if (mb02_state.ptr == 0) {
				TRACE("--out(%02x)", value);
				if (mb02_drv && mb02_drv->pr) {
					mb02_drv->cur_sec = value & 0x7f;
					mb02_drv->cur_side = value >> 7;
				}
				mb02_state.ptr++;
			}
			else if (mb02_state.ptr == 1) {
				TRACE("--out(%02x)", value);
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
					TRACE(".");

				if (mb02_state.ptr == 1027) {
					TRACE("!");
					mb02_reset_state();
				}
			}

			break;
		}

		case MB02_ACTIVE: {
			TRACE("--out(%02x)", value);
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
			TRACE("\n@> !error out(%02x) while CMD:%X", value, mb02_state.cmd_reg);
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
				TRACE("--in(%02x)", result);

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
					TRACE(".");

				mb02_state.ptr++;
				if (mb02_state.ptr == 1027) {
					TRACE("!");
					mb02_reset_state();
				}
			}

			break;
		}

		case MB02_WRSEC: {
			TRACE("--in(%02x)", mb02_state.ptr, mb02_state.state);
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
			TRACE("--in(%02x)", mb02_state.ptr, mb02_state.state);
			if (mb02_state.ptr == 1)
				result = mb02_state.state;

			mb02_reset_state();
			break;
		}

		default:
			TRACE("\n@> !error in(%02x) while CMD:%X", result, mb02_state.cmd_reg);
			mb02_reset_state();
		case MB02_IDLE:
			break;
	}

	return result;
}

//---------------------------------------------------------------------------------------
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
bool mb02_checkfmt(const char *file_name, int *numtrk, int *numsec)
{
	FIL f;
	if (f_open(&f, file_name, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		__TRACE("MB-02 disk open failed\n");
		return false;
	}

	if (!read_file(&f, secBuffer, 0x30)) {
		f_close(&f);
		return false;
	}

	bool result = mb02_check_image(secBuffer);
	f_close(&f);

	if (!result)
		return false;

	*numtrk = secBuffer[0x04];
	*numsec = secBuffer[0x06];

	if (*numtrk < 1 || *numtrk > 255) {
		__TRACE("MB-02 image: invalid number of tracks (%u not in range 1..255)", numtrk);
		result = false;
	}

	if (*numsec < 1 || *numsec > 127) {
		__TRACE("MB-02 image: invalid number of sectors (%u not in range 1..127)", numsec);
		result = false;
	}

	int disk_size = ((*numtrk) * (*numsec)) << 1;
	if (disk_size < 5 || disk_size >= 2047) {
		__TRACE("MB-02 image: invalid disk size (%u not in range 6..2046 kB)", disk_size);
		result = false;
	}

	return result;
}

bool mb02_formatdisk(const char *file_name, int numtrk, int numsec, const char *disk_name)
{
	int i, secbLen = sizeof(secBuffer);
	int disk_size = (numtrk * numsec) << 1;
	int disk_length = disk_size << 10;

	__TRACE("MB-02 CreateDisk: NumTrk:%u, NumSec:%u, DiskSize:%u kB\n", numtrk, numsec, disk_size);

	memset(secBuffer, 0, secbLen);

	secBuffer[0x00] = 0x18; // MB02 disk identification and relative jump to system loader
	secBuffer[0x01] = 0x5E; // Argument for relative jump instruction
	secBuffer[0x02] = 0x80; // High byte of address of system loader (not used)
	secBuffer[0x03] = 0x02; // MB02 disk identification
	secBuffer[0x04] = numtrk; // Total number of tracks
	secBuffer[0x06] = numsec; // Clusters per track
	secBuffer[0x08] = 0x02; // Number of sides
	secBuffer[0x0A] = 0x01; // Sectors per cluster
	secBuffer[0x0C] = 0x02; // DIRS sector

	tm *loctim = NULL;
	RTC_GetTime(loctim);

	// Date & time of creation
	secBuffer[0x21] = (loctim->tm_sec >> 1) | (loctim->tm_min << 5);
	secBuffer[0x22] = (loctim->tm_min >> 3) | (loctim->tm_hour << 3);
	secBuffer[0x23] = ((loctim->tm_mon + 1) << 5) | loctim->tm_mday;
	secBuffer[0x24] = ((loctim->tm_mon + 1) >> 3) | ((loctim->tm_year - 80) << 1);

	// Disk name
	strncpy((char *) secBuffer + 0x26, disk_name, 0x1A);

	srand(Timer_GetTickCounter());

	// Random disk identification
	byte chksum = 0x00;
	for (i = 0; i < 0x20; i++)
		secBuffer[0x40 + i] = secBuffer[0x20 + i] ^ (secBuffer[0x21 + (i & 0x03)]) ^ (rand() & 0xFF);
	for (i = 0; i < 0x20; i++)
		chksum ^= secBuffer[0x40 + i];
	secBuffer[0x16] = chksum;

	// Bootloader effect
	memcpy(secBuffer + 0x60, epromBootstrap + bootEffectOffset, bootEffectLength);

	int secfat = (disk_size >> 9) + 1; // Number of sector per FAT
	int fatsiz = secfat << 10; // Size of FAT
	int fatend = disk_size << 1;
	int fat1sec = 3;
	int fat2sec = 3 + secfat;
	int fat1pos = fat1sec << 10;

	__TRACE("MB-02 CreateDisk: FAT Length: %u, FAT1 start: %u, FAT2 start: %u\n", secfat, fat1sec, fat2sec);

	secBuffer[0x0E] = secfat;
	secBuffer[0x11] = secfat << 2;
	secBuffer[0x12] = fat1sec;
	secBuffer[0x14] = fat2sec;
	for (i = 1; i < secfat; i++) {
		secBuffer[0x18 + (i << 1)] = fat1sec + i;
		secBuffer[0x19 + (i << 1)] = fat2sec + i;
	}

	UINT r;
	FIL dst;
	int res = f_open(&dst, file_name, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK)
		return false;

	res = write_file(&dst, secBuffer, secbLen);
	if (res != secbLen)
		return false;

	memset(secBuffer, 0, secbLen);

	float total = (disk_length / secbLen);
	for (i = 0; i < (disk_length / secbLen) - 1; i++) {
		res = write_file(&dst, secBuffer, secbLen);
		if (res != secbLen)
			return false;

		if (!(i % 4))
			Shell_UpdateProgress(((float) i) / total);
	}

	secBuffer[0x03] = 0xFF; // Backup sector
	secBuffer[0x05] = 0x84; // DIRS   sector

	for (i = 1; i < secfat; i++) {
		secBuffer[(fat1sec << 1) + 1] = 0xC0;
		secBuffer[(fat2sec << 1) + 1] = 0xC0;
		secBuffer[fat1sec << 1] = fat1sec + 1;
		secBuffer[fat2sec << 1] = fat2sec + 1;
		fat1sec++;
		fat2sec++;
	}

	secBuffer[(fat1sec << 1) + 1] = 0x84;
	secBuffer[(fat2sec << 1) + 1] = 0x84;

	chksum = 0x00;
	for (i = 2; i < secbLen; i++)
		chksum += secBuffer[i];
	for (i = fatend; i < fatsiz; i++)
		chksum += 0xFF;

	secBuffer[0x01] = chksum;

	res = f_lseek(&dst, fat1pos);
	if (res != FR_OK)
		return false;

	write_file(&dst, secBuffer, secbLen);

	f_lseek(&dst, fat1pos + fatsiz);
	write_file(&dst, secBuffer, secbLen);

	Shell_UpdateProgress(1.0f);

	secBuffer[0xFF] = 0xFF;
	f_lseek(&dst, fat1pos + fatend);
	for (i = fatend; i < fatsiz; i++)
		f_write(&dst, &secBuffer[0xFF], 1, &r);

	f_lseek(&dst, fat1pos + fatsiz + fatend);
	for (i = fatend; i < fatsiz; i++)
		f_write(&dst, &secBuffer[0xFF], 1, &r);

	f_close(&dst);
	return true;
}
//---------------------------------------------------------------------------------------
