/* The routine of libgcc the objects may refer to. Weak: a compiler that emits
 * its own into an object defines it there, and that one is taken. */
    .text
    .weak __x86.get_pc_thunk.ax
    .hidden __x86.get_pc_thunk.ax
    .type __x86.get_pc_thunk.ax,@function
__x86.get_pc_thunk.ax:
    movl (%esp),%eax
    ret
    .section .note.GNU-stack,"",@progbits
