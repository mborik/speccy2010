/*
 *  This file is a part of spec2009 project.
 *
 *  betadisk
 *
 */

#include "sysinc.h"
#include "stdinc.h"

char *wd_stat_decode( word data, word data2 )
{
    static char str[16];
    sniprintf( str, sizeof(str),"0x%.4x, 0x%.4x", data, data2 );
    return str;
}

char *wd_command_decode( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *ticks_str( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *ticks_str1( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *ticks_str2( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *bdi_sys_ctl_decode( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *bdi_port_desc( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}

char *bdi_sys_stat_decode( word data )
{
    static char str[8];
    sniprintf( str, sizeof(str),"0x%.4x", data );
    return str;
}
