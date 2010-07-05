/*
 * Stub version of write.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _write( int file, char *ptr, int len )
{
  errno = ENOSYS;
  return -1;
}

