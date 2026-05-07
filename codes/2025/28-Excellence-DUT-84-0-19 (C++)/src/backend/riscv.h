#pragma once

#include "IR.h"
#include "string.h"
#include <string>
#include <cassert>

class RiscvLabel;
class RiscvBasicBlock;
class RiscvInstr;
class RegAlloca;

IRType *findPtrType(IRType *ty);

class Register { //寄存器类型
public:
  Register() = default;
  ~Register() = default;
  enum RegType {
    Int = 1, Float = 2, Stack = 3, Zero = 4    
  };
  RegType regtype_;
  int rid_; 
  Register(RegType regtype, int rid) : regtype_(regtype), rid_(rid) {}
  std::string print() {
    using std::to_string;
    // 处理浮点寄存器的名称映射
    if (this->regtype_ == Float) {
        if (this->rid_ >= 0 && this->rid_ <= 7) 
            return "ft" + std::to_string(this->rid_);
        else if (this->rid_ >= 8 && this->rid_ <= 9) 
            return "fs" + std::to_string(this->rid_ - 8);
        else if (this->rid_ >= 10 && this->rid_ <= 17) 
            return "fa" + std::to_string(this->rid_ - 10);
        else if (this->rid_ >= 18 && this->rid_ <= 27) 
            return "fs" + std::to_string(this->rid_ - 18 + 2);
        else if (this->rid_ >= 28 && this->rid_ <= 31) 
            return "ft" + std::to_string(this->rid_ - 28 + 8);
        else 
            return "wtf";
    }
    if (this->rid_ == 0) {
        return "zero";
    } else if (this->rid_ == 1) {
        return "ra";
    } else if (this->rid_ == 2) {
        return "sp";
    } else if (this->rid_ == 3) {
        return "gp";
    } else if (this->rid_ == 4) {
        return "tp";
    } else if (this->rid_ >= 5 && this->rid_ <= 7) {
        return "t" + to_string(this->rid_ - 5);
    } else if (this->rid_ == 8) {
        return "fp";
    } else if (this->rid_ == 9) {
        return "s1";
    }
    if (this->rid_ >= 10 && this->rid_ <= 17)
      return "a" + to_string(this->rid_ - 10);
    if (this->rid_ >= 18 && this->rid_ <= 27)
      return "s" + to_string(this->rid_ - 16);
    return "t" + to_string(this->rid_ - 25);
  }
};

class RiscvOperand {
public:
  enum OpTy {
    Void = 0, IntImm = 1,FloatImm = 2,IntReg = 3, FloatReg = 4, IntMem = 5, FloatMem = 6, Function = 7, Block = 8    
  };
  OpTy tid_;
  explicit RiscvOperand(OpTy tid) : tid_(tid) {}
  ~RiscvOperand() = default;
  virtual std::string print() = 0;
  OpTy getType();
  bool isRegister();
};
extern const int REG_NUMBER;
const int VARIABLE_ALIGN_BYTE = 8;

// 常数
class RiscvConst : public RiscvOperand {
public:
  int intval;
  float floatval;
  RiscvConst() = default;
  explicit RiscvConst(int val) : RiscvOperand(IntImm), intval(val) {}
  explicit RiscvConst(float val) : RiscvOperand(FloatImm), floatval(val) {}
  std::string print() {
    if (this->tid_ == IntImm)
      return std::to_string(intval);
    else
      return std::to_string(floatval);
  }
};

// 整型寄存器直接存储
class RiscvIntReg : public RiscvOperand {
public:
  Register *reg_;
  RiscvIntReg(Register *reg) : RiscvOperand(IntReg), reg_(reg) {
    assert(reg_->regtype_ == Register::Int); // 判断整型寄存器存储
  }
  std::string print() { return reg_->print(); }
};

//浮点型寄存器直接存储
class RiscvFloatReg : public RiscvOperand {
public:
  Register *reg_;
  RiscvFloatReg(Register *reg) : RiscvOperand(FloatReg), reg_(reg) {
    assert(reg_->regtype_ == Register::Float); // 判断整型寄存器存储
  }
  std::string print() { return reg_->print(); }
};

// 需间接寻址得到的整型
class RiscvIntPhiReg : public RiscvOperand {
public:
  int shift_;
  int isGlobalVariable;
  Register *base_;
  std::string MemBaseName;
  RiscvIntPhiReg(Register *base, int shift = 0, int isGVar = false)
      : RiscvOperand(IntMem), base_(base), shift_(shift),
        isGlobalVariable(isGVar), MemBaseName(base_->print()) {}
  // 内存以全局形式存在的变量（常量）
  RiscvIntPhiReg(std::string s, int shift = 0, int isGVar = false)
      : RiscvOperand(IntMem), base_(nullptr), shift_(shift), MemBaseName(s),
        isGlobalVariable(isGVar) {}
  std::string print() {
    std::string ans = "";
    if (base_ != nullptr)
      ans += "(" + base_->print() + ")";
    else {
      if (isGlobalVariable)
        return MemBaseName; 
      else
        ans += "(" + MemBaseName + ")";
    }
    if (shift_)
      ans = std::to_string(shift_) + ans;
    return ans;
  }
  bool overflow() { return std::abs(shift_) >= 1024; }
};


IRType *getStoreTypeFromRegType(RiscvOperand *riscvReg);

// 需间接寻址得到的浮点型
class RiscvFloatPhiReg : public RiscvOperand {

public:
  int shift_;
  Register *base_;
  std::string MemBaseName;
  int isGlobalVariable;
  RiscvFloatPhiReg(Register *base, int shift = 0, int isGVar = false)
      : RiscvOperand(FloatMem), base_(base), shift_(shift),
        isGlobalVariable(isGVar), MemBaseName(base_->print()) {}
        
  RiscvFloatPhiReg(std::string s, int shift = 0, int isGVar = false)
      : RiscvOperand(FloatMem), base_(nullptr), shift_(shift), MemBaseName(s),
        isGlobalVariable(isGVar) {}
  std::string print() {
    std::string ans = "";
    if (base_ != nullptr)
      ans += "(" + base_->print() + ")";
    else {
      if (isGlobalVariable)
        return MemBaseName; 
      else
        ans += "(" + MemBaseName + ")";
    }
    if (shift_)
      ans = std::to_string(shift_) + ans;
    return ans;
  }

  bool overflow() { return std::abs(shift_) >= 1024; }
};

class RiscvLabel : public RiscvOperand {
public:
  std::string name_; // 标号名称
  ~RiscvLabel() = default;
  RiscvLabel(OpTy Type, std::string name) : RiscvOperand(Type), name_(name) {
  }
  virtual std::string print() = 0;
};

class RiscvGlobalVariable : public RiscvLabel {
public:
  bool isConst_;
  bool isData; 
  int elementNum_;
  Constant *initValue_;
  
  RiscvGlobalVariable(OpTy Type, std::string name, bool isConst,
                      Constant *initValue)
      : RiscvLabel(Type, name), isConst_(isConst), initValue_(initValue),
        elementNum_(1) {}
  
  RiscvGlobalVariable(OpTy Type, std::string name, bool isConst,
                      Constant *initValue, int elementNum)
      : RiscvLabel(Type, name), isConst_(isConst), initValue_(initValue),
        elementNum_(elementNum) {}

  std::string print();
  std::string print(bool print_name, Constant *initVal);
};

class RiscvFunction : public RiscvLabel { //RISCV函数类
public:
  RegAlloca *regAlloca; //寄存器
  int num_args_; //参数个数
  OpTy resType_; //类型
  std::vector<RiscvOperand *> args; //函数参数
  RiscvFunction(std::string name, int num_args,
                OpTy Ty); 
  void setArgs(int ind, RiscvOperand *op) {
    assert(ind >= 0 && ind < args.size());
    args[ind] = op;
  }
  void deleteArgs(int ind) {
    assert(ind >= 0 && ind < args.size());
    args[ind] = nullptr;
  }
  ~RiscvFunction() = default;
  std::string printname() { return name_; }
  std::vector<RiscvBasicBlock *> blk;
  bool is_libfunc() {
    if (name_ == "putint" || name_ == "putch" || name_ == "putarray" || name_ == "_sysy_starttime" || name_ == "_sysy_stoptime" || name_ == "__aeabi_memclr4" || name_ == "__aeabi_memset4" ||
        name_ == "__aeabi_memcpy4" || name_ == "getint" || name_ == "getch" || name_ == "getarray" || name_ == "getfloat" || name_ == "getfarray" || name_ == "putfloat" || name_ == "putfarray" ||
        name_ == "llvm.memset.p0.i32") {
      return true;
    } else
      return false;
  }
  std::map<RiscvOperand *, int>
      argsOffset; 
  void addArgs(RiscvOperand *val) { 
    if (argsOffset.count(val) == 0) {
      argsOffset[val] = base_;
      base_ -= VARIABLE_ALIGN_BYTE;
    }
  }
  int querySP() { return base_; }
  void setSP(int SP) { base_ = SP; }
  void addTempVar(RiscvOperand *val) {
    addArgs(val);
    tempRange += VARIABLE_ALIGN_BYTE;
  }
  void shiftSP(int shift_value) { base_ += shift_value; }
  void storeArray(int elementNum) {
    if(elementNum & 7) {
      elementNum += 8 - (elementNum & 7);   
    }
    base_ -= elementNum;
  }

  void ChangeBlock(RiscvBasicBlock *bb, int ind) {
    blk[ind] = bb;
  }
  void addBlock(RiscvBasicBlock *bb) { blk.push_back(bb); } //添加基本块
  std::string
  print();
private:
  int base_;
  int tempRange; 
  std::map<RiscvOperand *, int>
      storedEnvironment; 
};

class RiscvModule { 
public:
  std::vector<RiscvFunction *> func_; //当前模块RISCV函数列表
  std::vector<RiscvGlobalVariable *> globalVariable_; //当前模块RISCV全局常量、变量列表
  void addFunction(RiscvFunction *foo) { func_.push_back(foo); }
  void addGlobalVariable(RiscvGlobalVariable *g) {
    globalVariable_.push_back(g);
  }
};

RiscvFunction *createSyslibFunc(::Function *foo);

template <typename T>
class DSU
{
private:
  std::map<T, T> father;
  T getfather(T x)
  {
    return father[x] == x ? x : (father[x] = getfather(father[x]));
  }

public:
  DSU() = default;
  T query(T id)
  {
    if (father.find(id) == father.end())
    {
      return father[id] = id;
    }
    else
    {
      return getfather(id);
    }
  }

  void merge(T u, T v)
  {
    u = query(u), v = query(v);
    assert(u != nullptr && v != nullptr);
    if (u == v)
      return;
    father[u] = v;
  }
};

Register *NamefindReg(std::string reg);

class RegAlloca //寄存器
{
public:
  DSU<Value *> DSU_for_Variable;

  RiscvOperand *findReg(Value *val, RiscvBasicBlock *bb,RiscvInstr *instr = nullptr, int inReg = 0,int load = 1, RiscvOperand *specified = nullptr,bool direct = true);

  RiscvOperand *findMem(Value *val, RiscvBasicBlock *bb, RiscvInstr *instr,bool direct);

  RiscvOperand *findMem(Value *val);

  RiscvOperand *findNonuse(IRType *ty, RiscvBasicBlock *bb, RiscvInstr *instr = nullptr);

  RiscvOperand *findSpecificReg(Value *val, std::string RegName, RiscvBasicBlock *bb, RiscvInstr *instr = nullptr, bool direct = true);

  void setPosition(Value *val, RiscvOperand *riscvVal);

  void setPositionReg(Value *val, RiscvOperand *riscvReg, RiscvBasicBlock *bb, RiscvInstr *instr = nullptr);

  void setPositionReg(Value *val, RiscvOperand *riscvReg);

  void setPointerPos(Value *val, RiscvOperand *PointerMem);

  RiscvInstr *writeback(RiscvOperand *riscvReg, RiscvBasicBlock *bb, RiscvInstr *instr = nullptr);

  RiscvInstr *writeback(Value *val, RiscvBasicBlock *bb,RiscvInstr *instr = nullptr);

  Value *getRegPosition(RiscvOperand *reg);

  RiscvOperand *getPositionReg(Value *val);

  std::vector<RiscvOperand *> savedRegister;

  RegAlloca();

  std::map<Value *, RiscvOperand *> ptrPos;

  RiscvOperand *findPtr(Value *val, RiscvBasicBlock *bb, RiscvInstr *instr = nullptr);

  void writeback_all(RiscvBasicBlock *bb, RiscvInstr *instr = nullptr);

  void clear();

  std::set<RiscvOperand *> getUsedReg() { return regUsed; }

private:
  std::map<Value *, RiscvOperand *> pos, curReg;
  std::map<RiscvOperand *, Value *> regPos;
  std::map<RiscvOperand *, int> regFindTimeStamp;
  int safeFindTimeStamp = 0;
  static const int SAFE_FIND_LIMIT = 3;
  std::set<RiscvOperand *> regUsed;
};

static std::vector<RiscvOperand *> regPool;

RiscvOperand *getRegOperand(std::string reg);

RiscvOperand *getRegOperand(Register::RegType op_ty_, int id);

class RiscvBasicBlock : public RiscvLabel {
public:
  RiscvFunction *func_;
  std::vector<RiscvInstr *> instruction;
  int blockInd_; 
  RiscvBasicBlock(std::string name, RiscvFunction *func, int blockInd)
      : RiscvLabel(Block, name), func_(func), blockInd_(blockInd) {
    func->addBlock(this);
  }
  RiscvBasicBlock(std::string name, int blockInd)
      : RiscvLabel(Block, name), func_(nullptr), blockInd_(blockInd) {}
  void addFunction(RiscvFunction *func) { func->addBlock(this); }
  std::string printname() { return name_; }
  void deleteInstr(RiscvInstr *instr) {
    auto it = std::find(instruction.begin(), instruction.end(), instr);
    if (it != instruction.end())
      instruction.erase(
          std::remove(instruction.begin(), instruction.end(), instr),
          instruction.end());
  }
  void replaceInstr(RiscvInstr *oldinst, RiscvInstr *newinst) {}
  void addInstrBack(RiscvInstr *instr) {
    if (instr == nullptr)
      return;
    instruction.push_back(instr);
  }
  void addInstrFront(RiscvInstr *instr) {
    if (instr == nullptr)
      return;
    instruction.insert(instruction.begin(), instr);
  }
  void addInstrBefore(RiscvInstr *instr, RiscvInstr *dst) {
    if (instr == nullptr)
      return;
    auto it = std::find(instruction.begin(), instruction.end(), dst);
    if (it != instruction.end())
      instruction.insert(it, instr);
    else
      addInstrBack(instr);
  }
  void addInstrAfter(RiscvInstr *instr, RiscvInstr *dst) {
    if (instr == nullptr)
      return;
    auto it = std::find(instruction.begin(), instruction.end(), dst);
    if (it != instruction.end()) {
      if (next(it) == instruction.end())
        instruction.push_back(instr);
      else
        instruction.insert(next(it), instr);
    } else {
      addInstrBack(instr);
    }
  }
  std::string print();
};


class RiscvInstr {
public:
  enum InstrType {
    ADD = 0,ADDI,SUB,SUBI,MUL,DIV = 6,REM,FADD = 8,FSUB = 10,FMUL = 12,FDIV = 14,XOR = 16,XORI,AND,
    ANDI,OR,ORI,SW,LW,FSW = 30,FLW,ICMP,FCMP,PUSH,POP,CALL,RET,LI,MOV,FMV,FPTOSI,SITOFP,JMP,SHL,LSHR,
    ASHR,SHLI = 52,LSHRI,ASHRI,LA,ADDIW,BGT
  };
  const static std::map<InstrType, std::string> RiscvName;

  InstrType type_;
  RiscvBasicBlock *parent_;
  std::vector<RiscvOperand *> operand_;

  RiscvInstr(InstrType type, int op_nums);
  RiscvInstr(InstrType type, int op_nums, RiscvBasicBlock *bb);
  ~RiscvInstr() = default;

  virtual std::string print() = 0;

  RiscvOperand *result_;
  void setOperand(int ind, RiscvOperand *val) {
    assert(ind >= 0 && ind < operand_.size());
    operand_[ind] = val;
  }
  void setResult(RiscvOperand *result) { result_ = result; }
  void removeResult() { result_ = nullptr; }
  void removeOperand(int i) { operand_[i] = nullptr; }
  void clear() { operand_.clear(), removeResult(); }
  std::vector<RiscvOperand *> getOperandResult() {
    std::vector<RiscvOperand *> ans(operand_);
    if (result_ != nullptr)
      ans.push_back(result_);
    return ans;
  }
  RiscvOperand *getOperand(int i) const { return operand_[i]; }
};

class BinaryRiscvInst : public RiscvInstr {
public:
  std::string print() override;
  BinaryRiscvInst() = default;
  BinaryRiscvInst(InstrType op, RiscvOperand *v1, RiscvOperand *v2,
                  RiscvOperand *target, RiscvBasicBlock *bb, bool flag = 0)
      : RiscvInstr(op, 2, bb), word(flag) {
    setOperand(0, v1);
    setOperand(1, v2);
    setResult(target);
    this->parent_ = bb;
    if(v2->getType() == v2->IntImm && static_cast<RiscvConst*>(v2)->intval == 0){
      type_ = ADD;
      setOperand(1, getRegOperand("zero"));
    }
  }
  bool word;
};

class UnaryRiscvInst : public RiscvInstr {
public:
  std::string print() override;
  UnaryRiscvInst() = default;
  UnaryRiscvInst(InstrType op, RiscvOperand *v1, RiscvOperand *target,
                 RiscvBasicBlock *bb, bool flag = 0)
      : RiscvInstr(op, 1, bb) {
    setOperand(0, v1);
    setResult(target);
    this->parent_ = bb;
    if (flag)
      this->parent_->addInstrBack(this);
  }
};

class MoveRiscvInst : public RiscvInstr {
public:
  std::string print() override;
  MoveRiscvInst() = default;
  MoveRiscvInst(RiscvOperand *v1, int Imm, RiscvBasicBlock *bb, bool flag = 0)
      : RiscvInstr(InstrType::LI, 2, bb) {
    RiscvOperand *Const = new RiscvConst(Imm);
    setOperand(0, v1);
    setOperand(1, Const);
    this->parent_ = bb;
    if (flag)
      this->parent_->addInstrBack(this);
  }
  MoveRiscvInst(RiscvOperand *v1, RiscvOperand *v2, RiscvBasicBlock *bb,
                bool flag = 0)
      : RiscvInstr(InstrType::MOV, 2, bb) {
    setOperand(0, v1);
    setOperand(1, v2);
    this->parent_ = bb;
    if (flag)
      this->parent_->addInstrBack(this);
  }
};

class PushRiscvInst : public RiscvInstr {
  int basicShift_;

public:
  PushRiscvInst(std::vector<RiscvOperand *> &lists, RiscvBasicBlock *bb,
                int basicShift)
      : RiscvInstr(InstrType::PUSH, lists.size(), bb), basicShift_(basicShift) {
    for (int i = 0; i < lists.size(); i++)
      setOperand(i, lists[i]);
  }
  std::string print() override;
};

class PopRiscvInst : public RiscvInstr {
  int basicShift_;

public:
  PopRiscvInst(std::vector<RiscvOperand *> &lists, RiscvBasicBlock *bb,
               int basicShift)
      : RiscvInstr(InstrType::POP, lists.size(), bb), basicShift_(basicShift) {
    for (int i = 0; i < lists.size(); i++)
      setOperand(i, lists[i]);
  }
  std::string print() override;
};

class CallRiscvInst : public RiscvInstr {
public:
  CallRiscvInst(RiscvFunction *func, RiscvBasicBlock *bb)
      : RiscvInstr(InstrType::CALL, 1, bb) {
    setOperand(0, func);
  }
  virtual std::string print() override;
};

class ReturnRiscvInst : public RiscvInstr {
public:
  ReturnRiscvInst(RiscvBasicBlock *bb) : RiscvInstr(InstrType::RET, 0, bb) {}
  std::string print() override;
};

class StoreRiscvInst : public RiscvInstr {

public:
  int shift_; 
  IRType type;  
  StoreRiscvInst(IRType *ty, RiscvOperand *source, RiscvOperand *target,
                 RiscvBasicBlock *bb, int shift = 0)
      : RiscvInstr(InstrType::SW, 2, bb), shift_(shift), type(ty->tid_) {
    setOperand(0, source);
    setOperand(1, target);
    this->parent_ = bb;
    if (source->isRegister() == false) {
      std::cerr << "[Fatal error] Invalid store instruction: " << print()
                << std::endl;
      std::terminate();
    }
  }
  std::string print() override;
};

class LoadRiscvInst : public RiscvInstr {
public:
  int shift_; 
  IRType type;  
  LoadRiscvInst(IRType *ty, RiscvOperand *dest, RiscvOperand *target,
                RiscvBasicBlock *bb, int shift = 0)
      : RiscvInstr(InstrType::LW, 2, bb), shift_(shift), type(ty->tid_) {
    setOperand(0, dest);
    setOperand(1, target);
    if (target == nullptr) {
      std::cerr << "[Fatal Error] Load Instruction's target is nullptr."
                << std::endl;
      std::terminate();
    }
    this->parent_ = bb;
  }
  std::string print() override;
};

class ICmpRiscvInstr : public RiscvInstr {
public:
  static const std::map<ICmpInst::ICmpOp, std::string> ICmpOpName;
  static const std::map<ICmpInst::ICmpOp, std::string> ICmpOpSName;
  static const std::map<ICmpInst::ICmpOp, ICmpInst::ICmpOp> ICmpOpEquiv;
  ICmpRiscvInstr(ICmpInst::ICmpOp op, RiscvOperand *v1, RiscvOperand *v2, RiscvBasicBlock *trueLink, RiscvBasicBlock *falseLink,  RiscvBasicBlock *bb)
      : RiscvInstr(ICMP, 4, bb), icmp_op_(op) {
    setOperand(0, v1);
    setOperand(1, v2);
    setOperand(2, trueLink);
    setOperand(3, falseLink);
  }
  ICmpRiscvInstr(ICmpInst::ICmpOp op, RiscvOperand *v1, RiscvOperand *v2,
                 RiscvBasicBlock *trueLink, RiscvBasicBlock *bb)
      : RiscvInstr(ICMP, 4, bb), icmp_op_(op) {
    setOperand(0, v1);
    setOperand(1, v2);
    setOperand(2, trueLink);
    setOperand(3, nullptr);
  }
  ICmpInst::ICmpOp icmp_op_;
  std::string print() override;
};

class ICmpSRiscvInstr : public ICmpRiscvInstr {
public:
  ICmpSRiscvInstr(ICmpInst::ICmpOp op, RiscvOperand *v1, RiscvOperand *v2, RiscvOperand *target, RiscvBasicBlock *bb)
      : ICmpRiscvInstr(op, v1, v2, nullptr, bb) {
    setOperand(0, v1);
    setOperand(1, v2);
    setResult(target);
  }
  std::string print() override;
};

class FCmpRiscvInstr : public RiscvInstr {
public:
  static const std::map<FCmpInst::FCmpOp, std::string> FCmpOpName;
  static const std::map<FCmpInst::FCmpOp, FCmpInst::FCmpOp> FCmpOpEquiv;
  FCmpRiscvInstr(FCmpInst::FCmpOp op, RiscvOperand *v1, RiscvOperand *v2, RiscvOperand *target, RiscvBasicBlock *bb)
      : RiscvInstr(FCMP, 2, bb), fcmp_op_(op) {
    setOperand(0, v1);
    setOperand(1, v2);
    setResult(target);
  }
  FCmpInst::FCmpOp fcmp_op_;
  std::string print() override;
};
class FpToSiRiscvInstr : public RiscvInstr {
public:
  FpToSiRiscvInstr(RiscvOperand *Source, RiscvOperand *Target, RiscvBasicBlock *bb)
      : RiscvInstr(FPTOSI, 2, bb) {
    setOperand(0, Source);
    setOperand(1, Target);
  }
  virtual std::string print() override;
};

class SiToFpRiscvInstr : public RiscvInstr {
public:
  SiToFpRiscvInstr(RiscvOperand *Source, RiscvOperand *Target,  RiscvBasicBlock *bb)
      : RiscvInstr(SITOFP, 2, bb) {
    setOperand(0, Source);
    setOperand(1, Target);
  }
  virtual std::string print() override;
};

class LoadAddressRiscvInstr : public RiscvInstr {
public:
  std::string name_;
  LoadAddressRiscvInstr(RiscvOperand *dest, std::string name, RiscvBasicBlock *bb)
      : RiscvInstr(LA, 1, bb), name_(name) {
    setOperand(0, dest);
  }
  virtual std::string print() override;
};

class BranchRiscvInstr : public RiscvInstr {
public:

  BranchRiscvInstr(RiscvOperand *rs1, RiscvBasicBlock *trueLink, RiscvBasicBlock *falseLink, RiscvBasicBlock *bb)
      : RiscvInstr(BGT, 3, bb) {
    setOperand(0, rs1);
    setOperand(1, trueLink);
    setOperand(2, falseLink);
  }
  virtual std::string print() override;
};