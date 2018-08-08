#ifndef SPEC_MB02_H_INCLUDED
#define SPEC_MB02_H_INCLUDED

#include "types.h"

void mb02_init();
void mb02_fill_eprom();

int  mb02_open_image(byte drv_id, const char *filename);
void mb02_close_image(byte drv_id);

void mb02_received(byte value);
byte mb02_transmit();

byte mb02_is_disk_wp(byte drv);
void mb02_set_disk_wp(byte drv, bool wp);

byte mb02_is_disk_loaded(byte drv);

#endif
