#pragma once
#include "cgen.hh"
#include <set>
using std::pair;
using std::set;

/////////////////////////////////////////////////////////////////
/////                   Liveness Info
/////////////////////////////////////////////////////////////////


template <int RegisterTagVal>
std::vector<Register *> getUses(MInstruction *ins) {
  std::vector<Register *> uses;
  switch (ins->getInsTag()) {
  case MInstruction::H_PHI: {
    assert(0);
    break;
  }
  case MInstruction::H_CALL: {
    auto call = static_cast<MHIcall *>(ins);
    for (int i = 0; i < call->getArgNum(); i++) {
      auto arg = call->getArg(i);
      if (arg.tp == MIOprandTp::Reg &&
          arg.arg.reg->getTag() == RegisterTagVal) {
        uses.push_back(arg.arg.reg);
      }
    }
    break;
  }
  case MInstruction::H_RET: {
    auto ret = static_cast<MHIret *>(ins);
    auto retv = ret->r;
    if (retv.tp == MIOprandTp::Reg &&
        retv.arg.reg->getTag() == RegisterTagVal) {
      uses.push_back(retv.arg.reg);
    }
    break;
  }
  default: {
    for (int i = 0; i < ins->getRegNum(); i++) {
      auto reg = ins->getReg(i);
      if (reg->getTag() == RegisterTagVal) {
        uses.push_back(reg);
      }
    }
    break;
  }
  }
  return uses;
}

template <int RegisterTagVal>
std::vector<Register *> getDefs(MInstruction *ins) {
  std::vector<Register *> defs;
  if (ins->getTarget() != nullptr &&
      ins->getTarget()->getTag() == RegisterTagVal) {
    defs.push_back(ins->getTarget());
  }
  return defs;
}

template <int RegisterTagVal>
std::vector<Register *> getPhiDefs(MBasicBlock *bb) {
  std::vector<Register *> defs;
  for (auto &phi : bb->getPhis()) {
    if (phi->getTarget()->getTag() == RegisterTagVal) {
      defs.push_back(phi->getTarget());
    }
  }
  return defs;
}

using GET_USES = vector<Register *> (*)(MInstruction *);
using GET_DEFS = vector<Register *> (*)(MInstruction *);
using GET_PHIS = vector<Register *> (*)(MBasicBlock *);

void printLivenessInfo(MFunction *func, LivenessInfo *liveness_ireg,
                       LivenessInfo *liveness_freg);

/////////////////////////////////////////////////////////////////
/////                   Functions
/////////////////////////////////////////////////////////////////

void fixRange(MFunction *mfunc);
void out_of_ssa(MFunction *func, LivenessInfo *liveness_ireg, LivenessInfo *liveness_freg, map<Register *, Register *>* alllocation);
void rewrite_program_spill(MFunction *func, map<Register *, int> *spill);
void rewrite_program_allocate(MFunction *func,
                              map<Register *, Register *> *allocation);
void lower_alloca(MFunction *func, int &stack_offset);
void lower_call(MFunction *func, int &stack_offset,
                map<Register *, Register *> *allocation,
                map<Register *, int> *spill,

                LivenessInfo *liveness_ireg, LivenessInfo *liveness_freg);
void add_prelude(MFunction *func, map<Register *, Register *> *allocation,
                 map<Register *, int> *spill, int stack_offset,
                 set<Register *> *callee_saved);
void add_conclude(MFunction *func, map<Register *, Register *> *allocation,
                  map<Register *, int> *spill, int stack_offset,
                  set<Register *> *callee_saved);

// https://stackoverflow.com/questions/3407012/rounding-up-to-the-nearest-multiple-of-a-number
static int roundUp(int numToRound, int multiple) {
  assert(multiple && ((multiple & (multiple - 1)) == 0));
  return (numToRound + multiple - 1) & -multiple;
}

/////////////////////////////////////////////////////////////////
/////                   Register Op
/////////////////////////////////////////////////////////////////

#define MAX_I_REG_NUM 23 // without t0 t1 t2, zero ra sp gp tp s0
#define MAX_F_REG_NUM 29 // without ft0 ft1 ft2

Register *getOneFRegiter(set<Register *> *used);
Register *getOneIRegiter(set<Register *> *used);
set<Register *>
getActuallCalleeSavedRegisters(map<Register *, Register *> *allocation);
set<Register *>
getActuallCallerSavedRegisters(map<Register *, Register *> *allocation,
                               set<Register *> liveInOfcallsite);
