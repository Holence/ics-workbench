先用几个例子搞懂`setjmp`和`longjmp`的工作原理

> Stack Overflow: [Practical usage of setjmp and longjmp in C](https://stackoverflow.com/questions/14685406/practical-usage-of-setjmp-and-longjmp-in-c)
>
> Wikipedia:
>
> A typical use of setjmp/longjmp is implementation of an exception mechanism that exploits the ability of longjmp to reestablish program or thread state, even across multiple levels of function calls. A less common use of setjmp is to create syntax similar to coroutines.

x86 reg table

| Register | Conventional use                | Low 32-bits | Low 16-bits | Low 8-bits |
|----------|---------------------------------|-------------|-------------|------------|
| `%rax`     | callee-owned, Return value      | `%eax`        | `%ax`         | `%al`        |
| `%rdi`     | callee-owned, 1st argument      | `%edi`        | `%di`         | `%dil`       |
| `%rsi`     | callee-owned, 2nd argument      | `%esi`        | `%si`         | `%sil`       |
| `%rdx`     | callee-owned, 3rd argument      | `%edx`        | `%dx`         | `%dl`        |
| `%rcx`     | callee-owned, 4th argument      | `%ecx`        | `%cx`         | `%cl`        |
| `%r8`      | callee-owned, 5th argument      | `%r8d`        | `%r8w`        | `%r8b`       |
| `%r9`      | callee-owned, 6th argument      | `%r9d`        | `%r9w`        | `%r9b`       |
| `%r10`     | callee-owned, Scratch/temporary | `%r10d`       | `%r10w`       | `%r10b`      |
| `%r11`     | callee-owned, Scratch/temporary | `%r11d`       | `%r11w`       | `%r11b`      |
|----------|---------------------------------|-------------|-------------|------------|
| `%rsp`     | caller-owned, Stack pointer     | `%esp`        | `%sp`         | `%spl`       |
| `%rbx`     | caller-owned, Local variable    | `%ebx`        | `%bx`         | `%bl`        |
| `%rbp`     | caller-owned, Local variable    | `%ebp`        | `%bp`         | `%bpl`       |
| `%r12`     | caller-owned, Local variable    | `%r12d`       | `%r12w`       | `%r12b`      |
| `%r13`     | caller-owned, Local variable    | `%r13d`       | `%r13w`       | `%r13b`      |
| `%r14`     | caller-owned, Local variable    | `%r14d`       | `%r14w`       | `%r14b`      |
| `%r15`     | caller-owned, Local variable    | `%r15d`       | `%r15w`       | `%r15b`      |
| `%rip`     | Instruction pointer             |             |             |            |
| `%eflags`  | Status/condition code bits      |             |             |            |

> caller-saved == callee-owned
> 
> callee-saved == caller-owned

```c
// A
int r = setjmp(jmpbuf);
// B 之后longjmp应该恢复的位置
......
......
longjmp(jmpbuf, 123);
```

按照Calling Convention的约定，在`A`处步入`setjmp`时，caller应该已经在栈中保存了callee-owned，进入`setjmp`后，这些寄存器都是可以被callee霍霍的，都是无关紧要的。而caller-owned是不允许霍霍的，是需要callee妥善保护并归还的。

我们想要保存`B`点处的状态机，其实就等价于从一个空的`setjmp`函数中return后的状态，**此时`rsp`恢复为调用前的状态，`rip`为`B`点，其他就是要维持caller-owned不变**。所以`setjmp`只需要正确存储`%rsp`,`%rbx`,`%rbp`,`%r12`,`%r13`,`%r14`,`%r15`和`%rip`就行了。

由于`-O1`（及以上）中`%rbp`被优化了（见[-fomit-frame-pointer](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)），那就又可以少存一个东西了。

```
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
```

`longjmp`就很简单了，恢复所有值即可，并设置返回值到`rax`中（注意`rip`的恢复只能用`jmp`指令实现）

> [!TIP]
> 由于接口是这样的`int asm_setjmp(asm_jmp_buf env);`，为了让`asm_setjmp`能够设置外部的值，`asm_jmp_buf`得定义为指针，而不是结构体！
