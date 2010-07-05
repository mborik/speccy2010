/*
 * Stub version of execve.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno

void _exit( int code )
{
    while(1);
}
