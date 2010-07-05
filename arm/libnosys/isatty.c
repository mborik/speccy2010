/*
 * Stub version of isatty.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _isatty( int file )
{
  errno = ENOSYS;
  return 0;
}

