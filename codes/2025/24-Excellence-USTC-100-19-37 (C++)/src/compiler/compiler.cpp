#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "ast.hpp"
#include "SysYBuilder.hpp"
#include "CodeGen.hpp"
#include "Mem2Reg.hpp"
#include "LiveVariableAnalysis.hpp"
#include "LICM.hpp"
#include "DeadCode.hpp"

#include "../parser/syntax_analyzer.h"
extern syntax_tree *parse(const char *input);

using std::string;
using std::operator""s;

struct Config {
  public:
    string exe_name; // compiler exe name
    std::filesystem::path input_file;
    std::filesystem::path output_file;
    bool ast = false;
    bool llvm = false;
    bool assembly = false;

    Config(int argc, char **argv) : argc(argc), argv(argv) {
        parse_cmd_line();
        check();
    }

  private:
    int argc = -1;
    char **argv = nullptr;

    void parse_cmd_line();
    void check();
};



int main(int argc, char **argv) {
    Config config(argc, argv);

    auto syntax_tree = parse(config.input_file.c_str());
    auto ast = AST(syntax_tree);

    {
        SysYBuilder builder;
        ast.run_visitor(builder);
        std::unique_ptr<Module> m = builder.getModule();
        
        auto DeadCodePass = std::make_unique<DeadCode>(m.get());
        auto Mem2RegPass = std::make_unique<Mem2Reg>(m.get());
        auto LICMPass = std::make_unique<LoopInvariantCodeMotion>(m.get());
        Mem2RegPass->run();
        DeadCodePass->run();
        LICMPass->run();

        std::ofstream output_stream(config.output_file);
        CodeGen codegen(m.get());
        codegen.run();
        output_stream << codegen.print();
    }

    return 0;
}

void Config::parse_cmd_line() {
    exe_name = argv[0];
    for (int i = 1; i < argc; i++) {
        if (argv[i] == "-o"s) {
            if (output_file.empty() && i + 1 < argc) {
                output_file = argv[i + 1];
                i += 1;
            }
        } else if (argv[i] == "-S"s) {
            assembly = true;
        } else if (argv[i] == "-O1"s) {
            ;
        } else {
            if (input_file.empty()) {
                input_file = argv[i];
            }
        }
    }
}

void Config::check() {
    if (output_file.empty()) {
        output_file = input_file.stem();
        output_file.replace_extension(".s");
    }
}

