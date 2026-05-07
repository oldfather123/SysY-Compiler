# antlr 基本用法

-   生成 Sysy 的 Visitor 等文件：

```sh
antlr4 Sysy.g4 -Dlanguage=Cpp -no-listener -visitor -o generated/
```

-   编译：

```sh
cd generated
make
```

编译最后得到的是一个可执行文件***parser***

-   清理：

```sh
cd generated
make clean
```

# 文件结构理解

-   Makefile：构建文件

-   Sysy.g4：文法定义，其中每个产生式都会在 SysyParser 中生成一个节点，如果在后面添加了'#'则同样会产生一个节点。

-   SysyLexer：Tokens 定义

-   SysyParser：等于是 AstNode

-   SysyVisitor：继承于 antlr 的 visitor 类

-   SysyBaseVisitor：继承于 SysyVisitor 类，不定义任何内容

-   AstVisitor：自定义的访问者类，用法参考 main.cpp

-   main.cpp：程序的入口

# AST 结构

-   `namespace ast`：AST 命名空间
    -   Node：AST 节点基类
        -   Stmt：语句
            -   BlockStmt：大括号作为一个语句 `{ ...; ...; }`
            -   ExprStmt：表达式作为一个语句 `f(1, 2);`
            -   DeclStmt：变量声明语句 `int i = 0;` `int j;` `int a[10] = { };`
            -   AssignStmt：赋值语句 `x = y;` `a[2] = 1 + x;`
            -   IfElseStmt：if else 分支语句 `if (x > 0) ... else ...`
            -   WhileStmt：while 循环语句 `while (i != 0) ...`
            -   BreakStmt：break 语句 `break;`
            -   ContinueStmt：continue 语句 `continue;`
            -   ReturnStmt：返回语句 `return a + b;` `return;`
        -   Expr：表达式
            -   LValueExpr：左值表达式 `x` `a[5]`
            -   BinaryExpr：双目运算符表达式 `x >= y` `x - 5`
            -   UnaryExpr：单目运算符表达式 `-x` `+x`
            -   LiteralExpr：字面值表达式 `1` `1.0` `"abc123"`
                -   IntLiteralExpr：int32 类型字面值
                -   FloatLiteralExpr：float 类型字面值
                -   StringLiteralExpr：string 类型字面值
            -   CallExpr：函数调用表达式 `f(x, y)`
        -   Function：函数 `int f(int x, int y) { ... }`
        -   CompUnit：代码文件：包含若干全局变量和函数
