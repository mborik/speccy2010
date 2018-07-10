#ifndef SPEC_MB02_H_INCLUDED
#define SPEC_MB02_H_INCLUDED

#include "types.h"

void mb02_init();

int  mb02_open_image(byte drv_id, const char *filename);
void mb02_close_image(byte drv_id);

void mb02_write_port(byte port, byte data);
byte mb02_read_port(byte port);

byte mb02_get_state();
byte mb02_cur_drv();
int  mb02_leds();

byte mb02_is_disk_wp(byte drv);
void mb02_set_disk_wp(byte drv, byte wp);

byte mb02_is_disk_loaded(byte drv);

#endif
