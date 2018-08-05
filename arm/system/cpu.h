#ifndef SYSTEM_CPU_H_INCLUDED
#define SYSTEM_CPU_H_INCLUDED

#include "../system.h"

void CPU_Start();
void CPU_Stop();
bool CPU_Stopped();

void CPU_NMI();
void CPU_Reset(bool res);
void CPU_Reset_Seq();
void CPU_ModifyPC(word pc, byte istate);

#endif
