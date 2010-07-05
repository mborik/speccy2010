#include<malloc.h>

#include "system.h"
#include "ps2port.h"

CPs2Port::CPs2Port( dword _rx_size )
{
	rx_size = _rx_size;
	rx_buffer = new byte[ rx_size ];

	if( rx_buffer == 0 ) rx_size = 0;

	rx_write_ptr = 0;
	rx_read_ptr = 0;
	rx_cntr = 0;

	kbdTick = 0;
	clkPrev = 0;
	pauseCounter = 0;
}

CPs2Port::~CPs2Port()
{
	if( rx_buffer ) delete rx_buffer;

	rx_size = 0;
	rx_buffer = 0;

	rx_write_ptr = 0;
	rx_read_ptr = 0;
	rx_cntr = 0;
}

void CPs2Port::Init( GPIO_TypeDef *_clk_gpio, dword _clk_pin, GPIO_TypeDef *_data_gpio, dword _data_pin )
{
	clk_gpio = _clk_gpio;
	clk_pin = _clk_pin;

	data_gpio = _data_gpio;
	data_pin = _data_pin;

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = clk_pin;
	GPIO_Init( clk_gpio, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = data_pin;
	GPIO_Init( data_gpio, &GPIO_InitStructure );

	Reset();
}

void CPs2Port::Reset()
{
	portENTER_CRITICAL();

	rx_write_ptr = 0;
	rx_read_ptr = 0;
	rx_cntr = 0;

	portEXIT_CRITICAL();

	WriteByte( 0xff );
}

void CPs2Port::TimerHandler()
{
	clk_gpio->PM &= ~clk_pin;
	data_gpio->PM &= ~data_pin;

	byte k_clk = GPIO_ReadBit( clk_gpio, clk_pin );
	byte k_dat = GPIO_ReadBit( data_gpio, data_pin );

	if( clkPrev != k_clk )
	{
	    pauseCounter = 0;
	}
	else
	{
	    pauseCounter++;

	    if( kbdTick != 0 && pauseCounter >= 400 )
	    {
            kbdTick = 0;
	    }
	}

	if( k_clk == Bit_RESET && pauseCounter == 0 )
	{
		if( kbdTick == 0 && k_dat == 0 )
		{
			tempParity = 0;
			kbdTick++;
		}
		else if( kbdTick >= 1 && kbdTick <= 8 )
		{
			tempCode >>= 1;

			if( k_dat != 0 ) tempCode |= 0x80;
			else tempCode &= ~0x80;

			if( k_dat != 0 ) tempParity++;
			kbdTick++;
		}
		else if( kbdTick == 9 )
		{
			if( k_dat != 0 ) tempParity++;
			kbdTick++;
		}
		else if( kbdTick == 10 )
		{
			if ( k_dat && ( tempParity & 0x01 ) != 0 )
			{
				if( tempCode == 0xfa ) {}
				else if( rx_cntr < rx_size )
				{
					rx_buffer[rx_write_ptr++] = tempCode;
					if ( rx_write_ptr >= rx_size ) rx_write_ptr = 0;
					rx_cntr++;
				}
			}

			kbdTick = 0;
		}
		else if( kbdTick >= 12 && kbdTick <= 19 )
		{
			if( ( tempCode & 0x01 ) != 0 )
			{
				GPIO_WriteBit( data_gpio, data_pin, Bit_SET );
				tempParity++;
			}
			else
			{
				GPIO_WriteBit( data_gpio, data_pin, Bit_RESET );
			}

			tempCode >>= 1;
			kbdTick++;
		}
		else if( kbdTick == 20 )
		{
			if( ( tempParity & 0x01 ) != 0 )
			{
				GPIO_WriteBit( data_gpio, data_pin, Bit_SET );
			}
			else
			{
				GPIO_WriteBit( data_gpio, data_pin, Bit_RESET );
			}

			kbdTick++;
		}
		else if( kbdTick == 21 )
		{
			GPIO_InitTypeDef GPIO_InitStructure;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
			GPIO_InitStructure.GPIO_Pin = data_pin;
			GPIO_Init( data_gpio, &GPIO_InitStructure );

			GPIO_WriteBit( data_gpio, data_pin, Bit_SET );
			kbdTick++;
		}
		else if( kbdTick == 22 )
		{
			kbdTick = 0;
		}
	}

	clkPrev = k_clk;
}

dword CPs2Port::GetRxCntr()
{
	return rx_cntr;
}

dword CPs2Port::ReadFile( byte *s, dword N )
{
	portENTER_CRITICAL();
	if( N > rx_cntr ) N = rx_cntr;
	portEXIT_CRITICAL();

	for ( dword i = 0; i < N; i++ )
	{
		*( s++ ) = rx_buffer[rx_read_ptr++];
		if ( rx_read_ptr >= rx_size ) rx_read_ptr = 0;
	}

	portENTER_CRITICAL();
	rx_cntr -= N;
	portEXIT_CRITICAL();

	return N;
}

byte CPs2Port::ReadByte() // ?oaiea ec ii?oa 1 aaeo
{
	volatile byte temp_byte = 0;

	if( rx_cntr == 0 ) return 0;

	temp_byte = rx_buffer[rx_read_ptr++];
	if ( rx_read_ptr >= rx_size ) rx_read_ptr = 0;

	portENTER_CRITICAL();
	rx_cntr--;
	portEXIT_CRITICAL();

	return temp_byte;
}

void CPs2Port::WriteByte( byte data )
{
	while( kbdTick != 0 ) DelayMs( 1 );

	portENTER_CRITICAL();

	kbdTick = 11;

	clk_gpio->PM &= ~clk_pin;
	data_gpio->PM &= ~data_pin;

	GPIO_WriteBit( clk_gpio, clk_pin, Bit_SET );
	GPIO_WriteBit( data_gpio, data_pin, Bit_SET );

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = clk_pin;
	GPIO_Init( clk_gpio, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = data_pin;
	GPIO_Init( data_gpio, &GPIO_InitStructure );

	GPIO_WriteBit( clk_gpio, clk_pin, Bit_RESET );

	portEXIT_CRITICAL();

	DelayUs( 200 );

	portENTER_CRITICAL();

	clk_gpio->PM &= ~clk_pin;
	data_gpio->PM &= ~data_pin;

	GPIO_WriteBit( data_gpio, data_pin, Bit_RESET );
	GPIO_WriteBit( clk_gpio, clk_pin, Bit_SET );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = clk_pin;
	GPIO_Init( clk_gpio, &GPIO_InitStructure );

	tempCode = data;
	tempParity = 1;

	kbdTick = 12;

	portEXIT_CRITICAL();
}

void CPs2Port::WriteFile( byte *s, dword cnt )
{
	while( cnt )
	{
		WriteByte( *s++ );
		cnt--;
	}
}




