#include <filesystem>
#include <fstream>
#include <iostream>

#include "BasicBlock.hh"
#include "Constant.hh"
#include "DomTree.hh"
#include "Function.hh"
#include "Instruction.hh"
#include "Machine.hh"
#include "Module.hh"
#include "MySysYParserVisitor.h"
#include "SysYParserLexer.h"
#include "SysYParserParser.h"
#include "antlr4-runtime.h"
#include "cgen.hh"

void create_directories_if_not_exists(const std::filesystem::path &file_path);

// With option -l, output is LLVM IR;
// With option -r, output is RISCV Code.
int main(int argc, char *argv[]) {
  bool opt = false;
  if (argc != 5 && argc != 6) {
    std::cout << argc << std::endl;
    std::cerr << "Usage: compiler -l/-S -o outputfile sourcefile [-O1]"
              << std::endl;
    return 1;
  }
  if (argc == 6) {
    opt = true;
  }

  std::string sourcefile = argv[4];
  std::string outputfile = argv[3];
  enum { LLVM, RISCV } mode = strcmp(argv[1], "-l") == 0 ? LLVM : RISCV;

  create_directories_if_not_exists(outputfile);

  std::ifstream source(sourcefile);
  assert(source);

  antlr4::ANTLRInputStream input(source);

  SysYParserLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);

  SysYParserParser parser(&tokens);

  auto visitor = new MySysYParserVisitor(new variableTable(nullptr));

  auto prgc = parser.program();

  visitor->visitProgram(prgc);

  ANTPIE::Module *module = &visitor->module;

  std::ofstream out_ll;
  // out_ll.open("tests/test.ll");
  // module->printIR(out_ll);

  module->irOptimize();

  // std::ofstream out_opt_ll;
  // out_opt_ll.open("tests/test.opt.ll");
  // module->printIR(out_opt_ll);

  if (mode == LLVM) {
    out_ll.open(outputfile);
    module->printIR(out_ll);
    return 0;
  }

  MModule *mmodule = new MModule();
  generate_code(mmodule, module, opt);
  std::ofstream out_s;
  out_s.open(outputfile);
  out_s << *mmodule;

  delete visitor;
}

void create_directories_if_not_exists(const std::filesystem::path &file_path) {
  std::filesystem::path parent_path = file_path.parent_path();
  if (!(std::filesystem::exists(parent_path) || parent_path.empty())) {
    std::filesystem::create_directories(parent_path);
  }
}