/*
 *  This file is a part of spec2009 project.
 */

#ifndef __FDC_H
#define __FDC_H

#define FDC_DRVSEL	0
#define FDC_RESET	2
#define FDC_HLT		3
#define FDC_SIDE	4

#define FDC_INDEX	5
#define FDC_DRQ		6
#define FDC_INTRQ	7
#define FDC_HLD		9
#define FDC_MFM		8
#define FDC_SDIR	10
#define FDC_STEP	11
#define FDC_READY	12
#define FDC_TRK0	13
#define FDC_WP		14

#if defined(FDC_PORTABLE)
#define fdc_set(id, val) _fdc_set(id, val)
#else
#define fdc_set(id, val) do { if (id != FDC_SIDE && id != FDC_HLT) _fdc_set(id, val); } while (0);
#endif

typedef void (*fdc_write_t)(word,byte);
typedef byte (*fdc_read_t)(word);
typedef void (*fdc_proc_t)(void);
typedef void (*fdc_ctl_t)(byte,byte*);

typedef int (*fdc_query_t)(int);
typedef void (*fdc_set_t)(int,int);


struct FDC {
	fdc_proc_t init;
	fdc_proc_t dispatch;

	fdc_write_t write;
	fdc_read_t read;

    fdc_set_t set;
    fdc_query_t query;
};

#ifdef __cplusplus
extern "C"
{
#endif

void _fdc_set(int id, int val);
int fdc_query(int id);

void fdc_init(void);
void fdc_open_images(void);
void fdc_close_images(void);
bool fdc_open_image( int drv, const char *img_file );
void fdc_close_image( int drv );
void fdc_dispatch(void);
void fdc_write(word addr, byte data);
byte fdc_read(word addr);
void fdc_reset(void);

#ifdef __cplusplus
}
#endif

#endif
