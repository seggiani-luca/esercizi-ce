.global _Z3sumP2stS_S_

# structure offsets
.set st1, -16
.set st2, -8

# offset within structures (a and b)
.set st_a, 0
.set st_b, 4

_Z3sumP2stS_S_:
	pushq %rbp
	movq %rsp, %rbp
	
	push %rax
	push %rbx

	# init 16 bytes for structures 
	# we'll read pointed structure straight from memory
	sub $16, %rsp

	# move structure 1 into st1
	movq %rsi, st1(%rbp)
	# move structure 2 into st2
	movq %rdx, st2(%rbp)

	# %eax holds a, move structure 1's a there
	movl st1 + st_a(%rbp), %eax
	# sum structure 2's a to it
	addl st2 + st_a(%rbp), %eax
	
	# same as above
	movl st1 + st_b(%rbp), %ebx
	addl st2 + st_b(%rbp), %ebx

	# pointed structure is in %rdi, move %eax and %ebx
	# at the correct offsets
	mov %eax, st_a(%rdi)
	mov %ebx, st_b(%rdi)

	pop %rbx
	pop %rax
	
	leave
	ret
