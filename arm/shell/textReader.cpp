#include "textReader.h"
#include "screen.h"
#include "../system/sdram.h"

//---------------------------------------------------------------------------------------
CTextReader::CTextReader(const char *fullName, int width)
{
	buffer = Malloc(LINES_MAX, sizeof(fil.fptr));

	if (f_open(&fil, fullName, FA_READ) != FR_OK) {
		lines = -1;
		return;
	}

	ReadLines(width);
}
//---------------------------------------------------------------------------------------
CTextReader::~CTextReader()
{
	f_close(&fil);
	Free(buffer);
}
//---------------------------------------------------------------------------------------
void CTextReader::ReadLines(int width)
{
	f_lseek(&fil, 0);

	lines = 0;
	Write(&fil.fptr, buffer, lines++);

	white = false;
	char c, wasCRLF = 0;
	int pos = 0;

	int lastPos = 0;
	dword lastPtr = 0;
	bool prevWhite = false;

	while (fil.fptr < fil.fsize && lines < LINES_MAX) {
		f_read(&fil, &c, 1, &res);

		if (wasCRLF) {
			if (c == wasCRLF) {
				lastPtr = fil.fptr;
				Write(&lastPtr, buffer, lines - 1);
				lastPtr = 0;
				wasCRLF = 0;
				continue;
			}

			wasCRLF = 0;
		}

		if (c == '\t' || c == ' ') {
			if (white)
				continue;
			else
				white = true;
		}
		else
			white = false;

		pos++;

		if (!white && prevWhite) {
			lastPos = pos - 1;
			lastPtr = fil.fptr - 1;
		}

		if (c == '\n' || c == '\r' || (!white && width > 0 && pos > width)) {
			if (c == '\n' || c == '\r') {
				wasCRLF = c ^ 7;
				lastPos = pos;
				lastPtr = fil.fptr;
			}
			else if (lastPtr == 0) {
				lastPos = pos - 1;
				lastPtr = fil.fptr - 1;
			}

			Write(&lastPtr, buffer, lines++);

			white = false;
			pos -= lastPos;

			lastPos = 0;
			lastPtr = 0;
		}

		prevWhite = white;
	}

	f_lseek(&fil, 0);
}
//---------------------------------------------------------------------------------------
void CTextReader::SetLine(int i)
{
	if (i >= 0 && i < lines) {
		dword ptr;
		Read(&ptr, buffer, i);
		f_lseek(&fil, ptr);

		if (i + 1 < lines)
			Read(&nextLinePos, buffer, i + 1);
		else
			nextLinePos = fil.fsize;
	}
	else {
		f_lseek(&fil, fil.fsize);
	}

	white = false;
}
//---------------------------------------------------------------------------------------
char CTextReader::GetChar()
{
	char result;

	while (true) {
		if (fil.fptr >= nextLinePos)
			return ' ';

		f_read(&fil, &result, 1, &res);
		if (result == '\r')
			continue;

		if (result == '\n' || result == '\t' || result == ' ') {
			if (white)
				continue;

			result = ' ';
			white = true;
		}
		else {
			white = false;
		}

		break;
	}

	return result;
}
//---------------------------------------------------------------------------------------
