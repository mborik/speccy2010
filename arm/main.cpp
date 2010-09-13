#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>
#include <stdarg.h>

#include "system.h"
#include "uarts.h"
#include "ps2port.h"
#include "fifo.h"

#include "specTape.h"
#include "specKeyboard.h"
#include "specShell.h"
#include "specConfig.h"
#include "specBetadisk.h"

#include "ramFile.h"

const int VER_MAJOR = 1;
const int VER_MINIR = 0;
const int REV = 35;

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
	//**USB MRCC_PeripheralClockConfig( MRCC_Peripheral_USB, ENABLE );

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

void USB_Config()
{
	MRCC_CKUSBConfig (MRCC_CKUSB_Internal);
    CFG_USBFilterConfig(CFG_USBFilter_Enable);

	EIC_IRQInitTypeDef EIC_IRQInitStructure;

	EIC_IRQInitStructure.EIC_IRQChannel = USB_HP_IRQChannel;
	EIC_IRQInitStructure.EIC_IRQChannelPriority = 3;
	EIC_IRQInitStructure.EIC_IRQChannelCmd = ENABLE;
	EIC_IRQInit(&EIC_IRQInitStructure);

	EIC_IRQInitStructure.EIC_IRQChannel = USB_LP_IRQChannel;
	EIC_IRQInitStructure.EIC_IRQChannelPriority = 4;
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
tm currentTime;

void GetTime( tm *time )
{
    byte data[ 7 ];
    byte address;

    address = 0;

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_TRANSMITTER);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED));

    I2C_Cmd(ENABLE);
    I2C_SendData( address );
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(RTC_ADDRESS, I2C_MODE_RECEIVER);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECTED));

    I2C_Cmd(ENABLE);

    while( address < sizeof( data ) )
    {
        while( !I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_RECEIVED ) );

        if( address == sizeof( data ) - 2 ) I2C_AcknowledgeConfig(DISABLE);
        else if( address == sizeof( data ) - 1 ) I2C_GenerateSTOP(ENABLE);

        data[ address++ ] = I2C_ReceiveData();
    }

    I2C_AcknowledgeConfig(ENABLE);

    currentTime.tm_sec = ( data[0] >> 4 ) * 10 + ( data[0] & 0x0f );
    currentTime.tm_min = ( data[1] >> 4 ) * 10 + ( data[1] & 0x0f );
    currentTime.tm_hour = ( data[2] >> 4 ) * 10 + ( data[2] & 0x0f );

    currentTime.tm_mday = ( data[4] >> 4 ) * 10 + ( data[4] & 0x0f );
    currentTime.tm_mon = ( data[5] >> 4 ) * 10 + ( data[5] & 0x0f ) - 1;
    currentTime.tm_year = 100 + ( data[6] >> 4 ) * 10 + ( data[6] & 0x0f );

    if( time ) memcpy( time, &currentTime, sizeof( tm ) );
}

void SetTime( tm *newTime )
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

    time.tm_mon += 1;
    time.tm_year %= 100;

    byte data[7];
    data[0] = ( ( time.tm_sec / 10 ) << 4 ) | ( time.tm_sec % 10 );
    data[1] = ( ( time.tm_min / 10 ) << 4 ) | ( time.tm_min % 10 );
    data[2] = ( ( time.tm_hour / 10 ) << 4 ) | ( time.tm_hour % 10 );
    data[3] = 0;
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

    time.tm_year += 100;
    time.tm_mon -= 1;
    memcpy( &currentTime, &time, sizeof( tm ) );
}

DWORD get_fattime()
{
    GetTime( NULL );

	return	( ( currentTime.tm_year - 80 ) << 25 ) | ( ( currentTime.tm_mon + 1 ) << 21 ) |
                ( currentTime.tm_mday << 16 ) | ( currentTime.tm_hour << 11 ) |
                ( currentTime.tm_min << 5 ) | ( currentTime.tm_sec >> 1 );
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
        UART0_WriteText( "Cannot init SD card...\n" );
        return;
    }

    FILINFO fpgaConfigInfo;
    char lfn[ _MAX_LFN * 2 + 1 ];
    fpgaConfigInfo.lfname = lfn;
    fpgaConfigInfo.lfsize = sizeof( lfn );

    if( f_stat( "speccy2010.rbf", &fpgaConfigInfo ) != FR_OK )
    {
        UART0_WriteText( "Cannot open speccy2010.rbf...\n" );
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
        UART0_WriteText( "The version of speccy2010.rbf is match...\n" );
        return;
    }

    FIL fpgaConfig;
    if( f_open( &fpgaConfig, "speccy2010.rbf", FA_READ ) != FR_OK )
    {
        UART0_WriteText( "Cannot open speccy2010.rbf...\n" );
        return;
    }

    //--------------------------------------------------------------------------

    UART0_WriteText( "FPGA configuration started...\n" );

    GPIO_InitTypeDef  GPIO_InitStructure;
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
        UART0_WriteText( "FPGA configuration status OK...\n" );

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
                UART0_WriteText( "." );
            }
        }
    }

    f_close( &fpgaConfig );
    UART0_WriteText( "FPGA configuration conf done...\n" );

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_Init( GPIO1, &GPIO_InitStructure );

    i = 10;
    while( i-- > 0 && !CONFIG_DONE() ) DelayMs( 10 );

	if( CONFIG_DONE() )
	{
	    UART0_WriteText( "FPGA configuration finished...\n" );
	    fpgaConfigVersionPrev = fpgaConfigVersion;
	}
	else
	{
	    UART0_WriteText( "FPGA configuration failed...\n" );
	    fpgaConfigVersionPrev = 0;
	}

    GPIO1->PD &= ~( 1 << 13 );              // FPGA RESET LOW
    DelayMs( 1 );
    GPIO1->PD |= ( 1 << 13 );               // FPGA RESET HIGH

    DelayMs( 1 );
    if( SystemBus_TestConfiguration() == false )
    {
        UART0_WriteText( "Wrong FPGA configuration...\n" );
        fpgaConfigVersionPrev = 0;
    }

    romConfigPrev = -1;
    FPGA_TestClock();

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
        UART0_WriteText( "Cannot open rom image " );
        UART0_WriteText( romFileName );
        UART0_WriteText( "\n" );
        return false;
    }

	bool result = true;

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

        SystemBus_Write( addr, data );
        word temp = SystemBus_Read( addr );

        if( temp != data ) break;

        addr++;
        if( ( addr & 0x0fff ) == 0 ) WDT_Kick();
	}

    if( pos < romImage.fsize )
    {
        UART0_WriteText( "Memory write error !\n" );
        result = false;
    }

    f_close( &romImage );
    CPU_Start();

    return result;
}

void Spectrum_InitRom()
{
    UART0_WriteText( "ROM configuration started...\n" );

    specConfig.specUseBank0 = Spectrum_LoadRomPage( 0, "roms/system.rom" );
    Spectrum_LoadRomPage( 1, "roms/trdos.rom" );

    if( specConfig.specRom == SpecRom_Classic48 ) Spectrum_LoadRomPage( 3, "roms/48.rom" );
    else Spectrum_LoadRomPage( 2, "roms/pentagon.rom" );

    UART0_WriteText( "ROM configuration finished...\n" );

    SystemBus_Write( 0xc00021, 0x0000 );
    SystemBus_Write( 0xc00022, 0x0000 );

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

    if( fpgaConfigVersionPrev != 0 && romConfigPrev != (dword) specConfig.specRom )
    {
        Spectrum_InitRom();
        romConfigPrev = specConfig.specRom;
    }
}

void SD_Init()
{
    if( ( disk_status(0) & STA_NODISK ) == 0 && ( disk_status(0) & STA_NOINIT ) != 0 )
    {
        disk_initialize( 0 );

        if( ( disk_status(0) & STA_NOINIT ) != 0 )
        {
            UART0_WriteText( "SD card init error :(\n" );
        }
        else
        {
            UART0_WriteText( "SD card init OK..\n" );
            //MassStorage_UpdateCharacteristics();

            static FATFS fatfs;
            f_mount( 0, &fatfs );

            FPGA_Config();
            RestreConfig();
            Spectrum_UpdateConfig();
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

dword Timer_GetTickCounter()
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
    while( ( keyCode & 0x8000 ) != 0 ) keyCode = SystemBus_Read( 0xc00032 );

    kbdInited = 0;
    SystemBus_Write( 0xc00032, 0xff );

    kbdResetTimer = 0;
    keybordSendPos = 0;
    keybordSendSize = 0;
}
void Keyboard_Send( const byte *data, int size )
{
    if( size <= 0 || size > (int) sizeof(keybordSend) ) return;

    memcpy( keybordSend, data, size );
    keybordSendSize = size;
    keybordSendPos = 0;

    SystemBus_Write( 0xc00032, keybordSend[ keybordSendPos ] );
    __TRACE( "Keyboard_Send : 0x%.2x\n", keybordSend[ keybordSendPos ] );
}

void Keyboard_SendNext( byte code )
{
    __TRACE( "Keyboard_Received : 0x%.2x\n", code );

    if( code == 0xfa ) keybordSendPos++;
    if( keybordSendPos >= keybordSendSize ) return;

    SystemBus_Write( 0xc00032, keybordSend[ keybordSendPos ] );
    __TRACE( "Keyboard_SendNext : 0x%.2x\n", keybordSend[ keybordSendPos ] );
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
                }
                else
                {
                    static dword x = 0x12345678;
                    static dword y = 0x12345678;

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
                        if( ( mouseBuff[0] & ( 1 << 0 ) ) != 0 ) buttons ^= ( 1 << 1 );
                        if( ( mouseBuff[0] & ( 1 << 1 ) ) != 0 ) buttons ^= ( 1 << 0 );
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
                if( kbdResetTimer > 100 ) Keyboard_Reset();
            }

            if( mouseInited != MOUSE_OK )
            {
                mouseResetTimer++;
                if( mouseResetTimer > 100 ) Mouse_Reset();
            }

            //-------------------------------------------------


            static byte leds_prev = 0;
            byte leds = floppy_leds();

            if( leds != leds_prev )
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
        }

        portENTER_CRITICAL();
        timer_flag_1Hz--;
        portEXIT_CRITICAL();
    }

    if( fpgaConfigVersionPrev != 0 )
    {
        word trdosStatus = SystemBus_Read( 0xc00019 );
        if( ( trdosStatus & 1 ) != 0 )
        {
            bool trdosWr = ( ( trdosStatus >> 1 ) & 1 ) != 0;
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
                        if( counter == 0 ) UART0_WriteText( "Data write : " );

                        __TRACE( "%.2x.", trdosData );

                        counter++;
                        if( counter == 16 )
                        {
                            counter = 0;
                            UART0_WriteText( "\n" );
                        }
                    }
                    else //if( trdosAddr != 0xff )
                    {
                        if( counter != 0 )
                        {
                            counter = 0;
                            UART0_WriteText( "\n" );
                        }

                        word specPc = SystemBus_Read( 0xc00001 );
                        __TRACE( "0x%.4x WR: 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData );
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
                        if( counter == 0 ) UART0_WriteText( "Data read : " );

                        __TRACE( "%.2x.", trdosData );

                        counter++;
                        if( counter == 16 )
                        {
                            counter = 0;
                            UART0_WriteText( "\n" );
                        }
                    }
                    else if( trdosAddr != 0xff )
                    {
                        if( counter != 0 )
                        {
                            counter = 0;
                            UART0_WriteText( "\n" );
                        }

                        word specPc = SystemBus_Read( 0xc00001 );
                        __TRACE( "0x%.4x RD : 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData );
                    }
                }
            }

            SystemBus_Write( 0xc00019, 0 );
        }
    }
}

void UpdateCarrier( int delta )
{
    specConfig.specVideoSubcarrierDelta += delta;
    SystemBus_Write( 0xc00044, (dword) specConfig.specVideoSubcarrierDelta & 0xffff );
    SystemBus_Write( 0xc00045, (dword) specConfig.specVideoSubcarrierDelta >> 16 );

    __TRACE( "subcarrierDelta - %d\n", specConfig.specVideoSubcarrierDelta );
}

static word pressedKeys[4] = { KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE };

bool ReadKey( word &_keyCode, word &_keyFlags )
{
    _keyCode = KEY_NONE;
    _keyFlags = 0;

    static word keyFlags = 0x0000;
    static byte keyPrefix = 0x00;

    while( _keyCode == KEY_NONE && keyboard.GetCntr() > 0 )
    {
        word keyCode = keyboard.ReadByte();

        //char str[ 0x20 ];
        //sniprintf( str, sizeof(str), "key - 0x%.2x\n", keyCode );
        //UART0_WriteText( str );

        if( kbdInited == 0 && keyCode == 0xfa )
        {
            kbdInited = 1;
        }
        else if( kbdInited == 1 && keyCode == 0xaa )
        {
            kbdInited = KBD_OK;
            keyFlags = 0;
        }
        else if( kbdInited == KBD_OK )
        {
            if( keyCode == 0xfa || keyCode == 0xfe ) Keyboard_SendNext( keyCode );
            else if ( keyCode == 0xf0 ) keyFlags |= fKeyRelease;
            else if ( keyCode == 0xe0 || keyCode == 0xe1 )
            {
                keyPrefix = keyCode;
            }
            else
            {
                word keyRelease = ( keyFlags & fKeyRelease );
                keyFlags &= ~fKeyRelease;

                keyCode |= ( (word) keyPrefix << 8 );
                keyPrefix = 0x00;

                //char str[ 0x20 ];
                //sniprintf( str, sizeof(str), "keyCode - 0x%.4x\n", keyCode );
                //UART0_WriteText( str );

                word tempFlag = 0;
                if( keyCode == KEY_LSHIFT ) tempFlag = fKeyShiftLeft;
                else if( keyCode == KEY_RSHIFT ) tempFlag = fKeyShiftRight;
                else if( keyCode == KEY_LCTRL ) tempFlag = fKeyCtrlLeft;
                else if( keyCode == KEY_RCTRL ) tempFlag = fKeyCtrlRight;
                else if( keyCode == KEY_LALT ) tempFlag = fKeyAltLeft;
                else if( keyCode == KEY_RALT ) tempFlag = fKeyAltRight;

                if( tempFlag != 0 )
                {
                    if( keyRelease == 0 ) keyFlags |= tempFlag;
                    else keyFlags &= ~tempFlag;
                }

                if( keyRelease == 0 && ( keyFlags & fKeyCtrl ) != 0 )
                {
                    if( keyCode == KEY_HOME ) UpdateCarrier( 1 );   // HOME
                    if( keyCode == KEY_END ) UpdateCarrier( -1 );   // END
                    if( keyCode == KEY_PAGEUP ) UpdateCarrier( 10 );   // PGUP
                    if( keyCode == KEY_PAGEDOWN ) UpdateCarrier( -10 );   // PGDOWN
                }

                int i;
                const int pressedKeysCount = sizeof( pressedKeys ) / sizeof( word );
                bool result = false;

                for( i = 0; i < pressedKeysCount; i++ )
                {
                    if( pressedKeys[ i ] == keyCode )
                    {
                        if( keyRelease != 0 )
                        {
                            pressedKeys[ i ] = KEY_NONE;
                            result = true;
                        }

                        break;
                    }
                }

                if( i >= pressedKeysCount && keyRelease == 0 )
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

                if( result )
                {
                    _keyCode = keyCode;
                    _keyFlags = keyFlags | keyRelease;
                }
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
        word keyFlags2 = 0;

        if( ( joyCode & 0x80 )  ) keyFlags2 = fKeyRelease;
        joyCode &= 0x7f;

        if( joyCode < 16 ) keyFlags2 |= fKeyJoy1;
        else keyFlags2 |= fKeyJoy2;
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
            _keyFlags = keyFlags2;
        }
    }

    return _keyCode != KEY_NONE;
}

void Keyboard_Routine()
{
    word keyCode, keyFlags;

    while( ReadKey( keyCode, keyFlags ) )
    {
        DecodeKey( keyCode, keyFlags );
    }
}

//extern "C" RESULT PowerOn(void);
//extern "C" RESULT PowerOff();

const dword STACK_FILL = 0x37ACF177;
extern char *current_heap_end;

void InitStack()
{
    dword *stackPos;
    {
        dword stackStart;

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
    dword *stackPos;

    {
        dword stackStart;

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
    __TRACE( "heap max - 0x%.8x\n\n", (dword) current_heap_end );
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

	//**USB GPIO2->PD |= ( 1 << 14 );           // USB power on

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

    __TRACE( "Speccy2010, ver %d.%d, rev %d !\n", VER_MAJOR, VER_MINIR, REV );
    if( !pllStatusOK ) UART0_WriteText( "PLL initialization error !\n" );
    DelayMs( 100 );

    //**USB USB_Config();
	//**USB USB_Init();
    fdc_init();

    while( true )
    {
        Timer_Routine();
        //**USB MassStorage_Routine();
        Keyboard_Routine();
        Tape_Routine();

        fdc_dispatch();
        SystemBus_Write( 0xc0001d, (word) fdc_read( 0xff ) );

        //------------------------------------------------------------------------------

        /*
        static bool traceMode = false;

        if( traceMode )
        {
            char str[ 0x20 ];

            static word pcPrev = 0;
            word pc = SystemBus_Read( 0xc00001 );

            //if( pc < pcPrev || pc > ( pcPrev + 0x10 ) )
            if( pc != 0 )
            {
                sniprintf( str, sizeof(str), "pc - 0x%.4x, ", pc );
                UART0_WriteText( str );
                sniprintf( str, sizeof(str), "tick - %d\n", SystemBus_Read( 0xc00008 ) | ( SystemBus_Read( 0xc00009 ) << 16 ) );
                UART0_WriteText( str );
            }

            pcPrev = pc;
        }*/

        if( uart0.GetRxCntr() > 0  )
        {
            uart0.ReadByte();

            /*
            if( b == 'z' )
            {
                CPU_Stop();
                sniprintf( str, sizeof(str), "PC = 0x%x\n", SystemBus_Read( 0xc00001 ) );
                UART0_WriteText( str );
            }
            else if( b == 'x' )
            {
                CPU_Start();
            }
            else if( b == 'c' )
            {
                if( !CPU_Stopped() ) CPU_Stop();
                SystemBus_Write( 0xc00008, 0x01 );
                DelayMs( 1 );

                sniprintf( str, sizeof(str), "PC = 0x%x\n", SystemBus_Read( 0xc00001 ) );
                UART0_WriteText( str );
            }
            else if( b == 'v' )
            {
                sniprintf( str, sizeof(str), "PC = 0x%x\n", SystemBus_Read( 0xc00001 ) );
                UART0_WriteText( str );
            }
            else if( b == 'b' )
            {
                SystemBus_Write( 0xc0000a, 0x6e5f );
                SystemBus_Write( 0xc00009, 0x01 );
            }
            else if( b == 't' )
            {
                TestStack();
            }*/
        }
    }
}

volatile dword delayTimer = 0;
volatile dword tapeTimer = 0;

void TIM0_UP_IRQHandler()       //40kHz
{
	static dword tick = 0;

	if( delayTimer >= 25 ) delayTimer -= 25;
	else delayTimer = 0;

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

word get_ticks()
{
    return SystemBus_Read( 0xc0001c );
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

//---------------------------------------------------------------------------------------------

void *operator new( std::size_t size )
{
    return malloc( size );
}

void *operator new[](std::size_t size)
{
    void *ptr = malloc( size );
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

void __TRACE( const char *str, ... )
{
    static char fullStr[ 0x80 ];

    va_list ap;
    va_start( ap, str );
    vsniprintf( fullStr, sizeof( fullStr ), str, ap );
    va_end(ap);

    UART0_WriteText( fullStr );
}

