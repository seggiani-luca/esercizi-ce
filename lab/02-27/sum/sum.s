.global sum

sum:
	movq %rdi, %rax
	addq %rsi, %rax
	ret
