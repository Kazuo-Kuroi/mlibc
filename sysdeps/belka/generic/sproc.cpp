/*
 * Copyright (c) 2026 Setsuna Software L.C.
 * Created: 2026-11-20
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/prctl.h>

#include <belka/syscall.hpp>
#include <frg/allocation.hpp>
#include <mlibc/allocator.hpp>
#include <mlibc/tcb.hpp>
#include <mlibc/thread.hpp>

extern "C" Tcb *__rtld_allocateTcb();
extern "C" void __rtld_discardTcb(Tcb *);
extern "C" void __rtld_markSprocTcb(Tcb *, unsigned char **);
extern "C" void __rtld_collectSprocTcbs(void (*)(Tcb *));
extern "C" long __mlibc_sproc_call(long, void *, unsigned int, void *, void *, size_t,
		const struct belka_sproc_private *);

namespace {
uint32_t next_sproc_tid;

void release_local_metadata(Tcb *tcb) {
	while (tcb->cxaThreadExitHandlers) {
		auto handler = tcb->cxaThreadExitHandlers;
		tcb->cxaThreadExitHandlers = handler->next;
		frg::destruct(getAllocator(), handler);
	}
	while (!tcb->cleanupHandlers.empty()) {
		auto handler = tcb->cleanupHandlers.pop_back();
		frg::destruct(getAllocator(), handler);
	}
	while (tcb->atforkBegin) {
		auto handler = tcb->atforkBegin;
		tcb->atforkBegin = handler->next;
		frg::destruct(getAllocator(), handler);
	}
	tcb->atforkEnd = nullptr;
}

pid_t create(long number, void *entry, unsigned int mask, void *arg,
		void *top, size_t len) {
	__rtld_collectSprocTcbs(release_local_metadata);
	auto parent_mask = belka::sc(belka::SYS_prctl, PR_GETSHMASK, 0);
	unsigned int inherited = parent_mask.error ? PR_SALL : parent_mask.value;
	bool shared = mask & inherited & PR_SADDR;
	bool runtime = !(mask & PR_NOLIBC);
	Tcb *tcb = mlibc::get_current_tcb();
	struct belka_sproc_private private_data = {tcb, nullptr};

	if (runtime) {
		// Lock ownership is unique within this VM even though each process has LWP 1.
		auto identity = __atomic_load_n(&next_sproc_tid, __ATOMIC_RELAXED);
		for (;;) {
			if (identity == 0x1fffffff) {
				errno = EAGAIN;
				return -1;
			}
			if (__atomic_compare_exchange_n(&next_sproc_tid, &identity,
					identity + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
				break;
		}
		++identity;
		tcb = __rtld_allocateTcb();
		tcb->tid = 0x20000000 | identity;
		tcb->isJoinable = 0;
		private_data.bsp_private = tcb;
		__rtld_markSprocTcb(tcb, &private_data.bsp_exited);
	}
	auto result = __mlibc_sproc_call(number, entry, mask, arg, top, len, &private_data);
	if (runtime && (result < 0 || !shared))
		__rtld_discardTcb(tcb);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return static_cast<pid_t>(result);
}

int block(unsigned int action, pid_t pid, int count) {
	auto result = belka::sc(belka::SYS_procblk, action, pid, count);
	if (result.error) {
		errno = result.error;
		return -1;
	}
	return 0;
}
}

extern "C" [[gnu::visibility("hidden")]]
void __mlibc_collect_sproc() {
	__rtld_collectSprocTcbs(release_local_metadata);
}

extern "C" [[gnu::visibility("hidden")]]
bool __mlibc_sproc_member() {
	auto result = belka::sc(belka::SYS_prctl, PR_GETNSHARE);
	return !result.error && result.value > 0;
}

extern "C" [[gnu::visibility("hidden"), noreturn]]
void __mlibc_sproc_trampoline(void *entry, void *arg, size_t len,
		unsigned int mask, long number, void *top) {
	if (!(mask & PR_NOLIBC)) {
		auto tcb = mlibc::get_current_tcb();
		tcb->stackSize = len;
		tcb->stackAddr = static_cast<char *>(top) - len;
	}
	if (number == belka::SYS_sproc)
		reinterpret_cast<void (*)(void *)>(entry)(arg);
	else
		reinterpret_cast<void (*)(void *, size_t)>(entry)(arg, len);
	exit(0);
}

extern "C" pid_t sproc(void (*entry)(void *), unsigned int mask, ...) {
	va_list args;
	va_start(args, mask);
	auto arg = va_arg(args, void *);
	va_end(args);
	return create(belka::SYS_sproc, reinterpret_cast<void *>(entry), mask,
			arg, nullptr, 0);
}

extern "C" pid_t sprocsp(void (*entry)(void *, size_t), unsigned int mask,
		void *arg, void *top, size_t len) {
	return create(belka::SYS_sprocsp, reinterpret_cast<void *>(entry), mask, arg, top, len);
}

extern "C" ptrdiff_t prctl(unsigned int option, ...) {
	long value = 0;
	va_list args;
	va_start(args, option);
	switch (option) {
	case PR_ISBLOCKED:
	case PR_GETSHMASK:
	case PR_UNBLKONEXEC:
	case PR_SETEXITSIG:
	case PR_SETABORTSIG:
		value = va_arg(args, int);
		break;
	case PR_SETSTACKSIZE:
		value = va_arg(args, size_t);
		break;
	}
	va_end(args);
	auto result = belka::sc(belka::SYS_prctl, option, value);
	if (result.error) {
		errno = result.error;
		return -1;
	}
	return result.value;
}

extern "C" int blockproc(pid_t pid) { return block(0, pid, 0); }
extern "C" int unblockproc(pid_t pid) { return block(1, pid, 0); }
extern "C" int setblockproccnt(pid_t pid, int count) { return block(2, pid, count); }
extern "C" int blockprocall(pid_t pid) { return block(3, pid, 0); }
extern "C" int unblockprocall(pid_t pid) { return block(4, pid, 0); }
extern "C" int setblockproccntall(pid_t pid, int count) { return block(5, pid, count); }
