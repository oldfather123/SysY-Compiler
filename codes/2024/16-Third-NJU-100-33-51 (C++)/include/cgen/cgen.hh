#pragma once
#include "Machine.hh"
#include "Module.hh"

using LivenessInfo = map<MInstruction *, set<Register *>>;

void generate_code(MModule *res, ANTPIE::Module *ir, bool opt);
void select_instruction(MModule *res, ANTPIE::Module *ir);
void ssa_liveness_analysis(MFunction *func, LivenessInfo *liveness_i,
                           LivenessInfo *liveness_f);
void allocate_register(MModule *mod);
void spill_all_register(MModule *mod);
void schedule_instruction(MModule *mod);
void peephole_optimize(MModule *mod);
void branch_simplify(MModule *mod);

static bool is_mem_read(MInstruction *ins) {
  switch (ins->getInsTag()) {
  case MInstruction::LD:
  case MInstruction::LW:
  case MInstruction::FLD:
  case MInstruction::FLW:
    return true;
  default:
    return false;
  }
}

static bool is_mem_write(MInstruction *ins) {
  switch (ins->getInsTag()) {
  case MInstruction::SD:
  case MInstruction::SW:
  case MInstruction::FSD:
  case MInstruction::FSW:
    return true;
  default:
    return false;
  }
}

static bool is_mem_op(MInstruction *ins) {
  return is_mem_read(ins) || is_mem_write(ins);
}

static bool is_float_op(MInstruction *ins) {
  switch (ins->getInsTag()) {
  case MInstruction::FEQ_S:
  case MInstruction::FLT_S:
  case MInstruction::FLE_S:
  case MInstruction::FCVTW_S:
  case MInstruction::FCVTS_W:
  case MInstruction::FLW:
  case MInstruction::FSW:
  case MInstruction::FLD:
  case MInstruction::FSD:
  case MInstruction::FMV_S_X:
  case MInstruction::FADD_S:
  case MInstruction::FSUB_S:
  case MInstruction::FMUL_S:
  case MInstruction::FDIV_S:
    return true;
  default:
    return false;
  }
}


static bool is_mul_div(MInstruction *ins) {
  switch (ins->getInsTag()) {
  case MInstruction::MUL:
  case MInstruction::MULW:
  case MInstruction::DIVW:
  case MInstruction::REMW:
  case MInstruction::FMUL_S:
  case MInstruction::FDIV_S:
    return true;
  default:
    return false;
  }
}