#include "AST/decl.h"
#include "AST/expr.h"
#include "AST/stmt.h"
#include "ASTContext.h"
#include "common.h"
#include "myparser.h"
#include "backend.h"
#include "frontend.h"
#include "ADT/CFG.h"
#include "reg_alloc.h"
#include "utils/Timer.h"
#include "riscv.h"
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace aaac;
using namespace aaac::frontend;

extern void SCCP(ControlFlowGraph *cfg);
extern void simplifyCFG(ControlFlowGraph *cfg);
extern void simplifyCFG_AfterSSA(ControlFlowGraph *cfg);
extern void CopyPropagation(ControlFlowGraph *cfg);
extern void StrengthReduction(ControlFlowGraph *cfg);
extern void analyzeLoopInfo(ControlFlowGraph *cfg);
extern void loopPassManager(ControlFlowGraph *cfg);

void printCFGPass(ControlFlowGraph *cfg){
  std::cout<<"========="<<cfg->getFunction()->getName()<<"========="<<std::endl;
  cfg->dump(std::cout);
}

void BuildSSAPass(ControlFlowGraph *cfg) {
   cfg->buildSSA();
}

void EliminatePHIPass(ControlFlowGraph *cfg) {
  cfg->eliminatePhi();
}

void BuildIntervalPass(ControlFlowGraph *cfg) {
  auto intervals = aaac::backend::buildLiveIntervals(cfg->getFunction()->getFunctionBody().get());
  utils::g_property_mgr.init<aaac::backend::LiveIntervalProperty>(cfg, intervals);
}

void RegAllocPass(ControlFlowGraph *cfg) {
    auto *prop = utils::g_property_mgr.get<aaac::backend::LiveIntervalProperty>(cfg);
    std::vector<aaac::backend::LiveInterval> &intervals = prop->intervals;
    //std::vector<backend::LiveInterval> intervals;
    std::vector<backend::RISCV_Reg> int_regs = {
        backend::RISCV_Reg::__a0,
        backend::RISCV_Reg::__a1,
        backend::RISCV_Reg::__a2,
        backend::RISCV_Reg::__a3,
        backend::RISCV_Reg::__a4,
        backend::RISCV_Reg::__a5,
        backend::RISCV_Reg::__a6,
        backend::RISCV_Reg::__a7,
        // backend::RISCV_Reg::__s0, 作为fp
        backend::RISCV_Reg::__s1,
        backend::RISCV_Reg::__s2,
        backend::RISCV_Reg::__s3,
        backend::RISCV_Reg::__s4,
        backend::RISCV_Reg::__s5,
        backend::RISCV_Reg::__s6,
        backend::RISCV_Reg::__s7,
        backend::RISCV_Reg::__s8,
        backend::RISCV_Reg::__s9,
        backend::RISCV_Reg::__s11,
        // 临时寄存器暂时不参与寄存器分配，用于简化codegen
    };
    //TODO: ftx不参与寄存器分配，因为这是callersave的，但是寄存器分配为了简单没考虑这个
    std::vector<backend::RISCV_Reg> float_regs = {
        backend::RISCV_Reg::__fs1,
        backend::RISCV_Reg::__fs2,
        backend::RISCV_Reg::__fs3,
        backend::RISCV_Reg::__fs4,
        backend::RISCV_Reg::__fs5,
        backend::RISCV_Reg::__fs6,
        backend::RISCV_Reg::__fs7,
        backend::RISCV_Reg::__fs8,
        backend::RISCV_Reg::__fs9,
        backend::RISCV_Reg::__fs10,
        backend::RISCV_Reg::__fs11,
        backend::RISCV_Reg::__fa0,
        backend::RISCV_Reg::__fa1,
        backend::RISCV_Reg::__fa2,
        backend::RISCV_Reg::__fa3,
        backend::RISCV_Reg::__fa4,
        backend::RISCV_Reg::__fa5,
        backend::RISCV_Reg::__fa6,
        backend::RISCV_Reg::__fa7,
    };
    aaac::backend::performLinearScan(cfg->getFunction()->getFunctionBody().get(), int_regs, float_regs, intervals);
}

int main(int argc, char *argv[]) {
    std::ifstream infile;
    std::istream *input = &std::cin;
    std::string output_file, input_file;
    // if (argc > 1) {
    //     infile.open(argv[1]);
    //     if (!infile.is_open()) {
    //         std::cerr << "Failed to open input file: " << argv[1] << std::endl;
    //         return 1;
    //     }
    //     input = &infile;
    //     std::string name(argv[1]);
    //     auto pos = name.find_last_of('.');
    //     if (pos != std::string::npos)
    //         output_file = name.substr(0, pos) + ".s";
    //     else
    //         output_file = name + ".s";
    // }

    for (int i = 1; i < argc; i++) {
        std::string str = argv[i];
        if (str.find('-') == 0) {
            if (str == "-o") {
                if(i + 1 >= argc) {
                    std::cerr << "No output file\n";
                    return 1;
                }
                output_file = argv[i + 1];
                i += 1;
            } else if(str == "-S") {
                continue;
            } else if(str.size() >= 2 && str[1] == 'O') {
                continue;
            }
        } else {
            input_file = str;
            infile.open(str);
            if (!infile.is_open()) {
                std::cerr << "Failed to open input file: " << str << std::endl;
                return 1;
            }
            input = &infile;
        }
    }

    if (output_file == "" && input_file != "") {
        std::string name(input_file);
        auto pos = name.find_last_of('.');
        if (pos != std::string::npos)
            output_file = name.substr(0, pos) + ".s";
        else
            output_file = name + ".s";
    }

    // ignore list
    // Assert(input_file.find("03_sort2")==std::string::npos, "Not implemented");

    ASTContext context(*input);
    context.parse();
    context.translate();
    auto module = context.transferModule();
    std::cout << *module << std::endl;
    auto errmsg = context.transferErrMsg();
    errmsg->traverse([](std::string msg) {
        std::cout << msg << std::endl;
    });
    utils::Timer timer;
    timer.reset();
    module->buildCFG();
    std::cout<<"Build CFG Time: "<<timer.elapsed()<<std::endl;
    timer.reset();
    module->executePass(BuildSSAPass);
    std::cout<<"Build SSA Time: "<<timer.elapsed()<<std::endl;
    timer.reset();
    module->executePass(SCCP);
    module->executePass(CopyPropagation);
    std::cout<<"SCCP Time: "<<timer.elapsed()<<std::endl;
    timer.reset();
    module->executePass(StrengthReduction);
    std::cout<<"SR Time: "<<timer.elapsed()<<std::endl;
    timer.reset();
    module->executePass(printCFGPass);
    module->executePass(simplifyCFG);
    module->executePass(printCFGPass);
    module->executePass(loopPassManager);
    module->executePass(printCFGPass);
    module->executePass(simplifyCFG);
    module->executePass(EliminatePHIPass);
    std::cout<<"Eliminate SSA Time: "<<timer.elapsed()<<std::endl;
    timer.reset();
    module->executePass(simplifyCFG_AfterSSA); // 我也不知道有没有用，不过先放着）
    auto backend_insns = module->generateInsnList();
    aaac::backend::peephole();
    std::cout << "========================codegen before reg alloc=========================\n";
    module->executePass(aaac::backend::LivenessPass);
    module->executePass(BuildIntervalPass);
    module->executePass(RegAllocPass);
    std::cout << "========================codegen=========================\n";
    if (output_file.empty())
        backend::codegen();
    else
        backend::codegen(output_file);
    return 0;
}
