#ifndef SHELL_DEBUGGER_H_INCLUDED
#define SHELL_DEBUGGER_H_INCLUDED

#include "../types.h"

#define COL_BACKGND 0054
#define COL_NORMAL  0007
#define COL_ACTIVE  0017
#define COL_CURSOR  0030
#define COL_EDITOR  0141
#define COL_TITLE   0151
#define COL_BREAKPT 0002
#define COL_SIDEPAN 0140
#define COL_SIDE_HI 0141

#ifdef __cplusplus
extern "C"
{
#endif

enum { WIN_TRACE, WIN_MEMDUMP };
enum {
	REG_AF  =  0, REG_AF_ =  1,
	REG_IR  =  2, REG_SP  =  3, REG_PC  =  4,
	REG_BC  =  5, REG_DE  =  6, REG_HL  =  7, REG_IX  =  8,
	REG_BC_ =  9, REG_DE_ = 10, REG_HL_ = 11, REG_IY  = 12,
	REG_IM  = 13, REG_IFF = 14, REG_FLG = 15
};

#ifdef __cplusplus
}
#endif

void Shell_Debugger();
byte ReadByteAtCursor(word addr);
void WriteByteAtCursor(word addr, byte data);

bool IsRegPanel();
word GetCPUState(byte reg);
void SetCPUState(byte reg, word value);

void Debugger_UpdateTrace();
bool Debugger_TestKeyTrace(byte key, bool *updateAll);
void Debugger_RefreshRequested(bool firstTime = false);
void Debugger_HandleBreakpoint();
void Debugger_Init();

#endif
