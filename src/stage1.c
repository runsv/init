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

enum {
  P_DISABLE	= ( 1 << 0 ),
  P_RESTART	= ( 1 << 1 ),
  P_STALE	= ( 1 << 2 ),
} ;

extern char ** environ ;
static const char * pname = "init" ;

#include "common.c"

static pid_t spawn ( const char * path )
{
  char * av [ 2 ] = { (char *) path, (char *) NULL } ;
  char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;
  const pid_t pid = xfork () ;

  if ( 0 == pid ) {
    //(void) sig_unblock_all () ;
    (void) setsid () ;
    (void) execve ( path, av, env ) ;
    exit ( 127 ) ;
  }

  return pid ;
}

static void exec_svscan ( char * svscan )
{
  char * av [ 3 ] = { svscan, SCAN_DIR, (char *) NULL } ;
  char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;
  (void) execve ( svscan, av, env ) ;
}

static void setup_kb ( void )
{
  const int fd = open ( "/dev/tty0", O_RDONLY | O_NOCTTY ) ;

  if ( 0 > fd ) {
    (void) fputs ( "INIT: Cannot open /dev/tty0 (kbrequest will not be handled)", stderr ) ;
  } else {
    if ( ioctl ( fd, KDSIGACCEPT, SIGWINCH ) < 0 ) {
      (void) fputs ( "INIT: ioctl KDSIGACCEPT on /dev/tty0 failed (kbrequest will not be handled)", stderr ) ;
    }

    close_fd ( fd ) ;
  }

  /* don't panic on early cad before s6-svscan catches it */
  //sig_block ( SIGINT ) ;
  if ( reboot ( RB_DISABLE_CAD ) ) {
    (void) fputs ( "Cannot trap ctrl-alt-del", stderr ) ;
  }
}

int main ( const int argc, char ** argv )
{
  /* initialize global variables */
  pname = ( ( 0 < argc ) && argv && * argv && ** argv ) ? * argv : "init" ;

  (void) setsid () ;
  (void) setpgid ( 0, 0 ) ;
  (void) umask ( 0022 ) ;
  (void) chdir ( "/" ) ;
  (void) sethostname ( "darkstar", 8 ) ;
  /*
  (void) setenv ( "PATH", PATH, 1 ) ;
  */

  (void) run ( STAGE1, STAGE1 ) ;
  (void) mkdir ( SCAN_DIR, 00755 ) ;
  (void) mkdir ( LOG_SERVICE, 00755 ) ;
  (void) mkfifo ( LOG_SERVICE "/fifo", 00600 ) ;
  (void) spawn ( STAGE2 ) ;
  exec_svscan ( SVSCAN ) ;

  return 111 ;
}

