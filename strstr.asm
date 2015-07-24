	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 10
	.globl	_mystrstr
	.align	4, 0x90
_mystrstr:                              ## @mystrstr
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-24(%rbp), %rsi
	cmpb	$0, (%rsi)
	je	LBB0_11
## BB#1:
	jmp	LBB0_2
LBB0_2:                                 ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB0_4 Depth 2
	movq	-16(%rbp), %rax
	cmpb	$0, (%rax)
	je	LBB0_10
## BB#3:                                ##   in Loop: Header=BB0_2 Depth=1
	movl	$0, -28(%rbp)
LBB0_4:                                 ##   Parent Loop BB0_2 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	movq	-16(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movsbl	(%rax,%rcx), %edx
	movq	-24(%rbp), %rax
	movslq	-28(%rbp), %rcx
	movsbl	(%rax,%rcx), %esi
	cmpl	%esi, %edx
	jne	LBB0_9
## BB#5:                                ##   in Loop: Header=BB0_4 Depth=2
	movq	-24(%rbp), %rax
	movslq	-28(%rbp), %rcx
	cmpb	$0, 1(%rax,%rcx)
	jne	LBB0_7
## BB#6:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
	jmp	LBB0_12
LBB0_7:                                 ##   in Loop: Header=BB0_4 Depth=2
	jmp	LBB0_8
LBB0_8:                                 ##   in Loop: Header=BB0_4 Depth=2
	movl	-28(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -28(%rbp)
	jmp	LBB0_4
LBB0_9:                                 ##   in Loop: Header=BB0_2 Depth=1
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	jmp	LBB0_2
LBB0_10:
	movq	$0, -8(%rbp)
	jmp	LBB0_12
LBB0_11:
	movq	-16(%rbp), %rax
	movq	%rax, -8(%rbp)
LBB0_12:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
	.cfi_endproc


.subsections_via_symbols
