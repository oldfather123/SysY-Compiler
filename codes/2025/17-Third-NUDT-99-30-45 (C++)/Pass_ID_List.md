# 记录中端遍的开发进度

| 名称       | 优化级别     | 开发进度   |
| ------------ | ------------ | ---------- |
| CFG优化     | 函数级   | 已完成     |
| DCE         | 函数级   | 待正确性测试     |
| Mem2Reg         | 函数级   | 待正确性测试     |
| Reg2Mem         | 函数级   | 待正确性测试     |


# 部分优化遍的说明

## Mem2Reg

Mem2Reg 遍的主要目标是将那些不必要的、只用于局部标量变量的内存分配 (alloca 指令) 消除，并将这些变量的值转换为 SSA 形式。这有助于减少内存访问，提高代码效率，并为后续的优化创造更好的条件。

通过Mem2Reg理解删除指令时对use关系的维护：

在 `Mem2Reg` 优化遍中，当 `load` 和 `store` 指令被删除时，其 `use` 关系（即它们作为操作数与其他 `Value` 对象之间的连接）的正确消除是一个关键问题，尤其涉及到 `AllocaInst`。

结合您提供的 `Mem2RegContext::renameVariables` 代码和我们之前讨论的 `usedelete` 逻辑，下面是 `use` 关系如何被正确消除的详细过程：

### 问题回顾：`Use` 关系的双向性

在您的 IR 设计中，`Use` 对象扮演着连接 `User`（使用者，如 `LoadInst`）和 `Value`（被使用者，如 `AllocaInst`）的双向角色：

* 一个 `User` 持有对其操作数 `Value` 的 `Use` 对象（通过 `User::operands` 列表）。
* 一个 `Value` 持有所有使用它的 `User` 的 `Use` 对象（通过 `Value::uses` 列表）。

原始问题是：当一个 `LoadInst` 或 `StoreInst` 被删除时，如果不对其作为操作数与 `AllocaInst` 之间的 `Use` 关系进行明确清理，`AllocaInst` 的 `uses` 列表中就会留下指向已删除 `LoadInst` / `StoreInst` 的 `Use` 对象，导致内部的 `User*` 指针悬空，在后续访问时引发 `segmentation fault`。

### `Mem2Reg` 中 `load`/`store` 指令的删除行为

在 `Mem2RegContext::renameVariables` 函数中，`load` 和 `store` 指令被处理时，其行为如下：

1.  **处理 `LoadInst`：**
    当找到一个指向可提升 `AllocaInst` 的 `LoadInst` 时，其用途会被 `replaceAllUsesWith(allocaToValueStackMap[alloca].top())` 替换。这意味着任何原本使用 `LoadInst` 本身计算结果的指令，现在都直接使用 SSA 值栈顶部的 `Value`。
    **重点：** 这一步处理的是 `LoadInst` 作为**被使用的值 (Value)** 时，其 `uses` 列表的清理。即，将 `LoadInst` 的所有使用者重定向到新的 SSA 值，并把这些 `Use` 对象从 `LoadInst` 的 `uses` 列表中移除。

2.  **处理 `StoreInst`：**
    当找到一个指向可提升 `AllocaInst` 的 `StoreInst` 时，`StoreInst` 存储的值会被压入值栈。`StoreInst` 本身并不产生可被其他指令直接使用的值（其类型是 `void`），所以它没有 `uses` 列表需要替换。
    **重点：** `StoreInst` 的主要作用是更新内存状态，在 SSA 形式下，它被移除后需要清理它作为**使用者 (User)** 时的操作数关系。

在这两种情况下，一旦 `load` 或 `store` 指令的 SSA 转换完成，它们都会通过 `instIter = SysYIROptUtils::usedelete(instIter)` 被显式删除。

### `SysYIROptUtils::usedelete` 如何正确消除 `Use` 关系

关键在于对 `SysYIROptUtils::usedelete` 函数的修改，使其在删除指令时，同时处理该指令作为 `User` 和 `Value` 的两种 `Use` 关系：

1.  **清理指令作为 `Value` 时的 `uses` 列表 (由 `replaceAllUsesWith` 完成)：**
    在 `usedelete` 函数中，`inst->replaceAllUsesWith(UndefinedValue::get(inst->getType()))` 的调用至关重要。这确保了：
    * 如果被删除的 `Instruction`（例如 `LoadInst`）产生了结果值并被其他指令使用，所有这些使用者都会被重定向到 `UndefinedValue`（或者 `Mem2Reg` 中具体的 SSA 值）。
    * 这个过程会遍历 `LoadInst` 的 `uses` 列表，并将这些 `Use` 对象从 `LoadInst` 的 `uses` 列表中移除。这意味着 `LoadInst` 自己不再被任何其他指令使用。

2.  **清理指令作为 `User` 时其操作数的 `uses` 列表 (由 `RemoveUserOperandUses` 完成)：**
    这是您提出的、并已集成到 `usedelete` 中的关键改进点。对于一个被删除的 `Instruction`（它同时也是 `User`），我们需要清理它**自己使用的操作数**所维护的 `use` 关系。
    * 例如，`LoadInst %op1` 使用了 `%op1`（一个 `AllocaInst`）。当 `LoadInst` 被删除时，`AllocaInst` 的 `uses` 列表中有一个 `Use` 对象指向这个 `LoadInst`。
    * `RemoveUserOperandUses` 函数会遍历被删除 `User`（即 `LoadInst` 或 `StoreInst`）的 `operands` 列表。
    * 对于 `operands` 列表中的每个 `std::shared_ptr<Use> use_ptr`，它会获取 `Use` 对象内部指向的 `Value`（例如 `AllocaInst*`），然后调用 `value->removeUse(use_ptr)`。
    * 这个 `removeUse` 调用会负责将 `use_ptr` 从 `AllocaInst` 的 `uses` 列表中删除。

### 总结

通过在 `SysYIROptUtils::usedelete` 中同时执行这两个步骤：

* `replaceAllUsesWith`：处理被删除指令**作为结果被使用**时的 `use` 关系。
* `RemoveUserOperandUses`：处理被删除指令**作为使用者（User）时，其操作数**的 `use` 关系。

这就确保了当 `Mem2Reg` 遍历并删除 `load` 和 `store` 指令时，无论是它们作为 `Value` 的使用者，还是它们作为 `User` 的操作数，所有相关的 `Use` 对象都能被正确地从 `Value` 的 `uses` 列表中移除，从而避免了悬空指针和后续的 `segmentation fault`。

最后，当所有指向某个 `AllocaInst` 的 `load` 和 `store` 指令都被移除后，`AllocaInst` 的 `uses` 列表将变得干净（只包含 Phi 指令，如果它们在 SSA 转换中需要保留 Alloca 作为操作数），这时在 `Mem2RegContext::cleanup()` 阶段，`SysYIROptUtils::usedelete(alloca)` 就可以安全地删除 `AllocaInst` 本身了。

## Reg2Mem

我们的Reg2Mem 遍的主要目标是作为 Mem2Reg 的一种逆操作，但更具体是解决后端无法识别 PhiInst 指令的问题。主要的速录是将函数参数和 PhiInst 指令的结果从 SSA 形式转换回内存形式，通过插入 alloca、load 和 store 指令来实现。其他非 Phi 的指令结果将保持 SSA 形式。

## SCCP

SCCP（稀疏条件常量传播）是一种编译器优化技术，它结合了常量传播和死代码消除。其核心思想是在程序执行过程中，尝试识别并替换那些在编译时就能确定其值的变量（常量），同时移除那些永远不会被执行到的代码块（不可达代码）。

以下是 SCCP 的实现思路：

1. 核心数据结构与工作列表：

Lattice 值（Lattice Value）: SCCP 使用三值格（Three-Valued Lattice）来表示变量的状态：

Top (T): 初始状态，表示变量的值未知，但可能是一个常量。

Constant (C): 表示变量的值已经确定为一个具体的常量。

Bottom (⊥): 表示变量的值不确定或不是一个常量（例如，它可能在运行时有多个不同的值，或者从内存中加载）。一旦变量状态变为 Bottom，它就不能再变回 Constant 或 Top。

SSAPValue: 封装了 Lattice 值和常量具体值（如果状态是 Constant）。

*valState (map<Value, SSAPValue>):** 存储程序中每个 Value（变量、指令结果等）的当前 SCCP Lattice 状态。

*ExecutableBlocks (set<BasicBlock>):** 存储在分析过程中被确定为可执行的基本块。

工作列表 (Worklists):

cfgWorkList (queue<pair<BasicBlock, BasicBlock>>):** 存储待处理的控制流图（CFG）边。当一个块被标记为可执行时，它的后继边会被添加到这个列表。

*ssaWorkList (queue<Instruction>):** 存储待处理的 SSA (Static Single Assignment) 指令。当一个指令的任何操作数的状态发生变化时，该指令就会被添加到这个列表，需要重新评估。

2. 初始化：

所有 Value 的状态都被初始化为 Top。

所有基本块都被初始化为不可执行。

函数的入口基本块被标记为可执行，并且该块中的所有指令被添加到 ssaWorkList。

3. 迭代过程 (Fixed-Point Iteration)：

SCCP 的核心是一个迭代过程，它交替处理 CFG 工作列表和 SSA 工作列表，直到达到一个不动点（即没有更多的状态变化）。

处理 cfgWorkList:

从 cfgWorkList 中取出一个边 (prev, next)。

如果 next 块之前是不可执行的，现在通过 prev 块可达，则将其标记为可执行 (markBlockExecutable)。

一旦 next 块变为可执行，其内部的所有指令（特别是 Phi 指令）都需要被重新评估，因此将它们添加到 ssaWorkList。

处理 ssaWorkList:

从 ssaWorkList 中取出一个指令 inst。

重要： 只有当 inst 所在的块是可执行的，才处理该指令。不可执行块中的指令不参与常量传播。

计算新的 Lattice 值 (computeLatticeValue): 根据指令类型和其操作数的当前 Lattice 状态，计算 inst 的新的 Lattice 状态。

常量折叠: 如果所有操作数都是常量，则可以直接执行运算并得到一个新的常量结果。

Bottom 传播: 如果任何操作数是 Bottom，或者运算规则导致不确定（例如除以零），则结果为 Bottom。

Phi 指令的特殊处理: Phi 指令的值取决于其所有可执行的前驱块传入的值。

如果所有可执行前驱都提供了相同的常量 C，则 Phi 结果为 C。

如果有任何可执行前驱提供了 Bottom，或者不同的可执行前驱提供了不同的常量，则 Phi 结果为 Bottom。

如果所有可执行前驱都提供了 Top，则 Phi 结果仍为 Top。

更新状态: 如果 inst 的新计算出的 Lattice 值与它当前存储的值不同，则更新 valState[inst]。

传播变化: 如果 inst 的状态发生变化，那么所有使用 inst 作为操作数的指令都可能受到影响，需要重新评估。因此，将 inst 的所有使用者添加到 ssaWorkList。

处理终结符指令 (BranchInst, ReturnInst):

对于条件分支 BranchInst，如果其条件操作数变为常量：

如果条件为真，则只有真分支的目标块是可达的，将该边添加到 cfgWorkList。

如果条件为假，则只有假分支的目标块是可达的，将该边添加到 cfgWorkList。

如果条件不是常量（Top 或 Bottom），则两个分支都可能被执行，将两边的边都添加到 cfgWorkList。

这会影响 CFG 的可达性分析，可能导致新的块被标记为可执行。

4. 应用优化 (Transformation)：

当两个工作列表都为空，达到不动点后，程序代码开始进行实际的修改：

常量替换:

遍历所有指令。如果指令的 valState 为 Constant，则用相应的 ConstantValue 替换该指令的所有用途 (replaceAllUsesWith)。

将该指令标记为待删除。

对于指令的操作数，如果其 valState 为 Constant，则直接将操作数替换为对应的 ConstantValue（常量折叠）。

删除死指令: 遍历所有标记为待删除的指令，并从其父基本块中删除它们。

删除不可达基本块: 遍历函数中的所有基本块。如果一个基本块没有被标记为可执行 (ExecutableBlocks 中不存在)，则将其从函数中删除。但入口块不能删除。

简化分支指令:

遍历所有可执行的基本块的终结符指令。

对于条件分支 BranchInst，如果其条件操作数在 valState 中是 Constant：

如果条件为真，则将该条件分支替换为一个无条件跳转到真分支目标块的指令。

如果条件为假，则将该条件分支替换为一个无条件跳转到假分支目标块的指令。

更新 CFG，移除不可达的分支边和其前驱信息。

computeLatticeValue 的具体逻辑：

这个函数是 SCCP 的核心逻辑，它定义了如何根据指令类型和操作数的当前 Lattice 状态来计算指令结果的 Lattice 状态。

二元运算 (Add, Sub, Mul, Div, Rem, ICmp, And, Or):

如果任何一个操作数是 Bottom，结果就是 Bottom。

如果任何一个操作数是 Top，结果就是 Top。

如果两个操作数都是 Constant，执行实际的常量运算，结果是一个新的 Constant。

一元运算 (Neg, Not):

如果操作数是 Bottom，结果就是 Bottom。

如果操作数是 Top，结果就是 Top。

如果操作数是 Constant，执行实际的常量运算，结果是一个新的 Constant。

Load 指令: 通常情况下，Load 的结果会被标记为 Bottom，因为内存内容通常在编译时无法确定。但如果加载的是已知的全局常量，可能可以确定。在提供的代码中，它通常返回 Bottom。

Store 指令: Store 不产生值，所以其 SSAPValue 保持 Top 或不关心。

Call 指令: 大多数 Call 指令（尤其是对外部或有副作用的函数）的结果都是 Bottom。对于纯函数，如果所有参数都是常量，理论上可以折叠，但这需要额外的分析。

GetElementPtr (GEP) 指令: GEP 计算内存地址。如果所有索引都是常量，地址本身是常量。但 SCCP 关注的是数据值，因此这里通常返回 Bottom，除非有特定的指针常量跟踪。

Phi 指令: 如上所述，基于所有可执行前驱的传入值进行聚合。

Alloc 指令: Alloc 分配内存，返回一个指针。其内容通常是 Bottom。

Branch 和 Return 指令: 这些是终结符指令，不产生一个可用于其他指令的值，通常 SSAPValue 保持 Top 或不关心。

类型转换 (ZExt, SExt, Trunc, FtoI, ItoF): 如果操作数是 Constant，则执行相应的类型转换，结果仍为 Constant。对于浮点数转换，由于 SSAPValue 的 constantVal 为 int 类型，所以对浮点数的操作会保守地返回 Bottom。

未处理的指令: 默认情况下，任何未明确处理的指令都被保守地假定为产生 Bottom 值。

浮点数处理的注意事项：

在提供的代码中，SSAPValue 的 constantVal 是 int 类型。这使得浮点数常量传播变得复杂。对于浮点数相关的指令（kFAdd, kFMul, kFCmp, kFNeg, kFNot, kItoF, kFtoI 等），如果不能将浮点值准确地存储在 int 中，或者不能可靠地执行浮点运算，那么通常会保守地将结果设置为 Bottom。一个更完善的 SCCP 实现会使用 std::variant<int, float> 或独立的浮点常量存储来处理浮点数。

## LoopSR循环归纳变量强度削弱 关于魔数计算的说明

魔数除法的核心思想是：将除法转换为乘法和移位

数学原理：x / d ≈ (x * m) >> (32 + s)

m 是魔数 (magic number)
s 是额外的移位量 (shift)
>> 是算术右移

2^(32+s) / d ≤ m < 2^(32+s) / d + 2^s / d

cd /home/downright/Compiler_Opt/mysysy && python3 -c "
# 真正的迭代原因：精度要求
def explain_precision_requirement():
    d = 10
    
    print('魔数算法需要找到精确的边界值：')
    print('目标：2^p > d * (2^31 - r)，其中r是余数')
    print()
    
    # 模拟我们算法的迭代过程
    p = 31
    two_p = 2**p
    r = two_p % d  # 余数
    m = two_p // d  # 商
    
    print(f'初始状态 (p={p}):')
    print(f'  2^{p} = {two_p:,}')
    print(f'  2^{p} / {d} = {m:,} 余 {r}')
    print(f'  需要检查: 2^{p} > {d} * (2^31 - {r}) = {d * (2**31 - r):,}')
    print(f'  条件满足: {two_p > d * (2**31 - r)}')
    print()
    
    # 继续迭代直到条件满足
    iteration = 0
    while r != 0 and m < 2**31:
        iteration += 1
        p += 1
        two_p *= 2
        m *= 2
        r *= 2
        if r >= d:
            m += 1
            r -= d
            
        print(f'迭代 {iteration} (p={p}):')
        print(f'  m = {m:,}, r = {r}')
        print(f'  m < 2^31? {m < 2**31} (2^31 = {2**31:,})')
        
        if m >= 2**31:
            print(f'  ✗ 魔数 {m:,} 超出32位范围!')
            break
        elif r == 0:
            print(f'  ✓ 余数为0，找到精确解')
            break
        
        if iteration > 10:  # 防止无限循环
            print('  ... (限制迭代次数)')
            break

explain_precision_requirement()
"

问题在于精度要求。理论魔数需要非常精确，不能简单地用 2^32 / d。让我展示真正的迭代原因：

cd /home/downright/Compiler_Opt/mysysy && python3 -c "
# 真正的迭代原因：精度要求
def explain_precision_requirement():
    d = 10
    
    print('魔数算法需要找到精确的边界值：')
    print('目标：2^p > d * (2^31 - r)，其中r是余数')
    print()
    
    # 模拟我们算法的迭代过程
    p = 31
    two_p = 2**p
    r = two_p % d  # 余数
    m = two_p // d  # 商
    
    print(f'初始状态 (p={p}):')
    print(f'  2^{p} = {two_p:,}')
    print(f'  2^{p} / {d} = {m:,} 余 {r}')
    print(f'  需要检查: 2^{p} > {d} * (2^31 - {r}) = {d * (2**31 - r):,}')
    print(f'  条件满足: {two_p > d * (2**31 - r)}')
    print()
    
    # 继续迭代直到条件满足
    iteration = 0
    while r != 0 and m < 2**31:
        iteration += 1
        p += 1
        two_p *= 2
        m *= 2
        r *= 2
        if r >= d:
            m += 1
            r -= d
            
        print(f'迭代 {iteration} (p={p}):')
        print(f'  m = {m:,}, r = {r}')
        print(f'  m < 2^31? {m < 2**31} (2^31 = {2**31:,})')
        
        if m >= 2**31:
            print(f'  ✗ 魔数 {m:,} 超出32位范围!')
            break
        elif r == 0:
            print(f'  ✓ 余数为0，找到精确解')
            break
        
        if iteration > 10:  # 防止无限循环
            print('  ... (限制迭代次数)')
            break

explain_precision_requirement()
"


cd /home/downright/Compiler_Opt/mysysy && python3 -c "
# 检查初始状态是否会溢出
def check_initial_overflow():
    int32_max = 2**31 - 1
    
    print('检查不同除数的初始魔数是否会溢出:')
    print(f'32位有符号整数最大值: {int32_max:,}')
    print()
    
    # 检查各种除数
    divisors = [2, 3, 5, 7, 9, 10, 11, 13, 17, 25, 100, 1000]
    
    for d in divisors:
        # 初始魔数 = 2^31 / d
        initial_magic = (2**31) // d
        
        overflow = initial_magic > int32_max
        status = \"溢出\" if overflow else \"安全\"
        
        print(f'd={d:4d}: 初始魔数 = 2^31/{d} = {initial_magic:10,} [{status}]')
        
    print()
    print('结论: 初始状态下魔数不会溢出，溢出发生在迭代过程中')

check_initial_overflow()
"

总结
迭代的必要性：

不是为了避免初始溢出（初始状态安全）
是为了找到最精确的魔数，减少舍入误差
每次迭代提高一倍精度，但魔数也翻倍
溢出发生时机：

初始状态：2^31 / d 总是在32位范围内
迭代过程：2^32 / d, 2^33 / d, ... 逐渐超出32位范围
回退值的正确性：

回退值是基于数学理论和实践验证的标准值
来自LLVM、GCC等成熟编译器的实现
通过测试验证，对各种输入都能产生正确结果
算法设计哲学：

先尝试最优解：通过迭代寻找最精确的魔数
检测边界条件：当超出32位范围时及时发现
智能回退：使用已验证的标准值保证正确性
保持通用性：对于没有预设值的除数仍然可以工作

## 死归纳变量消除

整体架构和工作流程
当前的归纳变量消除优化分为三个清晰的阶段：

识别阶段：找出所有潜在的死归纳变量
安全性分析阶段：验证每个变量消除的安全性
消除执行阶段：实际删除安全的死归纳变量


逃逸点检测 (已修复的关键安全机制)
数组索引检测：GEP指令被正确识别为逃逸点
循环退出条件：用于比较和条件分支的归纳变量不会被消除
控制流指令：condBr、br、return等被特殊处理为逃逸点
内存操作：store/load指令经过别名分析检查

# 后续优化可能涉及的改动

## 1）将所有的alloca集中到entryblock中（已实现）

好处：优化友好性，方便mem2reg提升
目前没有实现这个机制，如果想要实现首先解决同一函数不同域的同名变量命名区分
需要保证符号表能正确维护域中的局部变量


# 关于中端优化提升编译器性能的TODO

## usedelete_withinstdelte方法

这个方法删除了use关系并移除了指令，逻辑是根据Instruction* inst去find对应的迭代器并erase
有些情况下外部持有迭代器和inst,可以省略find过程