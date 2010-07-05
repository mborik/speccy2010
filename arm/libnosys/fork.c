/*
 * Stub version of fork.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _fork()
{
  errno = ENOSYS;
  return -1;
}

