.global max

# ci aspettiamo gli argomenti in %rdi, %rsi
# il valore di ritorno lo dobbiamo mettere in %rax
max:
	mov %rsi, %rax
	cmp %rdi, %rsi
	jae skip
	mov %rdi, %rax
skip:
	ret
