#include "file.h"

bool FileExists(const char *str)
{
	FILINFO finfo;
	char lfn[1];
	finfo.lfname = lfn;
	finfo.lfsize = 0;

	if (f_stat(str, &finfo) == FR_OK)
		return true;
	else
		return false;
}

bool ReadLine(FIL *file, CString &str)
{
	str = "";
	int result = 0;

	while (file->fptr < file->fsize) {
		char c;

		UINT res;
		f_read(file, &c, 1, &res);

		if (res == 0)
			break;
		if (c == '\r')
			continue;

		result++;
		if (c == '\n')
			break;
		str += c;
	}

	return result != 0;
}

bool WriteText(FIL *file, const char *str)
{
	UINT res;
	f_write(file, str, strlen(str), &res);
	if (res != strlen(str))
		return false;
	return true;
}

bool WriteLine(FIL *file, const char *str)
{
	if (!WriteText(file, str))
		return false;
	if (!WriteText(file, "\r\n"))
		return false;

	return true;
}

//---------------------------------------------------------------------------------------

byte read_file(FIL *f, byte *dst, byte sz)
{
	UINT nr;
	if (f_read(f, dst, sz, &nr) != FR_OK || nr != sz) {
		return 0;
	}
	return (byte)nr;
}

byte write_file(FIL *f, byte *dst, byte sz)
{
	UINT nw;
	if (f_write(f, dst, sz, &nw) != FR_OK || nw != sz) {
		return 0;
	}
	return (byte)nw;
}

byte read_le_byte(FIL *f, byte *dst)
{
	return read_file(f, dst, 1);
}

byte read_le_word(FIL *f, word *dst)
{
	byte w[2];
	if (!read_file(f, w, 2)) {
		return 0;
	}
	*dst = (((word)w[1]) << 8) | ((word)w[0]);
	return 2;
}

byte read_le_dword(FIL *f, dword *dst)
{
	byte dw[4];
	if (!read_file(f, dw, 4)) {
		return 0;
	}
	*dst = (((dword)dw[3]) << 24) | (((dword)dw[2]) << 16) | (((dword)dw[1]) << 8) | ((dword)dw[0]);
	return 4;
}
