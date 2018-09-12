#include "dialog.h"
#include "screen.h"
#include "../specKeyboard.h"

#define PROGRESSBAR_SIZE 24
char progressBarBuffer[PROGRESSBAR_SIZE + 1];
//---------------------------------------------------------------------------------------
void Shell_Window(int x, int y, int w, int h, const char *title, byte attr)
{
	DrawFrame(x, y, w, h, attr, "\xC9\xCD\xBB\xBA\xC8\xCD\xBC");

	if (title != NULL && title[0] != 0) {
		int titleSize = strlen(title);
		int titlePos = x + ((w - titleSize) / 2);

		WriteChar(titlePos - 1, y, ' ');
		WriteStrAttr(titlePos, y, title, attr);
		WriteChar(titlePos + titleSize, y, ' ');
	}
}
//---------------------------------------------------------------------------------------
void Shell_Toast(const char *str, const char *str2, byte attr, int timeout)
{
	int w = 12;
	int h = 3;

	if (w < (int) strlen(str) + 4)
		w = strlen(str) + 4;

	if (str2 != NULL && str2[0] != 0) {
		if (w < (int) strlen(str2) + 2)
			w = strlen(str2) + 2;
		h++;
	}

	int x = (32 - w) / 2;
	int y = ((22 - h) / 2) + 5;

	ScreenPush();
	Shell_Window(x, y++, w, h, NULL, attr);

	WriteStr(x + ((w - strlen(str)) / 2), y++, str);
	if (str2 != NULL && str2[0] != 0)
		WriteStr(x + ((w - strlen(str2)) / 2), y, str2);

	DelayMs(500);
	timeout -= 500;

	while (timeout > 0) {
		if (ReadKeySimple(true))
			break;

		DelayMs(10);
		timeout -= 10;
	}

	ScreenPop();
}
//---------------------------------------------------------------------------------------
bool Shell_MessageBox(const char *title, const char *str, const char *str2, const char *str3, int type, byte attr, byte attrSel)
{
	int w = 12;
	int h = 3;

	if (w < (int) strlen(title) + 6)
		w = strlen(title) + 6;
	if (w < (int) strlen(str) + 2)
		w = strlen(str) + 2;

	if (str2[0] != 0) {
		if (w < (int) strlen(str2) + 2)
			w = strlen(str2) + 2;
		h++;

		if (str3[0] != 0) {
			if (w < (int) strlen(str3) + 2)
				w = strlen(str3) + 2;
			h++;
		}
	}

	if (type != MB_PROGRESS)
		h++;

	int x = (32 - w) / 2;
	int y = (22 - h) / 2;

	ScreenPush();
	Shell_Window(x - 1, y - 1, w + 2, h + 2, title, attr);
	x++;
	y++;

	WriteStr(x, y++, str);
	if (str2 && str2[0] != 0) {
		WriteStr(x, y++, str2);
		if (str3 && str3[0] != 0)
			WriteStr(x, y++, str3);
	}

	if (type == MB_PROGRESS)
		return true;
	else if (type == MB_RECEIVE) {
		WriteStrAttr(16 - 3, ++y, " Done ", attrSel);
		return true;
	}

	bool result = true;
	while (true) {
		if (type == MB_OK) {
			WriteStrAttr(16 - 2, y, " OK ", attrSel);
		}
		else if (type == MB_DISK) {
			WriteStrAttr(16 - 5, y, " MBD ", result ? attrSel : attr);
			WriteStrAttr(16, y, " TRD ", result ? attr : attrSel);
		}
		else {
			WriteStrAttr(16 - 5, y, " Yes ", result ? attrSel : attr);
			WriteStrAttr(16, y, " No ", result ? attr : attrSel);
		}

		byte key = GetKey();

		if (key == K_ESC)
			result = false;
		if (key == K_ESC || key == K_RETURN)
			break;

		if (type > MB_OK)
			result = !result;
	}

	ScreenPop();
	return result;
}
//---------------------------------------------------------------------------------------
bool Shell_InputBox(const char *title, const char *str, CString &buff)
{
	int w = 22;
	int h = 4;

	if (w < (int) strlen(title) + 6)
		w = strlen(title) + 6;
	if (w < (int) strlen(str) + 2)
		w = strlen(str) + 2;

	int x = (32 - w) / 2;
	int y = (22 - h) / 2;

	ScreenPush();
	Shell_Window(x - 1, y - 1, w + 2, h + 2, title, 0050);

	x++;
	y++;
	w -= 2;

	WriteStr(x, y++, str);
	WriteAttr(x, y, 0150, w);

	bool result = false;
	int p = buff.Length();
	int s = 0;

	while (true) {
		while ((p - s) >= w)
			s++;
		while ((p - s) < 0)
			s--;
		while (s > 0 && (s + w - 1) > buff.Length())
			s--;

		for (int i = 0; i < w; i++) {
			WriteChar(x + i, y, buff.GetSymbol(s + i));
			WriteAttr(x + i, y, i == (p - s) ? 0106 : 0150);
		}

		char key = GetKey();

		if (key == K_RETURN) {
			result = true;
			break;
		}
		else if (key == K_ESC)
			break;
		else if (key == K_LEFT && p > 0)
			p--;
		else if (key == K_RIGHT && p < buff.Length())
			p++;
		else if (key == K_HOME)
			p = 0;
		else if (key == K_END)
			p = buff.Length();
		else if (key == K_BACKSPACE && p > 0)
			buff.Delete(--p, 1);
		else if (key == K_DELETE && p < buff.Length())
			buff.Delete(p, 1);
		else if ((byte) key >= 0x20)
			buff.Insert(p++, key);
	}

	ScreenPop();
	return result;
}
//---------------------------------------------------------------------------------------
void Shell_ProgressBar(const char *title, const char *str, byte attr)
{
	int i;
	for (i = 0; i < 24; i++)
		progressBarBuffer[i] = 0xB0; // shade block
	progressBarBuffer[24] = 0;

	Shell_MessageBox(title, str, progressBarBuffer, "", MB_PROGRESS, attr);

	for (i = 0; i < 24; i++)
		progressBarBuffer[i]++;
}
//---------------------------------------------------------------------------------------
void Shell_UpdateProgress(float progress, byte attr)
{
	byte prog = (progress * 24);
	if (prog > 0)
		WriteStrAttr(4, 11, progressBarBuffer, attr, prog);

	WDT_Kick();
}
//---------------------------------------------------------------------------------------
