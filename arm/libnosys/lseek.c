/*
 * Stub version of lseek.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _lseek( int file, int ptr, int dir )
{
  errno = ENOSYS;
  return -1;
}


