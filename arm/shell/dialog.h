#ifndef SHELL_DIALOG_H_INCLUDED
#define SHELL_DIALOG_H_INCLUDED

#include "../types.h"
#include "../utils/cstring.h"

enum { MB_NO, MB_OK, MB_YESNO };

void Shell_Window(int x, int y, int w, int h, const char *title, byte attr);
bool Shell_MessageBox(const char *title, const char *str, const char *str2 = "", const char *str3 = "", int type = MB_OK, byte attr = 027, byte attrSel = 052);
bool Shell_InputBox(const char *title, const char *str, CString &buff);

#endif
