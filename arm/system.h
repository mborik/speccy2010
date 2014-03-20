//  patches to toolchain
// uncomment mthumb-interwork in gcc/config/t-arm-elf
// 	--disable-hosted-libstdcxx
// 	empty crt0.s in newlib/libgloss

#ifndef _SYSTEM_
#define _SYSTEM_

#include "types.h"

#define VER_MAJOR 1
#define VER_MINIR 0
#include "revision.h"

#ifdef __cplusplus
extern "C"
{
#endif

    #include "libstr/inc/75x_lib.h"

    #include "fatfs/ff.h"
    #include "fatfs/diskio.h"

    #include "crc16.h"
    #include "ps2scancodes.h"

    #include <stdio.h>
    #include <string.h>
    #include <time.h>

    //-------------------------------------------------------------------------

    void UART0_WriteText( const char *str );
    void FPGA_Config();

    void Keyboard_Reset();
    void Mouse_Reset();

    void Spectrum_UpdateConfig();
    void Spectrum_UpdateDisks();

    void BDI_Routine();
    void BDI_ResetWrite();
    void BDI_Write( byte data );
    void BDI_ResetRead( word counter );
    bool BDI_Read( byte *data );
    void BDI_StopTimer();
    void BDI_StartTimer();


    //void AddMallocRecord( dword type, dword address, dword size );
    //void TestHeap();

    void WDT_Kick();

    void DelayUs( dword );
    void DelayMs( dword );

    #define CNTR_INTERVAL 1
    dword get_ticks();

    void __TRACE( const char *str, ... );
    void portENTER_CRITICAL();
    void portEXIT_CRITICAL();

    void TIM0_UP_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#define PATH_SIZE 0x80

#ifdef __cplusplus
    #include "cstring.h"

    const dword FPGA_NONE = 0x00000000;
    const dword FPGA_SPECCY2010 = 0x00000001;
    const dword FPGA_ZET = 0x00000002;

    extern dword fpgaStatus;

    bool SystemBus_TestConfiguration();
    void SystemBus_SetAddress( dword address );
    word SystemBus_Read();
    word SystemBus_Read( dword address );
    void SystemBus_Write( word data );
    void SystemBus_Write( dword address, word data );

    bool FileExists( const char *str );
    bool ReadLine( FIL *file, CString &str );
    bool WriteText( FIL *file, const char *str );
    bool WriteLine( FIL *file, const char *str );

    const word fKeyRelease = ( 1 << 0 );
    const word fKeyJoy1 = ( 1 << 2 );
    const word fKeyJoy2 = ( 1 << 3 );

    const word fKeyShiftLeft = ( 1 << 4 );
    const word fKeyShiftRight = ( 1 << 5 );
    const word fKeyShift = fKeyShiftLeft | fKeyShiftRight;
    const word fKeyCtrlLeft = ( 1 << 6 );
    const word fKeyCtrlRight = ( 1 << 7 );
    const word fKeyCtrl = fKeyCtrlLeft | fKeyCtrlRight;
    const word fKeyAltLeft = ( 1 << 8 );
    const word fKeyAltRight = ( 1 << 9 );
    const word fKeyAlt = fKeyAltLeft | fKeyAltRight;

    const word fKeyCaps = ( 1 << 10 );
    const word fKeyRus = ( 1 << 11 );

    bool ReadKey( word &key, word &flags );
    word ReadKeySimple( bool norepeat = false );
    word ReadKeyFlags();

    void Timer_Routine();
    dword Timer_GetTickCounter();

    //void MassStorage_Routine();

    bool RTC_GetTime( tm *newTime );
    bool RTC_SetTime( tm *newTime );

    const char *GetFatErrorMsg( int id );

#endif

#endif
