## 设计文档：segfault 编译器

### 概述

segfault 编译器是一款用于编译 SysY 语言（一种简化的 C-like 语言）到低级代码的工具。该编译器支持多种代码优化和转换阶段，可生成 LLVM IR 或汇编代码。

### 目录结构

以下是 SysYC 编译器的主要目录和文件结构概述：

- **`include/` 和 `src/`**:
  - 这两个目录包含了所有的头文件和源文件。
  - `include/` 子目录按模块组织，如 `ADT/`, `codegen/`, `common/`, `lightir/`, 和 `passes/`。
  - `src/` 目录结构与 `include/` 保持一致，每个头文件夹下有对应的实现文件。

- **`docs/`**:
  - 包含文档和设计说明，例如 `how_init_val_works.py` 展示了初始化复杂高维数组的工作原理。

- **`tests/`**:
  - 包含测试脚本和测试用例。

### 核心组件

#### 1. 抽象语法树（AST）

- **文件位置**：`include/common/ast.hpp`, `src/common/ast.cpp`
- **描述**：构建并维护语言的抽象语法树，是编译过程的初步阶段。

#### 2. IR 构建

- **文件位置**：`include/sysyc/sysyc_builder.hpp`, `src/sysyc/sysyc_builder.cpp`
- **描述**：将 AST 转换为中间表示（IR），为后续的优化和代码生成提供基础。

#### 3. 中间表示（IR）

- **文件位置**：`include/lightir/Module.hpp`, `src/lightir/Module.cpp` 等
- **描述**：定义和操作中间代码表示，为优化和代码生成提供基础。

#### 4. 代码生成

- **文件位置**：`include/codegen/`, `src/codegen/`
- **描述**：
  - 将 IR 转换为目标机器代码。
  - 包括寄存器分配、指令选择、活性分析等子模块。

#### 5. 优化

- **文件位置**：`include/passes/`, `src/passes/`
- **描述**：
  - 实现各种代码优化技术，如常量传播、死码删除、循环优化等。
  - 通过 `PassManager` 管理优化过程。

### 编译流程

1. **语法分析**：解析 `.sy` 输入文件，构建 AST。
2. **IR 构建**：`SysyBuilder` 类将 AST 转换为 IR。
3. **优化**：根据命令行指定的优化级别和优化通道进行 IR 优化。
4. **代码生成**：
   - 如果是生成 LLVM IR，直接输出。
   - 如果是生成汇编代码，进一步通过 `MachineModule` 和 `MachinePassManager` 处理。
5. **输出结果**：将最终生成的代码写入输出文件。

### 构建和使用

使用 CMake 构建系统，可以通过以下命令在根目录编译编译器：

```bash
mkdir build
cd build
cmake ..
sudo make install
```

运行编译器：

```bash
compiler -O1 -emit-llvm example.sy -o example.ll
```

或者
```
compiler -O1 -S example.sy -o example.s
```
