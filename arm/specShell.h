#ifndef SPEC_SHELL_H_INCLUDED
#define SPEC_SHELL_H_INCLUDED

#include "system.h"

void CPU_Start();
void CPU_Stop();
bool CPU_Stopped();

void CPU_Reset( bool res );
void CPU_ModifyPC( word pc, byte istate );

void Shell_Enter();
const char *Shell_GetPath();

#endif





