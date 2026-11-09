#ifndef _ABIBITS_VM_FLAGS_H
#define _ABIBITS_VM_FLAGS_H

/* mmap() protections and flags, as the Belka kernel defines them (<sys/mman.h>). */

#include <mlibc-config.h>

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x000
#define MAP_SHARED 0x001
#define MAP_PRIVATE 0x002
#define MAP_FIXED 0x010
#define MAP_ANON 0x100
#define MAP_ANONYMOUS MAP_ANON

#define MS_ASYNC 0x1
#define MS_SYNC 0x4
#define MS_INVALIDATE 0x2

#define MCL_CURRENT 0x1
#define MCL_FUTURE 0x2

#define POSIX_MADV_NORMAL 0
#define POSIX_MADV_RANDOM 1
#define POSIX_MADV_SEQUENTIAL 2
#define POSIX_MADV_WILLNEED 3
#define POSIX_MADV_DONTNEED 4

#endif /* _ABIBITS_VM_FLAGS_H */
