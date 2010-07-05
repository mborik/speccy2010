/*
 * Stub version of link.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _link( char *existing, char *n )
{
  errno = ENOSYS;
  return -1;
}
