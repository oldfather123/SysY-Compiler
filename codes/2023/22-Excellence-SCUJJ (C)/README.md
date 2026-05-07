# syCompiler

/src/include/m-bitset.h<br>
/src/include/m-core.h<br>
/src/include/uthash.h<br>
/src/include/utlist.h<br>
/src/lexer/lex.yy.c<br>
以上是本程序使用的第三方开源库


## 任务

- [x] 词法分析
- [x] 语法分析
- [x] 三地址码生成
- [x] 构建SSA
- [x] DVNT 基于支配者的值编号
- [ ] SCCP稀疏条件常量传播(进行中)
- [x] 强度削弱
- [x] 死代码消除
- [ ] 别名分析
- [x] 过程间常量传播
- [x] 线性扫描寄存器分配
- [x] 窥孔优化
- [x] 尾递归优化
- [x] 汇编代码生成

## 前端
前端单趟编译,使用yylex分析token.语法分析采用自上而下分析,不生成AST,直接生成CFG性质的IR.

## IR
指令后面加F是浮点数相关

|  指令   | 作用  |
|  ----  | ----  |
| IR_OP_ADD  |  |
| IR_OP_ADDF  |  |
| IR_OP_SUB  |  |
| IR_OP_SUBF  |  |
| IR_OP_MUL  |  |
| IR_OP_MULF  |  |
| IR_OP_DIV  |  |
| IR_OP_DIV  |  |
| IR_OP_DIVF  |  |
| IR_OP_MOD  |  |
| IR_OP_LSHIFT  |  |
| IR_OP_RSHIFT  |  |
| IR_OP_GE  |  |
| IR_OP_GEF  |  |
| IR_OP_GT  |  |
| IR_OP_GTF  |  |
| IR_OP_EQ  |  |
| IR_OP_EQF  |  |
| IR_OP_NE  |  |
| IR_OP_NEF  |  |
| IR_OP_LT  |  |
| IR_OP_LTF  |  |
| IR_OP_LE  |  |
| IR_OP_LEF  |  |
| IR_OP_GETITEMPTR  | 指针寻址,指令较为高级 |
| IR_OP_ADDPTR  | 由IR_OP_GETITEMPTR转化的指令用于强度削弱 |
| IR_OP_RETF  | 返回一个浮点数 |
| IR_OP_RETI  | 返回一个值 |
| IR_OP_RET  | 无参数返回 |
| IR_OP_PARAM  | 函数调用传入参数 |
| IR_OP_ARGUMENT  | 函数声明的参数 |
| IR_OP_PHI  | SSA相关 |
注:基本块跳转由BasicBlock结构中nextblock字段和branchblock字段指定,基本块不以JMP跳转类指令结尾


## 中端
中端接收CFG性质的IR进行优化并输出优化后的IR.
value由3种组成,地址、ssa value和立即数.在优化过程中可以给出md文件图形化的控制流图。

## 后端
经过寄存器分配后生成riscv汇编代码,再经过窥孔优化,移除冗余指令,然后输出汇编文件.
