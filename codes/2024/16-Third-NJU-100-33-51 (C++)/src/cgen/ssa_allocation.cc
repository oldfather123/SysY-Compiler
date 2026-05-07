#include "DomTree.hh"
#include "allocate_register.hh"
#include <set>

///////////////////////////////////////
///////////////////////////////////////
/////////  Liveness Analysis
///////////////////////////////////////
///////////////////////////////////////

static void scan_back(Register *r, MBasicBlock *bb,
                      vector<MInstruction *> instrs, int pos,
                      map<MInstruction *, set<Register *>> *liveness,
                      GET_DEFS get_defs, GET_PHIS get_phis) {
  // std::cout << "Back scan " << r->getName() << " of " << bb->getName() <<
  // endl;
  for (int i = pos; i >= 0; i--) {
    auto ins = instrs[i];
    if ((*liveness)[ins].find(r) != (*liveness)[ins].end()) {
      return;
    }

    (*liveness)[ins].insert(r);

    // std::cout << "  check def or " << *ins << endl;
    vector<Register *> defRegs = get_defs(ins);
    for (auto defReg : defRegs) {
      // std::cout << "    check" << defReg->getName() << endl;
      if (defReg == r) {
        return;
      }
    }
  }
  vector<Register *> phiDefs = get_phis(bb);
  for (auto phiDef : phiDefs) {
    if (phiDef == r) {
      return;
    }
  }
  for (auto pred : bb->getIncomings()) {
    scan_back(r, pred, pred->getAllInstructions(),
              pred->getAllInstructions().size() - 1, liveness, get_defs,
              get_phis);
  }
}

// todo: two pass liveness analysis:
// https://inria.hal.science/inria-00558509v2/document
// https://dl.acm.org/doi/abs/10.1007/978-3-319-07959-2_15
// We use a simple Path Exploration Algorithm currently
void ssa_liveness_analysis(MFunction *func, LivenessInfo *liveness_i,
                           LivenessInfo *liveness_f) {
#define VI Register::V_IREGISTER
#define VF Register::V_FREGISTER
  for (auto bb : func->getBasicBlocks()) {
    auto instrs = bb->getAllInstructions();
    for (auto ins : instrs) {
      (*liveness_i)[ins] = set<Register *>();
      (*liveness_f)[ins] = set<Register *>();
    }
  }

  for (auto bb : func->getBasicBlocks()) {
    auto instrs = bb->getAllInstructions(); // without phi
    for (int i = static_cast<int>(instrs.size()) - 1; i >= 0; i--) {
      auto ins = instrs[i];
      for (auto r : getUses<VI>(ins)) {
        scan_back(r, bb, instrs, i, liveness_i, getDefs<VI>, getPhiDefs<VI>);
      }
      for (auto r : getUses<VF>(ins)) {
        scan_back(r, bb, instrs, i, liveness_f, getDefs<VF>, getPhiDefs<VF>);
      }
    }
    // Definition 9.1 (Liveness for φ-Function Operands—Multiplexing Mode) For a
    // φ-function a0 = φ(a1, . . . , an) in block B0, where ai comes from block
    // Bi:
    // + Its definition-operand is considered to be at the entry of B0, in
    // other words variable a0 is live-in of B0.
    // + Its use operands are at the
    // exit of the corresponding direct predecessor basic blocks, in other
    // words, variable ai is live-out of basic block Bi
    for (auto &phi : bb->getPhis()) {
      for (int i = 0; i < phi->getOprandNum(); i++) {
        auto pred = phi->getIncomingBlock(i);
        auto opd = phi->getOprand(i);
        if (opd.tp == MIOprandTp::Reg) {
          if (opd.arg.reg->getTag() == Register::V_IREGISTER) {
            scan_back(opd.arg.reg, pred, pred->getAllInstructions(),
                      pred->getAllInstructions().size() - 1, liveness_i,
                      getDefs<VI>, getPhiDefs<VI>);
          } else {
            assert(opd.arg.reg->getTag() == Register::V_FREGISTER);
            scan_back(opd.arg.reg, pred, pred->getAllInstructions(),
                      pred->getAllInstructions().size() - 1, liveness_f,
                      getDefs<VF>, getPhiDefs<VF>);
          }
        }
      }
    }
  }
}

///////////////////////////////////////
///////////////////////////////////////
/////////  spill Register
///////////////////////////////////////
///////////////////////////////////////

#include <algorithm>

struct RegisterComparator {
  map<Register *, unsigned int> *reg_cost;
  bool operator()(Register *a, Register *b) {
    assert(reg_cost);
    int a_cost = 0;
    int b_cost = 0;
    if (reg_cost->find(a) != reg_cost->end()) {
      a_cost = reg_cost->at(a);
    }
    if (reg_cost->find(b) != reg_cost->end()) {
      b_cost = reg_cost->at(b);
    }
    return a_cost > b_cost;
  }
};

template <int MAX_REG_NUM>
static void spillRegisters(std::map<Register *, int> *spilled,
                           LivenessInfo *liveness, int &stackOffset,
                           map<Register *, unsigned int> *reg_cost) {
  for (auto &pair : *liveness) {
    std::vector<Register *> regs;
    // std::cout << "consider spill for " << *pair.first << endl;
    for (auto reg : pair.second) {
      // std::cout << "    " << reg->getName() << endl;
      if (spilled->find(reg) == spilled->end()) {
        regs.push_back(reg);
      }
    }

    if (regs.size() <= MAX_REG_NUM) {
      continue;
    }
    auto cmp = RegisterComparator();
    cmp.reg_cost = reg_cost;
    std::sort(regs.begin(), regs.end(), cmp);
    // std::cout << endl;
    while (regs.size() >= MAX_REG_NUM) {
      auto reg = static_cast<VRegister *>(regs.back());
      regs.pop_back();
      if (!reg->isInstruction() &&
          static_cast<ParaRegister *>(reg)->getRegister() == nullptr) {
        (*spilled)[reg] = -static_cast<ParaRegister *>(reg)->getOffset();
      } else {
        if (reg->isPointer()) {
          stackOffset += 8;
        } else {
          stackOffset += 4;
        }
        (*spilled)[reg] = stackOffset;
      }
    }
  }
}

unique_ptr<map<Register *, unsigned int>> cal_reg_cost(MFunction *func) {
  auto reg_cost = make_unique<map<Register *, unsigned int>>();
  for (auto bb : func->getBasicBlocks()) {
    int depth = 0;
    if (func->bbDepth->find(bb) != func->bbDepth->end()) {
      depth = func->bbDepth->at(bb);
    }
    // std::cout << " depth of " << bb->getName() << " is " << depth << endl;
    unsigned int base = 1;
    for (int i = 0; i < depth; i++) {
      base *= 10;
    }
    // std::cout << " base of " << bb->getName() << " is " << base << endl;
    for (auto next : bb->getOutgoings()) {
      for (auto ins : next->getPhis()) {
        auto phi = static_cast<MHIphi *>(ins);
        for (int i = 0; i < phi->getOprandNum(); i++) {
          if (phi->getIncomingBlock(i) == bb) {
            if (reg_cost->find(phi->getTarget()) == reg_cost->end()) {
              reg_cost->insert({phi->getTarget(), 0});
            }
            (*reg_cost)[phi->getTarget()] =
                (*reg_cost)[phi->getTarget()] + base;
            auto opd = phi->getOprand(i);
            if (opd.tp == MIOprandTp::Reg) {
              auto reg = opd.arg.reg;
              if (reg_cost->find(reg) == reg_cost->end()) {
                reg_cost->insert({reg, 0});
              }
              (*reg_cost)[reg] = (*reg_cost)[reg] + base;
            }
          }
        }
      }
    }

    for (auto ins : bb->getAllInstructions()) {
      auto iused = getUses<Register::V_IREGISTER>(ins);
      auto iwrite = getDefs<Register::V_IREGISTER>(ins);
      auto fused = getUses<Register::V_FREGISTER>(ins);
      auto fwrite = getDefs<Register::V_IREGISTER>(ins);
#define LOOP_REG(REGS)                                                         \
  for (auto i : REGS) {                                                        \
    if (reg_cost->find(i) == reg_cost->end()) {                                \
      reg_cost->insert({i, 0});                                                \
    }                                                                          \
    (*reg_cost)[i] = (*reg_cost)[i] + base;                                    \
  }
      LOOP_REG(iused);
      LOOP_REG(iwrite);
      LOOP_REG(fused);
      LOOP_REG(fwrite);
    }
  }
  return reg_cost;
}

static void spill_registers(int &stk_offset, std::map<Register *, int> *spilled,
                            LivenessInfo *liveness_ireg,
                            LivenessInfo *liveness_freg,
                            map<Register *, unsigned int> *reg_cost) {
  spillRegisters<MAX_F_REG_NUM>(spilled, liveness_freg, stk_offset, reg_cost);
  spillRegisters<MAX_I_REG_NUM>(spilled, liveness_ireg, stk_offset, reg_cost);
}

static void setUsedtoLiveOut(MInstruction *ins, set<Register *> *used,
                            map<Register *, Register *> *allocation,
                            LivenessInfo *liveness_ireg,
                            LivenessInfo *liveness_freg,
                            std::map<Register *, int> *spilled) {
  used->clear();
  // std::cout << "Check line IN for " << *ins << endl;
  auto iregs = liveness_ireg->at(ins);
  auto fregs = liveness_freg->at(ins);
  for (auto reg : iregs) {
    // std::cout << "  Check line IN " << reg->getName() << endl;
    // todo: read ssa book to know how this works
    if (allocation->find(reg) != allocation->end()) {
      // std::cout << "   Insert line IN " << reg->getName() << "->" << endl;
      // << allocation->at(reg)->getName() << endl;
      used->insert(allocation->at(reg));
    }
  }
  for (auto reg : fregs) {
    // std::cout << "  Check line IN " << reg->getName() << endl;
    if (spilled->find(reg) == spilled->end() &&
        allocation->find(reg) != allocation->end()) {
      // std::cout << "    Insert line IN " << reg->getName() << "->"
      //           << allocation->at(reg)->getName() << endl;
      used->insert(allocation->at(reg));
    }
  }
}

///////////////////////////////////////
///////////////////////////////////////
/////////  Allocate Register
///////////////////////////////////////
///////////////////////////////////////

void allocatTheRegister(Register *reg, map<Register *, Register *> *allocation,
                        set<Register *> &used) {
  Register *phyreg;
  if (reg->getTag() == Register::V_FREGISTER) {
    phyreg = getOneFRegiter(&used);
  } else {
    phyreg = getOneIRegiter(&used);
  }
  used.insert(phyreg);
  allocation->insert({reg, phyreg});
  // std::cout << "  allocate " << reg->getName() << " to " << phyreg->getName()
}

static void allocateParaRegister(ParaRegister *para,
                                 map<Register *, Register *> *allocation,
                                 set<Register *> &used) {
  if (para->getRegister() != nullptr &&
      used.find(para->getRegister()) == used.end()) {
    allocation->insert({para, para->getRegister()});
    used.insert(para->getRegister());
  } else {
    allocatTheRegister(para, allocation, used);
  }
}

static void allocateParameters(MFunction *func,
                               map<Register *, Register *> *allocation,
                               map<Register *, int> *spill,
                               LivenessInfo *liveness_ireg,
                               LivenessInfo *liveness_freg) {
  set<Register *> used;
  // don't allocate for unused parameters
  auto livei = liveness_ireg->at(func->getEntry()->getAllInstructions().at(0));
  auto livef = liveness_freg->at(func->getEntry()->getAllInstructions().at(0));
  for (int i = 0; i < func->getParaSize(); i++) {
    auto para = func->getPara(i);
    // std::cout << "Allocate Paremeter " << para->getName() << endl;
    if (livei.find(para) != livei.end() || livef.find(para) != livef.end()) {
      if (spill->find(para) == spill->end()) {
        allocateParaRegister(para, allocation, used);
      }
    }
  }
}

// the way we allcate (aggrate all phi together) can increase the register
// pressure on phi.. but make the allocation more simple
void allocatePhis(MBasicBlock *bb, map<Register *, Register *> *allocation,
                  LivenessInfo *liveness_ireg, LivenessInfo *liveness_freg,
                  map<Register *, int> *spill) {
  auto first = bb->getAllInstructions().at(0);
  set<Register *> used;
  setUsedtoLiveOut(first, &used, allocation, liveness_ireg, liveness_freg,
                  spill);
  set<MHIphi *> phis;
  for (auto phi : bb->getPhis()) {
    // std::cout << "  check phi : " << *phi << endl;
    phis.insert(phi);
  }

  for (auto reg : liveness_ireg->at(first)) {
    // std::cout << "    check reg of i: " << reg->getName() << endl;
    if (phis.find(static_cast<MHIphi *>(reg)) != phis.end() &&
        (spill->find(reg) == spill->end())) {
      allocatTheRegister(reg, allocation, used);
    }
  }
  for (auto reg : liveness_freg->at(first)) {
    // std::cout << "    check reg of f: " << reg->getName() << endl;
    if (phis.find(static_cast<MHIphi *>(reg)) != phis.end() &&
        (spill->find(reg) == spill->end())) {
      allocatTheRegister(reg, allocation, used);
    }
  }
}

// without phi
void allocateOtherInstruction(MBasicBlock *bb,
                              map<Register *, Register *> *allocation,
                              LivenessInfo *liveness_ireg,
                              LivenessInfo *liveness_freg,
                              map<Register *, int> *spill) {
  set<Register *> used;
  for (auto instr : bb->getInstructions()) {
    if (instr->getTarget() != nullptr &&
        spill->find(instr->getTarget()) == spill->end()) {
      // todo: we should use LiveOut for normal Instruction, but LiveIn for phi
      // Instruction... but for simplity, we just use LiveIn here...
      setUsedtoLiveOut(instr, &used, allocation, liveness_ireg, liveness_freg,
                      spill);
      // std::cout << "  Used Registers after " << *instr << endl;
      // for (auto u : used) {
      //   std::cout << "    " << u->getName() << endl;
      // }
      allocatTheRegister(instr->getTarget(), allocation, used);
    }
  }
}

void allocateInstructions(MFunction *func,
                          map<Register *, Register *> *allocation,
                          map<Register *, int> *spill,
                          LivenessInfo *liveness_ireg,
                          LivenessInfo *liveness_freg) {

  for (auto bb : *func->domtPreOrder) {
    // std::cout << endl << "Allocate for BB " << bb->getName() << endl;
    allocatePhis(bb, allocation, liveness_ireg, liveness_freg, spill);
    allocateOtherInstruction(bb, allocation, liveness_ireg, liveness_freg,
                             spill);
  }
}

static void allocate(MFunction *func, map<Register *, Register *> *allocation,
                     map<Register *, int> *spill, LivenessInfo *liveness_ireg,
                     LivenessInfo *liveness_freg) {
  allocateParameters(func, allocation, spill, liveness_ireg, liveness_freg);
  allocateInstructions(func, allocation, spill, liveness_ireg, liveness_freg);
}



static void clear_dead_assign_to_call(MFunction* func, LivenessInfo *liveness_ireg,
                     LivenessInfo *liveness_freg) {
  for (auto bb : func->getBasicBlocks()) {
    for (auto ins : bb->getInstructions()) {
      if (ins->getInsTag() == MInstruction::H_CALL) {
        auto target = ins->getTarget();
        if (liveness_freg->at(ins).count(target) == 0 && liveness_ireg->at(ins).count(target) == 0) {
          ins->setTarget(nullptr);
        }
      }
    }
  }

} 

///////////////////////////////////////
///////////////////////////////////////
/////////   Main Entry
///////////////////////////////////////
///////////////////////////////////////
// #include <iomanip>
// void print_stack_layout(int callee_saved_sz, int spilled_sz, int alloca_sz) {
//   std::cout << std::left << std::setw(20) << "Stack Layout:" << std::endl;
//   std::cout << std::left << std::setw(20) << "16 bytes:"
//             << "ra, sp" << std::endl;
//   std::cout << std::left << std::setw(20)
//             << std::to_string(callee_saved_sz) + " bytes:"
//             << "callee saved registers" << std::endl;
//   std::cout << std::left << std::setw(20)
//             << std::to_string(spilled_sz) + " bytes:"
//             << "spilled registers" << std::endl;
//   std::cout << std::left << std::setw(20)
//             << std::to_string(alloca_sz) + " bytes:"
//             << "allocated memory" << std::endl;
// }

// Register Allocation on SSA Form
void allocate_register(MModule *mod) {
  for (auto func : mod->getFunctions()) {
    // step1. Liveness Analysis
    auto liveness_ireg = make_unique<LivenessInfo>();
    auto liveness_freg = make_unique<LivenessInfo>();
    ssa_liveness_analysis(func, liveness_ireg.get(), liveness_freg.get());
    clear_dead_assign_to_call(func, liveness_ireg.get(), liveness_freg.get());

    // std::cout << "Function: " << func->getName() << endl;

    // printLivenessInfo(func, liveness_ireg.get(), liveness_freg.get());

    // step2. Spill
    auto reg_cost = cal_reg_cost(func);

    // for (auto &[reg, cost] : *reg_cost) {
    //   if (reg_cost->find(reg) != reg_cost->end()) {
    //     std::cout << reg->getName() << ": " << reg_cost->at(reg) << endl;
    //   } else {
    //     std::cout << reg->getName() << ": 0" << endl;
    //   }
    // }

    //
    //   std::cout << "  " << logical_reg->getName() << " -> "
    //             << physical_reg->getName() << endl;
    // }

    int offset = 16; // from fp, upward->minus
    auto spill = make_unique<map<Register *, int>>();
    spill_registers(offset, spill.get(), liveness_ireg.get(),
                    liveness_freg.get(), reg_cost.get());
    int spilled_sz = offset - 16; // debug

    // // step3. Allocate
    auto allocation = make_unique<map<Register *, Register *>>();
    allocate(func, allocation.get(), spill.get(), liveness_ireg.get(),
             liveness_freg.get());

    // std::cout << "Register Allocation:" << endl;
    // for (auto &[logical_reg, physical_reg] : *allocation) {
    //   std::cout << "  " << logical_reg->getName() << " -> "
    //             << physical_reg->getName() << endl;
    // }
    // std::cout << endl << endl;

    auto callee_saved = getActuallCalleeSavedRegisters(allocation.get());
    int callee_saved_sz = callee_saved.size() * 8;
    offset += callee_saved_sz;
    for (auto &it : *spill) {
      auto vreg = static_cast<VRegister *>(it.first);
      if (!vreg->isInstruction()) {
        auto para = static_cast<ParaRegister *>(vreg);
        if (para->getRegister() ==
            nullptr) { // already on stack (on top of current func stack)
          continue;
        }
      }
      it.second = it.second + callee_saved_sz;
    }
    // std::cout << "Saved Registers:" << endl;
    // int pof = 16;
    // for (auto reg : callee_saved) {
    //   std::cout << "  " << reg->getName() << " -> " << -pof << endl;
    //   pof += 8;
    // }

    // std::cout << "Spill to Memory:" << endl;
    // for (auto &[reg, offset] : *spill) {
    //   auto size = 4;
    //   if (reg->getTag() == Register::V_FREGISTER ||
    //       reg->getTag() == Register::V_IREGISTER) {
    //     if (static_cast<VRegister *>(reg)->isPointer()) {
    //       size = 8;
    //     }
    //   }
    //   std::cout << "  " << reg->getName() << "(" << size << " bytes) -> "
    //             << -offset << endl;
    // }
    // std::cout << endl;

    // step4. Rewrite program
    out_of_ssa(func, liveness_ireg.get(), liveness_freg.get(),
               allocation.get());

    // int offset_before_alloca = offset; // debug
    lower_alloca(func, offset);

    // int alloca_sz = offset - offset_before_alloca; // debug

    // print_stack_layout(callee_saved_sz, spilled_sz, alloca_sz);

    lower_call(func, offset, allocation.get(), spill.get(), liveness_ireg.get(),
               liveness_freg.get());
    add_prelude(func, allocation.get(), spill.get(), offset, &callee_saved);
    add_conclude(func, allocation.get(), spill.get(), offset, &callee_saved);
    rewrite_program_allocate(func, allocation.get());
    rewrite_program_spill(func, spill.get());
    fixRange(func);
  }
  mod->ssa_out();
}