#include <ctype.h>
#include "debugger.h"
#include "screen.h"
#include "dialog.h"
#include "../system.h"
#include "../specKeyboard.h"

#define BREAKPOINTS_COUNT 4

#define TABSTOP_DUMP  5
#define TABSTOP_INST 15
#define TABSTOP_OPER 20

#define TRACEWIN_OFF_Y  6
#define TRACEWIN_WIDTH (24 - TRACEWIN_OFF_Y)

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

unsigned breakpoints[BREAKPOINTS_COUNT];
byte cpuTraceCol = 2;
word cpuPCTrace[TRACEWIN_WIDTH + 1];
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
#define _iw       9
#define _ib      10
#define _shrt    27
#define _ld    0x89
#define _zr16  0x8A
#define _zr8   0x8B
#define _cb    0x8C
#define _zjr   0x8D
#define _hl    0x8E
#define _zr81  0x8F
#define _zop   0x90
#define _zf    0x91
#define _zr16a 0x92
#define _zr8_  0x9D
#define _zr81_ 0x9E

static const byte asm_tab_z80[] = {
	2, 0xED, 0x70, 0xFF, 0xFF, // in (c)
	'i', 'n', ' ', '(', 'c', ')', 0,
	2, 0xED, 0x71, 0xFF, 0xFF, // out (c),0
	'o', 'u', 't', ' ', '(', 'c', ')', ',', '0', 0,
	2, 0xED, 0x40, 0xFF, 0xC7, // in r8,(c)
	'i', 'n', ' ', _zr8, ',', '(', 'c', ')', 0,
	2, 0xED, 0x41, 0xFF, 0xC7, // out (c),r8
	'o', 'u', 't', ' ', '(', 'c', ')', ',', _zr8, 0,

	1, 0xCB, 0xFF, // all cb-opcodes
	_cb, 0,
	1, 0x00, 0xFF, // nop
	'n', 'o', 'p', 0,
	1, 0x08, 0xFF, // ex af,af'
	'e', 'x', ' ', 'a', 'f', ',', 'a', 'f', '\'', 0,
	1, 0x02, 0xFF, // ld (bc),a
	_ld, '(', 'b', 'c', ')', ',', 'a', 0,
	1, 0x12, 0xFF, // ld (de),a
	_ld, '(', 'd', 'e', ')', ',', 'a', 0,
	1, 0x0A, 0xFF, // ld a,(bc)
	_ld, 'a', ',', '(', 'b', 'c', ')', 0,
	1, 0x1A, 0xFF, // ld a,(de)
	_ld, 'a', ',', '(', 'd', 'e', ')', 0,
	1, 0x03, 0xCF, // inc r16
	'i', 'n', 'c', ' ', _zr16, 0,
	1, 0x0B, 0xCF, // dec r16
	'd', 'e', 'c', ' ', _zr16, 0,
	1, 0x04, 0xC7, // inc r8
	'i', 'n', 'c', ' ', _zr8, 0,
	1, 0x05, 0xC7, // dec r8
	'd', 'e', 'c', ' ', _zr8, 0,
	1, 0x07, 0xFF, // rlca
	'r', 'l', 'c', 'a', 0,
	1, 0x17, 0xFF, // rla
	'r', 'l', 'a', 0,
	1, 0x27, 0xFF, // daa
	'd', 'a', 'a', 0,
	1, 0x37, 0xFF, // scf
	's', 'c', 'f', 0,
	1, 0x0F, 0xFF, // rrca
	'r', 'r', 'c', 'a', 0,
	1, 0x1F, 0xFF, // rra
	'r', 'r', 'a', 0,
	1, 0x2F, 0xFF, // cpl
	'c', 'p', 'l', 0,
	1, 0x3F, 0xFF, // ccf
	'c', 'c', 'f', 0,
	1, 0x00, 0xC7, // relative jumps
	_zjr, _shrt, 0,
	1, 0x09, 0xCF, // add hl, r16
	'a', 'd', 'd', ' ', _hl, ',', _zr16, 0,
	1, 0x32, 0xFF, // ld (nnnn),a
	_ld, '(', _iw, ')', ',', 'a', 0,
	1, 0x3A, 0xFF, // ld a,(nnnn)
	_ld, 'a', ',', '(', _iw, ')', 0,
	1, 0x22, 0xFF, // ld (nnnn),hl
	_ld, '(', _iw, ')', ',', _hl, 0,
	1, 0x2A, 0xFF, // ld hl,(nnnn)
	_ld, _hl, ',', '(', _iw, ')', 0,
	1, 0x76, 0xFF, // halt
	'h', 'a', 'l', 't', 0,
	1, 0x40, 0xC0, // ld r8,r8
	_ld, _zr8_, ',', _zr81_, 0,
	1, 0x80, 0xC0, // op r8
	_zop /*, ' '*/, _zr81, 0,
	1, 0xC0, 0xC7, // ret cc
	'r', 'e', 't', ' ', _zf, 0,
	1, 0xC2, 0xC7, // jp cc, nnnn
	'j', 'p', ' ', _zf, ',', _iw, 0,
	1, 0xC4, 0xC7, // call cc, nnnn
	'c', 'a', 'l', 'l', ' ', _zf, ',', _iw, 0,
	1, 0xC6, 0xC7, // op immb
	_zop /*, ' '*/, _ib, 0,
	1, 0xC1, 0xCF, // pop r16a
	'p', 'o', 'p', ' ', _zr16a, 0,
	1, 0xC5, 0xCF, // push r16a
	'p', 'u', 's', 'h', ' ', _zr16a, 0,
	1, 0xC3, 0xFF, // jp nnnn
	'j', 'p', ' ', _iw, 0,
	1, 0xD3, 0xFF, // out (nn),a
	'o', 'u', 't', ' ', '(', _ib, ')', ',', 'a', 0,
	1, 0xE3, 0xFF, // ex (sp),hl
	'e', 'x', ' ', '(', 's', 'p', ')', ',', _hl, 0,
	1, 0xF3, 0xFF, // di
	'd', 'i', 0,
	1, 0xC9, 0xFF, // ret
	'r', 'e', 't', 0,
	1, 0xD9, 0xFF, // exx
	'e', 'x', 'x', 0,
	1, 0xE9, 0xFF, // jp (hl)
	'j', 'p', ' ', '(', _hl, ')', 0,
	1, 0xF9, 0xFF, // ld sp, hl
	_ld, 's', 'p', ',', _hl, 0,
	1, 0xDB, 0xFF, // in a,(nn)
	'i', 'n', ' ', 'a', ',', '(', _ib, ')', 0,
	1, 0xEB, 0xFF, // ex de,hl  - no 'ex de,ix' !
	'e', 'x', ' ', 'd', 'e', ',', 'h', 'l', 0,
	1, 0xFB, 0xFF, // ei
	'e', 'i', 0,
	1, 0xCD, 0xFF, // call nnnn
	'c', 'a', 'l', 'l', ' ', _iw, 0,
	1, 0xC7, 0xFF, // rst 0
	'r', 's', 't', ' ', '0', 0,
	1, 0xCF, 0xFF, // rst 8
	'r', 's', 't', ' ', '8', 0,
	1, 0xD7, 0xFF, // rst 10
	'r', 's', 't', ' ', '1', '0', 0,
	1, 0xDF, 0xFF, // rst 18
	'r', 's', 't', ' ', '1', '8', 0,
	1, 0xE7, 0xFF, // rst 20
	'r', 's', 't', ' ', '2', '0', 0,
	1, 0xEF, 0xFF, // rst 28
	'r', 's', 't', ' ', '2', '8', 0,
	1, 0xF7, 0xFF, // rst 30
	'r', 's', 't', ' ', '3', '0', 0,
	1, 0xFF, 0xFF, // rst 38
	'r', 's', 't', ' ', '3', '8', 0,

	// ED opcodes
	2, 0xED, 0x42, 0xFF, 0xCF, // sbc hl,r16
	's', 'b', 'c', ' ', 'h', 'l', ',', _zr16, 0,
	2, 0xED, 0x4A, 0xFF, 0xCF, // adc hl,r16
	'a', 'd', 'c', ' ', 'h', 'l', ',', _zr16, 0,
	2, 0xED, 0x43, 0xFF, 0xCF, // ld (nnnn), r16
	_ld, '(', _iw, ')', ',', _zr16, 0,
	2, 0xED, 0x4B, 0xFF, 0xCF, // ld r16, (nnnn)
	_ld, _zr16, ',', '(', _iw, ')', 0,
	2, 0xED, 0x44, 0xFF, 0xC7, // neg
	'n', 'e', 'g', 0,
	2, 0xED, 0x45, 0xFF, 0xCF, // retn
	'r', 'e', 't', 'n', 0,
	2, 0xED, 0x4D, 0xFF, 0xCF, // reti
	'r', 'e', 't', 'i', 0,
	2, 0xED, 0x46, 0xFF, 0xDF, // im 0
	'i', 'm', ' ', '0', 0,
	2, 0xED, 0x56, 0xFF, 0xDF, // im 1
	'i', 'm', ' ', '1', 0,
	2, 0xED, 0x5E, 0xFF, 0xDF, // im 2
	'i', 'm', ' ', '2', 0,
	2, 0xED, 0x4E, 0xFF, 0xDF, // im 0/1
	'i', 'm', ' ', '0', '/', '1', 0,
	2, 0xED, 0x47, 0xFF, 0xFF, // ld i,a
	_ld, 'i', ',', 'a', 0,
	2, 0xED, 0x57, 0xFF, 0xFF, // ld a,i
	_ld, 'a', ',', 'i', 0,
	2, 0xED, 0x67, 0xFF, 0xFF, // rrd
	'r', 'r', 'd', 0,
	2, 0xED, 0x4F, 0xFF, 0xFF, // ld r,a
	_ld, 'r', ',', 'a', 0,
	2, 0xED, 0x5F, 0xFF, 0xFF, // ld a,r
	_ld, 'a', ',', 'r', 0,
	2, 0xED, 0x6F, 0xFF, 0xFF, // rld
	'r', 'l', 'd', 0,

	2, 0xED, 0xA0, 0xFF, 0xFF, // ldi
	'l', 'd', 'i', 0,
	2, 0xED, 0xA1, 0xFF, 0xFF, // cpi
	'c', 'p', 'i', 0,
	2, 0xED, 0xA2, 0xFF, 0xFF, // ini
	'i', 'n', 'i', 0,
	2, 0xED, 0xA3, 0xFF, 0xFF, // outi
	'o', 'u', 't', 'i', 0,
	2, 0xED, 0xA8, 0xFF, 0xFF, // ldd
	'l', 'd', 'd', 0,
	2, 0xED, 0xA9, 0xFF, 0xFF, // cpd
	'c', 'p', 'd', 0,
	2, 0xED, 0xAA, 0xFF, 0xFF, // ind
	'i', 'n', 'd', 0,
	2, 0xED, 0xAB, 0xFF, 0xFF, // outd
	'o', 'u', 't', 'd', 0,

	2, 0xED, 0xB0, 0xFF, 0xFF, // ldir
	'l', 'd', 'i', 'r', 0,
	2, 0xED, 0xB1, 0xFF, 0xFF, // cpir
	'c', 'p', 'i', 'r', 0,
	2, 0xED, 0xB2, 0xFF, 0xFF, // inir
	'i', 'n', 'i', 'r', 0,
	2, 0xED, 0xB3, 0xFF, 0xFF, // otir
	'o', 't', 'i', 'r', 0,
	2, 0xED, 0xB8, 0xFF, 0xFF, // lddr
	'l', 'd', 'd', 'r', 0,
	2, 0xED, 0xB9, 0xFF, 0xFF, // cpdr
	'c', 'p', 'd', 'r', 0,
	2, 0xED, 0xBA, 0xFF, 0xFF, // indr
	'i', 'n', 'd', 'r', 0,
	2, 0xED, 0xBB, 0xFF, 0xFF, // otdr
	'o', 't', 'd', 'r', 0,

	2, 0xED, 0x00, 0xFF, 0x00, // all others 'ED'
	'n', 'o', 'p', '*', 0,

	// place immediates after all - 'ld a,b' is not 'ld a,0B'
	1, 0x01, 0xCF, // ld r16,imm16
	_ld, _zr16, ',', _iw, 0,
	1, 0x06, 0xC7, // ld r8, imm8
	_ld, _zr8, ',', _ib, 0,

	0 // end
};

static const char *z80r16_1 = "bc\0de\0hl\0sp";
static const char *z80r16_2 = "bc\0de\0ix\0sp";
static const char *z80r16_3 = "bc\0de\0iy\0sp";
static const char *z80r8_1 = "b\0\0\0\0c\0\0\0\0d\0\0\0\0e\0\0\0\0h\0\0\0\0l\0\0\0\0(hl)\0a";
static const char *z80r8_2 = "b\0\0\0\0c\0\0\0\0d\0\0\0\0e\0\0\0\0xh\0\0\0xl\0\0\0(1x)\0a";
static const char *z80r8_3 = "b\0\0\0\0c\0\0\0\0d\0\0\0\0e\0\0\0\0yh\0\0\0yl\0\0\0(1y)\0a";
static const char *cbtab = "rlc \0\0\0rrc \0\0\0rl \0\0\0\0rr \0\0\0\0sla \0\0\0sra \0\0\0sli \0\0\0srl \0\0\0"
				"bit 0,\0bit 1,\0bit 2,\0bit 3,\0bit 4,\0bit 5,\0bit 6,\0bit 7,\0"
				"res 0,\0res 1,\0res 2,\0res 3,\0res 4,\0res 5,\0res 6,\0res 7,\0"
				"set 0,\0set 1,\0set 2,\0set 3,\0set 4,\0set 5,\0set 6,\0set 7,\0";
static const char *zjr = "xxxxxx\0xxxxxx\0djnz \0\0jr \0\0\0\0jr nz,\0jr z,\0\0jr nc,\0jr c,\0";
static const char *zop = "add a,\0\0adc a,\0\0sub \0\0\0\0sbc a,\0\0and \0\0\0\0xor \0\0\0\0or \0\0\0\0\0cp \0\0\0\0\0";
static const char *zf = "nz\0z\0\0nc\0c\0\0po\0pe\0p\0\0m";

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
						sprintf(ln, "%04X", 0xFFFF & (current + cm - st + *(char *) cm + 1));
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
	byte atr0 = IsRegPanel() ? COL_NORMAL : COL_ACTIVE;
	word realPC = GetCPUState(REG_PC);
	unsigned y, ii, pc;

	cpuTraceFlg = disasm_flags();

	cpuNextPC = -1U;
	cpuCursorY = 0xFF;
	pc = cpuTraceTop;

	for (ii = 0, y = TRACEWIN_OFF_Y; ii < TRACEWIN_WIDTH; ii++, y++) {
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

			if (!IsRegPanel()) {
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
	byte y = cpuCursorY + TRACEWIN_OFF_Y;

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
bool Debugger_TestKeyTrace(byte key, bool *updateAll)
{
	unsigned i, curs = 0;

	if (!IsRegPanel()) {
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
				for (i = 1; i < TRACEWIN_WIDTH; i++) {
					if (cpuPCTrace[i] == cpuTraceCur)
						cpuTraceCur = cpuPCTrace[i - 1];
				}
			}
			else
				cpuTraceTop = cpuTraceCur = cpu_up(cpuTraceCur);
		}
		else if (key == K_DOWN) {
			for (i = 0; i < TRACEWIN_WIDTH; i++) {
				if (cpuPCTrace[i] == cpuTraceCur) {
					cpuTraceCur = cpuPCTrace[i + 1];

					if (i == TRACEWIN_WIDTH - 1)
						cpuTraceTop = cpuPCTrace[1];
					break;
				}
			}
		}
		else if (key == K_PAGEUP) {
			for (i = 0; i < TRACEWIN_WIDTH; i++)
				if (cpuTraceCur == cpuPCTrace[i])
					curs = i;
			for (i = 0; i < TRACEWIN_WIDTH; i++)
				cpuTraceTop = cpu_up(cpuTraceTop);

			Debugger_UpdateTrace();
			reqUpdateRefresh = URQ_PAGE_SW | curs;
		}
		else if (key == K_PAGEDOWN) {
			for (i = 0; i < TRACEWIN_WIDTH; i++)
				if (cpuTraceCur == cpuPCTrace[i])
					curs = i;
			cpuTraceTop = cpuPCTrace[TRACEWIN_WIDTH];

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
		else if (key == 'g') {
			cpuCursorY = cpuTraceCol = 0;
			if (Debugger_Editor()) {
				push_pos();
				sscanf(curline, "%04X", &curs);
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

	return false;
}
//---------------------------------------------------------------------------------------
void Debugger_RefreshRequested(bool firstTime)
{
	if (reqUpdateRefresh & URQ_LOAD_PC || firstTime) {
		word newpc = GetCPUState(REG_PC);

		cpuTraceCur = newpc;

		if (firstTime || newpc < cpuTraceTop || newpc >= cpuPCTrace[TRACEWIN_WIDTH] || cpuCursorY == 0xFF)
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
