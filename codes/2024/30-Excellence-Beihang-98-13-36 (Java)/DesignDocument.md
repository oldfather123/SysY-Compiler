## 一、使用方法
使用命令行参数传入源文件名(.sy后缀)和目标文件名(.s后缀)，可选的参数为-O1，表示优化等级为1，默认为0。
```
targetName.s sourceName.sy <-O1>
```
## 二、总体设计
### 2.1 编译过程
不开优化的过程为：
```
源代码→[词法分析器]→字序列→[语法分析器]→语法树→[中间代码生成器]→中间代码
                                                        ↓
              目标代码←[寄存器分配器]←目标代码的中间形式←[目标代码生成器]
```
开优化的过程为：
```
源代码→[词法分析器]→字序列→[语法分析器]→语法树→[中间代码生成器]→中间代码→[中间代码优化器]
                                                                   ↓
        调优参数←[寄存器虚拟分配器]←目标代码的中间形式←[目标代码生成器]←优化后的中间代码
           |------------------------------------------------------→↓
                                                             [目标代码生成器]
                                                                   ↓
        优化后的目标代码←[目标代码优化器]←目标代码←[寄存器虚拟分配器]←目标代码的中间形式
```
### 2.2 模块划分
1. 前端：
    - 词法分析器：负责将源代码分割成一个个的字，如关键字、标识符、操作符等。
    - 语法分析器：将字组织成语法树，表示源代码的结构，反映了程序的语法结构。
2. 中端：
    - 中间代码生成器：将语法树转化为自己设计的中间代码。
    - 中间代码优化器：对中间代码进行各种优化，包括但不限于常量折叠、死代码删除等。
3. 后端：
    - 目标代码生成器：将中间代码转化为未分配寄存器的目标机器代码的中间形式。
    - 寄存器分配器：为中间形式的目标代码分配寄存器。
    - 目标代码优化器：对目标代码进行各种优化，主要为窥孔优化。
### 2.3 文件结构
```
src
|--Compiler.java - (编译器入口，检查命令行参数)
|--entities - (实体类)
|  |--Assembler.java - (目标代码的基类)
|  |--DoubleList.java - (双向链表)
|  |--FOI.java - (表示一个Float或者Integer)
|  |--Label.java - (Assembler的子类，表示目标代码中的标签)
|  |--Mark.java - (Assembler的子类，表示当前需要保存临时寄存器)
|  |--Pair.java - (Pair)
|  |--QuaBlock.java - (中间代码块)
|  |--Quaternion.java - (中间代码实体类)
|  |--Register.java - (寄存器)
|  |--RegisterAllocator.java - (寄存器分配器)
|  |--Registers.java - (管理寄存器的类)
|  |--Statement.java - (Assembler的子类，表达式语句实体类)
|  |--TreeNode.java - (语法树节点)
|  |--Triple.java - (Triple)
|  |--TwoWayMap.java - (双向Map)
|  |--Val.java - (四元式的每一个变量)
|  |--Variable.java - (变量)
|  \--Word - (词法分析中的字)
|--file - (文件相关类)
|  |--Output.java - (输出类)
|  |--ReadSys.java - (读取源文件类)
|  |--SymbolBuilder.java - (构建操作符的工具类)
|  \--WordBuilder.java - (构建标识符的工具类)
|--handler - (中后端处理)
|  |--AssemblerHandler.java - (汇编代码生成类)
|  |--OptHandler.java - (优化控制类)
|  |--QuaHandler.java - (中间代码生成类)
|  |--RegisterHandler.java - (寄存器分配)
|  \--WordBuilder.java - (构建标识符的工具类)
|--main - (主类)
|  \--Main.java - (主类，定义编译过程)
|--opt - (优化相关类)
|  |--AssOptimize.java - (汇编代码类)
|  \--QuaOptimize.java - (中间代码优化类)
\--util - (工具类)
   |--ConstExp.java - (计算常数的工具类)
   |--Intergers.java - (存放整数常量)
   |--IntHandler.java - (生成整数的工具类)
   |--NumberHandler.java - (字符串转数字的工具类)
   |--Strings.java - (存放字符串常量)
   \--Util.java - (一些常量)
```
## 三、词法分析设计
由一个WordBuilder对象和一个SymbolBuilder对象负责，WordBuilder本质上是一个StringBuilder，而SymbolBuilder是一个有限状态自动机。    
对每个字符，如果是操作符就放到SymbolBuilder里跑自动机，匹配成功则输出对应的标识符对象，否则放入WordBuilder，生成关键字、变量名等。
## 四、语法分析设计
### 4.1 语法树节点设计
> 定义：x+为x至少一次, x*为x出现一次或多次或不出现, x?为x最多一次, x|y为x或y

> EMPTY: 空，用于标识stmt为只有一个分号以及在funcFParam中数组第一维为空  
> IDENT: 符号，此节点含变量信息  
> COMP_UNIT: 根节点，对应整个程序，其子节点为(ConstDecl | ValDecl | FuncDef | MainFuncDef)+  
> B_TYPE: 基本类型，为int或float  
> DECL: 不作为单独的节点，被CONST_DECL和VAR_DECL代替  
> CONST_DECL: 常量定义群，其子节点为 B_TYPE CONST_DEF+  
> CONST_DEF: 常量定义，其子节点为 IDENT ADD_EXP* (CONST_INIT_VAL | ADD_EXP)  
> CONST_INIT_VAL: 常数初始值，其子节点为 (ADD_EXP | CONST_INIT_VAL)+  
> VAR_DECL: 变量定义群，其子节点为 B_TYPE VAR_DEF+  
> VAR_DEF: 变量定义，其子节点为 IDENT ADD_EXP* (INIT_VAL | ADD_EXP)?  
> INIT_VAL: 变量初值，其子节点为 (CONST_INIT_VAL | INIT_VAL)+  
> FUNC_DEF: 函数定义，其子节点为 FUNC_TYPE IDENT FUNC_F_PARAM* BLOCK  
> FUNC_TYPE: 函数类型，为int或float或void  
> FUNC_F_PARAM: 函数形参，其子节点为 B_TYPE IDENT ADD_EXP*  
> BLOCK: 基本块，其子节点为 BLOCK_ITEM*  
> BLOCK_ITEM: 不作为单独的节点，被CONST_DECL、VAR_DECL和STMT替代  
> STMT: 语句，其子节点为 L_VAL ADD_EXP 或 ADD_EXP 或 EMPTY 或 BLOCK   
>                  或 IF L_OR_EXP STMT STMT? 或 WHILE L_OR_EXP STMT  
>                  或 BREAK 或 CONTINUE 或 RETURN ADD_EXP?  
> L_VAL: 左值表达式，其子节点为 IDENT ADD_EXP*  
> PRIMARY_EXP: PrimaryExp，其子节点为 INT_CONST 或 FLOAT_CONST 或 ADD_EXP 或 L_VAL  
> INT_CONST: 整数常数  
> FLOAT_CONST: 浮点数常数  
> FUNC_R_PARAM: 函数实参，不作为单独的节点，被ADD_EXP替代  
> UNARY_EXP: UnaryExp，其子节点为 OPERATOR UNARY_EXP 或 PRIMARY_EXP 或 IDENT ADD_EXP*  
> UNARY_OP: Unary运算符，为 + 或 - 或 !   
> MUL_EXP: MulExp，其子节点为 UNARY_EXP (OPERATOR{ * | / | % } UNARY_EXP)*  
> ADD_EXP: AddExp，其子节点为 MUL_EXP (OPERATOR{ + | - } MUL_EXP)*  
> REL_EXP: RelExp，其子节点为 ADD_EXP (OPERATOR{ < | <= | > | >= } ADD_EXP)*  
> EQ_EXP: EqExp，其子节点为 REL_EXP (OPERATOR{ == | != } REL_EXP)*  
> L_AND_EXP: LAndExp，其子节点为 EQ_EXP+  
> L_OR_EXP: LOrExp，其子节点为 L_AND_EXP+  
> IF: if，标记节点  
> WHILE: while，标记节点  
> BREAK: break，标记节点  
> CONTINUE: continue，标记节点  
> RETURN: return，标记节点  
> OPERATOR: 操作符，为各种操作符  
### 4.2 语法分析过程
语法分析采用递归下降的方法来生成语法树，例如下面是解析AddExp代码
```
private static TreeNode addExp() {
    //初始化父节点
    TreeNode addExp = new TreeNode(TreeNode.Type.ADD_EXP);
    //递归下降，将MulExp加入子节点组
    addExp.addSon(mulExp());
    //如果后面还有加号或减号说明还有MulExp
    while (current().is(Word.Type.PLUS) || current().is(Word.Type.MINU)) {
        //递归下降，将这个加号或减号加入子节点组
        addExp.addSon(operator());
        //递归下降，将MulExp加入子节点组
        addExp.addSon(mulExp());
    }
    //返回父节点
    return addExp;
}
```
## 五、中间代码设计
中间代码采用四元式表示，四元式的结构为(op, val1, val2, val3)，其中op为操作符，val1、val2、val3为操作数，部分操作符的操作数可能为null。
#### 5.1 四元式设计
> ASSIGN: 表示一个赋值操作val1 = val2，val3为null。  
> GET_RETURN_VALUE: 获取返回值，将返回值存入val1，val2和val3为null。  
> CALL: 调用函数，val1为函数名，val2和val3为null。  
> RETURN: 返回语句，如果val1为null，则表示无返回值，否则表示有返回值，val2和val3为null。  
> SAVE_ARG: 保存函数实参到栈上，val1表示第几个参数，val2是函数实参，val3为实参类型。  
> SET_ARG: 保存函数实参到参数寄存器上，val1表示第几个参数，val2是函数实参，val3为实参类型。  
> ADD_SP: 增加栈指针，val1为增加的量，val2和val3为null。  
> PLUS: 加法，表示val1 = val2 + val3。  
> MINU: 减法，表示val1 = val2 - val3。  
> MULT: 乘法，表示val1 = val2 * val3。  
> DIV: 除法，表示val1 = val2 / val3。  
> MOD: 取模，表示val1 = val2 % val3。  
> JUMP: 跳转，val1为要跳转的label，val2和val3为null。  
> DEFINE_LABEL: 在此处定义一个label，val1为定义的label名，val2和val3为null。  
> SLT: 对应slt(set less than)，如果val2<val3置val1为1否则为0。  
> SLE: 对应sle(set less or equal)，如果val2<=val3置val1为1否则为0。  
> SGT: 对应slt(set great than)，如果val2>val3置val1为1否则为0。  
> SGE: 对应slt(set great or equal)，如果val2>=val3置val1为1否则为0。  
> SEQ: 对应slt(set equal)，如果val2==val3置val1为1否则为0。  
> SNE: 对应slt(set not equal)，如果val2!=val3置val1为1否则为0。  
> EQZ: 对应beqz(branch if equal zero)，如果val1==0则跳转到val2存储的label。  
> DECLARE: 定义局部变量，val1为变量，val2和val3为null。  
> DECLARE_ARG: 定义函数参数，val1为参数，val2和val3为null。  
### 5.2 生成四元式过程
类似于语法树的递归下降过程，根据前文定义好四元式后很容易生成四元式序列。
其中有几个注意点：
1. 对于void函数，要在末尾加上return。
2. 对于短路求值，例如if，提前生成好trueLabel、falseLabel、endLabel，将trueLabel和falseLabel交给lOrExp递归用，并在Cond后放置trueLabel，else的语句前放置falseLabel，
   else的语句后放置endLabel，而lOrExp中在每个lAndExp前生成一个andLabel，将falseLabel和后一个lAndExp的andLabel交给lAndExp递归用，
   这样就可以保证如果lAndExp为否直接跳转到falseLabel就可以不进行后面的lAndExp判断了，而如果lAndExp为真就跳转到下一个lAndExp，
   并将falseLabel和下一个andLabel交给下一个lAndExp使用。
## 六、目标代码生成
目标代码生成时分两个阶段，第一阶段不做具体的寄存器分配，用虚拟的寄存器代替，第二阶段再进行寄存器分配。  
第一阶段对于每条四元式，简单的翻译成目标代码，使用的虚拟寄存器是无限的，用过一次之后不再使用，第二阶段的寄存器分配时暴力维护虚拟寄存器对应的寄存器。
## 七、代码优化设计
### 7.1 Mem2Reg
为变量分配寄存器，减少内存的访问。  
当开启优化后，会进行两次目标代码生成，第一次生成时记录需要的临时寄存器个数，看需要的临时寄存器个数是否多于risc-v的临时寄存器个数，如果使用得过多会在第二次正式生成时将s寄存器当作临时寄存器使用。
### 7.3 死代码删除
代码分块的时候记录了块之间的跳转关系，优化的最后根据块之间的跳转关系DFS得到整个函数的代码，死代码自然被消除。
### 7.2 常数优化
对于两个常数的加减乘除等运算直接将结果替换成常数，对于一个变量被赋值成常数，在下一次被赋值之前都可将其替换为常数。
### 7.3 Label优化
1. 如果有两个相邻的DEFINE_LABEL说明其中一个时不需要的，将其所有的用法替换成另一个label
2. 如果有一个跳转语句跳转到的label就是下一句他对应的DEFINE_LABEL，那么他是冗余的跳转，将跳转语句删除
3. 如果一个label没有被用到，将这个label删除
### 7.4 汇编代码优化
1. addi x0, x0, 0 可以直接删去
2. addi x0, x0, a 和 addi x0, x0, b 可以合并成一个 addi x0, x0, a+b
3. li x0, a 和 mv x1, x0 一起出现的时候如果x0后续没有被使用，可以合并成一个li x1, a

