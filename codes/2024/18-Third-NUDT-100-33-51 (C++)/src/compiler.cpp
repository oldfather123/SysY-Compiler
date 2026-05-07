#include "SysYLexer.h"
#include "SysYParser.h"
#include "include/backend/DAG2End.h"
#include "include/backend/DAGBuilder.h"
#include "include/backend/DAGCoverage.h"
#include "include/backend/DAGPrinter.h"
#include "include/backend/InstructionSchedule.h"
#include "include/backend/MacroExpansion.h"
#include "include/backend/Mid2End.h"
#include "include/backend/Mid2EndDAG.h"
#include "include/backend/Peephole.h"
#include "include/backend/RegisterAlloc.h"
#include "include/backend/RiscvPrinter.h"
#include "include/backend/SysyPhiDelete.h"
#include "include/frontend/SysYIRGenerator.h"
#include "include/frontend/SysYPrinter.h"
#include "include/midend/ActiveVarAnalysis.h"
#include "include/midend/CFA.h"
#include "include/midend/DCE.h"
#include "include/midend/DFE.h"
#include "include/midend/DataFlowAnalysis.h"
#include "include/midend/FuncAnalysis.h"
#include "include/midend/FuncInline.h"
#include "include/midend/FunctionCache.h"
#include "include/midend/Global2Local.h"
#include "include/midend/Mem2Reg.h"
#include "include/midend/SCCP.h"
#include "include/midend/SysYLoopAnalysis.h"
#include "include/midend/SysYLoopPreStore.h"
#include "include/midend/SysYLoopUnroll.h"
#include "include/midend/SysyLoopoptimization.h"
#include "include/midend/SysyLoopspecificate.h"
#include "include/midend/Sysyoptimization.h"
#include "include/utils/SysyDebugger.h"
#include "include/utils/priority_dequeue.h"
#include "include/midend/TailCallElimination.h"
#include <cstdlib>
#include <fstream>
#include <iostream>

auto main(int argc, char **argv) -> int {
  if (argc != 5 && argc != 6) {
    std::cerr << "Usage: \ncompiler -S -o testcase.s testcase.sy\ncompiler -S "
                 "-o testcase.s testcase.sy -O1\n";
    return EXIT_FAILURE;
  }

  std::string asmFileName = argv[3];
  std::string inputFileName = argv[4];
  std::ifstream fin(inputFileName);
  if (!fin) {
    std::cerr << "Failed to open file " << inputFileName;
    return EXIT_FAILURE;
  }

  // antlr4
  antlr4::ANTLRInputStream input(fin);
  SysYLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  SysYParser parser(&tokens);
  auto compUnit = parser.compUnit();

  sysy::SysYIRGenerator generator;
  generator.visitCompUnit(compUnit);

  sysy::SysyOptimization optimization(generator.getModule(),
                                      generator.getBuilder());
  optimization.SysyOptimizateAll();

  // dce for preoptimization handling
  sysy::DCE dce(generator.getModule());
  dce.run();

  // do function analysis and then dfe
  sysy::FuncAnalysis funcAnalysis(generator.getModule());
  funcAnalysis.run();

  sysy::DFE dfe(generator.getModule());
  dfe.run();

  // dfe may changed some func attributes, so we run func analysis again
  funcAnalysis.run();

  // maybe global will be unused, so we call dce again
  dce.run();

  // do tail call elimination
  sysy::TailCallElimination tce(generator.getModule(), generator.getBuilder());
  tce.run();

  // do func analysis again, cause tce may change the func attributes
  funcAnalysis.run();

  sysy::SysYLoopAnalysis loopanalysis(generator.getModule(),
                                      generator.getBuilder());
  loopanalysis.runBfSSA();

  // inline here
  sysy::FuncInline FI(generator.getModule(), generator.getBuilder());
  FI.run();

  // sysy::SysYPrinter printer(generator.getModule());
  // printer.printIR();
  // inlined func maybe dead.
  dfe.run();

  // maybe doing it here again is better.
  funcAnalysis.run();

  // do cfa
  sysy::CFA cfa(generator.getModule());
  cfa.run();

  // loop analysis for g2l
  loopanalysis.runBfSSA();

  // do g2l for scalar
  sysy::Global2Local G2L(generator.getModule(), generator.getBuilder());
  G2L.run();

  // do mem2reg
  sysy::Mem2Reg mem2reg(generator.getModule(), generator.getBuilder());
  mem2reg.run();

  // do it here for sccp if you want to cp call
  funcAnalysis.run();

  // do sccp
  sysy::SCCP cp(generator.getModule());
  cp.run();

  // do dce
  dce.run();

  optimization.SysyOpforCE();
  funcAnalysis.run();
  cfa.run();
  loopanalysis.runAfSSA();
  sysy::SysYLoopspecificator SysYLoopspecificator1(generator.getModule(),
                                                   generator.getBuilder());
  SysYLoopspecificator1.run();
  loopanalysis.runAfSSA();
  // sysy::SysYPrinter printer(generator.getModule());

  // invariant
  sysy::SysyLoopoptimization loopop(generator.getModule(),
                                    generator.getBuilder());
  loopop.SysyrunInvariant();

  // // loop analyze
  // loopanalysis.runAfSSA();

  // printer.printIR();
  // sysy::SysyLoopPreStore prestore1(generator.getModule(),
  // generator.getBuilder()); prestore1.run(); dce.run(); printer.printIR();
  // loop analyze
  loopanalysis.runAfSSA();
  loopanalysis.LoopfindPhiBeginAndStep();

  // loop unroll
  sysy::SysYLoopUnroll Unroller(generator.getModule(), generator.getBuilder());
  Unroller.run();
  optimization.SysyOpforCE();

  // reg2mem
  sysy::SysyPhiDelete phidelete(generator.getModule(), generator.getBuilder());
  phidelete.PhiDelete();

  // functionCache
  sysy::FunctionCacheBuilder funCacheBuilder(generator.getModule());
  funCacheBuilder.run();

  // printer.printIR();

  // block fix
  sysy::SysyOptimization optimization1(generator.getModule(),
                                       generator.getBuilder());
  optimization1.SysyOptimizateAll();

  // 后端代码生成
  auto selector = new mid2EndDAG::DAGCoverage;
  auto codeGenerator = mid2end::CodeGenerater(25, 29, selector);
  codeGenerator.run(generator.getModule());

  // 后调度
  riscv::InstructionSchedule instschedule;
  instschedule.scheduleModule("", codeGenerator.getModule(), 1, false);

  // 后端代码打印
  riscv::RiscvCodePrinter codePrinter;
  codePrinter.printRiscv(asmFileName, codeGenerator.getModule());

  // 清空前中端的表示，完全转为后端表示(open)
  generator.clear();

  // 调试信息处理
  // sysy::SysyDebugger::printCFG(generator.getModule());
  // sysy::SysyDebugger::printDomTree(generator.getModule());
  // sysy::SysyDebugger::printValue2Blocks(generator.getModule());
  // sysy::SysyDebugger::printDataFlow(dataFlowManager, generator.getModule());
  // sysy::SysyDebugger::printBackEndDependGraph(instschedule);
  // sysy::SysyDebugger::printBackEndBasicblockInfo(dAG2EndBuilder.getModule());

  return EXIT_SUCCESS;
}
