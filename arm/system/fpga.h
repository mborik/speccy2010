#ifndef SYSTEM_FPGA_H_INCLUDED
#define SYSTEM_FPGA_H_INCLUDED

#include "../system.h"

const dword FPGA_NONE = 0x00000000;
const dword FPGA_SPECCY2010 = 0x00000001;

void FPGA_Config();

extern dword fpgaConfigVersionPrev;
extern dword fpgaStatus;

extern dword romConfigPrev;

extern byte timer_flag_1Hz;
extern byte timer_flag_100Hz;

#endif
