/*
 * Stub version of gettimeofday.
 */

#include <_ansi.h>
#include <_syslist.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#undef errno
extern int errno;

struct timeval;
struct timezone;

int _gettimeofday( struct timeval  *ptimeval, struct timezone *ptimezone )
{
  errno = ENOSYS;
  return -1;
}


