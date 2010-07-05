/*
 * Stub version of kill.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#undef errno
extern int errno;

int _kill( int pid, int sig )
{
  errno = ENOSYS;
  return -1;
}
