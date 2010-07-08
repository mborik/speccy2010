/*
 * Stub version of getpid.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _getpid()
{
  errno = ENOSYS;
  return -1;
}

