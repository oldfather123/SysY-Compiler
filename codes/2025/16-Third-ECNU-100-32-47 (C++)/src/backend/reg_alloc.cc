#include "../include/reg_alloc.h"
#include "../include/sym.h"
#include "AST/type.h"
#include "backend.h"
#include "frontend.h"
#include "riscv.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace aaac {
namespace backend {
using std::vector;

vector<LiveInterval> buildLiveIntervals(Function *func) {
  std::vector<LiveInterval> intervals;
  std::unordered_map<std::shared_ptr<sym::Var>, int> var_to_interval;

  using BBRange = std::pair<int, int>;
  std::map<std::shared_ptr<BasicBlock>, BBRange> bb_to_range;
  int last_index = 0;
  for (auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
    auto &bb = *bb_ptr;
    const BasicBlockInfo bb_info = bb.prop<BasicBlockInfo>();
    bb_to_range[bb_ptr] = std::make_pair(last_index, last_index + bb_info.insn_list->size());
    last_index += bb_info.insn_list->size();
  }

  // First pass: collect all variables and their first use
  // TODO: 直接将当前index作为起点可能会出问题
  int instruction_index = 0;
  for (auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
    auto &bb = *bb_ptr;
    const BasicBlockInfo bb_info = bb.prop<BasicBlockInfo>();

    if (!bb_info.insn_list || bb_info.insn_list->empty())
      continue;

    //处理shiftofview的部分
    if (auto label = std::dynamic_pointer_cast<LabelledInsn>(bb_info.insn_list->front())) {
      if (label->getLabel().find(".shift_of_view") == 0) {
        auto &list = bb_info.insn_list;
        for(auto it = list->begin(); it != list->end(); it++) {
          auto &insn = *it;
          if(auto common = std::dynamic_pointer_cast<CommonInsn>(insn)) {
            if (common->getOpcode() == OpCode::Assign) {
              // passed by register
              auto def = common->getDestination();
              var_to_interval[def] = intervals.size();
              intervals.emplace_back(def, instruction_index, instruction_index);
              intervals.back().setFloat(def->isFloatVar());

              auto arg = common->getSrcVar(0);
              auto &info = arg->prop<sym::RegAllocInfo>();
              assert(info.precoloured);
              var_to_interval[arg] = intervals.size();
              intervals.emplace_back(arg,-1,instruction_index);
              intervals.back().setFloat(arg->isFloatVar());
              intervals.back().assignReg(index_to_reg(info.consumed_registers));
            } else {
              // passed by stack
              auto def = common->getDestination();
              var_to_interval[def] = intervals.size();
              intervals.emplace_back(def, instruction_index, instruction_index);
              intervals.back().setFloat(def->isFloatVar());
            }
          }
          instruction_index++;
        }
        continue;
      }
    }

    InsnList::iterator next;
    for (auto it_inst = bb_info.insn_list->begin(); it_inst != bb_info.insn_list->end(); it_inst = next) {
      next = std::next(it_inst);
      auto &insn = *it_inst;
      // Handle definitions
      if (auto common_insn = std::dynamic_pointer_cast<CommonInsn>(insn)) {
        if (auto def = common_insn->getDestination()) {
          if (var_to_interval.find(def) == var_to_interval.end()) {
            var_to_interval[def] = intervals.size();
            intervals.emplace_back(def, instruction_index, instruction_index);
            intervals.back().setFloat(def->isFloatVar());
            // intervals.emplace_back(def, 0, instruction_index);
          }
        }
      }

      // Handle uses
      for (int i = 0; i < 2; ++i) {
        if (auto common_insn = std::dynamic_pointer_cast<CommonInsn>(insn)) {
          try {
            common_insn->getSrcVar(i);
          } catch (std::logic_error) {
            continue;
          }
          if (auto use = common_insn->getSrcVar(i)) {
            if (var_to_interval.find(use) == var_to_interval.end()) {
              var_to_interval[use] = intervals.size();
              intervals.emplace_back(use, instruction_index, instruction_index);
              intervals.back().setFloat(use->isFloatVar());
              // intervals.emplace_back(use, 0, instruction_index);
            }
          }
        }
      }

      // Handle Branch use
      if(auto branch_insn = std::dynamic_pointer_cast<BranchInsn>(insn)) {
        auto cond = branch_insn->getCond();
        if (var_to_interval.find(cond) == var_to_interval.end()) {
          var_to_interval[cond] = intervals.size();
          intervals.emplace_back(cond, instruction_index, instruction_index);
          intervals.back().setFloat(cond->isFloatVar());
          // intervals.emplace_back(cond, 0, instruction_index);
        }
      }

      instruction_index++;
    }
  }

  // Second pass: update end points based on liveness analysis
  int total = instruction_index;
  instruction_index = 0;
  for (auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
    auto &bb = *bb_ptr;
    const Liveness liveness = bb.prop<Liveness>();
    const BasicBlockInfo bb_info = bb.prop<BasicBlockInfo>();
    std::unordered_set<std::shared_ptr<sym::Var>> live_outs;
    for (int i = 0; i < liveness.live_out_idx; i ++) {
      live_outs.insert(liveness.live_out[i]);
    }

    if (!bb_info.insn_list || bb_info.insn_list->empty())
      continue;

    if (auto label = std::dynamic_pointer_cast<LabelledInsn>(bb_info.insn_list->front())) {
      if (label->getLabel().find(".shift_of_view") == 0) {
        auto &list = bb_info.insn_list;
        for(auto it = list->begin(); it != list->end(); it++) {
          auto &insn = *it;
          if(auto common = std::dynamic_pointer_cast<CommonInsn>(insn)) {
            auto def = common->getDestination();
            if (common->getOpcode() == OpCode::Assign) {
              // passed by register
              // intervals[var_to_interval[def]].setEnd(total);
              intervals[var_to_interval[def]].setEnd(bb_to_range[bb_ptr].second);
            } else {
              // passed by stack
              // intervals[var_to_interval[def]].setEnd(total);
              intervals[var_to_interval[def]].setEnd(bb_to_range[bb_ptr].second);
            }
          }
          instruction_index ++;
        }
        continue;
      }
    }
      // TODO: 用 backend 里的 Liveness 信息来更新 Endpoint
    for (auto it_inst = bb_info.insn_list->begin(); it_inst != bb_info.insn_list->end(); ++it_inst) {
      // Update end points for live-out variables
      // for (int i = 0; i < liveness.live_out_idx; ++i) {
      //   auto live_var = liveness.live_out[i];
      //   if (live_var) {
      //     auto it_interval = var_to_interval.find(live_var);
      //     if (it_interval != var_to_interval.end()) {
      //       intervals[it_interval->second].setEnd(instruction_index);
      //     }
      //   }
      // }

      auto &insn = *it_inst;
      // Handle definitions
      if (auto common_insn = std::dynamic_pointer_cast<CommonInsn>(insn)) {
        if (auto def = common_insn->getDestination()) {
          // auto it_interval = var_to_interval.find(def);
          // if (it_interval != var_to_interval.end()) {
          //   intervals[it_interval->second].setEnd(total);
          // }
          if (!live_outs.count(def)) {
            auto it = var_to_interval.find(def);
            intervals[it->second].setEnd(instruction_index);
          }
        }
      }

      // Handle uses
      for (int i = 0; i < 2; ++i) {
        if (auto common_insn = std::dynamic_pointer_cast<CommonInsn>(insn)) {
          try {
            common_insn->getSrcVar(i);
          } catch (std::logic_error) {
            continue;
          }
          if (auto use = common_insn->getSrcVar(i)) {
            // auto it_interval = var_to_interval.find(use);
            // if (it_interval != var_to_interval.end()) {
            //   intervals[it_interval->second].setEnd(total);
            // }
            if (!live_outs.count(use)) {
              auto it = var_to_interval.find(use);
              intervals[it->second].setEnd(instruction_index);
            }
          }
        }
      }

      // Handle Branch use
      if(auto branch_insn = std::dynamic_pointer_cast<BranchInsn>(insn)) {
        auto cond = branch_insn->getCond();
        auto it_interval = var_to_interval.find(cond);
        if (it_interval != var_to_interval.end()) {
          intervals[it_interval->second].setEnd(instruction_index);
        }
      }
      instruction_index++;
    }
    for (auto liveout : live_outs) {
      auto it_interval = var_to_interval.find(liveout);
      if (it_interval != var_to_interval.end()) {
        intervals[it_interval->second].setEnd(bb_to_range[bb_ptr].second);
      }
    }
  }

  // 在修改了 emit-backend.cc 后，这个 Third Pass 应该不再需要了
  // // Third Pass add callersave constraints for function call
  instruction_index = 0;
  for(auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
    auto &bb = *bb_ptr;
    const Liveness liveness = bb.prop<Liveness>();
    const BasicBlockInfo bb_info = bb.prop<BasicBlockInfo>();

    if (!bb_info.insn_list || bb_info.insn_list->empty())
      continue;

    InsnList::iterator next;
    for(auto it_inst = bb_info.insn_list->begin(); it_inst != bb_info.insn_list->end(); it_inst = next) {
      next = std::next(it_inst);
      auto &insn = *it_inst;
      if (insn->getOpCode() == OpCode::Call) {
        // TODO: 避免浪费+避免硬编码 8+8
        // 没有push的情况
        for (int i = 0; i <= 7; i++) {
          //auto reg = index_to_reg(i);
          auto parm = sym::Var::generateTemp(frontend::TypeFactory::IntTy, "parm"+std::to_string(i));
          auto &info = parm->prop<sym::RegAllocInfo>();
          info.consumed_registers = reg_to_index(RISCV_Reg::__a0) + i;
          info.precoloured = true;
          var_to_interval[parm] = intervals.size();
          intervals.emplace_back(parm, instruction_index, instruction_index);
          intervals.back().setFloat(false);
          intervals.back().assignReg(index_to_reg(info.consumed_registers));
        }
        // Float, 为了清晰目前没有和上面合成单个循环
        for (int i = 0; i <= 7; i++) {
          auto parm = sym::Var::generateTemp(frontend::TypeFactory::FloatTy, "fparm"+std::to_string(i));
          auto &info = parm->prop<sym::RegAllocInfo>();
          info.consumed_registers = reg_to_index(RISCV_Reg::__fa0) + i;
          info.precoloured = true;
          var_to_interval[parm] = intervals.size();
          intervals.emplace_back(parm, instruction_index, instruction_index);
          intervals.back().setFloat(true);
          intervals.back().assignReg(index_to_reg(info.consumed_registers));
        }
        instruction_index++;
        continue;
      }
      // TODO: Float
      if (insn->getOpCode() == OpCode::Push) {
        auto push = it_inst;
        auto call = it_inst;
        int count = 0;
        for(; (*call)->getOpCode() != OpCode::Call; call++) {
          count ++;
        }
        int int_index = 0, float_index = 0;
        for (; push != call; push++) {
            auto common = std::dynamic_pointer_cast<CommonInsn>(*push);
            auto operand = common->getSrc0();
            bool isFloat;
            std::visit(
              [&](auto arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::shared_ptr<sym::Var>>) {
                  isFloat = (arg->getType() == frontend::TypeFactory::FloatTy);
                } else if constexpr (std::is_same_v<T, int>) {
                  isFloat = false;
                } else {
                  isFloat = true;
                }
              },
              operand
            );
            if (isFloat) {
              if (float_index > 7) {
                continue;
              }
              auto parm = sym::Var::generateTemp(frontend::TypeFactory::FloatTy, "fparm" + std::to_string(float_index) + "_");
              auto &info = parm->prop<sym::RegAllocInfo>();
              info.precoloured = true;
              info.consumed_registers = reg_to_index(RISCV_Reg::__fa0) + float_index;
              var_to_interval[parm] = intervals.size();
              intervals.emplace_back(parm, instruction_index, instruction_index + count);
              intervals.back().setFloat(true);
              intervals.back().assignReg(index_to_reg(info.consumed_registers));
              float_index++;
            } else {
              if(int_index > 7) {
                continue;
              }
              auto parm = sym::Var::generateTemp(frontend::TypeFactory::IntTy, "parm" + std::to_string(int_index) + "_");
              auto &info = parm->prop<sym::RegAllocInfo>();
              info.precoloured = true;
              info.consumed_registers = reg_to_index(RISCV_Reg::__a0) + int_index;
              var_to_interval[parm] = intervals.size();
              intervals.emplace_back(parm, instruction_index, instruction_index + count);
              intervals.back().setFloat(false);
              intervals.back().assignReg(index_to_reg(info.consumed_registers));
              int_index++;
            }
        }
        // assert(int_index + float_index == count);
        for (; int_index <= 7; int_index++) {
            auto parm = sym::Var::generateTemp(frontend::TypeFactory::IntTy, "parm" + std::to_string(int_index) + "_");
            auto &info = parm->prop<sym::RegAllocInfo>();
            info.precoloured  = true;
            info.consumed_registers = reg_to_index(RISCV_Reg::__a0) + int_index;
            var_to_interval[parm] = intervals.size();
            intervals.emplace_back(parm, instruction_index + count, instruction_index + count);
            intervals.back().setFloat(false);
            intervals.back().assignReg(index_to_reg(info.consumed_registers));
        }
        for (; float_index <= 7; float_index++) {
            auto parm = sym::Var::generateTemp(frontend::TypeFactory::FloatTy, "fparm" + std::to_string(float_index) + "_");
            auto &info = parm->prop<sym::RegAllocInfo>();
            info.precoloured = true;
            info.consumed_registers = reg_to_index(RISCV_Reg::__fa0) + float_index;
            var_to_interval[parm] = intervals.size();
            intervals.emplace_back(parm, instruction_index + count, instruction_index + count);
            intervals.back().setFloat(true);
            intervals.back().assignReg(index_to_reg(info.consumed_registers));
        }
        next = std::next(call);
        instruction_index += count + 1;
        continue;
      }
      instruction_index++;
    }
  }

  return intervals;
}

bool linearScanAllocation(Function* func,
                          std::vector<LiveInterval>& intervals,
                          const std::vector<RISCV_Reg>& int_regs,
                          const std::vector<RISCV_Reg>& float_regs) {
    // Sort intervals by start point
    std::sort(intervals.begin(), intervals.end());
    for(auto &interval : intervals) {
      auto &info = interval.getVar()->prop<sym::RegAllocInfo>();
      std::cout << interval.getVar()->getName() << " : [start, end] " << interval.getStart() << " " << interval.getEnd() << (interval.isFloat()?" float":" int") << (info.precoloured? " precoloured " + std::to_string(info.consumed_registers): "") << std::endl;
    }
    // active intervals currently using registers
    std::vector<LiveInterval*> int_active;
    std::vector<LiveInterval*> float_active;
    // Available registers
    std::vector<RISCV_Reg> int_free_regs = int_regs;
    std::vector<RISCV_Reg> float_free_regs = float_regs;

    for (auto& interval : intervals) {
        auto &active = interval.isFloat() ? float_active : int_active;
        auto &free_regs= interval.isFloat() ? float_free_regs : int_free_regs;

        // Expire old intervals
        for (auto it = active.begin(); it != active.end();) {
            if ((*it)->getEnd() <= interval.getStart()) {
                // Free the register
                auto reg = (*it)->getAssignedReg();
                if (isFloatReg(reg)) {
                  float_free_regs.push_back(reg);
                } else {
                  int_free_regs.push_back(reg);
                }
                // free_regs.push_back((*it)->getAssignedReg());
                it = active.erase(it);
            } else {
                ++it;
            }
        }
        auto &info = interval.getVar()->prop<sym::RegAllocInfo>();
        if (info.precoloured) {
          // interval of precoloured var
          auto it = std::find(free_regs.begin(),free_regs.end(),interval.getAssignedReg());
          if (it != free_regs.end()) {
            // 此时并没有被分配
            free_regs.erase(it);
          } else {
            // 已经被分配了,则spill对应的interval
            for (auto victim_it = active.begin(); victim_it != active.end(); victim_it++) {
              auto victim = *victim_it;
              if(victim->getAssignedReg() == interval.getAssignedReg()) {
                victim->setSpilled(true);
                std::cout << "spill for precoloured reg, victim : " << victim->getVar()->getName() << std::endl;
                victim->assignReg(RISCV_Reg::INVALID);
                active.erase(victim_it);
                break;
              }
            }
          }
          active.push_back(&interval);
          std::cout << "(precoloured)assign " << reg_to_string(interval.getAssignedReg()) << " to " << interval.getVar()->getName() << "\n";
          continue;
        }

        if (!free_regs.empty()) {
            // Assign a register
            RISCV_Reg reg = free_regs.back();
            free_regs.pop_back();
            interval.assignReg(reg);
            active.push_back(&interval);
            std::cout << "(maybe)assign " << reg_to_string(reg) << " to " << interval.getVar()->getName() << "\n";
        } else {
            // Need to spill
            if (active.empty()) {
                // No active intervals to spill
                std::cout << "No active intervals to spill\n";
                return false;
            }

            // Find the interval with the furthest end point
            // auto spill_candidate = std::max_element(
            //     active.begin(), active.end(),
            //     [](const LiveInterval* a, const LiveInterval* b) {
            //         return a->getEnd() < b->getEnd();
            //     });

            std::vector<LiveInterval*>::iterator spill_candidate = active.end();
            for(auto it = active.begin(); it != active.end(); it++) {
              auto &interval = *it;
              auto &info = interval->getVar()->prop<sym::RegAllocInfo>();
              if (info.precoloured == true) {
                continue;
              }
              if (spill_candidate == active.end()) {
                spill_candidate = it;
              } else {
                if (interval->getEnd() > (*spill_candidate)->getEnd()) {
                  spill_candidate = it;
                }
              }
            }
            assert(spill_candidate != active.end());
            if ((*spill_candidate)->getEnd() > interval.getEnd()) {
                // Spill the candidate
                auto victim_reg = (*spill_candidate)->getAssignedReg();
                free_regs.push_back(victim_reg);
                (*spill_candidate)->setSpilled(true);
                (*spill_candidate)->assignReg(RISCV_Reg::INVALID);
                active.erase(spill_candidate);
                std::cout << "spill victim " << (*spill_candidate)->getVar()->getName() << " " << reg_to_string(victim_reg) << std::endl;

                // Assign the register to current interval
                RISCV_Reg reg = free_regs.back();
                free_regs.pop_back();
                interval.assignReg(reg);
                active.push_back(&interval);
                std::cout << "(maybe)assign " << reg_to_string(reg) << " to " << interval.getVar()->getName() << "\n";
            } else {
                // Spill current interval
                interval.setSpilled(true);
                interval.assignReg(RISCV_Reg::INVALID);
                std::cout << "spill current " << interval.getVar()->getName() << std::endl;
            }
        }
    }

    // another chance
    int_active.clear();
    float_active.clear();
    // Available registers
    int_free_regs = int_regs;
    float_free_regs = float_regs;
    int count = 0;
    for (int i = 0; i < intervals.size(); i++) {
      LiveInterval &interval = intervals[i];
      auto &active = interval.isFloat() ? float_active : int_active;
      auto &free_regs= interval.isFloat() ? float_free_regs : int_free_regs;
      // Expire old intervals
      for (auto it = active.begin(); it != active.end();) {
        if ((*it)->getEnd() <= interval.getStart()) {
          // Free the register
          auto reg = (*it)->getAssignedReg();
          if (isFloatReg(reg)) {
            float_free_regs.push_back(reg);
          } else {
            int_free_regs.push_back(reg);
          }
          // free_regs.push_back((*it)->getAssignedReg());
          it = active.erase(it);
        } else {
          ++it;
        }
      }
      if (!interval.isSpilled()) {
        auto it = std::find(free_regs.begin(),free_regs.end(),interval.getAssignedReg());
        assert(it != free_regs.end());
        free_regs.erase(it);
        active.push_back(&interval);
        continue;
      }
      std::set<RISCV_Reg> future_active;
      for (int j = i + 1; j < intervals.size(); j++) {
        LiveInterval &future = intervals[j];
        if (future.getStart() >= interval.getEnd()) {
          break;
        }
        if (!future.isSpilled()) {
          future_active.insert(future.getAssignedReg());
          continue;
        }
      }
      for (auto it = free_regs.begin(); it != free_regs.end(); it++) {
        auto reg = *it;
        if (future_active.count(reg)) {
          continue;
        }
        interval.setSpilled(false);
        interval.assignReg(reg);
        free_regs.erase(it);
        active.push_back(&interval);
        count++;
        std::cout << "(another)assign " << reg_to_string(reg) << " to " <<  interval.getVar()->getName() << "\n";
        break;
      }
    }
    std::cout << "Assign " << count << " registers in another chance phase\n";
    return true;
}

void insertSpillCode(Function* func, const std::vector<LiveInterval>& intervals) {
    // Calculate total stack space needed for spills
    int total_spill_space = 0;
    for (const auto& interval : intervals) {
        if (interval.isSpilled()) {
            total_spill_space += 8; // Assuming 8 bytes per spill slot
        }
    }

    // Assign stack offsets to spilled variables
    int current_offset = 0;
    for (const auto& interval : intervals) {
        if (interval.isSpilled()) {
          const_cast<LiveInterval&>(interval).setSpillOffset(current_offset);
          std::cout << "spilled " << interval.getVar()->getName() << " " << current_offset << (interval.isFloat()?" float ":" int ");
          std::cout << "interval " << interval.getStart() << " " << interval.getEnd() << "\n";
          current_offset += 8;
        } else {
          std::cout << "assign " << reg_to_string(interval.getAssignedReg()) << " to var " << interval.getVar()->getName() << (interval.isFloat()?" float ":" int ") ;
          std::cout << "interval " << interval.getStart() << " " << interval.getEnd() << "\n";
        }
    }

    std::map<std::shared_ptr<sym::Var>, int> var_to_interval;
    for (int i = 0; i < intervals.size(); i++) {
      auto &var = intervals[i].getVar();
      var_to_interval[var] = i;
    }

    // Insert spill/restore code
    for (auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
        auto& bb = *bb_ptr;
        const BasicBlockInfo bb_info = bb.prop<BasicBlockInfo>();

        if (bb_info.insn_list->empty())
            continue;
        auto insn = bb_info.insn_list->begin();
        auto next = insn;
        // for (auto& insn : *bb_info.insn_list) {
        for (; insn != bb_info.insn_list->end(); insn = next) {
            next = std::next(insn);
            if (auto branch_insn = std::dynamic_pointer_cast<BranchInsn>(*insn)) {
                auto cond = branch_insn->getCond();
                // 这里循环套循环，如果函数比较长，interval比较多，乘起来数量级可能在9次方级别，下同
                // for (const auto& interval : intervals) {
                auto &interval = intervals[var_to_interval[cond]];
                if (interval.getVar() == cond && interval.isSpilled()) {
                  if (interval.isFloat()) {
                    auto ft = std::make_shared<sym::Var>(frontend::TypeFactory::FloatTy, "ft0");
                    auto &finfo = ft->prop<sym::RegAllocInfo>();
                    finfo.precoloured = true;
                    finfo.consumed_registers = reg_to_index(RISCV_Reg::__ft0);
                    auto load = std::make_shared<CommonInsn>(OpCode::Load_sp, ft, frontend::Operand(interval.getSpillOffset()), nullptr);
                    bb_info.insn_list->insert(insn, load);
                    branch_insn->setCond(ft);
                  } else {
                    auto t = std::make_shared<sym::Var>(frontend::TypeFactory::IntTy, "t0");
                    t->prop<sym::RegAllocInfo>().consumed_registers = reg_to_index(RISCV_Reg::__t0);
                    auto load = std::make_shared<CommonInsn>(OpCode::Load_sp, t, frontend::Operand(interval.getSpillOffset()), nullptr);
                    bb_info.insn_list->insert(insn, load);
                    branch_insn->setCond(t);
                  }
                }
                // }
            }
            if (auto common_insn = std::dynamic_pointer_cast<CommonInsn>(*insn)) {
                // Handle spilled destinations
                if (auto def = common_insn->getDestination()) {
                    // for (const auto& interval : intervals) {
                  auto &interval = intervals[var_to_interval[def]];
                  if (interval.getVar() == def && interval.isSpilled()) {
                    if (interval.isFloat()) {
                      auto ft0 = std::make_shared<sym::Var>(frontend::TypeFactory::FloatTy, "ft0");
                      auto &finfo = ft0->prop<sym::RegAllocInfo>();
                      finfo.precoloured = true;
                      finfo.consumed_registers = reg_to_index(RISCV_Reg::__ft0);
                      common_insn->setDestination(ft0);
                      auto fstore = std::make_shared<CommonInsn>(OpCode::Store_sp,nullptr, ft0, frontend::Operand(interval.getSpillOffset()));
                      bb_info.insn_list->insert(next, fstore);
                    } else {
                      auto t0 = std::make_shared<sym::Var>(frontend::TypeFactory::IntTy,"t0");
                      t0->prop<sym::RegAllocInfo>().consumed_registers = reg_to_index(RISCV_Reg::__t0);
                      common_insn->setDestination(t0);
                      auto store = std::make_shared<CommonInsn>(OpCode::Store_sp, nullptr, t0, frontend::Operand(interval.getSpillOffset()));
                      bb_info.insn_list->insert(next, store);
                    }
                  }
                }

                // Handle spilled sources
                for (int i = 0; i < 2; ++i) {
                    try {
                      common_insn->getSrcVar(i);
                    } catch (std::logic_error) {
                      continue;
                    }
                    if (auto use = common_insn->getSrcVar(i)) {

                      // for (const auto& interval : intervals) {
                      auto &interval = intervals[var_to_interval[use]];
                      if (interval.getVar() == use && interval.isSpilled()) {
                        if (interval.isFloat()) {
                          auto ft = std::make_shared<sym::Var>(frontend::TypeFactory::FloatTy, "ft" + std::to_string(i+1));
                          auto &finfo = ft->prop<sym::RegAllocInfo>();
                          finfo.precoloured = true;
                          finfo.consumed_registers = reg_to_index((i == 0 ? RISCV_Reg::__ft1 : RISCV_Reg::__ft2));
                          auto load = std::make_shared<CommonInsn>(OpCode::Load_sp, ft, frontend::Operand(interval.getSpillOffset()),nullptr);
                          bb_info.insn_list->insert(insn, load);
                          common_insn->setSrc(i, ft);
                        } else {
                          auto t = std::make_shared<sym::Var>(frontend::TypeFactory::IntTy, "t" + std::to_string(i + 1));
                          if (i == 0) {
                            t->prop<sym::RegAllocInfo>().consumed_registers = reg_to_index(RISCV_Reg::__t1);
                          } else {
                            t->prop<sym::RegAllocInfo>().consumed_registers = reg_to_index(RISCV_Reg::__t2);
                          }
                          auto load = std::make_shared<CommonInsn>(OpCode::Load_sp, t, frontend::Operand(interval.getSpillOffset()),nullptr);
                          bb_info.insn_list->insert(insn, load);
                          common_insn->setSrc(i, t);
                        }
                      }
                    }
                }
            }
        }
    }

    // Update function's stack size
    if (total_spill_space > 0) {
        func->addStackSize(total_spill_space);
    }
}

bool performLinearScan(Function* func,
                    const std::vector<RISCV_Reg>& available_regs,
                    const std::vector<RISCV_Reg>& float_regs,
                    std::vector<LiveInterval>& out_intervals) {
    std::cout << "linear scan func received " << func->name << std::endl;
    for(auto bb_ptr = func->bbs; bb_ptr; bb_ptr = bb_ptr->getNext()) {
      auto insn_list = bb_ptr->getInsnList();
      for(auto it: insn_list) {
        // std::cout << "\t" << it->getDest() << " " << it->getSrc0() << " " << it->getSrc1() << std::endl;
        // std::cout << "\t";
        // if(it->getDest()) std::cout << it->getDest() << " ";
        // std::cout << it->getSrc0() << " " << it->getSrc1() << "\n";
        // if(it->getSrc0()) std::cout << it->getSrc0() << " ";
        // if(it->getSrc0()) std::cout << it->getSrc0() << " ";

      }
    }
    if (!linearScanAllocation(func, out_intervals, available_regs, float_regs))
        return false;

    insertSpillCode(func, out_intervals);

    // Propagate results back to variable properties
    for (auto& interval : out_intervals) {
        auto& info = interval.getVar()->prop<sym::RegAllocInfo>();
        if (interval.isSpilled()) {
            info.consumed_registers = reg_to_index(RISCV_Reg::INVALID);
            info.stack_offset = interval.getSpillOffset();
        } else {
            func->reg_used.set(static_cast<size_t>(interval.getAssignedReg()));
            info.consumed_registers = reg_to_index(interval.getAssignedReg());
            info.stack_offset = -1;
        }
    }

    return true;
}

} // namespace backend
} // namespace aaac
