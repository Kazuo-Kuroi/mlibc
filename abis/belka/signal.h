#ifndef _ABIBITS_SIGNAL_H
#define _ABIBITS_SIGNAL_H

/*
 * Signals, as the Belka kernel defines them (<sys/signal.h> and
 * <sys/ucontext.h>): the UnixWare numbers and flags, with the native
 * amd64 structure layouts.
 */

#include <mlibc-config.h>
#include <abi-bits/pid_t.h>
#include <bits/ansi/clock_t.h>
#include <abi-bits/sigval.h>
#include <abi-bits/sigset_t.h>
#include <abi-bits/uid_t.h>
#include <bits/size_t.h>
#include <bits/types.h>

#if __MLIBC_POSIX_OPTION
#include <abi-bits/sigevent.h>
#endif

#if defined(_DEFAULT_SOURCE) || (__MLIBC_POSIX1 && !__MLIBC_POSIX2024)
#define POLL_IN 1
#define POLL_OUT 2
#define POLL_MSG 3
#define POLL_ERR 4
#define POLL_PRI 5
#define POLL_HUP 6
#endif

typedef struct {
	int si_signo;
	int si_code;
	int si_errno;
	int __si_pad0;
	union {
		struct {
			pid_t __pid;
			uid_t __uid;
			union {
				int __status;		/* SIGCHLD */
				union sigval __value;	/* queued signals */
			} __u;
			clock_t __utime;	/* SIGCHLD, exited: ticks */
			clock_t __stime;
		} __proc;
		struct {
			void *__addr;		/* faults */
		} __fault;
		char __pad[112];
	} __data;
} siginfo_t;

#define si_pid __data.__proc.__pid
#define si_uid __data.__proc.__uid
#define si_status __data.__proc.__u.__status
#define si_value __data.__proc.__u.__value
#define si_utime __data.__proc.__utime
#define si_stime __data.__proc.__stime
#define si_addr __data.__fault.__addr

#ifdef __cplusplus
extern "C" {
#endif

/* Argument for signal() */
typedef void (*__sighandler) (int);

#define SIG_ERR ((__sighandler)(void *)(-1))
#define SIG_DFL ((__sighandler)(void *)(0))
#define SIG_IGN ((__sighandler)(void *)(1))
#if defined(_DEFAULT_SOURCE) || __MLIBC_XOPEN
#define SIG_HOLD ((__sighandler)(void *)(2))
#endif

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGIOT 6
#define SIGABRT 6
#define SIGEMT 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGBUS 10
#define SIGSEGV 11
#define SIGSYS 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGUSR1 16
#define SIGUSR2 17
#define SIGCLD 18
#define SIGCHLD 18
#define SIGPWR 19
#define SIGWINCH 20
#define SIGURG 21
#define SIGPOLL 22
#define SIGIO 22
#define SIGSTOP 23
#define SIGTSTP 24
#define SIGCONT 25
#define SIGTTIN 26
#define SIGTTOU 27
#define SIGVTALRM 28
#define SIGPROF 29
#define SIGXCPU 30
#define SIGXFSZ 31
#define SIGWAITING 32
#define SIGLWP 33
#define SIGAIO 34

/*
 * There are no real-time signals yet: the range is empty, and the
 * numbers above 34 that a sigset_t has room for are reserved for them.
 */
#define SIGRTMIN 35
#define SIGRTMAX 34

#define SIGUNUSED SIGSYS

#if __MLIBC_BUILDING_MLIBC
/* Thread cancellation uses the signal UnixWare reserved for the threads library. */
#  define SIGCANCEL SIGLWP
#endif

#define _NSIG 35
#if defined(_DEFAULT_SOURCE)
#define NSIG _NSIG
#endif

/* constants for sigprocmask() */
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

#define SA_ONSTACK 0x00000001
#define SA_RESETHAND 0x00000002
#define SA_RESTART 0x00000004
#define SA_SIGINFO 0x00000008
#define SA_NODEFER 0x00000010
#define SA_NOCLDWAIT 0x00010000
#define SA_NOCLDSTOP 0x00020000
#define SA_NOMASK SA_NODEFER
#define SA_ONESHOT SA_RESETHAND

#if __MLIBC_XOPEN || defined(_DEFAULT_SOURCE) || __MLIBC_BUILDING_MLIBC
#define MINSIGSTKSZ 4096
#define SIGSTKSZ 16384
#define SS_ONSTACK 1
#define SS_DISABLE 2
#endif

typedef struct __stack {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
	int __ss_pad;
} stack_t;

/* constants for sigev_notify of struct sigevent */
#define SIGEV_NONE 1
#define SIGEV_SIGNAL 2
#define SIGEV_THREAD 3

#define SEGV_MAPERR 1
#define SEGV_ACCERR 2

#define BUS_ADRALN 1
#define BUS_ADRERR 2
#define BUS_OBJERR 3

#define ILL_ILLOPC 1
#define ILL_ILLOPN 2
#define ILL_ILLADR 3
#define ILL_ILLTRP 4
#define ILL_PRVOPC 5
#define ILL_PRVREG 6
#define ILL_COPROC 7
#define ILL_BADSTK 8

/* si_code values at or below zero: sent by a process, not the kernel. */
#define SI_MESGQ (-5)
#define SI_ASYNCIO (-4)
#define SI_TIMER (-3)
#define SI_QUEUE (-2)
#define SI_LWP (-1)	/* _lwp_kill() */
#define SI_TKILL SI_LWP
#define SI_USER 0

struct sigaction {
	int sa_flags;
	int __sa_pad;
	union {
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t *, void *);
	} __sa_handler;
	sigset_t sa_mask;
	int __sa_resv[2];
};
#define sa_handler __sa_handler.sa_handler
#define sa_sigaction __sa_handler.sa_sigaction

#if defined(__x86_64__)

#if defined(_GNU_SOURCE) || __MLIBC_BUILDING_MLIBC
#define REG_RDI 0
#define REG_RSI 1
#define REG_RDX 2
#define REG_RCX 3
#define REG_R8 4
#define REG_R9 5
#define REG_RAX 6
#define REG_RBX 7
#define REG_RBP 8
#define REG_R10 9
#define REG_R11 10
#define REG_R12 11
#define REG_R13 12
#define REG_R14 13
#define REG_R15 14
#define REG_TRAPNO 15
#define REG_ERR 16
#define REG_RIP 17
#define REG_CS 18
#define REG_RFL 19
#define REG_RSP 20
#define REG_SS 21
#define REG_FSBASE 22
#endif
#define __NGREG 23
#if defined(_DEFAULT_SOURCE)
#define NGREG __NGREG
#endif

/*
 * <bits/posix/posix_signal.h> defines greg_t (unsigned long) and, for
 * _DEFAULT_SOURCE, fpregset_t as glibc does; the kernel's gregs are
 * int64_t, of the same size.
 */
typedef unsigned long gregset_t[__NGREG];

typedef struct {
	gregset_t gregs;
	/* An FXSAVE image, or an XSAVE image whose first 512 bytes are one. */
	struct {
		unsigned char __fp_state[1024];
	} fpregs;
} mcontext_t;

#define UC_SIGMASK 0x0001
#define UC_STACK 0x0002
#define UC_CPU 0x0004
#define UC_FP 0x0008
#define UC_MCONTEXT (UC_CPU | UC_FP)
#define UC_ALL (UC_SIGMASK | UC_STACK | UC_MCONTEXT)

typedef struct __ucontext {
	unsigned long uc_flags;
	struct __ucontext *uc_link;
	sigset_t uc_sigmask;
	stack_t uc_stack;
	mcontext_t uc_mcontext;
	long __uc_filler[4];
} ucontext_t;

#else
#error "Missing architecture specific code."
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ABIBITS_SIGNAL_H */
