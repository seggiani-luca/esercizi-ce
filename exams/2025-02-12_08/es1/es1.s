.global _ZN2clC1E3st1, _ZN2clC1ER3st1Pi

.set cl_v2_1, 0 
.set cl_v2_2, 8 
.set cl_v2_3, 16 
.set cl_v2_4, 24

.set cl_v1_1, 32
.set cl_v1_2, 33
.set cl_v1_3, 34
.set cl_v1_4, 35

.set cl_v3_1, 36
.set cl_v3_2, 37
.set cl_v3_3, 38
.set cl_v3_4, 39

_ZN2clC1E3st1:
	push %rbp
	movq %rsp, %rbp

	// rdi punta a cl
	// rsi e rdx contengono ss.vi su 4 e 4 byte
	
	// espandi ss.vi su esi eax edx ecx

	movq %rsi, %rax 
	shrq $32, %rax

	movq %rdx, %rcx
	shrq $32, %rcx

	// copia ss.v1 * 2 in cl.v1
	movb %sil, cl_v1_1(%rdi)
	movb %al, cl_v1_2(%rdi)
	movb %dl, cl_v1_3(%rdi)
	movb %cl, cl_v1_4(%rdi)

	// raddoppia ss.vi nei registri
	shll $1, %esi
	shll $1, %eax
	shll $1, %edx
	shll $1, %ecx

	// copia ss.vi in cl.v2
	movl %esi, cl_v2_1(%rdi)
	movl %eax, cl_v2_2(%rdi)
	movl %edx, cl_v2_3(%rdi)
	movl %ecx, cl_v2_4(%rdi)
	
	// copia ss.v1 * 2 in cl.v3
	movb %sil, cl_v3_1(%rdi)
	movb %al, cl_v3_2(%rdi)
	movb %dl, cl_v3_3(%rdi)
	movb %cl, cl_v3_4(%rdi)

	leave
	ret

_ZN2clC1ER3st1Pi:
	pushq %rbp
	mov %rsp, %rbp

	// rdi punta a cl
	// rsi punta a s1
	// rdx punta a ar2
	
	// metti s1 in r9d eax r8d ecx
	movl (%rsi), %r9d
	movl 4(%rsi), %eax
	movl 8(%rsi), %r8d
	movl 12(%rsi), %ecx

	// copia s1 in cl.v1
	movb %r9b, cl_v1_1(%rdi)
	movb %al, cl_v1_2(%rdi)
	movb %r8b, cl_v1_3(%rdi)
	movb %cl, cl_v1_4(%rdi)
	
	// quadruplica s1 nei registri
	shll $2, %r9d
	shll $2, %eax
	shll $2, %r8d
	shll $2, %ecx

	// copia s1 * 4 in cl.v1
	movl %r9d, cl_v2_1(%rdi)
	movl %eax, cl_v2_2(%rdi)
	movl %r8d, cl_v2_3(%rdi)
	movl %ecx, cl_v2_4(%rdi)

	// metti s1 in r9d eax r8d ecx
	movl (%rdx), %r9d
	movl 4(%rdx), %eax
	movl 8(%rdx), %r8d
	movl 12(%rdx), %ecx
	
	// copia ar2 in cl.v3
	movb %r9b, cl_v3_1(%rdi)
	movb %al, cl_v3_2(%rdi)
	movb %r8b, cl_v3_3(%rdi)
	movb %cl, cl_v3_4(%rdi)

	leave
	ret
