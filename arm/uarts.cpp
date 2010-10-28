#include <string.h>
#include <malloc.h>

#include "system.h"
#include "uarts.h"

CUart::CUart( UART_TypeDef *_uart, dword _uart_periph, word _uart_itline, dword _tx_size, dword _rx_size )
{
	uart = _uart;
	uart_periph = _uart_periph;
	uart_itline = _uart_itline;

	tx_size = rx_size = 0;
	rx_read_ptr = rx_write_ptr = rx_cntr = 0;
	tx_read_ptr = tx_write_ptr = tx_cntr = 0;

	tx_Buffer = new byte[ _tx_size ];
	if( tx_Buffer != 0 ) tx_size = _tx_size;

	rx_Buffer = new byte[ _rx_size ];
	if( rx_Buffer != 0 ) rx_size = _rx_size;
}

CUart::~CUart()
{
	delete tx_Buffer;
	tx_Buffer = 0;
	tx_size = 0;

	delete rx_Buffer;
	rx_Buffer = 0;
	rx_size = 0;
}

void CUart::Init( GPIO_TypeDef *tx_gpio, dword tx_pin, GPIO_TypeDef *rx_gpio, dword rx_pin, int irqPriority )
{
	MRCC_PeripheralClockConfig( uart_periph | MRCC_Peripheral_GPIO, ENABLE );

	//---------------------------------------------

	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure the UART0_Tx as Alternate function Push-Pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin =  tx_pin;
	GPIO_Init(tx_gpio, &GPIO_InitStructure);

	/* Configure the UART0_Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = rx_pin;
	GPIO_Init(rx_gpio, &GPIO_InitStructure);

	SetBaudRate( 115200 );

	//---------------------------------------------

    EIC_IRQInitTypeDef EIC_IRQInitStructure;

	/* Configure the interrupt controller */
	EIC_IRQInitStructure.EIC_IRQChannel = uart_itline;
	EIC_IRQInitStructure.EIC_IRQChannelPriority = irqPriority;
	EIC_IRQInitStructure.EIC_IRQChannelCmd = ENABLE;
	EIC_IRQInit(&EIC_IRQInitStructure);

    EIC_IRQCmd( ENABLE );
}

void CUart::SetBaudRate( dword baudRate )
{
	UART_InitTypeDef UART_InitStructure;
	UART_InitStructure.UART_WordLength = UART_WordLength_8D;
	UART_InitStructure.UART_StopBits = UART_StopBits_1;
	UART_InitStructure.UART_Parity = UART_Parity_No ;
	UART_InitStructure.UART_BaudRate = baudRate;
	UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
	UART_InitStructure.UART_Mode = UART_Mode_Tx_Rx;
	UART_InitStructure.UART_FIFO = UART_FIFO_Disable;
	//UART_InitStructure.UART_FIFO = UART_FIFO_Enable;
	//UART_InitStructure.UART_TxFIFOLevel = UART_FIFOLevel_1_2; /* FIFO size 16 bytes, FIFO level 8 bytes */
	//UART_InitStructure.UART_RxFIFOLevel = UART_FIFOLevel_1_2; /* FIFO size 16 bytes, FIFO level 8 bytes */

	UART_Cmd( uart, DISABLE );
	UART_DeInit(uart);
	UART_Init( uart, &UART_InitStructure );

	UART_ITConfig( uart, UART_IT_Transmit | UART_IT_Receive, ENABLE );
	UART_Cmd( uart, ENABLE );
}

void CUart::IRQHandler()
{
	if( UART_GetITStatus( uart,  UART_IT_Receive ) == SET )
	{
		//UART_ClearITPendingBit( uart, UART_IT_Receive );

		byte temp_byte = UART_ReceiveData( uart );

		if ( rx_cntr < rx_size )
		{
			rx_Buffer[rx_write_ptr++] = temp_byte;

			if ( rx_write_ptr >= rx_size ) rx_write_ptr = 0;
			rx_cntr++;
		}
	}

	while( UART_GetITStatus( uart, UART_IT_Transmit ) == SET )
	{
		if ( tx_cntr )
		{
			byte temp_byte = tx_Buffer[tx_read_ptr++];
			UART_SendData( uart, temp_byte );

			if ( tx_read_ptr >= tx_size ) tx_read_ptr = 0;
			tx_cntr--;
		}
		else UART_ClearITPendingBit( uart, UART_IT_Transmit );
	}
}

dword CUart::GetRxCntr( void )
{
	return rx_cntr;
}

dword CUart::GetTxCntr( void )
{
	return tx_cntr;
}

dword CUart::GetTxFreeBuffer( void )
{
    return tx_size - tx_cntr;
}

dword CUart::ReadFile( byte *s, dword N ) // Чтение из порта N байт
{
	portENTER_CRITICAL();
	if( N > rx_cntr ) N = rx_cntr;
	portEXIT_CRITICAL();

	for ( dword i = 0; i < N; i++ )
	{
		*( s++ ) = rx_Buffer[rx_read_ptr++];
		if ( rx_read_ptr >= rx_size ) rx_read_ptr = 0;
	}

	portENTER_CRITICAL();
	rx_cntr -= N;
	portEXIT_CRITICAL();

	return N;
}

byte CUart::ReadByte() // Чтение из порта 1 байт
{
	if( rx_size == 0 ) return 0;

	byte temp_byte = rx_Buffer[rx_read_ptr++];
	if ( rx_read_ptr >= rx_size ) rx_read_ptr = 0;

	portENTER_CRITICAL();
	rx_cntr--;
	portEXIT_CRITICAL();

	return temp_byte;
}

void CUart::WriteFile( byte *s, dword cnt )
{
	while( cnt )
	{
        dword tx_free = tx_size - tx_cntr;
        if( cnt < tx_free ) tx_free = cnt;

        for ( dword i = 0; i < tx_free; i++ )
        {
            tx_Buffer[tx_write_ptr++] = *( s++ );
            if ( tx_write_ptr >= tx_size ) tx_write_ptr = 0;
        }

        portENTER_CRITICAL();

        tx_cntr += tx_free;
        cnt -= tx_free;

        if( UART_GetFlagStatus( uart, UART_FLAG_TxFIFOFull ) == RESET )
        {
            byte temp_byte = tx_Buffer[tx_read_ptr++];
            UART_SendData( uart, temp_byte );
            if ( tx_read_ptr >= tx_size ) tx_read_ptr = 0;

            tx_cntr--;
        }

        portEXIT_CRITICAL();
	}
}

//-------------------------------------------------------------------

CUart uart0( UART0, MRCC_Peripheral_UART0, UART0_IRQChannel, 0x10, 0x10 );
CUart uart1( UART1, MRCC_Peripheral_UART1, UART1_IRQChannel, 0x10, 0x10 );
CUart uart2( UART2, MRCC_Peripheral_UART2, UART2_IRQChannel, 0x10, 0x10 );

void UART0_IRQHandler()
{
	uart0.IRQHandler();
}

void UART1_IRQHandler()
{
	uart1.IRQHandler();
}

void UART2_IRQHandler()
{
	uart2.IRQHandler();
}


