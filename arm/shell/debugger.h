#ifndef SHELL_DEBUGGER_H_INCLUDED
#define SHELL_DEBUGGER_H_INCLUDED

#include "../types.h"

void Shell_Debugger();
byte ReadByteAtCursor(word addr);
void WriteByteAtCursor(word addr, byte data);

char *Shell_DebugDisasm(byte *cmd, unsigned current, int *length);

#endif
