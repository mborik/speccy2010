#ifndef UTILS_CSTRING_H_INCLUDED
#define UTILS_CSTRING_H_INCLUDED

#include "../types.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const int CSTRING_DEF_MIN_SIZE = 0x10;

class CString {
	char *str;

	int size;
	int length;

public:
	CString(int _size = CSTRING_DEF_MIN_SIZE);
	CString(const char *_str);
	CString(const CString &s);

	~CString();

//-----------------------------------------------------------------------------------------

	CString & operator = (const CString &s);
	CString & operator = (const char *_str);

	CString & Set(const char *_str, unsigned int maxSize = 0x100);
	CString & Format(const char *_str, ...);

	CString & operator += (const CString &_s);
	CString & operator += (const char *_str);
	CString & operator += (char c);

	CString & TrimRight(int i);
	CString & TrimLeft(int i);

	CString & Delete(int i, int num);

	CString & Insert(int i, const CString &_s);
	CString & Insert(int i, const char *_str);
	CString & Insert(int i, char c);

//-----------------------------------------------------------------------------------------

	bool operator == (const CString &s);
	bool operator == (const char *_str);

	bool operator != (const CString &s);
	bool operator != (const char *_str);

//-----------------------------------------------------------------------------------------

	operator const char *() const { return str ? str : ""; }

	const char *String() const { return str ? str : ""; }
	inline int Length() const { return length; }

	void SetSymbol(int position, char symbol);
	char GetSymbol(int position) const;

//-----------------------------------------------------------------------------------------

	int SetBufferSize(int _size);
	inline int GetBufferSize() const { return size; }
};

#endif
