# LycorisRecompile

## 脚本 & 命令说明

项目根目录`$ROOT`

```bash
# 克隆所有历史比赛仓库 会在 $ROOT/.. 创建并克隆进 RISCV_repos/ ARM_repos/
./scripts/setup/01-clone_repos.sh

# 简单运行并输出AST
./scripts/run/03-simple-run.sh

# 本地git exclude
./scripts/setup/02-git_exclude.sh

# Build compiler
RUSTFLAGS="-A warnings" cargo build

# Build with release optimizations
RUSTFLAGS="-A warnings" cargo build --release

# Run compiler with specific stage
# 注意运行的位置是相对于 compiler/Cargo.toml，意味着要用'../tests'才能访问到测试
RUSTFLAGS="-A warnings" cargo run -- --help
RUSTFLAGS="-A warnings" cargo run -- input.sysy --stage lexer -o output.tokens
RUSTFLAGS="-A warnings" cargo run -- input.sysy --stage parser -o output.ast
RUSTFLAGS="-A warnings" cargo run -- input.sysy -S -o output.s  # Generate assembly (default -O0)
RUSTFLAGS="-A warnings" cargo run -- input.sysy -S -o output.s -O1  # With optimization
```

---

## 时间

- 8月5日 23:59 省赛截止
- 8月5日-8月10日 省赛答辩

## 其他信息

- 比赛官网: https://compiler.educg.net

## 编译&平台参数

使用 rustc 1.85.0 以 `rustc -O2` 进行编译

```bash
rustup toolchain install 1.85.0
```

1. 软核 CPU 配置

    - **架构**：BOOM v3（Berkeley Out-of-Order Machine Version 3）  
    - **指令集**：RISC-V 64GC（RV64IMAFDC）  
    - **主频**：50 MHz  
    - **执行方式**：乱序执行（Out-of-Order Execution）  
    - **发射宽度**：双发射（Dual-issue）  
    - **架构类型**：超标量（Superscalar）  

2. 内存配置

    - **容量**：2 GB  
    - **类型**：DDR4 / AXI 外设总线对接  

3. FPU 配置说明

    BOOM v3 软核内置浮点数运算单元（FPU），支持符合 IEEE 754 标准的单精度（float）和双精度（double）浮点计算，具备以下特性：

    - 支持浮点加法、减法、乘法、除法、平方根运算  
    - 支持浮点乘加累加（FMA）  
    - 支持浮点比较、转换（浮点 → 整数）  
    - 符合 RISC-V F（单精度浮点）和 D（双精度浮点）扩展指令标准  

    > **注**：当前初赛阶段未配备 SIMD 指令扩展，浮点向量化运算暂不支持；决赛阶段可根据开放的 SIMD 指令文档使用。

4. SIMD 指令支持

    - **初赛阶段**：不支持 SIMD 扩展指令  
    - **决赛阶段**：可根据开放的 SIMD 指令文档使用  

5. 运行与调试

    - 编译链接后，目标程序直接在 BOOM CPU 软核上运行  
    - 可通过 GDB + OpenOCD 对软核上运行的目标程序进行调试  

6. 汇编器与链接器

    - **工具链**：`gcc version 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04)`  
    - **示例命令**：  
        ```bash
        gcc -march=rv64gc <输入汇编文件>.s -o <输出可执行文件>
        ```

7. 参赛队编译器

    * 可执行文件名：`compiler`
    * **功能测试**：

    ```bash
    compiler testcase.sysy -S -o testcase.s
    ```
    * **性能测试**：

    ```bash
    compiler testcase.sysy -S -o testcase.s -O1
    ```

8. 核心特性概述

    * **乱序执行调度器**：动态指令重排序，消除指令执行瓶颈
    * **双发射超标量结构**：单周期可发射最多两条指令，提升指令吞吐
    * **寄存器重命名**：消除写后读、写后写数据相关，保障乱序执行正确性
    * **分支预测**：支持动态分支预测，减少流水线停顿
    * **FPU 模块**：支持 IEEE 754 单/双精度浮点操作
    * **MMU 支持**：可选页表管理单元，支持虚拟地址映射

9. 性能评测与应用建议

    * 适用于指令级乱序执行性能评测、浮点密集型算法测试、软核 CPU 架构优化
    * 测试程序需考虑双发射超标量特性，合理优化指令流水
    * 浮点程序可充分利用 FPU 加速，但暂不支持 SIMD 批量数据并行运算


##  省赛评分标准

- 汇编代码要允许在较大地址空间内运行 即遵从 GCC -mcmodel=mandany 选项

- 通过汇编链接后在 64 位 FPGA BOOM CPU 软核上运行

- 初赛总成绩100分权重: 20% 功能测试 + 70% 性能测试 + 10% 设计文档/答辩

- 需要提交的完整内容: 完整工程文件 + 设计文档

- 第三方IP和源码借鉴: 必须在设计文档和源代码的头部予以明确说明, 代码重复率在 50%以上, 取消参赛队的参赛资格

- 人工智能使用: 必须在相关文档、 源码注释或项目说明中予以明确说明, 包括所使用的工具名称、 生成内容的范围及对生成结果所作的人工修改情况等信息

- 允许直接使用: 基于 Lex、 YACC、 Bison、 JavaCC、JavaCUP、 ANTLR 等通用词法、 语法解析器生成工具

- 禁止直接使用: GCC、 LLVM 等现有、 开源编译器与框架的源代码及裁剪

- 等级评定: 
    - 一级: 必过功能用例全部通过, 且性能分 ≥ 90 分, 达到GCC -O2水平
    - 二级: 必过功能用例全部通过, 且 90 >性能分 ≥ 40 分
    - 三级: 未达到二级标准, 但是功能分≥60 分
    - 入门级: 未达到三级标准, 编译通过, 能通过不少于 5 个功能用例

- 编译优化合理性认定: 禁止识别函数名，硬编码计算结果，判断输入模式，利用代码未定义行为

### | 功能测试

功能测试: 所有测试点都未通过计 0 分; 所有测试点都通过计 100 分; 部分测试点通过按所通过测试点的比例计算功能测试得分。参赛队的最终功能测试成绩为每个基准测试程序功能测试成绩的平均值

满分 100 分, 按省赛中给出的功能测试用例计算, 每通过 1% 个功能测试用例, 得 1 分

### | 性能测试

性能测试: 在通过功能测试的前提下, 执行时间最小者分值为 100 分,  其余依据 `性能得分=100 /（运行时间/最短运行时间）`计算性能得分, 参赛队的最终性能测试成绩为每个基准测试程序的性能成绩的平均值

满分 100 分, 基于省赛中给出的性能测试用例计算, 以 GCC 11.2.0 版本的 O2 优化运行时间为基准。
