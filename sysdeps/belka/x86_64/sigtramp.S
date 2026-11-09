/*
 * Where signal handlers return. The kernel leaves the handler's
 * ucontext_t in %rbx, which the handler preserves; resume it with
 * context(SETCONTEXT, uc), which does not return.
 */
#define SYS_context 100
#define SETCONTEXT 1

	.section .text
	.global __mlibc_belka_sigtramp
	.hidden __mlibc_belka_sigtramp
	.type __mlibc_belka_sigtramp, @function
__mlibc_belka_sigtramp:
	movq %rbx, %rsi
	movl $SETCONTEXT, %edi
	movl $SYS_context, %eax
	syscall
	hlt
	.size __mlibc_belka_sigtramp, . - __mlibc_belka_sigtramp

	.section .note.GNU-stack,"",%progbits
