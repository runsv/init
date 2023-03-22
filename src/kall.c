/*
// This is a simple 'killall5' alternative to remove the
// dependency on a rather unportable and "rare" tool for
// the purposes of shutting down the machine.
*/

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

int main ( const int argc, char * argv [] )
{
  int pid, sid ;
  int sig = SIGTERM ;
  struct dirent * ent = NULL ;
  DIR * dir = NULL ;
  const int mypid = getpid () ;
  const int mysid = getsid ( 0 ) ;

  if ( 1 < argc ) {
    const int s = atoi ( argv [ 1 ] ) ;
    if ( 0 < s && s <= _NSIG ) { sig = s ; }
  }

  dir = opendir ( "/proc" ) ;

  if ( NULL == dir ) {
    perror ( "Could not open /proc" ) ;
    return 111 ;
  }

  (void) kill ( -1, SIGSTOP ) ;

  while ( NULL != ( ent = readdir ( dir ) ) ) {
    if ( 0 == isdigit ( ent -> d_name [ 0 ] ) ) {
      continue ;
    }

    pid = atoi ( ent -> d_name ) ;

    if ( 2 > pid ) {
      continue ;
    }

    sid = getsid ( pid ) ;

    /* kernel thread: sid == 0 */
    if ( mypid != pid && mysid != sid && 0 < sid ) {
      (void) kill ( pid, sig ) ;
    }
  }

  (void) closedir ( dir ) ;
  (void) kill ( -1, SIGCONT ) ;

  return 0 ;
}

