# 素履“译”往队编译器设计文档

## 总体结构

我们小组编译器是采用Java语言编写的RISCV编译器。总体分为前端，中端，后端三部分

- 前端：词法分析、语法分析将源程序解析存储为语法树`AST`
- 中端：根据`AST`进行语义分析生成`LLVM IR`初步中间代码，然后进行中间代码优化，包括`mem2reg`，`GVN`，`RegAllocator`，`DeadCodeRemove`等等
- 后端：目标代码生成，即将`LLVM IR`进一步翻译成`RISCV`代码

### 项目结构图

```
─src
    ├─Backend
    │  ├─component
    │  ├─instruction
    │  ├─operand
    │  └─process
    │      └─RegAllocator
    ├─config
    ├─frontend
    │  └─AST
    ├─META-INF
    ├─midend
    │  ├─analysis
    │  ├─instr
    │  ├─LLVMType
    │  └─optimism
    └─Utils

```



## frontend前端

前端部分代码主要由语法分析、词法分析部分和用于存储分析结果的`AST`数据结构组成。

### AST

本模块为语法树模块，主要负责语法分析后的结果存储到语法树结构`AST`中以便后续语义分析随时取用，用于表示前端编程语言中的语法结构。

1. `CompUnit` 表示编译单元，可以是声明（`Decl`）或函数定义（`FuncDef`）。
2. `Decl` 表示声明，包括基本类型（`Btype`）、标识符（`Ident`）和初始化值（`InitVal`）。
3. `FuncDef` 表示函数定义，包括函数类型（`FuncType`）、函数名（`Ident`）、参数列表（`FuncFParams`）和函数体（`Block`）。
4. `Block` 表示代码块，包括多个代码块项（`BlockItem`）。
5. `Stmt` 表示语句，可以是赋值语句（`Assign`）、表达式（`Exp`）、代码块（`Block`）等等。
6. `Exp` 表示表达式，可以是二元表达式（`BinaryExp`）、一元表达式（`UnaryExp`）等。
7. 其他类似的定义包括条件语句（`If`）、循环语句（`While`）、返回语句（`Return`）等。

通过之前按的语法递归下降分析构建出由以上层次结构构成的语法树结构以便编译器理解和处理源代码生成中间代码。在 `AST` 类中，通过构造函数和 `getUnits` 方法来操作和访问这些语法单元。

具体语法组成单元及结构关系如下：

```c
Decl -> ['const'] Btype Def { ',' Def ',' } ';'
Def -> Ident { '[' Exp ']' } '=' InitVal
InitVal -> Exp | InitArray
InitArray -> '{' [ InitVal { ',' InitVal } ] '}'
FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
FuncType -> void | int | float
FuncFParams -> FuncFParam { ',' FuncFParam }
FuncFParam -> Btype Ident [ '[' ']' { '[' Exp ']'} ]
Block -> '{' { BlockItem } '}'
Blockitem -> Decl | Stmt=
Stmt -> Assign | Exp | Block | If | Else | While | Break | Continue | Return
Assign -> LVal '=' Exp ';'
If -> 'if' '(' Cond ')' Stmt [ Else ]
Cond -> Exp
Else -> 'else' Stmt
While -> 'while' '(' Cond ')' Stmt
Return -> 'return' [Exp] ';'
Exp -> BinaryExp | UnaryExp
BinaryExp -> Exp {BinaryOp Exp}
BinaryOp -> + - > >= < <= == && != ||
UnaryExp -> {UnaryOp} PrimaryExp
PrimaryExp -> '('Exp')' | LVal | Number | Func
LVal -> Ident { '[' Exp ''] }
Number -> IntConst | FloatConst
Func -> Ident '(' [FuncRParams] ')'
FuncRParams -> Exp {',' Exp}
```

### Token-词法基本单元

用于储存词法分析最基本的单元结构，基本属性如下

```java
public class Token {
    private final int type;
    private final String val;
    private int lineNum = 0;
    ......
}
```

`type`用于存储`token`的类型，`val`则存储具体的内容，`lineNum`用于存储行号,初始为0(用于错误处理，不过由于错误处理功能暂未实现目前始终置0)

### Lexer-词法分析模块

将源文件转化为输入流，然后依次读取每个字符，按固定的词法规则进行解析，每识别出一个`token`，就将其封装到上面的`Token`类中。

核心解析过程即为：首先进入一个无限循环，不断读取输入流中的字符。如果缓冲字符不为 0，则将其赋给 `temp`；否则从输入流中读取一个字符赋给 `temp`。如果读取到的字符为 `-1`，表示输入流结束，返回` null`。

接着判断当前字符` curToken `的类型：如果是**空格**、**制表符**、**换行符**等空白字符，则继续循环；如果是**数字**或**小数点**，则调用` number()` 方法解析数字；如果是**字母**或**下划线**，则调用 `ident() `方法解析标识符；如果是**双引号**，则调用 `formatString() `方法解析字符串；否则根据当前字符的不同情况返回相应的` Token` 对象。

```java
public Token readTok() throws IOException {
        while (true) {
            int temp;
            if (bufferedChar != 0) {
                temp = bufferedChar;
                bufferedChar = 0;
            } else {
                temp = input.read();
                if (temp == -1) {
                    return null;
                }
            }
            curToken = (char) temp;
            //换行符
            if (curToken == '\n' || curToken == ' ' || curToken == '\t' || curToken == '\r') {
//                if (curToken == '\n') {
//                    lineNum++;
//                }
                continue;
            }
            //数字
            if (isDigit(curToken) || curToken == '.') {
                return number();
            }
            //标识符
            if (isNonDidit(curToken)) {
                return ident();
            }
            //字符串
            if (curToken == '\"') {
                return formatString();
            }
            //其他
            switch (curToken) {
                case '/':
                    char next = next();
                    if (next == '/' || next == '*') {
                        bufferedChar = next;
                        note();
//                        if (curToken == -1) {
//                            return null;
//                        }
                        continue;
                    }
                    bufferedChar = next;
                    return new Token(DIV, "/");
                case '+':
                    return new Token(ADD, "+");
                case '-':
                    return new Token(SUB, "-");
                case '*':
                    return new Token(MULT, "*");
                case '%':
                    return new Token(MOD, "%");
                case '!':
                    char next1 = next();
                    if (next1 == '=') {
                        return new Token(NE, "!=");
                    }
                    bufferedChar = next1;
                    return new Token(NON, "!");
                case '{':
                    return new Token(LBRACE, "{");
                case '}':
                    return new Token(RBRACE, "}");
                case '[':
                    return new Token(LBRACKET, "[");
                case ']':
                    return new Token(RBRACKET, "]");
                case '(':
                    return new Token(LPARENT, "(");
                case ')':
                    return new Token(RPARENT, ")");
                case '&':
                    char next2 = next();
                    if (next2 == '&') {
                        return new Token(AND, "&&");
                    } else {
                        throw new IOException();
                    }
                case '|':
                    char next3 = next();
                    if (next3 == '|') {
                        return new Token(OR, "||");
                    } else {
                        throw new IOException();
                    }
                case ',':
                    return new Token(COMMA, ",");
                case ';':
                    return new Token(SEMICOLON, ";");
                case '<':
                    char next4 = next();
                    if (next4 == '=') {
                        return new Token(LE, "<=");
                    }
                    bufferedChar = next4;
                    return new Token(LESS, "<");
                case '>':
                    char next5 = next();
                    if (next5 == '=') {
                        return new Token(ME, ">=");
                    }
                    bufferedChar = next5;
                    return new Token(MORE, ">");
                case '=':
                    char next6 = next();
                    if (next6 == '=') {
                        return new Token(EQUAL, "==");
                    }
                    bufferedChar = next6;
                    return new Token(ASSIGN, "=");
                default:
                    return null;
            }
        }
    }
```

### Parser-语法分析单元

将词法分析阶段解析出来的`token`流作为自己的成员变量，然后按照文法规则对`token`流进行递归下降分析，从 `parseCompUnit() `开始按照上述`AST`中的语法结构依次向下递归调用对应层次的`parser`方法进行语法的分析，最终将分析结果储存在构建出的新语法树中。

## midend中端

我们的中端代码主要分为两部分，一部分是基于前端分析完的语法树生成`LLVM IR`的中间代码生成部分，另一部分是对于初步生成的中端代码进行优化提升生成最终传递给后端的`LLVM IR`代码的中端优化部分。生成的`LLVM IR`均封装存储在`Module`类中以便调用。

### 中间代码生成

#### 代码生成总体架构

由`Visitor`类调用`visitAST()`方法从语法树的根节点开始按照构建的语法结构依次向下（由`visitAST`到`visitFuncDef`和`visitDecl`，再依次往下）调用对应的`visit`方法进行由上而下的语义分析，将分析完的结果存入一个`Module`实例并存储在`Visitor`类`module`属性中，外部可通过`getModule()`获取module进行解析与使用

```java
public void visitAST(AST ast) {
        module = new Module(globalVars);
        ArrayList<CompUnit> units = ast.getUnits();

        LibFunction.initLib();
        symbolTable.pushsymbol();

        curBasicBlock = new BasicBlock(UndefinedType.undefined, null); //临时块
        for(CompUnit unit : units) {
            if (unit instanceof FuncDef) {
                visitFuncDef((FuncDef) unit);
            } else if (unit instanceof Decl) {
                visitDecl((Decl) unit, true);
            } else {
                throw new RuntimeException("IRgen: AST has problems");
            }
        }
    }
```



#### LLVMType

即`LLVM IR`中的每个`Value`的所属类型，以`Type`为基类继承实现了`ArrayType`，`FunctionType`，`VoidType`，`FloatType`，`PointerType`，`IntegerType`等基本`Type`（另有`UndefinedType`作为特殊类型存储不属于这些基本类型的未定义类型）。

同时每个类都内置了用于判断类型的`isArray`，`isFun`等配套方法用于快速判断当前`Value`所属的类型以使代码更加简洁直观。

![](https://article-picture-sourse.oss-cn-beijing.aliyuncs.com/oo/%E5%BE%AE%E4%BF%A1%E6%88%AA%E5%9B%BE_20240810003022.png)

#### 内存形式

根据`LLVM IR`的特点我们小组以`Value`类为母类设计了以下内存形式的继承关系

![](https://article-picture-sourse.oss-cn-beijing.aliyuncs.com/oo/%E5%BE%AE%E4%BF%A1%E6%88%AA%E5%9B%BE_20240809221527.png)

其中关键类的定义如下：

##### Value

`LLVM`内存形式的“`Object`”，主要属性有`Value`的`Type`与`name`，与`Value`相关的边的List

```java
public class Value {
    private Type type;
    private String name;
    private ArrayList<Use> useList = new ArrayList<>();

    public Value(Type type, String name) {
        this.type = type;
        this.name = name;
    }
    }
```



##### Module

顶层编译单元，主要由全局变量列表、函数列表组成（`output`用于输出`LLVM IR`代码，相当于`Module`的专用`toString`）

```java
public class Module {
    private ArrayList<Function> functions;
    private BufferedWriter output;
    private ArrayList<GlobalVar> globalVars;
    }
```

##### Function

全局函数类，主要属性有参数列表，返回值类型，基本块列表（其中前继基本块map，后继基本块map，父基本块map，子基本块map均为用于支配树构建的属性）

```java
public class Function extends User {
    private ArrayList<Param> params;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> preMap;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> nextMap;
    private HashMap<BasicBlock, BasicBlock> parentMap;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> childMap;
    private Type retType;
    private LinkedList<BasicBlock> blockList;
    }
```

##### BasicBlock

基本块类，整个内存结构中最或不可缺的一个类。其基本属性有指令列表，所属函数，所属父基本块（`preBlock`，`nextBlock`，`domBlock`，`childBlock`，`DF`均与上相同，为支配树相关属性，用于寻找需要添加`phi`指令的节点基本块）

```java
public class BasicBlock extends Value{
    private LinkedList<Instruction> instrList;
    private ArrayList<BasicBlock> preBlock = new ArrayList<>();
    private ArrayList<BasicBlock> nextBlock = new ArrayList<>();
    private ArrayList<BasicBlock> domBlock = new ArrayList<>();
    private BasicBlock parentBlock;
    private ArrayList<BasicBlock> childBlock = new ArrayList<>();

    private ArrayList<BasicBlock> DF;
    private Function parentFunc;
    }
```

##### User

可以使用其他`Value`的类的，有其能够使用的`Value`组成的`valuelist`

```java
public class User extends Value {
    private ArrayList<Value> valueList;
    public User(Type type, String name) {
        super(type, name);
        valueList = new ArrayList<>();
    }
    }
```

##### Use

用于存储`use-value`关联关系的边的类

```java
public class Use {
    private Value value;
    private User user;

    public Use(Value value, User user) {
        this.value = value;
        this.user = user;
    }
    }
```

##### Instruction

LLVM指令，由指令类型，所属基本块指针组成。每一种指令都有其单独所属的类，具体有`AllocaInstr`，`ALUInstr`，`BrInstr`，`CallInstr`，`CmpIntr`，`ConversionInstr`，`GEPIntr`，`MoveIntr`，`LoadInstr`，`PcopyInstr`，`PhiInstr`，`RetInstr`，`StoreInstr`等。用于实现对各种指令的表示

```java
public class Instruction extends User {
    private InstrType instrType;
    private BasicBlock basicBlock;

    public Instruction(Type type, String name, InstrType instrType) {
        super(type, name);
        this.instrType = instrType;
    }
}
```

### 中端优化

#### DeadCodeRemove

`DeadCodeRemove`是指“通过活跃变量分析，找出所有只定义不使用的变量，将其定义点删除”。在寄存器分配阶段，我们已经获得了所有变量的`def-use`信息，在这一阶段我们只需要遍历一遍指令，将没有被使用的变量的定义点删除即可。是否被使用这一点我们通过不同的`Instruction`的`canBeUsed()`方法来识别

```java
public class Instruction extends User {
    ......
    public boolean canBeUsed() {
        return false;
    }
}
```

Instruction中的`canBeUsed()`默认返回`false`即默认不需要删除，而在可能需要删除的指令中对`canBeUsed()`方法override，返回`true`，如`ALUInstr`：

```java
public class ALUInstr extends Instruction {
    @Override
    public boolean canBeUsed() {
        return true;
    }
}
```

进行完`canBeUsed()`标记后我们再在`DeadCodeRemove`类中遍历识别可能需要删除的指令，再检测其内部`useList`是否为空(是否是只定义不使用变量)确认是否删除，将需要删除的`deadCode`添加到`toBeRemove`容器中遍历删除即可

```java
for (BasicBlock block : function.getBlockList()) {
                    ArrayList<Instruction> toBeRemoved = new ArrayList<>();
                    for (Instruction instruction : block.getInstrList()) {
                        if (instruction.canBeUsed() || (instruction instanceof CallInstr && !((CallInstr) instruction).getFunction().isSideEffect())) {
                            if (instruction.getUseList().isEmpty()) {
                                toBeRemoved.add(instruction);
                            }
                        }
                    }
```



#### Phi指令与mem2reg

##### Phi指令

`Phi`指令用于实现 `Phi `节点。在运行时，`Phi `指令根据“在当前 block 之前执行的是哪一个 predecessor(前任) block”来得到相应的值。`Phi`指令基本架构如下

```java
public class PhiInstr extends Instruction {
    private ArrayList<BasicBlock> preBlockList;
    public PhiInstr(Type type, ArrayList<BasicBlock> preBlockList) {
        super(type, "%reg" + (Value.num++), InstrType.PHI);
//        if (this.getName().equals("%reg4379")) {
//            System.out.println("");
//        }
        this.preBlockList = preBlockList;
        for (int i = 0; i < preBlockList.size(); i++) {
            addValue(null);
        }
    }

    public PhiInstr(Type type) {
        super(type, "%reg" + (Value.num++), InstrType.PHI);
        this.preBlockList = new ArrayList<>();
    }

    public Value getValueFrom(BasicBlock block) {......}

    public void addOp(Value value, BasicBlock preBlock) {......}

    public void addOption(Value value, BasicBlock preBlock) {......}
    }

    public ArrayList<Value> getOptions() {
        return getValueList();
    }

    public boolean canUse() {return true;}

    public String getInstr() {......}

    public BasicBlock getPreBlock(int count) {......}

    public BasicBlock getPreBlock(Value value) {......}

    public ArrayList<BasicBlock> getPreBlockList() {......}

    public void removeValue(int index) {......}

    public PhiInstr clone(BasicBlock block) {......}

    public void modifyBlock(BasicBlock before, BasicBlock after) {......}

}
```

- `preBlockList`存储了该` phi `指令的前驱基本块列表。

- `PhiInstr(Type type, ArrayList<BasicBlock> preBlockList)`：构造方法，初始化 `phi` 指令的类型和前驱基本块列表。

- `getValueFrom(BasicBlock block)`：根据给定的前驱基本块，返回对应的数值。

- `addOp(Value value, BasicBlock preBlock)`：向 `phi `指令中添加操作数和对应的前驱基本块。

- `addOption(Value value, BasicBlock preBlock)`：更新对应前驱基本块的数值。

- `getOptions()`：获取所有操作数(`ValueList`)。

- `canUse()`：判断该指令是否可用（`deadcodeRemove`相关）。

- `getInstr()`：调用 `getOptions()` 方法，获取与当前 `phi` 指令相关联的所有值（操作数）。这些值是从不同的前驱基本块传递过来的然后经过如下处理返回`phi`指令的标准形式。

  ```java
   return getName() + " = phi " + getType() + " "
                  + preBlockList.stream()
                  .map(bb -> "[ " + options.get(preBlockList.indexOf(bb)).getName() + ", %" + bb.getName() + " ]")
                  .collect(Collectors.joining(", ")) + "\n";
  ```



- `getPreBlock(int count)`：根据索引获取前驱基本块。

- `getPreBlock(Value value)`：根据数值获取对应的前驱基本块。

- `getPreBlockList()`：获取前驱基本块列表。

- `removeValue(int index)`：移除指定索引处的数值。

- `clone(BasicBlock block)`：克隆 phi 指令。

- `modifyBlock(BasicBlock before, BasicBlock after)`：修改前驱基本块。



##### mem2reg

`mem2reg`最核心的工作就是在合适的位置插入`phi`指令，主要分为两个步骤**插入phi指令**和**变量重命名**

**插入phi指令**

只有在来自多个基本块的控制流汇合到一个基本块中时，才会出现同一个变量的多个不同定义。我们可以将该基本块称为汇合点。

如果某个变量在多个基本块中都有定义（在内存形式的`LLVM IR`中表现为——一个`alloca`出来的指针在多个基本块中有`store`操作），那么我们需要在这些基本块的汇合点添加和该变量相关的`phi`。而寻找汇合点则依赖于我们的支配树，由`CFGBuilder`类完成构建。

```java
public class CFGBuilder {
    private static Module module;
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> preMap;// 流图上一个
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> nextMap; //流图下一个
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> childMap; //支配树
    private static HashMap<BasicBlock, BasicBlock> parentMap;
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> DFMap;

    public CFGBuilder(Module module) {
        this.module = module;
    }
```

根据Module中各基本块存储的前继后继关系，我们可以得到各个基本块的支配边界，由此得到汇合点。

**变量重命名**

在之前的阶段，我们已经在必要的基本块中插入`phi`指令，但这些`phi`指令是空的，没有包含来自不同基本块的数据流信息（例如`%4 = phi i32 [ ?, ? ], [ ?, ? ]`），并且相关的`alloca`、`load`、`store`指令也未被移除。在当前阶段，我们需要完善这些`phi`指令的数据流信息，调整`use-def`关系，并最终清除相关的访存指令。

为了实现这一目标，我们将首先进行前序遍历支配树。在每个基本块中，我们将按顺序遍历所有指令。对于`store`指令，我们会将需要写入内存的`Value`推入对应指针的堆栈顶部，并删除该指令。而对于`load`指令，我们将修改所有使用该`load`指令对应`Value`的指令，使其使用指针对应堆栈的栈顶`Value`，并删除该指令。当遇到`alloca`指令时，我们会记录下指针名，为该指针创建一个空堆栈，并删除该指令。对于`phi`指令，我们会将该指令对应的`Value`推入指针对应的堆栈。

完成某个基本块所有指令的扫描后，我们需要再次扫描其在支配树中的子节点。这样，我们可以将最新的数据流信息（即每个指针对应的堆栈顶部`Value`）写入子节点中的`phi`指令。最后，我们会为所有子节点调用该重命名函数，以进行后续的遍历过程。通过这一流程，我们能够有效地完善`phi`指令的数据流信息，维护`use-def`关系，并逐步清除不必要的访存指令。

#### GVN

1. **目标**：发现程序中计算结果的重复，并用相同的计算结果代替这些重复计算。

2. **原理**：给每个变量一个哈希值，用来表示计算该变量的表达式，遍历支配树，如果遇到的表达式已经存在，则可以删除冗余代码。

3. **可GVN函数**：函数传参均为标量，且没有修改全局变量，所调用子函数也应满足上述条件。

   ![](https://article-picture-sourse.oss-cn-beijing.aliyuncs.com/oo/GVN%E7%A4%BA%E4%BE%8B.png)

#### GCM

1. **目标**：将计算和数据访问操作从循环或条件语句中提取到外部，以避免重复计算。

2. **Schedule Early**：若use 所在的基本块 A 的支配树深度小于当前非pin指令所在的基本块 B 的支配树深度，则将指令放入基本块 A。

3. **Schedule Late**:对指令 instr 的所有 user ，其所在基本块在支配树上的 LCA 到 instr 所在的原始基本块构成的链均可以放置该指令。把 instr 放入循环深度最小的基本块，循环深度相同时选择支配树深度较深的基本块。

   ![](https://article-picture-sourse.oss-cn-beijing.aliyuncs.com/oo/GCM%E7%A4%BA%E6%84%8F%E5%9B%BE.png)

#### 计算优化（ALUOptimize)

​	在汇编代码执行过程中，乘，除和取模运算耗费时间较长，而加减，移位运算耗费时间短，因而本部分将乘除取模运算转化为加减和移位操作，以加快汇编代码执行速度。

##### 取模优化

​	将函数中的取模操作（`%`）替换为除法（`/`）、乘法（`*`）和减法（`-`）操作的组合，如下：
$$
x \%  m = x - (x / m) * m
$$

##### 除法优化

- **除以-1**：如果除数是 `-1`，则使用 `0 - x` 代替 `x / -1`。

- **除以2的幂**：如果除数是2的幂，则用位移操作替代除法，即将除数的二进制对数作为位移量，使用算术右移（`ASHR`）操作代替除法。例如，`x / 8` 被替换为 `x >> 3`，需注意SysY2022语言中，除法应为向0取整，而移位操作默认为向下取整，这样会导致负数除法时出现错误，因而根据符号位对被除数增添偏移量，以实现除法向0取整的操作，如下：
  $$
  u/(2^k)=(u+(1<<k)-1)>>k
  $$

- **除数为一般立即数：**该情况下，为对除数和被除数均乘上一个大数，再进行移位操作，如下：
  $$
  u/o=(x-(u*c>>32/64)>>n1)+(u*c>>32/64)>>n2
  $$

##### 乘法优化

- **乘数为-1：**如果存在乘数是 `-1`，则使用 `0 - x` 代替 `x * -1`。
- **乘数为2的幂：**用位移操作代替乘法，与除法处理思路类似。
- **乘数为2的幂+1或-1：**在上述基础上，增加一条加法/减法指令。



#### 函数内联

`FuncInline`：函数内联旨在将可以内联的函数复制到相应的函数调用位置，减少调用函数的开销，带来更大的优化空间，函数内联的标志为不调用其他函数并且不是库函数，以此为基础即可进行迭代内联，将所有可以内联的函数都复制到相应位置。其示例如下：

```c
int fun () {
    return 1;
}

b = a + fun();
c = a + fun();
d = c + b;

//内联之后
b = a + 1;
c = a + 1;
d = c + b;

//优化之后
b = a + 1; 
d = b + b;
```

实现思路如下：

```java
//假设函数中块关系为preBlock-InlineBlock-nextBlock
//InlineBlock分为beforeFuncBlock（InlineBlock）-funcBlock-afterBlock
//1.构建afterBlock
//2.删除callInstr及后继指令
//3.修正基本块的前驱后继关系（pre,nextb）
//4.替换形参为实参
//5.处理ret\call以及储存内联函数的块
//6.ret只有一个可以直接将call替换为相应的Value跳转到nextBlock
//7.多个ret可使用phi指令
//8.加入内联函数块（即funcBlock）
//9.函数中的call构建新的函数关系
```



#### 块合并

此优化旨在将直接跳转的`br`指令以及选择跳转指令中已经确定跳转块的指令`br i1 1, block1, block2`等块合并起来，减少多余的跳转指令，实现此优化时需注意，若跳转到块中有phi指令，则不能实现块合并的操作，因为这说明跳转到的块中还有其他的块会跳转过去，直接合并会导致phi出现问题。

在合并之后，还需要维持所有的br和phi指令中的块，因为合并之后，跳转块即为`br`指令所在的块，因此br和phi指令中对应的块需转换为该块，实现如下：

```java
for (BasicBlock basicBlock : function.getBlockList()) {
                        if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                            ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(nextBlock, block);
                        }
                        if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                            Instruction instruction = basicBlock.getInstrList().get(0);
                            int count = 0;
                            while (instruction instanceof PhiInstr) {
                                ((PhiInstr) instruction).modifyBlock(nextBlock, block);
                                instruction = basicBlock.getInstrList().get(++count);
                            }
                        }
                    }
```

同时，还需要维护块的前驱后继关系：

```java
//前驱后继
                    ArrayList<BasicBlock> nextBlocks = new ArrayList<>();
                    nextBlocks = nextBlock.getNextBlock();
                    block.setNextBlock(nextBlocks);
                    for (BasicBlock basicBlock : nextBlocks) {
                        int index = basicBlock.getPreBlock().indexOf(nextBlock);
                        if (index == -1) {
                            continue;
                        }
                        basicBlock.getPreBlock().set(index, block);
                    }
```



#### 全局变量局部化

如果全局变量,不是数组且符合以下条件之一：

* 没有被使用过
* 没有被修改过（即没有被store）
* 只在main函数中被使用过

那么该全局变量就可以被下降为局部变量，即在函数开始时先`alloca`，然后将所有的全局变量变为局部变量即可暴露更多优化机会。实现如下

```java
if (useFunc.isEmpty()) { //没有被用到的全局变量
            return true;
        }

        if (storeInstrs.isEmpty()) { //没有被修改过
            for (LoadInstr loadInstr : loadInstrs) {
                loadInstr.replaceUse(globalVar.getValue());
                loadInstr.remove();
            }
            return true;
        }

        if (useFunc.size() == 1) { //只在一个函数被使用
            Function function = functions.get(0);
            if (!function.getName().equals("@main")) {
                return false;
            }
            BasicBlock block = function.getBlockList().get(0);
            AllocaInstr allocaInstr = builder.buildAllocaInstr(((PointerType) globalVar.getType()).getElementType(), block);
            StoreInstr storeInstr = builder.buildStoreInstr(globalVar.getValue(), allocaInstr, block);
            block.getInstrList().addFirst(storeInstr);
            block.getInstrList().removeLast();
            block.getInstrList().addFirst(allocaInstr);
            globalVar.replaceUse(allocaInstr);
            return true;
        }
```



#### 减少多余的Store和load

此优化大致分为如下几个小优化：

* LoadGVN(将所有`load`指令中对应的指针相同都替换为一个，前提是在中间没有store过其他值)
* 在store之后马上进行了load操作(可以利用store的值直接替换load指令)
* 若数组或全局变量在main函数中只store而没有load,那么可以直接将所有的store都删除，减少内存的开销



#### 循环优化

对于循环做了一下优化：

* LCSSA(loop close SSA)：循环闭环SSA，对于在循环中定义，且有在此循环外的使用的变量，在该循环的 exit 块添加**冗余的 phi 指令**，在进行循环优化的时候，可以只考虑对冗余 phi 的影响。

  示例：

  ```c
  c = ...;
  for (...) {
      if (c)
          X1 = ...
      else
          X2 = ...
      X3 = phi(X1, X2);
  }
  ... = X3 + 4; 
  
  //LCSSA后
  c = ...;
  for (...) {
      if (c)
          X1 = ...
      else
          X2 = ...
      X3 = phi(X1, X2);
  }
  X4 = phi(X3);
  ... = X4 + 4; 
  
  ```

* 循环展开：展开符合要求的循环，减少无用的判断和转跳，并带来更大的优化空间。首先需保证循环展开所有的变量都已知，同时循环展开的指令不能过多，否则放弃展开。

* 循环简化：主要针对以下几种情况：

  ```c
  while(i < n) {
      sum = sum + C;
      i = i + 1;
  }
  //优化后
  sum = sum + C * n;
  
  while (i < n) {
      sum = (sum + C) % D;
      i = i + 1;
  }
  //优化后
  sum = (sum % D + (C % D) * (n % D)) % D;
  
  while (i < n) {
      sum = sum + func(...) //func()的返回值是不变量
      i = i + 1;
  }
  //优化后
  int result = func(...);
  while (i < n) {
      sum = sum + result;
      i = i + 1;
  }
  ```

* 循环提取：将嵌套循环进行分离，从而暴露出更多的优化机会。

  针对情况如下：

  ```c
  while (i < n) {
      while (j < m) {
          //...
          j = j + 1;
      }
      k = 0;
      while (k < l) {
          //...
          k = k + 1;
      }
  }
  //优化后
  while (j < m) {
          //...
          j = j + 1;
  }
  //...
  while (i < n) {
      k = 0;
      while (k < l) {
          //...
          k = k + 1;
      }
  }
  ```



### 数组优化

主要为两个：

* 数组复用：对于进行多次取相同数组指针的操作，将其融合为一个`getelementptr`指令，从而方便之后的优化。示例如下：

  ```java
  /*  
      *   pre: gep 0 1 0
      *   now: gep 0 1
      *   fuse: gep 0 1 0 1
      *
      *   pre: gep 0 1 0
      *   now: gep 1 1
      *   fuse: gep 0 1 1 1
      * */
  ```

* 数组分离：在进行完所有的优化之后，我们将所有的`getelementptr`进行分离操作。

  ```java
  /*
  	pre:gep 0 1 0
  	now:gep 0 1
  		gep 0 0
  */
  ```

  从而减少读取内存的次数以及减少cache未命中次数。



#### 递归函数记忆化

对于递归函数，可做如下优化：

* 检测可以被记忆化的递归函数：
    * 无副作用，仅调用自身；
    * 不含对全局变量和参数指针的读取
* 在中层IR层面进行修改
    * 新增全局数组用来存储已被计算过的结果
    * 对参数组合计算哈希值，作为索引访问记忆化数组
    * 开头插入计算哈希值并判断是否直接返回的代码
    * 对原函数体的每一条 return 指令前插入 store 记录返回值



#### 消Phi

因为Phi指令不能被翻译为底层指令，因此需要消除Phi指令。

* 删除Phi，添加Pcopy（并行赋值）
* 删除Pcopy，添加Move
* 消除Phi指令的时候，如果当前基本块的前驱有多个后继则新建基本块消除Pcopy的时候，如果出现循环赋值，创建中间变量来解决循环的问题。

```java
public LinkedList<MoveInstr> getMove(PCopyInstr pCopyInstr) {
        ArrayList<Value> src = pCopyInstr.getNow();
        ArrayList<Value> dst = pCopyInstr.getBefore();
        LinkedList<MoveInstr> moveInstrs = new LinkedList<>();
        for (int count = 0; count < src.size(); count++) {
            MoveInstr moveInstr = new MoveInstr(src.get(count), dst.get(count));
            moveInstrs.add(moveInstr);
        }
        ArrayList<MoveInstr> midValue = new ArrayList<>();
        HashSet<Value> visited = new HashSet<>(); //?
        for (int count = 0; count < moveInstrs.size(); count++) {
            Value value = moveInstrs.get(count).getDst();
            if (!visited.contains(value)) {
                visited.add(value);
                boolean isLoopAssign = false;
                for (int count1 = count + 1; count1 < moveInstrs.size(); count1++) {
                    if (moveInstrs.get(count1).getSrc().equals(value)) {
                        isLoopAssign = true;
                        break;
                    }
                }
                if (isLoopAssign) {
                    Value midVal = new Value(value.getType(), value.getName() + "_temp");
                    MoveInstr moveInstr = new MoveInstr(midVal, value);
                    midValue.add(moveInstr);
                    for (MoveInstr moveInstr1 : moveInstrs) {
                        if (moveInstr1.getSrc().equals(value)) {
                            moveInstr1.setSrc(midVal);
                        }
                    }
                }
            }
        }

        //TODO:move中的寄存器冲突
        for (MoveInstr moveInstr : midValue) {
            moveInstrs.addFirst(moveInstr);
        }
        return moveInstrs;
    }
```



## backend后端

### 解析llvm并生成汇编指令(CodeGen)

#### 变量说明

| 变量名                | 作用                                                     |
| --------------------- | -------------------------------------------------------- |
| irModule              | 表示输入的中间表示模块                                   |
| asmModule             | 用于生成的汇编模块                                       |
| valueToComponent      | 映射中间表示中的值到汇编组件                             |
| curFunction、curBlock | 当前正在生成代码的函数和代码块                           |
| operandMap            | 映射操作数到汇编寄存器                                   |
| needChange            | 存储需要更改的汇编指令                                   |
| LibFunctions          | 一个包含库函数的映射，用于将特定的库函数名映射到汇编函数 |

​	在构造函数中，`LibFunctions`被初始化为多个预定义的库函数，这些库函数在汇编代码生成过程中可能会用到。

#### 解析全局变量

​	`getObjGlobalOther`方法负责根据全局变量的类型和初始化状态创建适当的`AsmGlobalOther`对象。对于未初始化的全局变量，计算大小并设置为未初始化状态；对于已初始化的全局变量，仅设置为已初始化状态。

​	`parseGlobal`将IR模块中的全局变量转换成汇编模块所需的格式。它处理不同类型的全局变量（数组、整数、浮点数），对于数组，根据值的类型（整数或浮点数）转化为整数表示。最后，将这些变量的值转换为汇编中使用的格式，并将其添加到汇编模块中。

#### 解析指令

**处理 `ALUInstr` 类型的指令**：

​	该类型指令包含加、减、乘、除和移位指令，对于其中每种指令均调用相应方法进行解析。一般解析方法为：获取相应操作数，并根据操作数生成相应指令。在该方法基础上，我们做了如下优化：

- 加减指令判断操作数是否全为常数，若是，则事先计算好结果，并将该指令替换为move指令，直接将计算结果赋给相应寄存器，以提高汇编代码运行效率。若不是，则判断操作数中是否包含常数，若包含则调用 `getIntOperandForImmMoreThan12()` 处理超过12位的立即数，再生成相应汇编指令。
- 乘除指令应判断乘数或除数是否为常数，若是，则将乘除指令优化为多条移位指令，已解决乘除汇编指令运行慢的问题，具体优化逻辑在中端实现。

##### 处理返回指令`RetInstr`

​	若函数有实际返回值，则应根据返回值类型决定使用哪种寄存器来储存返回值，并生成move指令来将值移到目标寄存器，并将该指令添加到当前基本块中。在恢复返回地址寄存器（ra）时，需根据 `immediate` 值的范围决定如何生成加载指令。如果 `immediate` 在一个可以直接表示的范围内，则直接使用简单的 `AsmLoad` 指令；如果超出了范围，则通过虚拟寄存器和加法操作来计算地址，并将多个汇编指令插入到基本块中以实现最终的加载操作，确保了即使在地址计算较复杂的情况下，也能够正确加载数据。最后，添加一个 `AsmRet` 指令表示函数返回。

##### 处理内存分配指令 `AllocaInstr`

​	`parseAlloc` 方法负责处理 `AllocaInstr` 指令的内存分配逻辑。它根据指针的类型和当前函数的内存分配情况，计算出一个合适的偏移量，并生成相应的汇编指令。流程包括：确定内存分配偏移量。处理偏移量以生成适当的操作数，根据指针类型更新内存分配大小，生成汇编指令并将其插入到当前基本块中。需注意，在创建操作数偏移量时，若偏移量超出了 -2048 到 2047 的范围，则通过创建一个 `AsmMove` 指令，将立即数移动到虚拟寄存器中，并将该虚拟寄存器作为操作数返回。在内存分配大小时，如果 `pointerType` 是数组类型 (`ArrayType`)，则按元素大小 (4 字节) 乘以数组的大小进行内存分配，如果 `pointerType` 是指针类型 (`PointerType`)，则为指针分配 8 字节（通常是 64 位系统中指针的大小），如果是其他类型，则分配 4 字节。

##### 处理加载指令 `LoadInstr`

​	`parseLoad` 方法负责将 `LoadInstr` 指令转换为汇编指令。它根据 `irAddress` 的类型和 `LoadInstr` 的目标操作数类型生成适当的 `AsmLoad` 指令，并将其添加到当前基本块中。对于全局变量，先使用 `AsmLa` 加载地址，再用适当的 `AsmLoad` 指令加载数据。对于指针类型，直接使用 `ld` 指令加载数据。对于其他类型，使用适当的 `AsmLoad` 指令（`flw` 或 `lw`）

##### 处理存储指令**`StoreInstr`**

​	`	parseStore` 方法根据不同类型的地址和数据，生成相应的汇编 `AsmStore` 指令，并将其添加到当前基本块中。对全局变量的地址进行处理，使用 `AsmLa` 加载地址，并选择合适的存储指令。对指向指针的指针类型使用 `sd` 指令。对其他情况，根据源操作数类型选择 `fsw` 或 `sw` 指令

##### 处理比较指令`CmpInstr`

​	`parseCmp` 方法将高层的比较操作转换为具体的汇编指令，处理不同的比较操作符，并生成适当的汇编代码。在处理等于和不等于指令时，我们进行如下优化：若操作数中存在立即数，则使用异或指令代替上述指令，以加快汇编代码运行效率。

##### 处理分支指令`BrInstr`

**判断条件**：如果 `BrInstr` 的 `getIsIf()` 方法返回 `true`，表示这是一个条件跳转指令。

- **常量条件**：如果条件值是 `ConstInt` 类型，且值为 `0`，则生成一个无条件跳转指令 `AsmJ` 跳转到 `ifFalseBlock`；否则，跳转到 `ifTrueBlock`。
- **比较条件**：如果条件值是 `CmpInstr` 类型，先生成一个条件相等指令 `AsmBeqz`，跳转到 `ifFalseBlock`；然后生成一个无条件跳转指令 `AsmJ` 跳转到 `ifTrueBlock`。
- **非条件跳转**：如果 `BrInstr` 的 `getIsIf()` 方法返回 `false`，生成一个无条件跳转指令 `AsmJ` 跳转到 `destBlock`。

​	代码使用了条件检查来决定生成的汇编指令类型，并将这些指令添加到当前块 `curBlock` 中。

##### 处理函数获取指令**`GetElementPtrInstr`**

​	 `parseGep` 方法用于将 `GetElementPtrInstr` （GEP指令）转换为相应的汇编指令。`GetElementPtrInstr` 通常用于计算结构体或数组中的元素地址。如果 GEP 指令的基地址是全局变量（`GlobalVar`），则使用 `AsmLa` 指令加载全局变量的地址。如果基地址是局部变量，则使用 `AsmMove` 指令将基地址移动到目标操作数中。然后，获取基地址所对应的数据结构的各维度的大小（单位：字节）。对于偏移量，有以下两种情况:

- **常量偏移量**：如果索引是 `ConstInt` 类型，计算出偏移量 `offset`，并根据偏移量的大小选择合适的操作数。然后使用 `AsmBinary` 指令将偏移量加到目标操作数上。

- **非常量偏移量**：如果索引不是常量，首先计算出乘法操作的结果，然后将该结果加到目标操作数上。对于非常量的索引，先生成一个乘法指令，并通过 `ALUOptimize.simplifyMul` 方法对其进行优化（具体在中端中实现）。然后将优化后的指令解析为汇编指令。

  最后更新目标数，最终生成对应的汇编指令。

##### 处理函数调用指令**`CallInstr`**

​	`parseCall`方法用于将函数参数传递给目标函数，执行函数调用，并处理返回值。通过获取函数指令 `inst` 指向的目标函数。如果目标函数在 `LibFunctions` 中已存在，则使用库函数中的版本。对于每个参数，如果其索引小于等于8，则直接将其移动到物理寄存器 中对应的位置，若其索引大于等于 8，则先将其移动到一个虚拟寄存器中，再将虚拟寄存器的值存储到堆栈中，根据参数的类型（整数或浮点），选择相应的存储指令（`sw` 或 `sd`）。之后，创建并添加一个 `AsmCall` 指令到当前代码块中，表示对目标函数的调用。如果函数有返回值，则根据返回值的类型（整数或浮点数）将结果从物理寄存器 `phyA[0]` 或 `fPhyA[0]` 移动到目标操作数中。整个过程确保了函数调用时参数的正确传递，函数调用的正确执行，以及返回值的正确处理。

##### 处理转换指令`ConversionInst`

​	`	parserConversionInst`根据指令对象类型和操作数，生成不同的汇编指令对象，将高级语言中的数据类型转换操作映射到低级语言中。

#### 解析函数对象

##### 预处理

​	首先遍历中间表示（IR）函数中的每个基本块（`BasicBlock`）并从映射中获取对应的汇编基本块，为汇编基本块设置前驱和后继基本块。

##### 计算函数所需的栈空间大小

​	调用`stackNeedToExpand` 方法通过遍历函数中的所有基本块和指令，特别是调用指令，来估算函数调用时额外需要的栈空间。这个估算是通过检查每个调用指令的操作数来完成的，假设从第 8 个操作数开始，指针类型参数需要 8 字节，非指针类型参数需要 4 字节。

##### 解析块与指令

​	遍历 IR 函数的基本块列表，进而解析块中指令。

##### 处理函数参数

​	如果函数没有参数，则处理函数的浮点参数和整数参数：

- 获取浮点参数列表 (`fParams`) 和整数参数列表 (`iParams`)。
- 遍历浮点参数，如果参数的索引小于 8，将其移动到物理寄存器 (`fPhyA` 中的寄存器)；否则，将其加载到栈中（通过 `getLoadInsertHead` 方法）。
- 遍历整数参数，如果参数的索引小于 8，将其移动到物理寄存器 (`phyA` 中的寄存器)；否则，根据参数的类型（整数或浮点），决定将其加载到栈中（`lw` 或 `ld` 指令），并更新栈偏移量。
- 调用 `resetStack` 方法重置栈状态。

​	如果函数有参数 (`curFunction.isIfHasParam()`)，则直接调用 `resetStack` 方法重置栈状态，而不处理参数。





### 寄存器分配（RegAllocator）

#### 活跃性分析

​	活跃性分析作为寄存器分配首要环节，包含：分析寄存器的使用情况，每个基本块的信息，每条指令的输入输出，处理特定的存储操作或相关情况。

##### 分析寄存器使用情况

​	初始化寄存器的最大索引值，并遍历函数中的所有基本块和指令，更新每个寄存器的最大索引值，同时根据寄存器的类型和使用情况，将寄存器分类为使用寄存器或定义寄存器，并将它们添加到相应的集合中，用于后续的寄存器分配和优化。

##### 分析基本块信息

​	`analyzeBlock` 方法通过深度优先搜索（DFS）从退出块开始遍历所有基本块，持续执行直到所有块的活跃寄存器状态稳定。`dfs` 方法递归地遍历基本块及其前驱块，调用 `computeLiveInOut` 更新每个块的 `liveIns` 和 `liveOuts` 寄存器集合。`computeLiveInOut` 方法首先备份当前基本块的 `liveIns`，然后清除并更新 `liveOuts` 集合，接着更新 `liveIns` 集合并移除已定义的寄存器。最后，通过比较备份和更新后的 `liveIns` 集合来判断是否有变化，返回是否发生了变化以便继续迭代。这种方法确保了寄存器活跃性的计算准确并且稳定。

##### 分析指令中活跃寄存器状态

​	`analyzeInstInOut` 方法遍历每个基本块的指令列表，从最后一条指令开始向前处理。对于每条指令，方法首先检查是否是基本块中的第一条指令。如果是，清除指令的寄存器状态，设置其活跃输入（`liveIn`）为基本块的活跃输入寄存器集合，并记录局部干扰。对于其他指令，方法基于指令的寄存器定义和使用情况更新活跃输入（`liveIn`）寄存器集合，并计算新的活跃输出（`liveOuts`）寄存器集合。它将每条指令的活跃输入更新为包含其使用的寄存器，并考虑到活跃输出中未被定义的寄存器。更新后的寄存器状态被应用于每条指令，并对基本块的局部干扰集合进行记录，直到处理完所有指令。这一过程确保了每条指令的活跃寄存器状态准确反映基本块的寄存器使用情况。

##### 处理特定的存储操作

​	遍历每个基本块，将每个基本块的活跃输入寄存器添加到 `SForOperands` 集合中。接着，它检查每条指令的前驱指令是否为 `AsmCall` 类型，如果是，则将该指令的活跃输入寄存器也添加到 `SForOperands` 中。最后，方法遍历所有操作数，将那些不在 `SForOperands` 中的操作数添加到 `TForOperands` 集合中。这一过程帮助标识和管理指令中涉及的寄存器和操作数，以便于进一步的处理或优化。

#### 构建冲突图

​		首先处理每个基本块，统计寄存器的使用频率和定义频率。对于每个基本块，代码遍历其指令列表，使用 `multiResult` 方法更新每个寄存器的频率统计信息。之后，基于寄存器的使用频率、基本块深度和其他参数，计算寄存器的溢出权重，并更新 `spillWeight` 映射。接着，方法处理每个基本块的活跃寄存器集合（`liveInsReg`），通过 `multiStage` 方法计算每对活跃寄存器之间的冲突关系，同时进行如下加边操作：

- 检查边是否已存在于 `adjSet` 中，并且确保 `left` 和 `right` 不相等。
- 将 `left` 和 `right` 之间的边（双向）添加到 `adjSet` 中，表示它们之间有冲突。
- 如果 `left` 是预着色的寄存器（`isPreColored()` 返回 true），则更新其邻接列表 `adjList` 并增加其度数 `degree`。
- 对 `right` 进行相同的处理，更新邻接列表和度数。

​	此外，它还处理基本块中的局部干扰寄存器集合（`blockLocalInterfere`），同样计算这些寄存器之间的冲突。`multiStage` 方法用于在冲突图中建立冲突边，记录每对寄存器之间的冲突关系。最终，这些冲突关系形成了寄存器分配的冲突图，为进一步的寄存器分配和优化提供了基础。

#### 工作列表划分

##### 变量定义

| 变量名              | 作用                                                   |
| ------------------- | ------------------------------------------------------ |
| **`K`**             | 一个阈值，用于区分高度数和低度数的寄存器。             |
| **`spillWorkList`** | 用于存储需要溢出处理的寄存器，即度数较高的寄存器。     |
| **`simWorkList`**   | 用于存储可以简化的寄存器，即度数较低的寄存器。         |
| **`degree`**        | 存储操作数（寄存器）的度数，即它们在冲突图中的连接边数 |

##### 流程

遍历操作数，并检查度数：

- 如果 `degree` 映射中不包含该操作数，则跳过该操作数。
- 如果操作数的度数大于或等于预设的阈值 `K`，则将该操作数添加到溢出工作列表 `spillWorkList` 中。
- 如果操作数的度数小于 `K`，则将其添加到简化工作列表 `simWorkList` 中。

**简化操作：**如果 `simWorkList` 不为空，调用 `simplify()` 方法将寄存器从图中移除，简化图的结构。

**选择溢出：**如果 `simWorkList` 为空但 `spillWorkList` 不为空，则调用 `selectSpill()` 方法选择一个寄存器，将其从图中移除，并插入到溢出寄存器列表中。

#### 图着色

​	从 `stackForSelect` 栈中逐一取出操作数（寄存器），为其选择合适的颜色。根据是否是浮点寄存器和阶段（`isFirstStage`），它初始化一个可用颜色集合 `baseOkColors`。然后，通过检查与当前操作数相邻的已着色寄存器，排除这些寄存器的颜色，更新 `baseOkColors`。如果没有可用颜色，则将寄存器标记为溢出节点；否则，给寄存器分配一个可用颜色，并将其标记为已着色，确保没有两个冲突的寄存器分配相同的颜色。

#### 处理溢出

​	如果有溢出节点，则应调用重写程序，遍历函数，确定需要重写的指令，并在这些指令中插入适当的内存操作（加载和存储），以处理因寄存器溢出而引起的寄存器位置调整。后再次调用寄存器分配操作。

#### 最终分配

​	根据寄存器分配结果，将颜色（寄存器编号）分配给每个寄存器对象，完成寄存器分配的最终步骤，并重置每个函数栈的大小。