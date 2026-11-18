    .text
    .globl __x86.get_pc_thunk.ax
    .hidden __x86.get_pc_thunk.ax
    .type __x86.get_pc_thunk.ax,@function
__x86.get_pc_thunk.ax:
    movl (%esp),%eax
    ret
    .section .note.GNU-stack,"",@progbits
