#include "allocate_register.hh"
#include <sstream>

///////////////////////////////////////
///////////////////////////////////////
/////////  Liveness Info
///////////////////////////////////////
///////////////////////////////////////
template std::vector<Register *>
getUses<Register::F_REGISTER>(MInstruction *ins);
template std::vector<Register *>
getUses<Register::I_REGISTER>(MInstruction *ins);
template std::vector<Register *>
getUses<Register::V_FREGISTER>(MInstruction *ins);
template std::vector<Register *>
getUses<Register::V_IREGISTER>(MInstruction *ins);
template std::vector<Register *>
getDefs<Register::F_REGISTER>(MInstruction *ins);
template std::vector<Register *>
getDefs<Register::I_REGISTER>(MInstruction *ins);
template std::vector<Register *>
getDefs<Register::V_FREGISTER>(MInstruction *ins);
template std::vector<Register *>
getDefs<Register::V_IREGISTER>(MInstruction *ins);
template std::vector<Register *>
getPhiDefs<Register::V_FREGISTER>(MBasicBlock *bb);
template std::vector<Register *>
getPhiDefs<Register::V_IREGISTER>(MBasicBlock *bb);

void printLivenessInfo(MFunction *func, LivenessInfo *liveness_ireg,
                       LivenessInfo *liveness_freg) {
  std::cout << "Liveness Information for Function: " << func->getName()
            << std::endl;

  std::cout << "Integer Register Liveness:" << std::endl;
  for (auto &pair : *liveness_ireg) {
    std::cout << "Instruction: " << *pair.first << std::endl;
    std::cout << "   LiveOut (" << pair.second.size() << " regs):  ";
    for (Register *reg : pair.second) {
      std::cout << reg->getName() << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl << std::endl;

  std::cout << "Floating-Point Register Liveness:" << std::endl;
  for (auto &pair : *liveness_freg) {
    std::cout << "Instruction: " << *pair.first << std::endl;
    std::cout << "   LiveOut: ";
    for (Register *reg : pair.second) {
      std::cout << reg->getName() << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl << std::endl;
}

///////////////////////////////////////
///////////////////////////////////////
/////////  Functions
///////////////////////////////////////
///////////////////////////////////////

// todo!!!!!!! use better algorithms!!!!!
vector<MInstruction *> solveParallelAssignment(vector<MInstruction *> instrs) {
  // ignore t0, t1, t2, ft0, ft1, ft2
  // store the registers used which could be write latter to stack
  // and then load from stack when using this register...

  // std::cout << "\n\n\nsolveParallelAssignment" << endl;
  // for (auto instr : instrs) {
  //   std::cout << *instr << endl;
  // }
  static set<Register *> temp_registers = {
      Register::reg_t0,  Register::reg_t1,  Register::reg_t2,
      Register::reg_ft0, Register::reg_ft1, Register::reg_ft2,
  };

  set<Register *> defs;
  set<Register *> needStoreRegs;

  for (auto instr : instrs) {
    for (int i = 0; i < instr->getRegNum(); i++) {
      Register *reg = instr->getReg(i);
      if (temp_registers.find(reg) != temp_registers.end()) {
        continue;
      }
      if (defs.find(reg) != defs.end()) {
        needStoreRegs.insert(reg);
      }
    }
    if (instr->getTarget() != nullptr) {
      defs.insert(instr->getTarget());
    }
  }

  // create store for defined registers
  vector<MInstruction *> stores;
  map<Register *, int> addr;
  int offset = 0;
  for (auto reg : needStoreRegs) {
    if (reg->getTag() == Register::I_REGISTER) {
      offset += 8;
      stores.emplace_back(new MIsd(reg, -offset, Register::reg_sp));
      addr.insert({reg, offset});
    } else if (reg->getTag() == Register::F_REGISTER) {
      offset += 8;
      stores.emplace_back(new MIfsd(reg, -offset, Register::reg_sp));
      addr.insert({reg, offset});
    } else if (reg->getTag() == Register::V_FREGISTER) {
      offset += 4;
      stores.emplace_back(new MIfsw(reg, -offset, Register::reg_sp));
      addr.insert({reg, offset});
    } else if (reg->getTag() == Register::V_IREGISTER) {
      auto vreg = static_cast<VRegister *>(reg);
      if (vreg->isPointer()) {
        offset += 8;
        stores.emplace_back(new MIsd(reg, -offset, Register::reg_sp));
        addr.insert({reg, offset});
      } else {
        offset += 4;
        stores.emplace_back(new MIsw(reg, -offset, Register::reg_sp));
        addr.insert({reg, offset});
      }
    }
  }

  // replace each instr which have used stored register with a load form temp
  // and use temp
  map<MInstruction *, vector<MInstruction *>> loads_mp;
  for (auto ins : instrs) {
    int float_cnt = 0;
    int int_cnt = 5;
    auto loads = vector<MInstruction *>();
    auto regs = std::vector<Register *>();
    for (int i = 0; i < ins->getRegNum(); i++) {
      auto reg = ins->getReg(i);
      if (needStoreRegs.find(reg) != needStoreRegs.end()) {
        regs.push_back(reg);
      }
    }
    std::set<Register *> unique_regs(regs.begin(), regs.end());
    regs.assign(unique_regs.begin(), unique_regs.end());
    for (auto reg : regs) {
      MInstruction *load;
      if (reg->getTag() == Register::I_REGISTER) {
        auto target = Register::getIRegister(int_cnt++);
        int offset = addr.at(reg);
        ins->replaceRegister(reg, target);
        load = new MIld(Register::reg_sp, -offset, target);
        loads.push_back(load);
      } else if (reg->getTag() == Register::F_REGISTER) {
        auto target = Register::getFRegister(float_cnt++);
        int offset = addr.at(reg);
        ins->replaceRegister(reg, target);
        load = new MIfld(Register::reg_sp, -offset, target);
        loads.push_back(load);
      } else if (reg->getTag() == Register::V_FREGISTER) {
        auto target = Register::getFRegister(float_cnt++);
        int offset = addr.at(reg);
        ins->replaceRegister(reg, target);
        load = new MIflw(Register::reg_sp, -offset, target);
        loads.push_back(load);
      } else if (reg->getTag() == Register::V_IREGISTER) {
        auto vreg = static_cast<VRegister *>(reg);
        if (vreg->isPointer()) {
          auto target = Register::getIRegister(int_cnt++);
          int offset = addr.at(reg);
          ins->replaceRegister(reg, target);
          load = new MIld(Register::reg_sp, -offset, target);
          loads.push_back(load);
        } else {
          auto target = Register::getIRegister(int_cnt++);
          int offset = addr.at(reg);
          ins->replaceRegister(reg, target);
          load = new MIlw(Register::reg_sp, -offset, target);
          loads.push_back(load);
        }
      }
    }

    loads_mp.insert({ins, loads});
  }
  vector<MInstruction *> result;
  for (auto instr : stores) {
    result.push_back(instr);
  }
  for (auto instr : instrs) {
    auto loads = loads_mp.at(instr);
    for (auto load : loads) {
      result.push_back(load);
    }
    result.push_back(instr);
  }

  // std::cout << "\nafter solveParallelAssignment" << endl;
  // for (auto instr : result) {
  //   std::cout << *instr << endl;
  // }

  return result;
}

void fixRange(MFunction *mfunc) {
  for (auto &mbb : mfunc->getBasicBlocks()) {
    vector<MInstruction *> instrs;
    for (auto &ins : mbb->getInstructions()) {
      instrs.push_back(&*ins);
    }
    int lo = -2048;
    int hi = 2047;
    for (auto ins : instrs) {
      if (ins->getInsTag() == MInstruction::ADDI) {
        auto addi = static_cast<MIaddi *>(ins);
        if (addi->imm < lo || addi->imm > hi) {
          auto immr = new MIli(addi->imm, Register::reg_t2);
          auto add =
              new MIadd(Register::reg_t2, addi->getReg(0), addi->getTarget());
          mfunc->reg_pool->push_back(addi->replaceWith({immr, add}));
        }
      } else if (ins->getInsTag() == MInstruction::ADDIW) {
        auto addiw = static_cast<MIaddiw *>(ins);
        if (addiw->imm < lo || addiw->imm > hi) {
          auto immr = new MIli(addiw->imm, Register::reg_t2);
          auto addw = new MIaddw(Register::reg_t2, addiw->getReg(0),
                                 addiw->getTarget());
          mfunc->reg_pool->push_back(addiw->replaceWith({immr, addw}));
        }
      } else if (ins->getInsTag() == MInstruction::ANDI) {
        auto andi = static_cast<MIandi *>(ins);
        if (andi->imm < lo || andi->imm > hi) {
          auto immr = new MIli(andi->imm, Register::reg_t2);
          auto and_ =
              new MIand(Register::reg_t2, andi->getReg(0), andi->getTarget());
          mfunc->reg_pool->push_back(andi->replaceWith({immr, and_}));
        }
      } else if (ins->getInsTag() == MInstruction::XORI) {
        auto xori = static_cast<MIxori *>(ins);
        if (xori->imm < lo || xori->imm > hi) {
          auto immr = new MIli(xori->imm, Register::reg_t2);
          auto xor_ =
              new MIxor(Register::reg_t2, xori->getReg(0), xori->getTarget());
          mfunc->reg_pool->push_back(xori->replaceWith({immr, xor_}));
        }
      } else if (ins->getInsTag() == MInstruction::SLTI) {
        auto slti = static_cast<MIslti *>(ins);
        if (slti->imm < lo || slti->imm > hi) {
          auto immr = new MIli(slti->imm, Register::reg_t2);
          auto slt =
              new MIslt(Register::reg_t2, slti->getReg(0), slti->getTarget());
          mfunc->reg_pool->push_back(slti->replaceWith({immr, slt}));
        }
      } else if (ins->getInsTag() == MInstruction::LW) { // todo: deduplicate
        auto memop = static_cast<MIlw *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t2);
          auto addr =
              new MIadd(Register::reg_t2, memop->getReg(0), Register::reg_t2);
          memop->imm = 0;
          memop->setReg(0, Register::reg_t2);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::LD) {
        auto memop = static_cast<MIld *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t2);
          auto addr =
              new MIadd(Register::reg_t2, memop->getReg(0), Register::reg_t2);
          memop->imm = 0;
          memop->setReg(0, Register::reg_t2);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::FLD) {
        auto memop = static_cast<MIfld *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t2);
          auto addr =
              new MIadd(Register::reg_t2, memop->getReg(0), Register::reg_t2);
          memop->imm = 0;
          memop->setReg(0, Register::reg_t2);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::FLW) {
        auto memop = static_cast<MIflw *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t2);
          auto addr =
              new MIadd(Register::reg_t2, memop->getReg(0), Register::reg_t2);
          memop->imm = 0;
          memop->setReg(0, Register::reg_t2);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::SW) {
        auto memop = static_cast<MIsw *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t0);
          auto addr =
              new MIadd(Register::reg_t0, memop->getReg(1), Register::reg_t0);
          memop->imm = 0;
          memop->setReg(1, Register::reg_t0);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::SD) {
        auto memop = static_cast<MIsd *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t0);
          auto addr =
              new MIadd(Register::reg_t0, memop->getReg(1), Register::reg_t0);
          memop->imm = 0;
          memop->setReg(1, Register::reg_t0);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::FSD) {
        auto memop = static_cast<MIsd *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t0);
          auto addr =
              new MIadd(Register::reg_t0, memop->getReg(1), Register::reg_t0);
          memop->imm = 0;
          memop->setReg(1, Register::reg_t0);
          memop->insertBefore({immr, addr});
        }
      } else if (ins->getInsTag() == MInstruction::FSW) {
        auto memop = static_cast<MIfsw *>(ins);
        if (memop->imm < lo || memop->imm > hi) {
          auto immr = new MIli(memop->imm, Register::reg_t0);
          auto addr =
              new MIadd(Register::reg_t0, memop->getReg(1), Register::reg_t0);
          memop->imm = 0;
          memop->setReg(1, Register::reg_t0);
          memop->insertBefore({immr, addr});
        }
      }
    }
  }
}

void out_of_ssa(MFunction *func, LivenessInfo *liveness_ireg,
                LivenessInfo *liveness_freg,
                map<Register *, Register *> *allocation) {
  // std::cout << "out of ssa" << std::endl;
  map<MBasicBlock *, vector<MInstruction *>> moves;

  for (auto bb : func->getBasicBlocks()) {
    for (auto phi : bb->getPhis()) {

      if (liveness_freg != nullptr && liveness_ireg != nullptr) {
        auto liveIni = &liveness_ireg->at(bb->getAllInstructions().at(0));
        auto liveInf = &liveness_freg->at(bb->getAllInstructions().at(0));
        if (liveIni->find(phi) == liveIni->end() &&
            liveInf->find(phi) == liveInf->end()) {
          continue;
        }
      }

      // std::cout << "check phi " << *phi << endl;
      assert(phi->getTarget()->getTag() == Register::V_FREGISTER ||
             phi->getTarget()->getTag() == Register::V_IREGISTER);
      for (int i = 0; i < phi->getOprandNum(); i++) {
        auto opd = phi->getOprand(i);
        auto pre = phi->getIncomingBlock(i);
        // std::cout << "  check " << pre->getName() << endl;
        auto phir = phi->getTarget();
        if (allocation != nullptr &&
            allocation->find(phir) != allocation->end()) {
          phir = allocation->at(phir);
        }
        vector<MInstruction *> inss;
        if (opd.tp == MIOprandTp::Reg) {
          auto reg = opd.arg.reg;
          auto reg_ = opd.arg.reg;
          assert(reg->getTag() == Register::V_FREGISTER ||
                 reg->getTag() == Register::V_IREGISTER);
          if (allocation != nullptr &&
              allocation->find(reg) != allocation->end()) {
            reg_ = allocation->at(reg);
          }

          if (reg->getTag() == Register::V_FREGISTER) {
            inss.push_back(new MIfmv_s(reg_, phir));
          } else {
            inss.push_back(new MImv(reg_, phir));
          }
        } else if (opd.tp == MIOprandTp::Float) {
          auto g = func->getMod()->addGlobalFloat(new FloatConstant(opd.arg.f));
          inss.push_back(new MIla(g, Register::reg_t0));
          inss.push_back(new MIflw(Register::reg_t0, 0, phir));
        } else {
          inss.push_back(new MIli(opd.arg.i, phir));
        }
        // If critical path
        if (pre->getOutgoings().size() > 1 && bb->getIncomings().size() > 1) {
          auto newbb =
              func->addBasicBlock("cirtical" + pre->getName() + bb->getName());
          newbb->pushJmp(new MIj(bb));
          int new_dp = func->bbDepth->at(pre);
          if (func->bbDepth->at(bb) < new_dp) {
            new_dp = func->bbDepth->at(bb);
          }
          func->bbDepth->insert({newbb, new_dp});

          pre->replaceOutgoing(bb, newbb);
          bb->replacePhiIncoming(pre, newbb);

          moves.insert({newbb, {}});
          for (auto ins : inss) {
            moves[newbb].push_back(ins);
          }
        } else if (bb->getIncomings().size() > 1) {
          assert(pre->getOutgoings().size() == 1);
          if (moves.find(pre) == moves.end()) {
            moves.insert({pre, {}});
          }
          for (auto ins : inss) {
            moves[pre].push_back(ins);
          }
        } else {
          assert(bb->getIncomings().size() == 1);
          if (moves.find(bb) == moves.end()) {
            moves.insert({bb, {}});
          }
          for (auto ins : inss) {
            moves[bb].push_back(ins);
          }
        }
      }
    }
  }

  for (auto phimove : moves) {
    phimove.first->pushInstrs(solveParallelAssignment(phimove.second));
  }
}

void rewrite_program_spill(MFunction *func, map<Register *, int> *spill) {
  for (auto bb : func->getBasicBlocks()) {
    auto instrs = vector<MInstruction *>() = bb->getAllInstructions();
    for (auto ins : instrs) {
      // std::ostringstream oss;
      // oss << " " << *ins;
      // ins->setComment(oss.str());

      // std::cout << "\nrewrite " << *ins << endl;

      auto loads = vector<MInstruction *>();
      auto regs = std::vector<Register *>();
      for (int i = 0; i < ins->getRegNum(); i++) {
        auto reg = ins->getReg(i);
        // std::cout << "  check reg " << reg->getName() << IS_VIRTUAL_REG(reg)
        //           << reg->getTag() << endl;
        if (IS_VIRTUAL_REG(reg) && spill->find(reg) != spill->end()) {
          // std::cout << "    handle!" << endl;
          regs.push_back(reg);
        }
      }
      int float_cnt = 0;
      int int_cnt = 5; // start from t0
      assert(regs.size() <= 2);
      std::set<Register *> unique_regs(regs.begin(), regs.end());
      regs.assign(unique_regs.begin(), unique_regs.end());
      MInstruction *load;
      for (auto reg : regs) {
        auto vreg = static_cast<VRegister *>(reg);
        if (vreg->getTag() == Register::V_FREGISTER) {
          auto target = Register::getFRegister(float_cnt++);
          int offset = spill->at(reg);
          ins->replaceRegister(reg, target);
          load = new MIflw(Register::reg_s0, -offset, target);
          loads.push_back(load);
        } else if (vreg->isPointer()) {
          auto target = Register::getIRegister(int_cnt++);
          int offset = spill->at(reg);
          ins->replaceRegister(reg, target);
          load = new MIld(Register::reg_s0, -offset, target);
          loads.push_back(load);
        } else {
          auto target = Register::getIRegister(int_cnt++);
          int offset = spill->at(reg);
          // std::cout << "  remove " << reg->getName() << " of " << *ins <<
          // endl;
          ins->replaceRegister(reg, target);
          load = new MIlw(Register::reg_s0, -offset, target);
          loads.push_back(load);
        }
        load->setComment(" Load " + reg->getName());
      }

      ins->insertBefore(loads);
      if (ins->getTarget() != nullptr && IS_VIRTUAL_REG(ins->getTarget()) &&
          (spill->find(ins->getTarget()) != spill->end())) {
        auto stores = vector<MInstruction *>();
        auto reg = ins->getTarget();
        auto vreg = static_cast<VRegister *>(reg);
        MInstruction *store;
        if (vreg->getTag() == Register::V_FREGISTER) {
          auto target = Register::getFRegister(float_cnt++);
          int offset = spill->at(reg);
          ins->replaceRegister(reg, target);
          store = new MIfsw(target, -offset, Register::reg_s0);
          stores.push_back(store);
        } else if (vreg->isPointer()) {
          auto target = Register::reg_t2; // always t2
          int offset = spill->at(reg);
          ins->replaceRegister(reg, target);
          store = new MIsd(target, -offset, Register::reg_s0);
          stores.push_back(store);
        } else {
          auto target = Register::reg_t2;
          int offset = spill->at(reg);
          ins->replaceRegister(reg, target);
          store = new MIsw(target, -offset, Register::reg_s0);
          stores.push_back(store);
        }
        store->setComment(" Store " + reg->getName());
        ins->insertAfter(stores);
      }
    }
  }
}

void rewrite_program_allocate(MFunction *func,
                              map<Register *, Register *> *allocation) {
  // TODO: DEF-USE CHAIN IS BROKEN!!!
  // for (auto &[logical_reg, physical_reg] : *allocation) {
  //   logical_reg->replaceRegisterWith(physical_reg);
  // }
  for (auto bb : func->getBasicBlocks()) {
    auto instrs = vector<MInstruction *>() = bb->getAllInstructions();
    for (auto ins : instrs) {
      std::ostringstream oss;
      oss << " " << *ins;
      if (ins->getComment() != "") {
        ins->setComment(ins->getComment() + oss.str());
      } else {
        ins->setComment(oss.str());
      }
      auto loads = vector<MInstruction *>();
      auto regs = std::vector<Register *>();
      for (int i = 0; i < ins->getRegNum(); i++) {
        auto reg = ins->getReg(i);

        if (allocation->find(reg) != allocation->end()) {
          regs.push_back(reg);
        }
      }
      std::set<Register *> s(regs.begin(), regs.end());
      regs.assign(s.begin(), s.end());
      for (auto reg : regs) {
        ins->replaceRegister(reg, (*allocation)[reg]);
      }

      if (ins->getTarget() != nullptr &&
          (allocation->find(ins->getTarget()) != allocation->end())) {
        auto reg = ins->getTarget();
        ins->replaceRegister(reg, (*allocation)[reg]);
        // ins->setComment(" ASSIGN TO " + reg->getName());
      }
    }
  }
}

void lower_alloca(MFunction *func, int &stack_offset) {
  for (auto bb : func->getBasicBlocks()) {
    for (auto ins : bb->getInstructions()) {
      if (ins->getInsTag() == MInstruction::H_ALLOCA) {
        auto alloca = static_cast<MHIalloca *>(ins);
        auto sz = alloca->getSize();
        stack_offset += sz;
        auto addr =
            new MIaddi(Register::reg_s0, -stack_offset, alloca->getTarget());
        func->reg_pool->push_back(ins->replaceWith({addr}));
      }
    }
  }
}

void lower_call(MFunction *func, int &stack_offset,
                map<Register *, Register *> *allocation,
                map<Register *, int> *spill, LivenessInfo *liveness_ireg,
                LivenessInfo *liveness_freg) {
  for (auto bb : func->getBasicBlocks()) {
    for (auto ins : bb->getInstructions()) {
      if (ins->getInsTag() == MInstruction::H_CALL) {
        auto call = static_cast<MHIcall *>(ins);
        auto live_iregs = liveness_ireg->at(call);
        auto live_fregs = liveness_freg->at(call);
        std::set<Register *> all_live;
        all_live.insert(live_iregs.begin(), live_iregs.end());
        all_live.insert(live_fregs.begin(), live_fregs.end());
        if (all_live.count(call->getTarget()) != 0) {
          all_live.erase(call->getTarget());
        }
        auto caller_saved =
            getActuallCallerSavedRegisters(allocation, all_live);
        // todo: delete?
        // if (ins->getTag() == Register::V_FREGISTER) {
        //   ins->replaceRegisterWith(Register::reg_fa0);
        // } else {
        //   ins->replaceRegisterWith(Register::reg_a0);
        // }
        // std::cout << "lower " << *ins << endl;
        auto rmd = ins->replaceWith(call->generateCallSequence(
            func, stack_offset, spill, allocation, &caller_saved));
        // std::cout << "lower over " << *ins << endl;
        func->reg_pool->push_back(std::move(rmd));
        // std::cout << "lower over 2" << *ins << endl;
      }
    }
  }
}

vector<MInstruction *> MHIcall::generateCallSequence(
    MFunction *func, int stack_offset, map<Register *, int> *spill,
    map<Register *, Register *> *allocation, set<Register *> *caller_saved) {
  // std::cout << "gen call seq save  " << *this << endl;
  vector<MInstruction *> push_callee_saved;
  vector<MInstruction *> pop_callee_saved;
  int stack_inc_sz = 0;
  for (auto reg : *caller_saved) {
    auto vreg = static_cast<VRegister *>(reg);
    if (vreg->isPointer()) {
      stack_inc_sz += 8;
    } else {
      stack_inc_sz += 4;
    }
    assert(allocation->find(vreg) != allocation->end());
    // maybe this piece of logic could be extrat away
    if (vreg->getTag() == V_FREGISTER) {
      push_callee_saved.push_back(
          new MIfsw(reg, -stack_inc_sz, Register::reg_sp));
      pop_callee_saved.push_back(
          new MIflw(Register::reg_sp, -stack_inc_sz, reg));
    } else if (vreg->isPointer()) {
      push_callee_saved.push_back(
          new MIsd(reg, -stack_inc_sz, Register::reg_sp));
      pop_callee_saved.push_back(
          new MIld(Register::reg_sp, -stack_inc_sz, reg));
    } else {
      push_callee_saved.push_back(
          new MIsw(reg, -stack_inc_sz, Register::reg_sp));
      pop_callee_saved.push_back(
          new MIlw(Register::reg_sp, -stack_inc_sz, reg));
    }
  }
  // std::cout << "gen call seq para" << endl;
  vector<MInstruction *> res;
  for (int i = 0; i < function->getParaSize(); i++) {
    auto funp = function->getPara(i);
    if (funp->getRegister() == nullptr) {
      stack_inc_sz += funp->getSize();
    }
  }
  int new_stack_pos = roundUp(stack_offset + stack_inc_sz, 16);

  int shift = new_stack_pos - stack_offset;
  res.push_back(new MIaddi(Register::reg_sp, -shift, Register::reg_sp));

  // std::cout << "gen call assignments" << endl;
  vector<MInstruction *> assignments;
  for (int i = 0; i < function->getParaSize(); i++) {
    auto para = function->getPara(i);
    auto arg = args->at(i);
    // std::cout << "  para " << para->getName() << endl;
    if (para->getRegister() == nullptr) {
      switch (arg.tp) {
      case MIOprandTp::Float: {
        auto g = func->getMod()->addGlobalFloat(new FloatConstant(arg.arg.f));
        res.push_back(new MIla(g, Register::reg_t2));
        res.push_back(new MIflw(Register::reg_t2, 0, Register::reg_ft0));
        res.push_back(
            new MIfsw(Register::reg_ft0, para->getOffset(), Register::reg_sp));
        break;
      }
      case MIOprandTp::Int: {
        res.push_back(new MIli(arg.arg.i, Register::reg_t2));
        res.push_back(
            new MIsw(Register::reg_t2, para->getOffset(), Register::reg_sp));
        break;
      }
      case MIOprandTp::Reg: {
        auto reg = static_cast<VRegister *>(arg.arg.reg);
        if (spill->find(reg) != spill->end()) {
          auto offset = spill->at(reg);
          if (reg->getTag() == Register::V_FREGISTER) {
            res.push_back(
                new MIflw(Register::reg_s0, -offset, Register::reg_ft0));
            res.push_back(new MIfsw(Register::reg_ft0, para->getOffset(),
                                    Register::reg_sp));
          } else if (reg->isPointer()) {
            assert(reg->getTag() == Register::V_IREGISTER);
            res.push_back(
                new MIld(Register::reg_s0, -offset, Register::reg_t2));
            res.push_back(new MIsd(Register::reg_t2, para->getOffset(),
                                   Register::reg_sp));
          } else {
            assert(reg->getTag() == Register::V_IREGISTER);
            res.push_back(
                new MIlw(Register::reg_s0, -offset, Register::reg_t2));
            auto sw =
                new MIsw(Register::reg_t2, para->getOffset(), Register::reg_sp);
            std::ostringstream oss;
            oss << " STORE" << reg->getName() << " ";
            sw->setComment(oss.str());
            res.push_back(sw);
          }
        } else {
          auto phyreg = allocation->at(reg);
          if (reg->getTag() == Register::V_FREGISTER) {
            assignments.push_back(
                new MIfsw(phyreg, para->getOffset(), Register::reg_sp));
          } else if (reg->isPointer()) {
            assert(reg->getTag() == Register::V_IREGISTER);
            assignments.push_back(
                new MIsd(phyreg, para->getOffset(), Register::reg_sp));
          } else {
            assert(reg->getTag() == Register::V_IREGISTER);
            assignments.push_back(
                new MIsw(phyreg, para->getOffset(), Register::reg_sp));
          }
        }
        break;
      }
      }
    } else {
      switch (arg.tp) {
      case Float: {
        auto g = func->getMod()->addGlobalFloat(new FloatConstant(arg.arg.f));
        assignments.push_back(new MIla(g, Register::reg_t2));
        assignments.push_back(
            new MIflw(Register::reg_t2, 0, para->getRegister()));
        break;
      }
      case Int: {
        assignments.push_back(new MIli(arg.arg.i, para->getRegister()));
        break;
      }
      case Reg: {
        // todo: maybe we can extract this bunch of things away...
        auto phyreg = para->getRegister();
        auto argr = static_cast<VRegister *>(arg.arg.reg);
        // std::cout << " HANDLE " <<  argr->getName() << endl;
        if (spill->find(argr) != spill->end()) {
          auto offset = spill->at(argr);
          if (phyreg->getTag() == Register::F_REGISTER) {
            assignments.push_back(new MIflw(Register::reg_s0, -offset, phyreg));
          } else if (argr->isPointer()) {
            assert(argr->getTag() == Register::V_IREGISTER);
            assignments.push_back(new MIld(Register::reg_s0, -offset, phyreg));
          } else {
            assert(argr->getTag() == Register::V_IREGISTER);
            assignments.push_back(new MIlw(Register::reg_s0, -offset, phyreg));
          }
        } else {
          auto phyargr = allocation->at(argr);
          if (phyreg->getTag() == Register::F_REGISTER) {
            assignments.push_back(new MIfmv_s(phyargr, phyreg));
          } else {
            assert(phyreg->getTag() == Register::I_REGISTER);
            assignments.push_back(new MImv(phyargr, phyreg));
          }
        }
        break;
      }
      }
    }
  }
  // std::cout << "rearranged_assignments" << endl;
  auto rearranged_assignments = solveParallelAssignment(assignments);
  for (auto assign : rearranged_assignments) {
    res.push_back(assign);
  }
  // std::cout << "gen call call" << endl;

  res.push_back(new MIcall(function));

  // std::cout << "gen call ret val" << endl;
  // handle returen value
  auto ret = this->getTarget();
  if (ret != nullptr) {
    if (spill->find(ret) != spill->end()) {
      auto addr = spill->at(ret);
      auto vret = static_cast<VRegister *>(ret);
      if (ret->getTag() == Register::V_FREGISTER) {
        res.push_back(new MIfsw(Register::reg_fa0, -addr, Register::reg_s0));
      } else if (vret->isPointer()) {
        res.push_back(new MIsd(Register::reg_a0, -addr, Register::reg_s0));
      } else {
        res.push_back(new MIsw(Register::reg_a0, -addr, Register::reg_s0));
      }
    } else {
      auto phyreg = allocation->at(this);
      if (ret->getTag() == Register::V_FREGISTER) {
        if (phyreg != Register::reg_fa0) {
          res.push_back(new MIfmv_s(Register::reg_fa0, phyreg));
        }
      } else {
        if (phyreg != Register::reg_a0) {
          res.push_back(new MImv(Register::reg_a0, phyreg));
        }
      }
    }
  }
  res.push_back(new MIaddi(Register::reg_sp, shift, Register::reg_sp));

  std::vector<MInstruction *> merged;
  merged.insert(merged.end(), push_callee_saved.begin(),
                push_callee_saved.end());
  merged.insert(merged.end(), res.begin(), res.end());
  merged.insert(merged.end(), pop_callee_saved.begin(), pop_callee_saved.end());
  // std::cout << "gen call over" << endl;

  return merged;
}

void add_prelude(MFunction *func, map<Register *, Register *> *allocation,
                 map<Register *, int> *spill, int stack_offset,
                 set<Register *> *callee_saved) {
  int stack_zs = stack_offset;
  auto entry = func->getEntry();
  auto sub_sp = new MIaddi(Register::reg_sp, -stack_zs, Register::reg_sp);
  auto store_ra = new MIsd(Register::reg_ra, stack_zs - 8, Register::reg_sp);
  auto store_fp = new MIsd(Register::reg_s0, stack_zs - 16, Register::reg_sp);
  auto get_fp = new MIaddi(Register::reg_sp, stack_zs, Register::reg_s0);
  auto prelude = vector<MInstruction *>({sub_sp, store_ra, store_fp, get_fp});

  int offset = 16;
  for (auto reg : *callee_saved) {
    offset += 8;
    if (reg->getTag() == Register::I_REGISTER) {
      prelude.push_back(new MIsd(reg, -offset, Register::reg_s0));
    } else {
      prelude.push_back(new MIfsd(reg, -offset, Register::reg_s0));
    }
  }

  vector<MInstruction *> assignments;
  for (int i = 0; i < func->getParaSize(); i++) {
    auto para = func->getPara(i);
    if (spill->find(para) != spill->end()) {
      auto addr = spill->at(para);
      if (para->getRegister() != nullptr) {
        if (para->getTag() == Register::V_FREGISTER) {
          assignments.push_back(
              new MIfsw(para->getRegister(), -addr, Register::reg_s0));
        } else if (para->isPointer()) {
          assignments.push_back(
              new MIsd(para->getRegister(), -addr, Register::reg_s0));
        } else {
          assignments.push_back(
              new MIsw(para->getRegister(), -addr, Register::reg_s0));
        }
      } else {
        // do nothing
      }
    } else if (allocation->find(para) != allocation->end()) {
      auto phyreg = allocation->at(para);
      if (phyreg->getTag() == Register::F_REGISTER) {
        if (para->getRegister() != nullptr) {
          assignments.push_back(new MIfmv_s(para->getRegister(), phyreg));
        } else {
          auto addr = para->getOffset();
          assignments.push_back(new MIflw(Register::reg_s0, addr, phyreg));
        }
      } else {
        assert(phyreg->getTag() == Register::I_REGISTER);
        if (para->getRegister() != nullptr) {
          assignments.push_back(new MImv(para->getRegister(), phyreg));
        } else {
          auto addr = para->getOffset();
          if (para->isPointer()) {
            assignments.push_back(new MIld(Register::reg_s0, addr, phyreg));
          } else {
            assignments.push_back(new MIlw(Register::reg_s0, addr, phyreg));
          }
        }
      }
    } else { // unused argument
    }
  }
  auto rearranged_assignments = solveParallelAssignment(assignments);
  for (auto assign : rearranged_assignments) {
    prelude.push_back(assign);
  }

  entry->pushInstrsAtHead(prelude);
}

void add_conclude(MFunction *func, map<Register *, Register *> *allocation,
                  map<Register *, int> *spill, int stack_offset,
                  set<Register *> *callee_saved) {

  for (auto bb : func->getBasicBlocks()) {
    if (bb->getJmpNum() != 0) {
      auto jmp = bb->getJmp(bb->getJmpNum() - 1);
      if (jmp->getInsTag() == MInstruction::H_RET) {
        auto hret = static_cast<MHIret *>(jmp);
        switch (hret->r.tp) {
        case MIOprandTp::Float: {
          auto g =
              func->getMod()->addGlobalFloat(new FloatConstant(hret->r.arg.f));
          bb->pushInstr(new MIla(g, Register::reg_t0));
          bb->pushInstr(new MIflw(Register::reg_t0, 0, Register::reg_fa0));
          break;
        }
        case MIOprandTp::Int: {
          bb->pushInstr(new MIli(hret->r.arg.i, Register::reg_a0));
          break;
        }
        case MIOprandTp::Reg: {
          auto reg = hret->r.arg.reg;
          if (allocation->find(reg) != allocation->end()) {
            auto phyreg = allocation->at(reg);
            if (reg->getTag() == Register::V_FREGISTER) {
              bb->pushInstr(new MIfmv_s(phyreg, Register::reg_fa0));
            } else {
              bb->pushInstr(new MImv(phyreg, Register::reg_a0));
            }
          } else {
            auto addr = spill->at(reg);
            if (reg->getTag() == Register::V_FREGISTER) {
              bb->pushInstr(
                  new MIflw(Register::reg_s0, -addr, Register::reg_fa0));
            } else {
              bb->pushInstr(
                  new MIlw(Register::reg_s0, -addr, Register::reg_a0));
            }
          }
          break;
        }
        }
        hret->replaceWith({});
        bb->pushJmp(new MIret());
      }
    }
  }

  auto exit = func->addBasicBlock(func->getName() + "_exit");
  func->bbDepth->insert({exit, 0});
  int offset = 16;
  for (auto reg : *callee_saved) {
    offset += 8;
    if (reg->getTag() == Register::I_REGISTER) {
      exit->pushInstr(new MIld(Register::reg_s0, -offset, reg));
    } else {
      exit->pushInstr(new MIfld(Register::reg_s0, -offset, reg));
    }
  }

  int stack_sz = stack_offset;
  auto load_ra = new MIld(Register::reg_sp, stack_sz - 8, Register::reg_ra);
  auto load_fp = new MIld(Register::reg_sp, stack_sz - 16, Register::reg_s0);
  auto add_sp = new MIaddi(Register::reg_sp, stack_sz, Register::reg_sp);
  auto ret = new MIret();
  exit->pushInstrs({load_ra, load_fp, add_sp});
  exit->pushJmp(ret);

  for (auto bb : func->getBasicBlocks()) {
    if (bb == exit)
      continue;
    if (bb->getJmpNum() != 0 &&
        bb->getJmp(bb->getJmpNum() - 1)->getInsTag() == MInstruction::RET) {
      bb->getJmp(bb->getJmpNum() - 1)->replaceWith({});
      bb->pushJmp(new MIj(exit));
    }
  }
}

///////////////////////////////////////
///////////////////////////////////////
/////////  Register Op
///////////////////////////////////////
///////////////////////////////////////

Register *getOneIRegiter(set<Register *> *used) {
  static set<Register *> all_registers = {
      Register::reg_s1, Register::reg_s2,  Register::reg_s3,  Register::reg_s4,
      Register::reg_s5, Register::reg_s6,  Register::reg_s7,  Register::reg_s8,
      Register::reg_s9, Register::reg_s10, Register::reg_s11, Register::reg_t3,
      Register::reg_t4, Register::reg_t5,  Register::reg_t6,  Register::reg_a0,
      Register::reg_a1, Register::reg_a2,  Register::reg_a3,  Register::reg_a4,
      Register::reg_a5, Register::reg_a6,  Register::reg_a7,
  };
  for (Register *reg : all_registers) {
    if (used->find(reg) == used->end()) {
      return reg;
    }
  }
  assert(0);
}

Register *getOneFRegiter(set<Register *> *used) {
  static set<Register *> all_registers = {
      Register::reg_ft3,  Register::reg_ft4,  Register::reg_ft5,
      Register::reg_ft6,  Register::reg_ft7,  Register::reg_fs0,
      Register::reg_fs1,  Register::reg_fs2,  Register::reg_fs3,
      Register::reg_fs4,  Register::reg_fs5,  Register::reg_fs6,
      Register::reg_fs7,  Register::reg_fs8,  Register::reg_fs9,
      Register::reg_fs10, Register::reg_fs11, Register::reg_ft8,
      Register::reg_ft9,  Register::reg_ft10, Register::reg_ft11,
      Register::reg_fa0,  Register::reg_fa1,  Register::reg_fa2,
      Register::reg_fa3,  Register::reg_fa4,  Register::reg_fa5,
      Register::reg_fa6,  Register::reg_fa7,
  };
  for (Register *reg : all_registers) {
    if (used->find(reg) == used->end()) {
      return reg;
    }
  }
  assert(0);
}

// return physical registers
set<Register *>
getActuallCalleeSavedRegisters(map<Register *, Register *> *allocation) {
  static set<Register *> saved_registers = {
      Register::reg_gp,   Register::reg_tp,  Register::reg_s1,
      Register::reg_s2,   Register::reg_s3,  Register::reg_s4,
      Register::reg_s5,   Register::reg_s6,  Register::reg_s7,
      Register::reg_s8,   Register::reg_s9,  Register::reg_s10,
      Register::reg_s11,  Register::reg_fs0, Register::reg_fs1,
      Register::reg_fs2,  Register::reg_fs3, Register::reg_fs4,
      Register::reg_fs5,  Register::reg_fs6, Register::reg_fs7,
      Register::reg_fs8,  Register::reg_fs9, Register::reg_fs10,
      Register::reg_fs11,
  };

  set<Register *> actual_saved;
  for (auto pair : *allocation) {
    auto reg = pair.first;
    auto phyreg = allocation->at(reg);
    if (saved_registers.find(phyreg) != saved_registers.end()) {
      actual_saved.insert(phyreg);
    }
  }
  return actual_saved;
}

// return vitual registers
set<Register *>
getActuallCallerSavedRegisters(map<Register *, Register *> *allocation,
                               set<Register *> liveInOfcallsite) {
  static set<Register *> saved_registers = {
      Register::reg_t0,   Register::reg_t1,   Register::reg_t2,
      Register::reg_a0,   Register::reg_a1,   Register::reg_a2,
      Register::reg_a3,   Register::reg_a4,   Register::reg_a5,
      Register::reg_a6,   Register::reg_a7,   Register::reg_t3,
      Register::reg_t4,   Register::reg_t5,   Register::reg_t6,
      Register::reg_ft0,  Register::reg_ft1,  Register::reg_ft2,
      Register::reg_ft3,  Register::reg_ft4,  Register::reg_ft5,
      Register::reg_ft6,  Register::reg_ft7,  Register::reg_fa0,
      Register::reg_fa1,  Register::reg_fa2,  Register::reg_fa3,
      Register::reg_fa4,  Register::reg_fa5,  Register::reg_fa6,
      Register::reg_fa7,  Register::reg_ft8,  Register::reg_ft9,
      Register::reg_ft10, Register::reg_ft11,
  };
  set<Register *> intersection;
  for (auto reg : liveInOfcallsite) {
    if (allocation->find(reg) != allocation->end()) {
      auto phyreg = allocation->at(reg);
      if (saved_registers.find(phyreg) != saved_registers.end()) {
        intersection.insert(reg);
      }
    }
  }
  return intersection;
}
