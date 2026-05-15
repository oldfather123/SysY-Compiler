# SysY Runtime Libraries

这个目录存放的是比赛官方提供的 SysY 运行时库相关文件，供当前 RISC-V 后端在链接阶段直接使用。

当前仓库已经把 `asm_test.sh` 的默认链接目标切换到了这里的官方静态库，而不再默认使用 `src/asm/sylib.s` 这份手写运行时实现。

## 文件说明

- `libsysy_riscv.a`
  当前默认使用的官方 SysY RISC-V 静态库。`asm_test.sh` 会直接链接它。
- `lib.h`
  官方运行时库的头文件声明，列出了 `getint`、`putint`、`getarray`、`putf`、`starttime`、`stoptime` 等接口原型和宏。
- `lib.c`
  运行时库的一份 C 源码版本，便于阅读接口语义、I/O 格式和计时行为。当前默认测试链路不会重新编译它，而是直接链接上面的官方静态库。

## 这些文件分别在什么时候用

### 1. 编译器前端/语义阶段

编译器并不会真的去 `#include "lib.h"`，但需要理解这些库函数的名字、参数和返回值语义，例如：

- `getint(): int`
- `getfloat(): float`
- `getarray(int[]): int`
- `putfloat(float): void`
- `putf(char[], ...): void`
- `starttime()` / `stoptime()`

其中 `starttime` 和 `stoptime` 在官方头文件里实际上是宏，会展开到：

- `_sysy_starttime(__LINE__)`
- `_sysy_stoptime(__LINE__)`

因此如果你的编译器支持计时相关用例，需要按这个语义去处理。

### 2. 汇编生成阶段

当前后端只负责生成用户程序本身的 RISC-V `.s`。它不会把运行时库函数内联展开，也不会再自己生成一份 `getint/putint` 的实现。

换句话说：

- 后端看到 `putint(x)`，只需要按 ABI 正确传参并 `call putint`
- 后端看到 `getarray(a)`，只需要把数组首地址传出去并取回返回值

真正的函数实体由这里的官方静态库提供。

### 3. 链接阶段

当前默认链路是：

```text
编译器生成的 case.s
  + src/lib/libsysy_riscv.a
  -> RISC-V 可执行文件
```

也就是说，`libsysy_riscv.a` 是现在实际参与测试运行的运行时库。

## 当前脚本如何选择库文件

`src/scripts/asm_test.sh` 的默认策略是：

1. 如果设置了环境变量 `SYSY_LIB`，优先用它；
2. 否则使用 `src/lib/libsysy_riscv.a`。

因此你可以手动切换库文件，例如：

```bash
SYSY_LIB=src/lib/libsysy_riscv.a src/scripts/asm_test.sh test/2026/functional
```

## 与 `src/asm/sylib.s` 的关系

`src/asm/sylib.s` 现在不再是默认测试链路的一部分，但它仍然有保留价值：

- 可以作为早期自定义运行时实现的参考
- 可以用来对照官方库接口
- 在调试某些 ABI 问题时，阅读汇编实现比只看 `.a` 更方便

如果没有特别需要，比赛测试和日常回归建议统一走 `src/lib` 下的官方静态库。
