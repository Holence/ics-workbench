#include "asm.h"

int64_t asm_add(int64_t a, int64_t b) {
    asm("add %1, %0"
        : "+r"(b)
        : "r"(a));
    return b;
}

int asm_popcnt(uint64_t x) {
    uint64_t res;
    asm("popcnt %1, %0"
        : "=r"(res)
        : "r"(x));
    return res;
}

void *asm_memcpy(void *dest, const void *src, size_t n) {
    uint8_t temp = 0;
    asm(
        "loop:"
        "cmp $0x0, %[n];"
        "je done;"
        "dec %[n];"
        "mov (%[src], %[n], 1), %[temp];"
        "mov %[temp], (%[dest], %[n], 1);"
        "jmp loop;"
        "done:"
        :
        : [temp] "r"(temp), [dest] "r"(dest), [src] "r"(src), [n] "r"(n));

    // magic
    // asm volatile(
    //     "rep movsb"   // Use the `rep movsb` instruction to move `n` bytes
    //     :             // No output operands (memory is modified implicitly)
    //     : "D"(dest),  // `D` is the destination pointer (rdi register)
    //       "S"(src),   // `S` is the source pointer (rsi register)
    //       "c"(n)      // `c` is the count of bytes to move (rcx register)
    //     : "memory"    // Clobbers memory
    // );

    return dest;
}

// int asm_setjmp(asm_jmp_buf env) {
//     asm(
//         // save old pc
//         "mov (%%rsp), %%rax;"  // load old pc to rax
//         "mov %%rax, %[rip];"   // save rax to env->rip
//         // save old rsp
//         "mov %%rsp, %%rax;"
//         "add $8, %%rax;"
//         "mov %%rax, %[rsp];"
//         // others
//         "mov %%rbx, %[rbx];"
//         "mov %%rbp, %[rbp];"
//         "mov %%r12, %[r12];"
//         "mov %%r13, %[r13];"
//         "mov %%r14, %[r14];"
//         "mov %%r15, %[r15];"
//         : [rip] "=m"(env[rip_index]),
//           [rsp] "=m"(env[rsp_index]),
//           [rbx] "=m"(env[rbx_index]),
//           [rbp] "=m"(env[rbp_index]),
//           [r12] "=m"(env[r12_index]),
//           [r13] "=m"(env[r13_index]),
//           [r14] "=m"(env[r14_index]),
//           [r15] "=m"(env[r15_index])
//         :
//         : "memory");
//     return 0;
// }

// 为了在进入asm_setjmp时，让rbp免受“不可控的编译器优化”影响
// 这里用上面的函数编译出汇编后直接固定贴入，具体解释见README.md
asm("\
.global asm_setjmp\n\
asm_setjmp:\n\
    mov (%rsp),%rax\n\
    mov %rax,0x38(%rdi)\n\
    mov %rsp,%rax\n\
    add $0x8,%rax\n\
    mov %rax,(%rdi)\n\
    mov %rbx,0x8(%rdi)\n\
    mov %rbp,0x10(%rdi)\n\
    mov %r12,0x18(%rdi)\n\
    mov %r13,0x20(%rdi)\n\
    mov %r14,0x28(%rdi)\n\
    mov %r15,0x30(%rdi)\n\
    mov $0x0,%eax\n\
    ret \n\
");

// #include <assert.h>
void asm_longjmp(asm_jmp_buf env, int val) {
    asm(
        // set rax to return value
        // int进来是esi，对应这里也要写成eax而不是rax
        "mov %[val], %%eax;"

        // restore others
        "mov %[rsp], %%rsp;"
        "mov %[rbx], %%rbx;"
        "mov %[rbp], %%rbp;"
        "mov %[r12], %%r12;"
        "mov %[r13], %%r13;"
        "mov %[r14], %%r14;"
        "mov %[r15], %%r15;"

        // use callee-owned reg to hold old pc value
        "mov %[rip], %%rcx;"
        // restore rip
        "jmp *%%rcx;"
        :
        : [val] "rm"(val),
          [rip] "m"(env[rip_index]),
          [rsp] "m"(env[rsp_index]),
          [rbx] "m"(env[rbx_index]),
          [rbp] "m"(env[rbp_index]),
          [r12] "m"(env[r12_index]),
          [r13] "m"(env[r13_index]),
          [r14] "m"(env[r14_index]),
          [r15] "m"(env[r15_index])
        : "eax");

    // should not be here
    // assert(0);
}
