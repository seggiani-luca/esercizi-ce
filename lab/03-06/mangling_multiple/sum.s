.global _Z3sumP2stS_S_

.set st1, -16
.set st2, -8

.set st_a, 0
.set st_b, 4

_Z3sumP2stS_S_:
	pushq %rbp
	movq %rsp, %rbp
	sub $16, %rsp

	movq %rsi, st1(%rbp)
	movq %rdx, st2(%rbp)

	movl st1 + st_a(%rbp), %eax
	addl st2 + st_a(%rbp), %eax
	
	movl st1 + st_b(%rbp), %ebx
	addl st2 + st_b(%rbp), %ebx

	mov %eax, st_a(%rdi)
	mov %ebx, st_b(%rdi)
	
	leave
	ret
