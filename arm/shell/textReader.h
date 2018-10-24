#ifndef SHELL_TEXTREADER_H_INCLUDED
#define SHELL_TEXTREADER_H_INCLUDED

#include "../system.h"

class CTextReader {
	FIL fil;
	dword buffer, res;

	int lines;
	bool white;
	dword nextLinePos;

public:
	static const int LINES_MAX = 0x4000;

	CTextReader(const char *fullName, int width);
	~CTextReader();

	inline int GetLines() { return lines; }
	inline int GetFileSize() { return fil.fsize; }

	void ReadLines(int width);
	void SetLine(int i);
	char GetChar();
};

#endif
