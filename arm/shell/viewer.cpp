#include "viewer.h"
#include "screen.h"
#include "dialog.h"
#include "textReader.h"
#include "../specKeyboard.h"

const int FILES_PER_ROW = 18;
//---------------------------------------------------------------------------------------
void Shell_Viewer(const char *fullName)
{
	CTextReader reader(fullName, 32);
	if (reader.GetLines() < 0) {
		Shell_MessageBox("Error", "Cannot open file !");
		return;
	}

	if (reader.GetLines() == reader.LINES_MAX) {
		Shell_MessageBox("Error", "Lines number > LINES_MAX !");
	}

	for (int j = 2; j < FILES_PER_ROW; j++)
		WriteAttr(0, 2 + j, 0007, 32);

	int pos = 0;
	int maxPos = max(0, reader.GetLines() - FILES_PER_ROW);

	byte key = 0;
	while (key != K_ESC) {
		int p = 100;
		if (maxPos > 0)
			p = pos * 10000 / maxPos;

		char str[10];
		sniprintf(str, sizeof(str), "%d.%.2d %%", p / 100, p % 100);
		WriteStr(0, FILES_PER_ROW + 5, str, 32);

		for (int j = 0; j < FILES_PER_ROW; j++) {
			reader.SetLine(pos + j);
			for (int i = 0; i < 32; i++)
				WriteChar(i, 2 + j, reader.GetChar());
		}

		key = GetKey();

		if (key == K_DOWN)
			pos = min(pos + 1, maxPos);
		else if (key == K_UP)
			pos = max(pos - 1, 0);
		else if (key == K_PAGEDOWN || key == K_RIGHT)
			pos = min(pos + FILES_PER_ROW, maxPos);
		else if (key == K_PAGEUP || key == K_LEFT)
			pos = max(pos - FILES_PER_ROW, 0);
		else if (key == K_HOME)
			pos = 0;
		else if (key == K_END)
			pos = maxPos;
	}
}
//---------------------------------------------------------------------------------------
