#pragma once

#include "GlobalVariable.hh"
#include "Instruction.hh"
#include "Register.hh"
#include <cassert>
#include <memory>
#include <set>
#include <vector>

using std::ostream;
using std::set;
using std::unique_ptr;

class MBasicBlock;
class MGlobal;
class MFunction;

class MInstruction : public VRegister {
public:
  enum MITag { // Machine Instruction Tag
    // Beacuse we only have 32-bits interger and float in SysY, so we don't need
    // 64-bits-wise instruction provided by RV64GC
    // H means H-level
    H_PHI,
    H_ALLOCA,
    H_RET,
    H_CALL,

    H_BR,
    H_ICMP,
    COMMENT,

    // RV64I
    //// Integer Computation
    ADD,
    ADDI, // to add ra/sp
    ADDIW,
    ADDW,

    SUBW,
    AND,
    OR,
    XOR,
    ANDI,
    ORI,
    XORI,

    SLL,
    SRA,
    SRL,
    SLLI,
    SRAI,
    SRLI,
    // LUI,
    // AUIPC,
    SLT,
    SLTI,
    SLTU,
    SLTIU,
    //// Loads and Stores
    LW,
    SW,
    LD, // to store/load ra/sp
    SD,
    //// Control transfer
    BEQ,
    BNE,
    BGE,
    BLT,
    // BGEU,
    // BLTU,
    // JAL,
    // JALR,

    // RV64M
    MUL,
    MULW,
    // MULHU,
    // MULHSU,
    DIVW,
    REMW,
    // DIVUW,
    // REMUW,

    // RV64F and RV64D
    //// Floating-Point Computation
    FADD_S,
    FSUB_S,
    FMUL_S,
    FDIV_S,
    // FSQRT_S,
    // FMIN_S,
    // FMAX_S,
    // FNMADD_S,
    // FNMSUB_S,
    FMV_S_X,
    // FMV_X_S,
    //// Load and Store
    FLW,
    FSW,
    FLD,
    FSD,
    //// Conversion
    FCVTS_W,
    // FCVTS_WU,
    FCVTW_S,
    // FCVTWU_S,
    //// Comparison
    FEQ_S,
    FLT_S,
    FLE_S,

    // presudo instruction: https://michaeljclark.github.io/asm.html
    CALL,
    J,
    RET,
    LI,
    LA,
    MV,
    FMV_S,
  };

private:
  MITag insTag;
  Register *target = nullptr; // target in non-SSA version
  unique_ptr<vector<Register *>> oprands;
  MBasicBlock *bb;
  string comment;

public:
  MInstruction(MITag insTag, RegTag rt, string name, bool is_pointer = false);
  MInstruction(MITag insTag, RegTag rt, bool is_pointer = false);

  void setTarget(Register *reg);
  Register *getTarget();

  void setComment(string comment);
  string getComment();

  // special case on ret, call and phi...
  virtual void replaceIRRegister(map<Instruction *, Register *> instr_map);
  virtual void replaceRegister(Register *oldReg, Register *newReg);

  unique_ptr<MInstruction> replaceWith(vector<MInstruction *> instrs);
  void insertBefore(vector<MInstruction *> instrs);
  void insertAfter(vector<MInstruction *> instrs);

  void setBasicBlock(MBasicBlock *bb);
  MBasicBlock *getBasicBlock() const { return bb; }

  void pushReg(Register *r);
  int getRegNum() const;
  Register *getReg(int idx) const;
  void setReg(int idx, Register *reg) const;

  MITag getInsTag() const;

  virtual ostream &printASM(ostream &stream) = 0;
  friend std::ostream &operator<<(std::ostream &os, MInstruction &obj);
};

///////////////////////////////////////////////////////
enum MIOprandTp { Float, Int, Reg, None };
struct MIOprand {
  MIOprandTp tp;
  union Arg {
    float f;
    int32_t i;
    Register *reg;
  } arg;
};

// todo: repalce arguments
class MHIphi : public MInstruction {
private:
  unique_ptr<vector<MBasicBlock *>> incoming;
  unique_ptr<vector<MIOprand>> opds;

public:
  MHIphi(string name, RegTag rt, bool is_pointer = false);
  ostream &printASM(ostream &stream) override;
  void pushIncoming(Register *reg, MBasicBlock *bb);
  void pushIncoming(int i, MBasicBlock *bb);
  void pushIncoming(float f, MBasicBlock *bb);
  void replaceIncoming(MBasicBlock *oldbb, MBasicBlock *newbb);
  MBasicBlock *getIncomingBlock(int idx) const;
  MIOprand getOprand(int idx) const { return opds->at(idx); };
  int getOprandNum() const { return opds->size(); };
  void replaceIRRegister(map<Instruction *, Register *> instr_map) override;
  void replaceRegister(Register *oldReg, Register *newReg) override;
};

class MHIalloca : public MInstruction {
private:
  uint32_t size;

public:
  MHIalloca(uint32_t size, string name);
  ostream &printASM(ostream &stream) override;
  uint32_t getSize() { return size; }
};

// todo: repalce arguments
class MHIret : public MInstruction {
public:
  MIOprand r;
  MHIret();
  MHIret(int imm);
  MHIret(float imm);
  MHIret(Register *reg);
  ostream &printASM(ostream &stream) override;
  void replaceIRRegister(map<Instruction *, Register *> instr_map) override;
  void replaceRegister(Register *oldReg, Register *newReg) override;
};

// todo: repalce arguments????
class MHIcall : public MInstruction {
public:
  MFunction *function;
  unique_ptr<vector<MIOprand>> args;

public:
  // return value can not be ptr in sysy
  MHIcall(MFunction *func, string name, RegTag rt);
  MHIcall(MFunction *func, RegTag rt);
  void pushArg(float f);
  void pushArg(int i);
  void pushArg(Register *r);
  int getArgNum();
  MIOprand &getArg(int idx);
  ostream &printASM(ostream &stream) override;
  vector<MInstruction *> generateCallSequence(
      MFunction *func, int stack_offset, map<Register *, int> *spill,
      map<Register *, Register *> *allocation, set<Register *> *lineIn);
  void replaceIRRegister(map<Instruction *, Register *> instr_map) override;
  void replaceRegister(Register *oldReg, Register *newReg) override;
};

class MHIbr : public MInstruction {
private:
  MBasicBlock *t_bb;
  MBasicBlock *f_bb;

public:
  MHIbr(Register *reg, MBasicBlock *t_bb, MBasicBlock *f_bb);
  void setTBlock(MBasicBlock *bb);
  void setFBlock(MBasicBlock *bb);
  MBasicBlock *getTBlock();
  MBasicBlock *getFBlock();
  ostream &printASM(ostream &stream) override;
};

class MHIicmp : public MInstruction {
private:
  OpTag optag;

public:
  MHIicmp(OpTag optag, Register *reg1, Register *reg2, std::string name);
  OpTag getOpTag();
  ostream &printASM(ostream &stream) override;
};

#define DEFINE_MI_BIN_CLASS(NAME)                                              \
  class MI##NAME : public MInstruction {                                       \
  public:                                                                      \
    MI##NAME(Register *reg1, Register *reg2);                                  \
    MI##NAME(Register *reg1, Register *reg2, Register *target);                \
    MI##NAME(Register *reg1, Register *reg2, std::string name);                \
    ostream &printASM(ostream &stream) override;                               \
  };

#define DEFINE_MI_IMM_CLASS(NAME)                                              \
  class MI##NAME : public MInstruction {                                       \
  public:                                                                      \
    int imm;                                                                   \
                                                                               \
  public:                                                                      \
    MI##NAME(Register *reg, int32_t imm);                                      \
    MI##NAME(Register *reg, int32_t imm, Register *target);                    \
    MI##NAME(Register *reg, int32_t imm, std::string name);                    \
    ostream &printASM(ostream &stream) override;                               \
  };

#define DEFINE_MIN_UNA_CLASS(NAME)                                             \
  class MI##NAME : public MInstruction {                                       \
  public:                                                                      \
    MI##NAME(Register *reg);                                                   \
    MI##NAME(Register *reg, Register *target);                                 \
    MI##NAME(Register *reg, std::string name);                                 \
    ostream &printASM(ostream &stream) override;                               \
  };

DEFINE_MI_IMM_CLASS(addi)
DEFINE_MI_BIN_CLASS(add)
DEFINE_MI_IMM_CLASS(addiw)
DEFINE_MI_BIN_CLASS(addw)
DEFINE_MI_BIN_CLASS(subw)
DEFINE_MI_BIN_CLASS(and)
DEFINE_MI_IMM_CLASS(andi)
DEFINE_MI_BIN_CLASS(or)
DEFINE_MI_IMM_CLASS(ori)
DEFINE_MI_BIN_CLASS(xor)
DEFINE_MI_IMM_CLASS(xori)
DEFINE_MI_BIN_CLASS(slt)
DEFINE_MI_IMM_CLASS(slti)
DEFINE_MI_BIN_CLASS(sltu)
DEFINE_MI_IMM_CLASS(sltiu)

// shifts
DEFINE_MI_IMM_CLASS(slli)
DEFINE_MI_BIN_CLASS(sll)
DEFINE_MI_IMM_CLASS(srli)
DEFINE_MI_BIN_CLASS(srl)
DEFINE_MI_IMM_CLASS(srai)
DEFINE_MI_BIN_CLASS(sra)

#define DEFINE_MI_LOAD_CLASS(NAME)                                             \
  class MI##NAME : public MInstruction {                                       \
  public:                                                                      \
    MGlobal *global = nullptr;                                                 \
    int imm;                                                                   \
                                                                               \
  public:                                                                      \
    MI##NAME(MGlobal *global);                                                 \
    MI##NAME(MGlobal *global, std::string name);                               \
    MI##NAME(MGlobal *global, Register *target);                               \
    MI##NAME(Register *addr, int offset);                                      \
    MI##NAME(Register *addr, int offset, std::string name);                    \
    MI##NAME(Register *addr, int offset, Register *target);                    \
    MGlobal *getGlobal();                                                      \
    ostream &printASM(ostream &stream) override;                               \
  };

#define DEFINE_MI_STORE_CLASS(NAME)                                            \
  class MI##NAME : public MInstruction {                                       \
  public:                                                                      \
    MGlobal *global = nullptr;                                                 \
    int imm;                                                                   \
                                                                               \
  public:                                                                      \
    MI##NAME(Register *val, MGlobal *global);                                  \
    MI##NAME(Register *val, int offset, Register *addr);                       \
    MGlobal *getGlobal();                                                      \
    ostream &printASM(ostream &stream) override;                               \
  };

DEFINE_MI_LOAD_CLASS(lw)
DEFINE_MI_STORE_CLASS(sw)
DEFINE_MI_LOAD_CLASS(ld)
DEFINE_MI_STORE_CLASS(sd)
DEFINE_MI_LOAD_CLASS(flw)
DEFINE_MI_STORE_CLASS(fsw)
DEFINE_MI_LOAD_CLASS(fld)
DEFINE_MI_STORE_CLASS(fsd)

DEFINE_MI_BIN_CLASS(mul);
DEFINE_MI_BIN_CLASS(mulw);
DEFINE_MI_BIN_CLASS(divw);
DEFINE_MI_BIN_CLASS(remw);

DEFINE_MI_BIN_CLASS(fadd_s);
DEFINE_MI_BIN_CLASS(fsub_s);
DEFINE_MI_BIN_CLASS(fmul_s);
DEFINE_MI_BIN_CLASS(fdiv_s);

DEFINE_MIN_UNA_CLASS(fmvs_x);

DEFINE_MIN_UNA_CLASS(fcvts_w);
DEFINE_MIN_UNA_CLASS(fcvtw_s);

DEFINE_MI_BIN_CLASS(feq_s);
DEFINE_MI_BIN_CLASS(flt_s);
DEFINE_MI_BIN_CLASS(fle_s);

#define DEFINE_BRANCH_CLASS(NAME)                                              \
  class MI##NAME : public MInstruction {                                       \
  private:                                                                     \
    MBasicBlock *targetBB;                                                     \
                                                                               \
  public:                                                                      \
    MI##NAME(Register *reg1, Register *reg2, MBasicBlock *targetBB);           \
    MBasicBlock *getTargetBB();                                                \
    void setTargetBB(MBasicBlock *bb);                                         \
    ostream &printASM(ostream &stream) override;                               \
  };

DEFINE_BRANCH_CLASS(beq);
DEFINE_BRANCH_CLASS(bne);
DEFINE_BRANCH_CLASS(bge);
DEFINE_BRANCH_CLASS(blt);

class MIj : public MInstruction { // presudo
private:
  MBasicBlock *targetBB;

public:
  MIj(MBasicBlock *targetBB);
  void setTargetBB(MBasicBlock *bb);
  MBasicBlock *getTargetBB();
  ostream &printASM(ostream &stream) override;
};

class MIcall : public MInstruction { // presudo
private:
  MFunction *func;

public:
  MIcall(MFunction *func);
  MFunction *getFunc();
  ostream &printASM(ostream &stream) override;
};

class MIret : public MInstruction { // presudo
public:
  MIret();
  ostream &printASM(ostream &stream) override;
};

class MIli : public MInstruction { // presudo
public:
  int imm;
  MIli(int32_t imm);
  MIli(int32_t imm, string name);
  MIli(int32_t imm, Register *target);
  ostream &printASM(ostream &stream) override;
};

class MIla : public MInstruction { // presudo
private:
  MGlobal *g;

public:
  MIla(MGlobal *g);
  MIla(MGlobal *g, string name);
  MIla(MGlobal *g, Register *target);
  ostream &printASM(ostream &stream) override;
};

DEFINE_MIN_UNA_CLASS(mv)
DEFINE_MIN_UNA_CLASS(fmv_s)

class Mcomment : public MInstruction {
private:
  string comment;

public:
  Mcomment(string comment);
  ostream &printASM(ostream &stream) override;
};
