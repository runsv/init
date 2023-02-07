
#ifndef _HEADER_COMMON_H_
#define _HEADER_COMMON_H_	1

#define PATH		"/bin:/sbin:/usr/bin:/usr/sbin"
#define SVSCAN		"/bin/s6-svscan"
#define SCAN_DIR	"/run/service"
#define LOG_SERVICE	SCAN_DIR "/Log"
#define LOG_FIFO	LOG_SERVICE "/fifo"
#define BASE_DIR	"/etc/s6"
#define STAGE1		BASE_DIR "/rc1"
#define STAGE2		BASE_DIR "/rc2"
#define STAGE3		BASE_DIR "/rc3"
#define STAGE4		BASE_DIR "/rc4"
#define STAGE5		BASE_DIR "/rc5"

#endif /* end of header file */

