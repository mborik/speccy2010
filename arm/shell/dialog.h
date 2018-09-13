#ifndef SHELL_DIALOG_H_INCLUDED
#define SHELL_DIALOG_H_INCLUDED

#include "../types.h"
#include "../utils/cstring.h"

enum { MB_PROGRESS, MB_OK, MB_YESNO, MB_DISK };

void Shell_Window(int x, int y, int w, int h, const char *title, byte attr);
void Shell_Toast(const char *str, const char *str2 = NULL, byte attr = 0137, int timeout = 2000);
bool Shell_MessageBox(const char *title, const char *str, const char *str2 = "", const char *str3 = "", int type = MB_OK, byte attr = 0127, byte attrSel = 0152);
bool Shell_InputBox(const char *title, const char *str, CString &buff);
void Shell_ProgressBar(const char *title, const char *str, byte attr = 0070);
void Shell_UpdateProgress(float progress, byte attr = 0062);

#endif
