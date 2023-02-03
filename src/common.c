/*
 * public domain
 */

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

    while ( ( 0 > nanosleep ( & ts, & rem ) ) && ( EINTR == errno ) )
    { ts = rem ; }
  }
}

/* "failsafe" version of the fork(2) syscall */
static pid_t xfork ( void )
{
  pid_t p = 0 ;

  while ( ( 0 > ( p = fork () ) ) && ( ENOSYS != errno ) ) {
    /* sleep a few seconds and try again */
    do_sleep ( 5, 0 ) ;
  }

  return p ;
}

static void open_console ( void )
{
  const int fd = open ( "/dev/console", O_RDWR | O_NOCTTY ) ;

  if ( 0 <= fd ) {
    (void) dup2 ( fd, 0 ) ;
    (void) dup2 ( fd, 1 ) ;
    (void) dup2 ( fd, 2 ) ;
    if ( 2 < fd ) { (void) close_fd ( fd ) ; }
  }
}

static int run ( char * cmd, ... )
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
      if ( p == pid ) { break ; }
    } while ( ( 0 > p ) && ( EINTR == errno ) ) ;
  } else if ( 0 == pid ) {
    /* child */
    char * env [ 3 ] = {
      "PATH=" PATH,
      "TERM=linux",
      (char *) NULL,
    } ;

    /*
    char * s = getenv ( "TERM" ) ;

    if ( s && * s ) {
    }
    */

    (void) sig_unblock_all () ;
    (void) setsid () ;
    /* opendevconsole() ? */
    (void) execve ( cmd, argv, env ) ;
    perror ( "execve() failed" ) ;
    exit ( 127 ) ;
  }

  return 0 ;
}

