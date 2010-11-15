//  patches to toolchain
// uncomment mthumb-interwork in gcc/config/t-arm-elf
// 	--disable-hosted-libstdcxx
// 	empty crt0.s

#ifndef _SYSTEM_
#define _SYSTEM_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    #include "75x_lib.h"
    #include "7xx_flash.h"
    void WDT_Kick();
    void __TRACE( const char *str );

#ifdef __cplusplus
}
#endif

#endif


