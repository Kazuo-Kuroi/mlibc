/*
 * Copyright (c) 2026 Setsuna Software L.C.
 * Created: 2026-11-20
 * SPDX-License-Identifier: MIT
 */

/* Preserve the real entry and call kind in the registers the child inherits. */
	.section .text
	.global __mlibc_sproc_call
	.hidden __mlibc_sproc_call
	.type __mlibc_sproc_call, @function
__mlibc_sproc_call:
	pushq %rbx
	pushq %r12
	pushq %r13
	movq %rdi, %r13
	movq %rsi, %rbx
	movq %rdx, %r12
	movq %rcx, %rdx
	movq %r8, %r10
	movq %r9, %r8
	movq 32(%rsp), %r9
	movq %r13, %rax
	leaq __mlibc_sproc_entry(%rip), %rdi
	movq %r12, %rsi
	syscall
	jnc 1f
	negq %rax
1:
	popq %r13
	popq %r12
	popq %rbx
	ret
	.size __mlibc_sproc_call, . - __mlibc_sproc_call

	.type __mlibc_sproc_entry, @function
__mlibc_sproc_entry:
	leaq 8(%rsp), %r9
	movq %r13, %r8
	movq %r12, %rcx
	movq %rsi, %rdx
	movq %rdi, %rsi
	movq %rbx, %rdi
	xorl %ebp, %ebp
	subq $8, %rsp
	call __mlibc_sproc_trampoline
	ud2
	.size __mlibc_sproc_entry, . - __mlibc_sproc_entry
	.section .note.GNU-stack,"",@progbits
