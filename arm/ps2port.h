#ifndef PS2PORT_H_INCLUDED
#define PS2PORT_H_INCLUDED

#include "system.h"

//------------------------------------------------------------

class CPs2Port
{
	GPIO_TypeDef *clk_gpio;
	dword clk_pin;

	GPIO_TypeDef *data_gpio;
	dword data_pin;

	volatile byte kbdTick;
	byte clkPrev;

	dword pauseCounter;

	byte tempCode;
	byte tempParity;

	byte *rx_buffer;

	dword rx_cntr;
	dword rx_write_ptr;
	dword rx_read_ptr;
	dword rx_size;

public:
	CPs2Port( dword _rx_size );
	~CPs2Port();

	void Init( GPIO_TypeDef *clk_gpio, dword clk_pin, GPIO_TypeDef *data_gpio, dword data_pin );
	void Reset();
	void TimerHandler();

	dword GetRxCntr();
	byte ReadByte(); // Чтение из порта 1 байт
	dword ReadFile( byte *s, dword N ); // Чтение из порта N байт

	void WriteByte( byte );
	void WriteFile( byte *s, dword cnt );
};

#endif





