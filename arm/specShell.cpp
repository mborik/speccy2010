#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>

#include "system.h"
#include "uarts.h"
#include "fifo.h"

#include "specTape.h"
#include "specKeyboard.h"
#include "specShell.h"
#include "specConfig.h"
#include "specSnapshot.h"
#include "specBetadisk.h"

#include "specChars.h"
#include "specCharsA.h"
#include "specCharsB.h"

#include "string.h"

const dword SDRAM_PAGES = 0x200;
const dword SDRAM_PAGE_SIZE = 0x4000;

const dword VIDEO_PAGE = 0x108;
const dword VIDEO_PAGE_PTR = VIDEO_PAGE * SDRAM_PAGE_SIZE;

void WriteDisplay( word address, byte data )
{
    SystemBus_Write( VIDEO_PAGE_PTR + address, data );
}

void ClrScr( byte attr )
{
    int i = 0;
    for( ; i < 0x1800; i++ ) SystemBus_Write( VIDEO_PAGE_PTR + i, 0 );
    for( ; i < 0x1b00; i++ ) SystemBus_Write( VIDEO_PAGE_PTR + i, attr );
}

void WriteChar( byte x, byte y, char c )
{
    const byte *charTable = specChars;
    if( specConfig.specFont == 1 ) charTable = specCharsA;
    else if( specConfig.specFont == 2 ) charTable = specCharsB;

    if( x < 32 && y < 24 )
    {
        //if( (byte) c < 0x20 && (byte) c >= 0x80 ) c = 0;
        //else c -= 0x20;

        word address = x + ( y & 0x07 ) * 32 + ( y & 0x18 ) * 32 * 8;
        const byte *tablePos = &charTable[ (byte) c * 8 ];

        for( byte i = 0; i < 8; i++ )
        {
            WriteDisplay( address, *tablePos++ );
            address += 32 * 8;
        }
    }
}

void WriteLine( byte y, byte cy )
{
    y = y * 8 + cy;

    if( y < 24 * 8 )
    {
        word address = ( ( y & 0xc0 ) | ( ( y & 0x38 ) >> 3 ) | ( ( y & 0x07 ) << 3 ) ) << 5;
        for( byte i = 0; i < 32; i++ ) WriteDisplay( address++, 0xff );
    }
}

void WriteAttr( byte x, byte y, byte attr, byte n = 1 )
{
    if( x < 32 && y < 24 )
    {
        word address = 0x1800 + x + y * 32;

        while( n-- ) WriteDisplay( address++, attr );
    }
}

void WriteStr( byte x, byte y, const char *str, byte size = 0 )
{
    if( size == 0 ) size = strlen( str );

    while( size > 0 )
    {
        if( *str ) WriteChar( x++, y, *str++ );
        else WriteChar( x++, y, ' ' );

        size--;
    }
}

void WriteStrAttr( byte x, byte y, const char *str, byte attr, byte size = 0 )
{
    if( size == 0 ) size = strlen( str );

    WriteStr( x, y, str, size );
    WriteAttr( x, y, attr, size );
}

void Beep()
{
    byte specPortFe = SystemBus_Read( 0xc00016 );

    for ( int i = 0; i < 20; i++ )
    {
        //for ( int j = 0; j < 2; j++ );

        specPortFe ^= 0x10;
        SystemBus_Write( 0xc00016, specPortFe );
    }
}

char GetKey( bool wait = true )
{
    do
    {
        word keyCode = ReadKeySimple();

        if( keyCode != KEY_NONE )
        {
            char c = CodeToChar( keyCode );
            if( c != 0 ) Beep();
            return c;
        }
    } while( wait );

    return 0;
}

dword sdramHeapPtr = VIDEO_PAGE_PTR + SDRAM_PAGE_SIZE;

dword GetHeapMemory()
{
    return ( SDRAM_PAGES * SDRAM_PAGE_SIZE ) - ( sdramHeapPtr + 4 );
}

dword Malloc( word recordCount, word recordSize )
{
    dword size = recordSize;
    if( ( size & 1 ) != 0 ) size++;

    size *= recordCount;
    if( sdramHeapPtr + 4 + size > SDRAM_PAGES * SDRAM_PAGE_SIZE ) return 0;

    dword result = sdramHeapPtr;
    sdramHeapPtr += 4 + size;

    dword addr = 0x800000 | ( result >> 1 );
    SystemBus_Write( addr++, recordSize );
    SystemBus_Write( addr++, recordCount );

    return result;
}

void Free( dword sdramPtr )
{
    if( sdramPtr != 0 ) sdramHeapPtr = sdramPtr;
}

bool Read( void *rec, dword sdramPtr, word pos )
{
    if( sdramPtr == 0 ) return false;

    dword addr = 0x800000 | ( sdramPtr >> 1 );
    word recordSize = SystemBus_Read( addr++ );
    word recordCount = SystemBus_Read( addr++ );

    if( pos >= recordCount ) return false;

    addr += ( ( recordSize + 1 ) >> 1 ) * pos;
    byte *ptr = (byte*) rec;

    while( recordSize > 0 )
    {
        word data = SystemBus_Read( addr++ );

        *ptr++ = (byte) data;
        recordSize--;

        if( recordSize > 0 )
        {
            *ptr++ = (byte) ( data >> 8 );
            recordSize--;
        }
    }

    return true;
}

bool Write( void *rec, dword sdramPtr, word pos )
{
    if( sdramPtr == 0 ) return false;

    dword addr = 0x800000 | ( sdramPtr >> 1 );
    word recordSize = SystemBus_Read( addr++ );
    word recordCount = SystemBus_Read( addr++ );

    if( pos >= recordCount ) return false;

    addr += ( ( recordSize + 1 ) >> 1 ) * pos;
    byte *ptr = (byte*) rec;

    for( word i = 0; i < recordSize; i += 2 )
    {
        SystemBus_Write( addr++, ptr[0] | ( ptr[1] << 8 ) );
        ptr += 2;
    }

    return true;
}

const int FILES_SIZE = 10000;

struct FRECORD
{
	dword size;
	byte attr;
	byte sel;
	word date;
	word time;
	char name[ _MAX_LFN + 1 ];
};

dword table_buffer = 0;
char path[ PATH_SIZE ] = "";
char destinationPath[ PATH_SIZE ] = "/";

int files_size = 0;
bool too_many_files = false;

int files_table_start;
int files_sel;
int files_sel_number = 0;
char files_lastName[ _MAX_LFN + 1 ] = "";

const int FILES_PER_ROW = 18;
byte selx = 0, sely = 0;

byte comp_name( int a, int b )
{
    FRECORD ra, rb;
    Read( &ra, table_buffer, a );
    Read( &rb, table_buffer, b );

    strlwr( ra.name );
    strlwr( rb.name );

	if (( ra.attr & AM_DIR ) && !( rb.attr & AM_DIR ) ) return true;
	else if ( !( ra.attr & AM_DIR ) && ( rb.attr & AM_DIR ) ) return false;
	else return strcmp( ra.name, rb.name ) <= 0;
}

void swap_name( int a, int b )
{
    FRECORD ra, rb;
    Read( &ra, table_buffer, a );
    Read( &rb, table_buffer, b );

    Write( &ra, table_buffer, b );
    Write( &rb, table_buffer, a );
}

void qsort( int l, int h )
{
	int i = l;
	int j = h;

	int k = ( l + h ) / 2;

	while ( true )
	{
		while ( i < k && comp_name( i, k ) ) i++;
		while ( j > k && comp_name( k, j ) ) j--;

		if ( i == j ) break;
		swap_name( i, j );

		if ( i == k ) k = j;
		else if ( j == k ) k = i;

		WDT_Kick();
	}

	if ( l < k - 1 ) qsort( l, k - 1 );
	if ( k + 1 < h ) qsort( k + 1, h );

	WDT_Kick();
}

void CycleMark()
{
    const char marks[ 4 ] = { '/', '-', '\\', '|' };
    static int mark = 0;

    WriteChar( 0, FILES_PER_ROW + 4, marks[ mark ] );
    mark = ( mark + 1 ) & 3;
}

void read_dir()
{
    if( table_buffer == 0 ) table_buffer = Malloc( FILES_SIZE, sizeof( FRECORD ) );

	files_size = 0;
	too_many_files = false;
	files_table_start = 0;
	files_sel = 0;
	files_sel_number = 0;

    FRECORD fr;

    if( strlen( path ) != 0 )
    {
        fr.attr = AM_DIR;
        fr.sel = 0;
        fr.size = 0;
        strcpy( fr.name, ".." );
        Write( &fr, table_buffer, files_size++ );
    }

    DIR dir;
	FRESULT r;

	int pathSize = strlen( path );
	if( pathSize > 0 ) path[ pathSize - 1 ]	= 0;

	r = f_opendir( &dir, path );
	if( pathSize > 0 ) path[ pathSize - 1 ]	= '/';

    while ( r == FR_OK )
    {
        FILINFO fi;
        char lfn[ _MAX_LFN + 1 ];
        fi.lfname = lfn;
        fi.lfsize = sizeof( lfn );

        r = f_readdir( &dir, &fi );

        if ( r != FR_OK || fi.fname[0] == 0 ) break;
        if ( fi.fattrib & ( AM_HID | AM_SYS ) ) continue;

        if( lfn[0] == 0 ) strcpy( lfn, fi.fname );

        if( path[0] == 0 )
        {
            char temp[ 11 ];
            sniprintf( temp, sizeof(temp), "%s", lfn );
            strlwr( temp );

            if( strcmp( temp, "roms" ) == 0 || strcmp( temp, "speccy2010" ) == 0 ) continue;
        }

        if( files_size >= FILES_SIZE )
        {
            too_many_files = true;

            if( strlen( path ) != 0 ) files_size = 1;
            else files_size = 0;

            break;
        }

        fr.sel = 0;
        fr.attr = fi.fattrib;
        fr.size = fi.fsize;
        fr.date = fi.fdate;
        fr.time = fi.ftime;

        strcpy( fr.name, lfn );
        Write( &fr, table_buffer, files_size );

        files_size++;
        WDT_Kick();

        if( ( files_size & 0x3f ) == 0 ) CycleMark();
    }

    if( files_size > 0 && files_size < 0x100 ) qsort( 0, files_size - 1 );

    if( strlen( files_lastName ) != 0 )
    {
        FRECORD fr;

        for ( int i = 0; i < files_size; i++ )
        {
            Read( &fr, table_buffer, i );
            if( strcmp( fr.name, files_lastName ) == 0 )
            {
                files_sel = i;
                break;
            }
        }
        WDT_Kick();
    }
}

void make_short_name( char *sname, word size, const char* name )
{
    word nSize = strlen( name );

    if( nSize + 1 <= size ) strcpy( sname, name );
    else
    {
        word sizeA = ( size - 2 ) / 2;
        word sizeB = ( size - 1 ) - ( sizeA + 1 );

        memcpy( sname, name, sizeA );
        sname[ sizeA ] = '~';
        memcpy( sname + sizeA + 1, name + nSize - sizeB, sizeB + 1 );
    }
}

void make_short_name( CString &str, word size, const char* name )
{
    word nSize = strlen( name );

    str = name;

    if( nSize + 1 > size )
    {
        str.Delete( ( size - 2 ) / 2, 2 + nSize - size );
        str.Insert( ( size - 2 ) / 2, "~" );
    }
}

void display_path( char *str, int col, int row, byte max_sz )
{
	char path_buff[ 33 ] = "/";
	char *path_short = str;

	if ( strlen( str ) > max_sz )
	{
		while ( strlen( path_short ) + 2 > max_sz )
		{
			path_short++;
			while ( *path_short != '/' ) path_short++;
		}

		strcpy( path_buff, "..." );
	}

	strcat( path_buff, path_short );
	WriteStr( col, row, path_buff, max_sz );
}

byte get_sel_attr( FRECORD &fr )
{
    byte result = 007;

    if( ( fr.attr & AM_DIR ) == 0 )
    {
        strlwr( fr.name );

        char *ext = fr.name + strlen( fr.name );
        while( ext > fr.name && *ext != '.' ) ext--;

        if ( strcmp( ext, ".trd" ) == 0 || strcmp( ext, ".fdi" ) == 0 || strcmp( ext, ".scl" ) == 0 ) result = 006;
        else if ( strcmp( ext, ".tap" ) == 0 || strcmp( ext, ".tzx" ) == 0 ) result = 004;
        else if ( strcmp( ext, ".sna" ) == 0 ) result = 0103;
        else if ( strcmp( ext, ".scr" ) == 0 ) result = 0102;
        else result = 005;
    }

    if( fr.sel ) result = ( result & 077) | 010;

    return result;
}

void show_table()
{
	display_path( path, 0, FILES_PER_ROW + 3, 32 );

    FRECORD fr;

    for ( int i = 0; i < FILES_PER_ROW; i++ )
    {
        for ( int j = 0; j < 2; j++ )
        {
            int col = j * 16;
            int row = i + 2;
            int pos = i + j * FILES_PER_ROW + files_table_start;

            Read( &fr, table_buffer, pos );

            char sname[ 16 ];
            make_short_name( sname, sizeof( sname ), fr.name );

            //if( ( fr.attr & AM_DIR ) == 0 ) strlwr( sname );
            //else strupr( sname );

            if ( pos < files_size )
            {
                WriteAttr( col, row, get_sel_attr( fr ), 16 );

                if( fr.sel ) WriteChar( col, row, 0x95 );
                else WriteChar( col, row, ' ' );

                WriteStr( col + 1, row, sname, 15 );
            }
            else
            {
                WriteAttr( col, row, 0, 16 );
                WriteStr( col, row, "", 16 );
            }
        }
	}

	if ( too_many_files )
	{
	    WriteStr( 3, 5, "too many files (>9999) !", 0 );
		WriteAttr( 3, 5, 0102, 24 );
	}
	else if ( files_size == 0 )
	{
	    WriteStr( 10, 5, "no files !", 0 );
		WriteAttr( 10, 5, 0102, 10 );
	}
}

bool calc_sel()
{
    if( files_size == 0 ) return false;

	if ( files_sel >= files_size ) files_sel = files_size - 1;
	else if ( files_sel < 0 ) files_sel = 0;

    bool redraw = false;

	while ( files_sel < files_table_start )
	{
		files_table_start -= FILES_PER_ROW;
		redraw = true;
	}

	while ( files_sel >= files_table_start + FILES_PER_ROW * 2 )
	{
		files_table_start += FILES_PER_ROW;
		redraw = true;
	}

	selx = (( files_sel - files_table_start ) / FILES_PER_ROW );
	sely = (( files_sel - files_table_start ) % FILES_PER_ROW );

	return redraw;
}

void hide_sel()
{
    FRECORD fr;
    Read( &fr, table_buffer, files_sel );

    if ( files_size != 0 )
    {
        WriteAttr( selx * 16, 2 + sely, get_sel_attr( fr ), 16 );

        if( fr.sel ) WriteChar( selx * 16, 2 + sely, 0x95 );
        else WriteChar( selx * 16, 2 + sely, ' ' );
    }

    WriteStr( 0, FILES_PER_ROW + 4, "", 32 );
    WriteStr( 0, FILES_PER_ROW + 5, "", 32 );
}

void show_sel( bool redraw = false )
{
	if( calc_sel() || redraw ) show_table();

    if ( files_size != 0 )
    {
        FRECORD fr;
        Read( &fr, table_buffer, files_sel );

        WriteAttr( selx * 16, 2 + sely, 071, 16 );

        if( fr.sel ) WriteChar( selx * 16, 2 + sely, 0x95 );
        else WriteChar( selx * 16, 2 + sely, ' ' );

        char sname[ PATH_SIZE ];
        make_short_name( sname, 33, fr.name );
        WriteStr( 0, FILES_PER_ROW + 4, sname, 32 );

        if ( files_sel_number > 0  )
        {
            sniprintf( sname, sizeof(sname), "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "" );
            WriteStr( 0, FILES_PER_ROW + 5, sname, 32 );
        }
        else
        {
            if( fr.date == 0 )
            {
                WriteStr( 0, FILES_PER_ROW + 5, "", 15 );
            }
            else
            {
                sniprintf( sname, sizeof(sname), "%.2u.%.2u.%.2u  %.2u:%.2u", fr.date & 0x1f,
                                                                            ( fr.date >> 5 ) & 0x0f,
                                                                            ( 80 + ( fr.date >> 9 ) ) % 100,
                                                                            (fr.time >> 11 ) & 0x1f,
                                                                            (fr.time >> 5 ) & 0x3f );
                WriteStr( 0, FILES_PER_ROW + 5, sname, 15 );
            }

            if ( ( fr.attr & AM_DIR ) != 0 ) sniprintf( sname, sizeof(sname), "      Folder" );
            else if( fr.size < 9999 ) sniprintf( sname, sizeof(sname), "%10u B", fr.size );
            else if( fr.size < 0x100000 ) sniprintf( sname, sizeof(sname), "%6u.%.2u kB", fr.size >> 10, ( ( fr.size & 0x3ff ) * 100 ) >> 10 );
            else sniprintf( sname, sizeof(sname), "%6u.%.2u MB", fr.size >> 20, ( ( fr.size & 0xfffff ) * 100 ) >> 20 );

            WriteStr( 20, FILES_PER_ROW + 5, sname, 12 );
        }
    }
}

void leave_dir()
{
    FRECORD fr;

    byte i = strlen( path );
    char dir_name[ _MAX_LFN + 1 ];

    if ( i != 0 )
    {
        i--;
        path[i] = 0;

        while ( i != 0 && path[i - 1] != '/' ) i--;
        strcpy( dir_name, &path[i] );

        path[i] = 0;
        read_dir();

        for ( files_sel = 0; files_sel < files_size; files_sel++ )
        {
            Read( &fr, table_buffer, files_sel );
            if ( strcmp( fr.name, dir_name ) == 0 ) break;
        }

        if (( files_table_start + FILES_PER_ROW * 2 - 1 ) < files_sel )
        {
            files_table_start = files_sel;
        }

        sely = (( files_sel - files_table_start ) % FILES_PER_ROW );
        selx = (( files_sel - files_table_start ) / FILES_PER_ROW );

        show_table();
    }
}

void InitScreen()
{
    byte attr = 0x07;
    ClrScr( attr );

    SystemBus_Write( 0xc00021, 0x8000 | VIDEO_PAGE );                   // Enable shell videopage
    SystemBus_Write( 0xc00022, 0x8000 | ( ( attr >> 3 ) & 0x03 ) );     // Enable shell border

    char str[33];
    sniprintf( str, sizeof(str), " -= Speccy2010, v%d.%.2d, r%.4d =-\n", VER_MAJOR, VER_MINIR, REV );

    WriteStr( 0, 0, str );
    WriteAttr( 0, 0, 0x44, strlen( str ) );

    WriteAttr( 0, 1, 0x06, 32 );
    WriteLine( 1, 3 );
    WriteLine( 1, 5 );

    WriteAttr( 0, FILES_PER_ROW + 2, 0x06, 32 );
    WriteLine( FILES_PER_ROW + 2, 3 );
    WriteLine( FILES_PER_ROW + 2, 5 );
}

int cpuStopNesting = 0;

void CPU_Start()
{
    if( cpuStopNesting > 0 )
    {
        if( cpuStopNesting == 1 )
        {
            ResetKeyboard();
            SystemBus_Write( 0xc00000, 0x0000 );
            SystemBus_Write( 0xc00008, 0x0001 );
            BDI_StartTimer();
        }

        cpuStopNesting--;
    }
}

void CPU_Stop()
{
    if( cpuStopNesting == 0 )
    {
        SystemBus_Write( 0xc00000, 0x0001 );
        BDI_StopTimer();

        while( true )
        {
            BDI_Routine();
            if( ( SystemBus_Read( 0xc00000 ) & 0x0001 ) != 0 ) break;
        }

        //SystemBus_Write( 0xc00000, 0x0000 );
    }

    cpuStopNesting++;
}

bool CPU_Stopped()
{
    return cpuStopNesting > 0;
}

void CPU_Reset( bool res )
{
    if( res == false )
    {
        if( specConfig.specUseBank0 && specConfig.specRom != SpecRom_Classic48 )
        {
            SystemBus_Write( 0xC00018, 0x01 ); //specTrdosFlag
        }
        else
        {
            SystemBus_Write( 0xC00018, 0x00 ); //specTrdosFlag
        }

        if( specConfig.specRom == SpecRom_Classic48 )
        {
            SystemBus_Write( 0xC00017, 0x30 ); //specPort7ffd
        }
        else
        {
            SystemBus_Write( 0xC00017, 0x00 ); //specPort7ffd
        }
    }

    if( !CPU_Stopped() )
    {
        if( res ) SystemBus_Write( 0xc00000, 0x0008 );
        else SystemBus_Write( 0xc00000, 0x0000 );
    }

    fdc_reset();

    //if( res ) GPIO1->PD |= ( 1 << 13 );               // RESET HIGH
    //else GPIO1->PD &= ~( 1 << 13 );                   // RESET LOW
}

void CPU_ModifyPC( word pc, byte istate )
{
    CPU_Stop();

    SystemBus_Write( 0xc00001, pc );
    SystemBus_Write( 0xc00002, istate );

    SystemBus_Write( 0xc00000, 0x0003 );
    DelayUs( 1 );
    SystemBus_Write( 0xc00000, 0x0001 );

    CPU_Start();
}

const char *Shell_GetPath()
{
    return path;
}

void Shell_Pause()
{
    CPU_Stop();
    GetKey( true );
    CPU_Start();
}

void Shell_Window( int x, int y, int w, int h, const char *title, byte attr )
{
    int titleSize = strlen( title );
    int titlePos = ( w - titleSize ) / 2;

    for( int i = 0; i < h; i++ )
        for( int j = 0; j < w; j++ )
        {
            char current = ' ';

            if( i == 0 && j == 0 ) current = 0x82;
            else if( i == 0 && j == w - 1 ) current = 0x83;
            else if( i == h - 1 && j == 0 ) current = 0x84;
            else if( i == h - 1 && j == w - 1 ) current = 0x85;
            else if( i == 0 || i == h - 1 ) current = 0x80;
            else if( j == 0 || j == w - 1 ) current = 0x81;

            if( i == 0 )
            {
                if( j >= titlePos && j < ( titlePos + titleSize ) )
                {
                    current = title[ j - titlePos ];
                }
                else if( j == ( titlePos - 1 ) || j == ( titlePos + titleSize ) )
                {
                    current = ' ';
                }
            }

            WriteAttr( x + j, y + i, attr );
            WriteChar( x + j, y + i, current );
        }
}

enum { MB_NO, MB_OK, MB_YESNO };

bool Shell_MessageBox( const char *title, const char *str, const char *str2 = "", const char *str3 = "", int type = MB_OK, byte attr = 027, byte attrSel = 052 )
{
    int w = 12;
    int h = 3;

    if( w < (int) strlen( title ) + 6 ) w = strlen( title ) + 6;
    if( w < (int) strlen( str ) + 2 ) w = strlen( str ) + 2;

    if( str2[0] != 0 )
    {
        if( w < (int) strlen( str2 ) + 2 ) w = strlen( str2 ) + 2;
        h++;

        if( str3[0] != 0 )
        {
            if( w < (int) strlen( str3 ) + 2 ) w = strlen( str3 ) + 2;
            h++;
        }
    }

    if( type != MB_NO ) h++;

    int x = ( 32 - w ) / 2;
    int y = ( 22 - h ) / 2;

    Shell_Window( x, y, w, h, title, attr );
    x++;
    y++;

    WriteStr( x, y++, str );
    if( str2[0] != 0 )
    {
        WriteStr( x, y++, str2 );
        if( str3[0] != 0 ) WriteStr( x, y++, str3 );
    }

    bool result = true;
    if( type == MB_NO ) return true;

    while( true )
    {
        if( type == MB_OK )
        {
            WriteStrAttr( 16 - 2, y, " OK ", attrSel );
        }
        else
        {
            WriteStrAttr( 16 - 5, y, " Yes ", result ? attrSel : attr );
            WriteStrAttr( 16, y, " No ", result ? attr : attrSel );
        }

        byte key = GetKey();

        if( key == K_ESC ) result = false;
        if( key == K_ESC || key == K_RETURN ) break;

        if( type == MB_YESNO ) result = !result;
    }

    return result;
}

bool Shell_InputBox( const char *title, const char *str, CString &buff )
{
    int w = 22;
    int h = 4;

    if( w < (int) strlen( title ) + 6 ) w = strlen( title ) + 6;
    if( w < (int) strlen( str ) + 2 ) w = strlen( str ) + 2;

    int x = ( 32 - w ) / 2;
    int y = ( 22 - h ) / 2;

    Shell_Window( x, y, w, h, title, 0050 );

    x++;
    y++;
    w -= 2;

    WriteStr( x, y++, str );
    WriteAttr( x, y, 0150, w );

    int p = strlen( buff );
    int s = 0;

    while( true )
    {
        while( ( p - s ) >= w ) s++;
        while( ( p - s ) < 0 ) s--;
        while( s > 0 && ( s + w - 1 ) > buff.Length() ) s--;

        for( int i = 0; i < w; i++ )
        {
            WriteChar( x + i, y, buff.GetSymbol( s + i ) );
            WriteAttr( x + i, y, i == ( p - s ) ? 0106 : 0150 );
        }

        char key = GetKey();

        if( key == K_RETURN ) return true;
        else if( key == K_ESC ) return false;
        else if( key == K_LEFT && p > 0 ) p--;
        else if( key == K_RIGHT && p < buff.Length() ) p++;
        else if( key == K_HOME ) p = 0;
        else if( key == K_END ) p = buff.Length();
        else if( key == K_BACKSPACE && p > 0 ) buff.Delete( --p, 1 );
        else if( key == K_DELETE && p < buff.Length() ) buff.Delete( p, 1 );
        else if( (byte) key >= 0x20 ) buff.Insert( p++, key );
    }
}

class CTextReader
{
    FIL fil;
    dword buffer;

    int lines;
    dword nextLinePos;
    bool white;

public:
    static const int LINES_MAX = 0x4000;

public:
    CTextReader( const char *fullName, int size );
    ~CTextReader();

    int GetLines() { return lines; }
    void SetLine( int i );
    char GetChar();
};

CTextReader::CTextReader( const char *fullName, int size )
{
    buffer = Malloc( LINES_MAX, sizeof( fil.fptr ) );

    if( f_open( &fil, fullName, FA_READ ) != FR_OK )
    {
        lines = -1;
        return;
    }

    lines = 0;
    Write( &fil.fptr, buffer, lines++ );

    white = false;
    char c;
    int pos = 0;

    int lastPos = 0;
    dword lastPtr = 0;
    bool prevWhite = false;

    while( fil.fptr < fil.fsize && lines < LINES_MAX )
    {
        dword res;
        f_read( &fil, &c, 1, &res );

        if( c == '\r' ) continue;

        if( c == '\t' || c == ' ' )
        {
            if( white ) continue;
            else white = true;
        }
        else white = false;

        pos++;

        if( !white && prevWhite )
        {
            lastPos = pos - 1;
            lastPtr = fil.fptr - 1;
        }

        if( c == '\n' || ( !white && pos > size ) )
        {
            if( c == '\n' )
            {
                lastPos = pos;
                lastPtr = fil.fptr;
            }
            else if( lastPtr == 0 )
            {
                lastPos = pos - 1;
                lastPtr = fil.fptr - 1;
            }

            Write( &lastPtr, buffer, lines++ );

            white = false;
            pos -= lastPos;

            lastPos = 0;
            lastPtr = 0;
        }

        prevWhite = white;
    }

    f_lseek( &fil, 0 );
}

CTextReader::~CTextReader()
{
    f_close( &fil );
    Free( buffer );
}

void CTextReader::SetLine( int i )
{
    if( i >= 0 && i < lines )
    {
        dword ptr;
        Read( &ptr, buffer, i );
        f_lseek( &fil, ptr );

        if( i + 1 < lines ) Read( &nextLinePos, buffer, i + 1 );
        else nextLinePos = fil.fsize;
    }
    else
    {
        f_lseek( &fil, fil.fsize );
    }

    white = false;
}

char CTextReader::GetChar()
{
    char result;
    dword res;

    while( true )
    {
        if( fil.fptr >= nextLinePos ) return ' ';

        f_read( &fil, &result, 1, &res );
        if( result == '\r' ) continue;

        if( result == '\n' || result == '\t' || result == ' ' )
        {
            if( white ) continue;

            result = ' ';
            white = true;
        }
        else
        {
            white = false;
        }

        break;
    }

    return result;
}

void Shell_Viewer( const char *fullName )
{
    CTextReader reader( fullName, 32 );
    if( reader.GetLines() < 0 )
    {
        Shell_MessageBox( "Error", "Cannot open file !" );
        return;
    }

    if( reader.GetLines() == reader.LINES_MAX )
    {
        Shell_MessageBox( "Error", "Lines number > LINES_MAX !" );
    }

    for( int j = 0; j < FILES_PER_ROW; j++ ) WriteAttr( 0, 2 + j, 007, 32 );

    int pos = 0;
    int maxPos = max( 0, reader.GetLines() - FILES_PER_ROW );

    byte key = 0;
    while( key != K_ESC )
    {
        int p = 100;
        if( maxPos > 0 ) p = pos * 10000 / maxPos;

        char str[ 10 ];
        sniprintf( str, sizeof( str ), "%d.%.2d %%", p / 100, p % 100 );
        WriteStr( 0, FILES_PER_ROW + 5, str, 32 );

        for( int j = 0; j < FILES_PER_ROW; j++ )
        {
            reader.SetLine( pos + j );
            for( int i = 0; i < 32; i++ ) WriteChar( i, 2 + j, reader.GetChar() );
        }

        key = GetKey();

        if( key == K_DOWN ) pos = min( pos + 1, maxPos );
        else if ( key == K_UP) pos = max( pos - 1, 0 );
		else if ( key == K_PAGEDOWN || key == K_RIGHT ) pos = min( pos + FILES_PER_ROW, maxPos );
		else if ( key == K_PAGEUP || key == K_LEFT ) pos = max( pos - FILES_PER_ROW, 0 );
		else if ( key == K_HOME ) pos = 0;
		else if ( key == K_END ) pos = maxPos;
    }
}

bool Shell_CopyItem( const char *srcName, const char *dstName, bool move = false )
{
    int res;

    const char *sname = &dstName[ strlen(dstName) ];
    while( sname != dstName && sname[-1] != '/' ) sname--;

    char shortName[ 20 ];
    make_short_name( shortName, sizeof( shortName ), sname );

    show_table();
    Shell_MessageBox( "Processing", shortName, "", "", MB_NO, 050 );

    if( move )
    {
        res = f_rename( srcName, dstName );
    }
    else
    {
        FILINFO finfo;
        char lfn[1];
        finfo.lfname = lfn;
        finfo.lfsize = 0;

        FIL src, dst;
        UINT size;

        byte buff[ 0x100 ];

        //dword block;
        //dword blockSize = GetHeapMemory() / sizeof( buff );

        res = f_open( &src, srcName, FA_READ | FA_OPEN_ALWAYS );
        if( res != FR_OK ) goto copyExit1;

        if( f_stat( dstName, &finfo ) == FR_OK )
        {
            res = FR_EXIST;
            goto copyExit2;
        }

        res = f_open( &dst, dstName, FA_WRITE | FA_CREATE_ALWAYS );
        if( res != FR_OK ) goto copyExit2;

        size = src.fsize;
        //block = Malloc( blockSize, sizeof( buff ) );

        while( size > 0 )
        {
            UINT currentSize = min( size, sizeof(buff) );
            UINT r;

            res = f_read( &src, buff, currentSize, &r );
            if( res != FR_OK ) break;

            res = f_write( &dst, buff, currentSize, &r );
            if( res != FR_OK ) break;

            size -= currentSize;
            if( ( src.fptr & 0x300 ) == 0 ) CycleMark();
        }

        //FreeBlock( block );

        f_close( &dst );
        if( res != FR_OK ) f_unlink( dstName );

    copyExit2:
        f_close( &src );

    copyExit1:
        ;
    }

    if( res == FR_OK ) return true;

    if( move ) Shell_MessageBox( "Error", "Cannot move/rename to", shortName, GetFatErrorMsg( res ) );
    else Shell_MessageBox( "Error", "Cannot copy to", shortName, GetFatErrorMsg( res ) );

    show_table();

    return false;
}

bool Shell_Copy( const char *_name, bool move )
{
    CString name;

    if( ( ReadKeyFlags() & fKeyShift ) != 0 ) name = _name;
    else name = destinationPath;

    const char *title = move ? "Move/Rename" : "Copy";
    if( !Shell_InputBox( title, "Enter new name/path :", name ) ) return false;

    bool newPath = false;

    char pname[ PATH_SIZE ];
    char pnameNew[ PATH_SIZE ];

    if( name.GetSymbol( 0 ) == '/' )
    {
        if( name.GetSymbol( name.Length() - 1 ) != '/' ) name += '/';
        newPath = true;
    }

    if( files_sel_number == 0 || !newPath )
    {
        sniprintf( pname, sizeof( pname ), "%s%s", path, _name );

        if( newPath ) sniprintf( pnameNew, sizeof( pnameNew ), "%s%s", name.String() + 1, _name );
        else sniprintf( pnameNew, sizeof( pnameNew ), "%s%s", path, name.String() );

        if( Shell_CopyItem( pname, pnameNew, move ) && !newPath && move )
        {
            sniprintf( files_lastName, sizeof( files_lastName ), "%s", name.String() );
        }
    }
    else
    {
        FRECORD fr;

        for ( int i = 0; i < files_size; i++ )
        {
            Read( &fr, table_buffer, i );

            if( fr.sel )
            {
                sniprintf( pname, sizeof( pname ), "%s%s", path, fr.name );
                sniprintf( pnameNew, sizeof( pnameNew ), "%s%s", name.String() + 1, fr.name );

                if( !Shell_CopyItem( pname, pnameNew, move ) ) break;

                fr.sel = false;
                files_sel_number--;

                Write( &fr, table_buffer, i );
            }
        }
    }

    return !newPath || move;
}

bool Shell_CreateFolder()
{
    CString name = "";
    if( !Shell_InputBox( "Create folder", "Enter name :", name ) ) return false;

    char pname[ PATH_SIZE ];
    sniprintf( pname, sizeof( pname ), "%s%s", path, name.String() );

    int res = f_mkdir( pname );
    if( res == FR_OK )
    {
        sniprintf( files_lastName, sizeof( files_lastName ), "%s", name.String() );
        return true;
    }

    char shortName[ 20 ];
    make_short_name( shortName, sizeof( shortName ), name );

    Shell_MessageBox( "Error", "Cannot create the folder", shortName, GetFatErrorMsg( res ) );
    show_table();

    return false;
}

bool Shell_DeleteItem( const char *name )
{
    int res = f_unlink( name );
    if( res == FR_OK ) return true;

    const char *sname = &name[ strlen(name) ];
    while( sname != name && sname[-1] != '/' ) sname--;

    char shortName[ 20 ];
    make_short_name( shortName, sizeof( shortName ), sname );

    Shell_MessageBox( "Error", "Cannot delete the item", shortName, GetFatErrorMsg( res ) );
    show_table();

    return false;
}

bool Shell_Delete( const char *name )
{
    if( files_sel_number == 0 && strcmp( name, ".." ) == 0 ) return false;

    char sname[ PATH_SIZE ];

    if( files_sel_number > 0 ) sniprintf( sname, sizeof(sname), "selected %u item%s", files_sel_number, files_sel_number > 1 ? "s" : "" );
    else make_short_name( sname, 21, name );

    strcat( sname, "?" );

    if( !Shell_MessageBox( "Delete", "Do you wish to delete", sname, "", MB_YESNO, 0050, 0151 ) ) return false;

    if( files_sel_number == 0 )
    {
        sniprintf( sname, sizeof(sname), "%s%s", path, name );
        Shell_DeleteItem( sname );
    }
    else
    {
        FRECORD fr;

        for ( int i = 0; i < files_size; i++ )
        {
            Read( &fr, table_buffer, i );

            if( fr.sel )
            {
                sniprintf( sname, sizeof(sname), "%s%s", path, fr.name );
                if( !Shell_DeleteItem( sname ) ) break;
            }
        }
    }

    return true;
}

bool Shell_EmptyTrd( const char *_name )
{
    char name[ PATH_SIZE ];
    bool result = false;

    if( strlen( _name ) == 0 )
    {
        CString newName = "empty.trd";
        if( !Shell_InputBox( "Format", "Enter name :", newName ) ) return false;
        show_table();
        sniprintf( name, sizeof( name ), "%s", newName.String() );
        result = true;

        char *ext = name + strlen( name );
        while( ext > name && *ext != '.' ) ext--;
        strlwr( ext );

        if( strcmp( ext, ".trd" ) != 0 ) sniprintf( name, sizeof( name ), "%s.trd", newName.String() );
        else sniprintf( name, sizeof( name ), "%s", newName.String() );
    }
    else
    {
        sniprintf( name, sizeof( name ), "%s", _name );

        char *ext = name + strlen( name );
        while( ext > name && *ext != '.' ) ext--;
        strlwr( ext );

        if( strcmp( ext, ".trd" ) != 0 ) return false;

        make_short_name( name, 21, _name );
        strcat( name, "?" );

        if( !Shell_MessageBox( "Format", "Do you wish to format", name, "", MB_YESNO, 0050, 0151 ) ) return false;
        show_table();

        sniprintf( name, sizeof( name ), "%s", _name );
    }

    CString label = name;
    label.TrimRight( 4 );
    if( label.Length() > 8 ) label.TrimRight( label.Length() - 8 );

    if( !Shell_InputBox( "Format", "Enter disk label :", label ) ) return false;
    show_table();

    char pname[ PATH_SIZE ];
    sniprintf( pname, sizeof(pname), "%s%s", path, name );

    const byte zero[ 0x10 ] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
    const byte sysArea[] = { 0x00, 0x00, 0x01, 0x16, 0x00, 0xf0, 0x09, 0x10 };
    char sysLabel[ 8 ];
    strncpy( sysLabel, label.String(), sizeof( sysLabel ) );

    int res;
    UINT r;
    FIL dst;

    {
        res = f_open( &dst, pname, FA_WRITE | FA_OPEN_ALWAYS );
        if( res != FR_OK ) goto formatExit1;

        for( int i = 0; i < 256 * 16; i += sizeof( zero )  )
        {
            res = f_write( &dst, zero, sizeof( zero ), &r );
            if( res != FR_OK ) break;
        }
        if( res != FR_OK ) goto formatExit2;

        res = f_lseek( &dst, 0x8e0 );
        if( res != FR_OK ) goto formatExit2;

        res = f_write( &dst, sysArea, sizeof( sysArea ), &r );
        if( res != FR_OK ) goto formatExit2;

        res = f_lseek( &dst, 0x8f5 );
        if( res != FR_OK ) goto formatExit2;

        res = f_write( &dst, sysLabel, sizeof( sysLabel ), &r );
        if( res != FR_OK ) goto formatExit2;

        res = f_lseek( &dst, 256 * 16 * 2 * 80 - sizeof( zero ) );
        if( res != FR_OK ) goto formatExit2;

        res = f_write( &dst, zero, sizeof( zero ), &r );
        if( res != FR_OK ) goto formatExit2;


    formatExit2:
        f_close( &dst );

    formatExit1:
        ;
    }

    if( res != FR_OK )
    {
        Shell_MessageBox( "Error", "Cannot format", name, GetFatErrorMsg( res ) );
    }

    return result;
}

void Shell_SaveSnapshot()
{
    CPU_Stop();
    SystemBus_Write( 0xc00020, 0 );  // use bank 0

    byte specPortFe = SystemBus_Read( 0xc00016 );
    byte specPort7ffd = SystemBus_Read( 0xc00017 );

    byte page = ( specPort7ffd & ( 1 << 3 ) ) != 0 ? 7 : 5;

    dword src = 0x800000 | ( page << 13 );
    dword dst = 0x800000 | ( VIDEO_PAGE << 13 );

    word data;

    for( int i = 0x0000; i < 0x1b00; i += 2 )
    {
        data = SystemBus_Read( src++ );
        SystemBus_Write( dst++, data );
    }

    SystemBus_Write( 0xc00021, 0x8000 | VIDEO_PAGE );               // Enable shell videopage
    SystemBus_Write( 0xc00022, 0x8000 | ( specPortFe & 0x07 ) );    // Enable shell border

    CString name = UpdateSnaName();
    bool result = Shell_InputBox( "Save snapshot", "Enter name :", name );

    SystemBus_Write( 0xc00021, 0 ); //armVideoPage
    SystemBus_Write( 0xc00022, 0 ); //armBorder

    if( result )
    {
        sniprintf( specConfig.snaName, sizeof( specConfig.snaName ), "%s", name.String() );
        SaveSnapshot( specConfig.snaName );
    }

    CPU_Start();
}

bool Shell_Browser()
{
    bool nextWindow = false;

    CPU_Stop();

    SystemBus_Write( 0xc00020, 0 );  // use bank 0

    InitScreen();

    read_dir();
    show_sel( true );

    while( true )
    {
        byte key = GetKey();

        FRECORD fr;
        Read( &fr, table_buffer, files_sel );

        sniprintf( files_lastName, sizeof( files_lastName ), "%s", fr.name );

		if ( ( key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT ) && files_size > 0 )
		{
			hide_sel();

			if ( key == K_LEFT ) files_sel -= FILES_PER_ROW;
			else if ( key == K_RIGHT ) files_sel += FILES_PER_ROW;
			else if ( key == K_UP ) files_sel--;
			else if ( key == K_DOWN ) files_sel++;

			show_sel();
		}
		else if ( key == ' ' && files_size > 0 )
		{
            if( strcmp( fr.name, ".." ) != 0 )
            {
                if( fr.sel ) files_sel_number--;
                else files_sel_number++;

                fr.sel = ( fr.sel + 1 ) % 2;
                Write( &fr, table_buffer, files_sel );
            }

			hide_sel();
			files_sel++;
			show_sel();
		}
		else if ( ( key == '+' || key == '=' || key == '-' || key == '/' || key == '\\' ) && files_size > 0 )
		{
			hide_sel();
			files_sel_number = 0;

		    for( int i = 0; i < files_size; i++ )
		    {
                Read( &fr, table_buffer, i );

                if( strcmp( fr.name, ".." ) != 0 )
                {
                    if( key == '+' || key == '=' ) fr.sel = 1;
                    else if( key == '-' ) fr.sel = 0;
                    else if( key == '/' || key == '\\' ) fr.sel = ( fr.sel + 1 ) % 2;

                    if( fr.sel ) files_sel_number++;
                    Write( &fr, table_buffer, i );
                }

                if( ( i & 0x3f ) == 0 ) CycleMark();
		    }

			show_sel( true );
		}
        else if( key == K_ESC ) break;
        else if( key == '[' || key == ']' )
        {
            static byte testVideo = 0;

            if( key == '[' ) testVideo++;
            else testVideo--;

            SystemBus_Write( 0xc00049, testVideo & 0x07 );
        }
        else if( key >= '1' && key <= '4' )
        {
			if ( ( fr.attr & AM_DIR ) == 0 )
			{
			    char fullName[ PATH_SIZE ];
			    sniprintf( fullName, sizeof( fullName ), "%s%s", path, fr.name );

                strlwr( fr.name );

                char *ext = fr.name + strlen( fr.name );
                while( ext > fr.name && *ext != '.' ) ext--;

				if ( strcmp( ext, ".trd" ) == 0 || strcmp( ext, ".fdi" ) == 0 || strcmp( ext, ".scl" ) == 0 )
				{
				    int i = key - '1';

				    if( fdc_open_image( i, fullName ) )
				    {
				        floppy_disk_wp( i, &specConfig.specImages[i].readOnly );

				        strcpy( specConfig.specImages[ key - '1' ].name, fullName );
				        SaveConfig();
				    }
				}
			}
        }
        else if ( key == K_BACKSPACE )
        {
            hide_sel();
            leave_dir();
            show_sel();
        }
        else if ( key == K_F1 )
        {
            WriteStr( 0, FILES_PER_ROW + 4, "speccy2010.hlp", 32 );
            Shell_Viewer( "speccy2010.hlp" );

            show_sel( true );
        }
        else if ( key == K_F2 )
        {
            sniprintf( destinationPath, sizeof( destinationPath ), "/%s", path );
        }
        else if ( key == K_F3 )
        {
            if ( ( fr.attr & AM_DIR ) == 0 )
            {
                char fullName[ PATH_SIZE ];
                sniprintf( fullName, sizeof( fullName ), "%s%s", path, fr.name );
                Shell_Viewer( fullName );

                show_sel( true );
            }
        }
        else if ( key == K_F4 )
        {
            hide_sel();
            if( Shell_EmptyTrd( "" ) ) read_dir();;
            show_sel( true );
        }
        else if ( key == K_F5 )
        {
            hide_sel();
            if( Shell_Copy( fr.name, false ) ) read_dir();
            show_sel( true );
        }
        else if ( key == K_F6 )
        {
            hide_sel();
            if( Shell_Copy( fr.name, true ) ) read_dir();
            show_sel( true );
        }
        else if ( key == K_F7 )
        {
            hide_sel();
            if( Shell_CreateFolder() ) read_dir();
            show_sel( true );
        }
        else if ( key == K_F8 )
        {
            hide_sel();

            if( Shell_Delete( fr.name ) )
            {
                int last_files_sel = files_sel;
                read_dir();
                files_sel = last_files_sel;
            }

            show_sel( true );
        }
        else if ( key == K_F9 )
        {
            hide_sel();
            if( Shell_EmptyTrd( fr.name ) ) read_dir();;
            show_sel( true );
        }
		else if ( key == K_RETURN )
		{
			if ( ( fr.attr & AM_DIR ) != 0 )
			{
				hide_sel();

				if ( strcmp( fr.name, ".." ) == 0 )
				{
				    leave_dir();
				}
				else if ( strlen( path ) + strlen( fr.name ) + 1 < PATH_SIZE )
				{
				    strcpy( files_lastName, "" );

					strcat( path, fr.name );
					strcat( path, "/" );
					read_dir();
					sely = (( files_sel - files_table_start ) % FILES_PER_ROW );
					selx = (( files_sel - files_table_start ) / FILES_PER_ROW );

					show_table();
				}

				show_sel();
			}
			else
			{
			    char fullName[ PATH_SIZE ];
			    sniprintf( fullName, sizeof( fullName ), "%s%s", path, fr.name );

                strlwr( fr.name );

                char *ext = fr.name + strlen( fr.name );
                while( ext > fr.name && *ext != '.' ) ext--;

				if ( strcmp( ext, ".tap" ) == 0 || strcmp( ext, ".tzx" ) == 0 )
				{
				    sniprintf( specConfig.snaName, sizeof( specConfig.snaName ), "/%s.00.sna", fullName );

				    Tape_SelectFile( fullName );
				    break;
				}
				else if ( strcmp( ext, ".sna" ) == 0 )
				{
                    sniprintf( specConfig.snaName, sizeof( specConfig.snaName ), "/%s", fullName );

                    if( LoadSnapshot( fullName ) )
                    {
                        break;
                    }
                }
				else if ( strcmp( ext, ".trd" ) == 0 || strcmp( ext, ".fdi" ) == 0 || strcmp( ext, ".scl" ) == 0 )
				{
				    if( fdc_open_image( 0, fullName ) )
				    {
				        sniprintf( specConfig.snaName, sizeof( specConfig.snaName ), "/%s.00.sna", fullName );

				        floppy_disk_wp( 0, &specConfig.specImages[0].readOnly );

				        strcpy( specConfig.specImages[ 0 ].name, fullName );
				        SaveConfig();

                        CPU_Start();
                        CPU_Reset( true );
                        CPU_Reset( false );
                        CPU_Stop();

                        CPU_ModifyPC( 0, 0 );

                        if( specConfig.specRom == SpecRom_Classic48 ) SystemBus_Write( 0xc00017, ( 1 << 4 ) | ( 1 << 5 ) );
                        else SystemBus_Write( 0xc00017, ( 1 << 4 ) );

                        SystemBus_Write( 0xc00018, 0x0001 );

                        break;
				    }
				}
				else if ( strcmp( ext, ".scr" ) == 0 )
				{
				    while( true )
				    {
                        FIL image;
                        if( f_open( &image, fullName, FA_READ ) == FR_OK )
                        {
                            dword addr = VIDEO_PAGE_PTR;

                            for( dword pos = 0; pos < 0x1b00; pos++ )
                            {
                                byte data;

                                UINT res;
                                if( f_read( &image, &data, 1, &res ) != FR_OK ) break;
                                if( res == 0 ) break;

                                SystemBus_Write( addr++, data );
                            }
                        }
                        else break;

                        byte key = GetKey();

                        if( key == ' ' )
                        {
                            while( true )
                            {
                                files_sel++;
                                if( files_sel >= files_size ) files_sel = 0;

                                Read( &fr, table_buffer, files_sel );
                                strlwr( fr.name );

                                ext = fr.name + strlen( fr.name );
                                while( ext > fr.name && *ext != '.' ) ext--;

                                if ( strcmp( ext, ".scr" ) == 0 ) break;
                            }

                            sniprintf( fullName, sizeof( fullName ), "%s%s", path, fr.name );
                        }
                        else if( key != 0 ) break;
				    }

                    InitScreen();
                    show_table();
                    show_sel();
				}
			}
		}
    }

    SystemBus_Write( 0xc00021, 0 );            // Enable Video
    SystemBus_Write( 0xc00022, 0 );

    CPU_Start();

    return nextWindow;
}

CMenuItem::CMenuItem( int _x, int _y, const char *_name, const char *_data )
{
    x = _x;
    y = _y;
    name = _name;

    data = _data;
    int maxLen = 32 - ( x + strlen(name) );
    if( data.Length() > maxLen )
    {
        data.Delete( 0, 1 + data.Length() - maxLen );
        data.Insert( 0, "~" );
    }

    state = 0;
    param = 0;
}

CMenuItem::CMenuItem( int _x, int _y, const char *_name, const CParameter *_param )
{
    x = _x;
    y = _y;

    param = _param;
    name = _name;
    state = 0;
}

void CMenuItem::Redraw()
{
    WriteStr( x, y, name );
    WriteAttr( x, y, 0x07, strlen( name ) );

    int x2 = x + strlen( name );
    int size = strlen( data );

    WriteStr( x2, y, data );
    if( state == 0 ) WriteAttr( x2, y, 0x47, size );
    else if( state == 1 ) WriteAttr( x2, y, 0x39, size );
    else WriteAttr( x2, y, 0x40 | 0x10 | 0x00, size );
}

void CMenuItem::UpdateData()
{
    if( param != 0 )
    {
        int delNumber = data.Length();

        param->GetValueText( data );
        int maxLen = 32 - ( x + strlen(name) );
        if( data.Length() > maxLen )
        {
            data.Delete( 0, 1 + data.Length() - maxLen );
            data.Insert( 0, "~" );
        }

        delNumber -= data.Length();

        Redraw();

        if( delNumber > 0 )
        {
            int x2 = x + strlen( name ) + strlen( data );
            WriteStr( x2, y, "", delNumber );
            WriteAttr( x2, y, 0x47, delNumber );
        }
    }
}

void CMenuItem::UpdateData( const char *_data )
{
    if( strncmp( data, _data, strlen( _data ) ) != 0 )
    {
        int delNumber = strlen( data ) - strlen( _data );

        data = _data;
        Redraw();

        if( delNumber > 0 )
        {
            int x2 = x + strlen( name ) + strlen( data );
            WriteStr( x2, y, "", delNumber );
            WriteAttr( x2, y, 0x47, delNumber );
        }
    }
}

void CMenuItem::UpdateState( int _state )
{
    if( state != _state )
    {
        state = _state;

        int x2 = x + strlen( name );
        int size = strlen( data );

        if( state == 0 ) WriteAttr( x2, y, 0x47, size );
        else if( state == 1 ) WriteAttr( x2, y, 0x39, size );
        else WriteAttr( x2, y, 0x40 | 0x10 | 0x00, size );
    }
}

CMenuItem mainMenu[] = {
    CMenuItem( 1, 2, "Date: ", "00" ),
    CMenuItem( 9, 2, ".", "00" ),
    CMenuItem( 12, 2, ".", "00" ),

    CMenuItem( 1, 3, "Time: ", "00" ),
    CMenuItem( 9, 3, ":", "00" ),
    CMenuItem( 12, 3, ":", "0000" ),

    CMenuItem( 1, 5, "ROM/RAM: ", GetParam( iniParameters, "ROM/RAM" ) ),
    CMenuItem( 1, 6, "Timings: ", GetParam( iniParameters, "Timings" ) ),
    CMenuItem( 1, 7, "Turbo: ", GetParam( iniParameters, "Turbo" ) ),
    CMenuItem( 1, 8, "AY mode: ", GetParam( iniParameters, "AY mode" ) ),
    CMenuItem( 1, 9, "BDI mode: ", GetParam( iniParameters, "BDI mode" ) ),

    CMenuItem( 1, 11, "Joystick emulation: ", GetParam( iniParameters, "Joystick emulation" ) ),
    CMenuItem( 1, 12, "Joystick 1: ", GetParam( iniParameters, "Joystick 1" ) ),
    CMenuItem( 1, 13, "Joystick 2: ", GetParam( iniParameters, "Joystick 2" ) ),
    CMenuItem( 1, 14, "Mouse Sensitivity: ", GetParam( iniParameters, "Mouse Sensitivity" ) ),

    CMenuItem( 1, 16, "Video mode: ", GetParam( iniParameters, "Video mode" ) ),
    CMenuItem( 1, 17, "Video aspect ratio: ", GetParam( iniParameters, "Video aspect ratio" ) ),
    CMenuItem( 1, 18, "Audio DAC mode: ", GetParam( iniParameters, "Audio DAC mode" ) ),
    CMenuItem( 1, 19, "Font: ", GetParam( iniParameters, "Font" ) ),
};

bool Shell_SettingsMenu()
{
    return Shell_Menu( mainMenu, sizeof( mainMenu ) / sizeof( CMenuItem ) );
}

CMenuItem disksMenu[] = {
    CMenuItem( 1, 3, "A: ", GetParam( iniParameters, "Disk A" ) ),
    CMenuItem( 3, 4, "read only :", GetParam( iniParameters, "Disk A read only" ) ),
    CMenuItem( 1, 6, "B: ", GetParam( iniParameters, "Disk B" ) ),
    CMenuItem( 3, 7, "read only :", GetParam( iniParameters, "Disk B read only" ) ),
    CMenuItem( 1, 9, "C: ", GetParam( iniParameters, "Disk C" ) ),
    CMenuItem( 3, 10, "read only :", GetParam( iniParameters, "Disk C read only" ) ),
    CMenuItem( 1, 12, "D: ", GetParam( iniParameters, "Disk D" ) ),
    CMenuItem( 3, 13, "read only :", GetParam( iniParameters, "Disk D read only" ) ),
};

bool Shell_DisksMenu()
{
    bool result = Shell_Menu( disksMenu, sizeof( disksMenu ) / sizeof( CMenuItem ) );
    Spectrum_UpdateDisks();

    return result;
}

bool Shell_Menu( CMenuItem *menu, int menuSize )
{
    bool nextWindow = false;

    static tm time;
    static dword tickCounter = 0;

    CPU_Stop();

    InitScreen();

    for( int i = 0; i < menuSize; i++ )
    {
        menu[i].UpdateData();
        menu[i].UpdateState( 0 );
        menu[i].Redraw();
    }

    int menuPos = 0;
    if( menu == mainMenu ) menuPos = 6;

    menu[ menuPos ].UpdateState( 1 );
    bool editMode = false;

    while( true )
    {
        byte key = GetKey( false );

        if( !editMode )
        {
            if ( key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT )
            {
                menu[ menuPos ].UpdateState( 0 );

                if ( key == K_LEFT || key == K_UP ) menuPos = ( menuPos - 1 +  menuSize ) % menuSize;
                else menuPos = ( menuPos + 1 ) % menuSize;

                menu[ menuPos ].UpdateState( 1 );
            }
        }
        else
        {
            if ( key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT )
            {
                int delta;
                if ( key == K_LEFT || key == K_DOWN ) delta = -1;
                else delta = 1;

                if( menu == mainMenu && menuPos >= 0 && menuPos < 6 )
                {
                    if( menuPos == 0 ) time.tm_mday += delta;
                    if( menuPos == 1 ) time.tm_mon += delta;
                    if( menuPos == 2 ) time.tm_year += delta;
                    if( menuPos == 3 ) time.tm_hour += delta;
                    if( menuPos == 4 ) time.tm_min += delta;
                    if( menuPos == 5 ) time.tm_sec += delta;

                    RTC_SetTime( &time );
                    tickCounter = 0;
                }
                else
                {
                    const CParameter *param = menu[ menuPos ].GetParam();
                    if( param != 0 )
                    {
                        if( param->GetType() == PTYPE_INT )
                        {
                            int value = param->GetValue() + delta;
                            if( value < param->GetValueMin() ) value = param->GetValueMin();
                            if( value > param->GetValueMax() ) value = param->GetValueMax();
                            param->SetValue( value );

                            menu[ menuPos ].UpdateData();
                        }
                    }
                }
            }
        }
        if ( key == K_RETURN )
        {
            if( menu == mainMenu && menuPos >= 0 && menuPos < 6 )
            {
                editMode = !editMode;

                if( editMode ) menu[ menuPos ].UpdateState( 2 );
                else menu[ menuPos ].UpdateState( 1 );
            }
            else
            {
                const CParameter *param = menu[ menuPos ].GetParam();
                if( param != 0 )
                {
                    if( param->GetType() == PTYPE_LIST )
                    {
                        param->SetValue( ( param->GetValue() + 1 ) % ( param->GetValueMax() + 1 ) );

                        if( param == GetParam( iniParameters, "Font" ) )
                        {
                            InitScreen();
                            for( int i = 0; i < menuSize; i++ ) menu[i].Redraw();
                        }

                        menu[ menuPos ].UpdateData();
                    }
                    else if( param->GetType() == PTYPE_INT )
                    {
                        editMode = !editMode;

                        if( editMode ) menu[ menuPos ].UpdateState( 2 );
                        else menu[ menuPos ].UpdateState( 1 );
                    }
                }
            }
        }
        else if( key == K_BACKSPACE )
        {
            const CParameter *param = menu[ menuPos ].GetParam();
            if( param != 0 )
            {
                if( param->GetType() == PTYPE_STRING )
                {
                    param->SetValueText( "" );
                    menu[ menuPos ].UpdateData();
                }
            }
        }
        else if( key == K_ESC ) break;

        //------------------------------------------------------------------------

        if( menu == mainMenu && (dword) ( Timer_GetTickCounter() - tickCounter ) >= 100 )
        {
            RTC_GetTime( &time );
            char temp[ 5 ];

            sniprintf( temp, sizeof( temp ), "%.2d", time.tm_sec );
            mainMenu[5].UpdateData( temp );
            sniprintf( temp, sizeof( temp ), "%.2d", time.tm_min );
            mainMenu[4].UpdateData( temp );
            sniprintf( temp, sizeof( temp ), "%.2d", time.tm_hour );
            mainMenu[3].UpdateData( temp );

            sniprintf( temp, sizeof( temp ), "%.2d", time.tm_mday );
            mainMenu[0].UpdateData( temp );
            sniprintf( temp, sizeof( temp ), "%.2d", time.tm_mon + 1 );
            mainMenu[1].UpdateData( temp );
            sniprintf( temp, sizeof( temp ), "%.4d", time.tm_year + 1900 );
            mainMenu[2].UpdateData( temp );

            tickCounter = Timer_GetTickCounter();
        }
    }

    SaveConfig();

    SystemBus_Write( 0xc00021, 0 );            // Enable Video
    SystemBus_Write( 0xc00022, 0 );

    CPU_Start();
    Spectrum_UpdateConfig();

    return nextWindow;
}

//============================================================================================

void Debugger_Screen0() //disasm
{
    ClrScr( 0x07 );

    char str[ 0x20 ];

    word pc = SystemBus_Read( 0xc00001 );
    sniprintf( str, sizeof(str), "pc #%.4x", pc );
    WriteStr( 0, 1, str );
}

dword CalculateAddrAtCursor(word addr, byte CurX, byte CurY)
{
    byte specPort7ffd = SystemBus_Read( 0xC00017 );
    byte specTrdosFlag = SystemBus_Read( 0xC00018 );

    byte ROMpage = 0;
    if( ( specPort7ffd & 0x10 ) != 0 ) ROMpage |= 0x01;
    if( ( specTrdosFlag & 0x01 ) == 0 ) ROMpage |= 0x02;

    byte RAMpage = specPort7ffd & 0b00000111;

    dword lineAddr = ( ( addr & 0xFFF8 ) + ( CurY * 8 ) + CurX );
    int page;

    if( lineAddr < 0x4000 ) page = 0x100 + ROMpage;
    else if( lineAddr < 0x8000 ) page = 0x05;
    else if( lineAddr < 0xc000 ) page = 0x02;
    else page = RAMpage;

    return page * 0x4000 + ( lineAddr & 0x3fff );
}

byte ReadByteAtCursor(word addr, byte CurX, byte CurY)
{
    return (byte) SystemBus_Read( CalculateAddrAtCursor( addr, CurX, CurY ) );
}

void WriteByteAtCursor(word addr, byte data, byte CurX, byte CurY)
{
    SystemBus_Write( CalculateAddrAtCursor( addr, CurX, CurY ), data );
}

void Debugger_Screen1( word addr, byte CurX, byte CurY) //hex editor
{
    char str[ 0x20 ];
    for( int i = 0; i < 23; i++)
    {
        sniprintf( str, sizeof(str), "%.4X", (addr & 0xFFF8) + (i << 3) );
        WriteStr( 0, i, str );
        for( int j = 0; j < 8; j++)
        {
            sniprintf( str, sizeof(str), "%.2X ", ReadByteAtCursor( addr, j, i ) );
            WriteStr( 5 + j * 3, i, str );
        }
    }
}

void Debugger_Enter()
{
    CPU_Stop();
    SystemBus_Write( 0xc00020, 0 );  // use bank 0

    ClrScr( 0x07 );
    SystemBus_Write( 0xc00021, 0x8000 | VIDEO_PAGE );       // Enable shell videopage
    SystemBus_Write( 0xc00022, 0x8000 | 0 );                // Enable shell border

    char str[ 0x20 ];
    byte CurX = 0, CurY = 0, CurXold, CurYold, half = 0, editAddr = 0, editAddrPos = 3;

    word addr = SystemBus_Read( 0xC00001 );
    CurX = addr & 0x0007;

    Debugger_Screen1( addr, CurX, CurY );
    WriteAttr( 5 + CurX * 3, CurY, 0x17, 2 );

    while( true )
    {
        byte key = GetKey( true );

        if( key == K_ESC ) break;

        CurXold = CurX;
        CurYold = CurY;

        if( !editAddr )
        {
            if( key == K_UP ) //KEY_UP
            {
                half = 0;
                if( CurY == 0)
                    if( addr > 8 ) addr -= 8;
                    else addr = 0;
                else
                    CurY--;
            }
            if( key == K_DOWN ) //KEY_DOWN
            {
                half = 0;
                if( CurY == 22)
                    if( addr < 0xFF48 ) addr += 8;
                    else addr = 0xFF48;
                else
                    CurY++;
            }
            if( key == K_LEFT ) //KEY_LEFT
            {
                half = 0;
                if( CurX > 0 ) CurX--;
            }
            if( key == K_RIGHT ) //KEY_RIGHT
            {
                half = 0;
                if( CurX < 7 ) CurX++;
            }
        }
        if( (key >= '0') && (key <='9') )
        {
            if( !editAddr)
            {
                if( !half )
                {
                    WriteByteAtCursor( addr, ( ReadByteAtCursor( addr, CurX, CurY ) & 0x0F) | (( key - '0' ) << 4), CurX, CurY );
                    half = 1;
                }
                else
                {
                    WriteByteAtCursor( addr, ( ReadByteAtCursor( addr, CurX, CurY ) & 0xF0) | ( key - '0' ), CurX, CurY );
                    half = 0;
                    if( CurX < 7 )
                        CurX++;
                    else
                    {
                        CurX = 0;
                        if( CurY == 22)
                            if( addr < 0xFF48 ) addr += 8;
                            else addr = 0xFF48;
                        else
                            CurY++;
                    }
                }
            }
            else
            {
                addr = ( addr & ~(0x0F << ( editAddrPos * 4) ) ) | ( ( key - '0' ) << ( editAddrPos * 4) );
                editAddrPos--;
                if( editAddrPos == 0xFF)
                {
                    if( addr > 0xFF48 ) addr = 0xFF48;
                    editAddr = 0;
                    editAddrPos = 3;
                    WriteAttr( 0, 0, 0x07, 4);
                    WriteAttr( 5 + CurX * 3, CurY, 0x17, 2 );
                }
            }
        }
        if( (key >= 'a') && (key <='f') )
        {
            if( !editAddr )
            {
                if( !half )
                {
                    WriteByteAtCursor( addr, ( ReadByteAtCursor( addr, CurX, CurY ) & 0x0F) | (( key - 'a' + 0x0A ) << 4), CurX, CurY );
                    half = 1;
                }
                else
                {
                    WriteByteAtCursor( addr, ( ReadByteAtCursor( addr, CurX, CurY ) & 0xF0) | ( key - 'a' + 0x0A ), CurX, CurY );
                    half = 0;
                    if( CurX < 7 )
                        CurX++;
                    else
                    {
                        CurX = 0;
                        if( CurY == 22)
                            if( addr < 0xFF48 ) addr += 8;
                            else addr = 0xFF48;
                        else
                            CurY++;
                    }
                }
            }
            else
            {
                addr = ( addr & ~(0x0F << ( editAddrPos * 4) ) ) | ( ( key - 'a' + 0x0A ) << ( editAddrPos * 4) );
                editAddrPos--;
                if( editAddrPos == 0xFF)
                {
                    if( addr > 0xFF48 ) addr = 0xFF48;
                    editAddr = 0;
                    editAddrPos = 3;
                    WriteAttr( 0, 0, 0x07, 4);
                    WriteAttr( 5 + CurX * 3, CurY, 0x17, 2 );
                }
            }
        }
        if( key == 'z' )  //one step
        {
            SystemBus_Write( 0xc00008, 0x01 );
            DelayMs( 1 );
            BDI_Routine();

            addr = SystemBus_Read( 0xC00001 );
            CurX = addr & 0x0007;
            CurY = 0;
        }
        if( key == 's' ) SaveSnapshot( UpdateSnaName() );
        if( key == 'm' ) editAddr = 1;
        if( key != 0 )
        {
            if( editAddr )
            {
                sniprintf( str, sizeof(str), "%.4X", addr & 0xFFF8);
                WriteStr( 0, 0, str );
                WriteAttr( 0, 0, 0x17, 4);
                WriteAttr( 3 - editAddrPos, 0, 0x57, 1);
            }
            else
                Debugger_Screen1( addr, CurX, CurY);
                WriteAttr( 5 + CurXold * 3, CurYold, 0x07, 2 );
                WriteAttr( 5 + CurX * 3, CurY, 0x17, 2 );
        }
    }

    DelayMs( 100 );

    CPU_Start();

    SystemBus_Write( 0xc00021, 0 ); //armVideoPage
    SystemBus_Write( 0xc00022, 0 ); //armBorder
}
