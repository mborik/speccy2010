#ifndef UTILS_FIFO_H_INCLUDED
#define UTILS_FIFO_H_INCLUDED

#include "../system.h"

//------------------------------------------------------------

class CFifo {
	byte *buffer;

	dword size;
	dword cntr;

	dword write_ptr;
	dword read_ptr;

public:
	CFifo(dword _size);
	~CFifo();

	void Clean();

	inline dword GetCntr() { return cntr; }
	inline dword GetFree() { return size - cntr; }

	byte ReadByte();
	dword ReadFile(byte *s, dword cnt);

	void WriteByte(byte);
	dword WriteFile(byte *s, dword cnt);
};

#endif
