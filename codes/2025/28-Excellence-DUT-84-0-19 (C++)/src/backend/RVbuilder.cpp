#include "RVbuilder.h"

int LabelCount = 0;
std::map<BasicBlock *, RiscvBasicBlock *> rbbLabel;
std::map<Function *, RiscvFunction *> functionLabel;
std::string toLabel(int ind) { return ".L" + std::to_string(ind); } //生成每个块的ID

int IRtoRISCV::computeConstantBinaryResult(Instruction::OpID opId, int lhs, int rhs) { //计算常量表达式
    switch (opId) {
        case Instruction::OpID::Add:  return lhs + rhs;
        case Instruction::OpID::Sub:  return lhs - rhs;
        case Instruction::OpID::Mul:  return lhs * rhs;
        case Instruction::OpID::SDiv: return lhs / rhs;
        case Instruction::OpID::SRem: return lhs % rhs;
        default:
            std::terminate();
    }
}

// 创建RISCV双目运算指令
BinaryRiscvInst* IRtoRISCV::createBinaryInstr(RegAlloca* regAlloca, BinaryInst* binaryInstr,RiscvBasicBlock* rbb) {
    // 获取对应的RISCV操作码
    const auto riscvOpId = toRiscvOp.at(binaryInstr->op_id_);

    // 常量折叠
    auto* lhsConstant = dynamic_cast<ConstantInt*>(binaryInstr->operands_[0]);
    auto* rhsConstant = dynamic_cast<ConstantInt*>(binaryInstr->operands_[1]);
    
    if (lhsConstant && rhsConstant) {
        // 计算常量运算结果
        const int result = computeConstantBinaryResult(binaryInstr->op_id_,lhsConstant->value_, rhsConstant->value_);
        const auto destReg = regAlloca->findReg(binaryInstr, rbb, nullptr, 0, 0);
        rbb->addInstrBack(new MoveRiscvInst(destReg, result, rbb));
        return nullptr;
    }
    const auto lhsReg = regAlloca->findReg(binaryInstr->operands_[0], rbb, nullptr, 1);
    const auto rhsReg = regAlloca->findReg(binaryInstr->operands_[1], rbb, nullptr, 1);
    const auto destReg = regAlloca->findReg(binaryInstr, rbb, nullptr, 1, 0);
    return new BinaryRiscvInst(riscvOpId, lhsReg, rhsReg, destReg, rbb, true);
}

RiscvBasicBlock* createRiscvBasicBlock(BasicBlock* irBasicBlock) { //创建RISCV基本块
    if (irBasicBlock == nullptr) { //如果是空块
        LabelCount++;
        return new RiscvBasicBlock(toLabel(LabelCount), LabelCount);
    }
    if (rbbLabel.count(irBasicBlock)) { //如果已经创建过了
        return rbbLabel[irBasicBlock];
    }
    LabelCount++;  
    RiscvBasicBlock* newRiscvBB = new RiscvBasicBlock(toLabel(LabelCount), LabelCount); //创建新块
    rbbLabel[irBasicBlock] = newRiscvBB;
    return newRiscvBB;
}
    
RiscvFunction *createRiscvFunction(Function *foo) { //创建RISCV函数
  assert(foo != nullptr);
  if (functionLabel.count(foo) == 0) { //检测是否已经添加了该函数
    auto ty = RiscvOperand::Void;
    switch (foo->IRType_->tid_) {      //检测函数类型
    case IRType::VoidTyID:
      ty = RiscvOperand::Void;
      break;
    case IRType::IntegerTyID:
      ty = RiscvOperand::IntReg;
      break;
    case IRType::FloatTyID:
      ty = RiscvOperand::FloatReg;
      break;
    }
    RiscvFunction *cur =
        new RiscvFunction(foo->name_, foo->arguments_.size(), ty); //建立Riscv函数 形参为ID，该函数参数个数以及IRType
    return functionLabel[foo] = cur; //返回已创建的对象
  }
  return functionLabel[foo];
}

UnaryRiscvInst *IRtoRISCV::createUnaryInstr(RegAlloca *regAlloca,UnaryInst *unaryInstr, RiscvBasicBlock *rbb) { //创建RISCV单目运算符
  UnaryRiscvInst *instr = new UnaryRiscvInst(toRiscvOp.at(unaryInstr->op_id_),regAlloca->findReg(unaryInstr->operands_[0], rbb, nullptr, 1),regAlloca->findReg(unaryInstr, rbb, nullptr, 1, 0), rbb);
  return instr;
}

std::vector<RiscvInstr*> IRtoRISCV::createStoreInstr(RegAlloca* regAlloca,StoreInst* storeInstr,RiscvBasicBlock* rbb) {
    // 处理常量整数的存储情况
    if (auto constInt = dynamic_cast<ConstantInt*>(storeInstr->operands_[0])) {
        std::vector<RiscvInstr*> instructions;
        // 获取临时寄存器并移动常量值到该寄存器
        auto tempReg = getRegOperand("t0");
        instructions.push_back(new MoveRiscvInst(tempReg, new RiscvConst(constInt->value_), rbb));
        // 查找内存位置并创建存储指令（合并重复逻辑）
        auto memLocation = regAlloca->findMem(storeInstr->operands_[1], rbb, nullptr, 0);
        instructions.push_back(new StoreRiscvInst(storeInstr->operands_[0]->IRType_, tempReg, memLocation, rbb));
        return instructions;
    }
    // 处理指针类型操作数的存储情况
    if (storeInstr->operands_[1]->IRType_->tid_ == IRType::IRTypeID::PointerTyID) {
        auto pointerType = static_cast<PointerIRType*>(storeInstr->operands_[1]->IRType_);
        auto sourceReg = regAlloca->findReg(storeInstr->operands_[0], rbb, nullptr, 1);
        auto memLocation = regAlloca->findMem(storeInstr->operands_[1], rbb, nullptr, 0);
        
        return {new StoreRiscvInst(pointerType->contained_, sourceReg, memLocation, rbb)};
    }
    // 处理其他类型的存储情况
    std::vector<RiscvInstr*> instructions;
    auto sourceReg = regAlloca->findReg(storeInstr->operands_[0], rbb, nullptr, 1);
    auto destReg = regAlloca->findReg(storeInstr->operands_[1], rbb);
    // 移动值到目标寄存器
    instructions.push_back(new MoveRiscvInst(destReg, sourceReg, rbb));
    auto memLocation = regAlloca->findMem(storeInstr->operands_[0]);
    instructions.push_back(new StoreRiscvInst(storeInstr->operands_[0]->IRType_, sourceReg, memLocation, rbb));
    return instructions;
}

std::vector<RiscvInstr *> IRtoRISCV::createLoadInstr(RegAlloca *regAlloca,LoadInst *loadInstr,RiscvBasicBlock *rbb) {
  assert(loadInstr->operands_[0]->IRType_->tid_ == IRType::IRTypeID::PointerTyID);
  auto curType = static_cast<PointerIRType *>(loadInstr->operands_[0]->IRType_);
  std::vector<RiscvInstr *> ans;
  auto regPos = regAlloca->findReg(static_cast<Value *>(loadInstr), rbb, nullptr, 1, 0);
  ans.push_back(new LoadRiscvInst(curType->contained_, regPos, regAlloca->findMem(loadInstr->operands_[0], rbb, nullptr, false), rbb));
  return ans;
}

ICmpRiscvInstr *IRtoRISCV::createICMPInstr(RegAlloca *regAlloca,ICmpInst *icmpInstr,BranchInst *brInstr,RiscvBasicBlock *rbb) {
  ICmpRiscvInstr *instr = new ICmpRiscvInstr(
      icmpInstr->icmp_op_,
      regAlloca->findReg(icmpInstr->operands_[0], rbb, nullptr, 1),
      regAlloca->findReg(icmpInstr->operands_[1], rbb, nullptr, 1),
      createRiscvBasicBlock(static_cast<BasicBlock *>(brInstr->operands_[1])),
      createRiscvBasicBlock(static_cast<BasicBlock *>(brInstr->operands_[2])),rbb);
  return instr;
}

ICmpRiscvInstr *IRtoRISCV::createICMPSInstr(RegAlloca *regAlloca,ICmpInst *icmpInstr,RiscvBasicBlock *rbb) {
  bool swap = ICmpSRiscvInstr::ICmpOpSName.count(icmpInstr->icmp_op_) == 0;
  if (swap) {
    std::swap(icmpInstr->operands_[0], icmpInstr->operands_[1]);
    icmpInstr->icmp_op_ =
        ICmpRiscvInstr::ICmpOpEquiv.find(icmpInstr->icmp_op_)->second;
  }
  bool inv = false;
  switch (icmpInstr->icmp_op_) {
  case ICmpInst::ICMP_SGE:
  case ICmpInst::ICMP_SLE:
  case ICmpInst::ICMP_UGE:
  case ICmpInst::ICMP_ULE:
    inv = true;
  default:
    break;
  }
  ICmpSRiscvInstr *instr = new ICmpSRiscvInstr(
      icmpInstr->icmp_op_,
      regAlloca->findReg(icmpInstr->operands_[0], rbb, nullptr, 1),
      regAlloca->findReg(icmpInstr->operands_[1], rbb, nullptr, 1),
      regAlloca->findReg(icmpInstr, rbb, nullptr, 1, 0), rbb);
  rbb->addInstrBack(instr);
  if (inv) {
    auto instr_reg = regAlloca->findReg(icmpInstr, rbb, nullptr, 1, 0);
    rbb->addInstrBack(new BinaryRiscvInst(RiscvInstr::XORI, instr_reg,
                                          new RiscvConst(1), instr_reg, rbb));
  }
  return instr;
}

RiscvInstr *IRtoRISCV::createFCMPInstr(RegAlloca *regAlloca,FCmpInst *fcmpInstr,RiscvBasicBlock *rbb) {
  if (fcmpInstr->fcmp_op_ == fcmpInstr->FCMP_TRUE ||
      fcmpInstr->fcmp_op_ == fcmpInstr->FCMP_FALSE) {
    auto instr =
        new MoveRiscvInst(regAlloca->findReg(fcmpInstr, rbb, nullptr, 1, 0),
                          fcmpInstr->fcmp_op_ == fcmpInstr->FCMP_TRUE, rbb);
    rbb->addInstrBack(instr);
    return instr;
  }
  bool swap = FCmpRiscvInstr::FCmpOpName.count(fcmpInstr->fcmp_op_) == 0;
  if (swap) {
    std::swap(fcmpInstr->operands_[0], fcmpInstr->operands_[1]);
    fcmpInstr->fcmp_op_ =
        FCmpRiscvInstr::FCmpOpEquiv.find(fcmpInstr->fcmp_op_)->second;
  }
  bool inv = false;
  bool inv_classify = false;
  switch (fcmpInstr->fcmp_op_) {
  case FCmpInst::FCMP_ONE:
  case FCmpInst::FCMP_UNE:
    inv = true;
  default:
    break;
  }
  switch (fcmpInstr->fcmp_op_) {
  case FCmpInst::FCMP_OEQ:
  case FCmpInst::FCMP_OGT:
  case FCmpInst::FCMP_OGE:
  case FCmpInst::FCMP_OLT:
  case FCmpInst::FCMP_OLE:
  case FCmpInst::FCMP_ONE:
  case FCmpInst::FCMP_ORD:
    inv_classify = true;
  default:
    break;
  }

  if (inv_classify) {
    std::cerr << "[Warning] Not implemented FCLASS yet.\n";
  }
  FCmpRiscvInstr *instr = new FCmpRiscvInstr(
      fcmpInstr->fcmp_op_,
      regAlloca->findReg(fcmpInstr->operands_[0], rbb, nullptr, 1),
      regAlloca->findReg(fcmpInstr->operands_[1], rbb, nullptr, 1),
      regAlloca->findReg(fcmpInstr, rbb, nullptr, 1, 0), rbb);
  rbb->addInstrBack(instr);
  if (inv) {
    auto instr_reg = regAlloca->findReg(fcmpInstr, rbb, nullptr, 1, 0);
    rbb->addInstrBack(new BinaryRiscvInst(RiscvInstr::XORI, instr_reg,
                                          new RiscvConst(1), instr_reg, rbb));
    return instr;
  }
  return instr;
}

CallRiscvInst *IRtoRISCV::createCallInstr(RegAlloca *regAlloca,
                                             CallInst *callInstr,
                                             RiscvBasicBlock *rbb) {
  int argnum = callInstr->operands_.size() - 1;
  CallRiscvInst *instr =
      new CallRiscvInst(createRiscvFunction(static_cast<Function *>(
                            callInstr->operands_[argnum])),
                        rbb);
  return instr;
}

ReturnRiscvInst *IRtoRISCV::createRetInstr(RegAlloca *regAlloca,
                                              ReturnInst *returnInstr,
                                              RiscvBasicBlock *rbb,
                                              RiscvFunction *rfoo) {
  RiscvOperand *reg_to_save = nullptr;

  if (returnInstr->num_ops_ > 0) {
    auto operand = returnInstr->operands_[0];
    if (operand->IRType_->tid_ == IRType::IRTypeID::IntegerTyID)
      reg_to_save = regAlloca->findSpecificReg(operand, "a0", rbb);
    else if (operand->IRType_->tid_ == IRType::IRTypeID::FloatTyID)
      reg_to_save = regAlloca->findSpecificReg(operand, "fa0", rbb);

    rbb->addInstrBack(new MoveRiscvInst(
        reg_to_save, regAlloca->findReg(operand, rbb, nullptr), rbb));
  }

  return new ReturnRiscvInst(rbb);
}

BranchRiscvInstr *IRtoRISCV::createBrInstr(RegAlloca *regAlloca,
                                              BranchInst *brInstr,
                                              RiscvBasicBlock *rbb) {

  BranchRiscvInstr *instr;
  if (brInstr->num_ops_ == 1) {
    instr = new BranchRiscvInstr(
        nullptr, nullptr,
        createRiscvBasicBlock(static_cast<BasicBlock *>(brInstr->operands_[0])),
        rbb);
  } else {
    instr = new BranchRiscvInstr(
        regAlloca->findReg(brInstr->operands_[0], rbb, nullptr, 1),
        createRiscvBasicBlock(static_cast<BasicBlock *>(brInstr->operands_[1])),
        createRiscvBasicBlock(static_cast<BasicBlock *>(brInstr->operands_[2])),
        rbb);
  }
  return instr;
}

SiToFpRiscvInstr *IRtoRISCV::createSiToFpInstr(RegAlloca *regAlloca,
                                                  SiToFpInst *sitofpInstr,
                                                  RiscvBasicBlock *rbb) {
  return new SiToFpRiscvInstr(
      regAlloca->findReg(sitofpInstr->operands_[0], rbb, nullptr, 1),
      regAlloca->findReg(static_cast<Value *>(sitofpInstr), rbb, nullptr, 1, 0),
      rbb);
}

FpToSiRiscvInstr *IRtoRISCV::createFptoSiInstr(RegAlloca *regAlloca,
                                                  FpToSiInst *fptosiInstr,
                                                  RiscvBasicBlock *rbb) {
  return new FpToSiRiscvInstr(
      regAlloca->findReg(fptosiInstr->operands_[0], rbb, nullptr, 1),
      regAlloca->findReg(static_cast<Value *>(fptosiInstr), rbb, nullptr, 1, 0),
      rbb);
}

RiscvInstr *IRtoRISCV::solveGetElementPtr(RegAlloca *regAlloca,
                                             GetElementPtrInst *instr,
                                             RiscvBasicBlock *rbb) {
  Value *op0 = instr->get_operand(0);
  RiscvOperand *dest = getRegOperand("t2");
  bool isConst = 1; 
  int finalOffset = 0;
  if (dynamic_cast<GlobalVariable *>(op0) != nullptr) {
    isConst = 0;
    rbb->addInstrBack(new LoadAddressRiscvInstr(dest, op0->name_, rbb));
  } else if (auto oi = dynamic_cast<Instruction *>(op0)) {
    int varOffset = 0;

    rbb->addInstrBack(new MoveRiscvInst(
        dest, regAlloca->findReg(op0, rbb, nullptr, 1, 1), rbb));

    finalOffset += varOffset;
  }
  int curTypeSize = 0;
  unsigned int num_operands = instr->num_ops_;
  int indexVal, totalOffset = 0;
  IRType *cur_type =
      static_cast<PointerIRType *>(instr->get_operand(0)->IRType_)->contained_;
  for (unsigned int i = 1; i <= num_operands - 1; i++) {
    if (i > 1)
      cur_type = static_cast<ArrayIRType *>(cur_type)->contained_;
    Value *opi = instr->get_operand(i);
    curTypeSize = calcTypeSize(cur_type);
    if (auto ci = dynamic_cast<ConstantInt *>(opi)) {
      indexVal = ci->value_;
      totalOffset += indexVal * curTypeSize;
    } else {
      isConst = 0;
      RiscvOperand *mulTempReg = getRegOperand("t3");
      rbb->addInstrBack(new MoveRiscvInst(mulTempReg, curTypeSize, rbb));
      rbb->addInstrBack(new BinaryRiscvInst(
          RiscvInstr::InstrType::MUL, regAlloca->findReg(opi, rbb, nullptr, 1),
          mulTempReg, mulTempReg, rbb));
      rbb->addInstrBack(new BinaryRiscvInst(RiscvInstr::InstrType::ADD,
                                            mulTempReg, dest, dest, rbb));
    }
  }
  rbb->addInstrBack(new BinaryRiscvInst(RiscvInstr::InstrType::ADDI, dest,
                                        new RiscvConst(totalOffset), dest,
                                        rbb));
  rbb->addInstrBack(
      new StoreRiscvInst(instr->IRType_, dest, regAlloca->findMem(instr), rbb));
  return nullptr;
}

void IRtoRISCV::initRetInstr(RegAlloca *regAlloca, RiscvInstr *returnInstr,
                                RiscvBasicBlock *rbb, RiscvFunction *foo) {
  int curSP = foo->querySP();
  auto reg_to_recover = regAlloca->savedRegister;
  auto reg_used = regAlloca->getUsedReg();
  reverse(reg_to_recover.begin(), reg_to_recover.end());
  for (auto reg : reg_to_recover)
    if (reg_used.find(reg) != reg_used.end()) {
      if (reg->getType() == reg->IntReg)
        rbb->addInstrBefore(new LoadRiscvInst(new IRType(IRType::PointerTyID), reg, new RiscvIntPhiReg("fp", curSP), rbb), returnInstr);
      else
        rbb->addInstrBefore(new LoadRiscvInst(new IRType(IRType::FloatTyID), reg, new RiscvIntPhiReg("fp", curSP), rbb), returnInstr);
      curSP += VARIABLE_ALIGN_BYTE;
    }
  rbb->addInstrBefore(new LoadRiscvInst(new IRType(IRType::PointerTyID), getRegOperand("fp"), new RiscvIntPhiReg("fp", curSP), rbb), returnInstr);
  rbb->addInstrBefore(new BinaryRiscvInst(RiscvInstr::ADDI, getRegOperand("sp"), new RiscvConst(-foo->querySP()), getRegOperand("sp"), rbb), returnInstr);
}

RiscvBasicBlock *IRtoRISCV::transferRiscvBasicBlock(BasicBlock *bb, RiscvFunction *foo) { //将IR基本块转成RISCV基本块
  int translationCount = 0;
  RiscvBasicBlock *rbb = createRiscvBasicBlock(bb);
  Instruction *forward = nullptr;
  bool brFound = false;
  for (Instruction *instr : bb->instr_list_) { //遍历基本块指令
    switch (instr->op_id_) {
    case Instruction::Ret:
      foo->regAlloca->writeback_all(rbb);
      brFound = true;
      rbb->addInstrBack(this->createRetInstr(
          foo->regAlloca, static_cast<ReturnInst *>(instr), rbb, foo));
      break;
    case Instruction::Br:
      foo->regAlloca->writeback_all(rbb);
      brFound = true;

      rbb->addInstrBack(this->createBrInstr(
          foo->regAlloca, static_cast<BranchInst *>(instr), rbb));
      break;
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::SDiv:
    case Instruction::SRem:
    case Instruction::UDiv:
    case Instruction::URem:
    case Instruction::FAdd:
    case Instruction::FSub:
    case Instruction::FMul:
    case Instruction::FDiv:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      rbb->addInstrBack(this->createBinaryInstr(
          foo->regAlloca, static_cast<BinaryInst *>(instr), rbb));
      break;
    case Instruction::FNeg:
      rbb->addInstrBack(this->createUnaryInstr(
          foo->regAlloca, static_cast<UnaryInst *>(instr), rbb));
      break;
    case Instruction::PHI:
      break;
    case Instruction::BitCast:
      break;
    case Instruction::ZExt:
      break;
    case Instruction::Alloca:
      break;
    case Instruction::GetElementPtr: {
      this->solveGetElementPtr(foo->regAlloca,
                               static_cast<GetElementPtrInst *>(instr), rbb);
      break;
    }
    case Instruction::FPtoSI:
      rbb->addInstrBack(this->createFptoSiInstr(
          foo->regAlloca, static_cast<FpToSiInst *>(instr), rbb));
      break;
    case Instruction::SItoFP:
      rbb->addInstrBack(this->createSiToFpInstr(
          foo->regAlloca, static_cast<SiToFpInst *>(instr), rbb));
      break;
    case Instruction::Load: {
      auto instrSet = this->createLoadInstr(
          foo->regAlloca, static_cast<LoadInst *>(instr), rbb);
      for (auto x : instrSet)
        rbb->addInstrBack(x);
      break;
    }
    case Instruction::Store: {
      auto instrSet = this->createStoreInstr(
          foo->regAlloca, static_cast<StoreInst *>(instr), rbb);
      for (auto *x : instrSet)
        rbb->addInstrBack(x);
      break;
    }
    case Instruction::ICmp:
      createICMPSInstr(foo->regAlloca, static_cast<ICmpInst *>(instr), rbb);
      break;
    case Instruction::FCmp:
      createFCMPInstr(foo->regAlloca, static_cast<FCmpInst *>(instr), rbb);
      break;
    case Instruction::Call: {
      CallInst *curInstr = static_cast<CallInst *>(instr);
      RiscvFunction *calleeFoo = createRiscvFunction(
          static_cast<Function *>(curInstr->operands_.back()));

      int sp_shift_for_paras = 0;
      int sp_shift_alignment_padding = 0; 
      int paraShift = 0;

      int intRegCount = 0, floatRegCount = 0;

      for (int i = 0; i < curInstr->operands_.size() - 1; i++) {
        sp_shift_for_paras += VARIABLE_ALIGN_BYTE;
      }

      sp_shift_alignment_padding =
          16 - ((abs(foo->querySP()) + sp_shift_for_paras) & 15);
      sp_shift_for_paras += sp_shift_alignment_padding;

      rbb->addInstrBack(new BinaryRiscvInst(
          BinaryRiscvInst::ADDI, getRegOperand("sp"),
          new RiscvConst(-sp_shift_for_paras), getRegOperand("sp"), rbb));

      for (int i = 0; i < curInstr->operands_.size() - 1; i++) {
        std::string name = "";
        auto operand = curInstr->operands_[i];
        if (operand->IRType_->tid_ != IRType::FloatTyID) {
          if (intRegCount < 8)
            name = "a" + std::to_string(intRegCount);
          intRegCount++;
        } else if (operand->IRType_->tid_ == IRType::FloatTyID) {
          if (floatRegCount < 8)
            name = "fa" + std::to_string(floatRegCount);
          floatRegCount++;
        }
        if (name.empty()) {
          if (operand->IRType_->tid_ != IRType::FloatTyID) {
            rbb->addInstrBack(new StoreRiscvInst(
                operand->IRType_,
                foo->regAlloca->findSpecificReg(operand, "t1", rbb),
                new RiscvIntPhiReg("sp", paraShift), rbb));
          } else {
            rbb->addInstrBack(new StoreRiscvInst(
                operand->IRType_,
                foo->regAlloca->findSpecificReg(operand, "ft1", rbb),
                new RiscvIntPhiReg("sp", paraShift), rbb));
          }
        } else {
          foo->regAlloca->findSpecificReg(operand, name, rbb, nullptr);
        }
        paraShift += VARIABLE_ALIGN_BYTE; 
      }

      rbb->addInstrBack(this->createCallInstr(foo->regAlloca, curInstr, rbb));

      rbb->addInstrBack(new BinaryRiscvInst(
          BinaryRiscvInst::ADDI, getRegOperand("sp"),
          new RiscvConst(sp_shift_for_paras), getRegOperand("sp"), rbb));

      if (curInstr->IRType_->tid_ != curInstr->IRType_->VoidTyID) {
        if (curInstr->IRType_->tid_ != curInstr->IRType_->FloatTyID) {
          rbb->addInstrBack(
              new StoreRiscvInst(new IntegerIRType(32), getRegOperand("a0"),
                                 foo->regAlloca->findMem(curInstr), rbb));
        } else {
          rbb->addInstrBack(new StoreRiscvInst(
              new IRType(IRType::FloatTyID), getRegOperand("fa0"),
              foo->regAlloca->findMem(curInstr), rbb));
        }
      }
      break;
    }
    }
  }
  if (!brFound) {
    foo->regAlloca->writeback_all(rbb);
  }
  return rbb;
}

std::string IRtoRISCV::buildRISCV(Module *m) {
  this->rm = new RiscvModule();
  std::string data = ".align 2\n.section .data\n"; //初始化
  // 全局常量/变量
  if (!m->global_list_.empty()) { //列表不为空
    for (GlobalVariable *gb : m->global_list_) {  //遍历全局常量/变量
        auto curType = static_cast<PointerIRType *>(gb->IRType_)->contained_;
        RiscvGlobalVariable *RGV = nullptr;
        IRType *containedType = nullptr;
        switch (curType->tid_) {
        case IRType::ArrayTyID:
            // 处理数组类型，找到最内层元素类型
            containedType = curType;
            while (containedType->tid_ == IRType::ArrayTyID) {
                containedType = static_cast<ArrayIRType *>(containedType)->contained_;
            }
            // 根据最内层元素类型创建全局变量
            if (containedType->tid_ == IRType::IntegerTyID) {
                RGV = new RiscvGlobalVariable(RiscvOperand::IntImm, gb->name_,
                                              gb->is_const_, gb->init_val_,
                                              calcTypeSize(curType) / 4);
            } else if (containedType->tid_ == IRType::FloatTyID) {
                RGV = new RiscvGlobalVariable(RiscvOperand::FloatImm, gb->name_,
                                              gb->is_const_, gb->init_val_,
                                              calcTypeSize(curType) / 4);
            } 
            rm->addGlobalVariable(RGV);
            data += RGV->print();
            break;
        case IRType::IntegerTyID:
        case IRType::FloatTyID: {
            // 处理基本类型（整数和浮点数）
            RiscvOperand::OpTy opType = 
                (curType->tid_ == IRType::IntegerTyID) ? 
                RiscvOperand::OpTy::IntImm : RiscvOperand::OpTy::FloatImm;
                
            RGV = new RiscvGlobalVariable(opType, gb->name_,
                                          gb->is_const_, gb->init_val_);
            rm->addGlobalVariable(RGV);
            data += RGV->print();
            break;
        }
        default:
            std::cerr << "GlobalVariable Type ERROR " << std::endl;
            break;
        }
    }
  }

  int ConstFloatCount = 0;
  std::string code = ".section .text\n";
  //函数体
  for (Function *foo : m->function_list_) {    //遍历每一个函数
    auto rfoo = createRiscvFunction(foo);      //创建RISCV函数
    rm->addFunction(rfoo); 
    if (rfoo->is_libfunc()) {                  //是否是库函数
      auto *libFunc = createSyslibFunc(foo);
      if (libFunc != nullptr)
        code += libFunc->print();              //优先打印库函数
      continue;
    }

    // 遍历函数内所有基本块
    for (BasicBlock* basicBlock : foo->basic_blocks_) {
        // 遍历当前基本块中的所有指令
        for (Instruction* instruction : basicBlock->instr_list_) {
            switch (instruction->op_id_) {
                case Instruction::OpID::PHI: {
                    // 处理PHI节点
                    for (Value* operand : instruction->operands_) {
                        rfoo->regAlloca->DSU_for_Variable.merge(operand, static_cast<Value*>(instruction));
                    }
                    break;
                }
                case Instruction::OpID::ZExt: {
                    // 处理零扩展
                    Value* sourceOperand = instruction->operands_[0];
                    rfoo->regAlloca->DSU_for_Variable.merge(sourceOperand,static_cast<Value*>(instruction));
                    break;
                }
                case Instruction::OpID::BitCast: {
                    // 处理位转换
                    Value* sourceOperand = instruction->operands_[0];
                    rfoo->regAlloca->DSU_for_Variable.merge( static_cast<Value*>(instruction), sourceOperand);
                    break;
                }
            }
        }
    }
    // 将该函数内的浮点常量全部处理出来并告知寄存器分配单元
    for (BasicBlock* basicBlock : foo->basic_blocks_) {
        // 遍历当前基本块中的所有指令
        for (Instruction* instruction : basicBlock->instr_list_) {
            for (Value* operand : instruction->operands_) {
                // 判断当前操作数是否为浮点常量
                ConstantFloat* floatConstant = dynamic_cast<ConstantFloat*>(operand);
                if (floatConstant != nullptr) {
                    std::string constantName = "FloatConst" + std::to_string(ConstFloatCount);
                    ConstFloatCount++;  
                    std::string valueStr = floatConstant->print32();
                    while (valueStr.length() < 10) {
                        valueStr += "0";
                    }
                    data += constantName + ":\n\t.word\t" + valueStr.substr(0, 10) + "\n";
                    rfoo->regAlloca->setPosition(
                        operand, 
                        new RiscvFloatPhiReg(constantName, 0)  
                    );
                }
            }
        }
    }

    // 首先检查所有的alloca指令，加入一个基本块进行寄存器保护以及栈空间分配
    RiscvBasicBlock *initBlock = createRiscvBasicBlock(); //创建初始化基本块
    std::map<Value *, int> haveAllocated;
    int IntParaCount = 0, FloatParaCount = 0;
    int sp_shift_for_paras = 0;
    int paraShift = 0;

    rfoo->setSP(0);

    auto storeOnStack = [&](Value **val) { //为变量分配储存空间
      //如果是空、已经分配过或者不需要分配直接返回
      if (val == nullptr)
        return;
      if (haveAllocated.count(*val))
        return;
      if (dynamic_cast<Function *>(*val) != nullptr)
        return;
      if (dynamic_cast<BasicBlock *>(*val) != nullptr)
        return;
      if ((*val)->name_.empty())
        return;
      if (dynamic_cast<GlobalVariable *>(*val) != nullptr) {
        auto curType = (*val)->IRType_;
        while (1) { //找到最基础的内部类型
          if (curType->tid_ == IRType::IRTypeID::ArrayTyID)
            curType = static_cast<ArrayIRType *>(curType)->contained_;
          else if (curType->tid_ == IRType::IRTypeID::PointerTyID)
            curType = static_cast<PointerIRType *>(curType)->contained_;
          else
            break;
        }
        if (curType->tid_ != IRType::IRTypeID::FloatTyID)
          rfoo->regAlloca->setPosition(*val, new RiscvIntPhiReg((*val)->name_, 0, 1));
        else
          rfoo->regAlloca->setPosition(*val, new RiscvFloatPhiReg((*val)->name_, 0, 1));
        return;
      }

      if (dynamic_cast<Argument *>(*val) != nullptr) {  //处理函数参数
        // 整型参数
        if ((*val)->IRType_->tid_ == IRType::IRTypeID::IntegerTyID ||
            (*val)->IRType_->tid_ == IRType::IRTypeID::PointerTyID) {
          if (IntParaCount < 8)
            rfoo->regAlloca->setPositionReg(
                *val, getRegOperand("a" + std::to_string(IntParaCount)));
          rfoo->regAlloca->setPosition(
              *val, new RiscvIntPhiReg(NamefindReg("fp"), paraShift));
          IntParaCount++;
        }
        // 浮点参数
        else {
          assert((*val)->IRType_->tid_ == IRType::IRTypeID::FloatTyID);
          if (FloatParaCount < 8) {
            rfoo->regAlloca->setPositionReg(
                *val, getRegOperand("fa" + std::to_string(FloatParaCount)));
          }
          rfoo->regAlloca->setPosition(
              *val, new RiscvFloatPhiReg(NamefindReg("fp"), paraShift));
          FloatParaCount++;
        }
        paraShift += VARIABLE_ALIGN_BYTE;
      }
      // 函数内变量
      else {
        int curSP = rfoo->querySP();
        RiscvOperand *stackPos = static_cast<RiscvOperand *>(
            new RiscvIntPhiReg(NamefindReg("fp"), curSP - VARIABLE_ALIGN_BYTE));
        rfoo->regAlloca->setPosition(static_cast<Value *>(*val), stackPos);
        rfoo->addTempVar(stackPos);
      }
      haveAllocated[*val] = 1;
    };

    // 分配函数参数、变量的储存空间
    for (Value *arg : foo->arguments_)
      storeOnStack(&arg);

    for (BasicBlock *bb : foo->basic_blocks_)  //非特殊类型的指令及其操作数分配存储位置
      for (Instruction *instr : bb->instr_list_)
        if (instr->op_id_ != Instruction::OpID::PHI &&
            instr->op_id_ != Instruction::OpID::ZExt &&
            instr->op_id_ != Instruction::OpID::Alloca) {
          // 所有的函数局部变量都要压入栈
          Value *tempPtr = static_cast<Value *>(instr);
          storeOnStack(&tempPtr);
          for (auto *val : instr->operands_) {
            tempPtr = static_cast<Value *>(val);
            storeOnStack(&tempPtr);
          }
        }

    for (BasicBlock *bb : foo->basic_blocks_)
      for (Instruction *instr : bb->instr_list_)
        if (instr->op_id_ == Instruction::OpID::Alloca) {
          // 分配指针，并且将指针地址也同步保存
          auto curInstr = static_cast<AllocaInst *>(instr);
          int curTypeSize = calcTypeSize(curInstr->alloca_ty_);
          rfoo->storeArray(curTypeSize);
          int curSP = rfoo->querySP();
          RiscvOperand *ptrPos = new RiscvIntPhiReg(NamefindReg("fp"), curSP);
          rfoo->regAlloca->setPosition(static_cast<Value *>(instr), ptrPos);
          rfoo->regAlloca->setPointerPos(static_cast<Value *>(instr), ptrPos);
        }

    // 添加初始化基本块
    rfoo->addBlock(initBlock);
    // 翻译语句并计算被使用的寄存器
    for (BasicBlock *bb : foo->basic_blocks_)
      rfoo->addBlock(this->transferRiscvBasicBlock(bb, rfoo));
    rfoo->ChangeBlock(initBlock, 0);

    // 保护寄存器
    rfoo->shiftSP(-VARIABLE_ALIGN_BYTE);
    int fp_sp = rfoo->querySP(); // 为保护 fp 分配空间
    auto &reg_to_save = rfoo->regAlloca->savedRegister;
    auto reg_used = rfoo->regAlloca->getUsedReg();
    for (auto reg : reg_to_save)
      if (reg_used.find(reg) != reg_used.end()) {
        rfoo->shiftSP(-VARIABLE_ALIGN_BYTE);
        if (reg->getType() == reg->IntReg)
          initBlock->addInstrBack(new StoreRiscvInst(
              new IRType(IRType::PointerTyID), reg,
              new RiscvIntPhiReg(NamefindReg("fp"), rfoo->querySP()),
              initBlock));
        else
          initBlock->addInstrBack(new StoreRiscvInst(
              new IRType(IRType::FloatTyID), reg,
              new RiscvIntPhiReg(NamefindReg("fp"), rfoo->querySP()),
              initBlock));
      }

    // 分配整体的栈空间，并设置s0为原sp
    initBlock->addInstrFront(new BinaryRiscvInst(
        RiscvInstr::ADDI, getRegOperand("sp"), new RiscvConst(-rfoo->querySP()),
        getRegOperand("fp"),
        initBlock)); // 3: fp <- t0
    initBlock->addInstrFront(new StoreRiscvInst(
        new IRType(IRType::PointerTyID), getRegOperand("fp"),
        new RiscvIntPhiReg(NamefindReg("sp"), fp_sp - rfoo->querySP()),
        initBlock)); // 2: 保护 fp
    initBlock->addInstrFront(new BinaryRiscvInst(
        RiscvInstr::ADDI, getRegOperand("sp"), new RiscvConst(rfoo->querySP()),
        getRegOperand("sp"), initBlock)); // 1: 分配栈帧

    // 扫描所有的返回语句
    for (RiscvBasicBlock *rbb : rfoo->blk)
      for (RiscvInstr *rinstr : rbb->instruction)
        if (rinstr->type_ == rinstr->RET) {
          initRetInstr(rfoo->regAlloca, rinstr, rbb, rfoo);
          break;
        }

    code += rfoo->print();
  }
  return data + code; //返回函数指令
}

// 计算IRType的大小（以字节为单位）
int calcTypeSize(IRType *ty) {
    // 处理数组类型：计算数组总大小
    if (ty->tid_ == IRType::ArrayTyID) {
        auto arrayTy = static_cast<ArrayIRType*>(ty);
        return calcTypeSize(arrayTy->contained_) * arrayTy->num_elements_;
    }
    // 处理基本类型：返回固定大小
    switch (ty->tid_) {
        case IRType::IntegerTyID:
        case IRType::FloatTyID:
            return 4; // 32位整数或浮点数
        case IRType::PointerTyID:
            return 8; // 64位指针
        case IRType::VoidTyID:
            return 0; // 空类型大小为0
        default:
            // 不支持的类型，抛出错误
            std::cerr << "Error: Unsupported type in calcTypeSize: " << ty->tid_ << std::endl;
            assert(false && "Unsupported type in calcTypeSize");
            return 0;
    }
}

const std::map<Instruction::OpID, RiscvInstr::InstrType> toRiscvOp = {
    {Instruction::OpID::Add, RiscvInstr::InstrType::ADD},
    {Instruction::OpID::Sub, RiscvInstr::InstrType::SUB},
    {Instruction::OpID::Mul, RiscvInstr::InstrType::MUL},
    {Instruction::OpID::SDiv, RiscvInstr::InstrType::DIV},
    {Instruction::OpID::SRem, RiscvInstr::InstrType::REM},
    {Instruction::OpID::FAdd, RiscvInstr::InstrType::FADD},
    {Instruction::OpID::FSub, RiscvInstr::InstrType::FSUB},
    {Instruction::OpID::FMul, RiscvInstr::InstrType::FMUL},
    {Instruction::OpID::FDiv, RiscvInstr::InstrType::FDIV},
    {Instruction::OpID::Ret, RiscvInstr::InstrType::RET},
    {Instruction::OpID::ICmp, RiscvInstr::InstrType::ICMP},
    {Instruction::OpID::FCmp, RiscvInstr::InstrType::FCMP},
    {Instruction::OpID::Call, RiscvInstr::InstrType::CALL},
    {Instruction::OpID::SItoFP, RiscvInstr::InstrType::SITOFP},
    {Instruction::OpID::FPtoSI, RiscvInstr::InstrType::FPTOSI},
    {Instruction::OpID::Or, RiscvInstr::InstrType::OR},
    {Instruction::OpID::And, RiscvInstr::InstrType::AND},
    {Instruction::OpID::Shl, RiscvInstr::InstrType::SHL},
    {Instruction::OpID::LShr, RiscvInstr::InstrType::LSHR},
    {Instruction::OpID::AShr, RiscvInstr::InstrType::ASHR},
    {Instruction::OpID::Load, RiscvInstr::InstrType::LW},
    {Instruction::OpID::Store, RiscvInstr::InstrType::SW},
};
