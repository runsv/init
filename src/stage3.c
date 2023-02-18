/*
 * public domain
 */

#include "feat.h"
#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <signal.h>
#include <poll.h>
#include <alloca.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include "os.h"

#if defined (OSLinux)
# include <sys/prctl.h>
# include <linux/kd.h>
# include <linux/vt.h>
#elif defined (OSfreebsd)
#elif defined (OSnetbsd)
#elif defined (OSopenbsd)
#endif

#include "version.h"
#include "common.h"

extern char ** environ ;

#include "common.c"

static void reap ( void )
{
  pid_t p = 0 ;

  do {
    p = waitpid ( -1, NULL, WNOHANG ) ;
  } while ( ( 0 < p ) || ( ( 0 > p ) && ( EINTR == errno ) ) ) ;

  return ;
}

static void exec_stage5 ( char * stage5, char * what )
{
  struct stat sb ;

  if ( stat ( stage5, & sb ) ) {
    ;
  } else if ( S_ISREG ( sb . st_mode ) && ( 00100 & sb . st_mode ) ) {
    char * av [ 3 ] = { stage5, what, (char *) NULL } ;
    char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;
    (void) execve ( stage5, av, env ) ;
  }

  return ;
}

static void fork_and_reboot ( const int cmd )
{
  const pid_t pid = xfork () ;

  if ( 0 == pid ) {
    /*
    (void) sig_unblock_all () ;
    (void) setsid () ;
    */
    (void) reboot ( cmd ) ;
    exit ( 127 ) ;
  } else if ( 0 < pid ) {
    pid_t p ;

    do {
      p = waitpid ( pid, NULL, 0 ) ;
      if ( pid == p ) { break ; }
    } while ( ( 0 > p ) && ( EINTR == p ) ) ;
  } else if ( 0 > pid ) {
  }

  return ;
}

int main ( const int argc, char ** argv )
{
  int opt = 0 ;
  int cmd = RB_POWER_OFF ;
  int secs = 2 ;
  char * what = NULL ;
  char * stage5 = STAGE5 ;
  const char * pname = ( ( 0 < argc ) && argv && * argv && ** argv ) ? * argv : "init" ;

  (void) setsid () ;
  (void) setpgid ( 0, 0 ) ;
  (void) umask ( 0022 ) ;
  (void) chdir ( "/" ) ;
  /*
  (void) setenv ( "PATH", PATH, 1 ) ;
  */
  open_console () ;

  while ( ( opt = getopt ( argc, argv, ":HhKkPpRrs:x:" ) ) != -1 ) {
    switch ( opt ) {
      case 'H' :
      case 'h' :
        cmd = RB_HALT_SYSTEM ;
        break ;
      case 'K' :
      case 'k' :
        cmd = RB_KEXEC ;
        break ;
      case 'P' :
      case 'p' :
        cmd = RB_POWER_OFF ;
        break ;
      case 'R' :
      case 'r' :
        cmd = RB_AUTOBOOT ;
        break ;
      case 's' :
        {
          const int s = atoi ( optarg ) ;
          if ( 0 < s ) { secs = s ; }
        }
        break ;
      case 'x' :
        if ( optarg && ( '/' == * optarg ) &&
          ( 0 == access ( optarg, F_OK | X_OK ) ) )
        {
          stage5 = optarg ;
        }
        break ;
      case '?' :
        (void) printf ( "%s: unknown option '%c'\n", pname, optopt ) ;
        break ;
      case ':' :
        break ;
      default :
        break ;
    }
  }

  switch ( cmd ) {
    case RB_HALT_SYSTEM :
      what = "halt" ;
      break ;
    case RB_POWER_OFF :
      what = "poweroff" ;
      break ;
    case RB_AUTOBOOT :
      what = "reboot" ;
      break ;
    case RB_KEXEC :
      what = "kexec" ;
      break ;
    default :
      what = "stop" ;
      break ;
  }

  (void) run ( STAGE3, STAGE3, what ) ;
  reap () ;
  sync () ;
  (void) printf ( "%s: Sending all processes the TERM signal...\n", pname ) ;
  (void) kill ( -1, SIGTERM ) ;
  (void) kill ( -1, SIGHUP ) ;
  (void) kill ( -1, SIGCONT ) ;
  do_sleep ( secs, 0 ) ;
  reap () ;
  (void) printf ( "%s: Sending all processes the KILL signal...\n", pname ) ;
  (void) kill ( -1, SIGKILL ) ;
  reap () ;
  sync () ;
  (void) run ( STAGE4, STAGE4, what ) ;
  reap () ;
  sync () ;
  exec_stage5 ( stage5, what ) ;
  sync () ;
  fork_and_reboot ( cmd ) ;

  while ( 1 ) { (void) pause () ; }

  return 0 ;
}

