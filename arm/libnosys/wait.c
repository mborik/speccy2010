/*
 * Stub version of wait.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _wait( int  *status )
{
  errno = ENOSYS;
  return -1;
}

