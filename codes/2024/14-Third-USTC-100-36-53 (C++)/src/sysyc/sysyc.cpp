#include "sysyc.hpp"
#include "AllUnrolling.hpp"
#include "AnalysisPass.hpp"
#include "ConstProp.hpp"
#include "DeadCode.hpp"
#include "FuncInline.hpp"
#include "GVN.hpp"
#include "Instruction.hpp"
#include "InstructionSelector.hpp"
#include "LICM.hpp"
#include "LivenessAnalysis.hpp"
#include "LoopDetection.hpp"
#include "LoopGepCombine.hpp"
#include "LoopSimplify.hpp"
#include "MIDCE.hpp"
#include "MachineModule.hpp"
#include "MachinePass.hpp"
#include "Mem2Reg.hpp"
#include "Module.hpp"
#include "Operand.hpp"
#include "PartiallyUnrolling.hpp"
#include "PassBuilder.hpp"
#include "PassManager.hpp"
#include "PeepholeOptimization.hpp"
#include "RegisterAllocation.hpp"
#include "SCEV.hpp"
#include "SimplifyCFG.hpp"
#include "sysy_builder.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using std::string;
using namespace std::string_literals;

struct Config {
    string exe_name; // compiler exe name
    std::filesystem::path input_file;
    std::filesystem::path output_file;

    bool emitllvm{false};
    bool emitasm{false};
    // bool opt{false};
    OptimizationLevel opt{OptimizationLevel::O0};
    std::string passes = "";

    Config(int argc, char **argv) : argc(argc), argv(argv) {
        parse_cmd_line();
        check();
    }

  private:
    int argc{-1};
    char **argv{nullptr};

    void parse_cmd_line();
    void check();
    // print helper infomation and exit
    void print_help() const;
    void print_err(const string &msg) const;
};

int main(int argc, char **argv) {
    Config config(argc, argv);

    std::unique_ptr<Module> m;
    {
        auto syntax_tree = parse(config.input_file.c_str());
        auto ast = AST(syntax_tree);
        SysyBuilder builder;
        ast.run_visitor(builder);
        m = builder.getModule();
    }

    PassBuilder PB;
    auto PM = std::make_shared<PassManager>();
    auto APM = std::make_shared<AnalysisPassManager>();
    PB.registerPassManager(PM);
    PB.registerAnalysisPassManager(APM);
    PB.parsePipeline(config.passes, config.opt);

    PM->run(m.get(), *APM);

    std::ofstream output_stream(config.output_file);
    if (config.emitllvm) {
        auto abs_path = std::filesystem::canonical(config.input_file);
        output_stream << "; ModuleID = 'SysY'\n";
        output_stream << "source_filename = " << abs_path << "\n\n";
        output_stream << m->print();
    } else if (config.emitasm) {
        // CodeGen codegen(m.get());
        // codegen.run();
        // output_stream << codegen.print();
        auto MM = std::make_shared<MachineModule>(m.get());
        auto MPM = std::make_shared<MachinePassManager>(MM);
        MPM->add_pass<InstructionSelector>();
        MPM->add_pass<PeepholeOptimization>();
        MPM->add_pass<MIDeadCodeElimination>();
        MPM->add_pass<RegisterAllocation>();
        MPM->run();
        output_stream << MM->print();
    }
    return 0;
}

void Config::parse_cmd_line() {
    exe_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help();
        } else if (argv[i] == "-o"s) {
            if (output_file.empty() && i + 1 < argc) {
                output_file = argv[i + 1];
                i += 1;
            } else {
                print_err("bad output file");
            }
        } else if (argv[i] == "-emit-llvm"s) {
            emitllvm = true;
        } else if (argv[i] == "-S"s) {
            emitasm = true;
        } else if (argv[i] == "-O1"s) {
            opt = OptimizationLevel::O1;
        } else if (argv[i] == "-O2"s) {
            opt = OptimizationLevel::O2;
        } else if (argv[i] == "-O3"s) {
            opt = OptimizationLevel::O3;
        } else if (strncmp(argv[i], "-passes=", 8) == 0) {
            passes = argv[i] + 8;
        } else {
            if (input_file.empty()) {
                input_file = argv[i];
            } else {
                string err =
                    "unrecognized command-line option \'"s + argv[i] + "\'"s;
                print_err(err);
            }
        }
    }
}

void Config::check() {
    if (input_file.empty()) {
        print_err("no input file");
    }
    if (input_file.extension() != ".sy") {
        print_err("file format not recognized");
    }
    if (emitllvm and emitasm) {
        print_err("emit llvm and emit asm both set");
    }
    if (not emitllvm and not emitasm) {
        print_err("not supported: generate executable file directly");
    }
    if (output_file.empty()) {
        output_file = input_file.stem();
        if (emitllvm) {
            output_file.replace_extension(".ll");
        } else if (emitasm) {
            output_file.replace_extension(".s");
        }
    }
}

void Config::print_help() const {
    std::cout << "Usage: " << exe_name
              << " [-h|--help] [-o <target-file>] [-opt] [-emit-llvm] [-S] "
                 "<input-file>"
              << std::endl;
    exit(0);
}

void Config::print_err(const string &msg) const {
    std::cout << exe_name << ": " << msg << std::endl;
    exit(-1);
}
