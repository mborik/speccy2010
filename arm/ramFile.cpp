#include "system.h"
#include "ramFile.h"
#include "types.h"
#include "specShell.h"

#define RF_MAX_SIZE         0x00100000
#define RF_MAX_FILES        8
#define RF_START_ADDRESS    0x00800000

static int bankState[ RF_MAX_FILES ];

int rf_open ( RFIL *rfile, const char *name, byte flags )
{
    int bank = 0;
    while( bankState[ bank ] != 0 && bank < RF_MAX_FILES ) bank++;
    if( bank >= RF_MAX_FILES ) return 0xff;

    FIL file;
    int result = f_open( &file, name, FA_READ );
    if( result != 0 ) return result;
    if( file.fsize > RF_MAX_SIZE ) return 0xff;

    dword address = RF_START_ADDRESS + bank * RF_MAX_SIZE;
    word page = address >> 23;
    SystemBus_Write( 0xc00020, page );

    dword size = file.fsize;
    //CPU_Stop();

    while( size > 0 )
    {
        byte buff[2];
        dword rsize;
        f_read( &file, buff, 2, &rsize );

        if( page != ( address >> 23 ) )
        {
            word page = address >> 23;
            SystemBus_Write( 0xc00020, page );
        }

        SystemBus_Write( 0x800000 | ( ( address & 0x007fffff ) >> 1 ), buff[0] | ( buff[1] << 8 ) );

        address += 2;
        size -= rsize;

        if( ( address & 0xff ) == 0 ) WDT_Kick();
    }

    //CPU_Start();

    rfile->flag = flags;
    rfile->fptr = 0;
    rfile->fsize = file.fsize;

    rfile->ramBank = bank;
    bankState[ bank ] = 1;

    f_close( &file );
    return 0;
}

int rf_close ( RFIL *rfile )
{
    bankState[ rfile->ramBank ] = 0;
    return 0;
}

int rf_read ( RFIL *rfile, void *buff, dword size, dword *psize )
{
    dword address = RF_START_ADDRESS + rfile->ramBank * RF_MAX_SIZE + rfile->fptr;
    word page = address >> 23;
    SystemBus_Write( 0xc00020, page );

    //CPU_Stop();

    *psize = 0;
    while( *psize < size && rfile->fptr < rfile->fsize && rfile->fptr < RF_MAX_SIZE )
    {
        if( page != ( address >> 23 ) )
        {
            word page = address >> 23;
            SystemBus_Write( 0xc00020, page );
        }

        *(byte*)buff = SystemBus_Read( address & 0x7fffff );
        buff = (byte*) buff + 1;
        address++;
        rfile->fptr++;
        (*psize)++;
    }

    //CPU_Start();

    return 0;
}

int rf_write ( RFIL *rfile, const void *buff, dword size, dword *psize )
{
    dword address = RF_START_ADDRESS + rfile->ramBank * RF_MAX_SIZE + rfile->fptr;
    word page = address >> 23;
    SystemBus_Write( 0xc00020, page );

    //CPU_Stop();

    *psize = 0;
    while( *psize < size && rfile->fptr < rfile->fsize && rfile->fptr < RF_MAX_SIZE )
    {
        if( page != ( address >> 23 ) )
        {
            word page = address >> 23;
            SystemBus_Write( 0xc00020, page );
        }

        SystemBus_Write( address & 0x7fffff, *(byte*)buff );
        buff = (byte*) buff + 1;
        address++;
        rfile->fptr++;
        (*psize)++;
    }

    //CPU_Start();

    return 0;
}

int rf_lseek ( RFIL *rfile, dword pos )
{
    if( pos < RF_MAX_SIZE && pos < rfile->fsize )
    {
        rfile->fptr = pos;
        return 0;
    }

    return 0xff;
}



