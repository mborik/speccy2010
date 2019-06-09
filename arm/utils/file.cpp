#include "file.h"

bool FileExists(const char *str)
{
	char lfn[1];
	mem.finfo.lfname = lfn;
	mem.finfo.lfsize = 0;

	return (f_stat(str, &mem.finfo) == FR_OK);
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

int read_file(FIL *f, byte *dst, int sz)
{
	UINT nr;
	if (f_read(f, (char *) dst, sz, &nr) != FR_OK || (int) nr != sz) {
		return 0;
	}
	return (word) nr;
}

int write_file(FIL *f, byte *dst, int sz)
{
#if !_FS_READONLY
	UINT nw;
	if (f_write(f, (const char *) dst, sz, &nw) != FR_OK || (int) nw != sz) {
		return 0;
	}
	return (word) nw;
#else
	return 0;
#endif
}

byte read_le_byte(FIL *f, byte *dst)
{
	return read_file(f, dst, 1);
}

byte read_le_word(FIL *f, word *dst)
{
#if defined(AVR_CTRL)
	byte w[2];
	if (!read_file(f, w, 2)) {
		return 0;
	}

	*dst = (((word) w[1]) << 8) | ((word) w[0]);
	return 2;
#else
	return read_file(f, (byte *) dst, sizeof(*dst));
#endif
}

byte read_le_dword(FIL *f, dword *dst)
{
#if defined(AVR_CTRL)
	byte dw[4];
	if (!read_file(f, dw, 4)) {
		return 0;
	}

	*dst = (((dword) dw[3]) << 24) | (((dword) dw[2]) << 16) | (((dword) dw[1]) << 8) | ((dword) dw[0]);
	return 4;
#else
	return read_file(f, (byte *) dst, sizeof(*dst));
#endif
}
