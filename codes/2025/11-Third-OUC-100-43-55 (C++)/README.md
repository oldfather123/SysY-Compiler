# SysY 编译器 - RISC-V 后端实现（2025 编译系统设计赛）

本项目是中国海洋大学/水军出击队参加 **2025 年全国大学生计算机系统能力大赛 - 编译系统设计赛 - 编译系统实现赛** 的初赛作品。

编译器将 **SysY 语言** 编译为 **RISC-V RV64GC** 架构下的汇编代码，涵盖了从词法/语法分析、AST 构建、中间表示(IR)生成与优化、到目标汇编代码生成的完整编译流程

---

## 🛠 编译与使用方式

本项目使用 `CMake` 作为构建系统，支持自动使用项目的 `.l`（Flex）和 `.y`（Bison）文件生成lexer与parser

### 环境依赖

- CMake ≥ 3.10
- Flex
- Bison
- C++17 编译器（g++ 或 clang++）

### Ubuntu 环境下安装依赖示例：

```bash
sudo apt update
sudo apt install cmake flex bison g++ -y
````

### 构建项目

```bash
git clone -b compile https://gitlab.eduxiji.net/T202510423206487/oucc2025.git
cd oucc2025
mkdir build && cd build
cmake ..
make -j
```

生成的可执行文件位于 `build/` 目录下，默认名称为 `compiler2025-0`。

### 使用方式

```bash
./build/compiler2025-0 path/to/source.sy -S -o output.s
```

可将 SysY 源码编译为 RISC-V 汇编代码。

> **提示：** 项目已提交由我们本地生成的 `parser` 和 `lexer` 文件，使用者可选择重新生成或直接使用。

使用-h选项查看完整命令行参数

```bash
./build/compiler2025-0 -h
```

---

## 📁 项目结构

```text
oucc2025/
├── CMakeLists.txt        # 构建配置
├── .clang-format         # 代码格式化配置
├── README.md             # 本文件
├── include/              # 公共头文件目录
├── src/                  # 代码实现
│   ├── parser/           # AST 遍历与 IR 生成
│   ├── ir/               # IR 系统实现
│   ├── pass/             # 优化实现
│   ├── riscv/            # IR 到 RISC-V 汇编生成
│   ├── sysy/             # flex/bison 词法与语法
│   ├── main.cpp          # 程序入口
├── 3rd/                  # fmt 与 CLI 工具
├── generated/            # flex/bison 生成代码
```

---

## 🧠 编译器架构简介

### 前端

* 使用 **Flex/Bison** 构建词法与语法分析器
* 构建抽象语法树（AST）
* 使用 **访问者模式**（`accept` + `visit`双派发实现）遍历 AST 并生成中间表示

### 中间表示（IR）

* 基本结构设计遵循 **LLVM IR** ，可视为其简化子集
* 支持函数、基本块、指令级优化
* 所有 IR 对象使用智能指针管理生命周期（如`std::unique_ptr`）
* 初赛提交优化仅包括：

  * 死代码删除（DCE）
  * 分支优化
  * 常量折叠（CF）
  * 内存提升（Mem2Reg）

### 后端

* 后端使用独立设计的 IR（可称呼为MIR），更接近 RISC-V 汇编
* 后端 IR 初始时符合 **静态单赋值形式**
* 采用 **图着色算法** 进行物理寄存器分配
* 分配后不再满足 SSA 形式，添加出入栈操作即可生成最终的 RISC-V 汇编代码（RV64GC）

---

## 📦 使用的第三方开源工具

| 名称        | 用途              | 链接                                                                         |
| --------- | --------------- | -------------------------------------------------------------------------- |
| **Flex**  | 构建词法分析器         | [https://github.com/westes/flex](https://github.com/westes/flex)           |
| **Bison** | 构建语法分析器         | [https://www.gnu.org/software/bison/](https://www.gnu.org/software/bison/) |
| **fmt**   | 高效的 C++ 字符串格式化库 | [https://github.com/fmtlib/fmt](https://github.com/fmtlib/fmt)             |
| **CLI11** | 命令行参数解析库        | [https://github.com/CLIUtils/CLI11](https://github.com/CLIUtils/CLI11)     |

这些库均为开源项目，已经在主流平台验证稳定性与兼容性。文件内部均标注相应项目copyright
