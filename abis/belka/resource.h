#ifndef _ABIBITS_RESOURCE_H
#define _ABIBITS_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mlibc-config.h>
#include <bits/timeval.h>

#if defined(_GNU_SOURCE)
#define PRIO_MIN (-20)
#define PRIO_MAX 20
#endif /* defined(_GNU_SOURCE) */

#if defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN
#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1

#define RLIMIT_CPU 0
#endif /* defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN */

/* System V's resource numbers, as Belka's getrlimit and setrlimit take. */
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_NOFILE 5
#define RLIMIT_VMEM 6
#define RLIMIT_AS RLIMIT_VMEM

#if defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN
#define RLIMIT_NLIMITS 7
#endif

#if defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN
struct rusage {
	struct timeval ru_utime;
	struct timeval ru_stime;
	long ru_maxrss;
	long ru_ixrss;
	long ru_idrss;
	long ru_isrss;
	long ru_minflt;
	long ru_majflt;
	long ru_nswap;
	long ru_inblock;
	long ru_oublock;
	long ru_msgsnd;
	long ru_msgrcv;
	long ru_nsignals;
	long ru_nvcsw;
	long ru_nivcsw;
};
#endif /* defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN */

#ifdef __cplusplus
}
#endif

#endif /* _ABIBITS_RESOURCE_H */
