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

