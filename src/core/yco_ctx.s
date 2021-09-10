.global yco_push_ctx, yco_use_ctx, yco_use_ctx_run

.text

yco_push_ctx:
	mov %r15, (%rdi)
	mov %r14, 0x8(%rdi)
	mov %r13, 0x10(%rdi)
	mov %r12, 0x18(%rdi)
	mov %rbx, 0x20(%rdi)
	mov (%rbp), %r11
	mov %r11, 0x28(%rdi) # rbp, rbp points to previous rbp pushed
	lea 0x10(%rbp), %r11
	mov %r11, 0x30(%rdi) # rsp, right before return addr
	mov 0x8(%rbp), %r11
	mov %r11, 0x38(%rdi) # rip, return addr of caller
	retq

yco_use_ctx:
	mov %rdi, %r11

	mov (%r11), %r15
	mov 0x8(%r11), %r14
	mov 0x10(%r11), %r13
	mov 0x18(%r11), %r12
	mov 0x20(%r11), %rbx
	mov 0x28(%r11), %rbp # rpb points to rbp pushed
	mov 0x30(%r11), %rsp # rsp, right before return addr
	mov 0x38(%r11), %rax # rip, return addr of caller
	jmp *%rax

yco_use_ctx_run:
	mov %rdi, %r11
	mov %rsi, %rax

	mov (%rax), %rdi	 # function pointer
	mov 0x8(%rax), %rsi  # input
	mov 0x10(%rax), %rdx # output

	mov (%r11), %r15
	mov 0x8(%r11), %r14
	mov 0x10(%r11), %r13
	mov 0x18(%r11), %r12
	mov 0x20(%r11), %rbx
	mov 0x28(%r11), %rbp # rpb points to rbp pushed
	mov 0x30(%r11), %rsp # rsp, right before return addr
	mov 0x38(%r11), %rax # rip, return addr of caller
	jmp *%rax
