#include "allocate_register.hh"
#include <set>

void spill_registers(MFunction *func, map<Register *, int> *spill,
                     int &stack_offset) {
#ifdef DEBUG_MODE
  static set<Register *> temp_registers = {
      Register::reg_t0,  Register::reg_t1,  Register::reg_t2,
      Register::reg_ft0, Register::reg_ft1, Register::reg_ft2,
  };
#endif

  // allocate arguments
  auto ftp = func->getType();
  for (int i = 0; i < func->getType()->getArgSize(); i++) {
    auto arg = func->getPara(i);
    if (arg->getRegister() == nullptr) {
      spill->insert({arg, -arg->getOffset()});
    } else {
      if (arg->isPointer()) {
        stack_offset += 8;
      } else {
        stack_offset += 4;
      }
      spill->insert({arg, stack_offset});
    }
  }
  // allocate registers
  for (auto bb : func->getBasicBlocks()) {
    for (auto ins : bb->getPhis()) {
      auto reg = ins;
      if (reg->isPointer()) {
        stack_offset += 8;
      } else {
        stack_offset += 4;
      }
      spill->insert({reg, stack_offset});
    }
    for (auto ins : bb->getInstructions()) {
      if (ins->getTarget() == nullptr) { // not used as register
        continue;
      }
      auto reg = ins->getTarget();
      if (spill->find(reg) != spill->end()) { // already spilled
        continue;
      }

      if (!(reg->getTag() == Register::V_IREGISTER ||
            reg->getTag() == Register::V_FREGISTER)) {
#ifdef DEBUG_MODE
        if (temp_registers.find(reg) == temp_registers.end()) {
          assert(0);
        }
#endif
        continue;
      }
      if (static_cast<VRegister *>(reg)->isPointer()) {
        stack_offset += 8;
      } else {
        stack_offset += 4;
      }
      spill->insert({reg, stack_offset});
    }
  }
}

static void lower_call_spill_only(MFunction *func, map<Register *, int> *spill,
                                  int &stack_offset) {
  // std::cout << "lower_call_spill_only start" << endl;
  map<Register *, Register *> allocation;
  set<Register *> caller_saved;
  for (auto bb : func->getBasicBlocks()) {
    for (auto ins : bb->getInstructions()) {
      if (ins->getInsTag() == MInstruction::H_CALL) {
        // std::cout << "spill " << *ins << endl;
        auto call = static_cast<MHIcall *>(ins);
        func->reg_pool->push_back(ins->replaceWith(call->generateCallSequence(
            func, stack_offset, spill, &allocation, &caller_saved)));
      }
    }
  }
  // std::cout << "lower_call_spill_only over" << endl;
}

void spill_register_for_func(MFunction *func) {
  auto spill = make_unique<map<Register *, int>>();
  map<Register *, Register *> allocation;
  set<Register *> callee_saved;
  int stack_offset = 16; // with ra and sp

  spill_registers(func, spill.get(), stack_offset);

  // for (const auto &pair : *spill) {
  //   std::cout << "Register ID: " << pair.first->getName() << ", Offset: " <<
  //   pair.second
  //             << std::endl;
  // }

  lower_alloca(func, stack_offset);
  lower_call_spill_only(func, spill.get(), stack_offset);
  add_prelude(func, &allocation, spill.get(), stack_offset, &callee_saved);
  add_conclude(func, &allocation, spill.get(), stack_offset, &callee_saved);
  rewrite_program_spill(func, spill.get());
  fixRange(func);
}

void spill_all_register(MModule *mod) {
  for (auto func : mod->getFunctions()) {
    out_of_ssa(func, nullptr, nullptr, nullptr);
    spill_register_for_func(func);
  }
  mod->ssa_out();
}
