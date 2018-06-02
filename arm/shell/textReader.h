#ifndef SHELL_TEXTREADER_H_INCLUDED
#define SHELL_TEXTREADER_H_INCLUDED

#include "../system.h"

class CTextReader {
	FIL fil;
	dword buffer;

	int lines;
	dword nextLinePos;
	bool white;

public:
	static const int LINES_MAX = 0x4000;

public:
	CTextReader(const char *fullName, int size);
	~CTextReader();

	inline int GetLines() { return lines; }
	void SetLine(int i);
	char GetChar();
};

#endif
