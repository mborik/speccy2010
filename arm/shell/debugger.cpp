#include "debugger.h"
#include "screen.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../specConfig.h"
#include "../specKeyboard.h"

#define COL_NORMAL  0007
#define COL_ACTIVE  0017
#define COL_CURSOR  0030
#define COL_EDITOR  0146
#define COL_TITLE   0151
#define COL_SIDEPAN 0140
#define COL_SIDE_HI 0141

typedef struct TRegLayout {
	byte i, width;
	byte x, y;
	byte lf, rt, up, dn;
} TRegLayout;

static const TRegLayout regLayout[16] = {
	// i, w,  x, y,  lf, rt, up, dn
	{  0, 4,  16, 1,   0,  4,  0,  1 }, //  0 af
	{  5, 4,  16, 2,   1,  5,  0,  2 }, //  1 bc
	{  6, 4,  16, 3,   2,  6,  1,  3 }, //  2 de
	{  7, 4,  16, 4,   3,  7,  2,  3 }, //  3 hl
	{  1, 4,  64, 1,   0,  8,  4,  5 }, //  4 af'
	{  9, 4,  64, 2,   1,  9,  4,  6 }, //  5 bc'
	{ 10, 4,  64, 3,   2, 10,  5,  7 }, //  6 de'
	{ 11, 4,  64, 4,   3, 11,  6,  7 }, //  7 hl'
	{  3, 4, 112, 1,   4, 12,  8,  9 }, //  8 sp
	{  4, 4, 112, 2,   5,  9,  8, 10 }, //  9 pc
	{  8, 4, 112, 3,   6, 13,  9, 11 }, // 10 ix
	{ 12, 4, 112, 4,   7, 15, 10, 11 }, // 11 iy
	{  2, 4, 168, 1,   8, 12, 12, 14 }, // 12 ir
	{ 13, 1, 162, 3,  10, 14, 12, 15 }, // 13 im
	{ 14, 2, 180, 3,  13, 14, 12, 15 }, // 14 iff
	{  0, 8, 144, 4,  11, 15, 13, 15 }, // 15 flags
};

enum { WIN_TRACE, WIN_WATCHERS, WIN_MEMDUMP };

word regBuffer[15];
byte activeWindow = WIN_TRACE;
bool activeRegPanel = false;
byte regPanelCursor = 0;
byte regPanelEditor = 0;
word regPanelEditBak;
//---------------------------------------------------------------------------------------
dword CalculateAddrAtCursor(word addr)
{
	byte specPort7ffd = SystemBus_Read(0xC00017);
	byte specPort1ffd = SystemBus_Read(0xC00023);
	byte specTrdos    = SystemBus_Read(0xC00018);
	byte divmmcState  = SystemBus_Read(0xC0001d);

	int page = 0;
	if (addr < 0x4000) {
		page = 0x100;

		if (specConfig.specMachine != SpecRom_Classic48 && (specPort7ffd & 0x10))
			page |= 1;

		if (specConfig.specDiskIf == SpecDiskIf_DivMMC) {
			if (addr < 0x2000) {
				if ((divmmcState & 0x1c0) == 0x140) {
					page  = 0x21; // 0000-1FFF > DivMMC bank #03
					addr |= 0x2000;
				}
				else if ((divmmcState & 0x180))
					page |= 3; // 0000-1FFF > DivMMC (E)EPROM (firmware in ROM3)
			}
			else if ((divmmcState & 0x180)) {
				page = 0x40 | (divmmcState & 0x3f); // 2000-3FFF > DivMMC bank #00-#3F
				addr = (addr & 0x1fff) | ((page & 1) << 13);
				page >>= 1;
			}
		}
		else if (specConfig.specMachine == SpecRom_Scorpion) {
			if ((specPort1ffd & 2))
				page = 0x102; // 0000-3FFF > Service ROM
			else if ((specPort7ffd & 1))
				page = 0; // 0000-3FFF > Disable ROM and connect RAM page 0
		}
		else if (specConfig.specDiskIf == SpecDiskIf_Betadisk && specTrdos)
			page = 0x103; // 0000-3FFF > TR-DOS ROM
	}
	else if (addr < 0x8000)
		page = 5;
	else if (addr < 0xc000)
		page = 2;
	else {
		page = (specPort7ffd & 7);

		if (specConfig.specMachine == SpecRom_Pentagon1024)
			page |= ((specPort7ffd & 0xc0) >> 3) | (specPort7ffd & 0x20);
		else if (specConfig.specMachine == SpecRom_Scorpion)
			page |= (specPort1ffd & 0x20) >> 1;
		else if (specConfig.specMachine == SpecRom_Classic48)
			page = 0;
	}

	return page * 0x4000 + (addr & 0x3fff);
}
//---------------------------------------------------------------------------------------
byte ReadByteAtCursor(word addr)
{
	return (byte) SystemBus_Read(CalculateAddrAtCursor(addr));
}
//---------------------------------------------------------------------------------------
void WriteByteAtCursor(word addr, byte data)
{
	SystemBus_Write(CalculateAddrAtCursor(addr), data);
}
//---------------------------------------------------------------------------------------
//=======================================================================================
//---------------------------------------------------------------------------------------
void Debugger_SwitchPanel()
{
	byte y;
	for (y = 1; y < 5; y++)
		DrawAttr8(0, y, activeRegPanel ? COL_ACTIVE : COL_NORMAL, 24);
	for (++y; y < 24; y++)
		DrawAttr8(0, y, activeRegPanel ? COL_NORMAL : COL_ACTIVE, 24);
}
//---------------------------------------------------------------------------------------
void Debugger_Screen0()
{
	ClrScr(0056);

	DrawStrAttr(  1,  0, "regs",  COL_TITLE, 4);
	DrawStrAttr(  1,  5, "trace", COL_TITLE, 5);
	DrawStrAttr(209,  0, "pages", COL_TITLE, 5);
	DrawStrAttr(209,  6, "stack", COL_TITLE, 5);
	DrawStrAttr(209, 17, "ports", COL_TITLE, 5);

	Debugger_SwitchPanel();

	byte y;
	for (y = 1; y < 5; y++) {
		DrawAttr8(26, y, COL_SIDE_HI, 2);
		DrawAttr8(28, y, COL_SIDEPAN, 4);
	}
	for (y = 7; y < 16; y++) {
		DrawAttr8(26, y, COL_SIDE_HI, 2);
		DrawAttr8(28, y, COL_SIDEPAN, 4);
	}
	for (y = 18; y < 24; y++)
		DrawAttr8(26, y, COL_SIDEPAN, 6);

	const char *exxRegs[9] = { "", "af", "bc", "de", "hl", "sp", "pc", "ix", "iy" };
	for (y = 1; y <= 4; y++) {
		DrawStr (  0, y, exxRegs[y]);
		DrawChar( 11, y, ':', true);
		DrawStr ( 48, y, exxRegs[y]);
		DrawChar( 59, y, '`', true);
		DrawStr ( 96, y, exxRegs[y + 4]);
		DrawChar(107, y, ':', true);
	}

	DrawStr(144, 1, "ir:");
	DrawStr(144, 3, "im");

	for (y = 0; y < 4; y++) {
		DrawChar(212, y + 1, y + '0');
		DrawChar(218, y + 1, ':');

		if (y == 0)
			continue;
		else if (y == 1 && specConfig.specMachine != SpecRom_Classic48)
			DrawStr(224, y + 1, "SCR");
		else
			DrawStr(224, y + 1, "RAM");
	}

	DrawStr(212, 7, "-2:");
	DrawStr(212, 8, "SP:");

	byte offset = 2;
	for (y = 9; y < 16; y++, offset += 2) {
		DrawChar(212, y, '+');
		DrawHexNum(218, y, offset, 1, 'A');
		DrawChar(224, y, ':');
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateRegs()
{
	const TRegLayout *r = NULL;
	word reg;
	byte i, c;

	for (i = 0; i < 14; i++) {
		reg = SystemBus_Read(0xc001f0 + i);

		if (i == 2) {
			// IR
			regBuffer[i] = (reg >> 8) | (reg << 8);
		}
		else if (i == 13) {
			// IM
			regBuffer[i++] = (reg & 0x03);
			// IFF in BCD
			regBuffer[i] = ((reg & 0x08) << 1) | (reg & 0x04) >> 2;
		}
		else
			regBuffer[i] = reg;
	}

	for (i = 0; i < 16; i++) {
		c = 0;
		r = &regLayout[i];
		reg = regBuffer[r->i];

		if (activeRegPanel) {
			c = COL_ACTIVE;
			if (regPanelCursor == i)
				c = (regPanelEditor > 0) ? COL_EDITOR : COL_CURSOR;

			DrawAttr(r->x, r->y, c, r->width * 6);
		}

		if (i == 15)
			break;

		DrawHexNum(r->x, r->y, reg, r->width, 'A');

		if (c == COL_EDITOR)
			DrawChar(r->x + ((regPanelEditor - 1) * 6), r->y, ' ', true, true);
	}

	for (i = 0; i < 8; i++) {
		DrawChar(
			r->x + (i * 6), r->y,
			"SZ5H3PNCsz.h.pnc"[((reg & (0x80 >> i)) ? 0 : 8) + i],
			false, (c == COL_EDITOR)
		);
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateTrace()
{
	byte dbuf[8];
	char *asmbuf;
	int i, x, len = 8, row;

	word addr = regBuffer[4]; // pc

	for (row = 6; row < 24; row++) {
		for (i = 0; i < 8; i++)
			dbuf[i] = ReadByteAtCursor(0xFFFF & (i + addr));

		asmbuf = Shell_DebugDisasm(dbuf, addr, &len);

		DrawHexNum(0, row, addr, 4, 'A');
		for (i = 0, x = 32; i < 4; i++, x += 12) {
			if (i < len)
				DrawHexNum(x, row, dbuf[i], 2, 'A');
			else
				DrawStr(x, row, "", 2);
		}

		DrawChar(x, row, (len > 4) ? '\x1C' : ' ');
		DrawStr(96, row, asmbuf, 16);

		addr += len;
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateSidePanel()
{
	word rom = CalculateAddrAtCursor(0x0000) >> 14;
	byte ram = CalculateAddrAtCursor(0xC000) >> 14;
	byte specPort7ffd = SystemBus_Read(0xC00017);

	const char *romTxt = "ROM??";
	switch (rom) {
		case 0:
			romTxt = "RAM 0"; // Scorpion AllRAM
			break;
		case 0x21:
			if (specConfig.specDiskIf == SpecDiskIf_DivMMC)
				romTxt = "MAPRM"; // DivMMC MAPRAM page 3
			break;
		case 0x100:
			romTxt = "ZX128";
			break;
		case 0x101:
			romTxt = "ZXROM";
			break;
		case 0x102:
			romTxt = "RESET";
			break;
		case 0x103:
			romTxt = (specConfig.specDiskIf == SpecDiskIf_DivMMC) ? "EPROM" : "TRDOS";
			break;
	}

	DrawStr (224, 1, romTxt, 5);
	DrawChar(248, 2, (specPort7ffd & 0x08) ? '7' : '5');
	DrawChar(248, 3, '2');
	DrawChar(248, 4, '0' + (ram & 7));

	word addr, stack = 0xFFFF & (((int) regBuffer[3]) - 2); // sp
	for (byte y = 7; y < 16; y++, stack += 2) {
		addr = ReadByteAtCursor(stack) | ReadByteAtCursor(stack + 1) << 8;
		DrawHexNum(230, y, addr, 4, 'A');
	}
}
//---------------------------------------------------------------------------------------
bool Debugger_TestKeyRegs(byte key)
{
	const TRegLayout *r = &regLayout[regPanelCursor];

	if (!regPanelEditor) {
		if (key == K_LEFT)
			regPanelCursor = r->lf;
		else if (key == K_RIGHT)
			regPanelCursor = r->rt;
		else if (key == K_UP)
			regPanelCursor = r->up;
		else if (key == K_DOWN)
			regPanelCursor = r->dn;
		else if (key == K_RETURN) {
			regPanelEditor = 1;
			regPanelEditBak = regBuffer[r->i];
		}
		else
			return false;
	}
	else {
		int shift = (r->width - regPanelEditor) * 4;
		word mask = 0xFFFF ^ (0x0F << shift);

		if (key == K_LEFT && regPanelEditor > 1)
			regPanelEditor--;
		else if (key == K_RIGHT && regPanelEditor < r->width)
			regPanelEditor++;
		else if (regPanelCursor < 13 && key >= '0' && key <= '9') {
			key -= (key <= '9') ? '0' : (key <= 'F') ? 'A' : 'a';
			regBuffer[r->i] = (regBuffer[r->i] & mask) | (key << shift);
		}
		else if (key == K_ESC) {
			regPanelEditor = 0;
			regBuffer[r->i] = regPanelEditBak;
		}
		else
			return false;
	}

	return true;
}
//---------------------------------------------------------------------------------------
void Shell_Debugger()
{
	CPU_Stop();
	SystemBus_Write(0xc00020, 0); // use bank 0

	Debugger_Screen0();

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | 0); // Enable shell border

	regPanelEditor = 0;

	byte key;
	bool updateAll = true;
	bool updateRegs = false;
	bool updateTrace = false;

	while (true) {
		if (updateRegs || updateAll) {
			Debugger_UpdateRegs();
			updateRegs = false;
		}
		if (updateTrace || updateAll) {
			Debugger_UpdateTrace();
			updateTrace = false;
		}
		if (updateAll) {
			Debugger_UpdateSidePanel();
			updateAll = false;
		}

		key = GetKey(true);

		if (key == K_TAB) {
			activeRegPanel = !activeRegPanel;
			Debugger_SwitchPanel();

			updateRegs = true;
			updateTrace = true;
		}
		else if (activeRegPanel && Debugger_TestKeyRegs(key)) {
			updateRegs = true;
			continue;
		}
		else if (key == K_F7) { // one step
			SystemBus_Write(0xc00008, 0x01);
			DelayMs(1);
			DiskIF_Routine();

			updateAll = true;
		}
		else if (key == K_ESC)
			break;
	}

	DelayMs(100);

	CPU_Start();

	SystemBus_Write(0xc00021, 0); //armVideoPage
	SystemBus_Write(0xc00022, 0); //armBorder
}
//---------------------------------------------------------------------------------------
