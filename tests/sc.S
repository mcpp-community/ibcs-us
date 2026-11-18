/* int sc(int nr, int a, int b, int c, int *edx, int *err): one SVR4 i386 system call
 * through the call gate; arguments on the stack beneath a return slot, failure by carry. */
    .text
    .globl sc
sc: pushl %ebp
    movl  %esp, %ebp
    pushl 20(%ebp)
    pushl 16(%ebp)
    pushl 12(%ebp)
    pushl $0
    movl  8(%ebp), %eax
    lcall $7, $0
    jnc   1f
    movl  28(%ebp), %ecx
    movl  %eax, (%ecx)
    movl  $-1, %eax
    jmp   2f
1:  movl  28(%ebp), %ecx
    movl  $0, (%ecx)
2:  movl  24(%ebp), %ecx
    movl  %edx, (%ecx)
    leave
    ret
    .globl _start
_start:
    xorl  %ebp, %ebp
    movl  %esp, %eax
    andl  $-16, %esp
    pushl %eax
    call  cmain
    hlt
