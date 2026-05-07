#include <iostream>
#include <ostream>
#include <string>
#include "antlr4-common.h"
#include "SysYLexer.h"
#include "SysYParser.h"
#include "MyVisitor.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/LinearScan.hpp"
#include "codegen/LiveAnalysis.hpp"
#include "codegen/MachineCodePass.hpp"
#include "codegen/MachineFunction.hpp"

#include "HIR-opt/HOptimizer.h"

#ifndef TESTCASE_PATH
#define TESTCASE_PATH "./testcases"
#endif

int main(int argc, const char* argv[]) {
    std::string testcase_path(TESTCASE_PATH);
    string test_file;
    std::string output_file;
    bool need_opt = true;
    if(argc == 1){
        test_file = testcase_path + "/test.sy";
    }
    else if(argc == 2) {// local test
        test_file = testcase_path + "/" + argv[1];
    }
    else{ // argc > 2 : online test
        test_file = argv[4];
        output_file = argv[3];
        if(argc == 5) need_opt = false;
    }


    std::ifstream stream(test_file);
    assert(stream);
    antlr4::ANTLRInputStream input(stream);
    antlr4::SysYLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    antlr4::SysYParser parser(&tokens);

    // gen HIR
    antlr4::tree::ParseTree *tree = parser.program();
    MyVisitor* visitor = new MyVisitor();
    visitor->add_sylib_func();
    visitor->visit(tree);

    // opt HIR
    GlobalUnit *gu = visitor->globalUnit;
    if(need_opt) {
        HOptimizer *opt = new HOptimizer(gu);
        opt->Optimize();
    }
    // gu->Emit(std::cout);

   auto builder = make_unique<AsmBuilder>();
   auto mUnit = make_unique<MachineUnit>();
   gu->codegen(mUnit.get(), builder.get());

   for (auto mFunc : mUnit->func_list) {
       // 按基本块的拓扑序给指令编号
       auto IR_func = mFunc->IR_func;
       for (auto bb : IR_func->getReversePostOrder()) {
           bb->mBlock->begin_no = MachineInstruction::counter;
           for (auto inst : bb->mBlock->inst_list) {
               inst->no = MachineInstruction::counter++;
               mFunc->no2inst.emplace_back(inst);
           }
           bb->mBlock->end_no = MachineInstruction::counter - 1;
       }
   }

// mUnit->output(std::cout);

   std::vector< std::unique_ptr<MachineCodePass> > mPasses;
   mPasses.emplace_back(new LiveAnalysis());
   mPasses.emplace_back(new LinearScan());
   for (auto &mPass : mPasses) mPass->pass(mUnit.get());
   if (output_file.empty()) output_file = std::string(TESTCASE_PATH) + "/"s + "out.s"s;
   std::ios::sync_with_stdio(false);
   std::ofstream output_stream(output_file, std::ios::out | std::ios::trunc);
   mUnit->output(output_stream);
//  mUnit->output(std::cerr);

    return 0;
}
