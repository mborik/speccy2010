#ifndef SHELL_SCREEN_H_INCLUDED
#define SHELL_SCREEN_H_INCLUDED

#include "../types.h"
#include "../system/sdram.h"

void ScreenPush();
void ScreenPop();
void ResetScreen(bool active);
void WriteDisplay(word address, byte data);
void ClrScr(byte attr);
void DrawLine(byte y, byte cy);
void DrawFrame(byte x, byte y, byte w, byte h, byte attr, const char corners[7]);
void WriteChar(byte x, byte y, char c);
void WriteAttr(byte x, byte y, byte attr, byte n = 1);
void WriteStr(byte x, byte y, const char *str, byte size = 0);
void WriteStrAttr(byte x, byte y, const char *str, byte attr, byte size = 0);
void CycleMark(byte x = 31, byte y = 22);

#endif
