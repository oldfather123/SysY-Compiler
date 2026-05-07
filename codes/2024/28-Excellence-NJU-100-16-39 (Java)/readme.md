# 一、词法分析部分
1. 在antlr包中，SysyLexer.g4
2. 增加了十进制浮点数常量 |十六进制浮点数常量部分，为FLOAT_CONST
# 二、语法、语义分析部分
1. 在antlr包中，SysyParser.g4
2. 暂时不用进行语法报错，增加了funcType的返回值可能，从原有的int和void增加了float的返回值，以及基本类型bType也添加了FLOAT，从而改变了常量（varDecl），变量声明（constDecl），以及函数的形参类型（funcFParam）。
# 三、中间代码生成部分（重要）
* 参考实现：https://gitlab.eduxiji.net/educg-group-17291-1894922/202310284201911-1365.git
   ## 总任务：为前端和后端提供可以操作IR的api调用
   ——对应于IR包
   ### 3.1 IR指令的选择和构建（对应具体的每一条LLVM指令）
   ——对应IR包中的IRInstrunction包
1. 首先确定IR需要有哪些指令，每个类型指令的构建对应一个类，以及如何进行指令的构建和对应。这个类会作为存储在IR module中最终的内容类，并在后端进行指令选择的时候用到。

3. 主类是一个抽象类IRInstrcution（抽象类与接口的区别在于抽象类可以包含具体的父类共同实现方法，但是接口不能包含具体的实现方法，只能提供空的方法接口）

4. 子类包含了所有的IR指令，包括如下（可以继续补充）共12个

1. Allocate

3. Load

4. Store

5. Calculate （对于一元运算符和二元运算符的IR)

6. Br （条件/无条件跳转）

7. Cond （对应Icmp，条件跳转）

8. Call （函数调用）

9. Return （返回）

10. TypeTransfer（int和float的类型转换）

11. GetElementptr （获取数组元素指针）

12. Zext （零扩展的zext .. to指令和符号扩展的sext .. to指令。）

13. Phi（合并多条路径变量的值）
    ### 3.2 IRValueRef的构建（对应LLVMValueRef）
    ——对应于IR包中的IRValueRef子包
1. 在借助LLVM进行理解，需要自行构建MyValueRef的类，用以代替LLVMValueRef。
2. 包括一个总的类IRValueRef，其余的类共6种，BaseBlockRef，FunctionBlockRef，ConstFloatRef，ConstIntRef，VirtualRegRef，GlobalRegRef，ArrayRef

### 3.3 相关Module以及Builder的构建（对应LLVMModuleRef和LLVMBuilderRef）
——对应于IR包中的IRModule类和IRBuilder类
1. 包含了MyIRModule对应LLVMModuleRef，用于存储所有的IR
2. 包含了MyIRBuilder对应LLVMBuilderRef，用于帮助构建，并把所有IR添加到module中的builder辅助类。

### 3.4 LLVMType的构建（对应LLVMTypeRef）
——对应于IR包中的IRType包
1. IRType作为主接口，所有的具体类都是IRType的实现
2. 需要如下类型：共七种
1. 数组类型
2. 浮点数类型
3. 函数类型
4. INT1类型
5. INT32类型
6. 指针类型
7. VOID类型


# 四、后端指令选择和代码生成
## 总任务：根据中间代码生成RISC-V汇编代码

### 4.1 后端代码基本结构构建
——对应于backend包中的RISCVCode包
1. 结构概述：
   后端代码结构层次清晰，从上到下依次为 RISCVCode、RISCVFunction、RISCVBlock、RISCVInstruction。
   RISCVCode 代表整个代码生成过程的顶层容器，持有所有的函数和全局变量。
   RISCVFunction 代表一个函数，每个函数持有一个或多个 RISCVBlock 实例，表示函数体内的基本块。
   RISCVBlock 代表一个基本块，持有多条 RISCVInstruction，这些指令会在最终的汇编代码中顺序排列。
   每一份源代码在 RISCVCode 中只有一个实例，这个实例负责管理整个后端代码生成过程。
2. 实例关系：
- 一个 RISCVFunction 持有多个 RISCVBlock。
- 一个 RISCVBlock 持有多个 RISCVInstruction。
- 最终所有这些结构一起组成了完整的 RISC-V 汇编代码。
  ### 4.2 后端指令选择
  ——对应于backend包中的RISCVInstruction包
1. 指令确定：
- 根据中间代码 (IR) 的需求，确定最终生成 RISC-V 汇编代码所需的指令。
- 每一类 RISC-V 指令对应一个类，这些类的实例代表一条或几条相关联的 RISC-V 指令。
- 这些指令类的实例最终会被存储在每个 RISCVBlock 实例中，用于生成最终的汇编代码。
2. 主类与子类：
- RISCVInstruction 是一个抽象类，定义了所有 RISC-V 指令类的基本接口和通用行为。
- 包中其他具体指令类继承自 RISCVInstruction，实现了不同类型的 RISC-V 指令。
- 指令类包含：
    - 比较指令：Feq、Seqz、Slt、Sltu、Snez
    - 逻辑与算术指令：Andi、Xor、Binary（32 位整数与浮点数的双目运算，如 addw、subw、fadd.s、fsub.s）
    - 跳转与分支指令：Beqz、J
    - 数据传输指令：La、Ld、Li、Lui、Lw、Flw、Sw、Fsw、Sd
    - 函数调用与返回指令：Call、Ret
    - 寄存器与类型转换指令：Mv、Fcv、Sext
      ### 4.3 后端指令操作数构建
      ——对应于backend包中RISCVOperand类及其子类和OperandType枚举类
1. 主类 RISCVOperand：
- RISCVOperand 类代表 RISC-V 指令中的操作数，每个实例表示一条指令的一个操作数。
- 操作数可能是立即数、寄存器、栈空间、标签或全局变量。
2. 操作数类型 OperandType 枚举类：
- OperandType 枚举了所有可能的操作数类型，包括：
    - imm: 立即数
    - reg: 寄存器
    - stackRoom: 栈空间
    - label: 标签
    - globalVar: 全局变量
      子类 RISCVLabel：
- 为方便标签操作，RISCVLabel 继承自 RISCVOperand，专门表示代码中的标签。
  ### 4.4 从中间代码到后端代码
1. 遍历中间代码：
- RISCVBuilder 负责从 IR 模块中遍历每个全局变量、函数、基本块和 IR 指令，并生成相应的 RISC-V 后端代码。
- 这个过程中，每个 IR 指令都会被转换成对应的 RISC-V 指令，并存储在 RISCVBlock 中。
2. 寄存器分配：
- 在函数内，RISCVBuilder 需要调用 RegisterAllocator 接口，为每个函数进行寄存器分配。
- 确保每个变量在 RISC-V 汇编代码中能够被正确引用，并分配到适当的寄存器或内存地址

### 4.5 后端代码输出
——存在于backend包中的RISCVCode类
遍历输出结构：
- 最终，RISCVCode 类遍历其内部所有结构，包括全局变量、函数、基本块和指令。
- 生成每条指令的字符串表示，并将其写入到 .s 汇编文件中。
  # 五、寄存器分配
  ## 总任务：管理和分配寄存器或栈地址用于变量存储
  ### 5.1 抽象接口RegisterAllocator类
1.  setFunction(IRFunctionBlockRef function)：设置当前所在的函数上下文。用于在特定函数内进行寄存器分配。

2. setModule(IRModule module)：设置当前所在的模块。这可能用于判断变量是否是全局变量，从而影响它们的存储位置。

3. allocate()：执行寄存器分配过程，并返回所需的栈帧大小。这决定了函数调用时需要多少栈空间。

4. getRegister(String variable)：返回给定变量对应的寄存器或栈地址。用于获取变量在当前上下文中的存储位置。
   ### 5.2 具体实现MemoryRegisterAlloc类
   ——在代码生成前对中间表示（IR）进行扫描，决定变量存放的位置（寄存器或栈）主要方法allocate：通过调用了一系列方法来完成变量分配的过程。
1.  initListAndMap()：初始化列表和映射，用于准备变量和数据结构以进行后续的操作。

2. setWorklist()：设置工作列表，通过遍历函数的基本块来初始化工作列表，对每个基本块进行去重和初始化操作，包括添加到相应列表中并设置其内部指令的相关信息。随后对工作列表和基本块列表进行反转操作，最后计算每个基本块的变量使用和定义情况，为后续的分析和处理做准备。

3. WorkListAlgorithm()：通过迭代计算每个基本块的输入（in）和输出（out），根据数据流分析的原理，更新基本块的信息，并在输出信息发生变化时将相关后继基本块加入工作列表，直至不再有变化为止。

4. calculateInstructions()：对每个基本块内的指令进行计算，处理每条指令的输入（in）和输出（out）信息，根据数据流分析的原理，更新指令的信息，确保正确处理指令间的数据流关系。通过逐条遍历指令并考虑其前后关系，更新每条指令的输入输出信息，以便后续的数据流分析和优化过程能够准确地操作。

5. calculateIntervals()：用于计算每个变量的活跃范围（live range）。通过迭代处理指令列表，分析每个指令的输入和输出信息，更新每个变量的活跃范围起始点和结束点。首先，对每个指令的输入和输出进行处理，为每个变量更新起始点和结束点。然后，检查所有变量，移除未出现的变量并更新其位置信息。

6. setArrayAndFloatVars()：方法用于设置数组和浮点型变量。通过遍历指令列表，获取指令的操作数，并筛选出变量类型的操作数。对于每个变量操作数，根据其类型进行分类处理：若为数组类型，则将其添加到数组变量列表中，并计算数组长度；若为浮点类型，则将其添加到浮点变量列表中，并从通用变量列表中移除。最后，对数组和浮点变量列表进行去重操作，并初始化可用寄存器数组。

7. linearScanAllocate()：方法实现线性扫描分配。首先，按照起始点递增的顺序对变量列表进行排序。然后，对每个变量进行处理：若为数组变量，则将其长度加一后存储到栈上；否则，根据变量的起始点处理活跃变量列表，检查活跃变量数是否超过寄存器数量。若超过，则进行溢出处理；否则，从可用寄存器池中选择一个寄存器分配给该变量，并更新变量的位置信息。最后，根据变量的结束点将变量添加到活跃变量列表中。

8. floatAllocate()：方法用于浮点型变量的分配。首先，按照浮点型变量起始点递增的顺序对变量列表进行排序。然后，对每个浮点型变量进行处理：根据变量的起始点处理活跃变量列表，检查活跃浮点变量数是否超过浮点寄存器数量。若超过，则进行溢出处理；否则，从可用浮点寄存器池中选择一个寄存器分配给该浮点型变量，并更新变量的位置信息。最后，根据变量的结束点将浮点型变量添加到活跃变量列表中。

9. functionCallPreserve()：方法用于在函数调用时保存现场。遍历指令列表，对每个函数调用指令进行处理：获取当前活跃的变量列表，为每个活跃变量分配新的栈空间并更新其位置信息。最后，在栈上保存返回地址（return address）。这个过程有助于在函数调用时正确保存现场信息，以便函数调用结束后能够正确恢复现场。
   # 六、优化
   ## 一、中端
   ### 1.内存提升至寄存器(Mem2Reg)
   将内存变量转换为寄存器变量的技术，从而减少内存访问次数，提高程序性能，引入phi指令

   首先做支配分析（DomAnalysis），采用数据流迭代求出支配关系，计算直接支配者与支配边界，参考文献如下：
   https://blog.csdn.net/qq_48201696/article/details/129979831

   再由支配边界进行mem2Reg，主要分为四个步骤：收集所有需要优化的分配指令，插入phi节点，更新所有使用和定义，移除无用的phi，参照对应伪代码实现。
   出处：（https://buaa-se-compiling.github.io/miniSysY-tutorial/challenge/mem2reg/help.html）
   收集所有需要优化的分配指令仅处理用于store和load的allocate指令。


   移除无用的phi指令旨在消除未使用的phi指令，来去掉冗余代码。
   ### 2.公共子表达式消除（CommonSubEli）
消除重复计算的子表达式，减少冗余计算。
   ### 3.常量折叠（ConstantPro）
将表达式中的常量进行计算并替换，简化表达式，提高执行效率。
   ### 4.尾递归消除（TailRecursionEli）
优化尾递归函数，将尾递归转换为循环结构，减少栈空间的使用。
   ### 5.分支跳转简化（BrCmpOpt）
简化分支结构，消除不必要的条件分支或简化条件表达式，提高程序执行效率。
   作用如下：
- 对于所有if语句跳转，原语句：
- %1 = icmp a1, a2
- %2 = zext i1 %1 to i32
- %3 = icmp ne %2, 0
- br %3, %true_block, %false_block
- 简化后的语句：
-  %1 = icmp a1, a2
-  br %1, %true_block, %false_block
   ### 6.全局变量局部化（仅针对main）（GlobalArrToLocal）

   ## 二、后端
   ### 1.不可达块消除（RemoveUnreachableBlock）
   //见代码注释，这个优化我感觉可能有些冗余，建议试一试去掉，看看性能分会不会变。
    ### 2. LoadStoreMv的优化（LSOpt）
   //见代码注释，这个优化中loadstore冗余可以去掉试试，做完mem2reg后loadstore除了数组和全局变量外都去掉了，loadstore可能基本无冗余。
   ## 三、实现

   1.面向接口编程：
   所有优化类继承于优化接口
   public interface OptForRISCV {
   void Optimize(RISCVFunction riscvFunction);
   }
   public interface OptForIR {
   void Optimize(IRFunctionBlockRef irFunctionBlockRef);
   }

   2.抽象工厂：
   IR中间指令和RISCV后端指令各有一个optimizer包，并由抽象工厂OptimizerFactory来创建类。
   RISCVBuilder在全部生成完指令后，对每个函数块优化。
   IRVisitor在visitProgram函数中生成IR后进行优化。