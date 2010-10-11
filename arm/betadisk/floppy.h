/*
 *  This file is a part of spec2009 project.
 */

#ifndef __FLOPPY_H
#define __FLOPPY_H

#define MAX_FLOPPIES	4

#ifdef __cplusplus
extern "C"
{
#endif

struct flp_image;

int floppy_open(int drive, const char *filename);
void floppy_close(int drive);

byte floppy_read();
void floppy_write(byte data);
word floppy_stat(void);
void floppy_set_sec_id(byte sec);

int floppy_disk_wp(int drive, int *protect);
int floppy_set_all_wp(int mask);
int floppy_disk_loaded(int drive);
word floppy_byte_time(void);

void floppy_std_params(struct flp_image *img);
void floppy_set_times(struct flp_image *img);

void floppy_dispatch();

void floppy_init(void);
void floppy_set_fast_mode(int fast);

int floppy_leds(void);

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////

#define FLP_DTA_GAP0	0
#define FLP_DTA_INDX	1
#define FLP_DTA_GAP1	2
#define FLP_DTA_IDAM	3
#define FLP_DTA_AM		4
#define FLP_DTA_AMCRC	5
#define FLP_DTA_GAP2	6
#define FLP_DTA_IDDTA	7
#define FLP_DTA_DTA		8
#define FLP_DTA_DTACRC	9
#define FLP_DTA_GAP3	10

#define FLP_STAT_DEL	0x10
#define FLP_STAT_AM     0x20	// 1 - addres mark, 0 - data			// does not need
#define FLP_STAT_EOD    0x40	// end of data, cleared on next sector 	// does not need
#define FLP_STAT_ERR    0x80	// operation error						// does not need

#define FLPO_ERR_OK		0
#define FLPO_ERR_READ	1
#define FLPO_ERR_FORMAT 2

#define AM_SIZE         6

#endif
