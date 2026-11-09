#include <abi-bits/errno.h>
#include <abi-bits/signal.h>
#include <abi-bits/vm-flags.h>
#include <bits/ensure.h>
#include <stdint.h>
#include <string.h>

#include <belka/syscall.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/arch-defs.hpp>
#include <mlibc/tcb.hpp>
#include <mlibc/thread.hpp>

/*
 * Threads are LWPs. Each starts on a stack PrepareStack made, with its
 * %fs base at its TCB, and is created detached: mlibc's own join waits
 * on the TCB, so the kernel need not keep the LWP for _lwp_wait.
 */

extern "C" void __mlibc_thread_entry();

extern "C" void __mlibc_thread_trampoline(void *(*fn)(void *), Tcb *tcb, void *arg) {
	/* The creator stores our ID after _lwp_create returns. */
	while (__atomic_load_n(&tcb->tid, __ATOMIC_RELAXED) == 0)
		mlibc::sysdep<FutexWait>(&tcb->tid, 0, nullptr);
	tcb->invokeThreadFunc(reinterpret_cast<void *>(fn), arg);
	mlibc::thread_exit(tcb->returnValue);
}

namespace {

constexpr size_t default_stack_size = 0x200000;

/* _lwp_create flags. */
constexpr unsigned long LWP_DETACHED = 0x40;

} // namespace

namespace mlibc {

int Sysdeps<PrepareStack>::operator()(void **stack, void *entry, void *arg, void *tcb,
		size_t *stack_size, size_t *guard_size, void **stack_base) {
	if (!*stack_size)
		*stack_size = default_stack_size;
	*stack_size = (*stack_size + page_size - 1) & ~(page_size - 1);
	if (*stack) {
		*guard_size = 0;
		*stack_base = *stack;
	} else {
		*guard_size = (*guard_size + page_size - 1) & ~(page_size - 1);
		void *base;
		if (int e = sysdep<VmMap>(nullptr, *stack_size + *guard_size,
				PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, &base); e)
			return e;
		/* The guard is the lowest pages, left inaccessible. */
		if (*guard_size) {
			if (int e = sysdep<VmProtect>(base, *guard_size, PROT_NONE); e) {
				sysdep<VmUnmap>(base, *stack_size + *guard_size);
				return e;
			}
		}
		*stack_base = static_cast<char *>(base) + *guard_size;
	}

	auto sp = reinterpret_cast<uintptr_t *>(
			(reinterpret_cast<uintptr_t>(*stack_base) + *stack_size) & ~uintptr_t(15));
	*--sp = reinterpret_cast<uintptr_t>(arg);
	*--sp = reinterpret_cast<uintptr_t>(tcb);
	*--sp = reinterpret_cast<uintptr_t>(entry);
	*stack = sp;
	return 0;
}

/* _lwp_create(ucontext_t *, flags): the new LWP starts in the context given. */
int Sysdeps<Clone>::operator()(void *tcb, pid_t *pid_out, void *stack) {
	ucontext_t uc;

	memset(&uc, 0, sizeof(uc));
	uc.uc_flags = UC_CPU;
	uc.uc_mcontext.gregs[REG_RIP] = reinterpret_cast<uintptr_t>(__mlibc_thread_entry);
	uc.uc_mcontext.gregs[REG_RSP] = reinterpret_cast<uintptr_t>(stack);
	uc.uc_mcontext.gregs[REG_FSBASE] = reinterpret_cast<uintptr_t>(tcb);
	auto r = belka::sc(belka::SYS_lwpcreate, &uc, LWP_DETACHED);
	if (r.error)
		return r.error;
	*pid_out = static_cast<pid_t>(r.value);
	return 0;
}

void Sysdeps<ThreadExit>::operator()() {
	belka::sc(belka::SYS_lwpexit);
	__builtin_unreachable();
}

/*
 * Whether a cancellation signal arrived inside a cancellable system
 * call. None is marked as one yet, so a deferred cancellation waits for
 * the next cancellation point (pthread_testcancel, joins, waits) rather
 * than happening wherever the signal lands, perhaps with a lock held.
 */
int Sysdeps<BeforeCancellableSyscall>::operator()(ucontext_t *) {
	return 0;
}

/* A signal to one LWP of this process; LWP IDs mean nothing in another one. */
int Sysdeps<Tgkill>::operator()(int tgid, int tid, int sig) {
	if (tgid != static_cast<int>(belka::sc(belka::SYS_getpid).value))
		return ESRCH;
	return belka::sc(belka::SYS_lwpkill, tid, sig).error;
}

/*
 * Sleeping: a timed address wait on a word no one wakes. When a signal
 * cuts it short, what is left is worked out from the time of day.
 */
int Sysdeps<Sleep>::operator()(time_t *secs, long *nanos) {
	int word = 0;
	int64_t start[2], now[2];
	struct timespec ts = {*secs, *nanos};

	if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000)
		return EINVAL;
	belka::sc(belka::SYS_gettimeofday, start);
	auto r = belka::syscall(belka::SYS_lwp_addr_wait,
			reinterpret_cast<long>(&word), 0, reinterpret_cast<long>(&ts));
	if (r.error == ETIMEDOUT) {
		*secs = 0;
		*nanos = 0;
		return 0;
	}
	if (r.error != EINTR)
		return r.error ? r.error : EINTR;
	belka::sc(belka::SYS_gettimeofday, now);
	int64_t left = (ts.tv_sec * 1000000000LL + ts.tv_nsec)
			- ((now[0] - start[0]) * 1000000000LL + (now[1] - start[1]) * 1000);
	if (left < 0)
		left = 0;
	*secs = left / 1000000000;
	*nanos = left % 1000000000;
	return EINTR;
}

} // namespace mlibc
