#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

/*
 * mount(2) and umount(2) as System V has them (Belka's <sys/mount.h>):
 * mflag must have MS_DATA, and fstype names the type ("ext2fs",
 * "tmpfs"); dataptr and datalen are passed on, and ignored for now.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define MS_RDONLY 0x01
#define MS_FSS 0x02
#define MS_DATA 0x04
#define MS_NOSUID 0x10
#define MS_REMOUNT 0x20

#ifndef __MLIBC_ABI_ONLY

int mount(const char *__spec, const char *__dir, int __mflag, ...);
int umount(const char *__file);

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_MOUNT_H */
