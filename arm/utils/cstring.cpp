#include <string.h>
#include "cstring.h"

CString::CString(int _size)
{
	str = 0;
	size = 0;

	if (SetBufferSize(_size))
		str[0] = '\0';
	length = 0;
}

CString::CString(const char *_str)
{
	str = 0;
	size = 0;

	*this = _str;
}

CString::CString(const CString &s)
{
	str = 0;
	size = 0;

	*this = s.str;
}

CString::~CString()
{
	if (str)
		delete str;
}

//-----------------------------------------------------------------------------------------

CString & CString::operator = (const CString &s)
{
	*this = s.str;
	return *this;
}

CString & CString::operator = (const char *_str)
{
	if (_str) {
		if (_str != str) {
			length = strlen(_str);

			if (SetBufferSize(length + 1))
				strcpy(str, _str);
			else
				length = 0;
		}
	}
	else {
		length = 0;
		if (SetBufferSize(length + 1))
			str[0] = '\0';
	}

	return *this;
}

CString & CString::Set(const char *_str, unsigned int maxSize)
{
	if (_str) {
		if (_str != str) {
			length = min(strlen(_str), maxSize);

			if (SetBufferSize(length + 1))
				sniprintf(str, length + 1, "%s", _str);
			else
				length = 0;
		}
	}
	else {
		length = 0;
		if (SetBufferSize(length + 1))
			str[0] = '\0';
	}

	return *this;
}

CString & CString::Format(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	while (true) {
		length = vsniprintf(str, GetBufferSize(), format, ap);

		if (length == -1)
			length = 0;
		if (length < GetBufferSize())
			break;

		if (!SetBufferSize(length + 1)) {
			length = 0;
			break;
		}
	}

	va_end(ap);
	return *this;
}

//-----------------------------------------------------------------------------------------

bool CString::operator == (const CString &s)
{
	if (str && s.str)
		return strcmp(str, s.str) == 0;
	return false;
}

bool CString::operator == (const char *_str)
{
	if (str && _str)
		return strcmp(str, _str) == 0;
	return false;
}

bool CString::operator != (const CString &s)
{
	if (str && s.str)
		return strcmp(str, s.str) != 0;
	return true;
}

bool CString::operator != (const char *_str)
{
	if (str && _str)
		return strcmp(str, _str) != 0;
	return true;
}

//-----------------------------------------------------------------------------------------

void CString::SetSymbol(int position, char symbol)
{
	if (str && position < length) {
		str[position] = symbol;
	}
}

char CString::GetSymbol(int position) const
{
	if (str && position < length) {
		return str[position];
	}

	return 0;
}

//-----------------------------------------------------------------------------------------

int CString::SetBufferSize(int _size)
{
	if (_size < SHORT_PATH)
		_size = SHORT_PATH;

	if (_size > size) {
		_size = (_size + SHORT_PATH - 1);
		_size = _size - _size % SHORT_PATH;

		char *_str = new char[_size];

		if (str) {
			if (_str)
				memcpy(_str, str, size);
			delete str;
		}

		str = _str;

		if (str)
			size = _size;
		else
			size = 0;
	}

	return size;
}

//-----------------------------------------------------------------------------------------

CString & CString::operator += (const CString &_s)
{
	if (str) {
		int slen = _s.length;

		if (SetBufferSize(length + slen + 1)) {
			strcpy(str + length, _s);
			length += slen;
		}
		else
			length = 0;
	}

	return *this;
}

CString & CString::operator += (const char *_str)
{
	if (str && _str) {
		int slen = strlen(_str);

		if (SetBufferSize(length + slen + 1)) {
			strcpy(str + length, _str);
			length += slen;
		}
		else
			length = 0;
	}

	return *this;
}

CString & CString::operator += (char c)
{
	if (c) {
		if (SetBufferSize(length + 2)) {
			str[length++] = c;
			str[length] = 0;
		}
		else
			length = 0;
	}

	return *this;
}

CString & CString::TrimRight(int i)
{
	if (str) {
		if (i >= length) {
			length = 0;
			str[0] = '\0';
		}
		else if (i > 0) {
			length -= i;
			str[length] = '\0';
		}
	}

	return *this;
}

CString & CString::TrimLeft(int i)
{
	if (str) {
		if (i >= length) {
			length = 0;
			str[0] = '\0';
		}
		else if (i > 0) {
			length -= i;
			for (int j = 0; j < length + 1; j++)
				str[j] = str[i++];
		}
	}

	return *this;
}

CString & CString::Delete(int i, int num)
{
	if (str) {
		if (i < 0) {
			num += i;
			i = 0;
		}
		else if (i > length) {
			num = 0;
			i = length;
		}

		if (num > (length - i))
			num = length - i;

		if (num > 0) {
			length -= num;

			for (int j = i; j <= length; j++)
				str[j] = str[j + num];
		}
	}

	return *this;
}

CString & CString::Insert(int i, const CString &_s)
{
	if (str) {
		if (i < 0)
			i = 0;
		else if (i > length)
			i = length;

		int slen = _s.length;

		if (SetBufferSize(length + slen + 1)) {
			for (int j = length; j >= i; j--)
				str[j + slen] = str[j];

			memcpy(&str[i], _s.str, slen);
			length += slen;
		}
		else
			length = 0;
	}

	return *this;
}

CString & CString::Insert(int i, const char *_str)
{
	if (str && _str) {
		if (i < 0)
			i = 0;
		else if (i > length)
			i = length;

		int slen = strlen(_str);

		if (SetBufferSize(length + slen + 1)) {
			for (int j = length; j >= i; j--)
				str[j + slen] = str[j];

			memcpy(&str[i], _str, slen);
			length += slen;
		}
		else
			length = 0;
	}

	return *this;
}

CString & CString::Insert(int i, char c)
{
	if (str && c) {
		if (i < 0)
			i = 0;
		else if (i > length)
			i = length;

		if (SetBufferSize(length + 1 + 1)) {
			for (int j = length; j >= i; j--)
				str[j + 1] = str[j];

			str[i] = c;
			length++;
		}
		else
			length = 0;
	}

	return *this;
}
