#pragma once
#include "Argument.hh"
#include "Instruction.hh"
#include <string>
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::set;

class MInstruction;
class IRegister;
class FRegister;

class Register {
private:
  set<MInstruction *> uses;

public:
  static IRegister *reg_zero;
  static IRegister *reg_ra; // saved
  static IRegister *reg_sp; // saved
  static IRegister *reg_gp;
  static IRegister *reg_tp;
  static IRegister *reg_t0;
  static IRegister *reg_t1;
  static IRegister *reg_t2;
  static IRegister *reg_s0; // 注意：s0 和 fp 是同一个寄存器
  static IRegister *reg_s1;
  static IRegister *reg_a0;
  static IRegister *reg_a1;
  static IRegister *reg_a2;
  static IRegister *reg_a3;
  static IRegister *reg_a4;
  static IRegister *reg_a5;
  static IRegister *reg_a6;
  static IRegister *reg_a7;
  static IRegister *reg_s2;
  static IRegister *reg_s3;
  static IRegister *reg_s4;
  static IRegister *reg_s5;
  static IRegister *reg_s6;
  static IRegister *reg_s7;
  static IRegister *reg_s8;
  static IRegister *reg_s9;
  static IRegister *reg_s10;
  static IRegister *reg_s11;
  static IRegister *reg_t3;
  static IRegister *reg_t4;
  static IRegister *reg_t5;
  static IRegister *reg_t6;

  static FRegister *reg_ft0;
  static FRegister *reg_ft1;
  static FRegister *reg_ft2;
  static FRegister *reg_ft3;
  static FRegister *reg_ft4;
  static FRegister *reg_ft5;
  static FRegister *reg_ft6;
  static FRegister *reg_ft7;
  static FRegister *reg_fs0;
  static FRegister *reg_fs1;
  static FRegister *reg_fa0;
  static FRegister *reg_fa1;
  static FRegister *reg_fa2;
  static FRegister *reg_fa3;
  static FRegister *reg_fa4;
  static FRegister *reg_fa5;
  static FRegister *reg_fa6;
  static FRegister *reg_fa7;
  static FRegister *reg_fs2;
  static FRegister *reg_fs3;
  static FRegister *reg_fs4;
  static FRegister *reg_fs5;
  static FRegister *reg_fs6;
  static FRegister *reg_fs7;
  static FRegister *reg_fs8;
  static FRegister *reg_fs9;
  static FRegister *reg_fs10;
  static FRegister *reg_fs11;
  static FRegister *reg_ft8;
  static FRegister *reg_ft9;
  static FRegister *reg_ft10;
  static FRegister *reg_ft11;

  static Register *getIRegister(int idx);

  static Register *getFRegister(int idx);

  enum RegTag {
    F_REGISTER,
    I_REGISTER,
    V_FREGISTER, // virtual float register
    V_IREGISTER, // virtual int register
    IR_REGISTER, // to be resolved
    NONE, // used when the subclass of Register is not actually a Register
  };

private:
  RegTag tag;
  string name;

public:
  Register(string name, RegTag tag);
  string getName();
  void setName(string name);
  RegTag getTag();

  void addUse(MInstruction *use);
  void removeUse(MInstruction *use);
  void replaceRegisterWith(Register *newReg);
  set<MInstruction *> &getUses();
  void clearUses();
  virtual ~Register() {};
};

class IRRegister : public Register {
public:
  Instruction *ir_reg;
  IRRegister(Instruction *ins);
};

class VRegister : public Register {
private:
  bool is_pointer;
  bool is_instruction;

public:
  VRegister(RegTag tag, bool is_pointer, bool is_instruction);
  VRegister(RegTag tag, string name, bool is_pointer, bool is_instruction);
  bool isPointer() { return is_pointer; }
  bool isInstruction() { return is_instruction; }
};

class ParaRegister : public VRegister {
private:
  Register *reg = nullptr;
  int offset;
  uint size;

public:
  ParaRegister(int offset, uint size, string name, RegTag rt, bool is_pointer)
      : VRegister(rt, name, is_pointer, false), offset(offset), size(size) {}
  ParaRegister(Register *reg, string name, RegTag rt, bool is_pointer)
      : VRegister(rt, name, is_pointer, false), reg(reg) {}
  Register *getRegister() { return reg; }
  int getOffset() { return offset; }
  uint getSize() { return size; }
};

class IRegister : public Register {
private:
  int id;

public:
  IRegister(int id, string n);
};

class FRegister : public Register {
private:
  int id;

public:
  FRegister(int id, string n);
};


#define IS_VIRTUAL_REG(reg) ((reg)->getTag() == Register::V_FREGISTER || (reg)->getTag() == Register::V_IREGISTER)