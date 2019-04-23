#include "dialog.h"
#include "screen.h"
#include "../specKeyboard.h"

#define PROGRESSBAR_SIZE 32
char progressBarBuffer[PROGRESSBAR_SIZE + 1];
//---------------------------------------------------------------------------------------
void Shell_Window(int x, int y, int w, int h, const char *title, byte attr)
{
	DrawFrame(x, y, w, h, attr, "\xC9\xCD\xBB\xBA\xC8\xCD\xBC");

	if (title != NULL && title[0] != 0) {
		int titleSize = strlen(title) * 6;
		int titlePos = x + ((w - titleSize) / 2);

		DrawChar(titlePos - 6, y, ' ');
		DrawStr(titlePos, y, title);
		DrawChar(titlePos + titleSize, y, ' ');
	}
}
//---------------------------------------------------------------------------------------
void Shell_Toast(const char *str, const char *str2, byte attr, int timeout)
{
	int w = 72;
	int h = 3;

	int len = (strlen(str) + 4) * 6;
	if (w < len)
		w = len;

	if (str2 != NULL && str2[0] != 0) {
		len = (strlen(str2) + 2) * 6;
		if (w < len)
			w = len;
		h++;
	}

	int x = (256 - w) / 2;
	int y = ((22 - h) / 2) + 5;

	byte mod = (x & 7);
	if (mod > 0) {
		w  = mod + ((((x + w) | 7) + 1) - x);
		x -= mod;
	}

	ScreenPush();
	Shell_Window(x, y++, w, h, NULL, attr);

	len = strlen(str) * 6;
	DrawStr(x + ((w - len) / 2), y++, str);
	if (str2 != NULL && str2[0] != 0) {
		len = strlen(str2) * 6;
		DrawStr(x + ((w - len) / 2), y, str2);
	}

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
	int w = 72;
	int h = 3;

	int len = (strlen(title) + 6) * 6;
	if (w < len)
		w = len;
	len = (strlen(str) + 2) * 6;
	if (w < len)
		w = len;

	if (str2[0] != 0) {
		len = (strlen(str2) + 2) * 6;
		if (w < len)
			w = len;
		h++;

		if (str3[0] != 0) {
			len = (strlen(str3) + 2) * 6;
			if (w < len)
				w = len;
			h++;
		}
	}

	if (type != MB_PROGRESS)
		h++;

	int x = (256 - w) / 2;
	int y = (22 - h) / 2;
	int xl = x + 6;

	byte mod = (x & 7);
	if (mod > 0) {
		w  = mod + ((((x + w) | 7) + 1) - x);
		x -= mod;
	}

	ScreenPush();
	Shell_Window(x - 8, y - 1, w + 16, h + 2, title, attr);

	y++;
	DrawStr(xl, y++, str);
	if (str2 && str2[0] != 0) {
		DrawStr(xl, y++, str2);
		if (str3 && str3[0] != 0)
			DrawStr(xl, y++, str3);
	}

	if (type == MB_PROGRESS)
		return true;

	bool result = true;
	while (true) {
		if (type == MB_OK) {
			DrawStrAttr(128 - 12, y, " OK ", attrSel);
		}
		else if (type == MB_DISK) {
			DrawStrAttr(128 - 31, y, " MBD ", result ? attrSel : attr);
			DrawStrAttr(128 +  1, y, " TRD ", result ? attr : attrSel);
		}
		else {
			DrawStrAttr(128 - 31, y, " Yes ", result ? attrSel : attr);
			DrawStrAttr(128 +  4, y, " No  ", result ? attr : attrSel);
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
	int w = 192;
	int h = 4;

	int len = (strlen(title) + 6) * 6;
	if (w < len)
		w = len;
	len = (strlen(str) + 2) * 6;
	if (w < len)
		w = len;

	int x = (256 - w) / 2;
	int y = (22 - h) / 2;

	byte mod = (x & 7);
	if (mod > 0) {
		w  = mod + ((((x + w) | 7) + 1) - x);
		x -= mod;
	}

	ScreenPush();
	Shell_Window(x - 8, y - 1, w + 16, h + 2, title, 0050);

	x += 8;
	w -= 16;

	DrawStr(x, ++y, str);
	DrawAttr(x, ++y, 0150, w);

	len = (w / 6);
	x += (w - (len * 6)) / 2;
	w = len;

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

		for (int i = 0; i < w; i++)
			DrawChar(x + (i * 6), y, buff.GetSymbol(s + i), false, (i == (p - s)));

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
	for (i = 0; i < PROGRESSBAR_SIZE; i++)
		progressBarBuffer[i] = 0xB0; // shade block
	progressBarBuffer[PROGRESSBAR_SIZE] = 0;

	Shell_MessageBox(title, str, progressBarBuffer, "", MB_PROGRESS, attr);

	for (i = 0; i < PROGRESSBAR_SIZE; i++)
		progressBarBuffer[i]++;
}
//---------------------------------------------------------------------------------------
void Shell_UpdateProgress(float progress, byte attr)
{
	byte prog = (progress * PROGRESSBAR_SIZE);
	if (prog > 0)
		DrawStr(32, 11, progressBarBuffer, prog);

	WDT_Kick();
}
//---------------------------------------------------------------------------------------
