# 设计报告

## 代码声明

**一、第三方 IP 借鉴声明**

1. 本项目的 IR 结构设计（IR data structures、pass 管理框架等）整体借鉴自 LLVM 项目的设计理念和架构组织；
2. ADCEPass 的全部实现代码，在原理、接口和部分实现细节上直接参考并改编自 LLVM 源码；
3. DominanceInfo（控制流支配树构建）模块的 IDF 计算部分，亦源于 LLVM 原始实现。

**二、人工智能辅助工具使用声明**

在本项目开发中，使用了人工智能辅助编程工具，生成的代码范围及使用方式说明如下：

   * 本项目的大部分代码使用 **GitHub Copilot** 或 **Cursor** 在编辑器中以“编码—提示-补全”模式辅助编写；
   * **Anthropic Claude Code** 生成以下模块的初版代码；
     * 中端全部单元测试
     * IR Pipeline 解析和运行
     * 多个 IR 数据结构类（如 `Value`、`BasicBlock`、`Function` 等）中部分辅助性功能函数（getter/setter、toString、辅助遍历等）
     * IRPrinter
   * **人工审阅与修改**：所有 AI 生成的代码，均由团队成员逐行审阅，针对项目接口规范、性能要求、安全检查等进行必要的补充、重构与单元测试，确保最终代码质量与一致性。

## 整体架构设计

### 编译流程设计

编译器采用经典的三个阶段设计，每个阶段都有明确的输入输出和职责分工。这种设计不仅保证了编译过程的清晰性，还为各个阶段的独立优化和扩展提供了良好的基础。

**前端阶段（Frontend）**：负责将SysY源代码转换为中间表示（IR）。该阶段包括词法分析、语法分析、语义分析和IR生成四个子阶段。词法分析器将源代码转换为token流，语法分析器构建抽象语法树，语义分析器进行类型检查和符号表管理，IR生成器将AST转换为SSA形式的中间表示。

**中端阶段（Midend）**：负责IR的优化和转换。该阶段实现了完整的Pass框架系统，包含多种分析和优化Pass。中端模块采用现代化的IR设计，支持SSA形式和各种高级优化算法。

**后端阶段（Backend）**：负责将优化后的IR转换为目标架构的汇编代码。该阶段包括指令选择、寄存器分配和栈帧管理三个核心步骤。后端模块针对RISC-V 64位架构进行了深度优化，实现了完整的ABI规范和高效的代码生成。

## 前端

### 模块概述

前端模块是编译器的第一个阶段，负责将SysY源代码转换为中间表示（IR）。该模块采用经典的词法分析、语法分析和语义分析三阶段设计，通过Flex/Bison工具链实现高效的语法分析，并构建完整的抽象语法树（AST）用于后续的IR生成。

### 词法分析器（Lexer）

词法分析器基于Flex工具实现，位于`flex_yacc/sysy_flex.l`文件中。该词法分析器能够识别SysY语言的所有词法单元，包括关键字、标识符、常量、运算符和分隔符。词法分析器采用状态机设计，支持单行注释和多行注释的处理，通过正则表达式定义各种词法模式，如整数常量支持十进制、八进制和十六进制表示，浮点数常量支持科学计数法表示。词法分析器还实现了错误恢复机制，能够跳过无法识别的字符并继续分析后续内容。

### 语法分析器（Parser）

语法分析器基于Bison工具实现，位于`flex_yacc/sysy_yacc.y`文件中，采用LALR(1)语法分析方法。该语法分析器定义了完整的SysY语言语法规则，包括变量声明、函数定义、表达式、语句和控制流结构。语法分析器在分析过程中同时构建抽象语法树，每个语法规则都对应AST中的一个节点类型。语法分析器还集成了符号表管理功能，在分析变量声明和函数定义时自动进行符号表的构建和维护。

### 抽象语法树（AST）

AST的实现位于`src/sy_parser/AST.c`中，采用动态内存管理设计，支持节点的动态添加和删除。AST节点包含节点类型、名称、行号、数据类型和子节点列表等信息，通过统一的节点创建和操作接口实现树结构的构建。AST支持多种节点类型，包括表达式节点、语句节点、声明节点和类型节点等，每种节点类型都有相应的数据结构和操作函数。

### 符号表管理

符号表管理系统位于`src/sy_parser/symbol_table.c`中，实现了完整的符号表功能，包括作用域管理、符号查找和类型检查。符号表采用哈希表实现，支持快速符号查找，同时实现了作用域栈来管理嵌套作用域。符号表支持多种符号类型，包括变量、函数、数组和常量，每种符号类型都有相应的属性信息。符号表还实现了函数作用域的特殊管理，支持函数参数的符号表管理。

### IR生成器

IR生成器是前端模块的核心组件，位于`src/ir_gen.cpp`文件中，负责将AST转换为中间表示。IR生成器采用访问者模式设计，通过递归遍历AST节点生成相应的IR指令。IR生成器支持SysY语言的所有语言特性，包括变量声明和初始化、函数定义和调用、表达式计算、控制流语句和数组操作等。IR生成器还实现了类型转换、常量折叠和表达式优化等高级功能，能够生成高质量的中间表示代码。

IR生成器的主要功能包括：变量和数组的IR表示生成、函数定义和调用的IR代码生成、表达式求值的IR指令序列生成、控制流语句的IR基本块生成、以及运行时库函数的集成。IR生成器还实现了复杂的数组初始化处理，支持多维数组的初始化列表解析和IR代码生成。

### 运行时库支持

运行时库支持位于`src/runtime_lib_def.cpp`文件中，提供了SysY语言标准库函数的实现。运行时库包括输入输出函数（如getint、putint、getfloat、putfloat等）、数组操作函数（如getarray、getfarray等）和字符处理函数（如getch、putch等）。这些函数在编译时被自动添加到符号表中，并在IR生成时创建相应的函数声明，确保程序能够正确调用这些标准库函数。

### 设计特点

前端模块采用模块化设计，各个组件之间接口清晰，便于维护和扩展。词法分析器和语法分析器采用成熟的工具链实现，提高了开发效率和代码质量。AST和符号表采用动态内存管理，支持复杂程序结构的处理。IR生成器采用访问者模式，具有良好的扩展性，能够方便地添加新的语言特性支持。整个前端模块还实现了完善的错误处理和调试支持，能够提供详细的编译错误信息和调试输出。

## 中端模块（Midend）

### 模块概述

中端模块是编译器的核心优化阶段，负责中间表示（IR）的定义、分析和优化。该模块采用现代化的IR设计，支持SSA（静态单赋值）形式，并实现了完整的Pass框架系统，提供了丰富的分析和优化Pass。中端模块的设计遵循LLVM IR的设计理念。

### IR设计架构

#### 核心IR类层次结构

中端模块的IR设计采用面向对象的层次结构，所有IR对象都继承自`Value`基类。`Value`类实现了使用-定义链（Use-Def Chain）机制，通过`Use`类维护操作数关系，支持高效的IR变换和优化。核心IR类包括：

- **Module**: 模块级IR，管理全局变量和函数集合，提供模块级别的分析和变换接口
- **Function**: 函数级IR，包含基本块序列和函数属性，支持函数级别的优化
- **BasicBlock**: 基本块，包含线性指令序列，维护控制流图的节点信息
- **Instruction**: 指令基类，定义指令的通用接口和属性
- **Value**: 值基类，所有IR对象（指令、常量、全局变量等）的基类

#### 类型系统设计

中端模块实现了完整的类型系统，支持SysY语言的所有数据类型：

```cpp
enum class TypeKind {
    Void,      // void类型，用于函数返回类型
    Integer,   // 整数类型，支持不同位宽（i1, i32等）
    Float,     // 浮点类型，支持单精度浮点
    Pointer,   // 指针类型，指向其他类型的指针
    Array,     // 数组类型，支持多维数组
    Function,  // 函数类型，包含参数类型和返回类型
    Label,     // 标签类型，用于基本块标识
};
```

类型系统采用继承设计，每种类型都有对应的类型类（如`IntegerType`、`FloatType`等），支持类型查询、类型转换和类型大小计算等功能。

#### IR生成约定和SSA支持

中端模块的IR设计遵循严格的SSA形式约定，确保每个变量只被赋值一次。关键约定包括：

1. **控制流指令约束**: `br`和`br cond`指令只能作为基本块的最后一条指令，确保控制流的正确性
2. **内存分配约束**: `alloca`指令只能在函数的入口基本块中使用，便于后续的内存到寄存器转换优化
3. **PHI节点支持**: 支持PHI节点表示控制流合并点的值选择，是SSA形式的核心组件

### Pass框架系统

#### Pass架构设计

中端模块实现了完整的Pass框架，支持模块级、函数级和基本块级的Pass。Pass框架采用模板方法模式，定义了统一的Pass接口：

```cpp
class Pass {
public:
    enum class PassKind { ModulePass, FunctionPass, BasicBlockPass };
    virtual bool runOnModule(Module& m, AnalysisManager& am) = 0;
    virtual bool runOnFunction(Function& f, AnalysisManager& am) = 0;
    virtual bool runOnBasicBlock(BasicBlock& bb, AnalysisManager& am) = 0;
};
```

#### PassManager和AnalysisManager

PassManager负责Pass的执行调度和依赖管理，支持Pass的串行和并行执行。AnalysisManager管理分析结果的生命周期，实现了分析结果的缓存和失效机制，确保Pass之间的正确协作。

### Transform Passes

#### Mem2RegPass - 内存到寄存器转换

Mem2RegPass是SSA构造的核心Pass，将栈分配的内存访问转换为寄存器访问。该Pass采用经典的SSA构造算法：

1. **可提升性分析**: 识别可以提升到寄存器的alloca指令，排除数组分配和复杂地址计算
2. **PHI节点插入**: 在支配边界插入PHI节点，处理控制流合并点的值选择
3. **SSA构造**: 使用重命名算法将内存访问转换为SSA形式的寄存器访问
4. **清理阶段**: 删除不再需要的alloca和store指令

```cpp
bool Mem2RegContext::runOnFunction(Function& function, AnalysisManager& am) {
    collectPromotableAllocas(function);
    if (promotableAllocas_.empty()) return false;
    
    insertPhiNodes(function);
    performSSAConstruction(function);
    cleanupInstructions();
    return true;
}
```

#### InlinePass - 函数内联优化

InlinePass实现函数内联优化，通过将函数调用替换为函数体来减少函数调用开销。该Pass采用启发式算法：

1. **内联成本计算**: 基于函数大小、调用频率和参数复杂度计算内联成本
2. **内联决策**: 使用阈值控制内联的激进程度，避免代码膨胀
3. **参数映射**: 将函数参数映射到调用点的实际参数
4. **返回处理**: 处理内联函数的返回值，支持多种返回模式

#### ADCEPass - 死代码消除

ADCEPass实现死代码消除，删除对程序输出没有影响的代码。该Pass采用标记-清除算法：

1. **可达性分析**: 从程序入口和输出函数开始标记可达代码
2. **副作用分析**: 识别具有副作用的指令（如函数调用、内存写入）
3. **死代码删除**: 删除未被标记的指令和基本块

#### InstCombinePass - 指令组合优化

InstCombinePass实现指令级优化，通过模式匹配和代数恒等式简化指令序列：

1. **常量折叠**: 在编译时计算常量表达式
2. **强度削弱**: 将复杂运算替换为简单运算
3. **冗余消除**: 删除冗余的指令和表达式
4. **Load-Store 前推**：消除冗余的 load 指令

#### SimplifyCFGPass - 控制流图简化

SimplifyCFGPass简化控制流图，删除冗余的基本块和分支：

1. **空基本块合并**: 合并只包含跳转的空基本块
2. **分支简化**: 简化条件分支，删除不可达分支
3. **跳转链优化**: 优化跳转链，减少跳转开销

#### TailRecursionOptimizationPass - 尾递归优化

TailRecursionOptimizationPass专门处理尾递归优化，将尾递归转换为循环：

1. **尾递归识别**: 识别函数中的尾递归调用
2. **参数映射**: 将递归参数映射为循环变量
3. **循环构造**: 构造等价的循环结构
4. **栈优化**: 减少栈空间使用，避免栈溢出

#### StrengthReductionPass - 强度削弱优化

StrengthReductionPass  将乘法替换为加法，将除法替换为移位.

### 分析Pass（Analysis Passes）

#### DominanceAnalysis - 支配关系分析

DominanceAnalysis计算基本块之间的支配关系，为其他优化Pass提供基础信息：

1. **支配计算**: 使用迭代算法计算每个基本块的支配者集合
2. **立即支配**: 计算立即支配者关系，构建支配树
3. **支配边界**: 计算支配边界，用于PHI节点插入
4. **后支配分析**: 支持后支配关系分析，用于死代码消除

#### LoopInfo - 循环信息分析

LoopInfo分析程序中的循环结构，为循环优化提供支持：

1. **循环检测**: 使用深度优先搜索检测自然循环
2. **循环层次**: 构建循环的嵌套层次结构
3. **循环属性**: 计算循环的预头、退出块、回边等属性
4. **循环变换**: 支持循环的变换和优化

#### CallGraphAnalysis - 调用图分析

CallGraphAnalysis构建程序的调用图，分析函数间的调用关系：

1. **调用图构建**: 分析函数调用关系，构建调用图
2. **强连通分量**: 计算调用图中的强连通分量，处理递归
3. **内联分析**: 为内联优化提供调用关系信息
4. **副作用识别**: 识别函数是否具有副作用

#### AliasAnalysis - 别名分析

AliasAnalysis分析内存访问的别名关系，为优化提供精确的内存访问信息：

1. **别名关系**: 分析指针和数组的别名关系
2. **内存依赖**: 计算内存访问之间的依赖关系
3. **优化支持**: 为内存优化提供精确的依赖信息

## 后端模块（RISC-V Backend）

### 模块概述

后端模块是编译器的最终阶段，负责将优化后的中间表示（IR）转换为RISC-V 64位汇编代码。该模块采用经典的三阶段编译架构，实现了完整的指令选择、寄存器分配和栈帧管理功能。后端模块针对RISC-V 64位架构进行了深度优化，支持RV64I基础指令集和浮点扩展，并实现了完整的ABI（应用程序二进制接口）规范。

### 三阶段编译架构

#### 指令选择阶段（Instruction Selection）

指令选择阶段将IR指令转换为目标架构的机器指令序列。该阶段采用访问者模式实现，通过`Visitor`类遍历IR模块，为每种IR指令生成对应的RISC-V指令序列。指令选择器位于`src/Visit.cpp`文件中，实现了完整的IR到RISC-V指令的映射。

指令选择器的主要功能包括：算术运算指令的转换（支持整数和浮点运算）、内存访问指令的生成（load/store指令）、控制流指令的处理（分支和跳转指令）、函数调用约定的实现（参数传递和返回值处理）。指令选择器还实现了复杂的地址计算优化，能够生成高效的基址+偏移量寻址模式，并处理立即数范围限制。

```cpp
// 指令选择的核心接口
class RISCV64Target {
public:
    std::string compileToAssembly(const midend::Module& module);
    
    // 三阶段编译流程
    Module instructionSelectionPass(const midend::Module& module);  // 阶段1：指令选择
    Module& initialFrameIndexPass(riscv64::Module& module);         // 阶段1.5：初始Frame Index
    Module& registerAllocationPass(riscv64::Module& module);        // 阶段2：寄存器分配
    Module& frameIndexEliminationPass(riscv64::Module& module);     // 阶段3：Frame Index消除
};
```

#### 寄存器分配阶段（Register Allocation）

寄存器分配阶段采用经典的Chaitin图着色算法，将虚拟寄存器映射到物理寄存器。该阶段位于`src/RegAllocChaitin.cpp`文件中，包含1800行代码，实现了完整的图着色寄存器分配器。

寄存器分配器的主要功能包括：活跃性分析（计算变量的生命周期）、干涉图构建（分析寄存器间的冲突关系）、图着色算法（为虚拟寄存器分配物理寄存器）、寄存器合并优化（消除冗余的寄存器拷贝）、溢出处理（当物理寄存器不足时将变量溢出到栈上）。寄存器分配器还实现了智能的溢出策略，能够最小化溢出开销并优化溢出代码的生成。

```cpp
void RegAllocChaitin::allocateRegisters() {
    computeLiveness();           // 计算活跃性信息
    buildInterferenceGraph();    // 构建干涉图
    performCoalescing();         // 执行寄存器合并
    bool success = colorGraph(); // 图着色分配
    
    if (!success) {
        handleSpills();          // 处理溢出
        allocateRegisters();     // 重新分配
        return;
    }
    
    removeCoalescedCopies();     // 移除合并的拷贝
    rewriteInstructions();       // 重写指令
}
```

#### Frame Index消除阶段（Frame Index Elimination）

Frame Index消除阶段负责最终的栈帧布局和地址计算优化。该阶段位于`src/FrameIndexElimination.cpp`文件中，实现了完整的栈帧管理和地址计算优化。

Frame Index消除器的主要功能包括：栈帧布局计算（为局部变量、溢出寄存器和保存寄存器分配栈空间）、地址计算优化（将Frame Index转换为基址+偏移量的形式）、栈帧对齐处理（确保栈帧满足架构对齐要求）、函数序言和尾声生成（处理栈帧的建立和销毁）。该阶段还实现了复杂的地址计算优化，能够处理大偏移量的分解和基址寄存器的选择。

### 机器指令系统设计

#### RISC-V指令集支持

后端模块实现了完整的RISC-V 64位指令集支持，包括RV64I基础整数指令集、RV64M乘除法扩展、RV64F单精度浮点扩展和RV64D双精度浮点扩展。指令系统采用枚举类型定义，支持所有RISC-V标准指令和伪指令：

```cpp
enum Opcode {
    // RV64I Base Integer Instruction Set
    ADD, SUB, SLL, SLT, SGT, SLTU, XOR, SRL, SRA, OR, AND,
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    LB, LH, LW, LD, LBU, LHU, LWU, SB, SH, SW, SD,
    BEQ, BNE, BLT, BGE, BLTU, BGEU, LUI, AUIPC, JAL, JALR,
    
    // RV64M Standard Extension
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,
    
    // RV64F Standard Extension
    FLW, FSW, FADD_S, FSUB_S, FMUL_S, FDIV_S, FSQRT_S,
    FCVT_W_S, FCVT_S_W, FEQ_S, FLT_S, FLE_S,
    
    // Pseudo Instructions
    LI, MV, NOT, NEG, SEQZ, SNEZ, BEQZ, BNEZ, J, JR, RET, CALL
};
```

#### 机器操作数系统

机器操作数系统支持多种操作数类型，包括寄存器操作数、立即数操作数、标签操作数、内存操作数和Frame Index操作数。每种操作数类型都有对应的类实现，支持类型安全的操作数处理：

```cpp
enum class OperandType {
    Register,    // 寄存器操作数
    Immediate,   // 立即数操作数
    Label,       // 标签操作数
    Memory,      // 内存地址操作数
    FrameIndex,  // 栈帧索引操作数
};

class MachineOperand {
public:
    virtual ~MachineOperand() = default;
    OperandType getType() const { return type; }
    virtual std::string toString() const = 0;
};
```

### ABI实现和调用约定

#### RISC-V 64位ABI规范

后端模块实现了完整的RISC-V 64位ABI规范，包括寄存器命名约定、函数调用约定和栈帧布局规范。ABI实现位于`src/ABI.cpp`文件中，定义了完整的寄存器映射和调用约定。

ABI实现的主要功能包括：寄存器命名映射（支持ABI名称和数字编号的转换）、调用约定实现（参数传递、返回值处理和寄存器保存约定）、寄存器分类管理（调用者保存寄存器、被调用者保存寄存器、参数寄存器和返回值寄存器）。ABI还实现了浮点寄存器的特殊处理，支持浮点参数的传递和浮点运算的寄存器分配。

```cpp
// ABI寄存器映射示例
namespace riscv64::ABI {
    // 根据ABI名称获取寄存器编号
    unsigned getRegNumFromABIName(const std::string& name);
    // 根据寄存器编号获取ABI名称
    std::string getABINameFromRegNum(unsigned num); 
    // 判断是否为调用者保存寄存器 (Caller-saved/Temporary registers)
    bool isCallerSaved(unsigned physreg, bool isFloat);
    // 判断是否为被调用者保存寄存器 (Callee-saved/Saved registers)
    bool isCalleeSaved(unsigned physreg, bool isFloat);
    // 判断是否为参数寄存器 (Argument registers)
    bool isArgumentReg(unsigned physreg, bool isFloat);
    // 判断是否为返回值寄存器 (Return value registers)
    bool isReturnReg(unsigned physreg, bool isFloat);
    bool isReservedReg(unsigned physreg, bool isFloat);
    std::vector<unsigned> getCallerSavedRegs(bool isFloat);
}
```

#### 函数调用约定实现

函数调用约定实现了完整的参数传递和返回值处理机制。整数参数通过a0-a7寄存器传递，浮点参数通过fa0-fa7寄存器传递，超出寄存器数量的参数通过栈传递。返回值通过a0/a1寄存器（整数）或fa0/fa1寄存器（浮点）返回。

调用约定还实现了寄存器保存和恢复机制，确保函数调用不会破坏调用者的寄存器状态。被调用者保存寄存器（s0-s11, fs0-fs11）在函数入口时保存，在函数出口时恢复。调用者保存寄存器（t0-t6, ft0-ft7）由调用者负责保存。

### 栈帧管理和内存布局

#### 栈帧布局设计

栈帧管理器实现了完整的栈帧布局功能，支持局部变量、溢出寄存器、保存寄存器和函数参数的栈空间分配。栈帧布局采用自顶向下的设计，确保所有栈对象都能正确对齐和访问。

栈帧布局的主要特点包括：16字节对齐的栈帧大小、自顶向下的栈对象分配、基址寄存器（s0/fp）指向栈帧顶部、栈指针（sp）指向栈帧底部。栈帧管理器还实现了复杂的对齐处理，确保不同大小的栈对象都能满足架构的对齐要求。

#### 地址计算优化

地址计算优化器实现了高效的栈地址计算，将Frame Index转换为基址+偏移量的形式。优化器能够处理大偏移量的分解，将超出立即数范围的偏移量分解为多个指令序列。优化器还实现了基址寄存器的智能选择，能够选择最合适的基址寄存器来最小化指令数量。

### 代码生成和汇编输出

汇编代码生成器位于`src/Printer.cpp`文件中，负责将机器指令转换为可读的RISC-V汇编代码。代码生成器支持所有RISC-V指令的文本表示，包括指令名称、操作数和注释的格式化输出。

代码生成器的主要功能包括：指令名称映射（将操作码转换为汇编助记符）、操作数格式化（支持寄存器、立即数、标签和内存地址的文本表示）、段管理（支持代码段、数据段和只读数据段的输出）。

### 调试和错误处理

#### 调试信息生成

后端模块支持调试信息的生成，包括优化的具体流程、符号表信息、行号信息和变量位置信息。调试信息能够帮助开发者理解生成的汇编代码与源代码的对应关系，便于程序的调试和优化。

#### 错误检测和报告

后端模块实现了错误检测和报告机制，能够检测寄存器分配失败、栈帧溢出、指令选择错误等问题。错误报告系统能够提供详细的错误信息，帮助开发者快速定位和解决问题。

### 性能优化特性

#### 寄存器分配优化

寄存器分配器实现了多种优化策略，包括寄存器合并优化、活跃范围分析和智能溢出处理。寄存器合并优化能够消除冗余的寄存器拷贝指令，活跃范围分析能够精确计算变量的生命周期，智能溢出处理能够最小化溢出开销。

#### 指令选择优化

指令选择器实现了多种优化策略，包括常量折叠、地址计算优化和指令组合优化。常量折叠能够在编译时计算常量表达式，地址计算优化能够生成高效的寻址模式，指令组合优化能够将多个简单指令组合为复杂指令。

#### 栈帧优化

栈帧管理器实现了多种优化策略，包括栈对象合并、对齐优化和访问模式优化。栈对象合并能够将相邻的小对象合并为大对象以减少栈空间使用，对齐优化能够确保栈对象满足架构对齐要求，访问模式优化能够生成高效的栈访问指令序列。

#### 值重用优化

值重用优化 Pass 实现了多种优化策略，在指令选择不分完成后，通过遍历支配树，消除指令选择阶段重复的立即数加载、内存加载等，将重复的加载转化为拷贝指令，大幅提高生成汇编代码的性能。
