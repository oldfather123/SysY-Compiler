# Compiler2023-Compiling

## 项目结构说明

```c++
├── build                                                                   存放cmake编译生成文件
├── CMakeLists.txt                                                          cmake编译规则文件
├── doc                                                                     收集的编译器优化以及后端生成的论文资料
│   ├── comp512                                                             莱斯大学高级编译课程
│   │   └── readme.md
│   ├── llvm                                                                llvm的参考资料
│   │   └── Building an LLVM backend.pdf
│   ├── official                                                            官方资料
│   │   ├── 2023编译系统赛-章程.pdf
│   │   ├── 2023编译系统设计赛道技术方案.pdf
│   │   ├── 2023编译系统挑战赛-代码规模优化赛题-技术方案.pdf
│   │   ├── 2023系统能力培养大赛提交指南.pdf
│   │   ├── SysY2022语言定义-V1.pdf
│   │   ├── SysY2022运行时库-V1.pdf
│   │   └── VisionFive2.pdf
│   ├── papers  for                                                         论文
│   │   ├── backend
│   │   │   ├── Reducing the Impact of Spill Code.pdf
│   │   │   ├── Register Allocation for Programs in SSA-Form.pdf
│   │   │   └── Rematerialization.pdf
│   │   └── optimize
│   │       ├── dom.pdf
│   │       ├── gvn.pdf
│   │       └── OSR.pdf
│   ├── riscv                                                               riscv文档
│   │   ├── RISC-V-Reader-Chinese-v2p1.pdf
│   │   └── RISCV-spec-v2.1中文版.pdf
│   └── SSA-based Compiler Design.pdf                                       基于SSA构造编译器的论文资料                                                           
├── include                                                                 项目头文件
│   ├── ast
│   │   └── sysY_ast.hh
│   ├── backend
│   │   ├── sysY_asbuilder.hh
│   │   ├── sysY_ir2as.hh
│   │   └── sysY_regalloc.hh
│   ├── frontend
│   │   ├── location.hh
│   │   ├── position.hh
│   │   ├── stack.hh
│   │   ├── sysY_driver.hh
│   │   ├── sysY_flexlexer.hh
│   │   └── sysY_parser.hh
│   ├── irbuilder
│   │   └── sysY_irbuilder.hh
│   ├── mir
│   │   ├── BasicBlock.hh
│   │   ├── Constant.hh
│   │   ├── Function.hh
│   │   ├── GlobalVariable.hh
│   │   ├── Instruction.hh
│   │   ├── IRBuilder.hh
│   │   ├── IRprinter.hh
│   │   ├── Module.hh
│   │   ├── Type.hh
│   │   ├── User.hh
│   │   └── Value.hh
│   ├── optimize
│   │   ├── ActiveVar.hh
│   │   ├── AlgeSimplify.hh
│   │   ├── CFGAnalyse.hh
│   │   ├── Check.hh
│   │   ├── ConstProp.hh
│   │   ├── DeadCodeEli.hh
│   │   ├── DominateTree.hh
│   │   ├── FuncInfo.hh
│   │   ├── GVN.hh
│   │   ├── LIR.hh
│   │   ├── LoopInvariant.hh
│   │   ├── Mem2Reg.hh
│   │   ├── Pass.hh
│   │   ├── RDominateTree.hh
│   │   └── StrengthReduction.hh
│   └── utils
│       ├── logging.hh
│       └── utils.hh
├── lexgram                                                                 包含词法分析和语法分析语法规则文件
│   ├── Makefile
│   ├── sysY_ast.cc
│   ├── sysY_ast.hh
│   ├── sysY_driver.cc
│   ├── sysY_driver.hh
│   ├── sysY_flexlexer.hh
│   ├── sysY_lexgram
│   ├── sysY_lexgram.cc
│   ├── logging.cc
│   ├── logging.hh
│   ├── sysY_parser.yy
│   ├── sysY_scanner.ll
│   ├── test.sy
│   └── test.txt
├── lib                                                                     sysY的输入输出依赖的io库
│   ├── libsysy.a
│   ├── sylib.c
│   └── sylib.h
├── Makefile                                                                makefile脚本
├── optlogs                                                                 记录优化的log文件
├── README.md
├── scripts                                                                 脚本文件
│   ├── cfg_visualize.py
│   ├── gen_rv64asm_by_llc.py
│   ├── ir_test.py
│   ├── opt_compare.py
│   ├── scan_asm_insts.py
│   └── scan_ir_insts.py
├── src                                                                     项目源码
│   ├── ast                                                                 语法分析树相关
│   │   ├── CMakeLists.txt
│   │   └── sysY_ast.cc
│   ├── backend                                                             编译器后端(寄存器分配+汇编代码生成)            
│   │   ├── CMakeLists.txt
│   │   ├── sysY_asbuilder.cc
│   │   ├── sysY_ir2as.cc
│   │   └── sysY_regalloc.cc
│   ├── frontend                                                            编译器前端(词法分析+语法分析)                                        
│   │   ├── CMakeLists.txt
│   │   ├── sysY_driver.cc
│   │   ├── sysY_parser.cc
│   │   └── sysY_scanner.cc
│   ├── irbuilder                                                           中间代码生成
│   │   ├── CMakeLists.txt
│   │   └── sysY_irbuilder.cc
│   ├── main.cc                                                             主程序
│   ├── mir                                                                 IR库
│   │   ├── BasicBlock.cc
│   │   ├── CMakeLists.txt
│   │   ├── Constant.cc
│   │   ├── Function.cc
│   │   ├── GlobalVariable.cc
│   │   ├── Instruction.cc
│   │   ├── IRprinter.cc
│   │   ├── Module.cc
│   │   ├── Type.cc
│   │   ├── User.cc
│   │   └── Value.cc
│   ├── optimize                                                             优化部分
│   │   ├── ActiveVar.cc
│   │   ├── AlgeSimplify.cc
│   │   ├── CFGAnalyse.cc
│   │   ├── Check.cc
│   │   ├── CMakeLists.txt
│   │   ├── ConstProp.cc
│   │   ├── DeadCodeEli.cc
│   │   ├── DominateTree.cc
│   │   ├── FuncInfo.cc
│   │   ├── GVN.cc
│   │   ├── LIR.cc
│   │   ├── LoopInvariant.cc
│   │   ├── Mem2Reg.cc
│   │   ├── Pass.cc
│   │   ├── RDominateTree.cc
│   │   └── StrengthReduction.cc
│   └── utils                                                                日志记录
│       ├── CMakeLists.txt
│       ├── logging.cc
│       └── utils.cc
└── tests                                                                    存放测试用例
    ├── final_performance
    ├── functional
    ├── hidden_functional
    └── performance
```
