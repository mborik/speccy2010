#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>

#include "system.h"
#include "uarts.h"
#include "fifo.h"

#include "specTape.h"
#include "specKeyboard.h"
#include "specChars.h"
#include "specShell.h"
#include "specConfig.h"
#include "specSnapshot.h"
#include "specBetadisk.h"

#include "string.h"


void WriteDisplay( word address, byte data )
{
    SystemBus_Write( 0x420000 + address, data );
}

void ClrScr( byte attr )
{
    SystemBus_Write( 0xc00021, 0x8000 | 0x108 );            // Enable Video
    SystemBus_Write( 0xc00022, 0x8000 | ( ( attr >> 3 ) & 0x03 ) );

    int i = 0;
    for( ; i < 0x1800; i++ ) SystemBus_Write( 0x420000 + i, 0 );
    for( ; i < 0x1b00; i++ ) SystemBus_Write( 0x420000 + i, attr );
}

void WriteChar( byte x, byte y, char c )
{
    if( x < 32 && y < 24 )
    {
        if( (byte) c < 0x20 && (byte) c >= 0x80 ) c = 0;
        else c -= 0x20;

        word address = x + ( y & 0x07 ) * 32 + ( y & 0x18 ) * 32 * 8;
        const byte *tablePos = &specChars[ (byte) c * 8 ];

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

byte GetKey( bool wait = true )
{
    static word prevKey = KEY_NONE;
    static dword keyRepeatTime = 0;

    word keyCode, keyFlags;

    while( true )
    {
        Timer_Routine();

        while( ReadKey( keyCode, keyFlags ) )
        {
            if( ( keyFlags & fKeyRelease ) == 0 )
            {
                prevKey = keyCode;
                keyRepeatTime = Timer_GetTickCounter() + 50;
                break;
            }
            else
            {
                prevKey = KEY_NONE;
            }

            keyCode = KEY_NONE;
        }

        if( keyCode != KEY_NONE ) break;

        if( (int) ( Timer_GetTickCounter() - keyRepeatTime ) >= 0 )
        {
            keyCode = prevKey;
            keyRepeatTime = Timer_GetTickCounter() + 5;
            break;
        }

        if( !wait ) break;
    }

    if( keyCode != KEY_NONE ) Beep();

    switch( keyCode )
    {
        case KEY_ESC : return 0x1b;
        case KEY_F12 : return 0x1c;

//        case KEY_LCTRL : return 0x0d; // !!! зачем это !!!
        case KEY_RETURN : return 0x0d;
        case KEY_SPACE : return ' ';

        case KEY_UP : return 0x0b;
        case KEY_DOWN : return 0x0a;
        case KEY_LEFT : return 0x08;
        case KEY_RIGHT : return 0x09;

        case KEY_LEFTBRACKET : return '[';
        case KEY_RIGHTBRACKET : return ']';

        case KEY_0 : return '0'; // западло бля !
        case KEY_1 : return '1';
        case KEY_2 : return '2';
        case KEY_3 : return '3';
        case KEY_4 : return '4';
        case KEY_5 : return '5';
        case KEY_6 : return '6';
        case KEY_7 : return '7';
        case KEY_8 : return '8';
        case KEY_9 : return '9';
        case KEY_A : return 'a';
        case KEY_B : return 'b';
        case KEY_C : return 'c';
        case KEY_D : return 'd';
        case KEY_E : return 'e';
        case KEY_F : return 'f';
        case KEY_G : return 'g';
        case KEY_H : return 'h';
        case KEY_I : return 'i';
        case KEY_J : return 'j';
        case KEY_K : return 'k';
        case KEY_L : return 'l';
        case KEY_M : return 'm';
        case KEY_N : return 'n';
        case KEY_O : return 'o';
        case KEY_P : return 'p';
        case KEY_Q : return 'q';
        case KEY_R : return 'r';
        case KEY_S : return 's';
        case KEY_T : return 't';
        case KEY_U : return 'u';
        case KEY_V : return 'v';
        case KEY_W : return 'w';
        case KEY_X : return 'x';
        case KEY_Y : return 'y';
        case KEY_Z : return 'z';
    }

    return 0;
}


const int FILES_PER_ROW = 16;

struct FRECORD
{
	byte attr;
	char name[ _MAX_LFN + 1 ];
};


void WriteRecord( FRECORD &rec, int i )
{
    dword addr = 0x800000 | ( 0x220000 + ( ( sizeof( FRECORD ) + 1 ) / 2 ) * i );
    byte *ptr = ( byte* ) &rec;

    for( word j = 0; j < sizeof( FRECORD ); j += 2 )
    {
        SystemBus_Write( addr, ptr[0] | ( ptr[1] << 8 ) );

        addr++;
        ptr += 2;
    }
}

void ReadRecord( FRECORD &rec, int i )
{
    dword addr = 0x800000 | ( 0x220000 + ( ( sizeof( FRECORD ) + 1 ) / 2 ) * i );
    byte *ptr = ( byte* ) &rec;

    for( word j = 0; j < sizeof( FRECORD ); j += 2 )
    {
        word data = SystemBus_Read( addr );

        addr++;
        *ptr++ = (byte) data;
        *ptr++ = (byte) ( data >> 8 );
    }
}

const int FILES_SIZE = 0x2000;
//const int FILES_SIZE = ( 0x400000 - 0x201000 ) / ( sizeof( FRECORD ) / 2 );

char path[ PATH_SIZE ] = "";

int files_size = 0;
int files_table_start;
int files_sel;
char files_lastName[ _MAX_LFN + 1 ] = "";

byte selx = 0, sely = 0;

byte comp_name( int a, int b )
{
    FRECORD ra, rb;
    ReadRecord( ra, a );
    ReadRecord( rb, b );

    strlwr( ra.name );
    strlwr( rb.name );

	if (( ra.attr & AM_DIR ) && !( rb.attr & AM_DIR ) ) return true;
	else if ( !( ra.attr & AM_DIR ) && ( rb.attr & AM_DIR ) ) return false;
	else return strcmp( ra.name, rb.name ) <= 0;
}

void swap_name( int a, int b )
{
    FRECORD ra, rb;
    ReadRecord( ra, a );
    ReadRecord( rb, b );

    WriteRecord( ra, b );
    WriteRecord( rb, a );
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

void read_dir()
{
	files_size = 0;
	files_table_start = 0;
	files_sel = 0;

    FRECORD fr;

    if( strlen( path ) != 0 )
    {
        fr.attr = AM_DIR;
        strcpy( fr.name, ".." );
        WriteRecord( fr, files_size++ );
    }

    DIR dir;
	FRESULT r;

	int pathSize = strlen( path );
	if( pathSize > 0 ) path[ pathSize - 1 ]	= 0;

	r = f_opendir( &dir, path );
	if( pathSize > 0 ) path[ pathSize - 1 ]	= '/';

    while ( r == FR_OK && files_size < FILES_SIZE )
    {
        FILINFO fi;
        char lfn[ _MAX_LFN + 1 ];
        fi.lfname = lfn;
        fi.lfsize = sizeof( lfn );

        r = f_readdir( &dir, &fi );

        if ( r != FR_OK || fi.fname[0] == 0 ) break;
        if ( fi.fattrib & ( AM_HID | AM_SYS ) ) continue;

        if( lfn[0] == 0 ) strcpy( lfn, fi.fname );

        //if ( fi.fattrib & ( AM_DIR ) ) strupr( lfn );
        //else strlwr( lfn );

        fr.attr = fi.fattrib;
        strcpy( fr.name, lfn );
        WriteRecord( fr, files_size );

        files_size++;
        WDT_Kick();
    }

    if( files_size > 0 && files_size < 0x100 ) qsort( 0, files_size - 1 );

    if( strlen( files_lastName ) != 0 )
    {
        FRECORD fr;

        for ( int i = 0; i < files_size; i++ )
        {
            ReadRecord( fr, i );
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

    return result;
}

void show_table()
{
	byte i;
	display_path( path, 0, 19, 32 );

    FRECORD fr;

	for ( i = 0; i < FILES_PER_ROW * 2; i++ )
	{
		int col = ( i / FILES_PER_ROW ) * 16 + 1;
		int row = ( i % FILES_PER_ROW ) + 2;
		int pos = i + files_table_start;

        ReadRecord( fr, pos );

		char sname[ 16 ];
		make_short_name( sname, sizeof( sname ), fr.name );

		//if( ( fr.attr & AM_DIR ) == 0 ) strlwr( sname );
		//else strupr( sname );

		if ( pos < files_size )
		{
		    WriteStr( col, row, sname, 15 );
	    	WriteAttr( col, row, get_sel_attr( fr ), 15 );
		}
		else WriteStr( col, row, "", 15 );
	}

	if ( !files_size )
	{
	    WriteStr( 10, 4, "no files !", 0 );
		WriteAttr( 10, 4, 0102, 10 );
	}
}

bool calc_sel()
{
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
    ReadRecord( fr, files_sel );

	WriteAttr( selx * 16, 2 + sely, get_sel_attr( fr ), 16 );
	WriteStr( 0, 20, "", 32 );
}

void show_sel( bool redraw = false )
{
	if ( files_size <= 0 ) return;

	if( calc_sel() || redraw ) show_table();
	WriteAttr( selx * 16, 2 + sely, 071, 16 );

    FRECORD fr;
    ReadRecord( fr, files_sel );

	char sname[ 32 + 1 ];
	make_short_name( sname, sizeof( sname ), fr.name );
	WriteStr( 0, 20, sname, 32 );
}

void InitScreen()
{
    ClrScr( 0x07 );

    char str[33];
    sniprintf( str, sizeof(str), " -= Speccy2010, v%d.%.2d, r%.4d =-\n", VER_MAJOR, VER_MINIR, REV );


    WriteStr( 0, 0, str );
    WriteAttr( 0, 0, 0x44, strlen( str ) );

    WriteAttr( 0, 1, 0x06, 32 );
    WriteLine( 1, 3 );
    WriteLine( 1, 5 );

    WriteAttr( 0, 18, 0x06, 32 );
    WriteLine( 18, 3 );
    WriteLine( 18, 5 );
}

int cpuStopNesting = 0;

void CPU_Start()
{
    if( cpuStopNesting > 0 )
    {
        if( cpuStopNesting == 1 )
        {
            //ResetKeyboard();
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
        ReadRecord( fr, files_sel );

        sniprintf( files_lastName, sizeof( files_lastName ), "%s", fr.name );

		if ( key >= 0x08 && key <= 0x0b && files_size > 0 )
		{
			hide_sel();

			if ( key == 0x08 ) files_sel -= FILES_PER_ROW;
			else if ( key == 0x09 ) files_sel += FILES_PER_ROW;
			else if ( key == 0x0a ) files_sel++;
			else if ( key == 0x0b ) files_sel--;

			show_sel();
		}
        else if( key == 0x1b ) break;
        else if( key == 0x1c )
        {
            nextWindow = true;
            break;
        }
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
				    if( fdc_open_image( key - '1', fullName ) )
				    {
				        strcpy( specConfig.specImages[ key - '1' ].name, fullName );
				        SaveConfig();
				    }
				}
			}
        }
		else if ( key == 0x0d )
		{
			if (( fr.attr & AM_DIR ) )
			{
				hide_sel();

				if ( strcmp( fr.name, ".." ) == 0 )
				{
					byte i = strlen( path );
					char dir_name[ 13 ];

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
                            ReadRecord( fr, files_sel );
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
				else if ( strlen( path ) + strlen( fr.name ) + 1 < PATH_SIZE )
				{
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
				    Tape_SelectFile( fullName );
				    break;
				}
				else if ( strcmp( ext, ".sna" ) == 0 )
				{
                    //char str[ 0x20 ];
                    //sniprintf( str, sizeof(str), "oldPc - 0x%.4x\n", SystemBus_Read( 0xc00001 ) );
                    //UART0_WriteText( str );
                    //sniprintf( str, sizeof(str), "oldINT - 0x%.4x\n", SystemBus_Read( 0xc00002 ) );
                    //UART0_WriteText( str );

                    if( LoadSnapshot( fullName ) )
                    {
                        break;
                    }
                }
				else if ( strcmp( ext, ".trd" ) == 0 || strcmp( ext, ".fdi" ) == 0 || strcmp( ext, ".scl" ) == 0 )
				{
				    //if( fdc_open_image( 0, fullName, false, true ) )
				    if( fdc_open_image( 0, fullName ) )
				    {
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
                            dword addr = 0x420000;

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

                                ReadRecord( fr, files_sel );
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

    CPU_Start();

    return nextWindow;
}

class CMenuItem
{
    int x, y, state;

    const char *name;
    CString data;

    const CParameter *param;

public:
    CMenuItem( int _x, int _y, const char *_name, const CParameter *_param );
    CMenuItem( int _x, int _y, const char *_name, const char *_data );

    void Redraw();
    void UpdateData();
    void UpdateData( const char *_data );
    void UpdateState( int _state );

    const char *GetName() { return name; }
    const CParameter *GetParam() { return param; }

    void OnKey( byte key );
};

CMenuItem::CMenuItem( int _x, int _y, const char *_name, const char *_data )
{
    x = _x;
    y = _y;
    name = _name;
    data = _data;
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
    CMenuItem( 1, 3, "Date: ", "00" ),
    CMenuItem( 9, 3, ".", "00" ),
    CMenuItem( 12, 3, ".", "00" ),

    CMenuItem( 1, 4, "Time: ", "00" ),
    CMenuItem( 9, 4, ":", "00" ),
    CMenuItem( 12, 4, ":", "0000" ),

    CMenuItem( 1, 6, "ROM/RAM: ", GetParam( iniParameters, "ROM/RAM" ) ),
    CMenuItem( 1, 7, "Timings: ", GetParam( iniParameters, "Timings" ) ),
    CMenuItem( 1, 8, "Turbo: ", GetParam( iniParameters, "Turbo" ) ),
    CMenuItem( 1, 9, "AY mode: ", GetParam( iniParameters, "AY mode" ) ),
    CMenuItem( 1, 10, "BDI mode: ", GetParam( iniParameters, "BDI mode" ) ),

    CMenuItem( 1, 11, "Joystick emulation: ", GetParam( iniParameters, "Joystick emulation" ) ),
    CMenuItem( 1, 12, "Joystick 1: ", GetParam( iniParameters, "Joystick 1" ) ),
    CMenuItem( 1, 13, "Joystick 2: ", GetParam( iniParameters, "Joystick 2" ) ),
    CMenuItem( 1, 14, "Mouse Sensitivity: ", GetParam( iniParameters, "Mouse Sensitivity" ) ),

    CMenuItem( 1, 15, "Video mode: ", GetParam( iniParameters, "Video mode" ) ),
    CMenuItem( 1, 16, "Video aspect ratio: ", GetParam( iniParameters, "Video aspect ratio" ) ),
    CMenuItem( 1, 17, "Audio DAC mode: ", GetParam( iniParameters, "Audio DAC mode" ) ),
};

const int mainMenuSize = sizeof( mainMenu ) / sizeof( CMenuItem );

bool Shell_SettingsMenu()
{
    bool nextWindow = false;

    static tm time;
    static dword tickCounter = 0;

    CPU_Stop();

    InitScreen();

    for( int i = 0; i < mainMenuSize; i++ )
    {
        mainMenu[i].UpdateData();
        mainMenu[i].UpdateState( 0 );
        mainMenu[i].Redraw();
    }

    int menuPos = 6;
    mainMenu[ menuPos ].UpdateState( 1 );

    bool editMode = false;

    while( true )
    {
        byte key = GetKey( false );

        if( !editMode )
        {
            if ( key == 0x08 || key == 0x09 || key == 0x0a || key == 0x0b )
            {
                mainMenu[ menuPos ].UpdateState( 0 );

                if ( key == 0x08 || key == 0x0b ) menuPos = ( menuPos - 1 +  mainMenuSize ) % mainMenuSize;
                else menuPos = ( menuPos + 1 ) % mainMenuSize;

                mainMenu[ menuPos ].UpdateState( 1 );
            }
        }
        else
        {
            if ( key == 0x08 || key == 0x09 || key == 0x0a || key == 0x0b )
            {
                int delta;
                if ( key == 0x08 || key == 0x0a ) delta = -1;
                else delta = 1;

                if( menuPos >= 0 && menuPos < 6 )
                {
                    if( menuPos == 0 ) time.tm_mday += delta;
                    if( menuPos == 1 ) time.tm_mon += delta;
                    if( menuPos == 2 ) time.tm_year += delta;
                    if( menuPos == 3 ) time.tm_hour += delta;
                    if( menuPos == 4 ) time.tm_min += delta;
                    if( menuPos == 5 ) time.tm_sec += delta;

                    SetTime( &time );
                    tickCounter = 0;
                }
                else
                {
                    const CParameter *param = mainMenu[ menuPos ].GetParam();
                    if( param != 0 )
                    {
                        if( param->GetType() == PTYPE_INT )
                        {
                            int value = param->GetValue() + delta;
                            if( value < param->GetValueMin() ) value = param->GetValueMin();
                            if( value > param->GetValueMax() ) value = param->GetValueMax();
                            param->SetValue( value );

                            mainMenu[ menuPos ].UpdateData();
                        }
                    }
                }
            }
        }
        if ( key == 0x0d )
        {
            if( menuPos >= 0 && menuPos < 6 )
            {
                editMode = !editMode;

                if( editMode ) mainMenu[ menuPos ].UpdateState( 2 );
                else mainMenu[ menuPos ].UpdateState( 1 );
            }
            else
            {
                const CParameter *param = mainMenu[ menuPos ].GetParam();
                if( param != 0 )
                {
                    if( param->GetType() == PTYPE_LIST )
                    {
                        param->SetValue( ( param->GetValue() + 1 ) % ( param->GetValueMax() + 1 ) );
                        mainMenu[ menuPos ].UpdateData();
                    }
                    else if( param->GetType() == PTYPE_INT )
                    {
                        editMode = !editMode;

                        if( editMode ) mainMenu[ menuPos ].UpdateState( 2 );
                        else mainMenu[ menuPos ].UpdateState( 1 );
                    }
                }
            }
        }
        else if( key == 0x1b ) break;
        else if( key == 0x1c )
        {
            nextWindow = true;
            break;
        }

        //------------------------------------------------------------------------

        if( (dword) ( Timer_GetTickCounter() - tickCounter ) >= 100 )
        {
            GetTime( &time );
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

    CPU_Start();
    Spectrum_UpdateConfig();

    return nextWindow;
}

void Shell_Enter()
{
    bool result = true;
    int pos = 0;

    while( result )
    {
        if( pos == 0 ) result = Shell_Browser();
        else if( pos == 1 ) result = Shell_SettingsMenu();
        else if( pos == 2 ) break;

        pos = ( pos + 1 ) % 3;
    }

    SystemBus_Write( 0xc00021, 0 );            // Enable Video
    SystemBus_Write( 0xc00022, 0 );
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

    char str[ 0x20 ];
    byte CurX = 0, CurY = 0, CurXold, CurYold, half = 0, editAddr = 0, editAddrPos = 3;

    word addr = SystemBus_Read( 0xC00001 );
    CurX = addr & 0x0007;

    Debugger_Screen1( addr, CurX, CurY );
    WriteAttr( 5 + CurX * 3, CurY, 0x17, 2 );

    while( true )
    {
        byte key = GetKey( true );

        if( key == 0x1B ) break;

        CurXold = CurX;
        CurYold = CurY;

        if( !editAddr )
        {
            if( key == 0x0B ) //KEY_UP
            {
                half = 0;
                if( CurY == 0)
                    if( addr > 8 ) addr -= 8;
                    else addr = 0;
                else
                    CurY--;
            }
            if( key == 0x0A ) //KEY_DOWN
            {
                half = 0;
                if( CurY == 22)
                    if( addr < 0xFF48 ) addr += 8;
                    else addr = 0xFF48;
                else
                    CurY++;
            }
            if( key == 0x08 ) //KEY_LEFT
            {
                half = 0;
                if( CurX > 0 ) CurX--;
            }
            if( key == 0x09 ) //KEY_RIGHT
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
        }
        if( key == 's' ) SaveSnapshot( Shell_GetPath(), 0 );
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
