.data
.balign 8
str0:
	.ascii "%d + %d is: %d\n"
	.byte 0
/* end data */

.text
.balign 16
.globl add
add:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movl %esi, %edx
	movl %edi, %esi
	movl %esi, %ecx
	addl %edx, %ecx
	leaq str0(%rip), %rdi
	movl $0, %eax
	callq printf
	movl $0, %eax
	leave
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
	movl $6, %esi
	movl $5, %edi
	callq add
	movl $0, %eax
	leave
	ret
.type main, @function
.size main, .-main
/* end function main */

.section .note.GNU-stack,"",@progbits
