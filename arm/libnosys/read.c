/*
 * Stub version of read.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _read( int file, char *ptr, int len )
{
  errno = ENOSYS;
  return -1;
}

