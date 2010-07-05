#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>

#include "system.h"
#include "uarts.h"
#include "ps2port.h"
#include "fifo.h"

#include "specLoader.h"
#include "specShell.h"
#include "specConfig.h"

void LoadPage( FIL *file, byte page )
{
    dword addr = 0x800000 | ( page << 13 );
    word data;
    UINT res;

    for( int i = 0; i < 0x4000; i += 2 )
    {
        if( f_read( file, &data, 2, &res ) != FR_OK ) break;
        if( res == 0 ) break;

        SystemBus_Write( addr, data );
        addr++;

        if( ( i & 0xff ) == 0 )
        {
            SystemBus_Write( 0xc00016, 1 );
            WDT_Kick();
        }
        else if( ( i & 0xff ) == 2 ) SystemBus_Write( 0xc00016, 0 );
    }
}

bool LoadSnapshot( const char *fileName )
{
    bool result = false;

    bool stopped = CPU_Stopped();
    if( !stopped ) CPU_Stop();

    SystemBus_Write( 0xc00020, 0 );  // use bank 0

    byte specPortFe = SystemBus_Read( 0xc00016 );
    byte specPort7ffd = SystemBus_Read( 0xc00017 );
    byte specTrdosFlag = SystemBus_Read( 0xc00018 );
    word specPc = 0;

    FIL snaFile;
    if( f_open( &snaFile, fileName, FA_READ ) == FR_OK )
    {
        if( snaFile.fsize >= 0xc01b )
        {
            CPU_Start();
            CPU_Reset( false );
            DelayMs( 10 );

            CPU_Reset( true );
            CPU_Stop();

            dword addr;
            word data;
            word i;

            byte header[ 0x1c ];

            UINT res;
            f_lseek( &snaFile, 0 );
            f_read( &snaFile, header, 0x1b, &res );

            SystemBus_Write( 0xc00021, 0 );
            SystemBus_Write( 0xc00022, 0 );

            specPortFe = header[26] & 0x07;

            if( snaFile.fsize == 0xc01b )
            {
                specPort7ffd = ( 1 << 4 ) | ( 1 << 5 );
                specTrdosFlag = 0;
            }
            else
            {
                byte header2[ 4 ];
                f_lseek( &snaFile, 0x1b + 0xc000 );
                f_read( &snaFile, header2, 0x04, &res );

                specPc = header2[0] | ( header2[1] << 8 );
                specPort7ffd = header2[ 2 ];
                specTrdosFlag = header2[ 3 ];
            }

            addr = ( 0x02 << 14 ) | ( 0x00 );
            for( i = 0; i < 0x1b; i++ ) SystemBus_Write( addr++, header[ i ] );
            addr = ( 0x02 << 14 ) | ( 0x40 );
            for( i = 0; i < sizeof( specLoader ); i++ ) SystemBus_Write( addr++, specLoader[ i ] );

            CPU_ModifyPC( 0x8040, 0 );
            CPU_Start();
            DelayMs( 1 );
            CPU_Stop();

            //char str[ 0x20 ];
            //sniprintf( str, sizeof(str), "oldPc - 0x%.4x\n", SystemBus_Read( 0xc00001 ) );
            //UART0_WriteText( str );

            f_lseek( &snaFile, 0x1b );

            LoadPage( &snaFile, 0x05 );
            LoadPage( &snaFile, 0x02 );
            LoadPage( &snaFile, specPort7ffd & 0x07 );

            f_lseek( &snaFile, 0x1b + 0xc000 + 0x04 );

            for( byte page = 0; page < 8; page++ )
            {
                if( page != 0x05 && page != 0x02 && page != ( specPort7ffd & 0x07 ) ) LoadPage( &snaFile, page );
            }

            SystemBus_Write( 0xc00016, specPortFe );
            SystemBus_Write( 0xc00017, specPort7ffd );
            SystemBus_Write( 0xc00018, specTrdosFlag & 1 );

            if( snaFile.fsize == 0xc01b )
            {
                addr = 0x800000 | ( ( 0x100 * 0x4000 ) >> 1 );
                if( ( specPort7ffd & ( 1 << 4 ) ) != 0 ) addr |= 0x4000 >> 1;

                for( i = 0; i < 0x4000; i += 2 )
                {
                    data = SystemBus_Read( addr++ );

                    if( ( data & 0xff ) == 0xc9 ) specPc = i;
                    else if( ( data & 0xff00 ) == 0xc900 ) specPc = i + 1;
                }
            }

            CPU_ModifyPC( specPc, ( header[25] & 0x03 ) | 0x08 | ( header[19] & 0x04 ) );
            result = true;
        }
    }

    if( !stopped ) CPU_Start();
    return result;
}

void SavePage( FIL *file, byte page )
{
    dword addr = 0x800000 | ( page << 13 );
    word data;
    UINT res;

    for( int i = 0; i < 0x4000; i += 2 )
    {
        data = SystemBus_Read( addr );
        addr++;

        if( f_write( file, &data, 2, &res ) != FR_OK ) break;
        if( res == 0 ) break;

        if( ( i & 0xff ) == 0 )
        {
            SystemBus_Write( 0xc00016, 2 );
            WDT_Kick();
        }
        else if( ( i & 0xff ) == 2 ) SystemBus_Write( 0xc00016, 0 );
    }
}

void SaveSnapshot( const char *path, const char *name )
{
    bool stopped = CPU_Stopped();
    if( !stopped ) CPU_Stop();

    SystemBus_Write( 0xc00020, 0 );  // use bank 0

    byte specPortFe = SystemBus_Read( 0xc00016 );
    byte specPort7ffd = SystemBus_Read( 0xc00017 );
    byte specTrdosFlag = SystemBus_Read( 0xc00018 );

    word specPc = SystemBus_Read( 0xc00001 );
    byte specInt = SystemBus_Read( 0xc00002 );

    char fullName[ PATH_SIZE + 1 ];

    if( path == 0 ) path = "";

    if( name != 0 )
    {
        sniprintf( fullName, sizeof( fullName ), "%s%s", path, name );
    }
    else
    {
        tm time;
        GetTime( &time );
        sniprintf( fullName, sizeof( fullName ), "%s%.2d%.2d%.2d_%.2d%.2d%.2d.sna", path, time.tm_year - 100, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec );
    }

    FIL snaFile;
    if( f_open( &snaFile, fullName, FA_CREATE_ALWAYS | FA_READ | FA_WRITE ) == FR_OK )
    {
        dword addr;
        word i;

        byte header[ 0x1c ];

        UINT res;
        f_write( &snaFile, header, 0x1b, &res );

        SavePage( &snaFile, 0x05 );
        SavePage( &snaFile, 0x02 );
        SavePage( &snaFile, specPort7ffd & 0x07 );

        addr = ( 0x02 << 14 ) | ( 0x40 );
        for( i = 0; i < sizeof( specLoader ); i++ ) SystemBus_Write( addr++, specLoader[ i ] );

        SystemBus_Write( 0xc00016, specPortFe );

        // Save context
        CPU_ModifyPC( 0x8080, 0 );
        CPU_Start();
        DelayMs( 1 );
        CPU_Stop();

        addr = ( 0x02 << 14 ) | ( 0x00 );
        for( i = 0; i < 0x1b; i++ ) header[ i ] = SystemBus_Read( addr++ );

        header[ 19 ] = specInt & 0x04;
        header[ 25 ] = specInt & 0x03;
        header[ 26 ] = specPortFe & 0x07;

/*
        word specSp = header[ 23 ] | ( header[ 24 ] << 8 );
        specSp -= 2;

        f_lseek( &snaFile, 0x1b + specSp - 0x4000 );
        f_write( &snaFile, &specPc, 2, &res );

        header[ 23 ] = (byte) specSp;
        header[ 24 ] = (byte) ( specSp >> 8 );
*/

        f_lseek( &snaFile, 0 );
        f_write( &snaFile, header, 0x1b, &res );

        byte header2[ 4 ];
        header2[0] = (byte) specPc;
        header2[1] = (byte) ( specPc >> 8 );
        header2[2] = specPort7ffd;
        header2[3] = specTrdosFlag & 1;

        f_lseek( &snaFile, 0x1b + 0xc000 );
        f_write( &snaFile, header2, 0x04, &res );

        for( byte page = 0; page < 8; page++ )
        {
            if( page != 0x05 && page != 0x02 && page != ( specPort7ffd & 0x07 ) ) SavePage( &snaFile, page );
        }

        // Restore context
        CPU_ModifyPC( 0x8040, 0 );
        CPU_Start();
        DelayMs( 1 );
        CPU_Stop();

        f_lseek( &snaFile, 0x1b + 0x4000 );

        addr = ( 0x02 << 14 ) | ( 0x00 );
        for( i = 0; i < 0x40 + sizeof( specLoader ); i++ )
        {
            byte data;

            if( f_read( &snaFile, &data, 1, &res ) != FR_OK ) break;
            if( res == 0 ) break;

            SystemBus_Write( addr++, data );
        }

        f_close( &snaFile );
    }

    SystemBus_Write( 0xc00016, specPortFe );

    CPU_ModifyPC( specPc, specInt );
    if( !stopped ) CPU_Start();
}



