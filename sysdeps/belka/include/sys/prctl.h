/*
 * Copyright (c) 2026 Setsuna Software L.C.
 * Created: 2026-11-20
 * SPDX-License-Identifier: MIT
 */

#ifndef _SYS_PRCTL_H_
#define _SYS_PRCTL_H_

#include <stddef.h>
#include <sys/types.h>

#define PR_SPROC         0x01
#define PR_SFDS          0x02
#define PR_SDIR          0x04
#define PR_SUMASK        0x08
#define PR_SULIMIT       0x10
#define PR_SID           0x20
#define PR_SADDR         0x40
#define PR_SALL          0x7f
#define PR_THREADS       0x80
#define PR_FLAGMASK      0xff000000U
#define PR_BLOCK        0x01000000U
#define PR_NOLIBC        0x02000000U
#define PR_EVENT         0x04000000U

#define PR_ISBLOCKED     2
#define PR_SETSTACKSIZE  3
#define PR_GETSTACKSIZE  4
#define PR_UNBLKONEXEC   6
#define PR_SETEXITSIG    8
#define PR_TERMCHILD     12
#define PR_GETSHMASK     13
#define PR_GETNSHARE     14
#define PR_SETABORTSIG   18
#define PR_INIT_THREADS  20
#define PR_LASTSHEXIT    22
#define PR_MAXBLOCKCNT  10000
#define PR_MINBLOCKCNT  (-10000)

/* Optional sixth syscall argument; the public functions prepare this. */
struct belka_sproc_private {
        void            *bsp_private;
        unsigned char   *bsp_exited;
};

#ifndef _KERNEL
#ifdef __cplusplus
extern "C" {
#endif
pid_t sproc(void (*)(void *), unsigned int, ...);
pid_t sprocsp(void (*)(void *, size_t), unsigned int, void *, void *, size_t);
ptrdiff_t prctl(unsigned int, ...);
int blockproc(pid_t);
int unblockproc(pid_t);
int setblockproccnt(pid_t, int);
int blockprocall(pid_t);
int unblockprocall(pid_t);
int setblockproccntall(pid_t, int);
#ifdef __cplusplus
}
#endif
#endif

#endif /* _SYS_PRCTL_H_ */
