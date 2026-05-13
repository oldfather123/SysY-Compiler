## IR 完整逻辑详解

### 一、数据结构层 (Defs.h/cpp)

整个 IR 采用类 MLIR 的分层结构：

```
Region (模块/函数容器)
 └── BasicBlock (基本块，顺序执行)
      └── Op (单条指令)
           ├── Opcode (操作码)
           ├── ValueType (返回值类型: Void/Int/Float)
           ├── operands (操作数列表，每个是 Value)
           ├── attrs (键值对属性)
           └── uses (使用链，记录谁引用了我)
```

**Value** 是轻量级指针包装，指向定义它的 Op：

```
class Value {
    Op *defining_;  // 谁产生了这个值
};
```

**Op** 是核心指令单元：

```
class Op {
    Opcode opcode_;              // 做什么
    ValueType returnType_;       // 产出类型
    std::vector<Value> operands_; // 输入操作数
    std::unordered_set<Op*> uses_; // 谁用了我（使用链）
    BasicBlock *parent_;         // 属于哪个基本块
};
```

**关键设计**：

- **所有权**：`unique_ptr` 管理生命周期，父级持有子级
- **使用链**：Op 构造时自动注册到被引用 Op 的 `uses_` 集合；删除时自动清理
- **eraseFromParent**：删除指令时，断开所有操作数的使用链，并把引用自己的操作数置空

------

### 二、操作码分类 (Ops.h)

```
┌─────────────────────────────────────────────────────────┐
│ 整数算术:  AddI SubI MulI DivI ModI AndI OrI XorI       │
│ 浮点算术:  AddF SubF MulF DivF ModF                      │
│ 地址算术:  AddL SubL MulL DivL ModL (用于指针偏移)       │
├─────────────────────────────────────────────────────────┤
│ 整数比较:  Eq Ne Lt Le                                   │
│ 浮点比较:  EqF NeF LtF LeF                               │
│ 注意: Greater/GreaterEqual 通过交换操作数转为 Lt/Le      │
├─────────────────────────────────────────────────────────┤
│ 控制流:    Goto Branch Return Break Continue              │
│ 函数:      Func Call GetArg                               │
│ 内存:      Alloca Store Load Global GetGlobal             │
│ 类型转换:  F2I I2F SetNotZero                             │
│ 一元:      Minus MinusF Not                               │
│ 位移:      LShift RShift LShiftL RShiftL                  │
│ 其他:      Phi Select Module Mulsh Muluh                  │
└─────────────────────────────────────────────────────────┘
```

------

### 三、Builder — 指令构造器

Builder 维护一个"插入点"（当前基本块），所有 `createOp` 自动 append 到该块末尾：

```
Builder::Builder(Region *region) {
    // 创建 entry 块作为初始插入点
    insertBlock_ = region_->createBlock("entry");
}

Op *Builder::createOp(...) {
    auto op = std::make_unique<Op>(...);
    return insertBlock_->appendOp(std::move(op));  // 插入当前块
}
```

------

### 四、CodeGen — AST→IR 降低

入口在 [CodeGen.cpp:127-134](vscode-webview://1ef3p2ghdsg6gv7eg2uni28vu7qd0lbku4upla3c093hdur9d1g0/src/codegen/CodeGen.cpp#L127-L134)，构造函数遍历 AST 的顶层声明。

#### 4.1 全局变量 (`emitVarDecl`)

```
GlobalOp <name="x", type="int", size=4>
```

全局变量生成 `GlobalOp`，无操作数，属性记录名称/类型/大小。如果有初始化器，再生成 `StoreOp`。

#### 4.2 函数定义 (`emitFunctionDecl`)

```
FuncOp <name="main", ret=int, argc=0>     ← 函数声明
block 1 (fn.entry):                       ← 新基本块
  %0 = getarg <idx=0, name=a>             ← 取参数
  %1 = alloca <name=a, type=int, size=4>  ← 参数分配栈空间
  %2 = store (%0, %1)                     ← 存参数到栈
  %3 = ...                                ← 函数体
  %n = ret (%3)                           ← 返回
```

**参数处理**：每个参数先 `GetArgOp` 取值，再 `AllocaOp` 分配栈空间，再 `StoreOp` 存入。后续代码统一通过 `LoadOp` 读取。

#### 4.3 if/else 控制流

```
  %cond = ...                             ← 条件表达式
  branch (%cond) <true=if.then.3, false=if.end.5>
block 2 (if.then.3):
  ... then 语句 ...
  goto <target=if.end.5>
block 3 (if.else.4):                      ← 仅有 else 分支时存在
  ... else 语句 ...
  goto <target=if.end.5>
block 4 (if.end.5):
  ... 后续代码 ...
```

#### 4.4 while 循环

```
  goto <target=while.cond.6>
block 5 (while.cond.6):                  ← 条件块
  %cond = ...
  branch (%cond) <true=while.body.7, false=while.end.8>
block 6 (while.body.7):                  ← 循环体
  break → goto while.end
  continue → goto while.cond
  ... 体代码 ...
  goto <target=while.cond.6>             ← 跳回条件
block 7 (while.end.8):                   ← 循环出口
```

**break/continue 处理**：

- `loopTargets_` 栈维护当前循环的 `{endBlock, condBlock}`
- break → `goto endBlock`，之后插入 dead 块（不可达代码收集器）
- continue → `goto condBlock`，同样插入 dead 块

#### 4.5 表达式求值 (`emitExpr`)

| AST 节点                     | IR 生成                         |
| ---------------------------- | ------------------------------- |
| `IntegerLiteral`             | `IntOp <value=42>`              |
| `FloatLiteral`               | `FloatOp <value=3.14>`          |
| `DeclRefExpr` (标量)         | `LoadOp` 从变量地址读取         |
| `DeclRefExpr` (数组)         | 直接返回地址（数组名即首地址）  |
| `UnaryOperator -`            | `MinusOp`/`MinusFOp`            |
| `UnaryOperator !`            | `NotOp`                         |
| `BinaryOperator +`           | `AddIOp`/`AddFOp`               |
| `BinaryOperator >`           | 交换操作数 + `LtOp`/`LtFOp`     |
| `BinaryOperator &&`          | `AndIOp` (注意：非短路！)       |
| `BinaryOperator ||`          | `OrIOp` (注意：非短路！)        |
| `BinaryOperator =`           | `emitLValueAddress` + `StoreOp` |
| `CallExpr`                   | `CallOp <callee="func">`        |
| `ImplicitCast int→float`     | `I2FOp`                         |
| `ImplicitCast float→int`     | `F2IOp`                         |
| `ImplicitCast x→bool`        | `SetNotZeroOp`                  |
| `ImplicitCast lvalue→rvalue` | 透传（不生成额外指令）          |
| `ArraySubscriptExpr`         | `emitLValueAddress` + `LoadOp`  |

#### 4.6 数组地址计算 (`emitLValueAddress`)

```
a[i][j] 的地址计算:
  base = alloca 的地址 (或参数 load 出的指针)
  stride = shape[1] * shape[2] * ... * 4  (字节)
  offset = i * stride
  addr = base + offset
  // 递归处理下一层下标
```

核心逻辑在 [CodeGen.cpp:464-479](vscode-webview://1ef3p2ghdsg6gv7eg2uni28vu7qd0lbku4upla3c093hdur9d1g0/src/codegen/CodeGen.cpp#L464-L479)：

- `MulIOp(index, strideBytes)` 计算字节偏移
- `AddLOp(baseAddr, byteOffset)` 得到元素地址

#### 4.7 数组初始化器

对 `InitListExpr` 递归展开，每个元素通过 `StoreOp` 写入计算好的地址：

```
base + linearOffset * 4  ← 元素地址
StoreOp(value, elemAddr) ← 写入
```

未被显式初始化的位置由 `arrayFiller` 填零。

------

### 五、IRDumper — IR 文本输出

格式：

```
block 0 (entry):
  %0 = global <name=x, type=int, size=4>
  %1 = int <value=42>
  %2 = store (%1, %0)

block 1 (fn.entry):
    %3 = getarg <idx=0, name=n>
    %4 = alloca <name=n, type=int, size=4>
    %5 = store (%3, %4)
    ...
    %n = ret (%result)
```

- 每个 Op 分配全局递增 ID（`%0`, `%1`, ...）
- 操作数引用显示为定义者的 ID
- 属性用 `<key=value>` 格式
- 函数体内指令缩进两层

------

### 六、当前 IR 的特点与局限

**特点**：

- 非 SSA：变量通过 `Alloca`/`Store`/`Load` 访问，未做 Mem2Reg
- 控制流用 `Goto`/`Branch` 终结符，CFG 已显式化
- 使用链内建，支持后续 DCE 等优化
- 比较运算只有 `<` 和 `<=`，`>`/`>=` 通过交换操作数实现

**局限**：

- `AndIOp`/`OrIOp` 用于逻辑运算，**未实现短路求值**（两边都会求值）
- `PhiOp` 已定义但 **CodeGen 不生成**（Mem2Reg 需要）
- `BreakOp`/`ContinueOp` 是冗余标记指令（实际跳转由 `GotoOp` 完成）
- `WhileOp`/`IfOp`/`ForOp`/`ProceedOp` 已定义但 **未使用**（用 BasicBlock 拆分代替）
- 没有 `FuncOp` 的结束标记，函数边界靠 BasicBlock 序列隐式确定