# CSC2024-NJU-ANTPIE

## 项目简介

该项目为`2024 年全国大学生计算机系统能力大赛编译系统设计赛`项目，实现将SysY语言编译为RISC-V汇编代码的编译器。项目主要分为三个部分：前端、中端和后端。前端负责对SysY源代码进行词法分析和语法分析，并生成中间代码。中端进行一系列优化处理。后端则负责生成高效的RISC-V汇编代码。**项目参考的开源项目和文献已在源码使用处标记。**

## 构建与运行

进入项目根目录，执行：

```bash
make
```

项目构建产物位于目录`build`内。

进入`build`目录，运行`compiler`程序可将SysY源程序编译为RISCV汇编代码（test.s）：

```bash
./compiler -S -o test.s test.sy -O1 
```

## 项目设计

### 前端

- 使用`antlr4`生成`lexer`和`parser`，构建抽象语法树（AST）。

- 采用访问者模式遍历AST，构建中间代码（IR）。

### 中端

#### IR设计

IR结构设计参考了LLVM IR的设计，基本组件包括：

- **Module**: Module是IR的顶级容器，用于表示一个完整的程序或翻译单元。它包含函数、全局变量和相关的元数据。    
- **Function**: Function表示程序中的一个函数，由一系列基本块（BasicBlock）组成。每个函数在Module中唯一标识。
- **BasicBlock**: BasicBlock是IR中的基本执行单元，由一系列指令组成。每个基本块以终结指令（如`br`或`ret`）结束，并且在执行过程中不包含控制流跳转。 
- **Instruction**:  Instruction是IR中的操作指令，类似于机器指令。每个指令可以产生一个值，并作为其他指令的操作数。常见指令包括算术运算（如`add`、`sub`）、内存访问（如`load`、`store`）和控制流指令（如`br`、`ret`）。 
- **Value**:  Value是IR中的数据表示，包括变量、常量和指令的结果。所有的Instruction和Constant都是Value的子类。

#### IR优化

中端实现了各类优化，提升了代码性能。实现的优化包括：

- **MemToReg**: 实现寄存器提升（Promote Memory to Register），减少内存访问。
- **CommonSubexpElimination**: 消除公共子表达式，减少重复计算。
- **MergeBlock**: 合并基本块以简化控制流图。
- **Inlining**: 函数内联，减少函数调用开销。
- **LoopSimplify**: 简化循环结构。
- **LoopInvariantCodeMotion**: 循环不变代码外提。
- **GlobalCodeMotion**: 全局代码移动。
- **GlobalValueNumbering**: 全局值编号，优化冗余计算。
- **LoopUnroll**: 循环展开，提高性能。
- **TailRecursionElimination**: 消除尾递归。
- **DeadCodeElimination**: 死代码消除。
- **ConstantFolding**: 常量折叠，计算常量表达式。
- **GlobalVariableLocalize**: 全局变量本地化，减少全局变量使用。
- **StrengthReduction**: 强度削减，用更低代价运算代替高代价运算。
- **LoadElimination**: 加载消除，减少不必要的内存加载。
- **Reassociate**: 重结合运算。
- **GEPSimplify**: 简化GEP指令。
- **CFGSimplify**: 控制流图简化。
- **StoreElimination.cc**: 存储消除，减少不必要的内存存储。
- **LruCache.cc**: 缓存递归纯函数的结果，避免冗余函数调用。
- **InductionVariableSimplify.cc**: 简化归纳变量。

### 后端

- **Instruction Seletction**: 将中间代码转换为面向目标机器的低层IR。
- **Register Allocation**: 采用SSA分配算法，先溢出以减小寄存器压力，再对寄存器进行着色。
- **Peephole Optimization**: 实施一系列小规模的代码优化，如指令合并和消除冗余指令。
- **Branch Simplification**: 实施和控制流相关的一些优化，如消除冗余跳转指令，基本块排序。
