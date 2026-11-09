#ifndef _ABIBITS_UTSNAME_T_H
#define _ABIBITS_UTSNAME_T_H

/* struct utsname, as the Belka kernel defines it (<sys/utsname.h>). */

#define SYS_NMLN 257

struct utsname {
	char sysname[SYS_NMLN];
	char nodename[SYS_NMLN];
	char release[SYS_NMLN];
	char version[SYS_NMLN];
	char machine[SYS_NMLN];
};

#endif /* _ABIBITS_UTSNAME_T_H */
