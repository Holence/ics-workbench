#include <stdint.h>
#include <stddef.h>
#include <ucontext.h>

// 不能定义结构体！
// 如果是结构体，那么asm_setjmp(asm_jmp_buf env)将传值传入一个结构体（克隆）
// 而在函数内部修改的是这个克隆体，是无法真正作用到外部的
// typedef struct
// {
//     uint64_t rsp;
//     uint64_t rbx;
//     uint64_t rbp;
//     uint64_t r12;
//     uint64_t r13;
//     uint64_t r14;
//     uint64_t r15;
//     uint64_t rip;
// } asm_jmp_buf;

#define rsp_index 0
#define rbx_index 1
#define rbp_index 2
#define r12_index 3
#define r13_index 4
#define r14_index 5
#define r15_index 6
#define rip_index 7

// 应该定义为array指针
typedef uint64_t asm_jmp_buf[8];

int64_t asm_add(int64_t a, int64_t b);
int asm_popcnt(uint64_t x);
void *asm_memcpy(void *dest, const void *src, size_t n);
int asm_setjmp(asm_jmp_buf env);
void asm_longjmp(asm_jmp_buf env, int val);
