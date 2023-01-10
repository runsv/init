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
/*
# include <linux/fcntl.h>
*/
# include <sys/prctl.h>
# include <linux/kd.h>
# include <linux/vt.h>
#elif defined (OSfreebsd)
#elif defined (OSnetbsd)
#elif defined (OSopenbsd)
#endif

#include "version.h"
#include "init.h"

extern char ** environ ;

static struct service {
  dev_t dev ;
  ino_t ino ;
  pid_t pid ;
  time_t started_at ;
} svc [ SVC_LEN ] ;

static int maxidx = 0 ;
static const char * pname = "init" ;

static size_t strcopy ( char * dest, const char * src, const size_t siz )
{
  size_t i ;

  for ( i = 0 ; ( i + 1 < siz ) && ( src [ i ] != '\0' ) ; ++ i ) {
    dest [ i ] = src [ i ] ;
  }

  for ( ; i < siz ; ++ i ) {
    dest [ i ] = '\0' ;
  }

  return i ;
}

static int close_fd ( const int fd )
{
  if ( 0 <= fd ) {
    int i ;

    do { i = close ( fd ) ; }
    while ( i && ( EINTR == errno ) ) ;

    return i ;
  }

  return -3 ;
}

/* sleep a number of seconds without being interrupted */
static void do_sleep ( long int s, long int m )
{
  if ( 0 < s || 0 < m ) {
    struct timespec ts, rem ;

    s = ( 0 < s ) ? s : 0 ;
    m = ( 0 < m ) ? 1000 * ( m % ( 1000 * 1000 ) ) : 0 ;
    ts . tv_sec = s ;
    ts . tv_nsec = m ;

    while ( 0 > nanosleep ( & ts, & rem ) && EINTR == errno )
    { ts = rem ; }
  }
}

/* "failsafe" version of the fork(2) syscall */
static pid_t xfork ( void )
{
  pid_t p = 0 ;

  while ( 0 > ( p = fork () ) && ENOSYS != errno ) {
    /* sleep a few seconds and try again */
    do_sleep ( 5, 0 ) ;
  }

  return p ;
}

void opendevconsole ( void )
{
  const int fd = open ( "/dev/console", O_RDWR | O_NOCTTY ) ;

  if ( 0 <= fd ) {
    (void) dup2 ( fd, 0 ) ;
    (void) dup2 ( fd, 1 ) ;
    (void) dup2 ( fd, 2 ) ;
    if ( 2 < fd ) { (void) close_fd ( fd ) ; }
  }
}

/* reap terminated child/sub processes with waitpid(2) */
static void reap ( void )
{
  pid_t p = 0 ;

  do {
    p = waitpid ( -1, NULL, WNOHANG ) ;
  } while ( ( 0 < p ) || ( ( 0 > p ) && ( EINTR == errno ) ) ) ;

  return ;
}

static int run_cmd ( char * cmd, ... )
{
  int i ;
  pid_t pid ;
  char * argv [ 10 ] ;

  va_list arguments ;

  va_start ( arguments, cmd ) ;

  for ( i = 0 ;
    i < 9 && ( argv [ i ] = va_arg ( arguments, char * ) ) != NULL ;
    ++ i ) ;

  va_end ( arguments ) ;
  argv [ i ] = NULL ;
  pid = xfork () ;

  if ( 0 > pid ) {
    return -1 ;
  } else if ( 0 < pid ) {
    /* parent */
    int w ;
    pid_t p ;

    do {
      p = waitpid ( pid, & w, 0 ) ;
    } while ( ( 0 > p ) && ( EINTR == errno ) ) ;
  } else if ( 0 == pid ) {
    /* child */
    char * env [ 3 ] = {
      "PATH=" PATH,
      (char *) NULL,
      (char *) NULL,
    } ;

    /*
    char * s = getenv ( "TERM" ) ;

    if ( s && * s ) {
    }
    */

    /* opendevconsole() ? */
    (void) execve ( cmd, argv, env ) ;
    perror ( "execve() failed" ) ;
    exit ( 127 ) ;
  }

  return 0 ;
}

static pid_t spawn ( const char * path )
{
  char * av [ 2 ] = { (char *) path, (char *) NULL } ;
  char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;
  const pid_t pid = xfork () ;

  if ( 0 == pid ) {
    (void) execve ( path, av, env ) ;
    exit ( 127 ) ;
  }

  return pid ;
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

/* services have pathnames relative to SVC_ROOT */
static int spawn_svc ( const char * name )
{
  int fd, dirfd ;
  pid_t pid = 0 ;
  char * av [ 2 ] = { (char *) name, (char *) NULL } ;
  char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;

  dirfd = open ( SVC_ROOT, O_RDONLY | O_CLOEXEC | O_PATH ) ;

  if ( 0 > dirfd ) { return -1 ; }

  fd = openat ( dirfd, name, O_RDONLY | O_CLOEXEC | O_PATH ) ;

  if ( 0 > fd ) { return -3 ; }

  pid = xfork () ;

  if ( 0 == pid ) {
    (void) fexecve ( fd, av, env ) ;
    (void) close_fd ( dirfd ) ;
    (void) close_fd ( fd ) ;
    exit ( 127 ) ;
  }

  (void) close_fd ( dirfd ) ;
  (void) close_fd ( fd ) ;
  return pid ;
}

static int find_by_pid ( const pid_t pid )
{
  int i = 0 ;

  for ( i = 0 ; i <= maxidx ; ++ i ) {
    if ( pid == svc [ i ] . pid ) {
      return i ;
      break ;
    }
  }

  return -1 ;
}

static int find_by_name ( const char * name )
{
  struct stat sb ;
  const int dirfd = open ( SVC_ROOT, O_RDONLY | O_CLOEXEC | O_PATH ) ;

  if ( 0 > dirfd ) { return -2 ; }

  if ( fstatat ( dirfd, name, & sb, 0 ) ) {
    return -3 ;
  } else if ( S_ISREG ( sb . st_mode ) && ( 00100 & sb . st_mode ) ) {
    int i ;

    for ( i = 0 ; i <= maxidx ; ++ i ) {
      if ( sb . st_dev == svc [ i ] . dev && sb . st_ino == svc [ i ] . ino ) {
        /* found the corresponding array element */
        return i ;
        break ;
      }
    }
  }

  return -1 ;
}

static int search_svc_file ( const char * base, const int idx,
  char * buf, const size_t len )
{
  int r = -1 ;

  if ( 0 <= idx && idx <= maxidx ) {
    int dfd ;
    struct stat sb ;
    struct dirent * ent = NULL ;
    const dev_t dev = svc [ idx ] . dev ;
    const ino_t ino = svc [ idx ] . ino ;
    DIR * dp = opendir ( base ) ;

    if ( NULL == dp ) { return -3 ; }

    dfd = dirfd ( dp ) ;

    while ( ( ent = readdir ( dp ) ) != NULL ) {
      const char * name = ent -> d_name ;

      if ( '.' == * name ) { continue ; }

      if ( fstatat ( dfd, name, & sb, 0 ) ) { continue ; }
      else if ( S_ISREG ( sb . st_mode ) && ( 00100 & sb . st_mode ) ) {
        if ( dev == sb . st_dev && ino == sb . st_ino ) {
          /* found the corresponding service file */
          (void) strcopy ( buf, name, len ) ;
          r = 0 ;
          break ;
        }
      }
    }

    (void) closedir ( dp ) ;
  }

  return r ;
}

static int add_svc ( struct stat * sp, const char * name )
{
  int i = maxidx ;

  if ( 0 <= i && SVC_LEN > i ) {
    svc [ i ] . dev = sp -> st_dev ;
    svc [ i ] . ino = sp -> st_ino ;
    svc [ i ] . pid = spawn_svc ( name ) ;
    svc [ i ] . started_at = time ( NULL ) ;
    ++ maxidx ;
  }

  return maxidx ;
}

static void setup_svc ( const char * base )
{
  int dfd ;
  struct stat sb ;
  struct dirent * ent = NULL ;
  DIR * dp = opendir ( base ) ;

  if ( NULL == dp ) { return ; }

  dfd = dirfd ( dp ) ;

  while ( ( ent = readdir ( dp ) ) != NULL ) {
    const char * name = ent -> d_name ;

    if ( '.' == * name ) { continue ; }

    if ( fstatat ( dfd, name, & sb, 0 ) ) { continue ; }
    else if ( S_ISREG ( sb . st_mode ) && ( 00100 & sb . st_mode ) ) {
      (void) add_svc ( & sb, name ) ;
    }
  }

  (void) closedir ( dp ) ;
}

static void check_pid ( const pid_t pid )
{
  const int idx = find_by_pid ( pid ) ;

  if ( 0 <= idx && idx <= maxidx ) {
    char buf [ 1 + NAME_MAX ] = { 0 } ;

    if ( 0 == search_svc_file ( SVC_ROOT, idx, buf, sizeof ( buf ) - 1 ) ) {
      svc [ idx ] . pid = spawn_svc ( buf ) ;
    }
  }
}

static void child_handler ( void )
{
  int w ;
  pid_t p ;

  do {
    p = waitpid ( -1, & w, WNOHANG ) ;

    if ( 0 < p ) {
      check_pid ( p ) ;
    }
  } while ( ( 0 < p ) || ( ( 0 > p ) && ( EINTR == errno ) ) ) ;
}

static void fork_and_reboot ( const int cmd )
{
  const pid_t pid = xfork () ;

  if ( 0 == pid ) {
    (void) reboot ( cmd ) ;
    exit ( 0 ) ;
  } else if ( 0 < pid ) {
    pid_t p ;

    do {
      p = waitpid ( pid, NULL, 0 ) ;
      if ( pid == p ) { break ; }
    } while ( ( 0 > p ) && ( EINTR == p ) ) ;
  } else if ( 0 > pid ) {
  }
}

static void sys_shutdown ( const int cmd )
{
  char * what = NULL ;

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

  (void) run_cmd ( STAGE3, STAGE3 ) ;
  reap () ;
  sync () ;
  (void) puts ( "Sending all processes the TERM signal...\n" ) ;
  (void) kill ( -1, SIGTERM ) ;
  (void) kill ( -1, SIGCONT ) ;
  do_sleep ( 2, 0 ) ;
  reap () ;
  (void) puts ( "Sending all processes the KILL signal...\n" ) ;
  (void) kill ( -1, SIGKILL ) ;
  reap () ;
  sync () ;
  (void) run_cmd ( STAGE4, STAGE4, what ) ;
  reap () ;
  sync () ;

  { struct stat sb ;

    if ( stat ( STAGE5, & sb ) ) {
      ;
    } else if ( S_ISREG ( sb . st_mode ) && ( 00100 & sb . st_mode ) ) {
      char * av [ 3 ] = { STAGE5, what, (char *) NULL } ;
      char * env [ 2 ] = { "PATH=" PATH, (char *) NULL } ;
      (void) execve ( STAGE5, av, env ) ;
    }
  }

  sync () ;
  fork_and_reboot ( cmd ) ;
}

static int open_fifo ( const char * path )
{
  int fd = -1 ;
  (void) remove ( path ) ;
  (void) mkfifo ( path, 00600 ) ;
  (void) chmod ( path, 00600 ) ;
  fd = open ( path, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOCTTY ) ;

  if ( 0 > fd ) {
    ;
  } else {
    (void) fchmod ( fd, 00600 ) ;
  }

  return fd ;
}

/* restore default dispostions for all signals (except SIGCHLD) */
static void reset_sigs ( void )
{
  int i ;
  struct sigaction sa ;

  /* zero out the struct before use */
  (void) memset ( & sa, 0, sizeof ( struct sigaction ) ) ;
  sa . sa_flags = SA_RESTART ;
  sa . sa_handler = SIG_DFL ;
  (void) sigemptyset ( & sa . sa_mask ) ;

  for ( i = 1 ; NSIG > i ; ++ i ) {
    if ( SIGKILL != i && SIGSTOP != i && SIGCHLD != i ) {
      (void) sigaction ( i, & sa, NULL ) ;
    }
  }

  //sa . sa_handler = pseudo_sighand ;
  (void) sigaction ( SIGCHLD, & sa, NULL ) ;

  /* unblock all signals */
  (void) sigprocmask ( SIG_SETMASK, & sa . sa_mask, NULL ) ;
}

static volatile unsigned int got_sig = 0 ;
static int sig_fd = -1 ;

static void signal_handler ( int sig )
{
  /* just set the corresponding bit */
  got_sig |= 1 << sig ;
  (void) write ( sig_fd, & sig, sizeof ( int ) ) ;
}

static void setup_sigs ( void )
{
  struct sigaction sa ;
  /* zero out the struct before use */
  (void) memset ( & sa, 0, sizeof ( struct sigaction ) ) ;
  sa . sa_flags = SA_RESTART ;
  sa . sa_handler = signal_handler ;
  (void) sigfillset ( & sa . sa_mask ) ;
  (void) sigaction ( SIGABRT, & sa, NULL ) ;
  (void) sigaction ( SIGALRM, & sa, NULL ) ;
  (void) sigaction ( SIGHUP, & sa, NULL ) ;
  (void) sigaction ( SIGINT, & sa, NULL ) ;
  (void) sigaction ( SIGTERM, & sa, NULL ) ;
  (void) sigaction ( SIGQUIT, & sa, NULL ) ;
  (void) sigaction ( SIGUSR1, & sa, NULL ) ;
  (void) sigaction ( SIGUSR2, & sa, NULL ) ;
#ifdef SIGWINCH
  (void) sigaction ( SIGWINCH, & sa, NULL ) ;
#endif
#ifdef SIGPWR
  (void) sigaction ( SIGPWR, & sa, NULL ) ;
#endif
  sa . sa_flags |= SA_NOCLDSTOP ;
  (void) sigaction ( SIGCHLD, & sa, NULL ) ;
}

int main ( const int argc, char ** argv )
{
  int fifo_fd = -1 ;
  int sfd [ 2 ] ;
  struct pollfd pfd [ 2 ] ;

  /* initialize global variables */
  maxidx = 0 ;
  got_sig = 0 ;
  sig_fd = -1 ;
  pname = ( ( 0 < argc ) && argv && * argv && ** argv ) ? * argv : "init" ;

  /*
  if ( 1 < getpid () ) {
    (void) fprintf ( stderr, "%s: must run as process #1\n", pname ) ;
    return 100 ;
  }

  if ( 0 < getuid () || 0 < geteuid () ) {
    (void) fprintf ( stderr, "%s: must be super user\n", pname ) ;
    return 100 ;
  }
  */

  (void) setsid () ;
  (void) setpgid ( 0, 0 ) ;
  (void) umask ( 0022 ) ;
  (void) chdir ( "/" ) ;
  (void) sethostname ( "darkstar", 8 ) ;
  (void) setenv ( "PATH", PATH, 1 ) ;
  (void) run_cmd ( STAGE1, STAGE1 ) ;
  setup_svc ( SVC_ROOT ) ;
  (void) spawn ( STAGE2 ) ;

  if ( pipe ( sfd ) ) {
    perror ( "pipe() failed" ) ;
  }

  (void) fcntl ( sfd [ 0 ], F_SETFD, FD_CLOEXEC ) ;
  (void) fcntl ( sfd [ 1 ], F_SETFD, FD_CLOEXEC ) ;
  (void) fcntl ( sfd [ 0 ], F_SETFL, O_NONBLOCK ) ;
  (void) fcntl ( sfd [ 1 ], F_SETFL, O_NONBLOCK ) ;
  sig_fd = sfd [ 1 ] ;
  fifo_fd = open_fifo ( INIT_FIFO ) ;
  (void) fcntl ( fifo_fd, F_SETFL, O_NONBLOCK ) ;
  pfd [ 0 ] . fd = sfd [ 0 ] ;
  pfd [ 0 ] . events = POLLIN ;
  pfd [ 1 ] . fd = fifo_fd ;
  pfd [ 1 ] . events = POLLIN ;

  /* main event loop */
  while ( 1 ) {
    const int r = poll ( pfd, 2, -1 ) ;

    if ( 0 < r ) {
      if ( pfd [ 0 ] . revents & POLLIN ) {
        /* input on the signal selfpipe */
        int s = 0 ;

        while ( 0 < read ( pfd [ 0 ] . fd, & s, sizeof ( int ) ) ) {
          switch ( s ) {
            case SIGABRT :
              break ;
            case SIGALRM :
              break ;
            case SIGCHLD :
              child_handler () ;
              break ;
            case SIGHUP :
              break ;
            case SIGINT :
              sys_shutdown ( RB_AUTOBOOT ) ;
              break ;
            case SIGTERM :
              sys_shutdown ( RB_POWER_OFF ) ;
              break ;
            case SIGQUIT :
              break ;
            case SIGUSR1 :
              /* reopen the init fifo */
              (void) close_fd ( fifo_fd ) ;
              fifo_fd = open_fifo ( INIT_FIFO ) ;
              (void) fcntl ( fifo_fd, F_SETFL, O_NONBLOCK ) ;
              pfd [ 1 ] . fd = fifo_fd ;
              break ;
            case SIGUSR2 :
              sys_shutdown ( RB_AUTOBOOT ) ;
              break ;
            case SIGWINCH :
#ifdef SIGPWR
            case SIGPWR :
#endif
              sys_shutdown ( RB_POWER_OFF ) ;
              break ;
          }
        }
      } else if ( pfd [ 1 ] . revents & POLLIN ) {
        /* input on the control fifo */
        int n ;
        char buf [ 512 ] = { 0 } ;

        while ( ( n = read ( pfd [ 1 ] . fd, buf, sizeof ( buf ) - 1 ) ) > 0 ) {
          buf [ n ] = '\0' ;

          if ( strcmp ( "reopen", buf ) == 0 ) {
            /* reopen the control fifo */
            (void) close_fd ( fifo_fd ) ;
            fifo_fd = open_fifo ( INIT_FIFO ) ;
            (void) fcntl ( fifo_fd, F_SETFL, O_NONBLOCK ) ;
            pfd [ 1 ] . fd = fifo_fd ;
          } else if ( strcmp ( "reboot", buf ) == 0 ) {
            sys_shutdown ( RB_AUTOBOOT ) ;
          } else if ( strcmp ( "poweroff", buf ) == 0 ) {
            sys_shutdown ( RB_POWER_OFF ) ;
          } else if ( strcmp ( "halt", buf ) == 0 ) {
            sys_shutdown ( RB_HALT_SYSTEM ) ;
          } else if ( strcmp ( "kexec", buf ) == 0 ) {
            sys_shutdown ( RB_KEXEC ) ;
          } else if ( strcmp ( "reexec", buf ) == 0 ) {
          } else if ( strcmp ( "reload", buf ) == 0 ) {
          } else if ( strncmp ( "stop", buf, 4 ) == 0 ) {
          } else if ( strncmp ( "start", buf, 5 ) == 0 ) {
          } else if ( strncmp ( "restart", buf, 7 ) == 0 ) {
          }
        }
      }
    } else if ( 0 == r ) {
      /* poll() timed out. Cannot happen since the poll() interval
       * is negative.
       */
      continue ;
    } else if ( 0 > r ) {
      if ( EINTR == errno ) {
        /* System call interrupted,
         * but could have been from another signal;
         * poll our fds again.
         */
        continue ;
      }
    }
  } /* while */

  return 0 ;
}

