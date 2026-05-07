#include "riscv.h"
#include "RVbuilder.h"
#include "IR.h"
#include <sstream>

const int REG_NUMBER = 32;
extern int LabelCount;
int IntRegID = 32, FloatRegID = 32; 


RiscvFunction::RiscvFunction(std::string name, int num_args,OpTy Ty) 
    : RiscvLabel(Function, name), num_args_(num_args), resType_(Ty), base_(-VARIABLE_ALIGN_BYTE) {
  regAlloca = new RegAlloca();
}

std::string RiscvGlobalVariable::print() { return print(true, nullptr); }

RiscvOperand::OpTy RiscvOperand::getType() { return tid_; }

bool RiscvOperand::isRegister() { return tid_ == FloatReg || tid_ == IntReg; }

std::string RiscvFunction::print() {
  std::string riscvInstr = ".global " + this->name_ + "\n" + this->name_ + ":\n"; 
  for (auto x : this->blk)
    riscvInstr += x->print();
  return riscvInstr;
}

std::string RiscvBasicBlock::print() {
  std::string riscvInstr = this->name_ + ":\n";
  for (auto x : this->instruction)
    riscvInstr += x->print();
  return riscvInstr;
}

IRType *findPtrType(IRType *ty) {
  while (ty->tid_ == IRType::PointerTyID) {
    ty = static_cast<PointerIRType *>(ty)->contained_;
  }
  while (ty->tid_ == IRType::ArrayTyID) {
    ty = static_cast<ArrayIRType *>(ty)->contained_;
  }
  assert(ty->tid_ == IRType::IntegerTyID || ty->tid_ == IRType::FloatTyID);
  return ty;
}

std::string RiscvGlobalVariable::print(bool print_name, Constant *initVal) {
  std::string code = "";

  if (print_name) {
    code += this->name_ + ":\n";
    initVal = initValue_;
  }

  if (initVal == nullptr){
    code += "\t.zero\t" + std::to_string(this->elementNum_ * 4) + "\n";
    return code;
  }
  
  if (dynamic_cast<ConstantZero *>(initVal) != nullptr) {
    code += "\t.zero\t" + std::to_string(calcTypeSize(initVal->IRType_)) + "\n";
    return code;
  }
  // 整型
  if (initVal->IRType_->tid_ == IRType::IRTypeID::IntegerTyID) {
    code += "\t.word\t" + std::to_string(dynamic_cast<ConstantInt *>(initVal)->value_) + "\n";
    return code;
  }
  // 浮点
  else if (initVal->IRType_->tid_ == IRType::IRTypeID::FloatTyID) {
    std::string valString = dynamic_cast<ConstantFloat *>(initVal)->print32();
    while (valString.length() < 10)
      valString += "0";
    code += "\t.word\t" + valString.substr(0, 10) + "\n";
    return code;
  }
  else if (initVal->IRType_->tid_ == IRType::IRTypeID::ArrayTyID) {
    ConstantArray *const_arr = dynamic_cast<ConstantArray *>(initVal);
    assert(const_arr != nullptr);
    int zeroSpace = calcTypeSize(initVal->IRType_);
    for (auto elements : const_arr->const_array) {
      code += print(false, elements);
      zeroSpace -= 4;
    }
    if (zeroSpace)
      code += "\t.zero\t" + std::to_string(zeroSpace) + "\n";
    return code;  
  } else {
    std::terminate();
  }
}

RiscvFunction *createSyslibFunc(Function *foo) { //创建库函数初始化
  if (foo->name_ == "__aeabi_memclr4") {
    auto *rfoo = createRiscvFunction(foo);
    auto *bb1 = createRiscvBasicBlock();
    bb1->addInstrBack(new MoveRiscvInst(getRegOperand("t5"), getRegOperand("a0"), bb1));
    bb1->addInstrBack(new MoveRiscvInst(getRegOperand("t6"), getRegOperand("a1"), bb1));
    bb1->addInstrBack(new BinaryRiscvInst(RiscvInstr::ADD, getRegOperand("a0"),getRegOperand("t6"),getRegOperand("t6"), bb1));
    bb1->addInstrBack(new MoveRiscvInst(getRegOperand("a0"), new RiscvConst(0), bb1));
    auto *bb2 = createRiscvBasicBlock();
    bb2->addInstrBack(new StoreRiscvInst(new IRType(IRType::IRTypeID::IntegerTyID), getRegOperand("zero"),new RiscvIntPhiReg(NamefindReg("t5")), bb2));
    bb2->addInstrBack(new BinaryRiscvInst(RiscvInstr::ADDI, getRegOperand("t5"),new RiscvConst(4),getRegOperand("t5"), bb1));
    bb2->addInstrBack(new ICmpRiscvInstr(ICmpInst::ICMP_SLT,getRegOperand("t5"),getRegOperand("t6"), bb2, bb2));
    bb2->addInstrBack(new ReturnRiscvInst(bb2));
    rfoo->addBlock(bb1);
    rfoo->addBlock(bb2);
    return rfoo;
  }
  return nullptr;
}

Register *NamefindReg(std::string reg) {
  if (reg.size() > 4)
    return nullptr;

  Register *reg_to_ret = new Register(Register::Int, 0);


  for (int i = 0; i < 32; i++) {
    reg_to_ret->rid_ = i;
    if (reg_to_ret->print() == reg)
      return reg_to_ret;
  }

  reg_to_ret->regtype_ = reg_to_ret->Float;
  for (int i = 0; i < 32; i++) {
    reg_to_ret->rid_ = i;
    if (reg_to_ret->print() == reg)
      return reg_to_ret;
  }

  return nullptr;
}

RiscvOperand *getRegOperand(std::string reg) {
  for (auto regope : regPool) {
    if (regope->print() == reg)
      return regope;
  }
  assert(false);
  return nullptr;
}

RiscvOperand *getRegOperand(Register::RegType op_ty_, int id) {
  Register *reg = new Register(op_ty_, id);
  for (auto regope : regPool) {
    if (regope->print() == reg->print()) {
      delete reg;
      return regope;
    }
  }
  assert(false);
  return nullptr;
}

IRType *getStoreTypeFromRegType(RiscvOperand *riscvReg) {
  return riscvReg->getType() == RiscvOperand::OpTy::FloatReg
             ? new IRType(IRType::IRTypeID::FloatTyID)
             : new IRType(IRType::IRTypeID::IntegerTyID);
}

RiscvOperand *RegAlloca::findReg(Value *val, RiscvBasicBlock *bb, RiscvInstr *instr, int inReg, int load, RiscvOperand *specified, bool direct) {
  safeFindTimeStamp++;
  val = this->DSU_for_Variable.query(val);
  bool isGVar = dynamic_cast<GlobalVariable *>(val) != nullptr;
  bool isAlloca = dynamic_cast<AllocaInst *>(val) != nullptr;
  bool isPointer = val->IRType_->tid_ == val->IRType_->PointerTyID;

  if (specified != nullptr)
    setPositionReg(val, specified, bb, instr);
  else if (curReg.find(val) == curReg.end() || isAlloca ||
           val->is_constant()) { 
    bool found = false;
    RiscvOperand *cur = nullptr;
    IntRegID = 32;
    FloatRegID = 32;
    while (!found) {
      if (val->IRType_->tid_ != IRType::FloatTyID) {
        ++IntRegID;
        if (IntRegID > 27)
          IntRegID = 18;

        cur = getRegOperand(Register::Int, IntRegID);
      } else {
        ++FloatRegID;
        if (FloatRegID > 27)
          FloatRegID = 18;
        cur = getRegOperand(Register::Float, FloatRegID);
      }
      if (regFindTimeStamp.find(cur) == regFindTimeStamp.end() ||
          safeFindTimeStamp - regFindTimeStamp[cur] > SAFE_FIND_LIMIT) {
        setPositionReg(val, cur, bb, instr);
        found = true;
      }
    }
  } else {
    regFindTimeStamp[curReg[val]] = safeFindTimeStamp;
    return curReg[val];
  }


  auto mem_addr = findMem(val, bb, instr, 1); 
  auto current_reg = curReg[val];            
  auto load_type = val->IRType_;

  regFindTimeStamp[current_reg] = safeFindTimeStamp;
  if (load) {
    if (mem_addr != nullptr) {
      bb->addInstrBefore(
          new LoadRiscvInst(load_type, current_reg, mem_addr, bb), instr);
    } else if (val->is_constant()) {
      auto cval = dynamic_cast<ConstantInt *>(val);
      if (cval != nullptr)
        bb->addInstrBefore(new MoveRiscvInst(current_reg, cval->value_, bb),
                           instr);
      else if (dynamic_cast<ConstantFloat *>(val) != nullptr)
        bb->addInstrBefore(
            new MoveRiscvInst(current_reg, this->findMem(val), bb), instr);
      else {
        std::cerr << "[Warning] Trying to find a register for unknown type of "
                     "constant value which is not implemented for now."
                  << std::endl;
      }
    } else if (isAlloca) {
      bb->addInstrBefore(
          new BinaryRiscvInst(
              BinaryRiscvInst::ADDI, getRegOperand("fp"),
              new RiscvConst(static_cast<RiscvIntPhiReg *>(pos[val])->shift_),
              current_reg, bb),
          instr);
    } else {
      std::terminate();
    }
  }

  return current_reg;
}

RiscvOperand *RegAlloca::findMem(Value *val, RiscvBasicBlock *bb,
                                 RiscvInstr *instr, bool direct) {
  val = this->DSU_for_Variable.query(val);
  if (pos.count(val) == 0 && !val->is_constant()) {
    std::cerr << "[Warning] Value " << std::hex << val << " (" << val->name_
              << ")'s memory map not found." << std::endl;
  }
  bool isGVar = dynamic_cast<GlobalVariable *>(val) != nullptr;
  bool isPointer = val->IRType_->tid_ == val->IRType_->PointerTyID;
  bool isAlloca = dynamic_cast<AllocaInst *>(val) != nullptr;
  isGVar = isGVar || dynamic_cast<ConstantFloat *>(val) != nullptr;

  if (isGVar) {
    if (bb == nullptr) {
      return nullptr;
    }
    bb->addInstrBefore(
        new LoadAddressRiscvInstr(getRegOperand("t5"), pos[val]->print(), bb),
        instr);
    return new RiscvIntPhiReg("t5");
  }

  if (isPointer && !isAlloca && !direct) {
    if (bb == nullptr) {
      return nullptr;
    }

    bb->addInstrBefore(new LoadRiscvInst(new IRType(IRType::PointerTyID),
                                         getRegOperand("t4"), pos[val], bb),
                       instr);
    return new RiscvIntPhiReg("t4");
  }
  
  else if (direct && isAlloca)
    return nullptr;

  if (pos.find(val) == pos.end())
    return nullptr;

  return pos[val];
}

RiscvOperand *RegAlloca::findMem(Value *val) {
  return findMem(val, nullptr, nullptr, true);
}

RiscvOperand *RegAlloca::findNonuse(IRType *ty, RiscvBasicBlock *bb, RiscvInstr *instr) {
  if (ty->tid_ == IRType::IntegerTyID || ty->tid_ == IRType::PointerTyID) {
    ++IntRegID;
    if (IntRegID > 27)
      IntRegID = 18;
    return getRegOperand(Register::Int, IntRegID);
  } else {
    ++FloatRegID;
    if (FloatRegID > 27)
      FloatRegID = 18;
    return getRegOperand(Register::Float, FloatRegID);
  }
}

void RegAlloca::setPosition(Value *val, RiscvOperand *riscvVal) {
  val = this->DSU_for_Variable.query(val);
  pos[val] = riscvVal;
}

RiscvOperand *RegAlloca::findSpecificReg(Value *val, std::string RegName, RiscvBasicBlock *bb, RiscvInstr *instr, bool direct) {
  val = this->DSU_for_Variable.query(val);
  RiscvOperand *retOperand = getRegOperand(RegName);

  return findReg(val, bb, instr, 0, 1, retOperand, direct);
}

void RegAlloca::setPositionReg(Value *val, RiscvOperand *riscvReg, RiscvBasicBlock *bb, RiscvInstr *instr) {
  val = this->DSU_for_Variable.query(val);
  Value *old_val = getRegPosition(riscvReg);
  RiscvOperand *old_reg = getPositionReg(val);
  if (old_val != nullptr && old_val != val)
    writeback(riscvReg, bb, instr);
  if (old_reg != nullptr && old_reg != riscvReg)
    writeback(old_reg, bb, instr);
  setPositionReg(val, riscvReg);
}

void RegAlloca::setPositionReg(Value *val, RiscvOperand *riscvReg) {
  val = this->DSU_for_Variable.query(val);
  if (riscvReg->isRegister() == false) {
    std::terminate();
  }

  curReg[val] = riscvReg;
  regPos[riscvReg] = val;
  regUsed.insert(riscvReg);
}

RiscvInstr *RegAlloca::writeback(RiscvOperand *riscvReg, RiscvBasicBlock *bb, RiscvInstr *instr) {
  Value *value = getRegPosition(riscvReg);
  if (value == nullptr)
    return nullptr; 

  value = this->DSU_for_Variable.query(value);

  regPos.erase(riscvReg);
  regFindTimeStamp.erase(riscvReg);
  curReg.erase(value);

  RiscvOperand *mem_addr = findMem(value);

  if (mem_addr == nullptr) {
    return nullptr; 
  }

  auto store_type = value->IRType_;
  auto store_instr = new StoreRiscvInst(value->IRType_, riscvReg, mem_addr, bb);

  if (instr != nullptr)
    bb->addInstrBefore(store_instr, instr);
  else
    bb->addInstrBack(store_instr);

  return store_instr;
}

RegAlloca::RegAlloca() {
  if (regPool.size() == 0) {
    for (int i = 0; i < 32; i++)
      regPool.push_back(new RiscvIntReg(new Register(Register::Int, i)));
    for (int i = 0; i < 32; i++)
      regPool.push_back(new RiscvFloatReg(new Register(Register::Float, i)));
  }

  regUsed.insert(getRegOperand("ra"));
  savedRegister.push_back(getRegOperand("ra")); 

  for (int i = 1; i <= 11; i++)
    savedRegister.push_back(getRegOperand("s" + std::to_string(i)));

  for (int i = 0; i <= 11; i++)
    savedRegister.push_back(getRegOperand("fs" + std::to_string(i)));
}

RiscvInstr *RegAlloca::writeback(Value *val, RiscvBasicBlock *bb, RiscvInstr *instr) {
  auto reg = getPositionReg(val);
  return writeback(reg, bb, instr);
}

Value *RegAlloca::getRegPosition(RiscvOperand *reg) {
  if (regPos.find(reg) == regPos.end())
    return nullptr;
  return this->DSU_for_Variable.query(regPos[reg]);
}

RiscvOperand *RegAlloca::getPositionReg(Value *val) {
  val = this->DSU_for_Variable.query(val);
  if (curReg.find(val) == curReg.end())
    return nullptr;
  return curReg[val];
}

RiscvOperand *RegAlloca::findPtr(Value *val, RiscvBasicBlock *bb, RiscvInstr *instr) {
  val = this->DSU_for_Variable.query(val);
  if (ptrPos.find(val) == ptrPos.end()) {
    std::terminate();
  }
  return ptrPos[val];
}

void RegAlloca::writeback_all(RiscvBasicBlock *bb, RiscvInstr *instr) {
  std::vector<RiscvOperand *> regs_to_writeback;
  for (auto p : regPos)
    regs_to_writeback.push_back(p.first);
  for (auto r : regs_to_writeback)
    writeback(r, bb, instr);
}

void RegAlloca::setPointerPos(Value *val, RiscvOperand *PointerMem) {
  val = this->DSU_for_Variable.query(val);
  assert(val->IRType_->tid_ == IRType::IRTypeID::PointerTyID ||
         val->IRType_->tid_ == IRType::IRTypeID::ArrayTyID);
  this->ptrPos[val] = PointerMem;
}

void RegAlloca::clear() {
  curReg.clear();
  regPos.clear();
  safeFindTimeStamp = 0;
  regFindTimeStamp.clear();
}

std::map<RiscvInstr::InstrType, std::string> instrTy2Riscv = {
    {RiscvInstr::ADD, "ADD"},         {RiscvInstr::ADDI, "ADDI"},
    {RiscvInstr::ADDIW, "ADDIW"},     {RiscvInstr::SUB, "SUB"},
    {RiscvInstr::SUBI, "SUBI"},       {RiscvInstr::FADD, "FADD.S"},
    {RiscvInstr::FSUB, "FSUB.S"},     {RiscvInstr::FMUL, "FMUL.S"},
    {RiscvInstr::FDIV, "FDIV.S"},     {RiscvInstr::MUL, "MUL"},
    {RiscvInstr::DIV, "DIV"},         {RiscvInstr::REM, "REM"},
    {RiscvInstr::AND, "AND"},         {RiscvInstr::OR, "OR"},
    {RiscvInstr::ANDI, "ANDI"},       {RiscvInstr::ORI, "ORI"},
    {RiscvInstr::XOR, "XOR"},         {RiscvInstr::XORI, "XORI"},
    {RiscvInstr::RET, "RET"},         {RiscvInstr::FPTOSI, "FCVT.W.S"},
    {RiscvInstr::SITOFP, "FCVT.S.W"}, {RiscvInstr::FMV, "FMV.S"},
    {RiscvInstr::CALL, "CALL"},       {RiscvInstr::LI, "LI"},
    {RiscvInstr::MOV, "MV"},          {RiscvInstr::PUSH, "PUSH"},
    {RiscvInstr::POP, "POP"},         {RiscvInstr::SW, "SW"},
    {RiscvInstr::LW, "LW"},           {RiscvInstr::FSW, "FSW"},
    {RiscvInstr::FLW, "FLW"},         {RiscvInstr::SHL, "SLL"},
    {RiscvInstr::ASHR, "SRA"},        {RiscvInstr::SHLI, "SLLI"},
    {RiscvInstr::LSHR, "SRL"},        {RiscvInstr::ASHRI, "SRAI"},
    {RiscvInstr::LSHRI, "SRLI"},
};

const std::map<ICmpInst::ICmpOp, std::string> ICmpRiscvInstr::ICmpOpName = {
    {ICmpInst::ICmpOp::ICMP_EQ, "BEQ"},   {ICmpInst::ICmpOp::ICMP_NE, "BNE"},
    {ICmpInst::ICmpOp::ICMP_UGE, "BGEU"}, {ICmpInst::ICmpOp::ICMP_ULT, "BLTU"},
    {ICmpInst::ICmpOp::ICMP_SGE, "BGE"},  {ICmpInst::ICmpOp::ICMP_SLT, "BLT"},
    {ICmpInst::ICmpOp::ICMP_SLE, "BLE"}};
const std::map<ICmpInst::ICmpOp, std::string> ICmpRiscvInstr::ICmpOpSName = {
    {ICmpInst::ICmpOp::ICMP_EQ, "SEQZ"},  {ICmpInst::ICmpOp::ICMP_NE, "SNEZ"},
    {ICmpInst::ICmpOp::ICMP_UGE, "SLTU"}, {ICmpInst::ICmpOp::ICMP_ULT, "SLTU"},
    {ICmpInst::ICmpOp::ICMP_SGE, "SLT"},  {ICmpInst::ICmpOp::ICMP_SLT, "SLT"}};
const std::map<ICmpInst::ICmpOp, ICmpInst::ICmpOp> ICmpRiscvInstr::ICmpOpEquiv =
    {{ICmpInst::ICmpOp::ICMP_ULE, ICmpInst::ICmpOp::ICMP_UGE},
     {ICmpInst::ICmpOp::ICMP_UGT, ICmpInst::ICmpOp::ICMP_ULT},
     {ICmpInst::ICmpOp::ICMP_SLE, ICmpInst::ICmpOp::ICMP_SGE},
     {ICmpInst::ICmpOp::ICMP_SGT, ICmpInst::ICmpOp::ICMP_SLT}};
const std::map<FCmpInst::FCmpOp, std::string> FCmpRiscvInstr::FCmpOpName = {
    {FCmpInst::FCmpOp::FCMP_OLT, "FLT.S"},
    {FCmpInst::FCmpOp::FCMP_ULT, "FLT.S"},
    {FCmpInst::FCmpOp::FCMP_OLE, "FLE.S"},
    {FCmpInst::FCmpOp::FCMP_ULE, "FLE.S"},
    {FCmpInst::FCmpOp::FCMP_ORD, "FCLASS.S"},
    {FCmpInst::FCmpOp::FCMP_UNO, "FCLASS.S"}, // 取反
    {FCmpInst::FCmpOp::FCMP_OEQ, "FEQ.S"},
    {FCmpInst::FCmpOp::FCMP_UEQ, "FEQ.S"},
    {FCmpInst::FCmpOp::FCMP_ONE, "FEQ.S"}, // 取反
    {FCmpInst::FCmpOp::FCMP_UNE, "FEQ.S"}  // 取反
};
const std::map<FCmpInst::FCmpOp, FCmpInst::FCmpOp> FCmpRiscvInstr::FCmpOpEquiv =
    {{FCmpInst::FCmpOp::FCMP_OGT, FCmpInst::FCmpOp::FCMP_OLT},
     {FCmpInst::FCmpOp::FCMP_UGT, FCmpInst::FCmpOp::FCMP_ULT},
     {FCmpInst::FCmpOp::FCMP_OGE, FCmpInst::FCmpOp::FCMP_OLE},
     {FCmpInst::FCmpOp::FCMP_UGE, FCmpInst::FCmpOp::FCMP_ULE}};
std::string print_as_op(Value *v, bool print_ty);
std::string print_cmp_type(ICmpInst::ICmpOp op);
std::string print_fcmp_type(FCmpInst::FCmpOp op);

RiscvInstr::RiscvInstr(InstrType type, int op_nums)
    : type_(type), parent_(nullptr) {
  operand_.resize(op_nums);
}

RiscvInstr::RiscvInstr(InstrType type, int op_nums, RiscvBasicBlock *bb)
    : type_(type), parent_(bb) {
  operand_.resize(op_nums);
}

std::string BinaryRiscvInst::print() {
    assert(operand_.size() == 2);  
    std::stringstream ss;
    bool overflow = false;

    if (type_ == ADDI) {
        if (auto* imm = static_cast<RiscvConst*>(operand_[1])) {
            if (std::abs(imm->intval) >= 1024) {
                overflow = true;
                type_ = ADD;  
                ss << "\t\tLI\tt6, " << operand_[1]->print() << "\n";
            }
        }
    }

    ss << "\t\t" << instrTy2Riscv.at(type_);

    if (word && (type_ == ADDI || type_ == ADD || type_ == MUL || 
                 type_ == REM || type_ == DIV)) {
        ss << "W";
    }

    ss << "\t" << result_->print() << ", " 
       << operand_[0]->print() << ", "
       << (overflow ? "t6" : operand_[1]->print()) << "\n";

    return ss.str();
}

std::string UnaryRiscvInst::print() {
    std::stringstream ss;
    ss << "\t\t"
       << instrTy2Riscv[type_] << "\t"  
       << result_->print() << ", "
       << operand_[0]->print() << ", ";  
    
    return ss.str();
}

std::string CallRiscvInst::print() {
  std::string riscv_instr = "\t\tCALL\t";
  riscv_instr += static_cast<RiscvFunction *>(this->operand_[0])->name_;
  riscv_instr += "\n";
  return riscv_instr;
}

std::string ReturnRiscvInst::print() {
  std::string riscv_instr = "\t\tRET\n";
  return riscv_instr;
}

std::string PushRiscvInst::print() {
    std::stringstream ss;
    int shift = basicShift_;  

    for (auto& x : operand_) { 
        shift -= VARIABLE_ALIGN_BYTE;
        ss << "\t\tSD\t" << x->print() << ", " << shift << "(sp)\n";
    }

    return ss.str();
}
std::string PopRiscvInst::print() {
    std::stringstream ss;
    int shift = basicShift_;  
    
    for (auto& x : operand_) {  
        shift -= VARIABLE_ALIGN_BYTE;
        ss << "\t\tLD\t" << x->print() << ", " << shift << "(sp)\n";
    }
    
    ss << "\t\tADDI\tsp, " << -shift << "\n";
    
    return ss.str();
}

std::string ICmpRiscvInstr::print() {
    std::stringstream ss;

    if (!ICmpOpName.count(icmp_op_)) {
        std::swap(operand_[0], operand_[1]);
        icmp_op_ = ICmpOpEquiv.at(icmp_op_); 
    }

    ss << "\t\t"
       << ICmpOpName.at(icmp_op_) << "\t"
       << operand_[0]->print() << ", "
       << operand_[1]->print() << ", "
       << static_cast<RiscvBasicBlock*>(operand_[2])->name_ << "\n";

    if (auto falseLink = dynamic_cast<RiscvBasicBlock*>(operand_[3])) {
        ss << "\t\tJ\t" << falseLink->name_ << "\n";
    }

    return ss.str();
}

std::string ICmpSRiscvInstr::print() {
    std::stringstream ss;
    const bool eorne = (icmp_op_ == ICmpInst::ICMP_EQ || icmp_op_ == ICmpInst::ICMP_NE);

    if (eorne) {
        ss << "\t\tSUB\t"
           << "t6, "
           << operand_[0]->print() << ", "
           << operand_[1]->print() << "\n\t\t";
    }

    ss << ICmpOpSName.at(icmp_op_) << "\t"
       << result_->print() << ", ";
    if (!eorne) {
        ss << operand_[0]->print() << ", " << operand_[1]->print() << "\n";
    } else {
        ss << "t6\n";
    }
    return ss.str();
}

std::string FCmpRiscvInstr::print() {
    std::stringstream ss;
    ss << "\t\t"
       << FCmpOpName.at(fcmp_op_) << "\t"
       << result_->print() << ", "
       << operand_[0]->print() << ", "
       << operand_[1]->print() << "\n";
    return ss.str();
}
std::string StoreRiscvInst::print() {
    std::stringstream ss;
    auto mem_addr = static_cast<RiscvIntPhiReg*>(operand_[1]);
    const bool overflow = mem_addr->overflow();

    if (overflow) {
        ss << "\t\tLI\tt6, " << mem_addr->shift_ << "\n"
           << "\t\tADD\tt6, t6, " << mem_addr->MemBaseName << "\n";
    }

    std::string storeInstr;
    if (type.tid_ == IRType::FloatTyID) {
        storeInstr = "FSW\t";
    } else if (type.tid_ == IRType::IntegerTyID) {
        storeInstr = "SW\t";
    } else if (type.tid_ == IRType::PointerTyID) {
        storeInstr = "SD\t";
    } else {
        std::cerr << "[Error] Unknown store instruction type." << std::endl;
        std::terminate();
    }

    ss << "\t\t" << storeInstr
       << operand_[0]->print() << ", "
       << (overflow ? "(t6)" : operand_[1]->print())
       << "\n";
    return ss.str();
}

std::string LoadRiscvInst::print() {
    if (!operand_[0] || !operand_[1]) {
        return "";
    }
    std::stringstream ss;
    auto mem_addr = static_cast<RiscvIntPhiReg*>(operand_[1]);
    const bool overflow = mem_addr->overflow();

    if (overflow) {
        ss << "\t\tLI\tt6, " << mem_addr->shift_ << "\n"
           << "\t\tADD\tt6, t6, " << mem_addr->MemBaseName << "\n";
    }

    std::string loadInstr;
    if (type.tid_ == IRType::FloatTyID) {
        loadInstr = "FLW\t";
    } else if (type.tid_ == IRType::IntegerTyID) {
        loadInstr = "LW\t";
    } else if (type.tid_ == IRType::PointerTyID) {
        loadInstr = "LD\t";
    } else {
        std::cerr << "[Error] Unknown load instruction type." << std::endl;
        std::terminate();  
    }

    ss << "\t\t" << loadInstr
       << operand_[0]->print() << ", "
       << (overflow ? "(t6)" : operand_[1]->print())
       << "\n";

    return ss.str();
}

std::string MoveRiscvInst::print() {
    // 检查操作数是否相同
    if (operand_[0] == operand_[1] || operand_[0]->print() == operand_[1]->print()) {
        return "";
    }
    const std::string instr = (operand_[1]->tid_ == RiscvOperand::IntImm) ? "LI\t"
                            : (operand_[1]->tid_ == RiscvOperand::IntReg) ? "MV\t"
                            : "FMV.S\t";
    return "\t\t" + instr + operand_[0]->print() + ", " + operand_[1]->print() + "\n";
}

std::string SiToFpRiscvInstr::print() {
    return "\t\tFCVT.S.W\t" + 
           operand_[1]->print() + ", " + 
           operand_[0]->print() + "\n";
}

std::string FpToSiRiscvInstr::print() {
    return "\t\tFCVT.W.S\t" + 
           operand_[1]->print() + ", " + 
           operand_[0]->print() + ", rtz\n";
}

std::string LoadAddressRiscvInstr::print() {
  std::string riscv_instr = "\t\tLA\t" + this->operand_[0]->print() + ", " + this->name_ + "\n";
  return riscv_instr;
}

std::string BranchRiscvInstr::print() {
    std::string instr = "\t\t";  // 初始缩进
    if (operand_[0] != nullptr) {
        auto trueBlock = static_cast<RiscvBasicBlock*>(operand_[1]);
        instr += "BGTZ\t"      
               + operand_[0]->print()  
               + ", " 
               + trueBlock->name_     
               + "\n\t\t";             // 换行并保持缩进
    }
    auto falseBlock = static_cast<RiscvBasicBlock*>(operand_[2]);
    instr += "J\t"            
           + falseBlock->name_  
           + "\n";            
    return instr;
}