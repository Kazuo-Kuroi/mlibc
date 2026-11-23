#ifndef _STROPTS_H
#define _STROPTS_H

/*
 * STREAMS, as Belka's kernel has it (<sys/stropts.h> there): the
 * stream head's ioctls, getmsg and putmsg. The numbers are System V's.
 */

#include <stdint.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __STR ('S' << 8)
#define I_NREAD (__STR | 001)
#define I_PUSH (__STR | 002)
#define I_POP (__STR | 003)
#define I_LOOK (__STR | 004)
#define I_FLUSH (__STR | 005)
#define I_SRDOPT (__STR | 006)
#define I_GRDOPT (__STR | 007)
#define I_STR (__STR | 010)
#define I_SETSIG (__STR | 011)
#define I_GETSIG (__STR | 012)
#define I_FIND (__STR | 013)
#define I_LINK (__STR | 014)
#define I_UNLINK (__STR | 015)
#define I_RECVFD (__STR | 016)
#define I_PEEK (__STR | 017)
#define I_FDINSERT (__STR | 020)
#define I_SENDFD (__STR | 021)
#define I_SWROPT (__STR | 023)
#define I_GWROPT (__STR | 024)
#define I_LIST (__STR | 025)
#define I_PLINK (__STR | 026)
#define I_PUNLINK (__STR | 027)
#define I_FLUSHBAND (__STR | 034)
#define I_CKBAND (__STR | 035)
#define I_GETBAND (__STR | 036)
#define I_ATMARK (__STR | 037)
#define I_SETCLTIME (__STR | 040)
#define I_GETCLTIME (__STR | 041)
#define I_CANPUT (__STR | 042)

#define FMNAMESZ 8

#define S_INPUT 0x0001
#define S_HIPRI 0x0002
#define S_OUTPUT 0x0004
#define S_MSG 0x0008
#define S_ERROR 0x0010
#define S_HANGUP 0x0020
#define S_RDNORM 0x0040
#define S_WRNORM S_OUTPUT
#define S_RDBAND 0x0080
#define S_WRBAND 0x0100
#define S_BANDURG 0x0200

#define FLUSHR 0x01
#define FLUSHW 0x02
#define FLUSHRW 0x03
#define FLUSHBAND 0x04

#define RNORM 0x0000
#define RMSGD 0x0001
#define RMSGN 0x0002
#define RMODEMASK 0x0003
#define RPROTDAT 0x0004
#define RPROTDIS 0x0008
#define RPROTNORM 0x0010
#define RPROTMASK 0x001c

#define SNDZERO 0x0001
#define SNDPIPE 0x0002

#define ANYMARK 0x01
#define LASTMARK 0x02

#define RS_HIPRI 0x01
#define MSG_HIPRI 0x01
#define MSG_ANY 0x02
#define MSG_BAND 0x04

#define MORECTL 1
#define MOREDATA 2

#define INFTIM (-1)
#define MUXID_ALL (-1)

struct strbuf {
	int maxlen;
	int len;
	char *buf;
};

struct strioctl {
	int ic_cmd;
	int ic_timout;
	int ic_len;
	char *ic_dp;
};

struct strpeek {
	struct strbuf ctlbuf;
	struct strbuf databuf;
	uint32_t flags;
};

struct bandinfo {
	unsigned char bi_pri;
	int bi_flag;
};

struct str_mlist {
	char l_name[FMNAMESZ + 1];
};

struct str_list {
	int sl_nmods;
	struct str_mlist *sl_modlist;
};

#ifndef __MLIBC_ABI_ONLY

int getmsg(int __fd, struct strbuf *__restrict __ctlptr,
		struct strbuf *__restrict __dataptr, int *__restrict __flagsp);
int getpmsg(int __fd, struct strbuf *__restrict __ctlptr,
		struct strbuf *__restrict __dataptr, int *__restrict __bandp,
		int *__restrict __flagsp);
int putmsg(int __fd, const struct strbuf *__ctlptr,
		const struct strbuf *__dataptr, int __flags);
int putpmsg(int __fd, const struct strbuf *__ctlptr,
		const struct strbuf *__dataptr, int __band, int __flags);
int isastream(int __fd);

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* _STROPTS_H */
