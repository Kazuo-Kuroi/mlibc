#ifndef _ABIBITS_WAIT_H
#define _ABIBITS_WAIT_H

/*
 * waitpid() and waitid() options and the status word, as the Belka
 * kernel defines them (<sys/wait.h>): the System V values.
 */

#define WEXITED 0x01
#define WTRAPPED 0x02
#define WSTOPPED 0x04
#define WUNTRACED WSTOPPED
#define WCONTINUED 0x08
#define WNOHANG 0x40
#define WNOWAIT 0x80

#define __WCOREFLAG 0x80

#define WEXITSTATUS(x) (((x) & 0xff00) >> 8)
#define WTERMSIG(x) ((x) & 0x7f)
#define WSTOPSIG(x) WEXITSTATUS(x)
#define WIFEXITED(x) (((x) & 0xff) == 0)
#define WIFSIGNALED(x) (((x) & 0xff) != 0 && ((x) & 0xff) != 0x7f && (x) != 0xffff)
#define WIFSTOPPED(x) (((x) & 0xff) == 0x7f && (x) != 0xffff)
#define WIFCONTINUED(x) ((x) == 0xffff)

#if defined(_DEFAULT_SOURCE)
#define WCOREFLAG __WCOREFLAG
#endif
#if defined(_DEFAULT_SOURCE) || __MLIBC_POSIX2024
#define WCOREDUMP(x) ((x) & __WCOREFLAG)
#endif

#if defined(_DEFAULT_SOURCE)
#define W_EXITCODE(ret, sig) (((ret) << 8) | (sig))
#endif

#endif /* _ABIBITS_WAIT_H */
