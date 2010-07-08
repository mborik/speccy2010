/*
 * Stub version of close.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _close_r( void *reent, int fildes )
{
  errno = ENOSYS;
  return -1;
}

