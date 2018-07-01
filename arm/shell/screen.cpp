#include "screen.h"
#include "font0.h"
#include "font1.h"
#include "font2.h"
#include "font3.h"

#include "../system.h"
#include "../system/sdram.h"
#include "../specConfig.h"

//---------------------------------------------------------------------------------------
void ScreenPush()
{
	word data;
	for (int i = 0; i < 0x1b00; i++) {
		data = SystemBus_Read(VIDEO_PAGE_PTR + i);
		SystemBus_Write(VIDEO_PAGE_PTR + 0x2000 + i, data);
	}
}
//---------------------------------------------------------------------------------------
void ScreenPop()
{
	word data;
	for (int i = 0; i < 0x1b00; i++) {
		data = SystemBus_Read(VIDEO_PAGE_PTR + 0x2000 + i);
		SystemBus_Write(VIDEO_PAGE_PTR + i, data);
	}
}
//---------------------------------------------------------------------------------------
void ResetScreen(bool active)
{
	if (active) {
		ClrScr(0022);
		SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
		SystemBus_Write(0xc00022, 0x8002); // Enable shell border
	}
	else {
		SystemBus_Write(0xc00021, 0); // Enable Video
		SystemBus_Write(0xc00022, 0);
	}
}
//---------------------------------------------------------------------------------------
void WriteDisplay(word address, byte data)
{
	SystemBus_Write(VIDEO_PAGE_PTR + address, data);
}
//---------------------------------------------------------------------------------------
void ClrScr(byte attr)
{
	int i = 0;
	for (; i < 0x1800; i++)
		SystemBus_Write(VIDEO_PAGE_PTR + i, 0);
	for (; i < 0x1b00; i++)
		SystemBus_Write(VIDEO_PAGE_PTR + i, attr);
}
//---------------------------------------------------------------------------------------
void DrawLine(byte y, byte cy)
{
	y = y * 8 + cy;

	if (y < 24 * 8) {
		word address = ((y & 0xc0) | ((y & 0x38) >> 3) | ((y & 0x07) << 3)) << 5;
		for (byte yy = 0; yy < 32; yy++)
			WriteDisplay(address++, 0xff);
	}
}
//---------------------------------------------------------------------------------------
void DrawFrame(byte x, byte y, byte w, byte h, byte attr, const char corners[7])
{
	char current;

	for (int yy = 0; yy < h; yy++) {
		WriteAttr(x, y + yy, attr, w);
		WriteStr(x + 1, y + yy, "", w - 2);

		for (int xx = 0; xx < w; xx++) {
			if (yy == 0 && xx == 0)
				current = corners[0];
			else if (yy == 0 && xx == w - 1)
				current = corners[2];
			else if (yy == h - 1 && xx == 0)
				current = corners[4];
			else if (yy == h - 1 && xx == w - 1)
				current = corners[6];
			else if (yy == 0)
				current = corners[1];
			else if (yy == h - 1)
				current = corners[5];
			else if (xx == 0 || xx == w - 1)
				current = corners[3];
			else
				continue;

			WriteChar(x + xx, y + yy, current);
		}
	}
}
//---------------------------------------------------------------------------------------
void WriteChar(byte x, byte y, char c)
{
	if (x < 32 && y < 24) {
		const byte *charTable = shellFont0;
		if (specConfig.specFont == 1)
			charTable = shellFont1;
		else if (specConfig.specFont == 2)
			charTable = shellFont2;

		if ((byte) c < 0x20) {
			// DOS CP437 char range (00 - 1F) special chars
			charTable = shellFont3_low;
		}
		else {
			c -= 0x20;

			// DOS CP437 char range (AE - DF) with special box-drawings
			if ((byte) c >= 0x8E && (byte) c <= 0xBF) {
				charTable = shellFont3_high;
				c -= 0x8E;
			}
			else if ((byte) c >= 0x80)
				c = 0;
		}

		word address = x + (y & 0x07) * 32 + (y & 0x18) * 32 * 8;
		const byte *tablePos = &charTable[(byte) c * 8];

		for (byte i = 0; i < 8; i++) {
			WriteDisplay(address, *tablePos++);
			address += 32 * 8;
		}
	}
}
//---------------------------------------------------------------------------------------
void WriteAttr(byte x, byte y, byte attr, byte n)
{
	if (x < 32 && y < 24) {
		word address = 0x1800 + x + y * 32;

		while (n--)
			WriteDisplay(address++, attr);
	}
}
//---------------------------------------------------------------------------------------
void WriteStr(byte x, byte y, const char *str, byte size)
{
	if (size == 0)
		size = strlen(str);

	while (size > 0) {
		if (*str)
			WriteChar(x++, y, *str++);
		else
			WriteChar(x++, y, ' ');

		size--;
	}
}
//---------------------------------------------------------------------------------------
void WriteStrAttr(byte x, byte y, const char *str, byte attr, byte size)
{
	if (size == 0)
		size = strlen(str);

	WriteStr(x, y, str, size);
	WriteAttr(x, y, attr, size);
}
//---------------------------------------------------------------------------------------
void CycleMark(byte x, byte y)
{
	const char marks[4] = { '/', '-', '\\', '|' };
	static int mark = 0;

	WriteChar(x, y, marks[mark]);
	mark = (mark + 1) & 3;
}
//---------------------------------------------------------------------------------------
