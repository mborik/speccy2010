#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>

#include "system.h"
#include "fifo.h"

#include "specTape.h"
#include "specShell.h"

bool tapeStarted = false;
bool tapeRestart = false;
bool tapeFinished = false;
bool tapeTzx = false;

#define BLOCK_DATA      0
#define SEQUENCE_DATA   1
#define SKIP_DATA       2

struct CTapeBlock
{
    dword pulse_pilot;
    dword pulse_sync1;
    dword pulse_sync2;
    dword pulse_zero;
    dword pulse_one;
    byte block_lastbit;

    word tape_pilot;
    byte tape_sync;
    word tape_pause;

    dword data_size;
    byte data_type;

    void Clean();
    void ParseHeader( byte *header );
};

void CTapeBlock::Clean()
{
    tape_pilot = 0;
    tape_sync  = 0;
    tape_pause = 0;
    data_size = 0;
}

byte ReadByte( byte *header )
{
	return header[ 0 ];
}

word ReadWord( byte *header )
{
	return ( word ) header[ 0 ] | (( word ) header[ 1 ] << 8 );
}


dword ReadWord3( byte *header )
{
	return ( word ) header[ 0 ] | (( word ) header[ 1 ] << 8 ) | (( dword ) header[ 2 ] << 16 );
}

dword ReadDword( byte *header )
{
	return ( word ) header[ 0 ] | (( word ) header[ 1 ] << 8 ) | (( dword ) header[ 2 ] << 16 ) | (( dword ) header[ 3 ] << 24 );
}

dword Convert( dword ticks )
{
    return ( ticks * 10 + 437 ) / ( 35 * 25 );
}

void CTapeBlock::ParseHeader( byte *header )
{
    pulse_pilot = Convert( 2168 );
    pulse_sync1 = Convert( 667 );
    pulse_sync2 = Convert( 735 );
    pulse_zero  = Convert( 855 );
    pulse_one   = Convert( 1710 );
    block_lastbit = 0;

    tape_pilot = 0;
    tape_sync = 0;
    tape_pause = 0;
    data_size = 0;

    if( !tapeTzx )
    {
        tape_pilot = 6000;
        tape_sync = 2;
        tape_pause = 2000;

        data_size = ReadWord( header );
        data_type = BLOCK_DATA;
    }
    else
    {
		switch ( header[0] )
		{
            case 0x10:
                tape_pilot = 6000;
                tape_sync = 2;
                tape_pause = ReadWord( header + 1 );

                data_size = ReadWord( header + 3 );
                data_type = BLOCK_DATA;
                break;

            case 0x11:
                pulse_pilot	= Convert( ReadWord( header + 1 ) );
                pulse_sync1 = Convert( ReadWord( header + 3 ) );
                pulse_sync2 = Convert( ReadWord( header + 5 ) );
                pulse_zero 	= Convert( ReadWord( header + 7 ) );
                pulse_one 	= Convert( ReadWord( header + 9 ) );

                tape_pilot 	= ReadWord( header + 11 );
                tape_sync = 2;
                block_lastbit = ( 8 - header[ 13 ] ) * 2;
                tape_pause = ReadWord( header + 14 );

                data_size = ReadWord3( header + 16 );
                data_type = BLOCK_DATA;
                break;

            case 0x12:
                pulse_pilot = Convert( ReadWord( header + 1 ) );
                tape_pilot = ReadWord( header + 3 );
                break;

            case 0x13:
                data_size = header[1] * 2;
                data_type = SEQUENCE_DATA;
                break;

            case 0x14:
                pulse_zero 	= Convert( ReadWord( header + 1 ) );
                pulse_one 	= Convert( ReadWord( header + 3 ) );
                block_lastbit = ( 8 - header[ 5 ] ) * 2;
                tape_pause = ReadWord( header + 6 );

                data_size = ReadWord3( header + 8 );
                data_type = BLOCK_DATA;
                break;

            case 0x15:
                data_size = ReadWord3( header + 6 ) * 2;
                data_type = SKIP_DATA;
                break;

            case 0x20:
                tape_pause = ReadWord( header + 1 );
                break;

            case 0x21:
            case 0x30:
                data_size = header[1];
                data_type = SKIP_DATA;
                break;

            case 0x22:
                break;
            case 0x23:
                break;
            case 0x24:
                break;
            case 0x25:
                break;
            case 0x31:
                data_size = header[2];
                data_type = SKIP_DATA;
                break;
            case 0x32:
                data_size = ReadWord( header + 1 );
                data_type = SKIP_DATA;
                break;
            case 0x33:
                data_size = header[1] * 3;
                data_type = SKIP_DATA;
                break;
            case 0x34:
                break;
            case 0x35:
                data_size = ReadDword( header + 0x11 );
                data_type = SKIP_DATA;
                break;
            case 0x40:
                data_size = ReadWord3( header + 9 );
                data_type = SKIP_DATA;
                break;

            case 0x5A:
                break;

            default:
                data_size = ReadDword( header + 1 );
                data_type = SKIP_DATA;
                break;
            }
    }
}

CTapeBlock currentBlock;

CFifo tapeFifo( 0x30 );
char tapePath[ PATH_SIZE + 1 ];

void Tape_SelectFile( const char *name )
{
    sniprintf( tapePath, sizeof( tapePath ), "%s", name );
}

void Tape_Start()
{
    tapeStarted = true;
    //UART0_WriteText( "Tape start...\n" );
}

void Tape_Stop()
{
    tapeStarted = false;
    //UART0_WriteText( "Tape stop...\n" );
}

void Tape_Restart()
{
    //UART0_WriteText( "Tape restart...\n" );

    currentBlock.Clean();
    tapeFifo.Clean();

    tapeRestart = true;
    tapeStarted = true;
}

bool Tape_Started()
{
    return tapeStarted;
}


byte GetHeaderSize( byte code )
{
    if( tapeTzx )
    {
        const byte tzx_header_size[] =
        {
            5, 19, 5, 2, 11, 9, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 2, 1, 3, 3, 1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 3, 3, 2, 9, 0x15,
        };

        if( code == 'Z' ) return 10;
        else return tzx_header_size[ code - 0x10 ];
    }
    else return 2;
}

dword GetDataSize( byte *header )
{
    if( tapeTzx ) return 0;
    else return ReadWord( header );
}

struct CTapeLoop
{
    dword fptr;
    dword counter;
};

void Tape_Routine()
{
    static FIL tapeFile;

    static dword headerSize = 0;
    static dword dataSize = 0;

    const int LOOPS_SIZE = 0x10;
    static CTapeLoop loops[ LOOPS_SIZE ];
    static int loopsSize;

    if( !tapeStarted && tapeFile.fsize != 0 && tapeFile.fptr >= tapeFile.fsize )
    {
        tapeRestart = true;
    }

    if( tapeStarted && ( tapeFile.fsize == 0 || tapeRestart ) )
    {
        if( f_open( &tapeFile, tapePath, FA_READ ) == FR_OK )
        {
            UART0_WriteText( "Tape openned...\n" );
            f_lseek( &tapeFile, 0 );
            tapeTzx = false;

            headerSize = 0;
            dataSize = 0;
            loopsSize = 0;

            if( tapeFile.fsize >= 10 )
            {
                char buff[ 10 ];
                UINT res;
                f_read( &tapeFile, buff, 10, &res );

                if( res == 10 && buff[0] == 'Z' && buff[1] == 'X' && buff[2] == 'T' ) tapeTzx = true;
                else f_lseek( &tapeFile, 0 );
            }
        }
        else
        {
            UART0_WriteText( "Cannot open tape file...\n" );
            tapeStarted = false;
        }

        tapeRestart = false;
    }

    static byte header[ 0x20 ];
    static dword headerPos;

    while( tapeStarted && tapeFifo.GetFree() > 0 )
    {
        if( headerSize > 0 )
        {
            tapeFifo.WriteByte( header[ headerPos++ ] );
            headerSize--;
            continue;
        }

        if( dataSize > 0 )
        {
            byte data;
            UINT res;
            f_read( &tapeFile, &data, 1, &res );

            if( res == 1 )
            {
                tapeFifo.WriteByte( data );
                dataSize--;
                continue;
            }
            else
            {
                tapeStarted = false;
                break;
            }
        }

        if( tapeFile.fptr >= tapeFile.fsize )
        {
            tapeFinished = true;
            break;
        }

        UINT res;
        f_read( &tapeFile, header, 1, &res );
        if( res != 1 )
        {
            tapeFinished = true;
            break;
        }

        byte hs = GetHeaderSize( header[0] );
        f_read( &tapeFile, header + 1, hs - 1, &res );
        if( res + 1 != hs )
        {
            tapeFinished = true;
            break;
        }

        CTapeBlock temp;
        temp.ParseHeader( header );

        if( temp.tape_pilot > 0 || temp.tape_sync > 0 || temp.tape_pause > 0 || ( temp.data_size > 0 && temp.data_type != SKIP_DATA ) )
        {
            headerPos = 0;
            headerSize = hs;
            dataSize = temp.data_size;
        }
        else if( header[ 0 ] == 0x24 )
        {
            if( loopsSize < LOOPS_SIZE )
            {
                loops[ loopsSize ].fptr = tapeFile.fptr;
                loops[ loopsSize ].counter = ReadWord( header + 1 );
                loopsSize++;
            }
        }
        else if( header[ 0 ] == 0x25 )
        {
            if( loopsSize > 0  )
            {
                if( loops[ loopsSize - 1 ].counter > 0 ) loops[ loopsSize - 1 ].counter--;

                if( loops[ loopsSize - 1 ].counter > 0 ) f_lseek( &tapeFile, loops[ loopsSize - 1 ].fptr );
                else loopsSize--;
            }
        }
        else
        {
            //char str[ 0x20 ];
            //sniprintf( str, sizeof(str), "0x%.4x - 0x%.2x - skip\n", tapeFile.fptr - hs, header[0] );
            //UART0_WriteText( str );

            f_lseek( &tapeFile, tapeFile.fptr + temp.data_size );
        }
    }
}

void Tape_TimerHandler()
{
    if( !tapeStarted || CPU_Stopped() ) return;

    static word tape_delay = 0;

    if( tape_delay > 0 )
    {
        tape_delay--;
    }

    if( tape_delay > 0 )
    {
        return;
    }

    static bool tape_pause_flag = false;
    static byte tape_phase = 0;

    if( !tape_pause_flag )
    {
        tape_phase ^= 1;
        SystemBus_Write( 0xc00015, tape_phase );
    }

    static byte tape_header[ 0x20 ];
    static byte tape_headerPos = 0;
    static byte tape_headerSize = 2;

    static byte tape_bit = 0;
    static byte tape_byte = 0;

    if( !currentBlock.tape_pilot && !currentBlock.tape_sync && !currentBlock.tape_pause && !currentBlock.data_size )
    {
        while( tapeFifo.GetCntr() > 0 && ( tape_headerPos == 0 || tape_headerPos < tape_headerSize ) )
        {
            tape_header[ tape_headerPos++ ] = tapeFifo.ReadByte();
            if( tape_headerPos == 1 ) tape_headerSize = GetHeaderSize( tape_header[0] );
        }

        if( tape_headerSize != 0 && tape_headerPos == tape_headerSize )
        {
            currentBlock.ParseHeader( tape_header );

            tape_bit = 0;
            tape_headerPos = 0;
            tape_headerSize = 0;

            tape_pause_flag = false;
        }
        else
        {
            tape_delay = 40;
            tape_pause_flag = true;

            if( tapeFinished )
            {
                tapeFinished = false;
                tapeStarted = false;
            }
        }
    }

    if ( currentBlock.tape_pilot > 0 )
	{
	    tape_delay = currentBlock.pulse_pilot;
        currentBlock.tape_pilot--;
	}
	else if ( currentBlock.tape_sync >= 2 )
	{
	    tape_delay = currentBlock.pulse_sync1;
		currentBlock.tape_sync--;
	}
	else if ( currentBlock.tape_sync == 1 )
	{
	    tape_delay = currentBlock.pulse_sync2;
		currentBlock.tape_sync--;
	}
	else if ( currentBlock.data_size > 0 )
	{
	    if( currentBlock.data_type == BLOCK_DATA )
        {
            if ( tape_bit == 0 )
            {
                if ( tapeFifo.GetCntr() > 0 )
                {
                    tape_byte = tapeFifo.ReadByte();
                    tape_bit = 16;
                }
            }

            if ( tape_bit != 0 )
            {
                if ( ( tape_byte & 0x80 ) != 0 ) tape_delay = currentBlock.pulse_one;
                else tape_delay = currentBlock.pulse_zero;

                if ( tape_bit & 1 ) tape_byte <<= 1;
                tape_bit--;

                if ( tape_bit == 0 ) currentBlock.data_size--;
                else if( currentBlock.data_size == 1 && tape_bit == currentBlock.block_lastbit )
                {
                    currentBlock.data_size = 0;
                    tape_bit = 0;
                }
            }
            else
            {
                tapeStarted = false;
            }
        }
        else if ( currentBlock.data_type == SEQUENCE_DATA )
        {
            if ( tapeFifo.GetCntr() >= 2 )
            {
                word next;
                tapeFifo.ReadFile( (byte*) &next, 2 );

                tape_delay = Convert( next );
                currentBlock.data_size -= 2;
            }
            else
            {
                tapeStarted = false;
            }
        }
        else
        {
            while ( tapeFifo.GetCntr() > 0 && currentBlock.data_size > 0 )
            {
                tapeFifo.ReadByte();
                currentBlock.data_size--;
                tape_delay = 0;
            }
        }
	}
	else if ( currentBlock.tape_pause > 0 )
	{
	    tape_delay = 40;
		currentBlock.tape_pause--;

		if ( currentBlock.tape_pause == 0 ) tape_pause_flag = false;
		else tape_pause_flag = true;
	}
}

