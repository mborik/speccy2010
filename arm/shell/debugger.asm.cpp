#include "../system.h"

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

static char ln[32], asmbuf[32];
//---------------------------------------------------------------------------------------
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

char *Shell_DebugDisasm(byte *cmd, unsigned current, int *length)
{
	byte *st = cmd, z80p;
	const char *z80r16, *z80r8;
	bool skipCmd = false;

	z80r16 = z80r16_1, z80r8 = z80r8_1, z80p = 0;
	for (;;) { // z80 prefixes
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
				const char *l1 = ln;
				ln[0] = 0;

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
					*q++ = ' '; // +5 - tab size=5, was 4
				while (*++p)
					*q++ = *p;
			}
			*q = 0;

			strncpy(asmbuf, ln, 16);
			if (length)
				*length = max(cm, cmd + *ptr) - st;
			return asmbuf;
		}

		ptr += (2 * *ptr) + 1;
		while (*ptr++); // skip mask, code and instruction
	}

	strcpy(asmbuf, "???");

	if (length)
		*length = (cmd + 1) - st;
	return asmbuf;
}
//---------------------------------------------------------------------------------------
