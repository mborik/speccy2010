/*
 *  This file is a part of spec2009 project.
 */

#ifndef TYPES_H_D
#define TYPES_H_D

#include "../types.h"

/*
#define false (0)
#define true (!0)

#if defined(PIC_CTRL)
typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;
typedef unsigned char bool;
#else
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned char bool;
#endif

#if !defined(min)
#define min( a, b ) ( ( a ) < ( b ) ? a : b )
#endif
*/

#if defined(__GNUC__)
#define PACKED __attribute__((packed))
#define WEAK __attribute__((weak))
#else
#define PACKED
#define WEAK
#endif

#if defined(PIC_CTRL)
#define PROGMEM __attribute__((space(auto_psv)))
#define PROGMEM_PTR
#else
#define PROGMEM
#define PROGMEM_PTR
#endif

#endif
