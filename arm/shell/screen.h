#ifndef SHELL_SCREEN_H_INCLUDED
#define SHELL_SCREEN_H_INCLUDED

#include "../types.h"
#include "../system/sdram.h"

void ScreenPush();
void ScreenPop();
void ResetScreen(bool active);
void ClrScr(byte attr);
void DrawLine(byte y, byte cy);
void DrawFrame(byte x, byte y, byte w, byte h, byte attr, const char corners[7]);
void DrawChar(byte x, byte y, char c, bool over = false, bool inv = false);
void DrawAttr(byte x, byte y, byte attr, int w = 6, bool incent = false);
void DrawAttr8(byte x, byte y, byte attr, byte n = 1);
void DrawStr(byte x, byte y, const char *str, byte size = 0, bool over = false, bool inv = false);
void DrawStrAttr(byte x, byte y, const char *str, byte attr, byte size = 0, bool over = false, bool inv = false);
void DrawFnKeys(byte x, byte y, const char *fnKeys, int len);
void CycleMark(byte x = 0, byte y = 22);

#endif
