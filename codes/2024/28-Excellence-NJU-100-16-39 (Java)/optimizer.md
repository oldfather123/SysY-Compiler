# 构建后的优化

## 中端

• 内存提升至寄存器(Mem2Reg)
将内存变量转换为寄存器变量的技术，从而减少内存访问次数，提高程序性能，引入phi指令
//• 死代码消除（Dead Code Elimination）
• 公共子表达式消除（CommonSubEli）
• 常量传播和折叠（ConstantPro）
• 函数内联（FuncInline）
函数调用的开销较大，通过在调用处直接插入被调用函数的代码，
消除函数调用的开销，但这会增加代码规模。
//• 指令重排（Reassociation）
• 循环优化（LoopOpt）
//• 控制流简化（CFG Simplification）
去除不必要的跳转和合并基本块，使代码结构更简洁

## 后端
• 死代码消除（DeadCodeEli）
• 循环优化（LoopOpt）
• LoadStore的优化（LSOpt）
//• 强度削减（Strength Reduction）
//• 地址生成（Address Generation）
//• 窥孔优化（Peephole）

## 实现
IR中间指令和RISCV后端指令各有一个optimizer包，用OptimizerFramework来创建优化
RISCVBuilder在全部生成完指令后，对每个函数块优化
`for (RISCVFunction function : riscvCode.getFunctions()) {
optimizerFramework.optimize(function);
}`
IRVisitor在visitProgram函数中生成IR后进行优化;