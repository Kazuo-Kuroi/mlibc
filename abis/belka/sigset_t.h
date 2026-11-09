#ifndef _ABIBITS_SIGSET_T_H
#define _ABIBITS_SIGSET_T_H

/* 128 signals: signal n is bit (n - 1), in 32-bit words. */
typedef struct {
	unsigned int __sig[4];
} sigset_t;

#endif /* _ABIBITS_SIGSET_T_H */
