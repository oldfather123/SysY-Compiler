#include <unordered_set>

#include "allocate_register.hh"
#include "cgen.hh"
using std::unordered_set;


int get_store_imm(MInstruction *store) {
  switch (store->getInsTag()) {
  case MInstruction::SW: {
    auto sw = static_cast<MIsw *>(store);
    return sw->imm;
  }
  case MInstruction::SD: {
    auto sd = static_cast<MIsd *>(store);
    return sd->imm;
  }
  case MInstruction::FSW: {
    auto fsw = static_cast<MIfsw *>(store);
    return fsw->imm;
  }
  case MInstruction::FSD: {
    auto fsd = static_cast<MIfsd *>(store);
    return fsd->imm;
  }
  default:
    assert(0);
  }
}

int get_load_imm(MInstruction *load) {
  switch (load->getInsTag()) {
  case MInstruction::LW: {
    auto lw = static_cast<MIlw *>(load);
    return lw->imm;
  }
  case MInstruction::LD: {
    auto ld = static_cast<MIld *>(load);
    return ld->imm;
  }
  case MInstruction::FLW: {
    auto lfw = static_cast<MIflw *>(load);
    return lfw->imm;
  }
  case MInstruction::FLD: {
    auto lfd = static_cast<MIfld *>(load);
    return lfd->imm;
  }
  default:
    assert(0);
  }
}

void scan_store_loads(MBasicBlock *bb, vector<vector<MInstruction *>> &ldsts) {
  int stk_off = 0;

  unordered_map<Register *, bool> hold_val; // if reg also hold the value?
  unordered_map<Register *, int> reg_addr;
  unordered_map<int, Register *> addr_reg;
  // invarient: addr always holds the value
  map<Register *, vector<MInstruction *>>
      tldsts; // a list of load/stores which its value never changes

  for (auto ins : bb->getInstructions()) {
    if (ins->getInsTag() == MInstruction::ADDI &&
        ins->getTarget() == Register::reg_sp) {
      int imm = static_cast<MIaddi *>(ins)->imm;
      stk_off += imm;
      continue;
    }
    if (stk_off == 0) { // not in call sequence

      if (is_mem_write(ins) && ins->getReg(1) == Register::reg_sp) {
        int off = get_store_imm(ins);
        auto reg = ins->getReg(0);
        if (reg == Register::reg_ra || reg == Register::reg_s0) {
          continue;
        }
        int addr = stk_off + off;

        if (reg_addr.count(reg) > 0 && reg_addr[reg] != addr) {
          ldsts.push_back(tldsts[reg]);
          tldsts[reg].clear();
          addr_reg.erase(reg_addr[reg]);
          reg_addr.erase(reg);
        }
        if (addr_reg.count(addr) > 0) {
          if (addr_reg[addr] == reg) {
            if (hold_val[reg]) { // find a redudent store
              // std::cout << endl << " Redudent store: " << *ins << endl;
              tldsts[reg].push_back(ins);
              // std::cout <<reg->getName() << " :"  << tldsts[reg].size() <<
              // endl;
              continue;
            }
          }

          // std::cout << endl << " Overwrite store: " << *ins << endl;
          // value was overwrite
          ldsts.push_back(tldsts[addr_reg[addr]]);
          tldsts[addr_reg[addr]].clear();
          auto erase = addr_reg[addr];
          addr_reg.erase(addr);
          reg_addr.erase(erase);
          // std::cout << reg_addr.count(reg) << endl;

          tldsts[reg].push_back(ins);
          hold_val[reg] = false; // only load will be condsidered hold value
          reg_addr[reg] = addr;
          addr_reg[addr] = reg;
          // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
        } else if (reg_addr.count(reg) == 0) {
          // std::cout << endl << " New store: " << *ins << endl;
          // no store here  add new
          if (tldsts.find(reg) == tldsts.end()) {
            tldsts.insert({reg, {}});
          }
          assert(tldsts[reg].size() == 0);
          hold_val[reg] = false;
          tldsts[reg].push_back(ins);
          reg_addr[reg] = addr;
          addr_reg[addr] = reg;
          // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
        }
        continue;
      } else if (is_mem_read(ins) && ins->getReg(0) == Register::reg_sp) {
        auto reg = ins->getTarget();
        auto off = get_load_imm(ins);
        if (reg == Register::reg_ra || reg == Register::reg_s0) {
          continue;
        }
        auto addr = off + stk_off;
        if (reg_addr.count(reg) != 0 && reg_addr[reg] == addr) {
          // std::cout << endl << " Load what stored before: " << *ins << endl;
          // std::cout << endl <<  reg_addr.count(reg) << endl;
          // std::cout << off << endl;
          // std::cout << stk_off << endl;
          // std::cout << reg_addr[reg] << endl;
          // std::cout << reg->getName() << endl;
          assert(tldsts[reg].size() != 0);
          tldsts[reg].push_back(ins);
          hold_val[reg] = true;
          // std::cout <<reg->getName() <<" :"  << tldsts[reg].size() << endl;
        }
        continue;
      }
    }

    static set<Register *> caller_saved_registers = {
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
    if (ins->getInsTag() == MInstruction::CALL) {
      for (auto reg : caller_saved_registers) {
        if (hold_val.count(reg) > 0) {
          hold_val[reg] = false;
        }
      }
    }

    auto iuses = getUses<Register::I_REGISTER>(ins);
    for (auto reg : iuses) {
      if (reg_addr.count(reg) > 0) { // the reg is in stack
        if (hold_val[reg]) { // the value was used, then the last load can not
                             // be deleted
          // std::cout << endl << " Invalidate: " << *ins << endl;
          ldsts.push_back(tldsts[reg]);
          tldsts[reg].clear();
          addr_reg.erase(reg_addr[reg]);
          reg_addr.erase(reg);
          // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
        }
      }
    }
    auto fuses = getUses<Register::F_REGISTER>(ins);
    for (auto reg : fuses) {
      if (reg_addr.count(reg) > 0) {
        if (hold_val[reg]) {
          // std::cout << endl << " Invalidate: " << *ins << endl;
          ldsts.push_back(tldsts[reg]);
          tldsts[reg].clear();
          addr_reg.erase(reg_addr[reg]);
          reg_addr.erase(reg);
          // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
        }
      }
    }
    auto idefs = getDefs<Register::I_REGISTER>(ins);
    for (auto reg : idefs) {
      if (reg_addr.count(reg) > 0) {
        // std::cout << endl << " Kill: " << *ins << endl;
        hold_val[reg] = false;
        // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
      }
    }
    auto fdefs = getDefs<Register::F_REGISTER>(ins);
    for (auto reg : fdefs) {
      if (reg_addr.count(reg) > 0) {
        // std::cout << endl << " Kill: " << *ins << endl;
        hold_val[reg] = false;
        // std::cout <<reg->getName() << " :"  << tldsts[reg].size() << endl;
      }
    }
  }

  for (auto pair : tldsts) {
    if (pair.second.size() != 0) {
      ldsts.push_back(pair.second);
    }
  }
}

// Be careful!!! The used Register of call Instr in IMPLICIT!!!
void peephole_optimize(MModule *mod) {
  // remove reduant load/stores
  for (auto func : mod->getFunctions()) {
    for (auto bb : func->getBasicBlocks()) {
      vector<vector<MInstruction *>> ldstss;
      scan_store_loads(bb, ldstss);
      // std::cout << "Scan over" << endl;
      for (auto ldsts : ldstss) {
        // std::cout << "Sequence in " << bb->getName() << ": " << endl;
        // for (auto ins : ldsts) {
        //   std::cout << "  " << *ins << endl;
        // }
        int num = ldsts.size();
        int last_ld = -1;
        for (int i = num - 1; i >= 1; i--) {
          if (is_mem_read(ldsts[i])) {
            last_ld = i;
            break;
          }
        }
        if (last_ld != -1) {
          for (int i = 1; i < last_ld; i++) {
            ldsts[i]->replaceWith({});
          }
        }
      }
    }
  }

  for (auto func : mod->getFunctions()) {
    for (auto bb : func->getBasicBlocks()) {
      auto inss = bb->getAllInstructions();
      int i = 0;
      while (i < inss.size()) {
        auto ins = inss[i];
        //////////////////////////////////////
        // mv a0, a0
        // ---------------------------
        // ==> nop
        //////////////////////////////////////
        if (ins->getInsTag() == MInstruction::MV) {
          if (ins->getTarget() == ins->getReg(0)) {
            ins->replaceWith({});
            i++;
            // std::cout << "match " << " mv a0, a0" << endl;
            continue;
          }
        }

        // addi a0, a0, 0 # addi arr.addr.3, arr.addr.2, 0
        if (ins->getInsTag() == MInstruction::ADDI) {
          if (ins->getTarget() == ins->getReg(0) &&
              static_cast<MIaddi *>(ins)->imm == 0) {
            ins->replaceWith({});
            i++;
            // std::cout << "match " << " addi a0, a0, 0" << endl;
            continue;
          }
        }
        i += 1;
      }
    }
  }

  for (auto func : mod->getFunctions()) {
    for (auto bb : func->getBasicBlocks()) {
      auto inss = bb->getAllInstructions();
      int i = 0;
      while (i + 1 < inss.size()) {
        auto ins = inss[i];
        //////////////////////////////////////
        // li  a0, 4
        // mul a2, a1, a0
        // ---------------------------
        // ==> slliw a2, a1, 2
        //////////////////////////////////////
        if (ins->getInsTag() == MInstruction::LI) {
          MIli *liIns = (MIli *)ins;
          auto nIns = inss[i + 1];
          if (liIns->imm == 4 && nIns->getInsTag() == MInstruction::MUL &&
              nIns->getReg(1) == ins->getTarget()) {
            auto sllIns = new MIslli(nIns->getReg(0), 2, nIns->getTarget());
            ins->replaceWith({sllIns});
            nIns->replaceWith({});
            i++;
            continue;
          }
        }

        // addi a1, a0, 0 # addi arr.addr.3, arr.addr.2, 0
        // addi a0, a1, 0 # a
        i += 1;
      }
    }
  }
}