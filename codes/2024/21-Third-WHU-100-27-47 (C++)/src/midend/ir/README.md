# IR
// TODO

## 相关头文件

-   [`midend/ir/IR.h`](/src/midend/ir/IR.h)：需要用到 IR 时，包含该头文件即可。

## 代码约定

### 数据类型

-   所有指针均使用 `std::shared_ptr`。
-   所有列表均使用 `std::vector`。
-   所有字符串均使用 `std::string`。
-   所有无序集均使用 `std::unordered_set`。

### 类的属性

-   带有 `const` 修饰符的类属性是*只读*的。
-   不带 `const` 修饰符的类属性是*引用类型*，可以读写。

### 构造函数

-   为了提高可读性，一些静态方法被作为构造函数使用。

## 可能存在的问题

-   代码在 Ide 没有提示错误和警告，但还没有编译和测试过。
-   类的定义可能有不合理或不符合需求之处，需要在后续编码过程中发现并调整。
-   使用 `const &` 修饰符的形参，在一些情况下可能对调用产生不便。
-   有部分代码还没有实现，包括 `ir::OprExpr::getType()`。
-   文档还在完善中，目前还没有写[语句](#语句基类irinst)和[表达式](#表达式基类irexpr)的派生类部分。

## 定义

### 基本数据类型（`ir::PrimaryDataType`，枚举）

#### 取值范围

-   `Void`：用于 void 类型的函数返回值
-   `Int`：整型
-   `Float`：浮点型

### 数据类型（`ir::DataType`，结构体）

#### 构造函数

-   `DataType(baseDataType, [arrayDimension = 0])`：指定基本类型和数组维度
-   `DataType::arrayOf(dataType)`：得到 `dataType`（非 void 类型）的数组类型
-   `DataType::elemOf(dataType)`：得到 `dataType`（至少一维的数组类型）的元素类型

#### 运算符重载

-   `== ir::PrimaryDataType`，`!= ir::PrimaryDataType`：与基本数据类型比较是否相等

#### 属性

-   `baseDataType()`：基本类型（_只读_）
-   `arrayDimension()`：数组维度（_只读_）

#### 方法

-   `isPrimary()`：是否为基本类型

### 数值（`ir::Value`，结构体）

即字面值；类型可以为整型、浮点型或 void。

#### 构造函数

-   `Value::ofVoid()`：创建一个 void 类型的数值
-   `Value(intValue)`：初始化一个整型数值
-   `Value(floatValue)`：初始化一个浮点型数值

#### 属性

-   `dataType()`：数据类型（_只读_）

#### 方法

-   `getInt()`：获取整型值（如果不是整型则报错）
-   `getFloat()`：获取浮点型值（如果不是浮点型则报错）

### 寄存器（`ir::Register`）

虚拟寄存器，即全局变量、局部变量或函数形参；可被[语句](#语句基类irinst)和[表达式](#表达式基类irexpr)引用。

#### 属性

-   `isConst()`：是否为常量
-   `dataType()`：数据类型
-   `name()`：变量名
-   `scope()`：作用域（_只读_）

### 模块（`ir::Module`）

即单个文件；是全局声明（包括全局[寄存器](#寄存器irregister)和[函数](#函数irfunction)）的集合。

#### 属性

-   `globalinsts()`：全局语句集（_只读_）
-   `globalFuncSet()`：全局函数集（_只读_）

#### 方法

-   `appendGlobalInst(that, globalInst)`：添加全局语句
-   `removeGlobalInst(that)`：移除全局语句
-   `addGlobalFunc(that, globalFunc)`：添加全局函数
-   `removeGlobalFunc(that, globalFunc)`：移除全局函数

### 函数（`ir::Function`）

是[模块](#模块irmodule)的构成元素（_根据语言定义，所有函数都是全局函数_）；可以在其他函数内被调用；具有返回值类型和若干形参。

#### 属性

-   `parentModule()`：所属的模块（_只读_）
-   `retType()`：返回值类型
-   `name()`：函数名
-   `paramList()`：形参列表（_只读_）
-   `bbSet()`：基本块集（_只读_）

#### 方法

-   `addBB(bb)`：添加基本块
-   `removeBB(bb)`：移除基本块

### 基本块（`ir::BasicBlock`）

是[函数](#函数irfunction)的构成元素；包含一系列[语句](#语句基类irinst)。缩写为 BB。

#### 属性

-   `parentFunction()`：所属的函数（_只读_）
-   `label()`：标签，即基本块的名称
-   `insts()`：语句集合

#### 方法

-   `appendInst(that, inst)`：添加语句
-   `clearInstSet(that)`：清空语句列表

### 语句（基类：`ir::Instruction`）

#### 属性

-   `parentBB()`：所属的基本块（_只读_）

### 表达式（基类：`ir::Expr`）

#### 方法

-   `getType()`：递归推导表达式的类型（_后续或许可以尝试记忆化推导，前提是已经推导过的节点不会更新_）
