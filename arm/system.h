// patches to toolchain
// uncomment mthumb-interwork in gcc/config/t-arm-elf
// 	--disable-hosted-libstdcxx
// 	empty crt0.s in newlib/libgloss

#ifndef _SYSTEM_
#define _SYSTEM_

#include "types.h"

#define VERSION "1.2.4"
#define PATH_SIZE 0x80

#ifdef __cplusplus
extern "C" {
#endif
	#include "libstr/inc/75x_lib.h"
	#include "specSerial.h"

	#include "fatfs/diskio.h"
	#include "fatfs/ff.h"

	#include "utils/crc16.h"

	#include <stdio.h>
	#include <string.h>
	#include <time.h>

//-------------------------------------------------------------------------

	void Mouse_Reset();

	void BDI_Routine();
	void BDI_ResetWrite();
	void BDI_Write(byte data);
	void BDI_ResetRead(word counter);
	bool BDI_Read(byte *data);
	void DivMMC_Routine();
	void MB02_Routine();
	void DiskIF_StopTimer();
	void DiskIF_StartTimer();
	void DiskIF_Routine();

	void WDT_Kick();

	void DelayUs(dword);
	void DelayMs(dword);

	#define CNTR_INTERVAL 1
	dword get_ticks();

	void TestStack();
	void portENTER_CRITICAL();
	void portEXIT_CRITICAL();

	void TIM0_UP_IRQHandler(void);

#ifdef __cplusplus
}

	#include "utils/cstring.h"
	#include "utils/fifo.h"
	#include "utils/file.h"
	#include "system/bus.h"
	#include "system/core.h"
	#include "system/cpu.h"
	#include "system/fpga.h"
	#include "system/uarts.h"

	void Timer_Routine();
	dword Timer_GetTickCounter();

	void Spectrum_UpdateConfig(bool hardReset = false);
	void Spectrum_UpdateDisks();
#endif

#endif
