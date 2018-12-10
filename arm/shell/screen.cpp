#include "screen.h"
#include "font0.h"
#include "logo.h"

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
		const int maxdest = 0x1b00;
		byte *src = (byte *) logoPacked;
		byte *maxsrc = src + sizeof(logoPacked);
		byte x, y;
		int i = 0;

		while (src < maxsrc && i < maxdest) {
			x = *src++;

			if (x & 0x80) {
				x &= 0x7F;
				x++;
				while (x-- > 0 && i < maxdest) {
					SystemBus_Write(VIDEO_PAGE_PTR + i, *src++);
					i++;
				}
			}
			else {
				x += 3;
				y = *src++;
				while (x-- > 0 && i < maxdest) {
					SystemBus_Write(VIDEO_PAGE_PTR + i, y);
					i++;
				}
			}
		}

		DrawStr(104, 6, "FIRMWARE");
		DrawStr(110, 7, "v" VERSION);

		SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable reset videopage
		SystemBus_Write(0xc00022, 0x8002); // Enable reset border
	}
	else {
		SystemBus_Write(0xc00021, 0); // Enable Video
		SystemBus_Write(0xc00022, 0);
	}
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

	if (y < 192) {
		dword address = (VIDEO_PAGE_PTR) + (((y & 0xc0) | ((y & 0x38) >> 3) | ((y & 0x07) << 3)) << 5);
		for (byte yy = 0; yy < 32; yy++)
			SystemBus_Write(address++, 0xff);
	}
}
//---------------------------------------------------------------------------------------
void DrawFrame(byte x, byte y, byte w, byte h, byte attr, const char corners[7])
{
	char current;

	for (int yy = 0; yy < h; yy++) {
		DrawAttr(x, y + yy, attr, w);

		for (int xx = 0, xw = w - 6; xx < w; xx += 6) {
			if (yy == 0 && xx == 0)
				current = corners[0];
			else if (yy == 0 && xx >= xw)
				current = corners[2];
			else if (yy == h - 1 && xx == 0)
				current = corners[4];
			else if (yy == h - 1 && xx >= xw)
				current = corners[6];
			else if (yy == 0)
				current = corners[1];
			else if (yy == h - 1)
				current = corners[5];
			else if (xx == 0 || xx >= xw)
				current = corners[3];
			else
				current = ' ';

			DrawChar(x + (xx < xw ? xx : xw), y + yy, current);
		}
	}
}
//---------------------------------------------------------------------------------------
void DrawChar(byte x, byte y, char c, bool over, bool inv)
{
	if (x >= 250 && y >= 24)
		return;

	byte col = (x >> 3);
	byte mod = (x & 0x07);
	byte rot = 7 - ((mod + 5) & 7);

	word data = (0b111111 << rot);
	word invr = inv ? data : 0;
	word mask = ~data;
	word copy;

	dword address = VIDEO_PAGE_PTR + (col + (y & 0x07) * 32 + (y & 0x18) * 256);
	const byte *tablePos = &shellFont0[(byte) c * 8];

	for (byte i = 0; i < 8; i++) {
		data = (*tablePos++) << rot;
		if (mod > 2) {
			copy = (SystemBus_Read(address) << 8) | SystemBus_Read(address + 1);
			if (over)
				copy ^= data ^ invr;
			else {
				copy &= mask;
				copy |= data;
				copy ^= invr;
			}

			SystemBus_Write(address, (copy >> 8));
			SystemBus_Write(address + 1, (copy & 0xff));
		}
		else {
			copy = SystemBus_Read(address);
			if (over)
				copy ^= data ^ invr;
			else {
				copy &= mask;
				copy |= data;
				copy ^= invr;
			}

			SystemBus_Write(address, copy);
		}

		address += 256;
	}
}
//---------------------------------------------------------------------------------------
void DrawAttr(byte x, byte y, byte attr, int w, bool incent)
{
	if (y < 24) {
		byte col = (x >> 3);
		byte mod = (x & 0x07);

		if (incent && mod > 4) {
			col++;
			w -= mod;
		}

		dword address = VIDEO_PAGE_PTR + 0x1800 + col + y * 32;

		while (w > (incent ? 8 : 0)) {
			SystemBus_Write(address++, attr);
			w -= 8;
		}
	}
}
//---------------------------------------------------------------------------------------
void DrawAttr8(byte x, byte y, byte attr, byte n)
{
	if (x < 32 && y < 24) {
		dword address = VIDEO_PAGE_PTR + 0x1800 + x + y * 32;

		while (n--)
			SystemBus_Write(address++, attr);
	}
}
//---------------------------------------------------------------------------------------
void DrawStr(byte x, byte y, const char *str, byte size, bool over, bool inv)
{
	if (size == 0)
		size = strlen(str);

	while (size > 0) {
		DrawChar(x, y, *str ? *str++ : ' ', over, inv);

		x += 6;
		size--;
	}
}
//---------------------------------------------------------------------------------------
void DrawStrAttr(byte x, byte y, const char *str, byte attr, byte size, bool over, bool inv)
{
	if (size == 0)
		size = strlen(str);

	DrawStr(x, y, str, size, over, inv);
	DrawAttr(x, y, attr, size * 6);
}
//---------------------------------------------------------------------------------------
void DrawHexNum(byte x, byte y, dword num, int len, char caps)
{
	dword rem;
	while (--len >= 0) {
		rem = num % 16;
		DrawChar(x + (len * 6), y, rem + ((rem > 9) ? (caps - 10) : '0'));
		num /= 16;
	}
}
//---------------------------------------------------------------------------------------
void DrawFnKeys(byte x, byte y, const char *fnKeys, int len)
{
	bool wasChar = false, isChar = false;
	for (int i = 0; i < len; i++, x += 6) {
		isChar = (fnKeys[i] > '9');
		if (!isChar && wasChar)
			++x;

		DrawChar(x, y, fnKeys[i], false, isChar);
		wasChar = isChar;
	}
}
//---------------------------------------------------------------------------------------
void CycleMark(byte x, byte y)
{
	const char marks[4] = { '/', '-', '\\', '|' };
	static int mark = 0;

	DrawChar(x, y, marks[mark]);
	mark = (mark + 1) & 3;
}
//---------------------------------------------------------------------------------------
