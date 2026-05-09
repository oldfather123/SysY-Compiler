# Scripts

本目录存放项目中的辅助脚本，当前包括运行结果测试脚本和 AST 对比脚本。所有输入路径必须位于仓库根目录下的 `test/` 目录内。

## 1. `test.sh`

用于运行 SysY 程序，收集实际输出，并与对应的标准输出文件进行对比。

### 用法

运行一个测试目录：

```bash
src/scripts/test.sh test/2026/functional
```

运行单个测试文件：

```bash
src/scripts/test.sh test/2026/functional/00_main.sy
```

可通过 `RUN_SY_TIMEOUT` 修改单个测试用例的超时时间，默认值为 `25s`：

```bash
RUN_SY_TIMEOUT=60s src/scripts/test.sh test/2026/functional
```

### 输出目录

脚本会把结果输出到：

```text
src/test_output/<target-dir>/
```

其中包括：

- `ast/<case>.ast`：编译器导出的 AST
- `out/<case>.out`：程序实际输出
- `err/<case>.err`：标准错误输出（如有）
- `diff/<case>.diff`：和标准输出的差异（如有）

## 2. `ast_diff.sh`

用于生成 Clang AST、SysY AST，并对规范格式化后的两份 AST 进行 diff，便于观察二者在语义结构上的差异。

### 用法

运行一个测试目录：

```bash
src/scripts/ast_diff.sh test/2026/functional
```

运行单个测试文件：

```bash
src/scripts/ast_diff.sh test/2026/functional/02_var_defn3.sy
```

查看没有规范格式的真实 diff：

```bash
src/scripts/ast_diff.sh test/2026/functional true
```

可通过 `AST_DIFF_TIMEOUT` 修改单个测试用例的超时时间，默认值为 `300s`：

```bash
AST_DIFF_TIMEOUT=60s src/scripts/ast_diff.sh test/2026/functional
```

### 输出目录

脚本会把结果输出到：

```text
src/test_output/<target-dir>/
```

其中包括：

- `ast_clang/<case>.ast`：`clang -Xclang -ast-dump` 生成的 AST
- `ast/<case>.ast`：编译器 `--dump-ast` 生成的 AST
- `ast_diff/<case>.diff`：两份 AST 的差异

## AST 对比说明

Clang AST 和 SysY AST 都是真实的抽象语法树，但二者的输出格式、字段组织方式和部分中间语义表示并不完全一致。为便于比较语义结构，`ast_diff.sh` 会在比较阶段进行规范格式处理，避免无意义的差异：

- 移除 Clang AST 行列位置信息；
- 仅省略 Clang 为 C 语义、预处理或 ABI 引入、但在 SysY 源语言中没有直接对应的包装节点；
- SysY AST 仅构造自身设计必需的字段，而两类 AST 中语义等价但表达形式不同的字段会做统一。

具体统一包括：

- SysY AST 不保留 `used`、`referenced` 与 `cinit`：前两者是 Clang 的声明引用状态标记，后者只是初始化器的展示标记，三者都不属于 SysY 语言语义。初始化语义已由 `initializer` 子树表达，因此 diff 阶段会从 Clang 临时输出中剥离这些标记。

```text
Clang：
    -|-VarDecl used b 'int' cinit
    -|-FunctionDecl referenced search 'int (int, int)'
SysY：
    +|-VarDecl b 'int'
    +|-FunctionDecl search 'int (int, int)'
```

- Clang 会把函数调用中的 callee 展开为 `FunctionToPointerDecay` 和函数 `DeclRefExpr` 组成的 callee 链；SysY 不支持函数指针语义，因此 diff 阶段会把这部分折叠到 `CallExpr` 的函数名字段中，并继续保留后续实参节点参与比较。

```text
Clang：
    -| `-CallExpr 'int'
    -|   |-ImplicitCastExpr 'int (*)(int)' <FunctionToPointerDecay>
    -|   | `-DeclRefExpr 'int (int)' Function 'func' 'int (int)'
    -|   `-IntegerLiteral 'int' 1
SysY：
    +| `-CallExpr 'int' 'func'
    +|   `-IntegerLiteral 'int' 1
```

- Clang 会按 C 语言规则把未带后缀的浮点字面量记为 `double`，再在需要时插入 `FloatingCast`；SysY 只有 `float`，因此 diff 阶段会把 Clang 侧的 `double` 归一化为 `float`，并折叠仅用于承接这类 C 浮点中间语义、但包裹的子表达式本身已经是 `float` 的 `FloatingCast`。

```text
Clang：
    -| `-ImplicitCastExpr 'float' <FloatingCast>
    -|   `-FloatingLiteral 'double' 5.500000e+00
SysY：
    +| `-FloatingLiteral 'float' 5.500000e+00
```

- SysY 数组维度中的 `ConstExp` 必须能在编译期求值，因此 SysY AST 会使用求值后的维度；Clang AST dump 可能保留源码中的常量名或常量表达式。diff 阶段会收集简单的 `const int` 定义，并将数组类型中的维度表达式归一化为对应整数。

```text
Clang：
    -| `-DeclRefExpr 'int[N]' lvalue Var 'a2' 'int[N]'
    -| `-DeclRefExpr 'int[len + 5]' lvalue Var 'c1' 'int[len + 5]'
SysY：
    +| `-DeclRefExpr 'int[10000]' lvalue Var 'a2' 'int[10000]'
    +| `-DeclRefExpr 'int[25]' lvalue Var 'c1' 'int[25]'
```

- Clang 在数组实参传递中可能额外插入 `BitCast`，例如把 `int (*)[4]` 转成 `int (*)[*]`；SysY 不保留这层 C 语义下的数组指针转换包装，因此 diff 阶段会折叠掉包裹 `ArrayToPointerDecay` 或 `LValueToRValue` 的 `BitCast`。

```text
Clang：
    -| | |-ImplicitCastExpr 'int (*)[*]' <BitCast>
    -| | | `-ImplicitCastExpr 'int (*)[4]' <ArrayToPointerDecay>
    -| | |-ImplicitCastExpr 'int (*)[*]' <BitCast>
    -| | | `-ImplicitCastExpr 'int (*)[4]' <LValueToRValue>
SysY：
    +| | |-ImplicitCastExpr 'int (*)[4]' <ArrayToPointerDecay>
    +| | |-ImplicitCastExpr 'int (*)[4]' <LValueToRValue>
```

- `starttime()` / `stoptime()` 在 Clang 侧会被预处理宏展开为 `_sysy_starttime(__LINE__)` / `_sysy_stoptime(__LINE__)`，并额外出现来自 `<scratch space>` 的行号实参；SysY AST 保留源语言层面的运行时调用，因此 diff 阶段会把这类宏展开折叠回 `starttime` / `stoptime`。

```text
Clang：
    -|-CallExpr 'void' '_sysy_starttime'
    -| `-IntegerLiteral <<scratch space>:2:1> 'int' 65
SysY：
    +|-CallExpr 'void' 'starttime'
```
