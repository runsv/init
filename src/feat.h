
#ifndef _HEADER_FEAT_H_
#define _HEADER_FEAT_H_		1

#define _POSIX_C_SOURCE 200819L
#define _XOPEN_SOURCE 700

#if defined (__linux__) || defined (__linux) || defined (linux)
#  define OSLinux		1
#  define _GNU_SOURCE		1
#  define _ALL_SOURCE		1
#  include <features.h>
/*
#  if defined (__GLIBC__)
#    define _GNU_SOURCE		1
#  endif
#  undef _FEATURES_H
*/
#endif

#endif /* end of header file */

