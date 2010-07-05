#ifndef UARTS_H_INCLUDED
#define UARTS_H_INCLUDED

#include "system.h"

//------------------------------------------------------------

class CUart
{
	UART_TypeDef *uart;
	dword uart_periph;
	word uart_itline;

	byte *tx_Buffer;
	dword tx_size;

    volatile dword tx_cntr;
	volatile dword tx_write_ptr;
	volatile dword tx_read_ptr;

	byte *rx_Buffer;
	dword rx_size;

	volatile dword rx_cntr;
	volatile dword rx_write_ptr;
	volatile dword rx_read_ptr;

public:
	CUart( UART_TypeDef *_uart, dword _uart_periph, word _uart_itline, dword _tx_size, dword _rx_size );
	~CUart();

	void Init( GPIO_TypeDef *tx_gpio, dword tx_pin, GPIO_TypeDef *rx_gpio, dword rx_pin, int vic_irq );
	void SetBaudRate( dword );

	void IRQHandler();

	dword GetRxCntr();
	dword GetTxCntr();
	dword GetTxFreeBuffer();

	byte ReadByte(); // Чтение из порта 1 байт

	dword ReadFile( byte *s, dword N ); // Чтение из порта N байт
	void WriteFile( byte *s, dword cnt );
};

extern CUart uart0;
extern CUart uart1;
extern CUart uart2;

extern "C" void UART0_IRQHandler();
extern "C" void UART1_IRQHandler();
extern "C" void UART2_IRQHandler();

#endif





