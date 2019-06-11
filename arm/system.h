// patches to toolchain
// uncomment mthumb-interwork in gcc/config/t-arm-elf
// 	--disable-hosted-libstdcxx
// 	empty crt0.s in newlib/libgloss

#ifndef _SYSTEM_
#define _SYSTEM_

#include "types.h"

#define VERSION "MEMTEST"
#define PATH_SIZE 0x80

#ifdef __cplusplus
extern "C" {
#endif
	#include "libstr/inc/75x_lib.h"

	#include "fatfs/diskio.h"
	#include "fatfs/ff.h"

	#include <stdio.h>
	#include <string.h>
	#include <time.h>

//-------------------------------------------------------------------------

	void WDT_Kick();
	void DelayUs(dword);
	void DelayMs(dword);

	#define CNTR_INTERVAL 1
	dword get_ticks();

	void __TRACE(const char *str, ...);
	void portENTER_CRITICAL();
	void portEXIT_CRITICAL();

	void TIM0_UP_IRQHandler(void);

#ifdef __cplusplus
}

	#include "system/bus.h"
	#include "system/core.h"
	#include "system/cpu.h"
	#include "system/fpga.h"
	#include "system/uarts.h"

	void Timer_Routine();
	dword Timer_GetTickCounter();
#endif

#endif
