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
