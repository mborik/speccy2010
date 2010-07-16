/*
 *  This file is a part of spec2009 project.
 *
 *  betadisk
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "bdi.h"
#include "wd1793.h"
#include "floppy.h"
#include "fdc.h"
#include "dskdebug.h"

#define BDI_PORT_WD_A1A0(x)		(((x) & 0x60) >> 5)
#define BDI_PORT(x)				((x) & 0x80)

#if BDI_TRACE_FLAG
#define BDI_TRACE(x)			TRACE(x)
#else
#define BDI_TRACE(x)
#endif

#if BDISQ_TRACE_FLAG
#define BDISQ_TRACE(x)			TRACE(x)
#else
#define BDISQ_TRACE(x)
#endif

#if !FDC_PORTABLE
#define bdi_write 		fdc_write
#define bdi_read 		fdc_read
#define bdi_set			_fdc_set
#define bdi_query		fdc_query
#define bdi_reset		fdc_reset
#define bdi_dispatch    fdc_dispatch
#define bdi_init		fdc_init
#endif

///////////////////////////////////////////////////////////////////////

static sysword_t flags;

///////////////////////////////////////////////////////////////////////

void bdi_write(word port, byte data)
{
	BDI_TRACE(("bdi: write port 0x%02x, 0x%02x (%s)\n", port, data, bdi_port_desc(port)));

	if (BDI_PORT(port)) {
		flags = (flags & ~BDI_SYS_MSK) | (data & BDI_SYS_MSK);
		if (data & BDI_SYS_MFM) flags |= (1 << FDC_MFM);
		else flags &= ~(1 << FDC_MFM);
		BDI_TRACE(("bdi: sys port %s\n", bdi_sys_ctl_decode(data)));

	} else {
		wd17xx_write(BDI_PORT_WD_A1A0(port), data);
	}
}

byte bdi_read(word port)
{
	byte data;

	BDI_TRACE(("bdi: read port 0x%02x (%s)\n", port, bdi_port_desc(port)));

	if (BDI_PORT(port)) {
		data = flags & (BDI_STAT_INTRQ | BDI_STAT_DRQ);
		BDI_TRACE(("bdi: stat %s\n", bdi_sys_stat_decode(data)));
		return data;
	} else {
		data = wd17xx_read(BDI_PORT_WD_A1A0(port));
	}
	return data;
}

///////////////////////////////////////////////////////////////////////

void bdi_set(int id, int val)
{
#if FDC_PORTABLE
	if (id == FDC_SIDE && id == FDC_HLT) {
		return;
	}
#endif
	val = val != 0;
	flags = (flags & ~(1 << id)) | (val << id);
}

int bdi_query(int id)
{
	if (id == FDC_INDEX && (flags & (1 << FDC_HLT)) == 0) {
		return 1;
	} else if (id == FDC_DRVSEL) {
		return flags & 3;
	} else {
		return (flags & (1 << id)) != 0;
	}
}

///////////////////////////////////////////////////////////////////////

void bdi_init(void)
{
	flags = (1 << FDC_RESET);;
	wd17xx_init();
	floppy_init();
}

///////////////////////////////////////////////////////////////////////

void bdi_dispatch(void)
{
	floppy_dispatch();
	wd17xx_dispatch();
}

void bdi_reset(void)
{
    fdc_set(FDC_RESET, 0);
    wd17xx_dispatch();
	fdc_set(FDC_RESET, 1);
}

///////////////////////////////////////////////////////////////////////

#if !FDC_PORTABLE

bool fdc_open_image( int drv, const char *img_file )
{
    if( floppy_open( drv, img_file ) != FLPO_ERR_OK ) return false;

    int wp = 0;
    floppy_disk_wp( drv, &wp );

	return true;
}

void fdc_close_image( int drv )
{
    floppy_close( drv );
}

void fdc_open_images(void)
{
}

void fdc_close_images(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		floppy_close(i);
	}
}

#else

struct FDC
PROGMEM
bdi = {
	init: bdi_init,
	dispatch: bdi_dispatch,

	write: bdi_write,
	read: bdi_read,

	set: bdi_set,
	query: bdi_query,
};

#endif
