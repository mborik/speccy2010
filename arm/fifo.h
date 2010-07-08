#ifndef FIFO_H_INCLUDED
#define FIFO_H_INCLUDED

#include "system.h"

//------------------------------------------------------------

class CFifo
{
	byte *buffer;

	dword size;
	dword cntr;

	dword write_ptr;
	dword read_ptr;

public:
	CFifo( dword _size );
	~CFifo();

    void Clean();

	dword GetCntr();
	dword GetFree();

	byte ReadByte();
	dword ReadFile( byte *s, dword cnt );

	void WriteByte( byte );
	dword WriteFile( byte *s, dword cnt );
};

#endif





