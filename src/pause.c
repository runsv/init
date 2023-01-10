/*
 * pause(2) until SIG(INT,TERM,USR(1,2)) are received and
 * don't react to SIGHUP
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/* pseudo signal handler */
static void nop ( const int s )
{
  (void) s ;
}

static volatile sig_atomic_t got_sig = 0 ;

/* the "real" signal handler routine */
static void sig_hand ( const int s )
{
  /* just set the corresponding bit */
  got_sig |= 1 << s ;
}

static void sig_conf ( void )
{
  struct sigaction sa ;

  /* zero out the sigaction struct before use */
  (void) memset ( & sa, 0, sizeof ( struct sigaction ) ) ;
  sa . sa_flags = SA_RESTART ;
  sa . sa_handler = nop ;
  (void) sigemptyset ( & sa . sa_mask ) ;
  /* "ignore" unwanted signals */
  (void) sigaction ( SIGHUP, & sa, NULL ) ;
  /* catch other signals of interest */
  sa . sa_handler = sig_hand ;
  (void) sigaction ( SIGTERM, & sa, NULL ) ;
  (void) sigaction ( SIGINT, & sa, NULL ) ;
  (void) sigaction ( SIGUSR1, & sa, NULL ) ;
  (void) sigaction ( SIGUSR2, & sa, NULL ) ;
  /* unblock all signals now */
  (void) sigprocmask ( SIG_SETMASK, & sa . sa_mask, NULL ) ;
}

int main ( void )
{
  sig_conf () ;
  got_sig = 0 ;

  while ( 0 == got_sig ) {
    (void) pause () ;
  }

  return 0 ;
}

