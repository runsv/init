/*
// This is a simple utility to instruct the kernel to shutdown
// or reboot the machine. This runs at the end of the shutdown
// process as an init-agnostic method of shutting down the system.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>

int main ( const int argc, char * argv [] )
{
  char what = 'p' ;
  int cmd = RB_POWER_OFF ;

  if ( geteuid () > 0 ) {
    (void) puts ( "Error: kpow must be run as root\n" ) ;
    return 100 ;
  }

  if ( 1 < argc ) {
    if ( argv [ 1 ] [ 0 ] ) { what = argv [ 1 ] [ 0 ] ; }
  }

  switch ( what ) {
    case 'K' :
    case 'k' :
      cmd = RB_KEXEC ;
      break ;

    case 'S' :
    case 's' :
      cmd = RB_SW_SUSPEND ;
      break ;

    case 'H' :
    case 'h' :
      cmd = RB_HALT_SYSTEM ;
      break ;

    case 'R' :
    case 'r' :
      cmd = RB_AUTOBOOT ;
      break ;

    case 'P' :
    case 'p' :
    default :
      cmd = RB_POWER_OFF ;
      break ;
  }

  sync () ;

  if ( reboot ( cmd ) ) {
    perror ( "reboot() failed" ) ;
    return 111 ;
  }

  return 0 ;
}

