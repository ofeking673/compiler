.data
.balign 8
str0:
	.ascii "%d\n"
	.byte 0
/* end data */

.text
.balign 16
.globl add
add:
	endbr64
	movl %edi, %eax
	addl %esi, %eax
	ret
.type add, @function
.size add, .-add
/* end function add */

.text
.balign 16
.globl main
main:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	subq $40, %rsp
	pushq %rbx
	movl $1, -20(%rbp)
	movl $2, -16(%rbp)
	movl $3, -12(%rbp)
	movl $4, -8(%rbp)
	movl $5, -4(%rbp)
	movl $1, %ebx
.p2align 4
.Lbb4:
	cmpl $10, %ebx
	jg .Lbb6
	imull $4, %ebx, %eax
	movslq %eax, %rcx
	leaq -20(%rbp), %rax
	movq %rax, %rsi
	addq %rcx, %rsi
	leaq str0(%rip), %rdi
	movl $0, %eax
	callq printf
	addl $1, %ebx
	jmp .Lbb4
.Lbb6:
	movl $0, %eax
	popq %rbx
	leave
	ret
.type main, @function
.size main, .-main
/* end function main */

.section .note.GNU-stack,"",@progbits
