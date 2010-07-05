#ifndef CRC16_H_INCLUDED
#define CRC16_H_INCLUDED

#include "types.h"

word crc_init();
word crc_add( word crc, byte data );

#endif
