## Compile into Antlr4 Visitor
```makefile
make antlr4
```
Then it will generate the corresponding visitor or listener in ./parser
## install 'antlr4-runtime.h'
visit https://blog.csdn.net/drutgdh/article/details/122816033
## UseAntlr Visitor
初始化
```c++
MySysYParserVisitor visitor=MySysYParserVisitor(variableTable(nullptr));
```
设置模块
```c++
void setModule(Module m);
```
访问模块
用类似在https://docs.compilers.cpl.icu
中获得抽象语法树，然后用
```c++
MySysYParserVisitor visitor = MySysYParserVisitor(VariableTable(nullptr));
visitor.visitProgram(xxxcontext);
```
