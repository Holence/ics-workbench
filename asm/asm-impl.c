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

int asm_setjmp(asm_jmp_buf env) {
    return setjmp(env);
}

void asm_longjmp(asm_jmp_buf env, int val) {
    longjmp(env, val);
}
