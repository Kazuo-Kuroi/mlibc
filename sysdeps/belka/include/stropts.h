#ifndef _STROPTS_H
#define _STROPTS_H

#include <sys/ioctl.h>

#define S_INPUT 0x0001
#define S_HIPRI 0x0002
#define S_OUTPUT 0x0004
#define S_MSG 0x0008
#define S_ERROR 0x0010
#define S_HANGUP 0x0020
#define S_RDNORM 0x0040
#define S_WRNORM S_OUTPUT
#define S_RDBAND 0x0080
#define S_WRBAND 0x0100
#define S_BANDURG 0x0200

#endif
