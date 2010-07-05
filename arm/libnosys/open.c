/*
 * Stub version of open.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _open( char *file, int flags, int mode )
{
  errno = ENOSYS;
  return -1;
}


