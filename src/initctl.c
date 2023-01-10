/*
 * public domain
 */

#include "feat.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main ( const int argc, char ** argv )
{
  if ( 0 < getuid () || 0 < geteuid () ) {
    (void) fputs ( "Must be super user\n", stderr ) ;
    return 100 ;
  }

  return 0 ;
}

