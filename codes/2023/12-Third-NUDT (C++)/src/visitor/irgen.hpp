#ifndef __IRGEN_HPP__
#define __IRGEN_HPP__

#include "global.hpp"
#include "ir.hpp"

using namespace IR;

namespace IR {

class Generator {
private:
  CompileUnit *_cu;
  const string IDENT = "  ";

public:
  Generator(CompileUnit *comp);
  void generate(LogLevel log_level);
  void generate(CompileUnit *comp);
  void generate(Function *func);
  void generate(BasicBlock *block);
  void generate(Instruction *inst);
};

}  // namespace IR

#endif
