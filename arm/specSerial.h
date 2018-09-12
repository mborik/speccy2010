#ifndef SPEC_SERIAL_H_INCLUDED
#define SPEC_SERIAL_H_INCLUDED

#include "fatfs/ff.h"

void Serial_Routine();
void Serial_InitReceiver(FIL *dest);
dword Serial_ReceiveBytes(FIL *dest, int msDelay = 1000);
dword Serial_CloseReceiver(FIL *dest);

void __TRACE(const char *str, ...);

#endif
