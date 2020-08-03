#include <ctype.h>
#include "debugger.h"
#include "debugger.asm.h"
#include "screen.h"
#include "dialog.h"
#include "../system.h"
#include "../specKeyboard.h"

#define BREAKPOINTS_COUNT 4

#define TABSTOP_DUMP  5
#define TABSTOP_INST 15
#define TABSTOP_OPER 20

#define TWF_BRANCH  0x010000
#define TWF_BRADDR  0x020000
#define TWF_LOOPCMD 0x040000
#define TWF_CALLCMD 0x080000
#define TWF_BLKCMD  0x100000
#define TWF_HALTCMD 0x200000
#define TWF_STEPOVR (TWF_HALTCMD | TWF_BLKCMD | TWF_CALLCMD)

#define URQ_LOAD_PC 0x010000
#define URQ_PAGE_SW 0x020000
#define URQ_BREAKPT 0x040000

//---------------------------------------------------------------------------------------
static byte dbuf[8];
static char line[33], curline[33];
static unsigned stackPos[8] = { -1U }, stackCur[8] = { -1U };

extern byte activeWindow;
extern bool activeRegPanel;

unsigned breakpoints[BREAKPOINTS_COUNT];
byte cpuTraceCol = 2;
word cpuPCTrace[WINDOW_HEIGHT + 1];
byte cpuCursorY;
word cpuTraceCur, cpuTraceTop;
unsigned cpuTraceFlg, cpuNextPC = -1U;
unsigned reqUpdateRefresh = 0;
bool allowROMAccess = false;

// cursor column / width / buf.offset / char.count
static const byte cs[3][4] = {
	{ 0, 4, 0, 4 },
	{ 4, 8, TABSTOP_DUMP, 10 },
	{ 12, 12, TABSTOP_INST, 16 }
};
static const byte flags[] = { 0x40, 0x01, 0x04, 0x80 }; // ZF,CF,PV,SF

//---------------------------------------------------------------------------------------
void disasm_genidx(char *buf, byte z80p, byte *cm)
{
	char cmd = *(char *) cm;

	char ixy = (z80p == 0xDD ? 'x' : 'y');
	char sig = cmd >= 0 ? '+' : '-';

	sprintf(buf, "(i%c%c%02X)", ixy, sig, cmd > 0 ? cmd : (-cmd));
}

int disasm(byte *cmd, unsigned current)
{
	static char ln[17];

	byte *st = cmd, z80p;
	const char *z80r16, *z80r8, *l1;
	char *asmbuf = line + TABSTOP_INST;
	bool skipCmd = false;

	z80r16 = z80r16_1, z80r8 = z80r8_1, z80p = 0;
	for (int t = 0; t < 4; t++) { // z80 prefixes
		if (*cmd == 0xDD)
			z80r16 = z80r16_2, z80r8 = z80r8_2, z80p = 0xDD;
		else if (*cmd == 0xFD)
			z80r16 = z80r16_3, z80r8 = z80r8_3, z80p = 0xFD;
		else
			break;
		cmd++;
	}

	for (byte *ptr = (byte *) asm_tab_z80; *ptr;) {
		// cmd - start of command, c1 - mod/rm, cm - current pointer
		byte *rcmd = cmd;
		if (*cmd == 0xED)
			rcmd++, z80r16 = z80r16_1, z80r8 = z80r8_1, z80p = 0;
		byte *cm = rcmd + 1;

		for (int j = 0; j < *ptr; j++) { // match mask
			if ((cmd[j] & ptr[j + *ptr + 1]) != ptr[j + 1]) {
				skipCmd = true;
				break;
			}
		}

		if (skipCmd)
			skipCmd = false;
		else {
			*asmbuf = 0;
			for (byte *pt = ptr + (2 * *ptr) + 1; *pt; pt++) { // scan all commands
				*ln = 0;
				l1 = ln;

				switch (*pt) {
					case _zr16: // in rcmd & 0x30
						l1 = z80r16 + 3 * ((*rcmd >> 4) & 3);
						break;
					case _zr16a: // in rcmd & 0x30
						if (((*rcmd >> 4) & 3) == 3)
							l1 = "af";
						else
							l1 = z80r16 + 3 * ((*rcmd >> 4) & 3);
						break;
					case _hl: // hl/ix/iy
						l1 = z80r16 + 3 * 2;
						break;
					case _zjr: // relative jumps
						l1 = zjr + 7 * ((*rcmd >> 3) & 7);
						break;
					case _zop: // z80 operations at rcmd & 0x38
						l1 = zop + 8 * ((*rcmd >> 3) & 7);
						break;
					case _zf: // z80 flags at rcmd & 0x38
						l1 = zf + 3 * ((*rcmd >> 3) & 7);
						break;
					case _cb: // all CB-opcodes
					{
						if (!z80p) {
							sprintf(ln, "%s%s", cbtab + (*cm >> 3) * 7, z80r8_1 + (*cm & 7) * 5);
							cm++;
						}
						else {
							if ((cm[1] & 7) != 6 && ((cm[1] & 0xC0) != 0x40)) { // operand is reg,(ix+nn)
								int offset = sprintf(ln, "%s%s,", cbtab + (cm[1] >> 3) * 7, z80r8_1 + (cm[1] & 7) * 5);
								disasm_genidx(ln + offset, z80p, cm);
							}
							else // only (ix+nn)
								disasm_genidx(ln + sprintf(ln, "%s", cbtab + (cm[1] >> 3) * 7), z80p, cm);
							cm += 2;
						}
						break;
					}
					case _zr8: // in rcmd & 0x38
						if (z80p && ((*rcmd & 0x38) == 0x30)) {
							disasm_genidx(ln, z80p, cm);
							cm++;
						}
						else
							l1 = z80r8 + 5 * ((*rcmd >> 3) & 7);
						break;
					case _zr8_: // in rcmd & 0x38, in ld r8,r8
						if (!z80p || (*rcmd & 7) == 6) {
							l1 = z80r8_1 + 5 * ((*rcmd >> 3) & 7);
							break;
						}
						if ((*rcmd & 0x38) == 0x30) {
							disasm_genidx(ln, z80p, cm);
							cm++;
						}
						else
							l1 = z80r8 + 5 * ((*rcmd >> 3) & 7);
						break;
					case _zr81: // in rcmd & 7
						if (z80p && (*rcmd & 7) == 6) {
							disasm_genidx(ln, z80p, cm);
							cm++;
						}
						else
							l1 = z80r8 + 5 * (*rcmd & 7);
						break;
					case _zr81_: // in rcmd & 7, in ld r8,r8
						if (!z80p || ((*rcmd & 0x38) == 0x30)) {
							l1 = z80r8_1 + 5 * (*rcmd & 7);
							break;
						}
						if ((*rcmd & 7) == 6) {
							disasm_genidx(ln, z80p, cm);
							cm++;
						}
						else
							l1 = z80r8 + 5 * (*rcmd & 7);
						break;
					case _ld:
						l1 = "ld ";
						break;
					case _shrt: // short jump
						sprintf(ln, "%04X", 0xFFFF & (current + (( *cm > 0x7f) ? 0xff00 : 0) + cm - st + *(char *) cm + 1));
						cm++;
						break;
					case _ib: // immediate byte at cm
						sprintf(ln, "%02X", *cm++);
						break;
					case _iw: // immediate word at cm
						sprintf(ln, "%04X", (*(cm + 1) * 256) + *cm);
						cm += 2;
						break;
					default:
						*(short *) ln = *pt;
				}

				strcat(asmbuf, l1);
			}

			// make tabulation between instruction and operands
			char *p = asmbuf, *q = ln;
			while (*p != ' ' && *p)
				*q++ = *p++;
			*q++ = *p;
			if (*p) {
				while (q < ln + 5)
					*q++ = ' '; // +5 - tab size=5
				while (*++p)
					*q++ = *p;
			}
			*q = 0;

			strncpy(asmbuf, ln, 16);
			return max(cm, cmd + *ptr) - st;
		}

		ptr += (2 * *ptr) + 1;
		while (*ptr++); // skip mask, code and instruction
	}

	strcpy(asmbuf, "???");
	return (cmd + 1) - st;
}
//---------------------------------------------------------------------------------------
int disasm_line(unsigned addr)
{
	unsigned i;
	for (i = 0; i < 8; i++)
		dbuf[i] = ReadByteAtCursor(0xFFFF & (i + addr));

	memset(line, 0, sizeof(line));
	sprintf(line, "%04X", addr);

	int ptr = TABSTOP_DUMP;
	int len = disasm(dbuf, addr);

	// 8000 ..DDCB0106 rr (ix+1)
	i = len;
	if (len > 4) {
		i = 4;
		*(line + ptr++) = '\x1C';
	}
	for (i = len - i; i < (unsigned) len; i++, ptr += 2)
		sprintf(line + ptr, "%02X", dbuf[i]);
	*(line + ptr) = 0;

	return len;
}
//---------------------------------------------------------------------------------------
unsigned disasm_flags()
{
	unsigned fl = 0;
	word readptr = GetCPUState(REG_PC), base = GetCPUState(REG_HL);
	byte opcode = 0, fstate = GetCPUState(REG_AF) & 0xFF;
	bool ed = false, prefixed = false;

	for (int t = 0; t < 4; t++) {
		opcode = ReadByteAtCursor(readptr++);

		if (opcode == 0xDD) {
			base = GetCPUState(REG_IX);
			prefixed = true;
		}
		else if (opcode == 0xFD) {
			base = GetCPUState(REG_IY);
			prefixed = true;
		}
		else if (opcode == 0xCB)
			prefixed = true;
		else if (opcode == 0xED) {
			ed = true;
			prefixed = true;
		}
		else
			break;
	}

	if (ed) {
		// ldir/lddr | cpir/cpdr | inir/indr | otir/otdr
		if ((opcode & 0xF4) == 0xB0)
			return TWF_BLKCMD;

		// reti/retn
		if ((opcode & 0xC7) != 0x45)
			return 0;

ret:
		readptr = GetCPUState(REG_SP);

		fl = TWF_BRANCH | TWF_BRADDR;
		fl |= ReadByteAtCursor(readptr++);
		fl |= ReadByteAtCursor(readptr) << 8;
		return fl;
	}

	// jp (hl/ix/iy)
	if (opcode == 0xE9)
		return base | TWF_BRANCH | TWF_BRADDR;

	if (prefixed)
		return 0;

	if (opcode == 0x76) { // halt
		fl = TWF_HALTCMD;

		if (GetCPUState(REG_IM) < 2)
			return fl | 0x38;

		// im2
		readptr = GetCPUState(REG_IR);

		fl |= ReadByteAtCursor(readptr++);
		fl |= ReadByteAtCursor(readptr) << 8;
		return fl;
	}

	if (opcode == 0xC9) // ret
		goto ret;

	if (opcode == 0xC3) { // jp
jp:
		fl |= TWF_BRANCH;
		fl |= ReadByteAtCursor(readptr++);
		fl |= ReadByteAtCursor(readptr) << 8;
		return fl;
	}

	if (opcode == 0xCD) { // call
		fl = TWF_CALLCMD;
		goto jp;
	}

	if ((opcode & 0xC1) == 0xC0) {
		byte flag = flags[(opcode >> 4) & 3];
		byte res = fstate & flag;

		if (!(opcode & 0x08))
			res ^= flag;
		if (!res)
			return 0;

		// ret cc
		if ((opcode & 0xC7) == 0xC0)
			goto ret;

		// call cc
		if ((opcode & 0xC7) == 0xC4) {
			fl = TWF_CALLCMD;
			goto jp;
		}

		// jp cc
		if ((opcode & 0xC7) == 0xC2) {
			fl = TWF_LOOPCMD;
			goto jp;
		}
	}

	// rst #xx
	if ((opcode & 0xC7) == 0xC7)
		return (opcode & 0x38) | TWF_CALLCMD | TWF_BRANCH;

	if (!(opcode & 0xC7)) {
		if (!opcode || opcode == 0x08)
			return 0;

		int offs = (signed char) ReadByteAtCursor(readptr++);
		fl = ((unsigned) (offs + readptr)) | TWF_BRANCH;

		// jr
		if (opcode == 0x18)
			return fl;

		fl |= TWF_LOOPCMD;

		// djnz
		if (opcode == 0x10)
			return (GetCPUState(REG_BC) < 256) ? fl : 0;

		// jr cc
		byte flag = flags[(opcode >> 4) & 1];
		byte res = fstate & flag;

		if (!(opcode & 0x08))
			res ^= flag;
		return res ? fl : 0;
	}

	return 0;
}
//---------------------------------------------------------------------------------------
bool disasm_breakpoint(unsigned pc)
{
	for (int n = 1; n < BREAKPOINTS_COUNT; n++)
		if (breakpoints[n] == pc)
			return true;
	return false;
}
//---------------------------------------------------------------------------------------
void push_pos()
{
	memmove(&stackPos[1], &stackPos[0], sizeof stackPos - sizeof *stackPos);
	memmove(&stackCur[1], &stackCur[0], sizeof stackCur - sizeof *stackCur);
	stackPos[0] = cpuTraceTop;
	stackCur[0] = cpuTraceCur;
}
//---------------------------------------------------------------------------------------
void pop_pos()
{
	if (stackPos[0] == -1U)
		return;

	cpuTraceCur = stackCur[0];
	cpuTraceTop = stackPos[0];
	memcpy(&stackPos[0], &stackPos[1], sizeof stackPos - sizeof *stackPos);
	memcpy(&stackCur[0], &stackCur[1], sizeof stackCur - sizeof *stackCur);
	stackPos[(sizeof stackPos / sizeof *stackPos) - 1] = -1U;
}
//---------------------------------------------------------------------------------------
unsigned cpu_up(unsigned ip)
{
	unsigned addr = (ip > 8) ? ip - 8 : 0;
	for (unsigned i = 0; i < 8; i++)
		dbuf[i] = ReadByteAtCursor(0xFFFF & (addr + i));

	byte *dispos = dbuf, *prev;
	do {
		prev = dispos;
		dispos += disasm(dispos, 0);
	} while ((unsigned) (dispos - dbuf + addr) < ip);

	return prev - dbuf + addr;
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateTrace()
{
	byte atr0 = activeRegPanel ? COL_NORMAL : COL_ACTIVE;
	word realPC = GetCPUState(REG_PC);
	unsigned y, ii, pc;

	cpuTraceFlg = disasm_flags();

	cpuNextPC = -1U;
	cpuCursorY = 0xFF;
	pc = cpuTraceTop;

	for (ii = 0, y = WINDOW_OFF_Y; ii < WINDOW_HEIGHT; ii++, y++) {
		pc &= 0xFFFF;
		cpuPCTrace[ii] = pc;
		int len = disasm_line(pc);

		byte atr = atr0;
		if (pc == realPC) {
			atr = (atr0 << 3) & 0x7F;

			if (cpuTraceFlg & TWF_STEPOVR)
				cpuNextPC = (realPC + len) & 0xFFFF;
		}
		if (disasm_breakpoint(pc))
			atr = (atr & ~7) | COL_BREAKPT;

		DrawAttr8(0, y, atr, 24);

		DrawStr(  0, y, line, 4);
		DrawStr( 32, y, line + TABSTOP_DUMP, 10);
		DrawStr( 96, y, line + TABSTOP_INST, 16);

		if (pc == cpuTraceCur) {
			cpuCursorY = ii;
			memcpy(curline, line, sizeof(line));

			if (!activeRegPanel) {
				if (cpuTraceCol > 1)
					atr = COL_CURSOR;
				DrawAttr8(cs[cpuTraceCol][0], y, COL_CURSOR, cs[cpuTraceCol][1]);
			}
		}

		char arr = ' ';
		if (cpuTraceFlg & TWF_BRANCH) {
			if (pc == realPC) {
				unsigned addr = cpuTraceFlg & 0xFFFF;
				arr = (addr <= realPC) ? 0x18 : 0x19; // up/down arrow

				if (cpuTraceFlg & TWF_BRADDR) {
					DrawHexNum(168, y, addr, 4, 'A');
					DrawAttr8(21, y, (atr & ~7) | (COL_BACKGND & 7), 3);
				}
			}

			if (pc == (cpuTraceFlg & 0xFFFF))
				arr = 0x11; // left chevron
		}

		DrawChar(193, y, arr);
		pc += len;
	}

	cpuPCTrace[ii] = pc;
}
//---------------------------------------------------------------------------------------
bool Debugger_Editor()
{
	const byte *c = cs[cpuTraceCol];
	char *ptr = curline + c[2], key;

	byte x = c[0] * 8;
	byte y = cpuCursorY + WINDOW_OFF_Y;

	bool result = false;
	int p = 0, s, i;
	int w = c[3];

	ptr[w--] = '\0';
	DrawAttr8(c[0], y, COL_EDITOR, c[1]);

	while (true) {
		for (i = 0; i <= w; i++)
			DrawChar(x + (i * 6), y, ptr[i], false, (i == p));

		key = GetKey();
		s = strlen(ptr);

		if (key == K_RETURN) {
			result = true;
			break;
		}
		else if (key == K_ESC)
			break;
		else if ((key == K_LEFT || key == K_BACKSPACE) && p > 0)
			p--;
		else if (key == K_RIGHT) {
movEdCur:
			if (p < w && ptr[p] != 0)
				p++;
		}
		else if (key == K_HOME)
			p = 0;
		else if (key == K_END)
			p = (s > w) ? w : (s - 1);
		else if (key == K_DELETE && p <= w && s > 1) {
			for (i = p; i < s; i++)
				ptr[i] = ptr[i + 1];
			ptr[i] = '\0';
		}

		else if (cpuTraceCol < 2 && (
				(key >= '0' && key <= '9') ||
				(key >= 'a' && key <= 'f') ||
				(key >= 'A' && key <= 'F'))) {

			ptr[p] = toupper(key);
			goto movEdCur;
		}
		else if (cpuTraceCol == 2 && key >= ' ') {
			ptr[p] = key;
			goto movEdCur;
		}
	}

	return result;
}
//---------------------------------------------------------------------------------------
int Debugger_GetMemAddress(int defaultValue)
{
	int result = -1;

	if (defaultValue >= 0)
		sprintf(curline, "%04X", defaultValue);

	cpuCursorY = cpuTraceCol = 0;
	if (Debugger_Editor())
		sscanf(curline, "%04X", &result);

	return result;
}
//---------------------------------------------------------------------------------------
bool Debugger_TestKeyTrace(byte key, bool *updateAll)
{
	unsigned i, curs = 0;

	if (activeWindow == WIN_TRACE && !activeRegPanel) {
		if (key == K_BACKSPACE)
			pop_pos();

		else if (key == K_HOME)
			cpuTraceTop = cpuTraceCur = GetCPUState(REG_PC);

		else if (key == K_LEFT)
			cpuTraceCol = (cpuTraceCol > 0) ? (cpuTraceCol - 1) : 0;

		else if (key == K_RIGHT)
			cpuTraceCol = (cpuTraceCol < 2) ? (cpuTraceCol + 1) : 2;

		else if (key == K_UP) {
			if (cpuTraceCur > cpuTraceTop) {
				for (i = 1; i < WINDOW_HEIGHT; i++) {
					if (cpuPCTrace[i] == cpuTraceCur)
						cpuTraceCur = cpuPCTrace[i - 1];
				}
			}
			else
				cpuTraceTop = cpuTraceCur = cpu_up(cpuTraceCur);
		}
		else if (key == K_DOWN) {
			for (i = 0; i < WINDOW_HEIGHT; i++) {
				if (cpuPCTrace[i] == cpuTraceCur) {
					cpuTraceCur = cpuPCTrace[i + 1];

					if (i == WINDOW_HEIGHT - 1)
						cpuTraceTop = cpuPCTrace[1];
					break;
				}
			}
		}
		else if (key == K_PAGEUP) {
			for (i = 0; i < WINDOW_HEIGHT; i++)
				if (cpuTraceCur == cpuPCTrace[i])
					curs = i;
			for (i = 0; i < WINDOW_HEIGHT; i++)
				cpuTraceTop = cpu_up(cpuTraceTop);

			Debugger_UpdateTrace();
			reqUpdateRefresh = URQ_PAGE_SW | curs;
		}
		else if (key == K_PAGEDOWN) {
			for (i = 0; i < WINDOW_HEIGHT; i++)
				if (cpuTraceCur == cpuPCTrace[i])
					curs = i;
			cpuTraceTop = cpuPCTrace[WINDOW_HEIGHT];

			Debugger_UpdateTrace();
			reqUpdateRefresh = URQ_PAGE_SW | curs;
		}

		else if (key == K_RETURN && cpuCursorY != 0xFF) {
			if (!Debugger_Editor())
				return true;

			if (!cpuTraceCol) {
				sscanf(curline, "%04X", &curs);
				cpuTraceCur = cpuTraceTop = curs;
			}
			else {
				if (cpuTraceCur < 0x4000 && !allowROMAccess) {
					allowROMAccess = Shell_MessageBox("ROM Access",
						"You are about to modify contents",
						"of (EP)ROM directly in SD-RAM.",
						"You have been warned! Sure?", MB_YESNO);

					if (!allowROMAccess)
						return true;
				}

				if (cpuTraceCol == 1) {
					curs = cpuTraceCur;
					for (char c = 0, *p = (curline + TABSTOP_DUMP); c < 5 && p[0] && p[1]; c++, p += 2) {
						if (isxdigit(p[0]) && isxdigit(p[1])) {
							sscanf(p, "%02X", &i);
							WriteByteAtCursor(curs++, i);
						}
					}

					Debugger_UpdateTrace();
				}
				else {
					// TODO parse & assembly
				}
			}
		}
		else if (key == ' ') {
			int firstFree = 0, same = 0;
			for (i = 1; i < BREAKPOINTS_COUNT; i++) {
				if (!same && breakpoints[i] == cpuTraceCur)
					same = i;
				else if (!firstFree && breakpoints[i] == -1U)
					firstFree = i;
			}

			if (same) {
				SystemBus_Write(0xc00009, same);
				breakpoints[same] = -1U;
			}
			else if (firstFree) {
				breakpoints[firstFree] = cpuTraceCur;
				SystemBus_Write(0xc00009, 0x8000 | firstFree);
				SystemBus_Write(0xc0000A, cpuTraceCur);
			}
			else
				Shell_Toast("No more breakpoint", "markers available.");
		}
		else if (key == 'g' || key == 'm') {
			if ((curs = Debugger_GetMemAddress()) != -1U) {
				if (key == 'g')
					push_pos();
				cpuTraceCur = cpuTraceTop = curs;
			}
		}
		else if (key == 'z') {
			SystemBus_Write(0xc001f0 + REG_PC, cpuTraceCur);
			DelayUs(1);
			SystemBus_Write(0xc001ff, 0);

			*updateAll = true;
		}
		else if (key == '\'') {
			char *ptr = NULL;
			for (char *p = (curline + TABSTOP_OPER); p[3] != 0; p++)
				if (isxdigit(p[0]) && isxdigit(p[1]) && isxdigit(p[2]) && isxdigit(p[3]))
					ptr = p;

			if (ptr) {
				push_pos();
				sscanf(ptr, "%04X", &curs);
				cpuTraceCur = cpuTraceTop = curs;
			}
		}

		else goto tstracekeys;

		return true;
	}

tstracekeys:
	// one step
	if (key == K_F7 || (key == K_F8 && cpuNextPC == -1U)) {
		SystemBus_Write(0xc00008, 1);
		DelayMs(1);
		DiskIF_Routine();

		reqUpdateRefresh = URQ_LOAD_PC;
		*updateAll = true;
	}
	// trace to cursor / step over
	else if (key == K_F4 || key == K_F8) {
		breakpoints[0] = (key == K_F8) ? cpuNextPC : cpuTraceCur;

		SystemBus_Write(0xc00009, 0x8000);
		SystemBus_Write(0xc0000A, breakpoints[0]);

		reqUpdateRefresh = URQ_BREAKPT;
		return true;
	}
	else
		return Debugger_TestKeyDump(key, updateAll);

	return false;
}
//---------------------------------------------------------------------------------------
void Debugger_RefreshRequested(bool firstTime)
{
	if (reqUpdateRefresh & URQ_LOAD_PC || firstTime) {
		word newpc = GetCPUState(REG_PC);

		cpuTraceCur = newpc;

		if (firstTime || newpc < cpuTraceTop || newpc >= cpuPCTrace[WINDOW_HEIGHT] || cpuCursorY == 0xFF)
			cpuTraceTop = newpc;
	}
	else if (reqUpdateRefresh & URQ_PAGE_SW)
		cpuTraceCur = cpuPCTrace[reqUpdateRefresh & 0xFFFF];

	if (reqUpdateRefresh & URQ_BREAKPT) {
		SystemBus_Write(0xc00009, 0);
		breakpoints[0] = -1U;
	}

	reqUpdateRefresh = 0;
}
//---------------------------------------------------------------------------------------
void Debugger_HandleBreakpoint()
{
	if ((SystemBus_Read(0xc00000) & 0x0001) == 1) {
		unsigned pc = SystemBus_Read(0xc001f0 + REG_PC);
		for (int n = 0; n < BREAKPOINTS_COUNT; n++)
			if (breakpoints[n] == pc)
				return Shell_Debugger();

		__TRACE("\nBREAKPOINT MISSED! (pc:%04X)", pc);
		CPU_Start();
	}
}
//---------------------------------------------------------------------------------------
void Debugger_Init()
{
	for (int n = 0; n < BREAKPOINTS_COUNT; n++) {
		SystemBus_Write(0xc00009, n);
		breakpoints[n] = -1U;
	}
}
//---------------------------------------------------------------------------------------
