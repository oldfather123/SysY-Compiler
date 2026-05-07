#include "MInstruction.hh"
#include "Machine.hh"

//////////////////////////////////////
//////////////////////////////////////
///      MachineInstruction        ///
//////////////////////////////////////
//////////////////////////////////////

MInstruction::MInstruction(MITag mt, RegTag rt, string name, bool is_pointer)
    : VRegister(rt, name, is_pointer, true) {
  oprands = make_unique<vector<Register *>>();
  this->insTag = mt;
}

MInstruction::MInstruction(MITag insTag, RegTag rt, bool is_pointer)
    : VRegister(rt, is_pointer, true) {
  oprands = make_unique<vector<Register *>>();
  this->insTag = insTag;
}

void MInstruction::setTarget(Register *reg) {
  if (target != nullptr) {
    target->removeUse(this);
    target = nullptr;
  }
  target = reg;
  if (target != nullptr) {
    target->addUse(this);
  }
}

Register *MInstruction::getTarget() { return target; }

void MInstruction::setComment(string comment) { this->comment = comment; }

string MInstruction::getComment() { return comment; }

void MInstruction::replaceIRRegister(map<Instruction *, Register *> instr_map) {
  for (auto &opd : *oprands) {
    if (opd->getTag() == IR_REGISTER) {
      auto irr = static_cast<IRRegister *>(opd);
      Instruction *inst = irr->ir_reg;
      auto it = instr_map.find(inst);
      if (it != instr_map.end()) {
        opd = it->second;
        opd->addUse(this);
        delete irr;
      } else {
        std::cout << "Try to replace " << inst->getName() << endl;
        assert(0);
      }
    }
  }
}

void MInstruction::replaceRegister(Register *oldReg, Register *newReg) {
  if (target == oldReg) {
    target = newReg;
  }

  for (auto it = oprands->begin(); it != oprands->end(); ++it) {
    if (*it == oldReg) {
      *it = newReg;
    }
  }
  oldReg->removeUse(this);
  newReg->addUse(this);
}

unique_ptr<MInstruction>
MInstruction::replaceWith(vector<MInstruction *> instrs) {
  return bb->replaceInstructionWith(this, instrs);
}

void MInstruction::insertBefore(vector<MInstruction *> instrs) {
  bb->insertBeforeInstructionWith(this, instrs);
}

void MInstruction::insertAfter(vector<MInstruction *> instrs) {
  bb->insertAfterInstructionWith(this, instrs);
}

void MInstruction::setBasicBlock(MBasicBlock *bb) { this->bb = bb; }

void MInstruction::pushReg(Register *r) {
  oprands->push_back(r);
  r->addUse(this);
}

int MInstruction::getRegNum() const {
  if (oprands == nullptr) {
    return 0;
  }
  return oprands->size();
}

void MInstruction::setReg(int idx, Register *reg) const {
  if (oprands == nullptr || idx < 0 || idx >= oprands->size()) {
    assert(0);
  }
  (*oprands)[idx] = reg;
}

Register *MInstruction::getReg(int idx) const {
  if (oprands == nullptr || idx < 0 || idx >= oprands->size()) {
    return nullptr;
  }
  return (*oprands)[idx];
}

MInstruction::MITag MInstruction::getInsTag() const { return insTag; }

std::ostream &operator<<(std::ostream &os, MInstruction &obj) {
  return obj.printASM(os);
}

//////////////////////////////////////
//////////////////////////////////////
///  Concrete MachineInstructions  ///
//////////////////////////////////////
//////////////////////////////////////

MHIphi::MHIphi(string name, RegTag rt, bool is_pointer)
    : MInstruction(MInstruction::MITag::H_PHI, rt, name, is_pointer) {
  incoming = make_unique<vector<MBasicBlock *>>();
  opds = make_unique<vector<MIOprand>>();
  setTarget(this);
}

void MHIphi::replaceIRRegister(map<Instruction *, Register *> instr_map) {
  for (int i = 0; i < opds->size(); i++) {
    auto arg = opds->at(i);
    if (arg.tp == MIOprandTp::Reg &&
        arg.arg.reg->getTag() == Register::IR_REGISTER) {
      auto irr = static_cast<IRRegister *>(arg.arg.reg);
      Instruction *inst = irr->ir_reg;
      auto it = instr_map.find(inst);
      if (it != instr_map.end()) {
        opds->at(i) = MIOprand{MIOprandTp::Reg, arg : {reg : it->second}};
        it->second->addUse(this);
        delete irr;
      } else {
        std::cout << "Try to replace " << inst->getName() << endl;
        assert(0);
      }
    }
  }
}

void MHIphi::replaceRegister(Register *oldReg, Register *newReg) {
  if (getTarget() == oldReg) {
    setTarget(newReg);
  }
  for (auto it = opds->begin(); it != opds->end(); ++it) {
    if (it->tp == MIOprandTp::Reg && it->arg.reg == oldReg) {
      *it = MIOprand{MIOprandTp::Reg, arg : {reg : newReg}};
    }
  }
  oldReg->removeUse(this);
  newReg->addUse(this);
}

ostream &MHIphi::printASM(ostream &os) {
  os << getTarget()->getName() << " = phi ";
  for (int i = 0; i < this->getOprandNum(); i++) {
    string opd;
    if (this->getOprand(i).tp == MIOprandTp::Reg) {
      opd = this->getOprand(i).arg.reg->getName();
    } else if (this->getOprand(i).tp == MIOprandTp::Float) {
      opd = std::to_string(this->getOprand(i).arg.f);
    } else {
      opd = std::to_string(this->getOprand(i).arg.i);
    }
    os << "[" << opd << "," << this->getIncomingBlock(i)->getName() + "]";
    if (i < this->getOprandNum() - 1) {
      os << ", ";
    }
  }
  return os;
}

void MHIphi::pushIncoming(Register *reg, MBasicBlock *bb) {
  MIOprand opd;
  opd.tp = MIOprandTp::Reg;
  opd.arg.reg = reg;
  opds->push_back(opd);
  incoming->push_back(bb);
}

void MHIphi::pushIncoming(float f, MBasicBlock *bb) {
  MIOprand opd;
  opd.tp = MIOprandTp::Float;
  opd.arg.f = f;
  opds->push_back(opd);
  incoming->push_back(bb);
}

void MHIphi::pushIncoming(int i, MBasicBlock *bb) {
  MIOprand opd;
  opd.tp = MIOprandTp::Int;
  opd.arg.i = i;
  opds->push_back(opd);
  incoming->push_back(bb);
}

void MHIphi::replaceIncoming(MBasicBlock *oldbb, MBasicBlock *newbb) {
  auto it = incoming->begin();
  while (it != incoming->end()) {
    if (*it == oldbb) {
      *it = newbb;
      break;
    }
    ++it;
  }
}

MBasicBlock *MHIphi::getIncomingBlock(int idx) const {
  if (idx < 0 || idx >= incoming->size()) {
    return nullptr;
  }
  return (*incoming)[idx];
}

////////////////////////////////////////////

MHIalloca::MHIalloca(uint32_t size_, string name)
    : MInstruction(MInstruction::MITag::H_ALLOCA, RegTag::V_IREGISTER, name,
                   true) {
  size = size_;
  this->setTarget(this);
}

ostream &MHIalloca::printASM(ostream &os) {
  return os << getTarget()->getName() << " = alloca " << std::to_string(size);
}
////////////////////////////////////////////

MHIret::MHIret() : MInstruction(MInstruction::MITag::H_RET, RegTag::NONE) {
  this->r.tp = MIOprandTp::None;
}

MHIret::MHIret(int imm)
    : MInstruction(MInstruction::MITag::H_RET, RegTag::NONE) {
  this->r.tp = MIOprandTp::Int;
  this->r.arg.i = imm;
}

MHIret::MHIret(float imm)
    : MInstruction(MInstruction::MITag::H_RET, RegTag::NONE) {
  this->r.tp = MIOprandTp::Float;
  this->r.arg.f = imm;
}
MHIret::MHIret(Register *reg)
    : MInstruction(MInstruction::MITag::H_RET, RegTag::NONE) {
  this->r.tp = MIOprandTp::Reg;
  this->r.arg.reg = reg;
}

void MHIret::replaceIRRegister(map<Instruction *, Register *> instr_map) {
  if (r.tp == MIOprandTp::Reg && r.arg.reg->getTag() == Register::IR_REGISTER) {
    auto irr = static_cast<IRRegister *>(r.arg.reg);
    Instruction *inst = irr->ir_reg;
    auto it = instr_map.find(inst);
    if (it != instr_map.end()) {
      r = MIOprand{MIOprandTp::Reg, arg : {reg : it->second}};
      it->second->addUse(this);
      delete irr;
    } else {
      std::cout << "Try to replace " << inst->getName() << endl;
      assert(0);
    }
  }
}

void MHIret::replaceRegister(Register *oldReg, Register *newReg) {
  if (r.tp == MIOprandTp::Reg && r.arg.reg == oldReg) {
    r = MIOprand{MIOprandTp::Reg, arg : {reg : newReg}};
  }
  oldReg->removeUse(this);
  newReg->addUse(this);
}

ostream &MHIret::printASM(ostream &os) {
  os << "ret ";
  switch (r.tp) {
  case Float:
    os << r.arg.f;
    break;
  case Int:
    os << r.arg.i;
    break;
  case Reg:
    os << r.arg.reg->getName();
    break;
  }
  return os;
}

////////////////////////////////////////////

MHIcall::MHIcall(MFunction *func, string name, RegTag rt)
    : MInstruction(MInstruction::MITag::H_CALL, rt, name) {
  this->function = func;
  this->setTarget(this);
  this->args = make_unique<vector<MIOprand>>();
}

MHIcall::MHIcall(MFunction *func, RegTag rt)
    : MInstruction(MInstruction::MITag::H_CALL, rt) {
  this->function = func;
  this->args = make_unique<vector<MIOprand>>();
}

void MHIcall::replaceIRRegister(map<Instruction *, Register *> instr_map) {
  for (int i = 0; i < args->size(); i++) {
    auto arg = args->at(i);
    if (arg.tp == MIOprandTp::Reg &&
        arg.arg.reg->getTag() == Register::IR_REGISTER) {
      auto irr = static_cast<IRRegister *>(arg.arg.reg);
      Instruction *inst = irr->ir_reg;
      auto it = instr_map.find(inst);
      if (it != instr_map.end()) {
        args->at(i) = MIOprand{MIOprandTp::Reg, arg : {reg : it->second}};
        it->second->addUse(this);
        delete irr;
      } else {
        std::cout << "Try to replace " << inst->getName() << endl;
        assert(0);
      }
    }
  }
}

void MHIcall::replaceRegister(Register *oldReg, Register *newReg) {
  if (getTarget() == oldReg) {
    setTarget(newReg);
  }
  for (auto it = args->begin(); it != args->end(); ++it) {
    if (it->tp == MIOprandTp::Reg && it->arg.reg == oldReg) {
      *it = MIOprand{MIOprandTp::Reg, arg : {reg : newReg}};
    }
  }
  oldReg->removeUse(this);
  newReg->addUse(this);
}

void MHIcall::pushArg(float f) {
  MIOprand arg;
  arg.tp = MIOprandTp::Float;
  arg.arg.f = f;
  this->args->push_back(arg);
}

void MHIcall::pushArg(int i) {
  MIOprand arg;
  arg.tp = MIOprandTp::Int;
  arg.arg.i = i;
  this->args->push_back(arg);
}

void MHIcall::pushArg(Register *r) {
  MIOprand arg;
  arg.tp = MIOprandTp::Reg;
  arg.arg.reg = r;
  this->args->push_back(arg);
}

int MHIcall::getArgNum() { return this->args->size(); }

MIOprand &MHIcall::getArg(int idx) {
  if (idx >= this->args->size()) {
    assert(0);
  }
  return args->at(idx);
}

ostream &MHIcall::printASM(ostream &os) {
  auto target = getTarget();
  if (target != nullptr) {
    os << target->getName() << " = ";
  }
  os << "call " << this->function->getName() << "(";
  for (int i = 0; i < this->args->size(); i++) {
    auto &arg = this->args->at(i);
    switch (arg.tp) {
    case MIOprandTp::Float:
      os << arg.arg.f;
      break;
    case MIOprandTp::Int:
      os << arg.arg.i;
      break;
    case MIOprandTp::Reg:
      os << arg.arg.reg->getName();
      break;
    }
    if (i < this->args->size() - 1) {
      os << ", ";
    }
  }
  os << ")";
  return os;
}

MHIicmp::MHIicmp(OpTag optag, Register *reg1, Register *reg2, std::string name)
    : MInstruction(MInstruction::MITag::H_ICMP, RegTag::V_IREGISTER, name) {
  this->optag = optag;
  pushReg(reg1);
  pushReg(reg2);
  this->setTarget(this);
}

OpTag MHIicmp::getOpTag() { return optag; }

ostream &MHIicmp::printASM(ostream &os) {
  auto target = this->getTarget()->getName();
  auto reg1 = this->getReg(0)->getName();
  auto reg2 = this->getReg(1)->getName();
  switch (optag) {
  case OpTag::EQ: {
    os << "h_eq ";
    break;
  }
  case OpTag::NE: {
    os << "h_ne ";
    break;
  }
  case OpTag::SLE: {
    os << "h_le ";
    break;
  }
  case OpTag::SLT: {
    os << "h_lt ";
    break;
  }
  case OpTag::SGE: {
    os << "h_ge ";
    break;
  }
  case OpTag::SGT: {
    os << "h_gt ";
    break;
  }
  default:
    assert(0);
  }
  return os << reg1 << ", " << reg2;
}

////////////////////////////////////////////

MHIbr::MHIbr(Register *reg, MBasicBlock *t_bb, MBasicBlock *f_bb)
    : MInstruction(MInstruction::MITag::H_BR, RegTag::NONE) {
  pushReg(reg);
  this->t_bb = t_bb;
  this->f_bb = f_bb;
}

void MHIbr::setTBlock(MBasicBlock *bb) { this->t_bb = bb; }
void MHIbr::setFBlock(MBasicBlock *bb) { this->f_bb = bb; }
MBasicBlock *MHIbr::getTBlock() { return t_bb; }
MBasicBlock *MHIbr::getFBlock() { return f_bb; }
ostream &MHIbr::printASM(ostream &os) {
  return os << "br " << this->getReg(0)->getName() << ", "
            << this->t_bb->getName() << ", " << this->f_bb->getName();
}

////////////////////////////////////////////

#define IMPLEMENT_MI_IMM_CLASS(NAME, INS_TAG, REG_TAG, ASM_NAME, IS_POINTER)   \
  MI##NAME::MI##NAME(Register *reg, int32_t imm)                               \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg);                                                        \
    this->imm = imm;                                                           \
    this->setTarget(this);                                                     \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg, int32_t imm, std::string name)             \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, name,             \
                     IS_POINTER) {                                             \
    this->pushReg(reg);                                                        \
    this->imm = imm;                                                           \
    this->setTarget(this);                                                     \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg, int32_t imm, Register *target)             \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg);                                                        \
    this->imm = imm;                                                           \
    this->setTarget(target);                                                   \
  }                                                                            \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    auto target = this->getTarget()->getName();                                \
    auto reg = this->getReg(0)->getName();                                     \
    auto imm = std::to_string(this->imm);                                      \
    return os << #ASM_NAME << " " << target << ", " << reg << ", " << imm;     \
  }

#define IMPLEMENT_MI_BIN_CLASS(NAME, INS_TAG, REG_TAG, ASM_NAME, IS_POINTER)   \
  MI##NAME::MI##NAME(Register *reg1, Register *reg2)                           \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg1);                                                       \
    this->pushReg(reg2);                                                       \
    this->setTarget(this);                                                     \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg1, Register *reg2, Register *target)         \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg1);                                                       \
    this->pushReg(reg2);                                                       \
    this->setTarget(target);                                                   \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg1, Register *reg2, string name)              \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, name,             \
                     IS_POINTER) {                                             \
    this->pushReg(reg1);                                                       \
    this->pushReg(reg2);                                                       \
    this->setTarget(this);                                                     \
  }                                                                            \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    auto target = this->getTarget()->getName();                                \
    auto reg1 = this->getReg(0)->getName();                                    \
    auto reg2 = this->getReg(1)->getName();                                    \
    return os << #ASM_NAME << " " << target << ", " << reg1 << ", " << reg2;   \
  }

#define IMPLEMENT_MI_UNA_CLASS(NAME, INS_TAG, REG_TAG, ASM_NAME, IS_POINTER)   \
  MI##NAME::MI##NAME(Register *reg)                                            \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg);                                                        \
    this->setTarget(this);                                                     \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg, Register *target)                          \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(reg);                                                        \
    this->setTarget(target);                                                   \
  }                                                                            \
  MI##NAME::MI##NAME(Register *reg, string name)                               \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, name,             \
                     IS_POINTER) {                                             \
    this->pushReg(reg);                                                        \
    this->setTarget(this);                                                     \
  }                                                                            \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    auto target = this->getTarget()->getName();                                \
    auto reg = this->getReg(0)->getName();                                     \
    return os << #ASM_NAME << " " << target << ", " << reg;                    \
  }

IMPLEMENT_MI_IMM_CLASS(addi, ADDI, V_IREGISTER, addi, true)
IMPLEMENT_MI_BIN_CLASS(add, ADD, V_IREGISTER, add, true)
IMPLEMENT_MI_IMM_CLASS(addiw, ADDIW, V_IREGISTER, addiw, false)
IMPLEMENT_MI_BIN_CLASS(addw, ADDW, V_IREGISTER, addw, false)
IMPLEMENT_MI_BIN_CLASS(subw, SUBW, V_IREGISTER, subw, false)
IMPLEMENT_MI_BIN_CLASS(and, AND, V_IREGISTER, and, false)
IMPLEMENT_MI_IMM_CLASS(andi, ANDI, V_IREGISTER, andi, false)
IMPLEMENT_MI_BIN_CLASS(or, OR, V_IREGISTER, or, false)
IMPLEMENT_MI_IMM_CLASS(ori, ORI, V_IREGISTER, ori, false)
IMPLEMENT_MI_BIN_CLASS(xor, XOR, V_IREGISTER, xor, false)
IMPLEMENT_MI_IMM_CLASS(xori, XORI, V_IREGISTER, xori, false)
IMPLEMENT_MI_BIN_CLASS(slt, SLT, V_IREGISTER, slt, false)
IMPLEMENT_MI_IMM_CLASS(slti, SLTI, V_IREGISTER, slti, false)
IMPLEMENT_MI_BIN_CLASS(sltu, SLTU, V_IREGISTER, sltu, false)
IMPLEMENT_MI_IMM_CLASS(sltiu, SLTIU, V_IREGISTER, sltiu, false)

// shifts
IMPLEMENT_MI_IMM_CLASS(slli, SLLI, V_IREGISTER, slli, true)
IMPLEMENT_MI_BIN_CLASS(sll, SLL, V_IREGISTER, sll, true)
IMPLEMENT_MI_IMM_CLASS(srli, SRLI, V_IREGISTER, srli, true)
IMPLEMENT_MI_BIN_CLASS(srl, SRL, V_IREGISTER, srl, true)
IMPLEMENT_MI_IMM_CLASS(srai, SRAI, V_IREGISTER, srai, true)
IMPLEMENT_MI_BIN_CLASS(sra, SRA, V_IREGISTER, sra, true)

#define IMPLEMENT_MI_LOAD_CLASS(NAME, INS_TAG, REG_TAG, IS_POINTER)            \
  MI##NAME::MI##NAME(MGlobal *global)                                          \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER),      \
        global(global) {                                                       \
    this->setTarget(this);                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(MGlobal *global, std::string name)                        \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, name,             \
                     IS_POINTER),                                              \
        global(global) {                                                       \
    this->setTarget(this);                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(MGlobal *global, Register *target)                        \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->setTarget(target);                                                   \
    this->global = global;                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(Register *addr, int offset)                               \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(addr);                                                       \
    imm = offset;                                                              \
    this->setTarget(this);                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(Register *addr, int offset, std::string name)             \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, name,             \
                     IS_POINTER) {                                             \
    this->pushReg(addr);                                                       \
    imm = offset;                                                              \
    this->setTarget(this);                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(Register *addr, int offset, Register *target)             \
      : MInstruction(MInstruction::INS_TAG, RegTag::REG_TAG, IS_POINTER) {     \
    this->pushReg(addr);                                                       \
    this->setTarget(target);                                                   \
    imm = offset;                                                              \
  }                                                                            \
                                                                               \
  MGlobal *MI##NAME::getGlobal() { return this->global; }                      \
                                                                               \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    if (this->global) {                                                        \
      std::cout << #NAME << " " << this->getTarget()->getName() << +", "       \
                << this->global->getName();                                    \
      assert(0);                                                               \
    } else {                                                                   \
      return os << #NAME << " " << this->getTarget()->getName() << ", "        \
                << std::to_string(imm) << "(" << this->getReg(0)->getName()    \
                << ")";                                                        \
    }                                                                          \
  }

#define IMPLEMENT_MI_STORE_CLASS(NAME, INS_TAG)                                \
  MI##NAME::MI##NAME(Register *val, MGlobal *global)                           \
      : MInstruction(MInstruction::INS_TAG, RegTag::NONE) {                    \
    assert(0);                                                                 \
    this->pushReg(val);                                                        \
    this->global = global;                                                     \
  }                                                                            \
                                                                               \
  MI##NAME::MI##NAME(Register *val, int offset, Register *addr)                \
      : MInstruction(MInstruction::INS_TAG, RegTag::NONE) {                    \
    this->pushReg(val);                                                        \
    this->pushReg(addr);                                                       \
    imm = offset;                                                              \
  }                                                                            \
                                                                               \
  MGlobal *MI##NAME::getGlobal() {                                             \
    assert(0);                                                                 \
    return this->global;                                                       \
  }                                                                            \
                                                                               \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    if (this->global) {                                                        \
      assert(0);                                                               \
      return os << #NAME << " " << this->getReg(0)->getName() << ", "          \
                << this->global->getName();                                    \
    } else {                                                                   \
      return os << #NAME << " " << this->getReg(0)->getName() << ", "          \
                << std::to_string(imm) << "(" << this->getReg(1)->getName()    \
                << ")";                                                        \
    }                                                                          \
  }

IMPLEMENT_MI_LOAD_CLASS(lw, LW, V_IREGISTER, false)
IMPLEMENT_MI_STORE_CLASS(sw, SW)

IMPLEMENT_MI_LOAD_CLASS(ld, LD, V_IREGISTER, true)
IMPLEMENT_MI_STORE_CLASS(sd, SD)

IMPLEMENT_MI_LOAD_CLASS(flw, FLW, V_FREGISTER, false)
IMPLEMENT_MI_STORE_CLASS(fsw, FSW)
IMPLEMENT_MI_LOAD_CLASS(fld, FLD, V_FREGISTER, false)
IMPLEMENT_MI_STORE_CLASS(fsd, FSD)

IMPLEMENT_MI_BIN_CLASS(mul, MUL, V_IREGISTER, mul, true)
IMPLEMENT_MI_BIN_CLASS(mulw, MULW, V_IREGISTER, mulw, false)
IMPLEMENT_MI_BIN_CLASS(divw, DIVW, V_IREGISTER, divw, false)
IMPLEMENT_MI_BIN_CLASS(remw, REMW, V_IREGISTER, remw, false)

IMPLEMENT_MI_BIN_CLASS(fadd_s, FADD_S, V_FREGISTER, fadd.s, false)
IMPLEMENT_MI_BIN_CLASS(fsub_s, FSUB_S, V_FREGISTER, fsub.s, false)
IMPLEMENT_MI_BIN_CLASS(fmul_s, FMUL_S, V_FREGISTER, fmul.s, false)
IMPLEMENT_MI_BIN_CLASS(fdiv_s, FDIV_S, V_FREGISTER, fdiv.s, false)

IMPLEMENT_MI_UNA_CLASS(fmvs_x, FMV_S_X, V_FREGISTER, fmv.s.x, false)

IMPLEMENT_MI_UNA_CLASS(fcvts_w, FCVTS_W, V_FREGISTER, fcvt.s.w, false)
// IMPLEMENT_MI_UNA_CLASS(fcvtw_s, FCVTW_S, V_IREGISTER, fcvt.w.s, false)

// add rtz mode
MIfcvtw_s::MIfcvtw_s(Register *reg)
    : MInstruction(MInstruction::FCVTW_S, RegTag::V_IREGISTER, false) {
  this->pushReg(reg);
  this->setTarget(this);
}
MIfcvtw_s::MIfcvtw_s(Register *reg, Register *target)
    : MInstruction(MInstruction::FCVTW_S, RegTag::V_IREGISTER, false) {
  this->pushReg(reg);
  this->setTarget(target);
}
MIfcvtw_s::MIfcvtw_s(Register *reg, string name)
    : MInstruction(MInstruction::FCVTW_S, RegTag::V_IREGISTER, name, false) {
  this->pushReg(reg);
  this->setTarget(this);
}
ostream &MIfcvtw_s::printASM(ostream &os) {
  auto target = this->getTarget()->getName();
  auto reg = this->getReg(0)->getName();
  return os << "fcvt.w.s"
            << " " << target << ", " << reg << ", rtz";
}

IMPLEMENT_MI_BIN_CLASS(feq_s, FEQ_S, V_IREGISTER, feq.s, false)
IMPLEMENT_MI_BIN_CLASS(flt_s, FLT_S, V_IREGISTER, flt.s, false)
IMPLEMENT_MI_BIN_CLASS(fle_s, FLE_S, V_IREGISTER, fle.s, false)

#define IMPLEMENT_MIT_BRANCH_CLASS(NAME, TAG)                                  \
  MI##NAME::MI##NAME(Register *reg1, Register *reg2, MBasicBlock *targetBB)    \
      : MInstruction(MInstruction::TAG, RegTag::NONE), targetBB(targetBB) {    \
    this->pushReg(reg1);                                                       \
    this->pushReg(reg2);                                                       \
  }                                                                            \
                                                                               \
  MBasicBlock *MI##NAME::getTargetBB() { return this->targetBB; }              \
  void MI##NAME::setTargetBB(MBasicBlock *bb) { this->targetBB = bb; }         \
  ostream &MI##NAME::printASM(ostream &os) {                                   \
    return os << #NAME << " " << this->getReg(0)->getName() << ", "            \
              << this->getReg(1)->getName() << ", "                            \
              << this->targetBB->getName();                                    \
  }

IMPLEMENT_MIT_BRANCH_CLASS(beq, BEQ)
IMPLEMENT_MIT_BRANCH_CLASS(bne, BNE)
IMPLEMENT_MIT_BRANCH_CLASS(bge, BGE)
IMPLEMENT_MIT_BRANCH_CLASS(blt, BLT)

MIj::MIj(MBasicBlock *targetBB)
    : MInstruction(MInstruction::J, RegTag::NONE), targetBB(targetBB) {}

void MIj::setTargetBB(MBasicBlock *bb) { this->targetBB = bb; }

MBasicBlock *MIj::getTargetBB() { return this->targetBB; }

ostream &MIj::printASM(ostream &os) {
  return os << "j " << this->targetBB->getName();
}

// MIcall
MIcall::MIcall(MFunction *func)
    : MInstruction(MInstruction::CALL, RegTag::NONE), func(func) {}

MFunction *MIcall::getFunc() { return this->func; }

ostream &MIcall::printASM(ostream &os) {
  return os << "call " << this->func->getName();
}

// MIret
MIret::MIret() : MInstruction(MInstruction::RET, RegTag::NONE) {}

ostream &MIret::printASM(ostream &os) { return os << "ret"; }

// MIli
MIli::MIli(int32_t imm) : MInstruction(MInstruction::LI, RegTag::V_IREGISTER) {
  this->imm = imm;
  this->setTarget(this);
}

MIli::MIli(int32_t imm, string name)
    : MInstruction(MInstruction::LI, RegTag::V_IREGISTER, name) {
  this->imm = imm;
  this->setTarget(this);
}

MIli::MIli(int32_t imm, Register *target)
    : MInstruction(MInstruction::LI, RegTag::V_IREGISTER) {
  this->imm = imm;
  this->setTarget(target);
}

ostream &MIli::printASM(ostream &os) {
  return os << "li "
            << this->getTarget()->getName() + ", " + std::to_string(imm);
}

// MIla
MIla::MIla(MGlobal *g)
    : MInstruction(MInstruction::LA, RegTag::V_IREGISTER, true) {
  this->g = g;
  this->setTarget(this);
}

MIla::MIla(MGlobal *g, string name)
    : MInstruction(MInstruction::LA, RegTag::V_IREGISTER, name, true) {
  this->g = g;
  this->setTarget(this);
}

MIla::MIla(MGlobal *g, Register *target)
    : MInstruction(MInstruction::LA, RegTag::V_IREGISTER, true) {
  this->g = g;
  this->setTarget(target);
}

ostream &MIla::printASM(ostream &os) {
  return os << "la "
            << this->getTarget()->getName() + ", " + this->g->getName();
}

IMPLEMENT_MI_UNA_CLASS(mv, MV, V_IREGISTER, mv, false)
IMPLEMENT_MI_UNA_CLASS(fmv_s, FMV_S, V_FREGISTER, fmv.s, false)

Mcomment::Mcomment(string comment)
    : MInstruction(MInstruction::COMMENT, RegTag::NONE) {
  this->comment = comment;
}
ostream &Mcomment::printASM(ostream &os) { return os << "# " << comment; }