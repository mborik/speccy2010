#include <stdarg.h>
#include <ctype.h>

#include "system.h"
#include "utils/file.h"
#include "utils/crc16.h"
#include "specMB02.h"
#include "specConfig.h"

#define RDCRC_TYPEI 0

#define DRAW_SEEK(x)
#define TRACE(x) // __TRACE x

#define SYS_NUM_OF_TRYOUTS 64
#define SEC_BUF_SIZE       16

#define MB02_DRIVES_COUNT  4

#define MB02_PORT_CMD      0x53
#define MB02_PORT_STA      0x53
#define MB02_PORT_DTA      0x73

#define STAT_NOTRDY        0x82  // drive not ready
#define STAT_WP            0x40  // disk write protected
#define STAT_BREAK         0x20  // break
#define STAT_SEEKERR       0x10  // record not found
#define STAT_NOTFOUND      0x10  // record not found
#define STAT_RDCRC         0x08
#define STAT_HEAD_CYL_0    0x04  // head on cyl 0
#define STAT_LOST          0x04  // data underrun
#define STAT_TIMEOUT       0x01  // operation in progress


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

	unsigned char wp : 1;
	unsigned char pr : 1;

	byte sec_cnt;
	byte cyl_cnt;
	byte side_cnt;
	word sec_size;
	byte cur_cyl;
	byte cur_sec;

	dword img_trkoff; // offset of the track from file begin
	word sec_off; // offset from begin of track
	word next_sec_off;
};

struct mb02_drive mb02_drives[MB02_DRIVES_COUNT];
struct mb02_drive *mb02_drv;

// state & wd1793 registers
struct mb02_state {
	byte cmd_reg;
	byte cur_side;
	byte state;
} mb02_state;

void mb02_dsk_init();
void mb02_init_drive(byte drv_id);
void mb02_clear_drive(byte drv_id);

//---------------------------------------------------------------------------------------

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

	mb02_drives[drv_id].sec_cnt = bootsec[6];
	mb02_drives[drv_id].sec_size = 1024;

	if (mb02_drives[drv_id].pr) {
		f_close(&mb02_drives[drv_id].fd);
	}
	memcpy(&mb02_drives[drv_id].fd, &f, sizeof(FIL));

	mb02_drives[drv_id].wp = ((fi.fattrib & AM_RDO) != 0) ? 1 : 0;
	mb02_drives[drv_id].pr = 1;
	mb02_drives[drv_id].side_cnt = bootsec[8];
	mb02_drives[drv_id].cyl_cnt = bootsec[4];

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
byte mb02_lookup_track()
{
	if (!mb02_drv->pr) {
		mb02_drv->sec_cnt = 0;
		return 0;
	}
	TRACE(("lookup_track: cur_cyl = %u, cyl_cnt = %u, cur_side=%u\n", mb02_drv->cur_cyl, mb02_drv->cyl_cnt, mb02_state.cur_side));
	if (mb02_drv->cur_cyl >= mb02_drv->cyl_cnt) {
		return 0;
	}
	mb02_drv->img_trkoff = ((dword)mb02_drv->cur_cyl) * ((dword)mb02_drv->sec_cnt) * ((dword)mb02_drv->sec_size) * ((dword)mb02_drv->side_cnt);
	TRACE(("lookup_track: trk=%x\n", mb02_drv->img_trkoff));
	return 1;
}

byte mb02_lookup_sector(byte sec, byte side, bool side_check = false)
{
	if (!mb02_drv->pr) {
		return 0;
	}
	TRACE(("lookup_sector: %u, c = %u, s = %u\n", sec, side_check, side));
	if (sec > mb02_drv->sec_cnt || sec == 0) {
		return 0;
	}
	if (side_check && side != mb02_state.cur_side) {
		return 0;
	}
	mb02_drv->sec_off = (((word) mb02_state.cur_side) * ((word)mb02_drv->sec_cnt) + sec - 1) * ((word)mb02_drv->sec_size);
	mb02_drv->cur_sec = sec;
	mb02_drv->next_sec_off = mb02_drv->sec_off + mb02_drv->sec_size;
	TRACE(("lookup_sector: data off = %x (r=%x,next_off=%x)\n", mb02_drv->sec_off, mb02_drv->sec_size, mb02_drv->next_sec_off));
	f_lseek(&mb02_drv->fd, mb02_drv->img_trkoff + (dword)mb02_drv->sec_off);
	return 1;
}

byte mb02_lookup_next_sector()
{
	if (!mb02_drv->pr) {
		return 0;
	}
	if (mb02_drv->cur_sec == mb02_drv->sec_cnt) {
		mb02_drv->cur_sec = 1;
		mb02_lookup_sector(1, 0);
	}
	else {
		mb02_lookup_sector(mb02_drv->cur_sec + 1, 0);
	}
	return mb02_drv->cur_sec;
}
//---------------------------------------------------------------------------------------
void mb02_clear_drive(byte drv_id)
{
	mb02_init_drive(drv_id);

	mb02_drives[drv_id].sec_cnt = 0;
	mb02_drives[drv_id].cyl_cnt = 0;
	mb02_drives[drv_id].pr = 0;
}

void mb02_init_drive(byte drv_id)
{
	TRACE(("mb02_init_drive\n"));

	mb02_drives[drv_id].wp = 0;
	mb02_drives[drv_id].cur_sec = 1;
	mb02_drives[drv_id].cur_cyl = 0;
	mb02_drives[drv_id].img_trkoff = 0;
	mb02_drives[drv_id].sec_off = 0;
	mb02_drives[drv_id].next_sec_off = 0;
}

void mb02_dsk_init()
{
}

void mb02_init()
{
	int i;

	for (i = 0; i < MB02_DRIVES_COUNT; i++) {
		mb02_clear_drive(i);
	}

	mb02_state.state = MB02_IDLE;
	mb02_drv = mb02_drives + 0;
	mb02_state.cur_side = 0;
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

int mb02_leds()
{
	int result = 0;
	// TODO
	return result;
}
//---------------------------------------------------------------------------------------
