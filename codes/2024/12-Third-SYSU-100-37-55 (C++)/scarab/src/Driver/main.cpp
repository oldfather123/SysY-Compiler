#include <iostream>
#include <getopt.h>
#include <string>

#include "antlr4-runtime.h"
#include "SysY2022Lexer.h"
#include "MyVisitor.h"
#include "optimize.h"
#include "riscv.h"

using namespace antlr4;

int main(int argc, char ** argv) {
  std::ifstream sourceFile(argv[4]);
  assert(sourceFile.is_open());

  ANTLRInputStream input(sourceFile);
  SysY2022Lexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();
  
  SysY2022Parser parser(&tokens);
  SysY2022Parser::CompUnitContext* tree = parser.compUnit();

  MyVisitor visitor;
  visitor.visitCompUnit(tree);
  optimize(visitor.irModule);

  //visitor.print();

  auto program_asm=emit_vis(visitor.irModule);
  cerr << "finish emit_vis" << endl;
  backendOpt(program_asm,false);
  cerr << "finish backendOpt" << endl;
  Asm_Buffer s;
  build_program_asm(&s, program_asm, visitor.irModule.globalVariables);
  s.add_terminator();

  FILE* assembly_file = fopen(argv[3], "w");
  if (assembly_file == NULL) {
      assert(false && "error opening assembly output file");
  }

  fprintf(assembly_file, "%s", s.c_str());
  fclose(assembly_file);
  return 0;
}