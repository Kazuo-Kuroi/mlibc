#ifndef _ABIBITS_STATVFS_H
#define _ABIBITS_STATVFS_H

#include <mlibc-config.h>

#include <abi-bits/fsblkcnt_t.h>
#include <abi-bits/fsfilcnt_t.h>

#define ST_RDONLY 1
#define ST_NOSUID 2

#define FSTYPSZ 16

/*
 * Belka's struct statvfs: System V's, with f_basetype (the file system
 * type's name), every field 64 bits wide. The kernel fills it as it is.
 */
struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	unsigned long f_fsid;
	char f_basetype[FSTYPSZ];
	unsigned long f_flag;
	unsigned long f_namemax;
	unsigned long __f_spare[4];
};

#endif /* _ABIBITS_STATVFS_H */
