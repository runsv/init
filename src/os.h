/*
 * try to determine the OS we build on/for
 */

#ifndef _HEADER_OS_H_
#define _HEADER_OS_H_		1

#if defined (__BSD__) || defined (BSD)
#  define OSbsd		1
#  define OS		"BSD"
#endif

#if defined (__linux__) || defined (__linux) || defined (linux)
#  define OSLinux	1
#  define OSlinux	1
#  define OS		"Linux"
#elif defined (__sun__) || defined (__sun) || defined (sun)
# if defined (__unix__) && defined (__svr4__)
#   define OSsunos	5
#   define OSsunos5	1
#   define OSsolaris	1
#   define OSsysv	1
#   define OS		"Solaris"
# else
#   define OSsunos	4
#   define OSsunos4	1
/*
#   define OSbsd	1
*/
#   define OS		"SunOS"
# endif
#elif defined (__AIX__) || defined (AIX)
#  define OSsysv	1
#  define OSaix		1
#  define OS		"AIX"
#elif defined (__FreeBSD__) || defined (FreeBSD) || defined (__FreeBSD_kernel__)
#  define OSfreebsd	1
#  define OSbsd		1
#  define OS		"FreeBSD"
#elif defined (__DragonFly__) || defined (DragonFly)
#  define OSdragonfly	1
#  define OSdragon	1
#  define OSdfly	1
#  define OSbsd		1
#  define OS		"DragonFly"
#elif defined (__NetBSD__) || defined (NetBSD)
#  define OSnetbsd	1
#  define OSbsd		1
#  define OS		"NetBSD"
#elif defined (__OpenBSD__) || defined (OpenBSD)
#  define OSopenbsd	1
#  define OSbsd		1
#  define OS		"OpenBSD"
#endif

#endif /* end of header file */

