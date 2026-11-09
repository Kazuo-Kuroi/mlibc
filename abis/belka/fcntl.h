#ifndef _ABIBITS_FCNTL_H
#define _ABIBITS_FCNTL_H

/*
 * open() flags and fcntl() commands, as the Belka kernel defines them
 * (<sys/fcntl.h>): the System V values.
 */

#include <mlibc-config.h>
#include <abi-bits/pid_t.h>

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_ACCMODE 0x0003
#define O_SYNC 0x0010
#define O_DSYNC O_SYNC
#define O_RSYNC O_SYNC
#define O_APPEND 0x0008
#define O_NONBLOCK 0x0080
#define O_CREAT 0x0100
#define O_TRUNC 0x0200
#define O_EXCL 0x0400
#define O_NOCTTY 0x0800

/*
 * Not in UnixWare 2; the values are the ones later System V releases
 * gave them. The kernel does not take these yet: the C library carries
 * them out around open().
 */
#define O_NOFOLLOW 0x20000
#define O_CLOEXEC 0x800000
#define O_DIRECTORY 0x1000000

/* Not supported: they never match a flags word. */
#define O_PATH 0
#define O_EXEC O_RDONLY
#define O_SEARCH O_RDONLY
#define O_TMPFILE 0x40000000
#define O_TTY_INIT 0

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_SETLK 6
#define F_SETLKW 7
#define F_DUP2FD 9
#define F_GETLK 14
#define F_GETOWN 23
#define F_SETOWN 24
#define F_DUPFD_CLOEXEC 37

#define F_RDLCK 1
#define F_WRLCK 2
#define F_UNLCK 3

#define FD_CLOEXEC 1

/* For the *at() calls, which the C library carries out itself for now. */
#define AT_FDCWD -100
#define AT_SYMLINK_NOFOLLOW 0x1000
#define AT_REMOVEDIR 0x1
#define AT_SYMLINK_FOLLOW 0x2
#define AT_EACCESS 0x4
#if defined(_GNU_SOURCE) || __MLIBC_BUILDING_MLIBC
#define AT_EMPTY_PATH 0x2000
#endif

#define POSIX_FADV_NORMAL 0
#define POSIX_FADV_RANDOM 1
#define POSIX_FADV_SEQUENTIAL 2
#define POSIX_FADV_WILLNEED 3
#define POSIX_FADV_DONTNEED 4
#define POSIX_FADV_NOREUSE 5

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

#endif /* _ABIBITS_FCNTL_H */
