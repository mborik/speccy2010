#include<malloc.h>

#include "system.h"
#include "fifo.h"

CFifo::CFifo( dword _size )
{
	size = _size;
	buffer = new byte[ size ];

	if( buffer == 0 ) size = 0;

	cntr = 0;
	write_ptr = 0;
	read_ptr = 0;
}

CFifo::~CFifo()
{
	if( buffer ) delete buffer;

	size = 0;
	buffer = 0;

	cntr = 0;
	write_ptr = 0;
	read_ptr = 0;
}

void CFifo::Clean()
{
	portENTER_CRITICAL();

	cntr = 0;
	write_ptr = 0;
	read_ptr = 0;

	portEXIT_CRITICAL();
}

dword CFifo::GetCntr()
{
	return cntr;
}

dword CFifo::GetFree()
{
	return size - cntr;
}

dword CFifo::ReadFile( byte *s, dword cnt )
{
	portENTER_CRITICAL();
	if( cnt > cntr ) cnt = cntr;
	portEXIT_CRITICAL();

	for ( dword i = 0; i < cnt; i++ )
	{
		*( s++ ) = buffer[ read_ptr++ ];
		if ( read_ptr >= size ) read_ptr = 0;
	}

	portENTER_CRITICAL();
	cntr -= cnt;
	portEXIT_CRITICAL();

	return cnt;
}

byte CFifo::ReadByte()
{
    if( cntr == 0 ) return 0;

	byte temp_byte = buffer[ read_ptr++ ];
	if ( read_ptr >= size ) read_ptr = 0;

	portENTER_CRITICAL();
	cntr--;
	portEXIT_CRITICAL();

	return temp_byte;
}

dword CFifo::WriteFile( byte *s, dword cnt )
{
	portENTER_CRITICAL();
	if( cnt > size - cntr ) cnt = size - cntr;
	portEXIT_CRITICAL();

	for ( dword i = 0; i < cnt; i++ )
	{
		buffer[ write_ptr++ ] = *( s++ );
		if ( write_ptr >= size ) write_ptr = 0;
	}

	portENTER_CRITICAL();
	cntr += cnt;
	portEXIT_CRITICAL();

	return cnt;
}


void CFifo::WriteByte( byte data )
{
    if( cntr == size ) return;

	buffer[ write_ptr++ ] = data;
	if ( write_ptr >= size ) write_ptr = 0;

	portENTER_CRITICAL();
	cntr++;
	portEXIT_CRITICAL();
}



