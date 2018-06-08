#include "viewer.h"
#include "screen.h"
#include "dialog.h"
#include "textReader.h"
#include "../utils/dirent.h"
#include "../specKeyboard.h"

const int VIEWER_LINES = 21;
//---------------------------------------------------------------------------------------
void Shell_Viewer(const char *fullName)
{
	CTextReader reader(fullName, 32);
	if (reader.GetLines() < 0) {
		Shell_MessageBox("Error", "Cannot open file!");
		return;
	}

	ClrScr(0117);
	DrawLine(1, 3);
	DrawLine(1, 5);

	WriteAttr(0,  0, 0115, 32);
	WriteAttr(0, 23, 0006, 32);

	display_path(fullName, 0, 0, 32);

	if (reader.GetLines() == reader.LINES_MAX) {
		Shell_MessageBox("Error", "Too many lines!");
	}

	int pos = 0;
	int maxPos = max(0, reader.GetLines() - VIEWER_LINES);

	byte key = 0;
	while (true) {
		int p = 100;
		if (maxPos > 0)
			p = pos * 100 / maxPos;

		char str[5];
		sniprintf(str, 5, "%3d%%", p);
		WriteStr(28, 23, str, 4);

		for (int j = 0; j < VIEWER_LINES; j++) {
			reader.SetLine(pos + j);
			for (int i = 0; i < 32; i++)
				WriteChar(i, 2 + j, reader.GetChar());
		}

		key = GetKey();
		if (key == K_ESC || key == K_F3 || key == K_F10)
			break;
		else if (key == K_DOWN)
			pos = min(pos + 1, maxPos);
		else if (key == K_UP)
			pos = max(pos - 1, 0);
		else if (key == K_PAGEDOWN || key == K_RIGHT)
			pos = min(pos + VIEWER_LINES, maxPos);
		else if (key == K_PAGEUP || key == K_LEFT)
			pos = max(pos - VIEWER_LINES, 0);
		else if (key == K_HOME)
			pos = 0;
		else if (key == K_END)
			pos = maxPos;
	}
}
//---------------------------------------------------------------------------------------
