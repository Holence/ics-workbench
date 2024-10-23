#include "asm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void simple_test() {
    asm_jmp_buf jmpbuf;
    int r = asm_setjmp(jmpbuf);
    int buf[10];
    for (int i = 0; i < 10; i++) {
        buf[i] = i * 123 + 114;
    }

    if (r == 0) {
        assert(asm_add(1234, 5678) == 6912);
        assert(asm_popcnt(0x0123456789abcdefULL) == 32);
        int *buf_copy = malloc(sizeof(int) * 10);
        asm_memcpy(buf_copy, buf, sizeof(int) * 10);
        for (int i = 0; i < 10; i++) {
            assert(buf_copy[i] == i * 123 + 114);
        }
        free(buf_copy);

        asm_longjmp(jmpbuf, 123);
    } else {
        assert(r == 123);
        printf("Simple Test Passed.\n");
    }
}

asm_jmp_buf bufferA, bufferB;
void routineB();

void routineA(int arg) {
    int r;

    r = asm_setjmp(bufferA);
    if (r == 0)
        routineB(514);
    assert(r == 10001);
    assert(arg == 114);

    r = asm_setjmp(bufferA);
    if (r == 0)
        asm_longjmp(bufferB, 20001);
    assert(r == 10002);
    assert(arg == 114);

    r = asm_setjmp(bufferA);
    if (r == 0)
        asm_longjmp(bufferB, 20002);
    assert(r == 10003);
    assert(arg == 114);
}

void routineB(int arg) {
    int r;

    r = asm_setjmp(bufferB);
    if (r == 0)
        asm_longjmp(bufferA, 10001);
    assert(r == 20001);
    assert(arg == 514);

    r = asm_setjmp(bufferB);
    if (r == 0)
        asm_longjmp(bufferA, 10002);
    assert(r == 20002);
    assert(arg == 514);

    r = asm_setjmp(bufferB);
    if (r == 0)
        asm_longjmp(bufferA, 10003);
    assert(r == 20003);
    assert(arg == 514);
}

void simple_coroutines() {
    routineA(114);
    printf("Simple Coroutines Passed.\n");
}

void f(asm_jmp_buf *env, int n) {
    if (n >= 114) {
        asm_longjmp(*env, n);
        assert(0);  // should not be here
    } else {
        f(env, n + 1);
    }
}

void simple_recursion() {
    // also test jmpbuf on heap
    asm_jmp_buf *jmpbuf = malloc(sizeof(asm_jmp_buf));
    int r = asm_setjmp(*jmpbuf);
    if (r == 0) {
        f(jmpbuf, 1);
    } else {
        assert(r == 114);
        printf("Recursion reaches %d\n", r);
    }
    free(jmpbuf);
}

int main() {
    simple_test();
    simple_coroutines();
    simple_recursion();
}
