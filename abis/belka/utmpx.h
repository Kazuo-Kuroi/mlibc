#ifndef _ABIBITS_UTMPX_H
#define _ABIBITS_UTMPX_H

#include <abi-bits/pid_t.h>
#include <bits/timeval.h>

#define __UT_HOSTSIZE 256
#define __UT_NAMESIZE 32
#define __UT_LINESIZE 32

/* Struct definition taken from musl */
struct utmpx {
	short ut_type;
	short __ut_pad1;
	pid_t ut_pid;
	char ut_line[__UT_LINESIZE];
	char ut_id[4];
	char ut_user[__UT_NAMESIZE];
	char ut_host[__UT_HOSTSIZE];
	struct {
		short __e_termination;
		short __e_exit;
	} ut_exit;
	int ut_session, __ut_pad2;
	struct timeval ut_tv;
	unsigned ut_addr_v6[4];
	char __ut_unused[20];	/* not __unused, which BSD headers define */
};

#define e_exit __e_exit
#define e_termination __e_termination

#define ut_name ut_user

/* System V's places for them. */
#define UTMPX_FILE "/var/adm/utmpx"
#define WTMPX_FILE "/var/adm/wtmpx"

#endif /* _ABIBITS_UTMPX_H */
