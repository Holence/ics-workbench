先用几个例子搞懂`setjmp`和`longjmp`的工作原理

> Stack Overflow: [Practical usage of setjmp and longjmp in C](https://stackoverflow.com/questions/14685406/practical-usage-of-setjmp-and-longjmp-in-c)
>
> Wikipedia:
>
> A typical use of setjmp/longjmp is implementation of an exception mechanism that exploits the ability of longjmp to reestablish program or thread state, even across multiple levels of function calls. A less common use of setjmp is to create syntax similar to coroutines.

x86-64 reg table

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

我们想要保存`B`点处的状态机，其实就等价于从一个空的`setjmp`函数中return后的状态，**此时`rsp`、`rbp`恢复为调用前的状态，`rip`为`B`点，其他就是要维持caller-owned不变**。所以`setjmp`只需要正确存储`%rsp`,`%rbx`,`%rbp`,`%r12`,`%r13`,`%r14`,`%r15`和`%rip`就行了。

`%rbp`的情况很特殊，x86的传统是进入函数（进入`setjmp`）会`push %rbp; mov %rsp,%rbp;`，但在`-O1`（及以上）中**一部分**函数中，这些又会被优化（见[-fomit-frame-pointer](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)），这些函数中就不存在"frame-pointer"的东西了，`%rbp`寄存器可能被编译器作为普通寄存器使用。

`B`点处的`%rbp`和`A`点处一样，但现在却因为编译器不同，导致进入`setjmp`后，需要存储的`%rbp`不知道在哪儿，可能`%rbp`被`push`到栈上了，可能`%rbp`还在寄存器里：

```
x86-64

-O0，繁文缛节
|         |
|---------| <-- rsp + 16 (old rsp, rsp before call asm_setjmp)
| old rip |
|---------| <-- rsp + 8
| old rbp |
|---------| <-- rsp (rsp after call asm_setjmp)
|         |

-O1以上的部分函数
|         |
|---------| <-- rsp + 8 (old rsp, rsp before call asm_setjmp)
| old rip |
|---------| <-- rsp (rsp after call asm_setjmp)
|         |
```

所以想要确定`%rbp`里的值，就得让编译器编译出一种确定的`setjmp`。

> 好像`__attribute__((naked))`是干这种事的，但不知道为什么函数的最后会生成`ud2`（不明指令）❓

我是先用Extended Asm写的，然后用`-O1`编译出不含`push %rbp; mov %rsp,%rbp;`的版本，然后直接把编译出的汇编函数贴到c文件里，然后用[`.global`]((https://sourceware.org/binutils/docs/as/Global.html))声明一下。

如果要追求可移植性，那就`#if`判断CPU架构，然后定义不同的呗。

> [!TIP]
> 由于接口是这样的`int asm_setjmp(asm_jmp_buf env);`，为了让`asm_setjmp`能够设置外部的值，`asm_jmp_buf`得定义为指针，而不是结构体！

`longjmp`就很简单了，恢复所有值即可，并设置返回值到`rax`中（注意`rip`的恢复只能用`jmp`指令实现）

> [!TIP]
> `asm_longjmp`的`__attribute__((noreturn))`十分重要，如果不加的话会在O2及以上的优化时发生很微妙的bug，见[我的libco](https://github.com/Holence/os-workbench-2024/tree/main/libco)

`main.c`中的测试并不完整，涉及到栈切换的测试见[我的libco](https://github.com/Holence/os-workbench-2024/tree/main/libco)。
