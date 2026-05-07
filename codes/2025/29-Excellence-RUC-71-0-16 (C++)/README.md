# SysY编译器系统设计分析文档

## 1. 项目概述

本项目实现了一个完整的SysY语言编译器，SysY是一种类C语言的子集，支持整型和浮点型数据类型、数组、函数、控制流等基本语言特性。编译器采用经典的多阶段设计，从源码生成RISC-V汇编代码，并包含SSA中间代码优化。

### 1.1 支持的语言特性

根据提供的文法规范，编译器支持以下语言特性：

- **数据类型**: `int`、`float`、`void`
- **变量声明**: 常量声明(`const`)和变量声明
- **数组**: 多维数组支持，包括数组初始化
- **函数**: 函数定义、参数传递、函数调用
- **控制流**: `if-else`、`while`、`break`、`continue`、`return`
- **表达式**: 算术运算、逻辑运算、比较运算、函数调用
- **作用域**: 块级作用域支持

### 1.2 系统架构

```
源代码(.sy) → 词法分析 → 语法分析 → 语义分析 → IR生成 → SSA优化 → 代码生成 → RISC-V汇编(.s)
```

## 2. 系统总体设计

### 2.1 编译流程

编译器采用经典的前端-中端-后端架构：

#### 前端 (Frontend)
- **词法分析器**: 使用Flex生成，将源代码转换为Token流
- **语法分析器**: 使用Bison生成，构建抽象语法树(AST)
- **语义分析器**: 进行类型检查、符号表管理、语义约束验证

#### 中端 (Middle-end)
- **IR生成器**: 将AST转换为SSA形式的LLVM风格中间代码
- **SSA优化器**: 进行死代码消除、常量传播等优化

#### 后端 (Backend)
- **代码生成器**: 将中间代码翻译为RISC-V汇编指令
- **寄存器分配器**: 使用图着色算法进行寄存器分配

### 2.2 主要模块

| 模块 | 文件 | 功能描述 |
|------|------|----------|
| 词法分析 | `lexer/lex.l` | Token识别和分类 |
| 语法分析 | `parser/parser.y` | AST构建 |
| AST定义 | `include/ast.h` | 抽象语法树节点定义 |
| 语义分析 | `include/semantic.h`<br>`src/semantic.cpp` | 类型检查、符号表管理 |
| IR生成 | `src/irgenerate.cpp` | 中间代码生成 |
| SSA优化 | `src/ssa_optimizer.cpp` | SSA形式优化 |
| 代码生成 | `src/irtranslater.cpp` | RISC-V代码生成 |
| 寄存器分配 | `src/register_allocator.cpp` | 寄存器分配算法 |

## 3. 详细设计分析

### 3.1 词法分析设计

**实现技术**: Flex (Fast Lexical Analyzer)

**主要特性**:
- 支持C风格注释 (`//` 和 `/* */`)
- 数值常量识别：整数、浮点数（包括科学记数法、十六进制浮点数）
- 关键字识别：`int`、`float`、`void`、`const`、`if`、`else`、`while`等
- 操作符和分隔符识别
- 行号和列号跟踪（用于错误报告）

**Token类型** (根据`parser.tab.h`):
```cpp
enum TokenType {
    CONST, INT, FLOAT, VOID,
    IF, ELSE, WHILE, BREAK, CONTINUE, RETURN,
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, EQ, NE, LT, LE, GT, GE,
    AND, OR, NOT, SEMICOLON, COMMA,
    LBRACE, RBRACE, LPAREN, RPAREN, LBRACKET, RBRACKET,
    IDENTIFIER, INTEGER_LITERAL, FLOAT_LITERAL, STRING_LITERAL
};
```

### 3.2 语法分析设计

**实现技术**: Bison (GNU Parser Generator)

**语法分析特点**:
- LR(1)语法分析器
- 优先级和结合性处理表达式
- 错误恢复机制
- 直接生成AST节点

**AST节点层次结构**:
```
SyntaxNode (抽象基类)
├── CompUnit (编译单元)
├── Decl (声明)
│   ├── ConstDecl (常量声明)
│   └── VarDecl (变量声明)
├── FuncDef (函数定义)
├── Stmt (语句)
│   ├── Block (代码块)
│   ├── IfStmt (if语句)
│   ├── WhileStmt (while语句)
│   ├── AssignStmt (赋值语句)
│   ├── ExpStmt (表达式语句)
│   ├── ReturnStmt (return语句)
│   ├── BreakStmt (break语句)
│   └── ContinueStmt (continue语句)
└── Exp (表达式)
    ├── BinaryExp (二元表达式)
    ├── UnaryExp (一元表达式)
    ├── FunctionCall (函数调用)
    ├── LVal (左值)
    └── Number (数值常量)
```

### 3.3 语义分析设计

**核心组件**:
- **符号表** (`SymbolTable`): 分层作用域管理
- **类型系统** (`Type`): 基本类型和数组类型处理
- **语义检查器** (`SemanticAnalyzer`): 实现ASTVisitor模式

**语义检查内容**:

1. **符号管理**:
   - 变量/函数重定义检查
   - 未定义符号引用检查
   - 作用域规则验证

2. **类型检查**:
   - 赋值兼容性检查
   - 表达式类型推导
   - 函数调用参数匹配
   - 数组访问边界检查

3. **语义约束**:
   - 常量修改检查
   - `break`/`continue`只能在循环中
   - 函数返回值检查
   - `main`函数存在性检查

**关键数据结构**:
```cpp
// 符号表项
class Symbol {
    std::string name;
    std::shared_ptr<Type> type;
    bool is_const;
    SourceLocation definition_location;
    std::optional<std::variant<int, float>> const_value;
};

// 语义错误类型
enum class SemanticErrorType {
    UNDEFINED_SYMBOL, REDEFINED_SYMBOL, TYPE_MISMATCH,
    INVALID_ARRAY_INDEX, INVALID_FUNCTION_CALL,
    BREAK_CONTINUE_ERROR, MISSING_RETURN,
    INVALID_ASSIGNMENT, CONST_MODIFICATION, DIMENSION_MISMATCH
};
```

### 3.4 中间代码生成设计

**IR形式**: SSA (Static Single Assignment) 形式的LLVM风格中间代码

**指令类型**:
- **算术指令**: ADD, SUB, MUL, DIV, MOD, FADD, FSUB, FMUL, FDIV
- **比较指令**: ICMP, FCMP
- **内存指令**: LOAD, STORE, ALLOCA, GETELEMENTPTR
- **控制流指令**: BR_COND, BR_UNCOND, PHI, RET
- **类型转换**: ZEXT, FPTOSI, SITOFP, BITCAST
- **函数调用**: CALL

**基本块结构**:
```cpp
class LLVMBlock {
    int block_id;
    std::string label;
    std::vector<Instruction> instructions;
    std::vector<int> predecessors;   // 前驱块
    std::vector<int> successors;     // 后继块
};
```

**IR生成策略**:
1. **表达式处理**: 递归下降，生成临时变量
2. **控制流处理**: 基本块划分，条件跳转和无条件跳转
3. **函数处理**: 调用约定、参数传递、返回值处理
4. **数组处理**: 地址计算、多维数组展开

### 3.5 SSA优化设计

**优化Pass实现**:

1. **死代码消除** (Dead Code Elimination):
   - 识别未使用的变量和指令
   - 删除无副作用的死指令

2. **常量传播** (Constant Propagation):
   - 编译期计算常量表达式
   - 替换已知常量值

3. **代数简化** (Algebraic Simplification):
   - 强度折减 (如 `x*2` → `x<<1`)
   - 身份元素消除 (如 `x+0` → `x`, `x*1` → `x`)

4. **公共子表达式消除** (Common Subexpression Elimination):
   - 识别重复计算
   - 重用已计算结果

**优化器架构**:
```cpp
class SSAOptimizer {
    bool runDeadCodeElimination(LLVMIR& ir);
    bool runConstantPropagation(LLVMIR& ir);
    bool runAlgebraicSimplification(LLVMIR& ir);
    bool runCommonSubexpressionElimination(LLVMIR& ir);
};
```

### 3.6 代码生成设计

**目标架构**: RISC-V RV32I + RV32F

**关键组件**:

1. **指令选择**:
   - IR指令到RISC-V指令映射
   - 复杂指令分解（如除法、取模）
   - 浮点指令处理

2. **栈帧管理**:
   ```cpp
   struct StackFrameInfo {
       int local_vars_size;      // 局部变量区
       int call_args_size;       // 调用参数区  
       int alignment_padding;    // 16字节对齐填充
       int total_frame_size;     // 总栈帧大小
       std::map<int, int> var_offsets;  // 虚拟寄存器偏移映射
   };
   ```

3. **调用约定**:
   - 参数传递：前8个整数参数使用a0-a7，浮点参数使用fa0-fa7
   - 返回值：整数返回值使用a0，浮点返回值使用fa0
   - 调用者保存/被调用者保存寄存器规范

4. **PHI指令处理**:
   - PHI指令在基本块边界插入复制指令
   - 解决SSA形式到机器码的转换

### 3.7 寄存器分配设计

**算法**: 基于干扰图的图着色算法

**关键步骤**:

1. **活跃性分析** (Liveness Analysis):
   - 数据流分析计算变量活跃区间
   - 构建def-use链

2. **干扰图构建** (Interference Graph):
   - 同时活跃的变量之间连边
   - 处理move指令的特殊情况

3. **图着色** (Graph Coloring):
   - Chaitin算法的简化版本
   - 贪心着色策略

4. **溢出处理** (Spilling):
   - 无法着色的变量溢出到内存
   - 插入load/store指令

**寄存器分类**:
```cpp
// RISC-V寄存器分配
enum RiscvRegType {
    // 通用寄存器
    ZERO = 0,   // 硬连线0
    RA = 1,     // 返回地址
    SP = 2,     // 栈指针
    GP = 3,     // 全局指针
    TP = 4,     // 线程指针
    T0_T6,      // 临时寄存器
    S0_S11,     // 保存寄存器
    A0_A7,      // 参数寄存器
    
    // 浮点寄存器
    FT0_FT11,   // 临时浮点寄存器
    FS0_FS11,   // 保存浮点寄存器
    FA0_FA7     // 参数浮点寄存器
};
```

## 4. 关键技术决策

### 4.1 访问者模式的使用

编译器在AST遍历中大量使用访问者模式，这样设计的优势：

1. **分离算法和数据结构**: AST节点只负责数据存储，算法逻辑在Visitor中
2. **易于扩展**: 新增分析pass只需实现新的Visitor
3. **代码复用**: 相同的AST可以被多个不同的Visitor处理

```cpp
class ASTVisitor {
public:
    virtual void visit(CompUnit& node) = 0;
    virtual void visit(ConstDecl& node) = 0;
    virtual void visit(VarDecl& node) = 0;
    // ... 其他visit方法
};

// 具体实现
class SemanticAnalyzer : public ASTVisitor { /* ... */ };
class IRGenerator : public ASTVisitor { /* ... */ };
class ASTPrinter : public ASTVisitor { /* ... */ };
```

### 4.2 SSA形式的选择

选择SSA中间代码的原因：

1. **优化友好**: 每个变量只定义一次，简化数据流分析
2. **并行化支持**: 明确的数据依赖关系
3. **工业标准**: 与LLVM IR兼容，便于后续扩展
4. **寄存器分配**: 简化活跃性分析

### 4.3 错误处理策略

编译器采用分阶段错误处理：

1. **词法/语法错误**: 立即报告并终止
2. **语义错误**: 收集所有错误后统一报告
3. **延迟检查**: 前向引用等需要延迟验证的项目

## 5. 性能优化策略

### 5.1 编译时优化

1. **符号表优化**: 使用哈希表实现O(1)查找
2. **AST内存管理**: 智能指针自动内存管理
3. **并行编译**: 多文件编译的并行化支持

### 5.2 生成代码优化

1. **窥孔优化**: 局部指令模式匹配优化
2. **尾调用优化**: 函数尾递归优化
3. **循环优化**: 循环不变量外提

## 6. 测试和验证

### 6.1 测试框架

项目包含完整的测试框架：

- `test_batch.sh`: 批量测试脚本
- `test_ir.sh`: 中间代码生成测试
- `test_opt.sh`: 优化效果测试
- `test_SSA_opt.sh`: SSA优化测试
- `test_execution.sh`: 代码执行测试

### 6.2 测试用例分类

- **功能测试** (`tests/functional/`): 基本语言特性
- **性能测试** (`tests/h_functional/`): 复杂算法测试

## 7. 构建系统

**构建工具**: GNU Make

**构建特性**:
- Debug/Release模式切换
- 增量编译支持
- 自动依赖生成
- 并行编译支持

**关键目标**:
```makefile
# 主要构建目标
all: $(TARGET)                  # 构建编译器
clean: ...                      # 清理构建文件
test: ...                       # 运行测试套件
debug: DEBUG=1 all             # Debug版本构建
```

## 8. 可扩展性设计

### 8.1 新语言特性支持

编译器架构支持以下扩展：

1. **新数据类型**: 在类型系统中添加新的BaseType
2. **新语句类型**: 继承Stmt基类并实现相应访问者方法
3. **新优化Pass**: 实现新的优化算法类

### 8.2 新目标架构支持

后端设计支持多目标架构：

1. **指令抽象**: IR层与目标无关
2. **代码生成器模块化**: 独立的翻译器实现
3. **寄存器分配器参数化**: 可配置寄存器数量和约束

## 9. 总结

本编译器实现了一个完整的、现代化的编译系统，具有以下特点：

**技术优势**:
- 模块化设计，各阶段职责清晰
- 使用现代C++特性，代码质量较高  
- SSA中间代码支持高效优化
- 完整的错误处理和报告机制

**实用价值**:
- 支持完整的C语言子集
- 生成高效的RISC-V代码
- 具备良好的可扩展性
- 包含完整的测试验证

