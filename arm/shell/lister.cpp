#include <math.h>

#include "lister.h"
#include "viewer.h"
#include "screen.h"
#include "dialog.h"
#include "textReader.h"
#include "../utils/dirent.h"
#include "../specKeyboard.h"

#define getword(ptr,x) (ptr[x] + 256 * ptr[x + 1])
static const char *INVALID_MSG = "~ invalid block length or corrupted file ~";
//---------------------------------------------------------------------------------------
int ListTap(FIL *file, int skipLines)
{
	char *lst_output = mem.sname;
	byte *tap_header = mem.sector + 0x80;

	int linesLeft = 0;
	byte j = 0, y = 2;
	word hdlen, length = -1, start, param3;
	UINT pos = 0, res, blklen, total = file->fsize;

	memset(lst_output, 0, sizeof(mem.sname));
	f_lseek(file, 0);

	while (pos < total) {
		if (f_read(file, &hdlen, 2, &res) != FR_OK || res != 2) {
			if (j < VIEWER_LINES) {
				j++;
				DrawStr(0, y++, INVALID_MSG, 42);
			}
			else
				linesLeft++;
			break;
		}

		pos += 2;

		if (hdlen == 19) {	// HEADER
			if (f_read(file, tap_header, 19, &res) != FR_OK || res != 19) {
				if (j < VIEWER_LINES) {
					j++;
					DrawStr(0, y++, INVALID_MSG, 42);
				}
				else
					linesLeft++;
				break;
			}

			pos += 19;

			length = getword(tap_header, 12);
			start  = getword(tap_header, 14);
			param3 = getword(tap_header, 16);

			if (skipLines > 0)
				skipLines--;
			else {
				if (j < VIEWER_LINES) {
					j++;

					// 0   12345  C tstTAPtest  12345 56789 32768
					sprintf(lst_output, "%-3u%6u%15s%6u%12u",
						(unsigned) *tap_header, length, "", length, param3);
					strncpy(lst_output + 13, (char *)(tap_header + 2), 10);

					switch ((unsigned) tap_header[1]) {
						case 0:
							lst_output[11] = 'P';
							if (start < 32768)
								sprintf(lst_output + 31, "%5u", start);
							break;
						case 1:
							lst_output[11] = '#';
							sprintf(lst_output + 31, "  %c()", (char) (tap_header[15] - 0x20));
							break;
						case 2:
							lst_output[11] = '$';
							sprintf(lst_output + 31, " %c$()", (char) (tap_header[15] - 0x60));
							break;
						default:
							lst_output[11] = (tap_header[1] != 3) ? '?' :
								((start == 16384 && length == 6912) ? 'S' : 'C');
							sprintf(lst_output + 31, "%5u", start);
							break;
					}

					lst_output[36] = ' ';
					DrawStr(0, y++, lst_output, 42);
				}
				else
					linesLeft++;
			}
		}
		else {	// HEADLESS
			if (length != (hdlen - 2)) {
				blklen = hdlen;
				length = hdlen - 2;

				if (f_read(file, tap_header, 1, &res) != FR_OK || res != 1) {
					if (j < VIEWER_LINES) {
						j++;
						DrawStr(0, y++, INVALID_MSG, 42);
					}
					else
						linesLeft++;
					break;
				}

				pos++;
				blklen--;

				if (skipLines > 0)
					skipLines--;
				else {
					if (j < VIEWER_LINES) {
						j++;

						sprintf(lst_output, "%-3u%6u  -", (unsigned) *tap_header, length);
						DrawStr(0, y++, lst_output, 42);
					}
					else
						linesLeft++;
				}
			}
			else
				blklen = hdlen;

			pos += blklen;
			if (f_lseek(file, pos) != FR_OK) {
				if (j < VIEWER_LINES) {
					j++;
					DrawStr(0, y++, INVALID_MSG, 42);
				}
				else
					linesLeft++;
				break;
			}
		}
	}

	return linesLeft;
}
//---------------------------------------------------------------------------------------
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//---------------------------------------------------------------------------------------
void ListerCore(int (*drawFn)(FIL *file, int skipLines), const char *fullName, const char *header)
{
	size_t bnlen = strlen(fullName);
	int linesLeft, skipLines = 0;
	bool noMod;
	char key;

restart:
	ClrScr(0117);
	DrawLine(1, 2);
	DrawLine(1, 4);

	DrawAttr8(0, 0, 0115, 32);
	DrawStr(0, 0, header);

	DrawAttr8(0, 23, 0005, 7);
	DrawAttr8(7, 23, 0006, 25);
	DrawFnKeys(1, 23, "4hex", 4);

	if (bnlen > 33)
		display_path(fullName, 57, 23, 33);
	else
		DrawStr(255 - (bnlen * 6), 23, fullName, bnlen);

	if (f_open(&mem.fsrc, fullName, FA_READ) != FR_OK)
		return;

	while (true) {
		linesLeft = (*drawFn)(&mem.fsrc, skipLines);

		key = GetKey();
		noMod = ModComb(MOD_ALT_0 | MOD_CTRL_0 | MOD_SHIFT_0);

		if (key == K_ESC || key == K_F3 || key == K_F10)
			break;
		else if (noMod && key == K_F4) {
			f_close(&mem.fsrc);
			if (!Shell_HexViewer(fullName, false, true))
				goto restart;
			break;
		}
		else if (noMod && key == K_UP && skipLines > 0)
			skipLines--;
		else if (noMod && key == K_DOWN && linesLeft > 0)
			skipLines++;
	}
}
//---------------------------------------------------------------------------------------
void Shell_TapLister(const char *fullName)
{
	ListerCore(&ListTap, fullName, "Flag  Len    Name       Length Start Param");
}
//---------------------------------------------------------------------------------------
