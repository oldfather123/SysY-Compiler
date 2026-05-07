#ifndef __RISCVGEN_HPP__
#define __RISCVGEN_HPP__

#include "error.hpp"
#include "global.hpp"
#include "ir.hpp"
#include "riscv.hpp"

namespace RISCV {

using std::string;
using std::to_string;
using std::variant;
using OType = enum OType : size_t { REG, IMM };

class RegMana;
class Operand;
class Gener;

class RegMana {
public:
  static string toString(Reg reg) {
    return "r" + to_string(reg);
  }
};

class Operand {
public:
  OType _otype;
  std::variant<uint32_t, Reg> _operand;
  Operand() = default;
  Operand(uint32_t imm) : _otype(IMM), _operand(imm) {
  }
  Operand(Reg reg) : _otype(REG), _operand(reg) {
  }
  template <typename T>
  T *operand() {
    return std::get_if<T>(&_operand);
  }
  string toString() {
    if (_otype == IMM)
      return "#" + to_string(*operand<uint32_t>());
    else
      return RegMana::toString(*operand<Reg>());
  }
};

class Gener {
private:
  static constexpr std::string_view IDENT = "  ";
  IR::CompileUnit *_comp;

public:
  Gener(IR::CompileUnit *comp);
  void gen(LogLevel log_level);
  void gen(IR::CompileUnit *comp);
  void gen(IR::Function *func);
  void gen(IR::BasicBlock *block);
  void gen(IR::Instruction *inst);
};

}  // namespace RISCV

#endif
