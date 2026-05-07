#include "RISCv64LinearScan.h"
#include "RISCv64LLIR.h"
#include "RISCv64ISel.h"
#include "RISCv64Info.h"
#include "RISCv64AsmPrinter.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <sstream>
#include <functional>

// 外部调试级别控制变量
extern int DEBUG;
extern int DEEPDEBUG;
extern int DEEPERDEBUG;

namespace sysy {

// --- 调试辅助函数 ---
// These helpers are self-contained and only used for logging.
static std::string pregToString(PhysicalReg preg) {
    // This map is a copy from AsmPrinter to avoid dependency issues.
    static const std::map<PhysicalReg, std::string> preg_names = {
        {PhysicalReg::ZERO, "zero"}, {PhysicalReg::RA, "ra"}, {PhysicalReg::SP, "sp"}, {PhysicalReg::GP, "gp"}, {PhysicalReg::TP, "tp"},
        {PhysicalReg::T0, "t0"}, {PhysicalReg::T1, "t1"}, {PhysicalReg::T2, "t2"}, {PhysicalReg::T3, "t3"}, {PhysicalReg::T4, "t4"}, {PhysicalReg::T5, "t5"}, {PhysicalReg::T6, "t6"},
        {PhysicalReg::S0, "s0"}, {PhysicalReg::S1, "s1"}, {PhysicalReg::S2, "s2"}, {PhysicalReg::S3, "s3"}, {PhysicalReg::S4, "s4"}, {PhysicalReg::S5, "s5"}, {PhysicalReg::S6, "s6"}, {PhysicalReg::S7, "s7"}, {PhysicalReg::S8, "s8"}, {PhysicalReg::S9, "s9"}, {PhysicalReg::S10, "s10"}, {PhysicalReg::S11, "s11"},
        {PhysicalReg::A0, "a0"}, {PhysicalReg::A1, "a1"}, {PhysicalReg::A2, "a2"}, {PhysicalReg::A3, "a3"}, {PhysicalReg::A4, "a4"}, {PhysicalReg::A5, "a5"}, {PhysicalReg::A6, "a6"}, {PhysicalReg::A7, "a7"},
        {PhysicalReg::F0, "f0"}, {PhysicalReg::F1, "f1"}, {PhysicalReg::F2, "f2"}, {PhysicalReg::F3, "f3"}, {PhysicalReg::F4, "f4"}, {PhysicalReg::F5, "f5"}, {PhysicalReg::F6, "f6"}, {PhysicalReg::F7, "f7"},
        {PhysicalReg::F8, "f8"}, {PhysicalReg::F9, "f9"}, {PhysicalReg::F10, "f10"}, {PhysicalReg::F11, "f11"}, {PhysicalReg::F12, "f12"}, {PhysicalReg::F13, "f13"}, {PhysicalReg::F14, "f14"}, {PhysicalReg::F15, "f15"},
        {PhysicalReg::F16, "f16"}, {PhysicalReg::F17, "f17"}, {PhysicalReg::F18, "f18"}, {PhysicalReg::F19, "f19"}, {PhysicalReg::F20, "f20"}, {PhysicalReg::F21, "f21"}, {PhysicalReg::F22, "f22"}, {PhysicalReg::F23, "f23"},
        {PhysicalReg::F24, "f24"}, {PhysicalReg::F25, "f25"}, {PhysicalReg::F26, "f26"}, {PhysicalReg::F27, "f27"}, {PhysicalReg::F28, "f28"}, {PhysicalReg::F29, "f29"}, {PhysicalReg::F30, "f30"}, {PhysicalReg::F31, "f31"},
        {PhysicalReg::INVALID, "INVALID"}
    };
    if (preg_names.count(preg)) return preg_names.at(preg);
    return "UnknownPreg";
}

template<typename T>
static std::string setToString(const std::set<T>& s, std::function<std::string(T)> formatter) {
    std::stringstream ss;
    ss << "{ ";
    bool first = true;
    for (const auto& item : s) {
        if (!first) ss << ", ";
        ss << formatter(item);
        first = false;
    }
    ss << " }";
    return ss.str();
}

static std::string vregSetToString(const std::set<unsigned>& s) {
    return setToString<unsigned>(s, [](unsigned v){ return "%v" + std::to_string(v); });
}

static std::string pregSetToString(const std::set<PhysicalReg>& s) {
    return setToString<PhysicalReg>(s, pregToString);
}

// Helper function to check if a register is callee-saved.
// Defined locally to avoid scope issues.
static bool isCalleeSaved(PhysicalReg preg) {
    if (preg >= PhysicalReg::S0 && preg <= PhysicalReg::S11) return true;
    if (preg >= PhysicalReg::F8 && preg <= PhysicalReg::F9) return true;
    if (preg >= PhysicalReg::F18 && preg <= PhysicalReg::F27) return true;
    return false;
}


RISCv64LinearScan::RISCv64LinearScan(MachineFunction* mfunc)
    : MFunc(mfunc), 
      ISel(mfunc->getISel()),
      vreg_type_map(ISel->getVRegTypeMap()) {
    
    allocable_int_regs = {
        PhysicalReg::T0, PhysicalReg::T1, PhysicalReg::T2, PhysicalReg::T3, PhysicalReg::T6,
        PhysicalReg::S1, PhysicalReg::S2, PhysicalReg::S3, PhysicalReg::S4, PhysicalReg::S5, PhysicalReg::S6, PhysicalReg::S7,
        PhysicalReg::S8, PhysicalReg::S9, PhysicalReg::S10, PhysicalReg::S11,
    };
    allocable_fp_regs = {
        PhysicalReg::F0, PhysicalReg::F1, PhysicalReg::F2, PhysicalReg::F3, PhysicalReg::F4, PhysicalReg::F5, PhysicalReg::F6, PhysicalReg::F7,
        PhysicalReg::F10, PhysicalReg::F11, PhysicalReg::F12, PhysicalReg::F13, PhysicalReg::F14, PhysicalReg::F15, PhysicalReg::F16, PhysicalReg::F17,
        PhysicalReg::F8, PhysicalReg::F9, PhysicalReg::F18, PhysicalReg::F19, PhysicalReg::F20, PhysicalReg::F21, PhysicalReg::F22,
        PhysicalReg::F23, PhysicalReg::F24, PhysicalReg::F25, PhysicalReg::F26, PhysicalReg::F27,
        PhysicalReg::F28, PhysicalReg::F29, PhysicalReg::F30, PhysicalReg::F31,
    };
    if (MFunc->getFunc()) {
        int int_arg_idx = 0;
        int fp_arg_idx = 0;
        for (Argument* arg : MFunc->getFunc()->getArguments()) {
            unsigned arg_vreg = ISel->getVReg(arg);
            if (arg->getType()->isFloat()) {
                if (fp_arg_idx < 8) {
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + fp_arg_idx++);
                    abi_vreg_map[arg_vreg] = preg;
                }
            } else {
                if (int_arg_idx < 8) {
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + int_arg_idx++);
                    abi_vreg_map[arg_vreg] = preg;
                }
            }
        }
    }
}

bool RISCv64LinearScan::run() {
    if (DEBUG) std::cerr << "===== [LSRA] Running for function: " << MFunc->getName() << " =====\n";
    
    const int MAX_ITERATIONS = 3;

    for (int iteration = 1; ; ++iteration) {
        if (DEBUG && iteration > 1) {
             std::cerr << "\n----- [LSRA] Re-running iteration " << iteration << " -----\n";
        }
        
        linearizeBlocks();
        computeLiveIntervals();
        bool needs_spill = linearScan();

        // 如果当前这轮线性扫描不需要溢出，说明分配成功，直接跳出循环。
        if (!needs_spill) {
            break; 
        }

        // --- 检查是否需要启动或已经失败于保底策略 ---
        if (iteration > MAX_ITERATIONS) {
            // 如果我们已经在保底模式下运行过，但这一轮 linearScan 仍然返回 true，
            // 这说明发生了无法解决的错误，此时才真正失败。
            if (conservative_spill_mode) {
                std::cerr << "\n!!!!!! [LSRA-FATAL] Allocation failed to converge even in Conservative Spill Mode. Triggering final fallback. !!!!!!\n\n";
                return false; // 返回失败，而不是exit
            }
            // 这是第一次达到最大迭代次数，触发保底策略。
            std::cerr << "\n!!!!!! [LSRA-WARN] Convergence failed after " << MAX_ITERATIONS 
                      << " iterations. Entering Conservative Spill Mode for the next attempt. !!!!!!\n\n";
            conservative_spill_mode = true; // 开启保守溢出模式，将在下一次循环生效
        }
        
        // 只要需要溢出，就重写程序
        if (DEBUG) std::cerr << "[LSRA] Spilling detected, will rewrite program.\n";
        rewriteProgram();
    }

    if (DEBUG) std::cerr << "[LSRA] Applying final allocation.\n";
    applyAllocation();
    MFunc->getFrameInfo().vreg_to_preg_map = this->vreg_to_preg_map;
    collectUsedCalleeSavedRegs();

    if (DEBUG) std::cerr << "===== [LSRA] Finished for function: " << MFunc->getName() << " =====\n\n";
    return true; // 分配成功
}

void RISCv64LinearScan::linearizeBlocks() {
    linear_order_blocks.clear();
    for (auto& mbb : MFunc->getBlocks()) {
        linear_order_blocks.push_back(mbb.get());
    }
}

void RISCv64LinearScan::computeLiveIntervals() {
    if (DEBUG) std::cerr << "[LSRA-Live] Starting live interval computation.\n";
    instr_numbering.clear();
    live_intervals.clear();
    unhandled.clear();

    int num = 0;
    std::set<int> call_locations;
    for (auto* mbb : linear_order_blocks) {
        for (auto& instr : mbb->getInstructions()) {
            instr_numbering[instr.get()] = num;
            if (instr->getOpcode() == RVOpcodes::CALL) call_locations.insert(num);
            num += 2;
        }
    }

    if (DEEPDEBUG) std::cerr << "  [Live] Starting live variable dataflow analysis...\n";
    std::map<const MachineBasicBlock*, std::set<unsigned>> live_in, live_out;
    bool changed = true;
    int df_iter = 0;
    while(changed) {
        changed = false;
        df_iter++;
        std::vector<MachineBasicBlock*> reversed_blocks = linear_order_blocks;
        std::reverse(reversed_blocks.begin(), reversed_blocks.end());
        for(auto* mbb : reversed_blocks) {
            std::set<unsigned> old_live_in = live_in[mbb];
            std::set<unsigned> current_live_out;
            for (auto* succ : mbb->successors) current_live_out.insert(live_in[succ].begin(), live_in[succ].end());
            std::set<unsigned> use, def;
            std::set<unsigned> temp_live = current_live_out;
            auto& instrs = mbb->getInstructions();
            for (auto it = instrs.rbegin(); it != instrs.rend(); ++it) {
                use.clear(); def.clear();
                getInstrUseDef(it->get(), use, def);
                for (unsigned vreg : def) temp_live.erase(vreg);
                for (unsigned vreg : use) temp_live.insert(vreg);
            }
            if (live_in[mbb] != temp_live || live_out[mbb] != current_live_out) {
                changed = true;
                live_in[mbb] = temp_live;
                live_out[mbb] = current_live_out;
            }
        }
    }
    if (DEEPDEBUG) std::cerr << "  [Live] Dataflow analysis converged after " << df_iter << " iterations.\n";
    if (DEEPERDEBUG) {
        std::cerr << "  [Live-Debug] Live-in sets:\n";
        for (auto* mbb : linear_order_blocks) std::cerr << "    " << mbb->getName() << ": " << vregSetToString(live_in[mbb]) << "\n";
        std::cerr << "  [Live-Debug] Live-out sets:\n";
        for (auto* mbb : linear_order_blocks) std::cerr << "    " << mbb->getName() << ": " << vregSetToString(live_out[mbb]) << "\n";
    }

    if (DEEPDEBUG) std::cerr << "  [Live] Building precise intervals...\n";
    std::map<unsigned, int> first_def, last_use;
    for (auto* mbb : linear_order_blocks) {
        for (auto& instr_ptr : mbb->getInstructions()) {
            int instr_num = instr_numbering.at(instr_ptr.get());
            std::set<unsigned> use, def;
            getInstrUseDef(instr_ptr.get(), use, def);
            for (unsigned vreg : def) if (first_def.find(vreg) == first_def.end()) first_def[vreg] = instr_num;
            for (unsigned vreg : use) last_use[vreg] = instr_num;
        }
    }
    if (DEEPERDEBUG) {
        std::cerr << "  [Live-Debug] First def points:\n";
        for (auto const& [vreg, pos] : first_def) std::cerr << "    %v" << vreg << ": " << pos << "\n";
        std::cerr << "  [Live-Debug] Last use points:\n";
        for (auto const& [vreg, pos] : last_use) std::cerr << "    %v" << vreg << ": " << pos << "\n";
    }

    for (auto const& [vreg, start] : first_def) {
        live_intervals.emplace(vreg, LiveInterval(vreg));
        auto& interval = live_intervals.at(vreg);
        interval.start = start;
        interval.end = last_use.count(vreg) ? last_use.at(vreg) : start;
    }

    for (auto const& [mbb, live_set] : live_out) {
        if (mbb->getInstructions().empty()) continue;
        int block_end_num = instr_numbering.at(mbb->getInstructions().back().get());
        for (unsigned vreg : live_set) {
            if (live_intervals.count(vreg)) {
                if (DEEPERDEBUG && live_intervals.at(vreg).end < block_end_num) {
                    std::cerr << "  [Live-Debug] Extending interval for %v" << vreg << " from " << live_intervals.at(vreg).end << " to " << block_end_num << " due to live_out of " << mbb->getName() << "\n";
                }
                live_intervals.at(vreg).end = std::max(live_intervals.at(vreg).end, block_end_num);
            }
        }
    }
    
    for (auto& pair : live_intervals) {
        auto& interval = pair.second;
        auto it = call_locations.lower_bound(interval.start);
        if (it != call_locations.end() && *it < interval.end) interval.crosses_call = true;
    }

    for (auto& pair : live_intervals) unhandled.push_back(&pair.second);
    std::sort(unhandled.begin(), unhandled.end(), [](const LiveInterval* a, const LiveInterval* b){ return a->start < b->start; });

    if (DEBUG) {
        std::cerr << "[LSRA-Live] Finished. Total intervals: " << unhandled.size() << "\n";
        if (DEEPDEBUG) {
            std::cerr << "  [Live] Computed Intervals (vreg: [start, end]):\n";
            for(const auto* interval : unhandled) {
                std::cerr << "    %v" << interval->vreg << ": [" << interval->start << ", " << interval->end << "]" 
                          << (interval->crosses_call ? " (crosses call)" : "") << "\n";
            }
        }
    }

    // ================== 新增的调试代码 ==================
    // 检查活性分析找到的vreg与指令扫描找到的vreg是否一致
    if (DEEPERDEBUG) {
        // 修正：将 std.set 修改为 std::set
        std::set<unsigned> vregs_from_liveness;
        for (const auto& pair : live_intervals) {
            vregs_from_liveness.insert(pair.first);
        }

        std::set<unsigned> vregs_from_instr_scan;
        for (auto* mbb : linear_order_blocks) {
            for (auto& instr_ptr : mbb->getInstructions()) {
                std::set<unsigned> use, def;
                getInstrUseDef(instr_ptr.get(), use, def);
                vregs_from_instr_scan.insert(use.begin(), use.end());
                vregs_from_instr_scan.insert(def.begin(), def.end());
            }
        }
        
        std::cerr << "  [Live-Debug] VReg Consistency Check:\n";
        std::cerr << "    VRegs found by Liveness Analysis: " << vregs_from_liveness.size() << "\n";
        std::cerr << "    VRegs found by getInstrUseDef Scan: " << vregs_from_instr_scan.size() << "\n";

        // 修正：将 std.set 修改为 std::set
        std::set<unsigned> diff;
        std::set_difference(vregs_from_liveness.begin(), vregs_from_liveness.end(),
                            vregs_from_instr_scan.begin(), vregs_from_instr_scan.end(),
                            std::inserter(diff, diff.begin()));

        if (!diff.empty()) {
            std::cerr << "    !!!!!! [Live-Debug] DISCREPANCY DETECTED !!!!!!\n";
            std::cerr << "    The following vregs were found by liveness but NOT by getInstrUseDef scan:\n";
            std::cerr << "    " << vregSetToString(diff) << "\n";
        } else {
            std::cerr << "    [Live-Debug] VReg sets are consistent.\n";
        }
    }
    // ======================================================
}

bool RISCv64LinearScan::linearScan() {
    // ================== 终极保底策略 (新逻辑) ==================
    // 当此标志位为true时，我们进入最暴力的溢出模式。
    if (conservative_spill_mode) {
        if (DEBUG) std::cerr << "[LSRA-Scan-Panic] In Conservative Mode. Spilling all unhandled vregs.\n";
        
        // 1. 清空溢出列表，准备重新计算
        spilled_vregs.clear();

        // 2. 遍历所有计算出的活性区间
        for (auto& pair : live_intervals) {
            // 3. 如果一个vreg不是ABI规定的寄存器，就必须溢出
            if (abi_vreg_map.find(pair.first) == abi_vreg_map.end()) {
                spilled_vregs.insert(pair.first);
            }
        }
        
        // 4. 只要有任何vreg被标记为溢出，就返回true以触发最终的rewriteProgram。
        //    下一轮迭代时，由于所有vreg都已被重写，将不再有新的溢出，保证收敛。
        return !spilled_vregs.empty();
    }
    // ==========================================================
    

    // ================== 常规线性扫描逻辑 (您已有的代码) ==================
    // 只有在非保守模式下才会执行以下代码
    if (DEBUG) std::cerr << "[LSRA-Scan] Starting main linear scan algorithm.\n";
    active.clear();
    spilled_vregs.clear();
    vreg_to_preg_map.clear();

    std::set<PhysicalReg> free_caller_int_regs, free_callee_int_regs;
    std::set<PhysicalReg> free_caller_fp_regs, free_callee_fp_regs;

    for (auto preg : allocable_int_regs) {
        if (isCalleeSaved(preg)) free_callee_int_regs.insert(preg); else free_caller_int_regs.insert(preg);
    }
    for (auto preg : allocable_fp_regs) {
        if (isCalleeSaved(preg)) free_callee_fp_regs.insert(preg); else free_caller_fp_regs.insert(preg);
    }

    if (DEEPDEBUG) {
        std::cerr << "  [Scan] Initial free regs:\n";
        std::cerr << "    Caller-Saved Int: " << pregSetToString(free_caller_int_regs) << "\n";
        std::cerr << "    Callee-Saved Int: " << pregSetToString(free_callee_int_regs) << "\n";
    }

    vreg_to_preg_map.insert(abi_vreg_map.begin(), abi_vreg_map.end());
    std::vector<LiveInterval*> normal_unhandled;
    for(LiveInterval* interval : unhandled) {
        if(abi_vreg_map.count(interval->vreg)) {
            active.push_back(interval);
            PhysicalReg preg = abi_vreg_map.at(interval->vreg);
            if (isFPVReg(interval->vreg)) {
                if(isCalleeSaved(preg)) free_callee_fp_regs.erase(preg); else free_caller_fp_regs.erase(preg);
            } else {
                if(isCalleeSaved(preg)) free_callee_int_regs.erase(preg); else free_caller_int_regs.erase(preg);
            }
        } else {
            normal_unhandled.push_back(interval);
        }
    }
    unhandled = normal_unhandled;
    std::sort(active.begin(), active.end(), [](const LiveInterval* a, const LiveInterval* b){ return a->end < b->end; });
    
    for (LiveInterval* current : unhandled) {
        if (DEEPDEBUG) std::cerr << "\n  [Scan] Processing interval %v" << current->vreg << " [" << current->start << ", " << current->end << "]\n";
        
        std::vector<LiveInterval*> new_active;
        for (LiveInterval* active_interval : active) {
            if (active_interval->end < current->start) {
                PhysicalReg preg = vreg_to_preg_map.at(active_interval->vreg);
                if (DEEPDEBUG) std::cerr << "    [Scan] Expiring interval %v" << active_interval->vreg << ", freeing " << pregToString(preg) << "\n";
                if (isFPVReg(active_interval->vreg)) {
                        if(isCalleeSaved(preg)) free_callee_fp_regs.insert(preg); else free_caller_fp_regs.insert(preg);
                } else {
                        if(isCalleeSaved(preg)) free_callee_int_regs.insert(preg); else free_caller_int_regs.insert(preg);
                }
            } else {
                new_active.push_back(active_interval);
            }
        }
        active = new_active;

        bool is_fp = isFPVReg(current->vreg);
        auto& free_caller = is_fp ? free_caller_fp_regs : free_caller_int_regs;
        auto& free_callee = is_fp ? free_callee_fp_regs : free_callee_int_regs;
        PhysicalReg allocated_preg = PhysicalReg::INVALID;

        if (current->crosses_call) {
            if (!free_callee.empty()) {
                allocated_preg = *free_callee.begin();
                free_callee.erase(allocated_preg);
            }
        } else {
            if (!free_caller.empty()) {
                allocated_preg = *free_caller.begin();
                free_caller.erase(allocated_preg);
            } else if (!free_callee.empty()) {
                allocated_preg = *free_callee.begin();
                free_callee.erase(allocated_preg);
            }
        }

        if (allocated_preg != PhysicalReg::INVALID) {
            if (DEEPDEBUG) std::cerr << "    [Scan] Allocated " << pregToString(allocated_preg) << " to %v" << current->vreg << "\n";
            vreg_to_preg_map[current->vreg] = allocated_preg;
            active.push_back(current);
            std::sort(active.begin(), active.end(), [](const LiveInterval* a, const LiveInterval* b){ return a->end < b->end; });
        } else {
            if (DEEPDEBUG) std::cerr << "    [Scan] No free registers for %v" << current->vreg << ". Spilling...\n";
            spillAtInterval(current);
        }
    }
    return !spilled_vregs.empty();
}

void RISCv64LinearScan::spillAtInterval(LiveInterval* current) {
    // 保持您的原始逻辑
    LiveInterval* spill_candidate = nullptr;
    if (!active.empty()) {
        spill_candidate = active.back();
    }
    
    if (DEEPERDEBUG) {
        std::cerr << "      [Spill-Debug] Spill decision for current=%v" << current->vreg << "[" << current->start << "," << current->end << "]\n";
        std::cerr << "      [Spill-Debug] Active intervals (sorted by end point):\n";
        for (const auto* i : active) {
            std::cerr << "        %v" << i->vreg << "[" << i->start << "," << i->end << "] in " << pregToString(vreg_to_preg_map[i->vreg]) << "\n";
        }
        if(spill_candidate) {
            std::cerr << "      [Spill-Debug] Candidate is %v" << spill_candidate->vreg << ". Its end is " << spill_candidate->end << ", current's end is " << current->end << "\n";
        } else {
            std::cerr << "      [Spill-Debug] No active candidate.\n";
        }
    }

    if (spill_candidate && spill_candidate->end > current->end) {
        if (DEEPDEBUG) std::cerr << "      [Spill] Decision: Spilling active %v" << spill_candidate->vreg << ".\n";
        PhysicalReg preg = vreg_to_preg_map.at(spill_candidate->vreg);
        vreg_to_preg_map.erase(spill_candidate->vreg); // 确保移除旧映射
        vreg_to_preg_map[current->vreg] = preg;
        active.pop_back();
        active.push_back(current);
        std::sort(active.begin(), active.end(), [](const LiveInterval* a, const LiveInterval* b){ return a->end < b->end; });
        spilled_vregs.insert(spill_candidate->vreg);
    } else {
        if (DEEPDEBUG) std::cerr << "      [Spill] Decision: Spilling current %v" << current->vreg << ".\n";
        spilled_vregs.insert(current->vreg);
    }
}

void RISCv64LinearScan::rewriteProgram() {
    if (DEBUG) {
        std::cerr << "[LSRA-Rewrite] Starting program rewrite. Spilled vregs: " << vregSetToString(spilled_vregs) << "\n";
    }
    StackFrameInfo& frame_info = MFunc->getFrameInfo();
    int spill_current_offset = frame_info.locals_end_offset - frame_info.spill_size;

    for (unsigned vreg : spilled_vregs) {
        // 保持您的原始逻辑
        if (frame_info.spill_offsets.count(vreg)) continue;
        
        Type* type = vreg_type_map.count(vreg) ? vreg_type_map.at(vreg) : Type::getIntType();
        int size = isFPVReg(vreg) ? 4 : (type->isPointer() ? 8 : 4);
        spill_current_offset -= size;
        spill_current_offset = (spill_current_offset & ~7);
        frame_info.spill_offsets[vreg] = spill_current_offset;
        if (DEEPDEBUG) std::cerr << "  [Rewrite] Assigned new stack offset " << frame_info.spill_offsets.at(vreg) << " to spilled %v" << vreg << "\n";
    }
    frame_info.spill_size = -(spill_current_offset - frame_info.locals_end_offset);

    for (auto& mbb : MFunc->getBlocks()) {
        auto& instrs = mbb->getInstructions();
        std::vector<std::unique_ptr<MachineInstr>> new_instrs;
        if (DEEPERDEBUG) std::cerr << "  [Rewrite] Processing block " << mbb->getName() << "\n";
        
        for (auto it = instrs.begin(); it != instrs.end(); ++it) {
            auto& instr = *it;
            std::set<unsigned> use_vregs, def_vregs;
            getInstrUseDef(instr.get(), use_vregs, def_vregs);
            
            if (conservative_spill_mode) {
                // ================== 紧急模式重写逻辑 ==================
                // 直接使用物理寄存器 t4 (SPILL_TEMP_REG) 进行加载/存储
                
                // 为调试日志准备一个指令打印机
                auto printer = DEEPERDEBUG ? std::make_unique<RISCv64AsmPrinter>(MFunc) : nullptr;
                auto original_instr_str_for_log = DEEPERDEBUG ? printer->formatInstr(instr.get()) : "";
                bool modified = false;

                for (unsigned old_vreg : use_vregs) {
                    if (spilled_vregs.count(old_vreg)) {
                        modified = true;
                        Type* type = vreg_type_map.at(old_vreg);
                        RVOpcodes load_op = isFPVReg(old_vreg) ? RVOpcodes::FLW : (type->isPointer() ? RVOpcodes::LD : RVOpcodes::LW);
                        auto load = std::make_unique<MachineInstr>(load_op);
                        // 直接加载到保留的物理寄存器
                        load->addOperand(std::make_unique<RegOperand>(SPILL_TEMP_REG));
                        load->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))));
                        
                        if (DEEPERDEBUG) {
                            std::cerr << "    [Rewrite-Panic] Inserting LOAD for use of %v" << old_vreg 
                                      << " into " << pregToString(SPILL_TEMP_REG) 
                                      << " before: " << original_instr_str_for_log << "\n";
                        }
                        new_instrs.push_back(std::move(load));

                        // 替换指令中的操作数
                        instr->replaceVRegWithPReg(old_vreg, SPILL_TEMP_REG);
                    }
                }

                // 在处理 def 之前，先替换定义自身的 vreg
                for (unsigned old_vreg : def_vregs) {
                    if (spilled_vregs.count(old_vreg)) {
                        modified = true;
                        instr->replaceVRegWithPReg(old_vreg, SPILL_TEMP_REG);
                    }
                }
                
                // 将原始指令（可能已被修改）放入新列表
                new_instrs.push_back(std::move(instr));
                if (DEEPERDEBUG && modified) {
                    std::cerr << "    [Rewrite-Panic] Original: " << original_instr_str_for_log 
                              << " -> Rewritten: " << printer->formatInstr(new_instrs.back().get()) << "\n";
                }
                
                for (unsigned old_vreg : def_vregs) {
                    if (spilled_vregs.count(old_vreg)) {
                        // 指令本身已经被修改为定义到 SPILL_TEMP_REG，现在从它存回内存
                        Type* type = vreg_type_map.at(old_vreg);
                        RVOpcodes store_op = isFPVReg(old_vreg) ? RVOpcodes::FSW : (type->isPointer() ? RVOpcodes::SD : RVOpcodes::SW);
                        auto store = std::make_unique<MachineInstr>(store_op);
                        store->addOperand(std::make_unique<RegOperand>(SPILL_TEMP_REG));
                        store->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))));
                        if (DEEPERDEBUG) {
                             std::cerr << "    [Rewrite-Panic] Inserting STORE for def of %v" << old_vreg 
                                       << " from " << pregToString(SPILL_TEMP_REG) << " after original instr.\n";
                        }
                        new_instrs.push_back(std::move(store));
                    }
                }

            } else {
                // ================== 常规模式重写逻辑 (您的原始代码) ==================
                std::map<unsigned, unsigned> use_remap, def_remap;
                for (unsigned old_vreg : use_vregs) {
                    if (spilled_vregs.count(old_vreg) && use_remap.find(old_vreg) == use_remap.end()) {
                        Type* type = vreg_type_map.at(old_vreg);
                        unsigned new_temp_vreg = ISel->getNewVReg(type);
                        use_remap[old_vreg] = new_temp_vreg;
                        RVOpcodes load_op = isFPVReg(old_vreg) ? RVOpcodes::FLW : (type->isPointer() ? RVOpcodes::LD : RVOpcodes::LW);
                        auto load = std::make_unique<MachineInstr>(load_op);
                        load->addOperand(std::make_unique<RegOperand>(new_temp_vreg));
                        load->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))));
                        if (DEEPERDEBUG) {
                            RISCv64AsmPrinter printer(MFunc);
                            std::cerr << "    [Rewrite] Inserting LOAD for use of %v" << old_vreg << " into new %v" << new_temp_vreg << " before: " << printer.formatInstr(instr.get()) << "\n";
                        }
                        new_instrs.push_back(std::move(load));
                    }
                }
                for (unsigned old_vreg : def_vregs) {
                    if (spilled_vregs.count(old_vreg) && def_remap.find(old_vreg) == def_remap.end()) {
                        Type* type = vreg_type_map.at(old_vreg);
                        unsigned new_temp_vreg = ISel->getNewVReg(type);
                        def_remap[old_vreg] = new_temp_vreg;
                    }
                }
                auto original_instr_str_for_log = DEEPERDEBUG ? RISCv64AsmPrinter(MFunc).formatInstr(instr.get()) : "";
                instr->remapVRegs(use_remap, def_remap);
                new_instrs.push_back(std::move(instr));
                if (DEEPERDEBUG && (!use_remap.empty() || !def_remap.empty())) std::cerr << "    [Rewrite] Original: " << original_instr_str_for_log << " -> Rewritten: " << RISCv64AsmPrinter(MFunc).formatInstr(new_instrs.back().get()) << "\n";
                for(const auto& pair : def_remap) {
                    unsigned old_vreg = pair.first;
                    unsigned new_temp_vreg = pair.second;
                    Type* type = vreg_type_map.at(old_vreg);
                    RVOpcodes store_op = isFPVReg(old_vreg) ? RVOpcodes::FSW : (type->isPointer() ? RVOpcodes::SD : RVOpcodes::SW);
                    auto store = std::make_unique<MachineInstr>(store_op);
                    store->addOperand(std::make_unique<RegOperand>(new_temp_vreg));
                    store->addOperand(std::make_unique<MemOperand>(
                        std::make_unique<RegOperand>(PhysicalReg::S0),
                        std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))));
                    if (DEEPERDEBUG) std::cerr << "    [Rewrite] Inserting STORE for def of %v" << old_vreg << " from new %v" << new_temp_vreg << " after original instr.\n";
                    new_instrs.push_back(std::move(store));
                }
            }
        }
        instrs = std::move(new_instrs);
    }
}

void RISCv64LinearScan::applyAllocation() {
    if (DEBUG) std::cerr << "[LSRA-Apply] Applying final vreg->preg mapping.\n";
    for (auto& mbb : MFunc->getBlocks()) {
        for (auto& instr_ptr : mbb->getInstructions()) {
            for (auto& op_ptr : instr_ptr->getOperands()) {
                if (op_ptr->getKind() == MachineOperand::KIND_REG) {
                    auto reg_op = static_cast<RegOperand*>(op_ptr.get());
                    if (reg_op->isVirtual()) {
                        unsigned vreg = reg_op->getVRegNum();
                        if (vreg_to_preg_map.count(vreg)) {
                            reg_op->setPReg(vreg_to_preg_map.at(vreg));
                        } else {
                            std::cerr << "ERROR: Uncolored virtual register %v" << vreg << " found during applyAllocation! in func " << MFunc->getName() << "\n";
                            // Forcing an error is better than silent failure.
                            // reg_op->setPReg(PhysicalReg::T5); 
                        }
                    }
                } else if (op_ptr->getKind() == MachineOperand::KIND_MEM) {
                    auto mem_op = static_cast<MemOperand*>(op_ptr.get());
                    auto reg_op = mem_op->getBase();
                    if (reg_op->isVirtual()) {
                        unsigned vreg = reg_op->getVRegNum();
                        if (vreg_to_preg_map.count(vreg)) {
                            reg_op->setPReg(vreg_to_preg_map.at(vreg));
                        } else {
                            std::cerr << "ERROR: Uncolored virtual register %v" << vreg << " in memory operand! in func " << MFunc->getName() << "\n";
                            // reg_op->setPReg(PhysicalReg::T5);
                        }
                    }
                }
            }
        }
    }
}

// void getInstrUseDef(const MachineInstr* instr, std::set<unsigned>& use, std::set<unsigned>& def) {
//     auto opcode = instr->getOpcode();
//     const auto& operands = instr->getOperands();
    
//     auto get_vreg_id_if_virtual = [&](const MachineOperand* op, std::set<unsigned>& s) {
//         if (op->getKind() == MachineOperand::KIND_REG) {
//             auto reg_op = static_cast<const RegOperand*>(op);
//             if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
//         } else if (op->getKind() == MachineOperand::KIND_MEM) {
//             auto mem_op = static_cast<const MemOperand*>(op);
//             auto reg_op = mem_op->getBase();
//             if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
//         }
//     };

//     if (op_info.count(opcode)) {
//         const auto& info = op_info.at(opcode);
//         for (int idx : info.first) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), def);
//         for (int idx : info.second) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), use);
//         for (const auto& op : operands) if (op->getKind() == MachineOperand::KIND_MEM) get_vreg_id_if_virtual(op.get(), use);
//     } else if (opcode == RVOpcodes::CALL) {
//         if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[0].get(), def);
//         for (size_t i = 1; i < operands.size(); ++i) if (operands[i]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[i].get(), use);
//     }
// }

bool RISCv64LinearScan::isFPVReg(unsigned vreg) const {
    return vreg_type_map.count(vreg) && vreg_type_map.at(vreg)->isFloat();
}

void RISCv64LinearScan::collectUsedCalleeSavedRegs() {
    StackFrameInfo& frame_info = MFunc->getFrameInfo();
    frame_info.used_callee_saved_regs.clear();

    const auto& callee_saved_int = getCalleeSavedIntRegs();
    const auto& callee_saved_fp = getCalleeSavedFpRegs();
    std::set<PhysicalReg> callee_saved_set(callee_saved_int.begin(), callee_saved_int.end());
    callee_saved_set.insert(callee_saved_fp.begin(), callee_saved_fp.end());
    callee_saved_set.insert(PhysicalReg::S0);

    for(const auto& pair : vreg_to_preg_map) {
        PhysicalReg preg = pair.second;
        if(callee_saved_set.count(preg)) {
            frame_info.used_callee_saved_regs.insert(preg);
        }
    }
}

} // namespace sysy
