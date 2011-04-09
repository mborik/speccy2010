#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>
#include <stdarg.h>

#include "system.h"
#include "uarts.h"
#include "fifo.h"

#include "specTape.h"
#include "specKeyboard.h"
#include "specShell.h"
#include "specConfig.h"
#include "specBetadisk.h"
#include "specRtc.h"

#include "ramFile.h"

bool LOG_BDI_PORTS = false;
bool LOG_WAIT = false;

byte timer_flag_1Hz = 0;
byte timer_flag_100Hz = 0;

bool MRCC_Config(void)
{
	MRCC_DeInit();

	int i = 16;
	while( i > 0 && MRCC_WaitForOSC4MStartUp() != SUCCESS ) i--;

	if( i != 0 )
	{
		/* Set HCLK to 60 MHz */
		MRCC_HCLKConfig(MRCC_CKSYS_Div1);
		/* Set CKTIM to 60 MHz */
		MRCC_CKTIMConfig(MRCC_HCLK_Div1);
		/* Set PCLK to 30 MHz */
		MRCC_PCLKConfig(MRCC_CKTIM_Div2);

		CFG_FLASHBurstConfig(CFG_FLASHBurst_Enable);
		MRCC_CKSYSConfig(MRCC_CKSYS_OSC4MPLL, MRCC_PLL_Mul_15);
	}

	/* GPIO pins optimized for 3V3 operation */
	MRCC_IOVoltageRangeConfig(MRCC_IOVoltageRange_3V3);

	MRCC_PeripheralClockConfig( MRCC_Peripheral_GPIO, ENABLE );
	MRCC_PeripheralClockConfig( MRCC_Peripheral_TIM0, ENABLE );
	MRCC_PeripheralClockConfig( MRCC_Peripheral_I2C, ENABLE );
	MRCC_PeripheralClockConfig( MRCC_Peripheral_SSP0, ENABLE );

	return i > 0;
}

void TIM0_Config()
{
	TIM_InitTypeDef TIM_InitStructure;
	TIM_InitStructure.TIM_Mode = TIM_Mode_OCTiming;
	TIM_InitStructure.TIM_Prescaler = 60 - 1;
	TIM_InitStructure.TIM_ClockSource = TIM_ClockSource_Internal;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Channel = TIM_Channel_2;
	TIM_InitStructure.TIM_Period = 25 - 1;
	TIM_Init(TIM0, &TIM_InitStructure);

	TIM_ClearFlag( TIM0, TIM_FLAG_OC1 | TIM_FLAG_OC2 | TIM_FLAG_Update );
	TIM_ITConfig( TIM0, TIM_IT_Update, ENABLE );
	TIM_Cmd( TIM0, ENABLE );

	EIC_IRQInitTypeDef EIC_IRQInitStructure;

	EIC_IRQInitStructure.EIC_IRQChannel = TIM0_UP_IRQChannel;
	EIC_IRQInitStructure.EIC_IRQChannelPriority = 1;
	EIC_IRQInitStructure.EIC_IRQChannelCmd = ENABLE;
	EIC_IRQInit(&EIC_IRQInitStructure);

	EIC_IRQCmd( ENABLE );
}

void I2C_Config()
{
    /* SCL and SDA I2C pins configuration */
    GPIO_InitTypeDef  GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 ;
    GPIO_Init(GPIO0, &GPIO_InitStructure);

    /* I2C configuration */
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_InitStructure.I2C_GeneralCall = I2C_GeneralCall_Disable;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_CLKSpeed = 100000;
    I2C_InitStructure.I2C_OwnAddress = 0xA0;

    /* I2C Peripheral Enable */
    I2C_Cmd( ENABLE );
    /* Apply I2C configuration after enabling it */
    I2C_Init( &I2C_InitStructure );
}

void SPI_Config()
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIO0, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

	GPIO_WriteBit( GPIO1, GPIO_Pin_10, Bit_SET );
	GPIO_WriteBit( GPIO0, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7, Bit_SET );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_18 | GPIO_Pin_19;
	GPIO_Init( GPIO2, &GPIO_InitStructure );

	//----------------------------------------------------------------------------------------------------

    SSP_InitTypeDef   SSP_InitStructure;
    SSP_InitStructure.SSP_FrameFormat = SSP_FrameFormat_Motorola;
    SSP_InitStructure.SSP_Mode = SSP_Mode_Master;
    SSP_InitStructure.SSP_CPOL = SSP_CPOL_Low;
    SSP_InitStructure.SSP_CPHA = SSP_CPHA_1Edge;
    SSP_InitStructure.SSP_DataSize = SSP_DataSize_8b;
    SSP_InitStructure.SSP_NSS = SSP_NSS_Soft;
    SSP_InitStructure.SSP_ClockRate = 0;
    SSP_InitStructure.SSP_ClockPrescaler = 2; /* SSP baud rate : 30 MHz/(4*(2+1))= 2.5MHz */
    SSP_Init(SSP0, &SSP_InitStructure);

    SSP_Cmd(SSP0, ENABLE);
}

void WDT_Config()
{
    WDG_InitTypeDef WDG_InitStructure;
    WDG_InitStructure.WDG_Mode = WDG_Mode_WDG;
    WDG_InitStructure.WDG_Preload = 0xffff;
    WDG_InitStructure.WDG_Prescaler = 0xff;
    WDG_Init(&WDG_InitStructure);
    WDG_Cmd(ENABLE);
}

void WDT_Kick()
{
    WDG_Cmd(ENABLE);
}

void UART0_WriteText( const char *str )
{
	uart0.WriteFile( (byte*) str, strlen(str) );
	WDT_Kick();
}

const byte RTC_ADDRESS = 0xd0;
const int RTC_TIMEOUT = 100;
tm currentTime;
bool rtcInited = false;

void RTC_Init()
{
    currentTime.tm_sec = 00;
    currentTime.tm_min = 11;
    currentTime.tm_hour = 17;

    currentTime.tm_wday  = 0;
    currentTime.tm_mday = 5;
    currentTime.tm_mon = 11;
    currentTime.tm_year = 110;

    if( RTC_GetTime( NULL ) ) __TRACE( "RTC init OK..\n" );
    else __TRACE( "RTC init error..\n" );
}

bool RTC_GetTime( tm *time )
{
    byte data[ 7 ];
    byte address;

    int rtc_timer = 0;
    address = 0;

    I2C_GenerateSTART(ENABLE);
    while( !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

    I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_TRANSMITTER);
    while( !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

    I2C_Cmd(ENABLE);
    I2C_SendData( address );
    while( !I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

    I2C_GenerateSTART(ENABLE);
    while( !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

    I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_RECEIVER);
    while( !I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

    I2C_Cmd(ENABLE);

    while( address < sizeof( data ) )
    {
        while( !I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_RECEIVED ) && ++rtc_timer < RTC_TIMEOUT ) DelayUs(100);

        if( address == sizeof( data ) - 2 ) I2C_AcknowledgeConfig(DISABLE);
        else if( address == sizeof( data ) - 1 ) I2C_GenerateSTOP(ENABLE);

        data[ address++ ] = I2C_ReceiveData();
    }

    I2C_AcknowledgeConfig(ENABLE);
    //__TRACE( "RTC read time %d..\n", rtc_timer );

    if( rtc_timer < RTC_TIMEOUT )
    {
        currentTime.tm_sec = ( data[0] >> 4 ) * 10 + ( data[0] & 0x0f );
        currentTime.tm_min = ( data[1] >> 4 ) * 10 + ( data[1] & 0x0f );
        currentTime.tm_hour = ( data[2] >> 4 ) * 10 + ( data[2] & 0x0f );

        currentTime.tm_wday  = ( data[3] & 0x0f );
        currentTime.tm_mday = ( data[4] >> 4 ) * 10 + ( data[4] & 0x0f );
        currentTime.tm_mon = ( data[5] >> 4 ) * 10 + ( data[5] & 0x0f ) - 1;
        currentTime.tm_year = 100 + ( data[6] >> 4 ) * 10 + ( data[6] & 0x0f );

        rtcInited = true;
    }
    else
    {
        rtcInited = false;
    }

    if( time ) memcpy( time, &currentTime, sizeof( tm ) );
    return rtcInited;
}

bool RTC_SetTime( tm *newTime )
{
    tm time;
    memcpy( &time, newTime, sizeof( tm ) );

    time.tm_sec = ( 60 + time.tm_sec ) % 60;
    time.tm_min = ( 60 + time.tm_min ) % 60;
    time.tm_hour = ( 24 + time.tm_hour ) % 24;

    time.tm_mon = ( 12 + time.tm_mon ) % 12;

    int mdayMax = 31;
    if( time.tm_mon == 1 && ( time.tm_year % 4 ) != 0 ) mdayMax = 28;
    else if( time.tm_mon == 1 ) mdayMax = 29;
    else if( time.tm_mon == 3 || time.tm_mon == 5 ) mdayMax = 30;
    else if( time.tm_mon == 8 || time.tm_mon == 10 ) mdayMax = 30;

    if( time.tm_mon != currentTime.tm_mon )
    {
        if( time.tm_mday < 1 ) time.tm_mday = 1;
        else if( time.tm_mday > mdayMax ) time.tm_mday = mdayMax;
    }
    else
    {
        time.tm_mday = 1 + ( mdayMax + time.tm_mday - 1 ) % mdayMax;
    }

    mktime( &time );
    memcpy( &currentTime, &time, sizeof( tm ) );

    if( rtcInited )
    {
        time.tm_mon += 1;
        time.tm_year %= 100;

        byte data[7];
        data[0] = ( ( time.tm_sec / 10 ) << 4 ) | ( time.tm_sec % 10 );
        data[1] = ( ( time.tm_min / 10 ) << 4 ) | ( time.tm_min % 10 );
        data[2] = ( ( time.tm_hour / 10 ) << 4 ) | ( time.tm_hour % 10 );
        data[3] = time.tm_wday;
        data[4] = ( ( time.tm_mday / 10 ) << 4 ) | ( time.tm_mday % 10 );
        data[5] = ( ( time.tm_mon / 10 ) << 4 ) | ( time.tm_mon % 10 );
        data[6] = ( ( time.tm_year / 10 ) << 4 ) | ( time.tm_year % 10 );

        byte address = 0;

        I2C_GenerateSTART(ENABLE);
        while( !I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );

        I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_TRANSMITTER );
        while( !I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECTED ) );

        I2C_Cmd(ENABLE);
        I2C_SendData( address );
        while( !I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );

        I2C_Cmd(ENABLE);
        while( address < sizeof( data ) )
        {
            I2C_SendData( data[ address++ ] );
            while( !I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );
        }

        I2C_GenerateSTOP(ENABLE);
    }

    return rtcInited;
}

DWORD get_fattime()
{
    RTC_GetTime( NULL );

	return	( ( currentTime.tm_year - 80 ) << 25 ) | ( ( currentTime.tm_mon + 1 ) << 21 ) |
                ( currentTime.tm_mday << 16 ) | ( currentTime.tm_hour << 11 ) |
                ( currentTime.tm_min << 5 ) | ( currentTime.tm_sec >> 1 );
}

bool FileExists( const char *str )
{
    FILINFO finfo;
    char lfn[1];
    finfo.lfname = lfn;
    finfo.lfsize = 0;

    if( f_stat( str, &finfo ) == FR_OK ) return true;
    else return false;
}

bool ReadLine( FIL *file, CString &str )
{
    str = "";
    int result = 0;

    while( file->fptr < file->fsize )
    {
        char c;

        UINT res;
        f_read( file, &c, 1, &res );

        if( res == 0 ) break;
        if( c == '\r' ) continue;

        result++;
        if( c == '\n' ) break;
        str += c;
    }

    return result != 0;
}

bool WriteText( FIL *file, const char *str )
{
    UINT res;
    f_write( file, str, strlen( str ), &res );
    if( res != strlen( str ) ) return false;
    return true;
}

bool WriteLine( FIL *file, const char *str )
{
    if( !WriteText( file, str ) ) return false;
    if( !WriteText( file, "\r\n" ) ) return false;

    return true;
}

#define DCLK( x ) GPIO_WriteBit( GPIO1, GPIO_Pin_6, x )
#define DATA( x ) GPIO_WriteBit( GPIO1, GPIO_Pin_7, x )
#define nCONFIG( x ) GPIO_WriteBit( GPIO1, GPIO_Pin_9, x )

#define CONFIG_DONE() GPIO_ReadBit( GPIO1, GPIO_Pin_5 )
#define nSTATUS() GPIO_ReadBit( GPIO1, GPIO_Pin_8 )

dword fpgaConfigVersionPrev = 0;
dword romConfigPrev = -1;

void FPGA_TestClock()
{
    if( fpgaConfigVersionPrev == 0 ) return;

    CPU_Stop();

    SystemBus_Write( 0xc00050, 0 );
    SystemBus_Write( 0xc00050, 1 );
    DelayMs( 100 );
    SystemBus_Write( 0xc00050, 0 );

    dword counter20 = SystemBus_Read( 0xc00050 ) | ( SystemBus_Read( 0xc00051 ) << 16 );
    dword counterMem = SystemBus_Read( 0xc00052 ) | ( SystemBus_Read( 0xc00053 ) << 16 );

    __TRACE( "FPGA clock - %d.%.5d MHz\n", counter20 / 100000, counter20 % 100000 );
    __TRACE( "FPGA PLL clock - %d.%.5d MHz\n", counterMem / 100000, counterMem % 100000 );

    CPU_Start();
}

void FPGA_Config()
{
    if( ( disk_status(0) & STA_NOINIT ) != 0 )
    {
        __TRACE( "Cannot init SD card...\n" );
        return;
    }

    FILINFO fpgaConfigInfo;
    char lfn[1];
    fpgaConfigInfo.lfname = lfn;
    fpgaConfigInfo.lfsize = 0;

    if( f_stat( "speccy2010.rbf", &fpgaConfigInfo ) != FR_OK )
    {
        __TRACE( "Cannot open speccy2010.rbf...\n" );
        return;
    }

    dword fpgaConfigVersion = ( fpgaConfigInfo.fdate << 16 ) | fpgaConfigInfo.ftime;

    //char str[ 0x20 ];
    //sniprintf( str, sizeof(str), "current - %x\n", fpgaConfigVersionPrev );
    //UART0_WriteText( str );
    //sniprintf( str, sizeof(str), "new - %x\n", fpgaConfigVersion );
    //UART0_WriteText( str );

    if( fpgaConfigVersionPrev >= fpgaConfigVersion )
    {
        __TRACE( "The version of speccy2010.rbf is match...\n" );
        return;
    }

    FIL fpgaConfig;
    if( f_open( &fpgaConfig, "speccy2010.rbf", FA_READ ) != FR_OK )
    {
        __TRACE( "Cannot open speccy2010.rbf...\n" );
        return;
    }

    //--------------------------------------------------------------------------

    __TRACE( "FPGA configuration started...\n" );

    GPIO_InitTypeDef  GPIO_InitStructure;

	//BOOT0 pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init( GPIO0, &GPIO_InitStructure );

    //Reset FPGA pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

	nCONFIG( Bit_SET );
	DCLK( Bit_SET );
	DATA( Bit_SET );

	nCONFIG( Bit_RESET );
	DelayMs( 2 );
	nCONFIG( Bit_SET );

    int i = 10;
    while( i-- > 0 && nSTATUS() != Bit_RESET ) DelayMs( 10 );

    if( nSTATUS() == Bit_RESET )
    {
        __TRACE( "FPGA configuration status OK...\n" );

        for( dword pos = 0; pos < fpgaConfig.fsize; pos++ )
        {
            byte data8;

            UINT res;
            if( f_read( &fpgaConfig, &data8, 1, &res ) != FR_OK ) break;
            if( res == 0 ) break;

            for( byte j = 0; j < 8; j++ )
            {
                DCLK( Bit_RESET );

                DATA( ( data8 & 0x01 ) == 0 ? Bit_RESET : Bit_SET );
                data8 >>= 1;

                DCLK( Bit_SET );
            }

            if( ( pos & 0xfff ) == 0 )
            {
                WDT_Kick();
                __TRACE( "." );
            }
        }
    }

    f_close( &fpgaConfig );
    __TRACE( "FPGA configuration conf done...\n" );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

    i = 10;
    while( i-- > 0 && !CONFIG_DONE() ) DelayMs( 10 );

	if( CONFIG_DONE() )
	{
	    __TRACE( "FPGA configuration finished...\n" );
	    fpgaConfigVersionPrev = fpgaConfigVersion;
	}
	else
	{
	    __TRACE( "FPGA configuration failed...\n" );
	    fpgaConfigVersionPrev = 0;
	}

    GPIO_WriteBit( GPIO1, GPIO_Pin_13, Bit_RESET );     // FPGA RESET LOW
    DelayMs( 10 );
    GPIO_WriteBit( GPIO1, GPIO_Pin_13, Bit_SET );     // FPGA RESET HIGH

    if( SystemBus_TestConfiguration() == false )
    {
        __TRACE( "Wrong FPGA configuration...\n" );
        fpgaConfigVersionPrev = 0;
    }
    else
    {

        FPGA_TestClock();
        Keyboard_Reset();
        Mouse_Reset();
    }

    romConfigPrev = -1;
    WDT_Kick();

    timer_flag_1Hz = 0;
    timer_flag_100Hz = 0;
}

const dword ARM_RD = 1 << 8;
const dword ARM_WR = 1 << 9;
const dword ARM_ALE = 1 << 10;
const dword ARM_WAIT = 1 << 11;

bool SystemBus_TestConfiguration()
{
    bool result = true;

    SystemBus_SetAddress( 0xc00000 );

    portENTER_CRITICAL();

    GPIO2->PM = ~( ARM_RD | ARM_WR | ARM_ALE );
    GPIO2->PD = ( ARM_RD | ARM_WR | ARM_ALE );

    GPIO0->PM = ~0xffff0000;
    GPIO0->PC2 = 0x00000000;
    GPIO0->PD = 0xffff0000;

    GPIO2->PM = ~ARM_RD;
    GPIO2->PD = 0;

    DelayUs( 10 );
    if( ( GPIO2->PD & ARM_WAIT ) == 0 ) result = false;

    GPIO2->PD = ARM_RD;
    GPIO0->PC2 = 0xffff0000;

    portEXIT_CRITICAL();

    return result;
}

void SystemBus_SetAddress( dword address )
{
    portENTER_CRITICAL();

    GPIO0->PM = ~0xffff0000;
    GPIO2->PM = ~0x000007ff;

    GPIO0->PD = ( address & 0xffff ) << 16;
    GPIO2->PD = ( ARM_RD | ARM_WR | ARM_ALE ) | ( ( address >> 16 ) & 0xff );

    GPIO2->PM = ~ARM_ALE;
    GPIO2->PD = 0;
    GPIO2->PD = ARM_ALE;

    portEXIT_CRITICAL();

    WDT_Kick();
}

word SystemBus_Read()
{
    portENTER_CRITICAL();

    GPIO0->PM = ~0xffff0000;
    GPIO0->PC2 = 0x00000000;      // input
    //GPIO0->PC1 = 0xffff0000;
    //GPIO0->PC0 = 0xffff0000;
    GPIO0->PD = 0xffff0000;

    GPIO2->PM = ~ARM_RD;
    GPIO2->PD = 0;

    while( ( GPIO2->PD & ARM_WAIT ) == 0 );
    word result = GPIO0->PD >> 16;

    GPIO2->PD = ARM_RD;

    GPIO0->PC2 = 0xffff0000;      // input
    //GPIO0->PC1 = 0x00000000;
    //GPIO0->PC0 = 0xffff0000;

    portEXIT_CRITICAL();

    return result;
}

word SystemBus_Read( dword address )
{
    word result;

    portENTER_CRITICAL();
    SystemBus_SetAddress( address );
    result = SystemBus_Read();
    portEXIT_CRITICAL();

    return result;
}

void SystemBus_Write( word data )
{
    portENTER_CRITICAL();

    GPIO0->PM = ~0xffff0000;
    GPIO2->PM = ~ARM_WR;

    GPIO0->PD = data << 16;
    GPIO2->PD = 0;
    while( ( GPIO2->PD & ARM_WAIT ) == 0 );
    GPIO2->PD = ARM_WR;

    portEXIT_CRITICAL();
}

void SystemBus_Write( dword address, word data )
{
    portENTER_CRITICAL();

    SystemBus_SetAddress( address );
    SystemBus_Write( data );

    portEXIT_CRITICAL();
}

bool Spectrum_LoadRomPage( int page, const char *romFileName )
{
    FIL romImage;
    if( f_open( &romImage, romFileName, FA_READ ) != FR_OK )
    {
        __TRACE( "Cannot open rom image %s \n", romFileName );
        return false;
    }

	bool result = true;

	int writeErrors = 0;
	int readErrors = 0;

    CPU_Stop();
    SystemBus_Write( 0xc00020, 0 );  // use bank 0
    dword addr = 0x800000 | ( ( ( 0x100 + page ) * 0x4000 ) >> 1 );

    f_lseek( &romImage, 0 );

    dword pos;
	for( pos = 0; pos < romImage.fsize; pos += 2 )
	{
	    word data;

	    UINT res;
	    if( f_read( &romImage, &data, 2, &res ) != FR_OK ) break;
	    if( res == 0 ) break;

        word temp;
	    //temp = SystemBus_Read( addr );
        //if( temp != data ) __TRACE( "a: 0x%.4x -> r: 0x%.4x w: 0x%.4x\n", addr << 1, temp, data );

        SystemBus_Write( addr, data );
        temp = SystemBus_Read( addr );

        if( temp != data )
        {
            for( int i = 0; i < 3; i++ )
            {
                temp = SystemBus_Read( addr );
                if( temp == data ) break;
            }

            if( temp == data ) readErrors++;
            else writeErrors++;
        }

        addr++;
        if( ( addr & 0x0fff ) == 0 ) WDT_Kick();
	}

    if( readErrors > 0 ||  writeErrors > 0 )
    {
        __TRACE( "Memory errors !\n" );
        __TRACE( " - read errors : %d\n", readErrors );
        __TRACE( " - write errors : %d\n", writeErrors );

        result = false;
    }

    f_close( &romImage );
    CPU_Start();

    return result;
}

void Spectrum_InitRom()
{
    __TRACE( "ROM configuration started...\n" );

    specConfig.specUseBank0 = Spectrum_LoadRomPage( 0, "roms/system.rom" );
    Spectrum_LoadRomPage( 1, "roms/trdos.rom" );

    if( specConfig.specRom == SpecRom_Classic48 ) Spectrum_LoadRomPage( 3, "roms/48.rom" );
    else Spectrum_LoadRomPage( 2, "roms/pentagon.rom" );

    __TRACE( "ROM configuration finished...\n" );

    SystemBus_Write( 0xc00021, 0x0000 );
    SystemBus_Write( 0xc00022, 0x0000 );

    ResetKeyboard();
    RTC_Update();

    CPU_Reset( true );
    CPU_Reset( false );

    WDT_Kick();
    timer_flag_1Hz = 0;
    timer_flag_100Hz = 0;
}

void Spectrum_UpdateConfig()
{
    if( fpgaConfigVersionPrev == 0 ) return;

    SystemBus_Write( 0xc00040, specConfig.specRom );
    SystemBus_Write( 0xc00041, specConfig.specSync );
    SystemBus_Write( 0xc00042, specConfig.specTurbo );

    SystemBus_Write( 0xc00043, specConfig.specVideoMode );
    SystemBus_Write( 0xc00048, specConfig.specVideoAspectRatio );

    SystemBus_Write( 0xc00046, specConfig.specDacMode );
    SystemBus_Write( 0xc00047, specConfig.specAyMode );

    floppy_set_fast_mode( specConfig.specBdiMode );

    if( fpgaConfigVersionPrev != 0 && romConfigPrev != (dword) specConfig.specRom )
    {
        Spectrum_InitRom();
        romConfigPrev = specConfig.specRom;
    }
}

void Spectrum_UpdateDisks()
{
    for( int i = 0; i < 4; i++ )
    {
        fdc_open_image( i, specConfig.specImages[ i ].name );
        floppy_disk_wp( i, &specConfig.specImages[i].readOnly );
    }
}

void SD_Init()
{
    if( ( disk_status(0) & STA_NODISK ) == 0 && ( disk_status(0) & STA_NOINIT ) != 0 )
    {
        disk_initialize( 0 );

        if( ( disk_status(0) & STA_NOINIT ) != 0 )
        {
            __TRACE( "SD card init error :(\n" );
        }
        else
        {
            __TRACE( "SD card init OK..\n" );
            //MassStorage_UpdateCharacteristics();

            static FATFS fatfs;
            f_mount( 0, &fatfs );

            FPGA_Config();
            RestreConfig();
            Spectrum_UpdateConfig();
            Spectrum_UpdateDisks();
        }
    }
}

dword tickCounter = 0;

int kbdInited = 0;
const int KBD_OK = 2;
int kbdResetTimer = 0;

int mouseInited = 0;
const int MOUSE_OK = 4;
int mouseResetTimer = 0;

CFifo keyboard( 0x10 );
CFifo joystick( 0x10 );

dword Timer_GetTickCounter()        // 100 Hz
{
    return tickCounter;
}

byte keybordSend[ 4 ];
int keybordSendPos = 0;
int keybordSendSize = 0;

void Mouse_Reset()
{
    word mouseCode = SystemBus_Read( 0xc00033 );
    while( ( mouseCode & 0x8000 ) != 0 ) mouseCode = SystemBus_Read( 0xc00033 );

    mouseInited = 0;
    SystemBus_Write( 0xc00033, 0xff );

    mouseResetTimer = 0;
}

void Keyboard_Reset()
{
    word keyCode = SystemBus_Read( 0xc00032 );
    while( ( keyCode & 0x8000 ) != 0 )
    {
        //__TRACE( "keyCode_Skip : 0x%.2x, 0x%.2x\n", keyCode & 0xff, ( keyCode >> 8 ) & 0x0f );
        keyCode = SystemBus_Read( 0xc00032 );
    }

    kbdInited = 0;
    SystemBus_Write( 0xc00032, 0xff );
    //__TRACE( "Keyboard_Send : 0x%.2x\n", 0xff );

    kbdResetTimer = 0;
    keybordSendPos = 0;
    keybordSendSize = 0;

    keyboard.Clean();
    joystick.Clean();
}
void Keyboard_Send( const byte *data, int size )
{
    if( size <= 0 || size > (int) sizeof(keybordSend) ) return;
    if( kbdInited != KBD_OK ) return;

    memcpy( keybordSend, data, size );
    keybordSendSize = size;
    keybordSendPos = 0;

    SystemBus_Write( 0xc00032, keybordSend[ keybordSendPos ] );
    //__TRACE( "Keyboard_Send : 0x%.2x\n", keybordSend[ keybordSendPos ] );
}

void Keyboard_SendNext( byte code )
{
    if( code == 0xfa ) keybordSendPos++;
    if( keybordSendPos >= keybordSendSize ) return;

    SystemBus_Write( 0xc00032, keybordSend[ keybordSendPos ] );
    //__TRACE( "Keyboard_SendNext : 0x%.2x\n", keybordSend[ keybordSendPos ] );
}

void Timer_Routine()
{
    if( fpgaConfigVersionPrev == 0 )
    {
        kbdInited = 0;
        mouseInited = 0;
    }

    static byte prev_status = STA_NODISK;

    byte status = ( disk_status( 0 ) & STA_NODISK );
    if( status != prev_status )
    {
        if( status == 0 )
        {
            DelayMs( 10 );
            SD_Init();
        }

        prev_status = status;
    }

    //if( timer_flag_100Hz )        // 100Hz
    {
        if( fpgaConfigVersionPrev != 0 )
        {
            static dword joyCodePrev = 0;
            dword joyCode = SystemBus_Read( 0xc00030 ) | ( SystemBus_Read( 0xc00031 ) << 16 );

            if( joyCode != joyCodePrev )
            {
                //if( joyCode != 0 )
                //{
                    //char str[ 0x20 ];
                    //sniprintf( str, sizeof(str), "joyCode - 0x%.8x\n", joyCode );
                    //UART0_WriteText( str );
                //}

                dword bit = 1;
                dword joyCodeDiff = joyCode ^ joyCodePrev;

                for( byte i = 0; i < 32; i++, bit <<= 1 )
                {
                    if( ( joyCodeDiff & bit ) != 0 )
                    {
                        if( ( joyCode & bit ) != 0 ) joystick.WriteByte( i );
                        else joystick.WriteByte( 0x80 | i );
                    }
                }

                joyCodePrev = joyCode;
            }

            //--------------------------------------------------------------------------

            word keyCode = SystemBus_Read( 0xc00032 );
            while( ( keyCode & 0x8000 ) != 0 )
            {
                //__TRACE( "keyCode : 0x%.2x, 0x%.2x\n", keyCode & 0xff, ( keyCode >> 8 ) & 0x0f );

                keyboard.WriteByte( (byte) keyCode );
                keyCode = SystemBus_Read( 0xc00032 );
            }

            //--------------------------------------------------------------------------

            word mouseCode = SystemBus_Read( 0xc00033 );
            while( ( mouseCode & 0x8000 ) != 0 )
            {
                if( mouseCode & 0x4000 )
                {
                    //__TRACE( "Mouse buffer overflow\n" );
                    Mouse_Reset();
                    break;
                }

                mouseCode &= 0xff;
                //__TRACE( "mouse code - 0x%.2x\n", mouseCode );

                static byte mouseBuff[4];
                static byte mouseBuffPos = 0;

                if( mouseInited == 0 && mouseCode == 0xfa ) mouseInited++;
                else if( mouseInited == 1 && mouseCode == 0xaa ) mouseInited++;
                else if( mouseInited == 2 && mouseCode == 0x00 )
                {
                    mouseInited++;
                    SystemBus_Write( 0xc00033, 0xf4 );
                }
                else if( mouseInited == 3 && mouseCode == 0xfa )
                {
                    mouseInited++;
                    mouseBuffPos = 0;

                    __TRACE( "PS/2 mouse init OK..\n" );

                    SystemBus_Write( 0xc0001e, 0x5555 );
                    SystemBus_Write( 0xc0001f, 0xff );
                }
                else if( mouseInited == MOUSE_OK )
                {
                    static dword x = 0x55555555;
                    static dword y = 0x55555555;

                    mouseBuff[ mouseBuffPos++ ] = mouseCode;

                    if( mouseBuffPos >= 3 )
                    {
                        int dx, dy;

                        //char str[ 0x20 ];
                        //sniprintf( str, sizeof(str), "mouse 0x%.2x.0x%.2x.0x%.2x, ", mouseBuff[0], mouseBuff[1], mouseBuff[2] );
                        //UART0_WriteText( str );

                        if( ( mouseBuff[0] & ( 1 << 6 ) ) == 0 )
                        {
                            if( ( mouseBuff[0] & ( 1 << 4 ) ) == 0 ) dx = mouseBuff[ 1 ];
                            else dx = mouseBuff[ 1 ] - 256;
                        }
                        else
                        {
                            if( ( mouseBuff[0] & ( 1 << 4 ) ) == 0 ) dx = 256;
                            else dx = -256;
                        }

                        if( ( mouseBuff[0] & ( 1 << 7 ) ) == 0 )
                        {
                            if( ( mouseBuff[0] & ( 1 << 5 ) ) == 0 ) dy = mouseBuff[ 2 ];
                            else dy = mouseBuff[ 2 ] - 256;
                        }
                        else
                        {
                            if( ( mouseBuff[0] & ( 1 << 5 ) ) == 0 ) dy = 256;
                            else dy = -256;
                        }

                        x += dx;
                        y += dy;

                        byte buttons = 0xff;
                        if( !specConfig.specMouseSwap )
                        {
                            if( ( mouseBuff[0] & ( 1 << 0 ) ) != 0 ) buttons ^= ( 1 << 1 );
                            if( ( mouseBuff[0] & ( 1 << 1 ) ) != 0 ) buttons ^= ( 1 << 0 );
                        }
                        else
                        {
                            if( ( mouseBuff[0] & ( 1 << 0 ) ) != 0 ) buttons ^= ( 1 << 0 );
                            if( ( mouseBuff[0] & ( 1 << 1 ) ) != 0 ) buttons ^= ( 1 << 1 );
                        }

                        if( ( mouseBuff[0] & ( 1 << 2 ) ) != 0 ) buttons ^= ( 1 << 2 );

                        byte sx = x >> (byte)( ( 6 - specConfig.specMouseSensitivity ) );
                        byte sy = y >> (byte)( ( 6 - specConfig.specMouseSensitivity ) );

                        SystemBus_Write( 0xc0001e, sx | ( sy << 8 ) );
                        SystemBus_Write( 0xc0001f, buttons );

                        mouseBuffPos = 0;

                        //char str[ 0x20 ];
                        //sniprintf( str, sizeof(str), "x - 0x%.2x, y - 0x%.2x, b - 0x%.2x\n", sx, sy, buttons );
                        //UART0_WriteText( str );
                    }
                }

                mouseCode = SystemBus_Read( 0xc00033 );
            }
        }
    }

    if( timer_flag_100Hz )        // 100Hz
    {
        if( fpgaConfigVersionPrev != 0 )
        {
            if( kbdInited != KBD_OK )
            {
                kbdResetTimer++;
                if( kbdResetTimer > 200 ) Keyboard_Reset();
            }

            if( mouseInited != MOUSE_OK )
            {
                mouseResetTimer++;
                if( mouseResetTimer > 200 ) Mouse_Reset();
            }

            //-------------------------------------------------
            // reset FPGA

            /*
            static bool resetFPGA = false;

            if( GPIO_ReadBit( GPIO0, GPIO_Pin_0 ) == Bit_RESET )
            {
                resetFPGA = true;
            }
            else if( resetFPGA )
            {
                __TRACE( "Reset FPGA..\n" );

                GPIO_WriteBit( GPIO1, GPIO_Pin_13, Bit_RESET );
                DelayMs( 1 );
                GPIO_WriteBit( GPIO1, GPIO_Pin_13, Bit_SET );

                resetFPGA = false;
            }*/

            //-------------------------------------------------


            static byte leds_prev = 0;
            byte leds = floppy_leds();

            if( leds != leds_prev && kbdInited == KBD_OK )
            {
                byte data[2] = { 0xed, leds };
                Keyboard_Send( data, 2 );
                leds_prev = leds;
            }
        }

        portENTER_CRITICAL();
        timer_flag_100Hz--;
        portEXIT_CRITICAL();

        tickCounter++;
        WDT_Kick();
    }

    if( timer_flag_1Hz )        // 1Hz
    {
        //char str[ 0x20 ];
        //sniprintf( str, sizeof(str), "disk status - %x\n", disk_status(0) );
        //UART0_WriteText( str );

        if( ( disk_status(0) & STA_NODISK ) == 0 && ( disk_status(0) & STA_NOINIT ) != 0 ) SD_Init();

        if( fpgaConfigVersionPrev == 0 && ( disk_status(0) & STA_NOINIT ) == 0 )
        {
            FPGA_Config();
            RestreConfig();
            Spectrum_UpdateConfig();
            Spectrum_UpdateDisks();
        }

        RTC_Update();

        portENTER_CRITICAL();
        timer_flag_1Hz--;
        portEXIT_CRITICAL();
    }
}

void BDI_ResetWrite()
{
    SystemBus_Write( 0xc00060, 0x8000 );
}

void BDI_Write( byte data )
{
    SystemBus_Write( 0xc00060, data );
}

void BDI_ResetRead( word counter )
{
    SystemBus_Write( 0xc00061, 0x8000 | counter );
}

bool BDI_Read( byte *data )
{
    word result = SystemBus_Read( 0xc00061 );
    *data = (byte) result;

    return ( result & 0x8000 ) != 0;
}

void BDI_Routine()
{
    if( fpgaConfigVersionPrev == 0 ) return;
    int ioCounter = 0x20;

    word trdosStatus = SystemBus_Read( 0xc00019 );

    while( ( trdosStatus & 1 ) != 0 )
    {
        bool trdosWr = ( trdosStatus & 0x02 ) != 0;
        byte trdosAddr = SystemBus_Read( 0xc0001a );

        static int counter = 0;

        if( trdosWr )
        {
            byte trdosData = SystemBus_Read( 0xc0001b );
            fdc_write( trdosAddr, trdosData );

            if( LOG_BDI_PORTS )
            {
                if( trdosAddr == 0x7f )
                {
                    if( counter == 0 ) __TRACE( "Data write : " );

                    __TRACE( "%.2x.", trdosData );

                    counter++;
                    if( counter == 16 )
                    {
                        counter = 0;
                        __TRACE( "\n" );
                    }
                }
                else //if( trdosAddr != 0xff )
                {
                    if( counter != 0 )
                    {
                        counter = 0;
                        __TRACE( "\n" );
                    }

                    word specPc = SystemBus_Read( 0xc00001 );
                    __TRACE( "0x%.4x WR : 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData );
                }
            }
        }
        else
        {
            byte trdosData = fdc_read( trdosAddr );
            SystemBus_Write( 0xc0001b, trdosData );

            if( LOG_BDI_PORTS )
            {
                if( trdosAddr == 0x7f )
                {
                    if( counter == 0 ) __TRACE( "Data read : " );

                    __TRACE( "%.2x.", trdosData );

                    counter++;
                    if( counter == 16 )
                    {
                        counter = 0;
                        __TRACE( "\n" );
                    }
                }
                else if( trdosAddr != 0xff )
                {
                    if( counter != 0 )
                    {
                        counter = 0;
                        __TRACE( "\n" );
                    }

                    word specPc = SystemBus_Read( 0xc00001 );
                    __TRACE( "0x%.4x RD : 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData );
                }
            }
        }

        SystemBus_Write( 0xc0001d, fdc_read( 0xff ) );
        SystemBus_Write( 0xc00019, 0 );

        fdc_dispatch();

        SystemBus_Write( 0xc0001d, fdc_read( 0xff ) );
        //SystemBus_Write( 0xc00019, 0 );

        if( --ioCounter == 0 ) break;

        trdosStatus = SystemBus_Read( 0xc00019 );
        if( ( trdosStatus & 1 ) == 0 ) break;
    }

    fdc_dispatch();
    SystemBus_Write( 0xc0001d, fdc_read( 0xff ) );
}

static word pressedKeys[4] = { KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE };

word prevKey = KEY_NONE;
dword keyRepeatTime = 0;

word keyFlags = 0;

word ReadKeyFlags()
{
    return keyFlags;
}

bool ReadKey( word &_keyCode, word &_keyFlags )
{
    static byte keyPrefix = 0x00;
    static bool keyRelease = false;

    _keyCode = KEY_NONE;
    _keyFlags = keyFlags;

    while( _keyCode == KEY_NONE && keyboard.GetCntr() > 0 )
    {
        word keyCode = keyboard.ReadByte();

        //__TRACE( "keyCode - 0x%.2x\n", keyCode );

        if( kbdInited == 0 && keyCode == 0xfa )
        {
            kbdInited = 1;
        }
        else if( kbdInited == 1 && keyCode == 0xaa )
        {
            kbdInited = KBD_OK;
            keyFlags = 0;
            keyPrefix = 0x00;
            keyRelease = false;

            __TRACE( "PS/2 keyboard init OK..\n" );
        }
        else if( kbdInited == KBD_OK )
        {
            if( keyCode == 0xfa || keyCode == 0xfe ) Keyboard_SendNext( keyCode );
            else if ( keyCode == 0xf0 ) keyRelease = true;
            else if ( keyCode == 0xe0 || keyCode == 0xe1 )
            {
                keyPrefix = keyCode;
            }
            else
            {
                keyCode |= ( (word) keyPrefix << 8 );
                keyPrefix = 0x00;

                if( keyCode == KEY_PRNTSCR_SKIP || keyCode == KEY_PAUSE_SKIP )
                {
                    keyCode = KEY_NONE;
                    keyRelease = false;
                    continue;
                }

                //__TRACE( "# keyCode - 0x%.4x, 0x%.4x\n", keyCode, (word) keyRelease );

                int i;
                const int pressedKeysCount = sizeof( pressedKeys ) / sizeof( word );
                bool result = false;

                for( i = 0; i < pressedKeysCount; i++ )
                {
                    if( pressedKeys[ i ] == keyCode )
                    {
                        if( keyRelease )
                        {
                            pressedKeys[ i ] = KEY_NONE;
                            result = true;
                        }

                        break;
                    }
                }

                if( i >= pressedKeysCount && !keyRelease )
                {
                    for( i = 0; i < pressedKeysCount; i++ )
                    {
                        if( pressedKeys[ i ] == KEY_NONE )
                        {
                            pressedKeys[ i ] = keyCode;
                            result = true;
                            break;
                        }
                    }
                }

                if( !result )
                {
                    keyCode = KEY_NONE;
                    keyRelease = false;
                    continue;
                }

                word tempFlag = 0;
                if( keyCode == KEY_LSHIFT ) tempFlag = fKeyShiftLeft;
                else if( keyCode == KEY_RSHIFT ) tempFlag = fKeyShiftRight;
                else if( keyCode == KEY_LCTRL ) tempFlag = fKeyCtrlLeft;
                else if( keyCode == KEY_RCTRL ) tempFlag = fKeyCtrlRight;
                else if( keyCode == KEY_LALT ) tempFlag = fKeyAltLeft;
                else if( keyCode == KEY_RALT ) tempFlag = fKeyAltRight;

                if( tempFlag != 0 )
                {
                    if( !keyRelease ) keyFlags |= tempFlag;
                    else keyFlags &= ~tempFlag;
                }

                if( !keyRelease )
                {
                    if( keyCode == KEY_CAPSLOCK ) keyFlags = keyFlags ^ fKeyCaps;
                    else if( keyCode == KEY_LSHIFT && ( keyFlags & fKeyCtrlLeft ) != 0 ) keyFlags = keyFlags ^ fKeyRus;
                    else if( keyCode == KEY_LCTRL && ( keyFlags & fKeyShiftLeft ) != 0 ) keyFlags = keyFlags ^ fKeyRus;
                }

                _keyCode = keyCode;
                _keyFlags = keyFlags;

                if( keyRelease ) _keyFlags |= fKeyRelease;
                keyRelease = false;
            }
        }
    }

    //----------------------------------------------------------------

    while( _keyCode == KEY_NONE && joystick.GetCntr() > 0 )
    {
        byte joyCode = joystick.ReadByte();

        //char str[ 0x20 ];
        //sniprintf( str, sizeof(str), "joyCode - 0x%.2x\n", joyCode );
        //UART0_WriteText( str );

        word keyCode = KEY_NONE;
        word tempFlag = 0;

        if( ( joyCode & 0x80 )  ) tempFlag = fKeyRelease;
        joyCode &= 0x7f;

        if( joyCode < 16 ) tempFlag |= fKeyJoy1;
        else tempFlag |= fKeyJoy2;
        joyCode &= 0x0f;

        switch( joyCode )
        {
            case 0: keyCode = KEY_UP; break;
            case 1: keyCode = KEY_DOWN; break;
            case 2: keyCode = KEY_LEFT; break;
            case 3: keyCode = KEY_RIGHT; break;

            case 4: keyCode = KEY_LCTRL; break;
            case 5: keyCode = KEY_LCTRL; break;
        }

        if( keyCode != KEY_NONE )
        {
            _keyCode = keyCode;
            _keyFlags |= tempFlag;
        }
    }

    //-------------------------------------------------------------------------

    if( _keyCode != KEY_NONE )
    {
        //__TRACE( "# keyCode - 0x%.4x, 0x%.4x\n", _keyCode, _keyFlags );

        DecodeKeySpec( _keyCode, _keyFlags );

        if( ( _keyFlags & fKeyRelease ) == 0 )
        {
            prevKey = _keyCode;
            keyRepeatTime = Timer_GetTickCounter() + 50;
        }
        else
        {
            prevKey = KEY_NONE;
        }
    }

    return _keyCode != KEY_NONE;
}

word ReadKeySimple( bool norepeat )
{
    Timer_Routine();

    word keyCode;
    word keyFlags;

    while( ReadKey( keyCode, keyFlags ) )
    {
        if( ( keyFlags & fKeyRelease ) == 0 ) return keyCode;
    }

    if( norepeat ) return KEY_NONE;

    if( prevKey != KEY_NONE && (int) ( Timer_GetTickCounter() - keyRepeatTime ) >= 0 )
    {
        keyRepeatTime = Timer_GetTickCounter() + 5;
        return prevKey;
    }

    return KEY_NONE;
}

void Keyboard_Routine()
{
    word keyCode, keyFlags;

    while( ReadKey( keyCode, keyFlags ) )
    {
        DecodeKey( keyCode, keyFlags );
        //__TRACE( "% keyCode - 0x%.4x, 0x%.4x\n", keyCode, keyFlags );
    }
}

//extern "C" RESULT PowerOn(void);
//extern "C" RESULT PowerOff();

const dword STACK_FILL = 0x37ACF177;
extern char *current_heap_end;

void InitStack()
{
    volatile dword *stackPos;
    {
        volatile dword stackStart;

        stackPos = &stackStart;
        while( (void*) stackPos >= (void*) current_heap_end )
        {
            *stackPos = STACK_FILL;
            stackPos--;
        }
    }
}

void TestStack()
{
    dword stackCurrent;
    dword stackMin;
    volatile dword *stackPos;

    {
        volatile dword stackStart;

        stackCurrent = (dword) &stackStart;
        stackMin = (dword) &stackStart;

        stackPos = &stackStart;
        while( (void*) stackPos >= (void*) current_heap_end )
        {
            if( *stackPos != STACK_FILL ) stackMin = (dword) stackPos;
            stackPos--;
        }
    }

    __TRACE( "stack current - 0x%.8x\n", stackCurrent );
    __TRACE( "stack max - 0x%.8x\n", stackMin );
    __TRACE( "heap max - 0x%.8x\n", (dword) current_heap_end );
}

const int MAX_CMD_SIZE = 0x20;
static char cmd[ MAX_CMD_SIZE + 1 ];
static int cmdSize = 0;
static bool traceNewLine = false;

void Serial_Routine()
{
    while( uart0.GetRxCntr() > 0  )
    {
        char temp = uart0.ReadByte();

        if( temp == 0x0a )
        {
            cmd[ cmdSize ] = 0;
            cmdSize = 0;

            if( strcmp( cmd, "test stack" ) == 0 ) TestStack();
            else if( strcmp( cmd, "log bdi" ) == 0 ) LOG_BDI_PORTS = true;
            else if( strcmp( cmd, "update rom" ) == 0 )
            {
                Spectrum_InitRom();
            }
            else if( strcmp( cmd, "log wait off" ) == 0 ) LOG_WAIT = false;
            else if( strcmp( cmd, "log wait" ) == 0 ) LOG_WAIT = true;
            //else if( strcmp( cmd, "test heap" ) == 0 ) TestHeap();
            else if( strcmp( cmd, "reset" ) == 0 ) while( true );
            else
            {
                __TRACE( "cmd: %s\n", cmd );
               LOG_BDI_PORTS = false;
            }
        }
        else if( temp == 0x08 )
        {
            if( cmdSize > 0 )
            {
                cmdSize--;

                const char delStr[2] = { 0x20, 0x08 };
                uart0.WriteFile( (byte*) delStr, 2 );
            }
        }
        else if( temp != 0x0d && cmdSize < MAX_CMD_SIZE )
        {
            cmd[ cmdSize++ ] = temp;
        }
    }
}

void __TRACE( const char *str, ... )
{
    static char fullStr[ 0x80 ];

    va_list ap;
    va_start( ap, str );
    vsniprintf( fullStr, sizeof( fullStr ), str, ap );
    va_end(ap);

    if( traceNewLine )
    {
        Serial_Routine();
        const char delChar = 0x08;
        for( int i = 0; i <= cmdSize; i++ ) uart0.WriteFile( (byte*) &delChar, 1 );
    }

    char lastChar = 0;
    char *strPos = fullStr;

    while( *strPos != 0 )
    {
        lastChar = *strPos++;

        if( lastChar == '\n' ) uart0.WriteFile( (byte*) "\r\n", 2 );
        else uart0.WriteFile( (byte*) &lastChar, 1 );

        WDT_Kick();
    }

    traceNewLine = ( lastChar == '\n' );

    if( traceNewLine )
    {
        UART0_WriteText( ">" );

        Serial_Routine();
        if( cmdSize > 0 ) uart0.WriteFile( (byte*) cmd, cmdSize );
    }
}

int main()
{
	bool pllStatusOK = MRCC_Config();
	TIM0_Config();
    I2C_Config();
    SPI_Config();

    WDT_Config();

	uart0.Init( GPIO0, GPIO_Pin_11, GPIO0, GPIO_Pin_10, 2 );

    GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 1 << 14;
	GPIO_Init( GPIO2, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 0xffff0000;
	GPIO_Init( GPIO0, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 0x07ff;
	GPIO_Init( GPIO2, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 1 << 13;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = 0x0800;
	GPIO_Init( GPIO2, &GPIO_InitStructure );

	InitStack();

    __TRACE( "\n" );
    __TRACE( "Speccy2010, ver %d.%d, rev %d !\n", VER_MAJOR, VER_MINIR, REV );

    if( !pllStatusOK ) __TRACE( "PLL initialization error !\n" );
    DelayMs( 100 );

    fdc_init();
    RTC_Init();

    while( true )
    {
        Timer_Routine();
        Serial_Routine();
        Keyboard_Routine();
        Tape_Routine();
        BDI_Routine();
    }
}

volatile dword delayTimer = 0;
volatile dword tapeTimer = 0;

volatile bool bdiTimerFlag = true;
volatile dword bdiTimer = 0;

void TIM0_UP_IRQHandler()       //40kHz
{
	static dword tick = 0;

	if( delayTimer >= 25 ) delayTimer -= 25;
	else delayTimer = 0;

	if( bdiTimerFlag ) bdiTimer += 25;

	tick++;
	if( tick >= 400 )  // 100Hz
	{
		tick = 0;

	    static dword tick2 = 0;

	    tick2++;
	    if( tick2 == 100 ) // 1Hz
	    {
            tick2 = 0;
            timer_flag_1Hz++;
	    }

	    disk_timerproc();
	    timer_flag_100Hz++;
	}

	//---------------------------------------------------

	TIM_ClearITPendingBit( TIM0, TIM_IT_Update );
}

void DelayUs( dword delay )
{
	word prevValue = TIM0->CNT;
    delay++;

	while( delay )
	{
		if( TIM0->CNT != prevValue )
		{
            prevValue = TIM0->CNT;
            delay--;
		}
	}

	WDT_Kick();
}

void DelayMs( dword delay )
{
	delayTimer = delay * 1000;
	while( delayTimer ) WDT_Kick();
}

dword get_ticks()
{
    return bdiTimer;
}

void BDI_StopTimer()
{
    bdiTimerFlag = false;
}

void BDI_StartTimer()
{
    bdiTimerFlag = true;
}

dword prevEIC_ICR = 0;
dword nesting = 0;

void portENTER_CRITICAL()
{
    if( nesting == 0 )
    {
        dword temp = EIC->ICR & 3;
        EIC->ICR &= ~3;
        prevEIC_ICR = temp;
    }

    nesting++;
}

void portEXIT_CRITICAL()
{
    if( nesting )
    {
        nesting--;

        if( nesting == 0 )
        {
            EIC->ICR |= prevEIC_ICR;
        }
    }
}

const char *GetFatErrorMsg( int id )
{
    const char *fatErrorMsg[] = {
       	"FR_OK",
        "FR_DISK_ERR",
        "FR_INT_ERR",
        "FR_NOT_READY",
        "FR_NO_FILE",
        "FR_NO_PATH",
        "FR_INVALID_NAME",
        "FR_DENIED",
        "FR_EXIST",
        "FR_INVALID_OBJECT",
        "FR_WRITE_PROTECTED",
        "FR_INVALID_DRIVE",
        "FR_NOT_ENABLED",
        "FR_NO_FILESYSTEM",
        "FR_MKFS_ABORTED",
        "FR_TIMEOUT",
        "FR_LOCKED",
        "FR_NOT_ENOUGH_CORE",
    };

    return fatErrorMsg[ id ];
}


//---------------------------------------------------------------------------------------------

struct CMallocRecord
{
    dword type;
    dword address;
    dword size;
};

/*
CMallocRecord mallocRecords[ 0x100 ];
int mallocRecordsNumber = 0;

void AddMallocRecord( dword type, dword address, dword size )
{
    if( mallocRecordsNumber < 0x80 )
    {
        mallocRecords[mallocRecordsNumber].type = type;
        mallocRecords[mallocRecordsNumber].address = address;
        mallocRecords[mallocRecordsNumber].size = size;
        mallocRecordsNumber++;
    }
}

void TestHeap()
{
    for( int i = 0; i < mallocRecordsNumber; i++ )
    {
        if( mallocRecords[i].type == 0 ) __TRACE( "[%d] sbrk - 0x%.8x, 0x%.8x\n", i, mallocRecords[i].address, mallocRecords[i].size );
        else __TRACE( "[%d] new - 0x%.8x, 0x%.8x\n", i, mallocRecords[i].address, mallocRecords[i].size );
    }
}*/

void *operator new( std::size_t size )
{
    void *ptr = malloc( size );
    //AddMallocRecord( 1, (dword) ptr, size );
    return ptr;
}

void *operator new[](std::size_t size)
{
    void *ptr = malloc( size );
    //AddMallocRecord( 1, (dword) ptr, size );
    return ptr;
}

void operator delete( void *ptr )
{
    free( ptr );
}

void operator delete[]( void *ptr )
{
    free( ptr );
}

extern "C" void __cxa_guard_acquire()
{
}

extern "C" void __cxa_guard_release()
{
}

