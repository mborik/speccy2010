#ifndef SYSTEM_FPGA_H_INCLUDED
#define SYSTEM_FPGA_H_INCLUDED

#include "../system.h"

void FPGA_Config();

extern dword fpgaConfigVersionPrev;
extern dword romConfigPrev;

extern byte timer_flag_1Hz;
extern byte timer_flag_100Hz;

#endif
