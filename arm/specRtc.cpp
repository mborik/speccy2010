#include <stdio.h>

#include "system.h"
#include "specRtc.h"

static byte rtcData[ 0x20 ] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00,
                                0x80, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

byte RTC_ToDec( byte dat )
{
    return ( ( dat / 10 ) << 4 ) | ( dat % 10 );
}

void RTC_Update()
{
    rtcData[ 0x0a ] |= 0x80;
    SystemBus_Write( 0xc0010a, rtcData[ 0x0a ] );

    tm time;
    RTC_GetTime( &time );

    rtcData[0] = RTC_ToDec( time.tm_sec );
    rtcData[2] = RTC_ToDec( time.tm_min );
    rtcData[4] = RTC_ToDec( time.tm_hour );
    rtcData[6] = RTC_ToDec( time.tm_wday );
    rtcData[7] = RTC_ToDec( time.tm_mday );
    rtcData[8] = RTC_ToDec( time.tm_mon + 1 );
    rtcData[9] = RTC_ToDec( time.tm_year % 100 );

    for( int i = 0; i < 0x20; i++ ) SystemBus_Write( 0xc00100 + i, rtcData[i] );

    rtcData[ 0x0a ] &= ~0x80;
    SystemBus_Write( 0xc0010a, rtcData[ 0x0a ] );
}
