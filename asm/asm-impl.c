#include "asm.h"
#include <string.h>

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

// -O0，繁文缛节
// |         |
// |---------| <-- rsp + 16 (old rsp, rsp before call asm_setjmp)
// | old rip |
// |---------| <-- rsp + 8
// | old rbp |
// |---------| <-- rsp (rsp after call asm_setjmp)
// |         |

// -O1就没有rbp的破事了
// |         |
// |---------| <-- rsp + 8 (old rsp, rsp before call asm_setjmp)
// | old rip |
// |---------| <-- rsp (rsp after call asm_setjmp)
// |         |

// 所以下面就不管rbp了

int asm_setjmp(asm_jmp_buf env) {
    asm(
        // save old pc
        "mov (%%rsp), %%rax;"  // load old pc to rax
        "mov %%rax, %[rip];"   // save rax to env->rip
        // save old rsp
        "mov %%rsp, %%rax;"
        "add $8, %%rax;"
        "mov %%rax, %[rsp];"
        // others
        "mov %%rbx, %[rbx];"
        "mov %%r12, %[r12];"
        "mov %%r13, %[r13];"
        "mov %%r14, %[r14];"
        "mov %%r15, %[r15];"
        : [rip] "=m"(env[rip_index]),
          [rsp] "=m"(env[rsp_index]),
          [rbx] "=m"(env[rbx_index]),
          [r12] "=m"(env[r12_index]),
          [r13] "=m"(env[r13_index]),
          [r14] "=m"(env[r14_index]),
          [r15] "=m"(env[r15_index])
        :
        : "memory");
    return 0;
}

#include <assert.h>
void asm_longjmp(asm_jmp_buf env, int val) {
    // (uint64_t*)env is in %rdi
    // val is in %rsi
    asm(
        // set rax to return value
        "mov %%rsi, %%rax;"

        // restore others
        "mov %[rsp], %%rsp;"
        "mov %[rbx], %%rbx;"
        "mov %[r12], %%r12;"
        "mov %[r13], %%r13;"
        "mov %[r14], %%r14;"
        "mov %[r15], %%r15;"

        // use callee-owned reg to hold old pc value
        "mov %[rip], %%rcx;"
        // restore rip
        "jmp *%%rcx;"
        :
        : [rip] "m"(env[rip_index]),
          [rsp] "m"(env[rsp_index]),
          [rbx] "m"(env[rbx_index]),
          [r12] "m"(env[r12_index]),
          [r13] "m"(env[r13_index]),
          [r14] "m"(env[r14_index]),
          [r15] "m"(env[r15_index]));

    // should not be here
    assert(0);
}
