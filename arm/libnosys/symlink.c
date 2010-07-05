/*
 * Stub version of symlink.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _symlink( const char *path1, const char *path2 )
{
  errno = ENOSYS;
  return -1;
}

