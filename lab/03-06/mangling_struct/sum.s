.global _Z3sum2st


# st is a from -8 to -4,
#       b from -4 to 0
.set st, -8 # base at -8
.set st_a, 0 # a is at base
.set st_b, 4 # b is at base + 4

_Z3sum2st:
	pushq %rbp
	movq %rsp, %rbp
	sub $8, %rsp

	# in: st in %edi
	# out: int in %eax

	movq %rdi, st(%rbp) # means %rbp - st (8)
	movl st + st_a (%rbp), %eax
	addl st + st_b (%rbp), %eax

	leave
	ret

