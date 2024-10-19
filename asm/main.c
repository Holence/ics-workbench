#include "asm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
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

        // TODO: add more tests here.
        asm_longjmp(jmpbuf, 123);
    } else {
        assert(r == 123);
        printf("PASSED.\n");
    }
}
