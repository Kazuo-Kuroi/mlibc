#include <abi-bits/errno.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/stat.h>
#include <abi-bits/vm-flags.h>
#include <abi-bits/wait.h>
#include <bits/ensure.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include <belka/syscall.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>

using belka::sc;

namespace {

/* Flags the kernel does not take yet, which open() carries out itself. */
constexpr int libc_open_flags = O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW;

/* TCGETA, the System V terminal attribute request, as an isatty() probe. */
constexpr unsigned long belka_TCGETA = ('T' << 8) | 1;

int set_cloexec(int fd) {
	return sc(belka::SYS_fcntl, fd, F_SETFD, FD_CLOEXEC).error;
}

/* Paths relative to a directory descriptor other than the current directory are not supported yet. */
bool at_cwd(int dirfd, const char *path) {
	return dirfd == AT_FDCWD || (path && path[0] == '/');
}

/* The kernel's directory entry: d_ino, d_off, d_reclen, then the name. */
struct kernel_dirent {
	uint64_t d_ino;
	int64_t d_off;
	uint16_t d_reclen;
	char d_name[];
};

} // namespace

namespace mlibc {

void Sysdeps<LibcLog>::operator()(const char *message) {
	sc(belka::SYS_write, 2, message, strlen(message));
	sc(belka::SYS_write, 2, "\n", 1);
}

void Sysdeps<LibcPanic>::operator()() {
	sysdep<LibcLog>("mlibc: panic");
	auto pid = sc(belka::SYS_getpid);
	sc(belka::SYS_kill, pid.value, SIGABRT);
	sysdep<Exit>(127);
}

void Sysdeps<Exit>::operator()(int status) {
	sc(belka::SYS_exit, status);
	__builtin_unreachable();
}

/* The thread pointer is the %fs base, set with __lwp_private(). */
int Sysdeps<TcbSet>::operator()(void *pointer) {
	return sc(belka::SYS_lwpprivate, pointer).error;
}

/*
 * Futexes are Belka's address waits: _lwp_addr_wait sleeps while the
 * word holds the expected value, for a relative time; _lwp_addr_wake
 * wakes one waiter or all of them.
 */
int Sysdeps<FutexWait>::operator()(int *pointer, int expected, const struct timespec *time) {
	return belka::sc(belka::SYS_lwp_addr_wait, pointer, expected, time).error;
}

int Sysdeps<FutexWake>::operator()(int *pointer, bool all) {
	return belka::sc(belka::SYS_lwp_addr_wake, pointer, all ? 0 : 1).error;
}

pid_t Sysdeps<FutexTid>::operator()() {
	return static_cast<pid_t>(belka::sc(belka::SYS_lwpself).value);
}

int Sysdeps<AnonAllocate>::operator()(size_t size, void **pointer) {
	return sysdep<VmMap>(nullptr, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, pointer);
}

int Sysdeps<AnonFree>::operator()(void *pointer, size_t size) {
	return sysdep<VmUnmap>(pointer, size);
}

int Sysdeps<VmMap>::operator()(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
	auto r = sc(belka::SYS_mmap, hint, size, prot, flags, fd, offset);
	if (r.error)
		return r.error;
	*window = reinterpret_cast<void *>(r.value);
	return 0;
}

int Sysdeps<VmUnmap>::operator()(void *pointer, size_t size) {
	return sc(belka::SYS_munmap, pointer, size).error;
}

int Sysdeps<VmProtect>::operator()(void *pointer, size_t size, int prot) {
	return sc(belka::SYS_mprotect, pointer, size, prot).error;
}

int Sysdeps<Open>::operator()(const char *pathname, int flags, mode_t mode, int *fd) {
	if (flags & O_NOFOLLOW) {
		struct stat st;
		auto r = sc(belka::SYS_lstat, pathname, &st);
		if (!r.error && S_ISLNK(st.st_mode))
			return ELOOP;
	}
	auto r = sc(belka::SYS_open, pathname, flags & ~libc_open_flags, mode);
	if (r.error)
		return r.error;
	int nfd = static_cast<int>(r.value);
	if (flags & O_DIRECTORY) {
		struct stat st;
		auto s = sc(belka::SYS_fstat, nfd, &st);
		int e = s.error ? s.error : !S_ISDIR(st.st_mode) ? ENOTDIR : 0;
		if (e) {
			sc(belka::SYS_close, nfd);
			return e;
		}
	}
	if (flags & O_CLOEXEC) {
		if (int e = set_cloexec(nfd); e) {
			sc(belka::SYS_close, nfd);
			return e;
		}
	}
	*fd = nfd;
	return 0;
}

int Sysdeps<Openat>::operator()(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	if (!at_cwd(dirfd, path))
		return ENOSYS;
	return sysdep<Open>(path, flags, mode, fd);
}

int Sysdeps<OpenDir>::operator()(const char *path, int *handle) {
	return sysdep<Open>(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0, handle);
}

/*
 * The kernel's records lack d_type, so they are rewritten into mlibc's
 * struct dirent. A record grows by at most 8 bytes and is at least 24,
 * so reading 3/4 of the buffer's worth always fits.
 */
int Sysdeps<ReadEntries>::operator()(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
	alignas(8) char kbuf[2048];
	size_t ksize = (max_size / 4 * 3) & ~size_t(7);

	if (ksize > sizeof(kbuf))
		ksize = sizeof(kbuf);
	if (ksize < 32)
		return EINVAL;
	auto r = sc(belka::SYS_getdents, handle, kbuf, ksize);
	if (r.error)
		return r.error;

	size_t in = 0, out = 0, n = static_cast<size_t>(r.value);
	auto obuf = static_cast<char *>(buffer);
	while (in < n) {
		auto k = reinterpret_cast<kernel_dirent *>(kbuf + in);
		size_t namelen = strlen(k->d_name);
		size_t reclen = (offsetof(struct dirent, d_name) + namelen + 1 + 7) & ~size_t(7);
		__ensure(out + reclen <= max_size);
		auto d = reinterpret_cast<struct dirent *>(obuf + out);
		d->d_ino = k->d_ino;
		d->d_off = k->d_off;
		d->d_reclen = reclen;
		d->d_type = DT_UNKNOWN;
		memcpy(d->d_name, k->d_name, namelen + 1);
		in += k->d_reclen;
		out += reclen;
	}
	*bytes_read = out;
	return 0;
}

int Sysdeps<Read>::operator()(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	auto r = sc(belka::SYS_read, fd, buf, count);
	if (r.error)
		return r.error;
	*bytes_read = r.value;
	return 0;
}

int Sysdeps<Write>::operator()(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	auto r = sc(belka::SYS_write, fd, buf, count);
	if (r.error)
		return r.error;
	*bytes_written = r.value;
	return 0;
}

int Sysdeps<Pread>::operator()(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
	auto r = sc(belka::SYS_pread, fd, buf, n, off);
	if (r.error)
		return r.error;
	*bytes_read = r.value;
	return 0;
}

int Sysdeps<Pwrite>::operator()(int fd, const void *buf, size_t n, off_t off, ssize_t *bytes_written) {
	auto r = sc(belka::SYS_pwrite, fd, buf, n, off);
	if (r.error)
		return r.error;
	*bytes_written = r.value;
	return 0;
}

int Sysdeps<Seek>::operator()(int fd, off_t offset, int whence, off_t *new_offset) {
	auto r = sc(belka::SYS_lseek, fd, offset, whence);
	if (r.error)
		return r.error;
	*new_offset = r.value;
	return 0;
}

int Sysdeps<Close>::operator()(int fd) {
	return sc(belka::SYS_close, fd).error;
}

int Sysdeps<Dup>::operator()(int fd, int flags, int *newfd) {
	auto r = sc(belka::SYS_dup, fd);
	if (r.error)
		return r.error;
	if (flags & O_CLOEXEC) {
		if (int e = set_cloexec(static_cast<int>(r.value)); e) {
			sc(belka::SYS_close, r.value);
			return e;
		}
	}
	*newfd = static_cast<int>(r.value);
	return 0;
}

int Sysdeps<Dup2>::operator()(int fd, int flags, int newfd) {
	auto r = sc(belka::SYS_fcntl, fd, F_DUP2FD, newfd);
	if (r.error)
		return r.error;
	if (flags & O_CLOEXEC)
		return set_cloexec(newfd);
	return 0;
}

int Sysdeps<Fcntl>::operator()(int fd, int request, va_list args, int *result) {
	long arg = va_arg(args, long);
	bool cloexec = false;

	if (request == F_DUPFD_CLOEXEC) {
		request = F_DUPFD;
		cloexec = true;
	}
	auto r = sc(belka::SYS_fcntl, fd, request, arg);
	if (r.error)
		return r.error;
	if (cloexec) {
		if (int e = set_cloexec(static_cast<int>(r.value)); e) {
			sc(belka::SYS_close, r.value);
			return e;
		}
	}
	*result = static_cast<int>(r.value);
	return 0;
}

int Sysdeps<Ioctl>::operator()(int fd, unsigned long request, void *arg, int *result) {
	auto r = sc(belka::SYS_ioctl, fd, request, arg);
	if (r.error)
		return r.error;
	if (result)
		*result = static_cast<int>(r.value);
	return 0;
}

int Sysdeps<Isatty>::operator()(int fd) {
	char termio[64];
	return sc(belka::SYS_ioctl, fd, belka_TCGETA, termio).error;
}

int Sysdeps<Stat>::operator()(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
	if (flags & ~(AT_SYMLINK_NOFOLLOW | AT_EMPTY_PATH))
		return EINVAL;
	if (fsfdt == fsfd_target::fd_path && (flags & AT_EMPTY_PATH) && !*path)
		fsfdt = fsfd_target::fd;
	switch (fsfdt) {
	case fsfd_target::fd:
		return sc(belka::SYS_fstat, fd, statbuf).error;
	case fsfd_target::fd_path:
		if (!at_cwd(fd, path))
			return ENOSYS;
		[[fallthrough]];
	case fsfd_target::path:
		return sc((flags & AT_SYMLINK_NOFOLLOW) ? belka::SYS_lstat : belka::SYS_stat,
				path, statbuf).error;
	default:
		return EINVAL;
	}
}

int Sysdeps<Access>::operator()(const char *path, int mode) {
	return sc(belka::SYS_access, path, mode).error;
}

int Sysdeps<Readlink>::operator()(const char *path, void *buffer, size_t max_size, ssize_t *length) {
	auto r = sc(belka::SYS_readlink, path, buffer, max_size);
	if (r.error)
		return r.error;
	*length = r.value;
	return 0;
}

int Sysdeps<Ftruncate>::operator()(int fd, size_t size) {
	return sc(belka::SYS_ftruncate, fd, size).error;
}

int Sysdeps<Chdir>::operator()(const char *path) {
	return sc(belka::SYS_chdir, path).error;
}

int Sysdeps<Fchdir>::operator()(int fd) {
	return sc(belka::SYS_fchdir, fd).error;
}

int Sysdeps<Mkdir>::operator()(const char *path, mode_t mode) {
	return sc(belka::SYS_mkdir, path, mode).error;
}

int Sysdeps<Rmdir>::operator()(const char *path) {
	return sc(belka::SYS_rmdir, path).error;
}

int Sysdeps<Unlinkat>::operator()(int dirfd, const char *path, int flags) {
	if (flags & ~AT_REMOVEDIR)
		return EINVAL;
	if (!at_cwd(dirfd, path))
		return ENOSYS;
	return sc((flags & AT_REMOVEDIR) ? belka::SYS_rmdir : belka::SYS_unlink, path).error;
}

int Sysdeps<Rename>::operator()(const char *path, const char *new_path) {
	return sc(belka::SYS_rename, path, new_path).error;
}

int Sysdeps<Link>::operator()(const char *old_path, const char *new_path) {
	return sc(belka::SYS_link, old_path, new_path).error;
}

int Sysdeps<Symlink>::operator()(const char *target_path, const char *link_path) {
	return sc(belka::SYS_symlink, target_path, link_path).error;
}

int Sysdeps<Umask>::operator()(mode_t mode, mode_t *old) {
	auto r = sc(belka::SYS_umask, mode);
	if (r.error)
		return r.error;
	*old = static_cast<mode_t>(r.value);
	return 0;
}

void Sysdeps<Sync>::operator()() {
	sc(belka::SYS_sync);
}

/*
 * Every clock reads the time of day, which gettimeofday() gives to the
 * microsecond; there is no separate monotonic clock yet.
 */
int Sysdeps<ClockGet>::operator()(int clock, time_t *secs, long *nanos) {
	switch (clock) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_MONOTONIC_RAW:
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
	case CLOCK_BOOTTIME:
		break;
	default:
		return EINVAL;
	}
	int64_t tv[2];
	auto r = sc(belka::SYS_gettimeofday, tv);
	if (r.error)
		return r.error;
	*secs = tv[0];
	*nanos = tv[1] * 1000;
	return 0;
}

int Sysdeps<Uname>::operator()(struct utsname *buf) {
	return sc(belka::SYS_uname, buf).error;
}

int Sysdeps<GetHostname>::operator()(char *buffer, size_t bufsize) {
	struct utsname u;
	if (int e = sysdep<Uname>(&u); e)
		return e;
	size_t n = strlen(u.nodename);
	if (n >= bufsize)
		return ENAMETOOLONG;
	memcpy(buffer, u.nodename, n + 1);
	return 0;
}

/* getpid() returns the process ID and the parent's; getuid() and getgid() the real and effective IDs. */
pid_t Sysdeps<GetPid>::operator()() {
	return static_cast<pid_t>(sc(belka::SYS_getpid).value);
}

pid_t Sysdeps<GetPpid>::operator()() {
	return static_cast<pid_t>(sc(belka::SYS_getpid).value2);
}

pid_t Sysdeps<GetTid>::operator()() {
	return static_cast<pid_t>(sc(belka::SYS_lwpself).value);
}

uid_t Sysdeps<GetUid>::operator()() {
	return static_cast<uid_t>(sc(belka::SYS_getuid).value);
}

uid_t Sysdeps<GetEuid>::operator()() {
	return static_cast<uid_t>(sc(belka::SYS_getuid).value2);
}

gid_t Sysdeps<GetGid>::operator()() {
	return static_cast<gid_t>(sc(belka::SYS_getgid).value);
}

gid_t Sysdeps<GetEgid>::operator()() {
	return static_cast<gid_t>(sc(belka::SYS_getgid).value2);
}

int Sysdeps<GetPgid>::operator()(pid_t pid, pid_t *pgid) {
	auto r = sc(belka::SYS_pgrpsys, belka::PGRP_GETPGID, pid);
	if (r.error)
		return r.error;
	*pgid = static_cast<pid_t>(r.value);
	return 0;
}

int Sysdeps<SetPgid>::operator()(pid_t pid, pid_t pgid) {
	return sc(belka::SYS_pgrpsys, belka::PGRP_SETPGID, pid, pgid).error;
}

int Sysdeps<GetSid>::operator()(pid_t pid, pid_t *sid) {
	auto r = sc(belka::SYS_pgrpsys, belka::PGRP_GETSID, pid);
	if (r.error)
		return r.error;
	*sid = static_cast<pid_t>(r.value);
	return 0;
}

int Sysdeps<SetSid>::operator()(pid_t *sid) {
	auto r = sc(belka::SYS_pgrpsys, belka::PGRP_SETSID);
	if (r.error)
		return r.error;
	*sid = static_cast<pid_t>(r.value);
	return 0;
}

/* fork() returns the other process's ID, and in %rdx whether this is the child. */
int Sysdeps<Fork>::operator()(pid_t *child) {
	auto r = sc(belka::SYS_fork);
	if (r.error)
		return r.error;
	*child = r.value2 ? 0 : static_cast<pid_t>(r.value);
	return 0;
}

int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
	return sc(belka::SYS_execve, path, argv, envp).error;
}

/* waitpid() over waitsys(), which reports through a siginfo_t. */
int Sysdeps<Waitpid>::operator()(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
	if (ru)
		return ENOSYS;
	if (flags & ~(WNOHANG | WUNTRACED | WCONTINUED))
		return EINVAL;

	int idtype;
	long id;
	if (pid == -1) {
		idtype = belka::P_ALL;
		id = 0;
	} else if (pid > 0) {
		idtype = belka::P_PID;
		id = pid;
	} else {
		idtype = belka::P_PGID;
		if (pid == 0) {
			auto g = sc(belka::SYS_pgrpsys, belka::PGRP_GETPGRP);
			id = g.value;
		} else {
			id = -pid;
		}
	}

	siginfo_t si;
	memset(&si, 0, sizeof(si));
	auto r = sc(belka::SYS_waitsys, idtype, id, &si, WEXITED | flags);
	if (r.error)
		return r.error;
	if (si.si_pid != 0 && status) {
		int s = si.si_status;
		switch (si.si_code) {
		case CLD_EXITED:
			*status = (s & 0xff) << 8;
			break;
		case CLD_KILLED:
			*status = s & 0x7f;
			break;
		case CLD_DUMPED:
			*status = (s & 0x7f) | __WCOREFLAG;
			break;
		case CLD_STOPPED:
		case CLD_TRAPPED:
			*status = ((s & 0xff) << 8) | 0x7f;
			break;
		case CLD_CONTINUED:
			*status = 0xffff;
			break;
		default:
			*status = 0;
			break;
		}
	}
	*ret_pid = si.si_pid;
	return 0;
}

int Sysdeps<Kill>::operator()(pid_t pid, int sig) {
	return sc(belka::SYS_kill, pid, sig).error;
}

int Sysdeps<Pause>::operator()() {
	return sc(belka::SYS_pause).error;
}

} // namespace mlibc
