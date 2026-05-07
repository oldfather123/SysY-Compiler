#include <bits/stdc++.h>

#include "SysYLexer.h"
#include "SysYParser.h"
#include "ir.hpp"
#include "irgen.hpp"
#include "pass.hpp"
#include "riscvgen.hpp"
#include "visitor.hpp"

#define GENIR
// #define GENASM

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  std::string full_filename("137_dct.sy");
  std::string filename("137_dct");
  fs::path sy_dir("../test/sy");
  std::ifstream sy_stream;
  sy_stream.open(sy_dir / (filename + ".sy"), std::ios::in);
  antlr4::ANTLRInputStream antlr_input_stream(sy_stream);
  SysYLexer sysy_lexer(&antlr_input_stream);
  antlr4::CommonTokenStream common_token_stream(&sysy_lexer);
  SysYParser sysy_parser(&common_token_stream);
  IR::Visitor sysy_visitor;
  IR::CompileUnit *cu = std::any_cast<IR::CompileUnit *>(sysy_visitor.visitComp(sysy_parser.comp()));
  configuration.setFilename(full_filename);

  Pass::function_pass_mana.newPass(new Pass::Rename());
  for (const auto &[name, f] : *cu->functionTable()) {
    Pass::function_pass_mana.run(f.get());
  }

#ifdef GENIR  // allow gen ir
  configuration.setOstream(std::cout);
  IR::Generator sysy_ir_gener(cu);
  sysy_ir_gener.generate(INFO);
#endif

#ifdef GENASM  // allow gen asm
  configuration.setOstream(std::cout);
  RISCV::Gener sysy_riscv_gener(comp);
  sysy_riscv_gener.gen(INFO);
#endif

  sy_stream.close();
  return 0;
}
