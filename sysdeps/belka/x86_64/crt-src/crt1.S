/* Start-up for static programs: __mlibc_entry(entry stack, main). */
	.section .text
	.global _start
	.type _start, @function
_start:
	xorl %ebp, %ebp
	movq %rsp, %rdi
	leaq main(%rip), %rsi
	call __mlibc_entry
	hlt
	.size _start, . - _start

	.section .note.GNU-stack,"",%progbits
