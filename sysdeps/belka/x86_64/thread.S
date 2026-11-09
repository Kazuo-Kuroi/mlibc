/*
 * Where a new LWP starts (see Clone): PrepareStack left the entry
 * function, the TCB and the argument on its stack, which is 16-byte
 * aligned once they are popped.
 */
	.section .text
	.global __mlibc_thread_entry
	.hidden __mlibc_thread_entry
	.type __mlibc_thread_entry, @function
__mlibc_thread_entry:
	xorl %ebp, %ebp
	popq %rdi
	popq %rsi
	popq %rdx
	call __mlibc_thread_trampoline
	hlt
	.size __mlibc_thread_entry, . - __mlibc_thread_entry

	.section .note.GNU-stack,"",%progbits
