/*
 *  This file is a part of spec2009 project.
 *
 *  FDC
 *
 */

#include "stdinc.h"
#include "fdc.h"
#include "floppy.h"
#include "bdi.h"

#if FDC_PORTABLE

///////////////////////////////////////////////////////////////////////

static struct FDC PROGMEM_PTR * p_fdc;

///////////////////////////////////////////////////////////////////////

void fdc_init(void)
{
	p_fdc = &bdi;

	p_fdc->init();
	floppy_init();
}

void fdc_open_images(void)
{
    char img_file[128];
    int i;
	
    dword flags;
	if (cfg_read(CFG_FLAGS, (char *)&flags) <= 0) {
		flags = 0;
	}
	for (i = 0; i < 4; i++) {
		if (cfg_read(CFG_DRVA_FILENAME + i, img_file) > 0) {
			TRACE(("sys: %d - %s\n", i, img_file));
			int wp = 0;
			floppy_open(i, img_file);
			wp = (flags & (1 << i)) != 0;
			floppy_disk_wp(i, &wp);
		}
	}
	floppy_set_fast_mode((flags & CFG_FLAGS_FASTFLP) != 0);
}

void fdc_close_images(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		floppy_close(i);
	}
}

///////////////////////////////////////////////////////////////////////

void _fdc_set(int id, int val)
{
	(*p_fdc->set)(id, val);
}

int fdc_query(int id)
{
	return (*p_fdc->query)(id);
}

void fdc_write(word addr, byte data)
{
	(*p_fdc->write)(addr, data);
}

byte fdc_read(word addr)
{
	return (*p_fdc->read)(addr);
}

void fdc_reset(void)
{
    TRACE(("fdc: reset_n = 0\n"));
	(*p_fdc->set)(FDC_RESET, 0);
	p_fdc->dispatch();
    TRACE(("fdc: reset_n = 1\n"));
	(*p_fdc->set)(FDC_RESET, 1);
}

///////////////////////////////////////////////////////////////////////

void fdc_dispatch(void)
{
	floppy_dispatch();
	p_fdc->dispatch();
}

///////////////////////////////////////////////////////////////////////

#endif
