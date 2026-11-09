#include <abi-bits/errno.h>
#include <abi-bits/signal.h>

#include <belka/syscall.hpp>
#include <mlibc/all-sysdeps.hpp>

/* Where every handler returns (x86_64/sigtramp.S). */
extern "C" void __mlibc_belka_sigtramp();

namespace mlibc {

/*
 * sigaction(sig, act, oact, trampoline): System V passes the address
 * handlers return to with every call.
 */
int Sysdeps<Sigaction>::operator()(int sig, const struct sigaction *__restrict act, struct sigaction *__restrict oact) {
	return belka::sc(belka::SYS_sigaction, sig, act, oact, __mlibc_belka_sigtramp).error;
}

/* The signal mask belongs to the calling LWP. */
int Sysdeps<Sigprocmask>::operator()(int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve) {
	return belka::sc(belka::SYS_sigprocmask, how, set, retrieve).error;
}

int Sysdeps<ThreadSigmask>::operator()(int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve) {
	return sysdep<Sigprocmask>(how, set, retrieve);
}

int Sysdeps<Sigsuspend>::operator()(const sigset_t *set) {
	return belka::sc(belka::SYS_sigsuspend, set).error;
}

int Sysdeps<Sigpending>::operator()(sigset_t *set) {
	return belka::sc(belka::SYS_sigpending, belka::SIGPENDING_PENDING, set).error;
}

int Sysdeps<Sigaltstack>::operator()(const stack_t *ss, stack_t *oss) {
	return belka::sc(belka::SYS_sigaltstack, ss, oss).error;
}

} // namespace mlibc
