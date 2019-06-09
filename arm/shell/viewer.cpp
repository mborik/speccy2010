#include <math.h>

#include "viewer.h"
#include "screen.h"
#include "dialog.h"
#include "textReader.h"
#include "../utils/dirent.h"
#include "../specKeyboard.h"

int pos = 0;
bool wrapLines = false;
bool allowFileEdit = false;
//---------------------------------------------------------------------------------------
void Shell_HelpViewer(bool running)
{
	static int helpPos = 0;

	if (running) {
		CPU_Stop();
		SystemBus_Write(0xc00020, 0); // use bank 0

		ClrScr(0117);

		SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
		SystemBus_Write(0xc00022, 0x8000 | 0); // Enable shell border
	}

	pos = helpPos;
	Shell_TextViewer("speccy2010.hlp", true, "Controls & Keymapping");
	helpPos = pos;

	if (running) {
		SystemBus_Write(0xc00021, 0); //armVideoPage
		SystemBus_Write(0xc00022, 0); //armBorder

		CPU_Start();
	}
}
//---------------------------------------------------------------------------------------
void Shell_TextViewer(const char *fullName, bool simpleMode, const char *description)
{
	bool wrap = wrapLines;
	CTextReader reader(fullName, (simpleMode || wrap) ? SCREEN_WIDTH : -1);
	if (reader.GetLines() < 0) {
		Shell_MessageBox("Error", "Cannot open file!", fullName);
		return;
	}

	ClrScr(0117);
	DrawLine(1, 2);
	DrawLine(1, 4);

	DrawAttr8(0, 0, simpleMode ? 0114 : 0115, 32);

	if (simpleMode) {
		DrawAttr8(0, 23, 0006, 32);

		DrawStr(2, 0, "Speccy2010 v" VERSION " \7 ");
		DrawStr(122, 0, description ? description : "Simple Textfile Viewer", 22);

		display_path(fullName, -6, 23, 36);

		wrap = true;
	}
	else {
		DrawAttr8(0, 23, 0005, 7);
		DrawAttr8(7, 23, 0006, 25);

		display_path(fullName, 0, 0, 42);

restart:
		DrawFnKeys(1, 23, "2wrap4hex", 9);
		if (wrap)
			DrawFnKeys(1, 23, "2unwr", 5);

		sniprintf(mem.sname, 12, "%d", reader.GetFileSize());
		DrawStr(96, 23, mem.sname);
		DrawStr(176, 23, "col 1");

		if (reader.GetLines() >= reader.LINES_MAX)
			Shell_MessageBox("Error", "Too many lines!");

		pos = 0;
	}

	int hshift = 0;
	int maxPos = max(0, reader.GetLines() - VIEWER_LINES);

	byte key = 0;
	while (true) {
		int p = 100;
		if (maxPos > 0)
			p = pos * 100 / maxPos;

		sniprintf(mem.sname, 5, "%3d%%", p);
		DrawStr(232, 23, mem.sname, 4);

		for (int j = 0, i, x; j < VIEWER_LINES; j++) {
			reader.SetLine(pos + j);
			for (i = 0; i < hshift; i++)
				reader.GetChar();
			for (i = 0, x = 0; i < SCREEN_WIDTH; i++, x += 6)
				DrawChar(x, 2 + j, reader.GetChar());
		}

		key = GetKey();
		if (key == K_ESC || key == K_F3 || key == K_F10)
			break;
		else if (key == K_F2 && !simpleMode) {
			float wasCount = reader.GetLines();

			wrap = !wrap;
			reader.ReadLines(wrap ? SCREEN_WIDTH : -1);

			int newCount = reader.GetLines();
			if (newCount >= reader.LINES_MAX)
				Shell_MessageBox("Error", "Too many lines!");

			pos = round((wasCount / (float) newCount) * pos);
			maxPos = max(0, newCount - VIEWER_LINES);

			if (pos < 0)
				pos = 0;
			if (pos > maxPos)
				pos = maxPos;

			DrawFnKeys(1, 23, wrap ? "2unwr" : "2wrap", 5);
		}
		else if (key == K_F4 && !simpleMode) {
			if (!Shell_HexViewer(fullName, false, true))
				goto restart;
			break;
		}
		else if (key == K_UP)
			pos = max(pos - 1, 0);
		else if (key == K_DOWN)
			pos = min(pos + 1, maxPos);
		else if (key == K_LEFT) {
			if (wrap)
				pos = max(pos - VIEWER_LINES, 0);
			else if (!simpleMode) {
				hshift = max(hshift - 1, 0);
				sniprintf(mem.sname, 4, "%d", hshift + 1);
				DrawStr(200, 23, mem.sname, 3);
			}
		}
		else if (key == K_RIGHT) {
			if (wrap)
				pos = min(pos + VIEWER_LINES, maxPos);
			else if (!simpleMode) {
				hshift = min(hshift + 1, 255);
				sniprintf(mem.sname, 4, "%d", hshift + 1);
				DrawStr(200, 23, mem.sname, 3);
			}
		}
		else if (key == K_PAGEUP)
			pos = max(pos - VIEWER_LINES, 0);
		else if (key == K_PAGEDOWN)
			pos = min(pos + VIEWER_LINES, maxPos);
		else if (key == K_HOME) {
			pos = 0;
			if (!simpleMode && hshift > 0) {
				hshift = 0;
				sniprintf(mem.sname, 4, "%d", hshift + 1);
				DrawStr(200, 23, mem.sname, 3);
			}
		}
		else if (key == K_END) {
			pos = maxPos;
			if (!simpleMode && hshift > 0) {
				hshift = 0;
				sniprintf(mem.sname, 4, "%d", hshift + 1);
				DrawStr(200, 23, mem.sname, 3);
			}
		}
	}

	if (!simpleMode)
		wrapLines = wrap;
}
//---------------------------------------------------------------------------------------
bool Shell_HexViewer(const char *fullName, bool editMode, bool fromTextView)
{
	if (f_open(&mem.fsrc, fullName, FA_READ | (editMode ? FA_WRITE : 0)) != FR_OK)
		return false;

	ClrScr(0117);
	DrawLine(1, 2);
	DrawLine(1, 4);

	DrawAttr8(0,  0, 0115, 32);
	DrawAttr8(0, 23, 0005, 8);
	DrawAttr8(8, 23, 0006, 24);

	display_path(fullName, 0, 0, 42);

	if (fromTextView) {
		DrawFnKeys(32, 23, "4text", 5);
		sniprintf(mem.sname, 12, "%lu", mem.fsrc.fsize);
		DrawStr(96, 23, mem.sname);
	}
	else if (editMode) {
		DrawHexNum(152, 23, 0, 8);
		DrawChar(201, 23, '/');
		DrawHexNum(208, 23, mem.fsrc.fsize, 8);
	}

	UINT res;
	bool rightPane = false, lowNibble = false;
	int pos = 0, ptr = 0, cursor = 0;
	int maxPos = 0;

	if (mem.fsrc.fsize > (signed) VIEWER_BYTES)
		maxPos = max(0, (1 + (mem.fsrc.fsize | 7)) - VIEWER_BYTES);

	byte key = 0, c;
	while (true) {
		if (editMode)
			DrawHexNum(152, 23, pos + cursor, 8);
		else {
			int p = 100;
			if (maxPos > 0)
				p = pos * 100 / maxPos;

			sniprintf(mem.sname, 5, "%3d%%", p);
			DrawStr(232, 23, mem.sname, 4);
		}

		for (int j = 0, y = 2; j < VIEWER_LINES; j++, y++) {
			ptr = pos + (j << 3);
			f_lseek(&mem.fsrc, ptr);
			f_read(&mem.fsrc, mem.sname, 8, &res);

			DrawHexNum(8, y, ptr, 8);
			if (editMode) {
				DrawAttr8(8, y, 0117, 17);
				DrawAttr8(25, y, (rightPane && j == (cursor >> 3)) ? 0126 : 0117, 6);
			}

			ptr -= pos;
			for (int i = 0, x1 = 66, x2 = 200; i < 8; i++, ptr++, x1 += 16, x2 += 6) {
				if (pos + ptr > (int) mem.fsrc.fsize) {
					DrawChar(x1, y, ' ');
					DrawChar(x1 + 6, y, ' ');
					DrawChar(x2, y, ' ');
					continue;
				}

				DrawHexNum(x1, y, mem.sname[i]);
				DrawSaveChar(x2, y, mem.sname[i], false, (editMode && rightPane && cursor == ptr));

				if (editMode && !rightPane && cursor == ptr) {
					DrawAttr8(x1 >> 3, y, 0126, 2);
					c = mem.sname[i];
				}
			}
		}

		key = GetKey();
		if (ModComb(MOD_ALT_0 | MOD_CTRL_0) && key >= ' ' && editMode) {
			if (!allowFileEdit) {
				allowFileEdit = Shell_MessageBox("Direct File Access",
					"You are about to modify contents",
					"of file directly on SD card.",
					"You have been warned! Sure?", MB_YESNO);

				if (!allowFileEdit)
					continue;
			}

			f_lseek(&mem.fsrc, pos + cursor);

			if (rightPane) {
				f_write(&mem.fsrc, &key, 1, &res);

				if (cursor < VIEWER_BYTES - 1)
					cursor++;
				else {
					pos = min(pos + 8, maxPos);
					if (pos < maxPos)
						cursor -= 7;
				}

				if ((pos + cursor) > mem.fsrc.fsize)
					cursor = mem.fsrc.fsize - pos;
			}
			else {
				if (key >= '0' && key <= '9')
					key -= '0';
				else if (key >= 'A' && key <= 'F')
					key -= ('A' - 10);
				else if (key >= 'a' && key <= 'f')
					key -= ('a' - 10);
				else
					continue;

				if (lowNibble)
					c |= key;
				else
					c = key << 4;

				f_write(&mem.fsrc, &c, 1, &res);
				lowNibble = !lowNibble;
			}
		}
		else if (ModComb(MOD_ALT_0 | MOD_SHIFT_0) && (key == K_HOME || key == K_END)) {
			if (key == K_HOME) {
				lowNibble = false;

				if (editMode && !ModComb(MOD_CTRL_1)) {
					cursor &= ~7;
					continue;
				}

				pos = 0;
			}
			else if (key == K_END) {
				lowNibble = false;

				if (editMode) {
					if (!ModComb(MOD_CTRL_1)) {
						cursor |= 7;
						continue;
					}
					if (pos == maxPos) {
						cursor = mem.fsrc.fsize - pos;
						continue;
					}
				}

				pos = maxPos;
			}
		}
		else if (ModComb(MOD_ALT_0 | MOD_CTRL_0 | MOD_SHIFT_0)) {
			if (key == K_ESC || key == K_F3 || key == K_F10)
				break;

			else if (key == K_F4 && fromTextView) {
				DrawStr(52, 23, "", 34);
				return false;
			}
			else if (key == K_TAB && editMode) {
				rightPane = !rightPane;
				lowNibble = false;
			}
			else if (key == K_UP) {
				lowNibble = false;

				if (editMode && cursor >= 8) {
					cursor -= 8;
					continue;
				}

				pos = max(pos - 8, 0);
			}
			else if (key == K_DOWN) {
				lowNibble = false;

				if (editMode &&
					(cursor < (signed) mem.fsrc.fsize || cursor < (VIEWER_BYTES - 8))) {

					cursor += 8;

					if ((pos + cursor) > mem.fsrc.fsize)
						cursor = mem.fsrc.fsize - pos;
					continue;
				}
				else if (mem.fsrc.fsize < (signed) VIEWER_BYTES)
					continue;

				pos = min(pos + 8, maxPos);
			}
			else if (key == K_LEFT) {
				if (editMode) {
					lowNibble = false;

					if (cursor > 0)
						cursor--;
					else {
						pos = max(pos - 8, 0);
						if (pos > 0)
							cursor += 7;
					}
				}
				else
					pos = max(pos - VIEWER_BYTES, 0);
			}
			else if (key == K_RIGHT) {
				if (editMode) {
					lowNibble = false;

					if (cursor < VIEWER_BYTES - 1)
						cursor++;
					else {
						pos = min(pos + 8, maxPos);
						if (pos < maxPos)
							cursor -= 7;
					}

					if ((pos + cursor) > mem.fsrc.fsize)
						cursor = mem.fsrc.fsize - pos;
				}
				else
					pos = min(pos + VIEWER_BYTES, maxPos);
			}
			else if (key == K_PAGEUP) {
				lowNibble = false;

				if (editMode && pos == 0) {
					cursor &= 7;
					continue;
				}

				pos = max(pos - VIEWER_BYTES, 0);
			}
			else if (key == K_PAGEDOWN) {
				lowNibble = false;

				if (editMode && pos == maxPos) {
					cursor = mem.fsrc.fsize - pos;
					continue;
				}

				pos = min(pos + VIEWER_BYTES, maxPos);
			}
		}
	}

	f_close(&mem.fsrc);
	return true;
}
//---------------------------------------------------------------------------------------
