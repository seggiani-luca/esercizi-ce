.global _Z3sumP2st


.set st_pt, -8 # pointer at -8

# st is a from -8 to -4,
#       b from -4 to 0
.set st_a, 0 # a is at base
.set st_b, 4 # b is at base + 4

_Z3sumP2st:
	pushq %rbp
	movq %rsp, %rbp
	sub $8, %rsp

	# in: st* in %edi (8 byte pointer, not whole structure)
	# out: int in %eax

	movq %rdi, st_pt(%rbp) # means %rbp - st (8)
	
	movq st_pt(%rbp), %rdx # rdx contains st pointer
	
	# dereference a and b, sum
	movl st_a(%rdx), %eax
	addl st_b(%rdx), %eax

	leave
	ret

