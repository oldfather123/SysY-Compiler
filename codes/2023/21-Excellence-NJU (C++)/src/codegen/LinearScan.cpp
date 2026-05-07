#include "codegen/LinearScan.hpp"
#include "codegen/MachineBlock.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"
#include "codegen/MachineUnit.hpp"
#include "codegen/utils/NotImplemented.hpp"
#include "Function.h"
#include "magic_enum.hpp"
#include "codegen/utils/UtilFunctions.hpp"

#include <iomanip>
#include <ostream>
#include <string_view>
#include <vector>

std::vector<IREGISTER> LinearScan::init_free_regs{
    T0, T1, T2, T3, T4, T5, T6,
    S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11
};
std::vector<Fregister> LinearScan::init_free_f_regs{
    Fregister::ft0,
    Fregister::ft1,
    Fregister::ft2,
    Fregister::ft3,
    Fregister::ft4,
    Fregister::ft5,
    Fregister::ft6,
    Fregister::ft7,
    Fregister::ft8,
    Fregister::ft9,
    Fregister::ft10,
    Fregister::ft11,

    Fregister::fs0,
    Fregister::fs1,
    Fregister::fs2,
    Fregister::fs3,
    Fregister::fs4,
    Fregister::fs5,
    Fregister::fs6,
    Fregister::fs7,
    Fregister::fs8,
    Fregister::fs9,
    Fregister::fs10,
    Fregister::fs11,
};

static MachineOperand *getMOperandReg(int physical_reg, bool fpu = false) {
    return new MachineOperand(MachineOperand::REG, physical_reg, fpu);
}
static MachineOperand *getMOperandImm(int imm) {
    return new MachineOperand(MachineOperand::IMM, imm);
}

void LinearScan::pass(MachineUnit *mUnit) {

    for (auto mFunc : mUnit->func_list) {
        computeLiveIntervals(mFunc);

        
        // 初始化可分配的寄存器
        free_regs = std::move(std::vector<IREGISTER>(init_free_regs.rbegin(), init_free_regs.rend()));
        free_f_regs = std::move(std::vector<Fregister>(init_free_f_regs.rbegin(), init_free_f_regs.rend()));

        // 其他初始化
        intervals.clear(), active.clear();
        f_intervals.clear(), f_active.clear();
        reg_allocation.clear();
        // 寄存器分配
        linearScanRegisterAllocation(mFunc);

        // std::cerr << mFunc->IR_func->name << " spills " << spilledReg << " register(s). ";

        rewrite(mFunc);
    }
    // std::cerr << std::endl;
}

void LinearScan::computeLiveIntervals(MachineFunction *mFunc) {
    auto IR_func = mFunc->IR_func;
    std::vector<BasicBlock *> bb_post_order = std::move(IR_func->getReversePostOrder());

    for (auto bb_it = bb_post_order.rbegin(); bb_it != bb_post_order.rend(); ++bb_it) {
        auto mBlock = (*bb_it)->mBlock;
        for (auto vreg : mBlock->live_out) {
            auto &live_range = mFunc->live_ranges[vreg];
            live_range.insert({mBlock->begin_no, mBlock->end_no});
        }

        for (auto inst_it = mBlock->inst_list.rbegin(); inst_it != mBlock->inst_list.rend(); ++inst_it) {
            for (auto def : (*inst_it)->def_list) {
                if (def->type != MachineOperand::VREG) continue;
                auto &live_range = mFunc->live_ranges[def->vreg_name];
                live_range.insert({(*inst_it)->no, (*inst_it)->no});

                for (auto range_it = live_range.begin(); range_it != live_range.end();) {
                    if ((*inst_it)->no != mBlock->begin_no && range_it->first == mBlock->begin_no) {
                        live_range.insert({(*inst_it)->no, range_it->second});
                        live_range.erase(range_it++);
                    } else {
                        ++range_it;
                    }
                }
            }

            for (auto use : (*inst_it)->use_list) {
                if (use->type != MachineOperand::VREG) continue;
                auto &live_range = mFunc->live_ranges[use->vreg_name];
                live_range.insert({mBlock->begin_no, (*inst_it)->no});
            }
        }
    }
    for (auto bb_it = bb_post_order.rbegin(); bb_it != bb_post_order.rend(); ++bb_it) {
        auto mBlock = (*bb_it)->mBlock;
        for (auto vreg : mBlock->f_live_out) {
            auto &f_live_range = mFunc->f_live_ranges[vreg];
            f_live_range.insert({mBlock->begin_no, mBlock->end_no});
        }

        for (auto inst_it = mBlock->inst_list.rbegin(); inst_it != mBlock->inst_list.rend(); ++inst_it) {
            for (auto def : (*inst_it)->def_list) {
                if (def->type != MachineOperand::FVREG) continue;
                auto &f_live_range = mFunc->f_live_ranges[def->vreg_name];
                f_live_range.insert({(*inst_it)->no, (*inst_it)->no});

                for (auto range_it = f_live_range.begin(); range_it != f_live_range.end();) {
                    if ((*inst_it)->no != mBlock->begin_no && range_it->first == mBlock->begin_no) {
                        f_live_range.insert({(*inst_it)->no, range_it->second});
                        f_live_range.erase(range_it++);
                    } else {
                        ++range_it;
                    }
                }
            }

            for (auto use : (*inst_it)->use_list) {
                if (use->type != MachineOperand::FVREG) continue;
                auto &f_live_range = mFunc->f_live_ranges[use->vreg_name];
                f_live_range.insert({mBlock->begin_no, (*inst_it)->no});
            }
        }
    }

    for (auto &[vreg, live_range] : mFunc->live_ranges) {
        auto &live_intervals = mFunc->live_intervals;
        live_intervals[vreg] = *live_range.begin();
        for (auto [begin, end] : live_range) {
            live_intervals[vreg].first = std::min(live_intervals[vreg].first, begin);
            live_intervals[vreg].second = std::max(live_intervals[vreg].second, end);
        }
    }
    for (auto &[vreg, f_live_range] : mFunc->f_live_ranges) {
        auto &f_live_intervals = mFunc->f_live_intervals;
        f_live_intervals[vreg] = *f_live_range.begin();
        for (auto [begin, end] : f_live_range) {
            f_live_intervals[vreg].first = std::min(f_live_intervals[vreg].first, begin);
            f_live_intervals[vreg].second = std::max(f_live_intervals[vreg].second, end);
        }
    }
}

std::ostream & operator << (std::ostream &os, const LinearScan::Interval &i) {
    os << setw(6) << i.vreg_name << "   [" << setw(2) << i.begin << ", " << setw(2) << i.end << "]   ";
    if (!i.spilled) {
        if (!i.fpu) os << codegen_utils::lower(magic_enum::enum_name(static_cast<IREGISTER>(i.phy_reg)));
        else os << magic_enum::enum_name(static_cast<Fregister>(i.phy_reg));
    } else {
        os << i.offset;
    }
    return os;
}

void LinearScan::linearScanRegisterAllocation(MachineFunction *mFunc) {
    for (auto [vreg, intvl] : mFunc->live_intervals) {
        intervals.emplace_back(new Interval{vreg, intvl.first, intvl.second, false});
    }
    for (auto [vreg, intvl] : mFunc->f_live_intervals) {
        f_intervals.emplace_back(new Interval{vreg, intvl.first, intvl.second, true});
    }

    // 按活跃区间interval的开始点从小到大排序
    std::sort(intervals.begin(), intervals.end(), [] (std::unique_ptr<Interval> &a, std::unique_ptr<Interval> &b) -> bool { return a->begin < b->begin; });
    std::sort(f_intervals.begin(), f_intervals.end(), [] (std::unique_ptr<Interval> &a, std::unique_ptr<Interval> &b) -> bool { return a->begin < b->begin; });
    // std::sort(intervals.begin(), intervals.end(), IntervalBeginCmper());
    // std::sort(f_intervals.begin(), f_intervals.end(), IntervalBeginCmper());

    for (auto &interval : intervals) {
        expireOldIntervals(interval.get());
        if (active.size() == init_free_regs.size()) {
            spillInterval(interval.get(), mFunc->framesize);
        } else {
            assert(free_regs.back());
            interval->phy_reg = free_regs.back();
            free_regs.pop_back();
            active.insert(interval.get());
            // std::cerr << interval->vreg_name << " " << interval->phy_reg << ' ' << active.size() << ' ' << free_regs.size() << std::endl;
        }
    }
    for (auto &f_interval : f_intervals) {
        expireOldIntervals_f(f_interval.get());
        if (f_active.size() == init_free_f_regs.size()) {
            spillInterval_f(f_interval.get(), mFunc->framesize);
        } else {
            f_interval->phy_reg = free_f_regs.back();
            free_f_regs.pop_back();
            f_active.insert(f_interval.get());
        }
    }

    for (auto &interval : intervals) reg_allocation[interval->vreg_name] = std::move(interval);
    for (auto &f_interval : f_intervals) reg_allocation[f_interval->vreg_name] = std::move(f_interval);

    // std::cerr << "Live Intervals and Allocated Registers of Function '" << mFunc->IR_func->name << "':" << std::endl;
    // for (auto &[vreg, interval] : reg_allocation) if (!interval->fpu) std::cerr << setw(10) << mFunc->IR_func->name << *interval << '\n';
    // std::cerr << std::endl;
    // for (auto &[vreg, interval] : reg_allocation) if (interval->fpu) std::cerr << setw(10) << mFunc->IR_func->name << *interval << '\n';
    // std::cerr << std::endl;
}

void LinearScan::expireOldIntervals(Interval *i) {
    for (auto it = active.begin(); it != active.end();) {
        auto j = *it;
        if (j->end >= i->begin) return;
        active.erase(it++);
        free_regs.push_back(IREGISTER(j->phy_reg));
        // std::cerr << "expire " << j->vreg_name << ' ' << j->phy_reg << ' ' << active.size() << ' ' << free_regs.size() << std::endl;
    }
}
void LinearScan::expireOldIntervals_f(Interval *i) {
    for (auto it = f_active.begin(); it != f_active.end();) {
        auto j = *it;
        if (j->end >= i->begin) return;
        f_active.erase(it++);
        free_f_regs.push_back(Fregister(j->phy_reg));
    }
}

void LinearScan::spillInterval(Interval *i, int framesize) {
    // 3 temp * 8 Bytes
    int offset = - framesize - 24;
    ++spilledReg;
    // TODO (done) BUG!!! offset 是一直固定的，没有变化
    // std::cerr << "before spill " << active.size() << std::endl;
    auto spill = *active.rbegin();
    if (spill->end > i->end) {
        i->phy_reg = spill->phy_reg;
        spill->spilled = true;
        spill->offset = offset - spilledReg * 8;
        active.erase(spill);
        active.insert(i);
    } else {
        i->spilled = true;
        i->offset = offset - spilledReg * 8;
    }
    // std::cerr << "after spill " << active.size() << std::endl;
}
void LinearScan::spillInterval_f(Interval *i, int framesize) {
    // 3 temp * 8 Bytes
    int offset = - framesize - 24;
    ++spilledReg;
    // TODO (done) BUG!!! offset 是一直固定的，没有变化
    auto spill = *f_active.rbegin();
    if (spill->end > i->end) {
        i->phy_reg = spill->phy_reg;
        spill->spilled = true;
        spill->offset = offset - spilledReg * 8;
        f_active.erase(spill);
        f_active.insert(i);
    } else {
        i->spilled = true;
        i->offset = offset - spilledReg * 8;
    }
}

void LinearScan::updateUsedPhysicalRegs(const std::string &func_name, IREGISTER reg) {
    if (T0 <= reg && reg <= T2 || T3 <= reg && reg <= T6) caller_saved_regs[func_name].insert(reg);
    else if (reg == S1 || S2 <= reg && reg <= S11) callee_saved_regs[func_name].insert(reg);
    else if (reg == FP) {
        std::cerr << "对S0的错误使用!";
        assert(false);
    } else {
        std::cerr << reg << " 使用了非通用寄存器!";
        assert(false);
    }
}
void LinearScan::updateUsedPhysicalRegs_f(const std::string &func_name, Fregister reg) {
    assert(reg >= ft0);
    if (ft0 <= reg && reg <= ft7 || ft8 <= reg && reg <= ft11) caller_saved_f_regs[func_name].insert(reg);
    else if (fs0 <= reg && reg <= fs1 || fs2 <= reg && reg <= fs11) callee_saved_f_regs[func_name].insert(reg);
    else {
        std::cerr << reg << " 使用了非通用寄存器!";
        assert(false);
    }
}

static int getNewPhyReg(std::set<int> &used_regs, bool fpu);
static void rewriteLdSd(MachineInstruction *inst);

void LinearScan::rewrite(MachineFunction *mFunc) {
    // int spilledReg{0};
    for (auto inst : mFunc->no2inst) {
        for (auto &def : inst->def_list) {
            if (def->type != MachineOperand::VREG && def->type != MachineOperand::FVREG) continue;
            auto alloc = reg_allocation[def->vreg_name].get();
            if (!alloc->spilled) {
                if (!alloc->fpu) updateUsedPhysicalRegs(mFunc->IR_func->name, IREGISTER(alloc->phy_reg));
                else updateUsedPhysicalRegs_f(mFunc->IR_func->name, Fregister(alloc->phy_reg));
                delete def;
                def = new MachineOperand(MachineOperand::REG, alloc->phy_reg, alloc->phy_reg > 31);
                def->parent = inst;
            } else {
                // TODO
                // throw NotImplemented("寄存器溢出！");
                // ++spilledReg;
            }
        }

        for (auto &use : inst->use_list) {
            if (use->type != MachineOperand::VREG && use->type != MachineOperand::FVREG) continue;
            auto alloc = reg_allocation[use->vreg_name].get();
            if (!alloc->spilled) {
                if (!alloc->fpu) updateUsedPhysicalRegs(mFunc->IR_func->name, IREGISTER(alloc->phy_reg));
                else updateUsedPhysicalRegs_f(mFunc->IR_func->name, Fregister(alloc->phy_reg));
                delete use;
                use = new MachineOperand(MachineOperand::REG, alloc->phy_reg, alloc->phy_reg > 31);
                use->parent = inst;
            } else {
                // TODO
                // throw NotImplemented("寄存器溢出！");
                // ++spilledReg;
            }
        }
    }

    // 保存 caller-saved registers
    // for (auto inst : mFunc->no2inst) {
    for (std::size_t no = 0; no < mFunc->no2inst.size(); ++no) {
        auto inst = mFunc->no2inst[no];
        if (inst->type == MachineInstruction::JUMP && inst->optype == JalMInstruction::JAL) {
            // TODO 移到参数压栈之前 (done)
            assert(typeid(*inst) == typeid(JalMInstruction));
            auto &label = inst->use_list[1]->label;
            std::set<int> regs{};
            regs.insert(caller_saved_regs[mFunc->IR_func->name].begin(), caller_saved_regs[mFunc->IR_func->name].end());
            regs.insert(caller_saved_f_regs[mFunc->IR_func->name].begin(), caller_saved_f_regs[mFunc->IR_func->name].end());
            regs.insert(T0);
            // regs.insert({T0, T1, T2, T3, T4, T5, T6});
            if (!regs.empty()) {
                auto cur = inst;
                int args_cnt = 0;
                if (label.find("starttime") != std::string::npos || label.find("stoptime") != std::string::npos) args_cnt = 1;
                else if (label.find("memset") != std::string::npos) args_cnt = 3;
                else args_cnt = mFunc->parent->IR_func_table[label]->funcType->arguments.size();

                if (args_cnt > 8) {
                    int inst_index = no - (args_cnt - 8) - 1;
                    cur = mFunc->no2inst[inst_index];
                    if (cur->type != MachineInstruction::BINARY || cur->def_list[0]->type != MachineOperand::REG || cur->def_list[0]->reg_no != IREGISTER::SP || cur->optype != BinaryMInstruction::ADD) {
                        while (cur->type != BinaryMInstruction::BINARY || cur->def_list[0]->type != MachineOperand::REG || cur->def_list[0]->reg_no != IREGISTER::SP || cur->optype != BinaryMInstruction::ADD) {
                            --inst_index;
                            cur = mFunc->no2inst[inst_index];
                        }
                    }
                    --inst_index;
                    cur = mFunc->no2inst[inst_index];
                }
                // TODO 栈16字节对齐 (done)
                int delta{static_cast<int>(regs.size())};
                if (delta & 1) ++delta;
                delta <<= 3;
                auto addi1 = new BinaryMInstruction(cur->parent, BinaryMInstruction::ADDI,
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP), 
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP), 
                                                new MachineOperand(MachineOperand::IMM, -delta)
                                            );
                cur->insertBefore(addi1);
                int cnt = 0;
                for (auto reg : regs) {
                    auto sd = new StoreMInstruction(cur->parent,
                                                            reg < 32 ? StoreMInstruction::SD : StoreMInstruction::FSW,
                                                            new MachineOperand(MachineOperand::REG, reg, reg > 31),
                                                            new MachineOperand(MachineOperand::IMM, cnt * 8),
                                                            new MachineOperand(MachineOperand::REG, IREGISTER::SP)
                                                        );
                    cur->insertBefore(sd);
                    ++cnt;
                }
                
                cnt = 0;
                cur = inst;
                if (args_cnt > 8) cur = mFunc->no2inst[no + 2];
                for (auto reg : regs) {
                    auto ld = new LoadMInstruction(inst->parent,
                                                            reg < 32 ? LoadMInstruction::LD : LoadMInstruction::FLW,
                                                            new MachineOperand(MachineOperand::REG, reg, reg > 31),
                                                            new MachineOperand(MachineOperand::IMM, cnt * 8),
                                                            new MachineOperand(MachineOperand::REG, IREGISTER::SP)
                                                        );
                    cur->insertAfter(ld);
                    cur = ld;
                    ++cnt;
                }

                auto addi2 = new BinaryMInstruction(inst->parent, BinaryMInstruction::ADDI,
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP), 
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP), 
                                                new MachineOperand(MachineOperand::IMM, delta)
                                            );
                cur->insertAfter(addi2);
            }
        }
    }

    int spilled_region_start_offset = -mFunc->framesize;
    
    // 保存 callee-saved registers
    // auto &regs = callee_saved_regs[mFunc->IR_func->name];
    std::set<int> regs;
    regs.insert(callee_saved_regs[mFunc->IR_func->name].begin(), callee_saved_regs[mFunc->IR_func->name].end());
    regs.insert(callee_saved_f_regs[mFunc->IR_func->name].begin(), callee_saved_f_regs[mFunc->IR_func->name].end());
    // if (!regs.empty()) std::cerr << "保存寄存器\n";
    regs.insert(S1);
    // regs.insert({S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11});
    if (!regs.empty() || mFunc->framesize % 16 != 0) {
        int cnt{0};
        assert(mFunc->block_list[0]);
        // auto cur = mFunc->block_list[0]->inst_list[3];

        auto it = mFunc->block_list[0]->inst_list.begin();
        ++it, ++it, ++it;
        auto cur = *it;

        assert(cur->optype == BinaryMInstruction::ADDI);
        int delta{static_cast<int>(regs.size() << 3)};

        if (!regs.empty()) {
            // 保证栈16字节对齐
            if ((mFunc->framesize + delta + (spilledReg + 3) * 8) % 16 != 0) {
                int tmp = (mFunc->framesize + delta + (spilledReg + 3) * 8 + 15) >> 4;
                delta = (tmp << 4) - mFunc->framesize;
            } else {
                delta += (spilledReg + 3) << 3;
            }
        } else {
            int tmp = (mFunc->framesize + 15) >> 4;
            delta = (tmp << 4) - mFunc->framesize;
        }
        mFunc->framesize += delta;
        cur->use_list[1]->val -= delta;

        // std::cerr << mFunc->IR_func->name << ' ' << mFunc->framesize << ' ' << cur->use_list[1]->val << ' ' << spilledReg << std::endl;

        // 修正fp ra的偏移量
        // mFunc->block_list[0]->inst_list[1]->use_list[1]->val += delta;
        // mFunc->block_list[0]->inst_list[2]->use_list[1]->val += delta;

        // cur = mFunc->block_list[0]->inst_list[3];
        it = mFunc->block_list[0]->inst_list.begin();
        ++it, ++it, ++it;
        cur = *it;

        // std::cerr << cur->no << ' ' << cur->use_list[1]->val << std::endl;
        // cur->use_list[1]->val += delta;
        while (cur->use_list[1]->val < -2032) {
            auto addi = new BinaryMInstruction(mFunc->block_list[0], BinaryMInstruction::ADDI,
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                                    new MachineOperand(MachineOperand::IMM, -2032));
            cur->insertBefore(addi);
            cur->use_list[1]->val += 2032;
        }

        for (auto reg : regs) {
            auto sd = new StoreMInstruction(cur->parent,
                                                    reg < 32 ? StoreMInstruction::SD : StoreMInstruction::FSW,
                                                    new MachineOperand(MachineOperand::REG, reg, reg > 31),
                                                    new MachineOperand(MachineOperand::IMM, cnt * 8),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::SP)
                                                    );
            cur->insertAfter(sd), cur = sd;
            ++cnt;
        }

        for (auto block : mFunc->block_list) {
            if (block->inst_list.back()->type == MachineInstruction::JUMP && block->inst_list.back()->optype == JrMInstruction::JR) {
                // block->inst_list[block->inst_list.size() - 4]->use_list[0]->val += delta;
                // block->inst_list[block->inst_list.size() - 3]->use_list[0]->val += delta;
                // auto cur = block->inst_list[block->inst_list.size() - 5];
                auto it = block->inst_list.end();
                --it, --it, --it, --it, --it;
                auto cur = *it;
                cnt = 0;

                for (auto reg : regs) {
                    auto ld = new LoadMInstruction(block,
                                                    reg < 32 ? LoadMInstruction::LD : LoadMInstruction::FLW,
                                                    new MachineOperand(MachineOperand::REG, reg, reg > 31),
                                                    new MachineOperand(MachineOperand::IMM, cnt * 8),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::SP)
                                                    );
                    if (block->inst_list.size() <= 4) {
                        ld->current_it = block->inst_list.insert(block->inst_list.begin(), ld);
                    }
                    else cur->insertAfter(ld);
                    cur = ld;
                    ++cnt;
                }
                // cur = block->inst_list[block->inst_list.size() - 4];
                it = block->inst_list.end();
                --it, --it, --it, --it;
                cur = *it;

                cur->use_list[1]->val += delta;

                while (cur->use_list[1]->val > 2032) {
                    auto addi = new BinaryMInstruction(block, BinaryMInstruction::ADDI,
                                                            new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                                            new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                                            new MachineOperand(MachineOperand::IMM, 2032));
                    cur->insertBefore(addi);
                    cur->use_list[1]->val -= 2032;
                }
            }
        }
    }

    if (spilledReg > 0) {
        for (auto inst : mFunc->no2inst) {
            // 初始化
            std::set<int> used_pregs{};
            int temp_offset = spilled_region_start_offset;

            for (auto operand : inst->use_list)
                if (operand->type == MachineOperand::REG) used_pregs.insert(operand->reg_no);
            for (auto operand : inst->def_list)
                if (operand->type == MachineOperand::REG) used_pregs.insert(operand->reg_no);

            for (auto &use : inst->use_list) {
                if (use->type != MachineOperand::VREG && use->type != MachineOperand::FVREG) continue;
                assert(reg_allocation[use->vreg_name]);
                auto alloc = reg_allocation[use->vreg_name].get();
                assert(alloc->spilled);

                int preg = getNewPhyReg(used_pregs, alloc->fpu);
                int slot_offset = alloc->offset;
                temp_offset -= 8;

                auto store = new StoreMInstruction(inst->parent, 
                                        !alloc->fpu ? StoreMInstruction::SD : StoreMInstruction::FSW,
                                        new MachineOperand(MachineOperand::REG, preg, alloc->fpu),
                                        new MachineOperand(MachineOperand::IMM, temp_offset),
                                        new MachineOperand(MachineOperand::REG, FP));
                inst->insertBefore(store);

                auto load1 = new LoadMInstruction(inst->parent, 
                                        !alloc->fpu ? LoadMInstruction::LD : LoadMInstruction::FLW,
                                        getMOperandReg(preg, alloc->fpu),
                                        getMOperandImm(slot_offset),
                                        getMOperandReg(IREGISTER(FP)));
                inst->insertBefore(load1);

                delete use;
                use = getMOperandReg(preg, alloc->fpu);

                auto load2 = new LoadMInstruction(inst->parent,
                                        !alloc->fpu ? LoadMInstruction::LD : LoadMInstruction::FLW,
                                        getMOperandReg(preg, alloc->fpu),
                                        getMOperandImm(temp_offset),
                                        getMOperandReg(FP));
                inst->insertAfter(load2);
                rewriteLdSd(store);
                rewriteLdSd(load1);
                rewriteLdSd(load2);
            }
            
            for (auto &def : inst->def_list) {
                if (def->type != MachineOperand::VREG && def->type != MachineOperand::FVREG) continue;
                auto alloc = reg_allocation[def->vreg_name].get();
                assert(alloc->spilled);

                int preg = getNewPhyReg(used_pregs, alloc->fpu);
                int slot_offset = alloc->offset;
                temp_offset -= 8;

                auto store1 = new StoreMInstruction(inst->parent,
                                        !alloc->fpu ? StoreMInstruction::SD : StoreMInstruction::FSW,
                                        new MachineOperand(MachineOperand::REG, preg, alloc->fpu),
                                        new MachineOperand(MachineOperand::IMM, temp_offset),
                                        new MachineOperand(MachineOperand::REG, FP));
                inst->insertBefore(store1);

                delete def;
                def = getMOperandReg(preg, alloc->fpu);

                auto store2 = new StoreMInstruction(inst->parent,
                                        !alloc->fpu ? StoreMInstruction::SD : StoreMInstruction::FSW,
                                        getMOperandReg(preg, alloc->fpu),
                                        getMOperandImm(slot_offset),
                                        getMOperandReg(IREGISTER(FP)));
                inst->insertAfter(store2);

                auto load = new LoadMInstruction(inst->parent,
                                        !alloc->fpu ? LoadMInstruction::LD : LoadMInstruction::FLW,
                                        getMOperandReg(preg, alloc->fpu),
                                        getMOperandImm(temp_offset),
                                        getMOperandReg(FP));
                store2->insertAfter(load);
                rewriteLdSd(store1);
                rewriteLdSd(store2);
                rewriteLdSd(load);
            }
        }
    }
}

int getNewPhyReg(std::set<int> &used_regs, bool fpu) {
    if (!fpu) {
        for (auto preg : LinearScan::init_free_regs) {
            if (used_regs.find(preg) == used_regs.end()) {
                used_regs.insert(preg);
                return IREGISTER(preg);
            }
        }
    } else {
        for (auto preg : LinearScan::init_free_f_regs) {
            if (used_regs.find(preg) == used_regs.end()) {
                used_regs.insert(preg);
                return Fregister(preg);
            }
        }
    }

    // 无意义，只是为了消除编译器警告
    return -1;
}

void pushToStack(IREGISTER reg, MachineInstruction *inst) {
    auto addi = new BinaryMInstruction(inst->parent, BinaryMInstruction::ADDI,
                                    getMOperandReg(IREGISTER::SP), getMOperandReg(IREGISTER::SP),
                                    getMOperandImm(-16));
    inst->insertBefore(addi);
    auto sd = new StoreMInstruction(inst->parent, StoreMInstruction::SD,
                                        getMOperandReg(reg),
                                        getMOperandImm(0), getMOperandReg(IREGISTER::SP));
    inst->insertBefore(sd);
}
void popFromStack(IREGISTER reg, MachineInstruction *inst) {
    auto ld = new LoadMInstruction(inst->parent, LoadMInstruction::LD,
                                            getMOperandReg(reg),
                                            getMOperandImm(0), getMOperandReg(IREGISTER::SP));
    inst->insertAfter(ld);
    auto addi = new BinaryMInstruction(inst->parent, BinaryMInstruction::ADDI,
                                    getMOperandReg(IREGISTER::SP), getMOperandReg(IREGISTER::SP),
                                    getMOperandImm(16));
    ld->insertAfter(addi);
}

/**
 * @brief 将一条"offset超限的"的ld/sd
 *  sd preg, offset(fp) 改写为若干条 指令
 *  addi fp, fp, offset;
 *  sd preg, 0(fp);
 *  addi fp, fp, offset;
 * 
 * @param inst 必须是一条ld/sd
 */
void rewriteLdSd(MachineInstruction *inst) {
    MachineBlock *block = inst->parent;
    int offset{-1};
    if (inst->type == MachineInstruction::LOAD) {
        if (abs(inst->use_list[0]->val) <= 2032) return;
        offset = inst->use_list[0]->val;
        inst->use_list[0]->val = 0;
        inst->use_list[1]->reg_no = IREGISTER::S11;
    } else {
        if (abs(inst->use_list[1]->val) <= 2032) return;
        offset = inst->use_list[1]->val;
        inst->use_list[1]->val = 0;
        inst->use_list[2]->reg_no = IREGISTER::S11;
    }

    auto li = new LoadImmInstruction(block, LoadImmInstruction::LI,
                        getMOperandReg(IREGISTER::S11),
                        getMOperandImm(offset));
    inst->insertBefore(li);

    pushToStack(IREGISTER::S11, li);

    auto add = new BinaryMInstruction(block, BinaryMInstruction::ADD,
                            getMOperandReg(IREGISTER::S11),
                            getMOperandReg(IREGISTER::S11),
                            getMOperandReg(IREGISTER::FP));
    inst->insertBefore(add);

    popFromStack(IREGISTER::S11, inst);
}