1. **系统架构设计**

* 结构图

  * ```
    └── src
        ├── Compiler.java
        ├── META-INF
        │   └── MANIFEST.MF
        ├── Utils
        │   └── CustomList.java
        ├── arg
        │   └── Arg.java
        ├── backend
        │   ├── BackendPrinter.java
        │   ├── BlockSort.java
        │   ├── DeleteUnusedBlock.java
        │   ├── IrParser.java
        │   ├── PeepHole.java
        │   ├── RegAlloc.java
        │   ├── RegAllocAno.java
        │   ├── RegAllocLinear.java
        │   ├── asmInstr
        │   │   ├── AsmInstr.java
        │   │   ├── asmBinary
        │   │   │   ├── AsmAdd.java
        │   │   │   ├── AsmAnd.java
        │   │   │   ├── AsmBinary.java
        │   │   │   ├── AsmDiv.java
        │   │   │   ├── AsmFAdd.java
        │   │   │   ├── AsmFDiv.java
        │   │   │   ├── AsmFMul.java
        │   │   │   ├── AsmFSub.java
        │   │   │   ├── AsmFeq.java
        │   │   │   ├── AsmFge.java
        │   │   │   ├── AsmFgt.java
        │   │   │   ├── AsmFle.java
        │   │   │   ├── AsmFlt.java
        │   │   │   ├── AsmFneg.java
        │   │   │   ├── AsmMod.java
        │   │   │   ├── AsmMul.java
        │   │   │   ├── AsmOr.java
        │   │   │   ├── AsmSll.java
        │   │   │   ├── AsmSlt.java
        │   │   │   ├── AsmSlti.java
        │   │   │   ├── AsmSltu.java
        │   │   │   ├── AsmSra.java
        │   │   │   ├── AsmSrl.java
        │   │   │   ├── AsmSub.java
        │   │   │   └── AsmXor.java
        │   │   ├── asmBr
        │   │   │   ├── AsmBnez.java
        │   │   │   └── AsmJ.java
        │   │   ├── asmConv
        │   │   │   ├── AsmFtoi.java
        │   │   │   ├── AsmZext.java
        │   │   │   └── AsmitoF.java
        │   │   ├── asmLS
        │   │   │   ├── AsmFld.java
        │   │   │   ├── AsmFlw.java
        │   │   │   ├── AsmFsd.java
        │   │   │   ├── AsmFsw.java
        │   │   │   ├── AsmL.java
        │   │   │   ├── AsmLLa.java
        │   │   │   ├── AsmLa.java
        │   │   │   ├── AsmLd.java
        │   │   │   ├── AsmLw.java
        │   │   │   ├── AsmMove.java
        │   │   │   ├── AsmS.java
        │   │   │   ├── AsmSd.java
        │   │   │   ├── AsmSw.java
        │   │   │   └── LSType.java
        │   │   └── asmTermin
        │   │       ├── AsmCall.java
        │   │       └── AsmRet.java
        │   ├── itemStructure
        │   │   ├── AsmBlock.java
        │   │   ├── AsmFunction.java
        │   │   ├── AsmGlobalVar.java
        │   │   ├── AsmImm12.java
        │   │   ├── AsmImm32.java
        │   │   ├── AsmLabel.java
        │   │   ├── AsmModule.java
        │   │   ├── AsmOperand.java
        │   │   ├── AsmType.java
        │   │   ├── Group.java
        │   │   └── LibFunctionGeter.java
        │   └── regs
        │       ├── AsmFPhyReg.java
        │       ├── AsmFVirReg.java
        │       ├── AsmPhyReg.java
        │       ├── AsmReg.java
        │       ├── AsmVirReg.java
        │       └── RegGeter.java
        ├── debug
        │   └── DEBUG.java
        ├── frontend
        │   ├── ir
        │   │   ├── DataType.java
        │   │   ├── FuncDef.java
        │   │   ├── Use.java
        │   │   ├── Value.java
        │   │   ├── constvalue
        │   │   │   ├── ConstBool.java
        │   │   │   ├── ConstFloat.java
        │   │   │   ├── ConstInt.java
        │   │   │   └── ConstValue.java
        │   │   ├── instr
        │   │   │   ├── Instruction.java
        │   │   │   ├── binop
        │   │   │   │   ├── AShrInstr.java
        │   │   │   │   ├── AddInstr.java
        │   │   │   │   ├── BinaryOperation.java
        │   │   │   │   ├── FAddInstr.java
        │   │   │   │   ├── FDivInstr.java
        │   │   │   │   ├── FMulInstr.java
        │   │   │   │   ├── FRemInstr.java
        │   │   │   │   ├── FSubInstr.java
        │   │   │   │   ├── MulInstr.java
        │   │   │   │   ├── SDivInstr.java
        │   │   │   │   ├── SRemInstr.java
        │   │   │   │   ├── ShlInstr.java
        │   │   │   │   ├── SubInstr.java
        │   │   │   │   └── Swappable.java
        │   │   │   ├── convop
        │   │   │   │   ├── Bitcast.java
        │   │   │   │   ├── ConversionOperation.java
        │   │   │   │   ├── Fp2Si.java
        │   │   │   │   ├── Si2Fp.java
        │   │   │   │   └── Zext.java
        │   │   │   ├── memop
        │   │   │   │   ├── AllocaInstr.java
        │   │   │   │   ├── GEPInstr.java
        │   │   │   │   ├── LoadInstr.java
        │   │   │   │   ├── MemoryOperation.java
        │   │   │   │   └── StoreInstr.java
        │   │   │   ├── otherop
        │   │   │   │   ├── CallInstr.java
        │   │   │   │   ├── EmptyInstr.java
        │   │   │   │   ├── MoveInstr.java
        │   │   │   │   ├── PCInstr.java
        │   │   │   │   ├── PhiInstr.java
        │   │   │   │   └── cmp
        │   │   │   │       ├── Cmp.java
        │   │   │   │       ├── CmpCond.java
        │   │   │   │       ├── FCmpInstr.java
        │   │   │   │       └── ICmpInstr.java
        │   │   │   ├── terminator
        │   │   │   │   ├── BranchInstr.java
        │   │   │   │   ├── JumpInstr.java
        │   │   │   │   ├── ReturnInstr.java
        │   │   │   │   └── Terminator.java
        │   │   │   └── unaryop
        │   │   │       └── FNegInstr.java
        │   │   ├── lib
        │   │   │   ├── FuncGetarray.java
        │   │   │   ├── FuncGetch.java
        │   │   │   ├── FuncGetfarray.java
        │   │   │   ├── FuncGetfloat.java
        │   │   │   ├── FuncGetint.java
        │   │   │   ├── FuncMemset.java
        │   │   │   ├── FuncPutarray.java
        │   │   │   ├── FuncPutch.java
        │   │   │   ├── FuncPutfarray.java
        │   │   │   ├── FuncPutfloat.java
        │   │   │   ├── FuncPutint.java
        │   │   │   ├── FuncStarttime.java
        │   │   │   ├── FuncStoptime.java
        │   │   │   ├── Lib.java
        │   │   │   └── LibFunc.java
        │   │   ├── structure
        │   │   │   ├── BasicBlock.java
        │   │   │   ├── FParam.java
        │   │   │   ├── Function.java
        │   │   │   ├── GlobalObject.java
        │   │   │   ├── Procedure.java
        │   │   │   └── Program.java
        │   │   └── symbols
        │   │       ├── ArrayInitVal.java
        │   │       ├── InitExpr.java
        │   │       ├── SymTab.java
        │   │       └── Symbol.java
        │   ├── lexer
        │   │   ├── Lexer.java
        │   │   ├── Token.java
        │   │   ├── TokenList.java
        │   │   └── TokenType.java
        │   └── syntax
        │       ├── Ast.java
        │       └── Parser.java
        └── midend
            ├── FMHBR.java
            ├── FuncMemorize.java
            ├── RemovePhi.java
            ├── SSA
            │   ├── ArrayFParamMem2Reg.java
            │   ├── ConstBroadcast.java
            │   ├── DFG.java
            │   ├── DeadBlockRemove.java
            │   ├── DeadCodeRemove.java
            │   ├── DetectTailRecursive.java
            │   ├── FI.java
            │   ├── GCM.java
            │   ├── GVN.java
            │   ├── GlobalValueSimplify.java
            │   ├── Mem2Reg.java
            │   ├── MergeBlock.java
            │   ├── MergeGEP.java
            │   ├── OIS.java
            │   ├── OISR.java
            │   ├── PtrMem2Reg.java
            │   ├── RemoveUseLessPhi.java
            │   └── SimplifyBranch.java
            └── loop
                ├── AnalysisLoop.java
                ├── BlockType.java
                ├── LCSSA.java
                ├── Loop.java
                ├── LoopInvariantMotion.java
                ├── LoopRotate.java
                ├── LoopSimplify.java
                ├── LoopUnroll.java
                └── RemoveUseLessLoop.java
    
    ```

2. **关键技术与方法**

- 前端

  - 词法分析
    - 目的是将字符串形式的代码转化为存储代码信息的有意义的数据结构。
    - 初步解析传入的源文件，将文本分解为词法单元（Token）列表形式。
  - 语法分析
    - 根据 SysY 语法对词法单元列表进行解析，将其划分为语法单元，并通过递归下降的方法建立抽象语法树。
    - 在这一步我们对标准 SysY 语法进行了一些改写，不影响原意的情况下消除了左递归、将一些类似的单元抽象出共同父类，以便后续解析。
  - 语义分析
    - 遍历上一步建立的语法树，建立符号表、生成中间代码。
    - 中间代码的框架结构大体为：Program -> Function -> Procedure -> BasicBlock -> Instruction，左侧为最高层级，层层包含。

- 中端

  - Function inlining 函数内联
    - 将非递归函数内敛到调用者中，避免函数调用带来的跳转和栈操作，同时为后续优化创造机会
    - 做完函数内联删掉不再有用的函数定义，减轻后端翻译负担
  - GlobalValueSimplify 全局对象简化
    - 全局对象简化，包括将没有被更新过的全局对象直接用初值替代，以及【局部化】
    - 局部化，即将所有仅在一个函数中被使用的全局对象变成对应函数中的局部对象，以便后续 mem2reg 操作
    - 全局对象简化之后也会删去没用的全局变量定义
  - DeadBlockRemove 死块删除
    - 移除无用的基本块，简化代码
  - DFG
    - 构建控制流图
    - 实现了对函数控制流图（CFG）进行支配树（Dominator Tree）和支配边界（Dominance Frontier）的计算
  - Mem2Reg 
    - 将内存操作转化为虚拟寄存器操作
    - 基于 alloca 指令进行，构造其 def-use 关系，简化 load 和 store 指令
    - 这部加入 phi 指令，实现非数组对象不基于内存操作的 SSA
  - ArrayFParamMem2Reg
    - 针对数组形参的 mem2reg
    - 传入的数组形参实际上是数组首元素的指针，理论上不会被修改，因此 load 的结果就应该是第一次 store 的内容
  - MergeGEP 
    - 将针对同一地址的多条 GEP 指令合并为一条
  - Operation instruction simplification 运算指令简化
    - 对计算指令进行简化，包括常数折叠、常数传播等
  - FuncMemorize （递归）函数记忆化
    - 通过添加全局数组减少递归运算
  - AnalysisLoop 循环分析
    - 对中间代码的控制流进行分析，找出其中的循环结构以便后续优化
  - LCSSA 循环闭环静态单赋值化操作
    - 便于此后的循环优化
  - LoopUnroll 循环展开
    - 将循环结构变成顺序结构以创造更多优化机会
  - LoopInvariantMotion 循环不变量外提
    - 将循环中与迭代变量无关且无副作用的计算等指令提出到循环外
  - RemoveUseLessPhi
    - 删除没有用的 phi 指令
  - DeadCodeRemove 死代码删除
    - 结果没有被用到的代码应该被删掉
  - Global Value Numbering 全局值编号
    - 同样的计算只进行一次，避免重复计算
  - SimplifyBranch 分支简化
    - 将只有一种可能的条件判断块简化掉
  - MergeBlock 基本块合并
    - 将多个跳转关系比较简单的基本块合成一个
  - PtrMem2Reg 
    - 针对指针的 mem2reg，可以认为是在做别名分析
    - 通过分析 load 和 store 对应的地址是否一定一致来减少内存操作
  - LoopSimplify 循环简化
    - 将循环累加变成乘法以减少循环操作
  - RemoveUseLessLoop
    - 删除仅剩下迭代对象的无意义循环
  - DetectTailRecursive
    - 识别尾递归
  - RemovePhi 
    - 将 phi 指令替换成 move 操作以方便后端翻译成汇编

  - *注：以上部分 PASS 可能在不同地方被运行多次*

- 后端

  - IrParser
    - IrParser的主要任务是将IR代码翻译为RISC-V代码。对于大多指令，在不考虑优化的情况下可对应进行翻译。较为特殊的有以下几个要点：
      - 栈空间的分配。我们的设计是每个函数维护自己的栈，从栈顶到栈底依次存储函数内Call指令8个以后的参数（默认使用a0-a7进行传参），执行函数过程中使用alloca指令申请的栈空间，寄存器溢出占用的栈空间，ra。由于寄存器溢出占用的栈空间在寄存器分配之前无法计算，因此在IrParser中会计算剩下三个部分的栈空间大小，最后在寄存器分配部分完善栈空间的申请。
      - 指令翻译。通过HashMap建立IR中虚拟寄存器与RISC-V中虚拟寄存器的映射关系。在翻译过程中尽量采用基本指令，从而避免拓展指令对寄存器值进行意外的修改，后续优化也会更加灵活。
      - 指令降级。对乘、除、模等指令，转化为CPU周期更小的移位、加减等指令。
  - RegAlloc, RegAllocAno, RegAllocLinear
    - 后端我们一共写了三个版本的寄存器分配，前两个是图着色，最后一个是线性扫描分配
      - 第一版图着色我们是参考 虎书 上的伪代码实现的，实现了数据流分析、化简、合并和溢出，我们先对浮点寄存器进行分配，然后对整数寄存器进行分配，同时我们也参照riscv的调用规约，做了调用者保存和被调用者保存，还做了进入函数后的栈分配
      - 第二版的图着色是在第一版的基础上做了更细致的分配，在每一轮分配时我们会把虚拟寄存器分成两类，一类是跨函数调用活跃的虚拟寄存器，另一类是它的补集，我们会有先对跨函数调用活跃的寄存器进行分配，只会为它分配S类寄存器，接着我们会对另一类进行分配，寄存器分配的顺序是T，A，S。
      - 特殊的寄存器：t0, 我们是专门用来做地址加载的，a0我们是专门用来做传参和加载返回值的
      - 合理地选择着色顺序：我们通过根据循环的深度为寄存器附上权重，再按照权重和冲突数组合选择从图中抽出的顺序
      - 合理地选择溢出：溢出的顺序也是像着色顺序一样