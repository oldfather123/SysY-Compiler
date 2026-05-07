# SysY 编译器 by 32bit Brain Storm

SysY 编译器是一个基于 ANTLR4 的编译器，支持 SysY 语言的解析和编译。该编译器使用 C++ 实现，并提供了一些简单的命令行操作来处理 SysY 源代码。

### 项目配置。

> 请确保你已经安装了CMake。
```bash
mysysy/ $ bash setup.sh
``` 

### 常用操作

- 查看帮助信息：
    ```bash
    mysysy/ $ build/bin/sysyc -h
    ```
- 运行并打印IR：
    ```bash
    mysysy/ $ build/bin/sysyc -s ir testdata/functional/21_if_test2.sy
    ```
- 运行并打印汇编码：
    ```bash
    build/bin/sysyc -s asm testdata/functional/21_if_test2.sy
    ```
    或者输出到文件中：
    ```bash
    build/bin/sysyc -S testdata/functional/21_if_test2.sy -o 21_if_test2.s
    ```
- 运行并打印IR（包含调试信息）：
    ```bash
    build/bin/sysyc -s ird testdata/functional/21_if_test2.sy
    ```
- 运行并打印汇编码（包含调试信息）：
    ```bash
    build/bin/sysyc -s asmd testdata/functional/21_if_test2.sy
    ```

### 配套脚本
   （TODO: 需要完善）


### TODO_list:

除开注释中的TODO后续时间充足可以考虑的TODO:

- store load指令由于gep指令的引入, 维度信息的记录是非必须的, 考虑删除

- use def关系经过mem2reg和phi函数明确转换为ssa形式, 以及函数参数通过value数组明确定义, 使得基本块的args参数信息记录非必须, 考虑删除

---

## 编译器后端 TODO 列表

### 1. `CALL` 指令处理不完善 (高优先级)

* **问题描述**：当前 `RISCv64RegAlloc::getInstrUseDef()` 方法中，对 `CALL` 指令的 `use`/`def` 分析不完整。它正确识别了返回值为 `def` 和参数为 `use`，但**没有将所有调用者保存 (Caller-saved) 的物理寄存器（`T0-T6`, `A0-A7`）标记为隐式 `def` (即 `CALL` 会破坏它们)**。
* **潜在后果**：
    * **活跃性分析错误**：寄存器分配器可能会错误地认为某个跨函数调用活跃的虚拟寄存器是安全的，并将其分配给 `T` 或 `A` 寄存器。
    * **值被破坏**：在 `CALL` 指令执行后，这些 `T` 或 `A` 寄存器中本应保留的值会被被调用的函数破坏，导致程序行为异常。
* **参考文件**：`RISCv64RegAlloc.cpp` (在 `getInstrUseDef` 函数中对 `RVOpcodes::CALL` 的处理)。

### 2. `T6` 寄存器作为溢出寄存器的问题 (中等优先级)

* **问题描述**：`RISCv64RegAlloc::rewriteFunction()` 方法中，所有未能成功着色并被溢出 (spilled) 的虚拟寄存器，都被统一替换为物理寄存器 `T6`。
* **问题 2.1：`T6` 是调用者保存寄存器，但未被调用者保存**：
    * `T6` 属于调用者保存寄存器 (`T0-T6` 范围)。
    * 标准 ABI 要求，如果一个调用者保存寄存器在函数调用前后都活跃（例如，它存储了一个被溢出的变量，而这个变量在 `CALL` 指令之后还需要用到），那么**调用者**有责任在 `CALL` 前保存该寄存器，并在 `CALL` 后恢复它。
    * 目前的 `rewriteFunction` 没有为 `T6` 插入这种保存/恢复逻辑。
    * **潜在后果**：如果一个溢出变量被分配到 `T6`，并且它跨函数调用活跃，那么 `putint` 或其他任何被调用的函数可能会随意使用 `T6`，从而破坏该溢出变量的值。
* **问题 2.2：所有溢出变量共用一个 `T6`**：
    * 将所有溢出变量映射到同一个物理寄存器 `T6` 是一种简化的溢出策略。
    * **潜在后果**：这意味着，每当需要使用一个溢出变量时，其值必须从栈中加载到 `T6`；每当一个溢出变量被定义时，其值必须从 `T6` 存储回栈。这会引入大量的 `load`/`store` 指令，并导致 `T6` 本身成为一个高度冲突的寄存器，严重降低代码效率。
* **参考文件**：`RISCv64RegAlloc.cpp` (在 `rewriteFunction` 函数中处理 `spilled_vregs` 的部分)。
