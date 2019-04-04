#include "debugger.h"
#include "screen.h"
#include "viewer.h"
#include "../system.h"
#include "../system/sdram.h"
#include "../specConfig.h"
#include "../specKeyboard.h"

typedef struct TRegLayout {
	byte i, width;
	byte x, y;
	byte lf, rt, up, dn;
} TRegLayout;

static const TRegLayout regLayout[16] = {
	// index  , w,  x,  y,  lf, rt, up, dn
	{ REG_AF  , 4,  16, 1,   0,  4,  0,  1 }, //  0 af
	{ REG_BC  , 4,  16, 2,   1,  5,  0,  2 }, //  1 bc
	{ REG_DE  , 4,  16, 3,   2,  6,  1,  3 }, //  2 de
	{ REG_HL  , 4,  16, 4,   3,  7,  2,  3 }, //  3 hl
	{ REG_AF_ , 4,  64, 1,   0,  8,  4,  5 }, //  4 af'
	{ REG_BC_ , 4,  64, 2,   1,  9,  4,  6 }, //  5 bc'
	{ REG_DE_ , 4,  64, 3,   2, 10,  5,  7 }, //  6 de'
	{ REG_HL_ , 4,  64, 4,   3, 11,  6,  7 }, //  7 hl'
	{ REG_SP  , 4, 112, 1,   4, 12,  8,  9 }, //  8 sp
	{ REG_PC  , 4, 112, 2,   5,  9,  8, 10 }, //  9 pc
	{ REG_IX  , 4, 112, 3,   6, 13,  9, 11 }, // 10 ix
	{ REG_IY  , 4, 112, 4,   7, 15, 10, 11 }, // 11 iy
	{ REG_IR  , 4, 168, 1,   8, 12, 12, 14 }, // 12 ir
	{ REG_IM  , 1, 162, 3,  10, 14, 12, 15 }, // 13 im
	{ REG_IFF , 2, 180, 3,  13, 14, 12, 15 }, // 14 iff
	{ REG_AF  , 8, 144, 4,  11, 15, 13, 15 }, // 15 flags
};

byte regPanelCursor = 0;
byte regPanelEditor = 0;
word regPanelEditBak;

word regBuffer[15];

byte activeWindow = WIN_TRACE;
bool activeRegPanel = false;

byte watcherMode = 0;
byte lastWatcherMode = -1;

extern bool dumpWindowAsciiMode;
extern bool dumpWindowRightPane;
//---------------------------------------------------------------------------------------
dword CalculateAddrAtCursor(word addr)
{
	byte specPort7ffd = SystemBus_Read(0xc00017);
	byte specPort1ffd = SystemBus_Read(0xc00023);
	byte specTrdos    = SystemBus_Read(0xc00018);
	byte diskIfState  = SystemBus_Read(0xc00042);

	int page = 0;
	if (addr < 0x4000) {
		page = 0x100;

		if (specConfig.specMachine != SpecRom_Classic48 && (specPort7ffd & 0x10))
			page |= 1;

		if (specConfig.specDiskIf == SpecDiskIf_DivMMC) {
			if (addr < 0x2000) {
				if ((diskIfState & 0x1c0) == 0x140) {
					page  = 0x21; // 0000-1FFF > DivMMC bank #03
					addr |= 0x2000;
				}
				else if ((diskIfState & 0x180))
					page |= 3; // 0000-1FFF > DivMMC (E)EPROM (firmware in ROM3)
			}
			else if ((diskIfState & 0x180)) {
				page = 0x40 | (diskIfState & 0x3f); // 2000-3FFF > DivMMC bank #00-#3F
				addr = (addr & 0x1fff) | ((page & 1) << 13);
				page >>= 1;
			}
		}
		else if (specConfig.specDiskIf == SpecDiskIf_MB02) {
			if ((diskIfState & 0x40)) // 0000-3FFF > MB-02 SRAM bank #00-#1F
				page = 0x20 | (diskIfState & 0x1f);
			else if ((diskIfState & 0x80)) // 0000-3FFF > MB-02 EPROM (in ROM3 position)
				page |= 3;
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
word GetCPUState(byte reg) { return regBuffer[reg]; }
//---------------------------------------------------------------------------------------
void SetCPUState(byte reg, word value) { regBuffer[reg] = value; }
//---------------------------------------------------------------------------------------
//=======================================================================================
//---------------------------------------------------------------------------------------
void Debugger_SwitchPanel()
{
	byte y;
	for (y = 1; y < 5; y++)
		DrawAttr8(0, y, activeRegPanel ? COL_ACTIVE : COL_NORMAL, 24);

	if (activeWindow == WIN_TRACE) {
		for (++y; y < 24; y++)
			DrawAttr8( 0, y, activeRegPanel ? COL_NORMAL : COL_ACTIVE, 24);
	}
	else if (activeWindow == WIN_MEMDUMP)
		Debugger_UpdateDumpAttrs();
}
//---------------------------------------------------------------------------------------
void Debugger_Screen()
{
	ClrScr(COL_BACKGND);

	DrawStrAttr(  1,  0, "regs",  COL_TITLE, 4);
	DrawStrAttr(209,  0, "pages", COL_TITLE, 5);

	if (activeWindow == WIN_TRACE) {
		DrawStrAttr(  1,  5, "trace", COL_TITLE, 5);
		DrawStrAttr(209,  6, "stack", COL_TITLE, 5);
	}
	else
		DrawStrAttr(  1,  5, "dump", COL_TITLE, 4);

	lastWatcherMode = -1;
	Debugger_SwitchPanel();

	byte y;
	for (y = 1; y < 5; y++) {
		DrawAttr8(26, y, COL_SIDE_HI, 2);
		DrawAttr8(28, y, COL_SIDEPAN, 4);
	}

	if (activeWindow == WIN_TRACE) {
		for (y = 7; y < 14; y++) {
			DrawAttr8(26, y, COL_SIDE_HI, 2);
			DrawAttr8(28, y, COL_SIDEPAN, 4);
		}
		for (y = 16; y < 24; y++)
			DrawAttr8(26, y, COL_SIDEPAN, 6);
	}

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

	if (activeWindow == WIN_TRACE) {
		DrawStr(212, 7, "-2:");
		DrawStr(212, 8, "sp:");

		byte offset = 2;
		for (y = 9; y < 14; y++, offset += 2) {
			DrawChar(212, y, '+');
			DrawHexNum(218, y, offset, 1, 'A');
			DrawChar(224, y, ':');
		}
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateRegs(bool updateCpuState = false)
{
	const TRegLayout *r = NULL;
	word reg;
	byte i, c;

	if (updateCpuState) {
		for (i = 0; i < 14; i++) {
			reg = SystemBus_Read(0xc001f0 + i);

			if (i == REG_IM) {
				// IM
				regBuffer[REG_IM] = (reg & 0x03);
				// IFF in BCD
				regBuffer[REG_IFF] = ((reg & 0x08) << 1) | (reg & 0x04) >> 2;
			}
			else
				regBuffer[i] = reg;
		}
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

		if (i == REG_FLG)
			break;

		DrawHexNum(r->x, r->y, reg, r->width, 'A');

		if (i != REG_IM && c == COL_EDITOR)
			DrawChar(r->x + ((regPanelEditor - 1) * 6), r->y, ' ', true, true);
	}

	for (i = 0; i < 8; i++) {
		DrawChar(
			r->x + (i * 6), r->y,
			"SZ5H3PNCsz.h.pnc"[((reg & (0x80 >> i)) ? 0 : 8) + i],
			false, (c == COL_EDITOR) ? (i == (regPanelEditor - 1)) : false
		);
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateWatchers()
{
	byte y = 15;
	bool advDiskIf = (
		specConfig.specDiskIf == SpecDiskIf_DivMMC ||
		specConfig.specDiskIf == SpecDiskIf_MB02);

	if (lastWatcherMode != watcherMode) {
		for (byte i = y; i < 24; i++)
			DrawStr(208, i, "", 8);

		if (watcherMode) {
			DrawStrAttr(209, y, "watch", COL_TITLE, 5);

			bool apos = false;
			const char *sreg = "";

			switch (watcherMode) {
				case REG_BC_:
					apos = true;
				case REG_BC:
					sreg = "bc"; break;
				case REG_DE_:
					apos = true;
				case REG_DE:
					sreg = "de"; break;
				case REG_HL_:
					apos = true;
				case REG_HL:
					sreg = "hl"; break;
				case REG_IX:
					sreg = "ix"; break;
				case REG_IY:
					sreg = "iy"; break;
			}

			DrawStr(242, y, sreg, 2);
			if (apos)
				DrawChar(252, y, '`', true);
		}
		else {
			DrawStrAttr(209, y, "ports", COL_TITLE, 5);

			DrawStr(212, y + 1, "xxFE:", 5);
			DrawStr(212, y + 2, "7FFD:", 5);
			DrawStr(212, y + 3, "1FFD:", 5);

			if (advDiskIf)
				DrawStr(212, y + 4,
					(specConfig.specDiskIf == SpecDiskIf_MB02) ? "xx17:" : "xxE3:", 5);
		}

		DrawAttr8(30, y, watcherMode ? COL_WTCHREG : COL_BACKGND, 2);

		lastWatcherMode = watcherMode;
	}

	if (watcherMode) {
		word pc = regBuffer[watcherMode];

		for (++y; y < 24; y++) {
			byte buf0 = ReadByteAtCursor(pc++);
			byte buf1 = ReadByteAtCursor(pc++);

			DrawHexNum(212, y, buf0, 2, 'A');
			DrawHexNum(227, y, buf1, 2, 'A');
			DrawSaveChar(243, y, buf0);
			DrawSaveChar(249, y, buf1);
		}
	}
	else {
		byte specPortFe   = SystemBus_Read(0xc00016);
		byte specPort7ffd = SystemBus_Read(0xc00017);
		byte specPort1ffd = SystemBus_Read(0xc00023);
		byte diskIfState  = SystemBus_Read(0xc00042) & 0xFF;

		DrawHexNum(242, y + 1, specPortFe, 2, 'A');
		DrawHexNum(242, y + 2, specPort7ffd, 2, 'A');
		DrawHexNum(242, y + 3, specPort1ffd, 2, 'A');

		if (advDiskIf)
			DrawHexNum(242, y + 4, diskIfState, 2, 'A');
	}
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateSidePanel()
{
	word rom = CalculateAddrAtCursor(0x0000) >> 14;
	byte ram = CalculateAddrAtCursor(0xC000) >> 14;
	byte specPort7ffd = SystemBus_Read(0xC00017);
	word diskIfState  = SystemBus_Read(0xc00042);

	const char *romTxt = "ROM??";
	switch (rom) {
		case 0:
			romTxt = "RAM 0"; // Scorpion AllRAM
			break;
		case 0x20:
			if (specConfig.specDiskIf == SpecDiskIf_MB02)
				romTxt = "BSROM"; // MB-02 SRAM bank #00 (BS-ROM)
			break;
		case 0x21:
			if (specConfig.specDiskIf == SpecDiskIf_MB02)
				romTxt = "BSDOS"; // MB-02 SRAM bank #01 (BS-DOS)
			break;
		case 0x22 ... 0x3F:
			if (specConfig.specDiskIf == SpecDiskIf_MB02)
				romTxt = "BNK";   // MB-02 SRAM bank #02-#1F
			break;
		case 0x100:
			romTxt = "ZX128";
			break;
		case 0x101:
			romTxt = "ZXROM";
			break;
		case 0x102:
			romTxt = "GLKRS";
			break;
		case 0x103:
			romTxt = (specConfig.specDiskIf == SpecDiskIf_MB02) ? "EPROM" : "TRDOS";
			break;
	}

	DrawStr (224, 1, romTxt);
	DrawChar(248, 2, (specPort7ffd & 0x08) ? '7' : '5');
	DrawChar(248, 3, '2');
	DrawChar(248, 4, '0' + (ram & 7));

	if (specConfig.specDiskIf == SpecDiskIf_DivMMC && (diskIfState & 0x180)) {
		// 0000-1FFF > DivMMC (E)EPROM or MAPRAM R/O bank #03
		DrawStr(224, 1, ((diskIfState & 0x1c0) == 0x140) ? "03\7" : "EP\7");
		// 2000-3FFF > DivMMC bank #00-#3F (controlled by port #E3)
		DrawHexNum(242, 1, (diskIfState & 0x3f), 2, 'A');
	}
	else if (specConfig.specDiskIf == SpecDiskIf_MB02 && rom >= 0x22)
		DrawHexNum(242, 1, (diskIfState & 0x1f), 2, 'A');

	if (activeWindow == WIN_TRACE) {
		word addr, stack = 0xFFFF & (((int) regBuffer[REG_SP]) - 2); // sp
		for (byte y = 7; y < 14; y++, stack += 2) {
			addr = ReadByteAtCursor(stack) | ReadByteAtCursor(stack + 1) << 8;
			DrawHexNum(230, y, addr, 4, 'A');
		}

		Debugger_UpdateWatchers();
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
		word value = regBuffer[r->i];

		if (key == K_LEFT && regPanelEditor > 1)
			regPanelEditor--;
		else if (key == K_RIGHT && regPanelEditor < r->width)
			regPanelEditor++;
		else if (regPanelCursor < REG_IM && (
				(key >= '0' && key <= '9') ||
				(key >= 'a' && key <= 'f') ||
				(key >= 'A' && key <= 'F'))) {

			int shift = (r->width - regPanelEditor) * 4;
			word mask = 0xFFFF ^ (0x0F << shift);
			byte nibble = key;

			if (key >= '0' && key <= '9')
				nibble -= '0';
			else if (key >= 'a' && key <= 'f')
				nibble -= ('a' - 10);
			else if (key >= 'A' && key <= 'F')
				nibble -= ('A' - 10);

			regBuffer[r->i] = (value & mask) | (nibble << shift);

			if (regPanelEditor < 4)
				regPanelEditor++;
		}
		else if (regPanelCursor == REG_IM && (key >= '0' && key <= '2'))
			regBuffer[REG_IM] = (word) (key - '0');

		else if (regPanelCursor == REG_IFF && (key == '0' || key == '1')) {
			int shift = (r->width - regPanelEditor) * 4;
			word masked = value & (0xFFFF ^ (0x0F << shift));

			regBuffer[REG_IFF] = masked | ((key - '0') << shift);
		}
		else if (regPanelCursor == REG_FLG && key == ' ') {
			regBuffer[r->i] ^= (0x100 >> regPanelEditor);
		}
		else if (key == K_RETURN) {
			if (regPanelCursor == REG_IM || regPanelCursor == REG_IFF) {
				SystemBus_Write(0xc001fd,
					(regBuffer[REG_IM] & 0x03) |
					((regBuffer[REG_IFF] & 0x01) << 2) |
					((regBuffer[REG_IFF] & 0x10) >> 1)); // IFF/IM
			}
			else
				SystemBus_Write(0xc001f0 + r->i, value);

			DelayUs(1);
			SystemBus_Write(0xc001ff, 0);

			regPanelEditor = 0;
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

	Debugger_Screen();

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | 0); // Enable shell border

	regPanelEditor = 0;

	byte key;
	bool firstTime = true;
	bool updateAll = true;
	bool updateRegs = false;
	bool updateWindow = false;

	while (true) {
		if (updateRegs || updateAll) {
			Debugger_UpdateRegs(updateAll);
			updateRegs = false;
		}
		if (updateWindow || updateAll) {
			Debugger_RefreshRequested(firstTime);
			firstTime = false;

			if (activeWindow == WIN_TRACE)
				Debugger_UpdateTrace();
			else if (activeWindow == WIN_MEMDUMP)
				Debugger_UpdateDump(updateAll);

			updateWindow = false;
		}
		if (updateAll) {
			Debugger_UpdateSidePanel();
			updateAll = false;
		}

		key = GetKey(true);

		if (key == K_F1) {
			Shell_TextViewer("speccy2010.hlp", true);

			Debugger_Screen();
			updateAll = true;
		}
		else if (key == K_F2 && activeWindow == WIN_MEMDUMP) {
			activeWindow = WIN_TRACE;
			Debugger_Screen();
			updateAll = true;
		}
		else if (key == K_F3) {
			if (activeWindow == WIN_TRACE)
				activeWindow = WIN_MEMDUMP;
			else
				dumpWindowAsciiMode = !dumpWindowAsciiMode;

			Debugger_Screen();
			updateAll = true;
		}
		else if (key == K_TAB) {
			if (activeWindow == WIN_MEMDUMP && !activeRegPanel && !dumpWindowAsciiMode && !dumpWindowRightPane)
				dumpWindowRightPane = true;
			else {
				activeRegPanel = !activeRegPanel;
				dumpWindowRightPane = false;
			}

			Debugger_SwitchPanel();

			updateRegs = true;
			updateWindow = true;
		}
		else if (key == K_ESC) {
			DelayMs(100);
			break;
		}
		else if (activeRegPanel && Debugger_TestKeyRegs(key))
			updateRegs = true;
		else if (Debugger_TestKeyTrace(key, &updateAll)) {
			if (key == K_F4 || key == K_F8)
				break;

			updateWindow = true;
		}
	}

	CPU_Start();

	SystemBus_Write(0xc00021, 0); //armVideoPage
	SystemBus_Write(0xc00022, 0); //armBorder
}
//---------------------------------------------------------------------------------------
