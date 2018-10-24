#include <math.h>

#include "viewer.h"
#include "screen.h"
#include "dialog.h"
#include "textReader.h"
#include "../utils/dirent.h"
#include "../specKeyboard.h"

bool wrapLines = false;
const int SCREEN_WIDTH = 42;
const int VIEWER_LINES = 21;
//---------------------------------------------------------------------------------------
void Shell_TextViewer(const char *fullName, bool simpleMode)
{
	CTextReader reader(fullName, (simpleMode || wrapLines) ? SCREEN_WIDTH : -1);
	if (reader.GetLines() < 0) {
		Shell_MessageBox("Error", "Cannot open file!");
		return;
	}

	ClrScr(0117);
	DrawLine(1, 3);
	DrawLine(1, 5);

	DrawAttr8(0,  0, simpleMode ? 0114 : 0115, 32);
	DrawAttr8(0, 23, 0005, 7);
	DrawAttr8(7, 23, 0006, 26);

	char wrkstr[8];

	if (simpleMode) {
		DrawStr(5, 0, "Speccy2010 Commander v" VERSION " \7 Info Viewer");
		wrapLines = true;
	}
	else {
		display_path(fullName, 0, 0, 42);

		DrawFnKeys(1, 23, "2wrap4hex", 9);
		if (wrapLines)
			DrawFnKeys(1, 23, "2unwr", 5);

		sniprintf(wrkstr, 8, "%7d", reader.GetFileSize());
		DrawStr(96, 23, wrkstr);
		DrawStr(176, 23, "col 1");

		if (reader.GetLines() >= reader.LINES_MAX)
			Shell_MessageBox("Error", "Too many lines!");
	}

	int pos = 0, hshift = 0;
	int maxPos = max(0, reader.GetLines() - VIEWER_LINES);

	byte key = 0;
	while (true) {
		int p = 100;
		if (maxPos > 0)
			p = pos * 100 / maxPos;

		sniprintf(wrkstr, 5, "%3d%%", p);
		DrawStr(232, 23, wrkstr, 4);

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

			wrapLines = !wrapLines;
			reader.ReadLines(wrapLines ? SCREEN_WIDTH : -1);

			int newCount = reader.GetLines();
			if (newCount >= reader.LINES_MAX)
				Shell_MessageBox("Error", "Too many lines!");

			pos = round((wasCount / (float) newCount) * pos);
			maxPos = max(0, newCount - VIEWER_LINES);

			if (pos < 0)
				pos = 0;
			if (pos > maxPos)
				pos = maxPos;

			DrawFnKeys(1, 23, wrapLines ? "2unwr" : "2wrap", 5);
		}
		else if (key == K_UP)
			pos = max(pos - 1, 0);
		else if (key == K_DOWN)
			pos = min(pos + 1, maxPos);
		else if (key == K_LEFT) {
			if (wrapLines)
				pos = max(pos - VIEWER_LINES, 0);
			else if (!simpleMode) {
				hshift = max(hshift - 1, 0);
				sniprintf(wrkstr, 4, "%d", hshift + 1);
				DrawStr(200, 23, wrkstr, 3);
			}
		}
		else if (key == K_RIGHT) {
			if (wrapLines)
				pos = min(pos + VIEWER_LINES, maxPos);
			else if (!simpleMode) {
				hshift = min(hshift + 1, 255);
				sniprintf(wrkstr, 4, "%d", hshift + 1);
				DrawStr(200, 23, wrkstr, 3);
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
				sniprintf(wrkstr, 4, "%d", hshift + 1);
				DrawStr(200, 23, wrkstr, 3);
			}
		}
		else if (key == K_END) {
			pos = maxPos;
			if (!simpleMode && hshift > 0) {
				hshift = 0;
				sniprintf(wrkstr, 4, "%d", hshift + 1);
				DrawStr(200, 23, wrkstr, 3);
			}
		}
	}
}
//---------------------------------------------------------------------------------------
