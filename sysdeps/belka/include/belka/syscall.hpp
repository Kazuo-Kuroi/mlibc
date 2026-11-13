#pragma once

/*
 * Belka system calls (UnixWare numbers) and the calling sequence.
 *
 * The number goes in %rax and the arguments in %rdi, %rsi, %rdx, %r10,
 * %r8 and %r9; the syscall instruction clobbers %rcx and %r11. On return
 * the carry flag says whether the call failed: if so %rax is the error
 * number, otherwise %rax and %rdx are the two results. A call that fails
 * with ERESTART was interrupted by a signal whose handler asked for
 * restarts, and is issued again with the same arguments.
 */

namespace belka {

enum : long {
	SYS_exit = 1,
	SYS_fork = 2,
	SYS_read = 3,
	SYS_write = 4,
	SYS_open = 5,
	SYS_close = 6,
	SYS_wait = 7,
	SYS_creat = 8,
	SYS_link = 9,
	SYS_unlink = 10,
	SYS_exec = 11,
	SYS_chdir = 12,
	SYS_time = 13,
	SYS_mknod = 14,
	SYS_chmod = 15,
	SYS_chown = 16,
	SYS_brk = 17,
	SYS_stat = 18,
	SYS_lseek = 19,
	SYS_getpid = 20,
	SYS_getuid = 24,
	SYS_alarm = 27,
	SYS_fstat = 28,
	SYS_pause = 29,
	SYS_access = 33,
	SYS_sync = 36,
	SYS_kill = 37,
	SYS_pgrpsys = 39,
	SYS_dup = 41,
	SYS_pipe = 42,
	SYS_times = 43,
	SYS_getgid = 47,
	SYS_signal = 48,
	SYS_ioctl = 54,
	SYS_fsync = 58,
	SYS_execve = 59,
	SYS_umask = 60,
	SYS_fcntl = 62,
	SYS_rmdir = 79,
	SYS_mkdir = 80,
	SYS_getdents = 81,
	SYS_poll = 87,
	SYS_lstat = 88,
	SYS_symlink = 89,
	SYS_readlink = 90,
	SYS_fchmod = 93,
	SYS_fchown = 94,
	SYS_sigprocmask = 95,
	SYS_sigsuspend = 96,
	SYS_sigaltstack = 97,
	SYS_sigaction = 98,
	SYS_sigpending = 99,
	SYS_context = 100,
	SYS_waitsys = 107,
	SYS_mmap = 115,
	SYS_mprotect = 116,
	SYS_munmap = 117,
	SYS_fchdir = 120,
	SYS_readv = 121,
	SYS_writev = 122,
	SYS_setrlimit = 128,
	SYS_getrlimit = 129,
	SYS_lchown = 130,
	SYS_rename = 134,
	SYS_uname = 135,
	SYS_gettimeofday = 171,
	SYS_lwpcreate = 173,
	SYS_lwpexit = 174,
	SYS_lwpwait = 175,
	SYS_lwpself = 176,
	SYS_lwpinfo = 177,
	SYS_lwpprivate = 178,
	SYS_lwp_addr_wait = 181,
	SYS_lwp_addr_wake = 182,
	SYS_pread = 189,
	SYS_pwrite = 190,
	SYS_ftruncate = 192,
	SYS_lwpkill = 193,
	SYS_sigwait = 194,
	SYS_fork1 = 195,
	SYS_forkall = 196,
	SYS_lwpsuspend = 203,
	SYS_lwpcontinue = 204,
	SYS_lwp_sema_wait = 207,
	SYS_lwp_sema_post = 208,
	SYS_lwp_sema_trywait = 209,
	SYS_clock_gettime = 210,
	SYS_clock_getres = 211,
	SYS_yield = 212,
	SYS_openat = 213,
	SYS_mknodat = 214,
	SYS_mkdirat = 215,
	SYS_unlinkat = 216,
	SYS_renameat = 217,
	SYS_linkat = 218,
	SYS_symlinkat = 219,
	SYS_readlinkat = 220,
	SYS_fstatat = 221,
	SYS_fchownat = 222,
	SYS_fchmodat = 223,
	SYS_faccessat = 224,
};

/* context() commands. */
enum { GETCONTEXT = 0, SETCONTEXT = 1 };

/* sigpending() commands. */
enum { SIGPENDING_PENDING = 1, SIGPENDING_FILLSET = 2 };

/* pgrpsys() commands. */
enum {
	PGRP_GETPGRP = 0,
	PGRP_SETPGRP = 1,
	PGRP_GETSID = 2,
	PGRP_SETSID = 3,
	PGRP_GETPGID = 4,
	PGRP_SETPGID = 5,
};

/* waitsys() id types. */
enum { P_PID = 0, P_PPID = 1, P_PGID = 2, P_SID = 3, P_ALL = 7 };

constexpr long ERESTART_ = 91;

struct Result {
	long value;	/* %rax, or -1 on failure */
	long value2;	/* %rdx */
	int error;	/* 0, or the error number */
};

inline Result syscall(long n, long a1 = 0, long a2 = 0, long a3 = 0,
		long a4 = 0, long a5 = 0, long a6 = 0) {
	for (;;) {
		register long r10 asm("r10") = a4;
		register long r8 asm("r8") = a5;
		register long r9 asm("r9") = a6;
		long rax = n, rdi = a1, rsi = a2, rdx = a3;
		bool failed;

		asm volatile ("syscall"
				: "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx),
				  "+r"(r10), "+r"(r8), "+r"(r9), "=@ccc"(failed)
				:
				: "rcx", "r11", "memory");
		if (!failed)
			return {rax, rdx, 0};
		if (rax == ERESTART_)
			continue;
		return {-1, 0, static_cast<int>(rax)};
	}
}

template<typename... Args>
inline Result sc(long n, Args... args) {
	return syscall(n, ((long)args)...);
}

} // namespace belka
