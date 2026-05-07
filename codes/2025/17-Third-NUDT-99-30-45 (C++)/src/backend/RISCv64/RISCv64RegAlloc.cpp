#include "RISCv64RegAlloc.h"
#include "RISCv64AsmPrinter.h"
#include "RISCv64Info.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cassert>
#include <chrono>
#include <thread>
#include <atomic>

namespace sysy {

// 构造函数：初始化寄存器池和数据结构
RISCv64RegAlloc::RISCv64RegAlloc(MachineFunction* mfunc)
    : MFunc(mfunc), 
      ISel(mfunc->getISel()),
      vreg_to_value_map(ISel->getVRegValueMap()),
      vreg_type_map(ISel->getVRegTypeMap()) {
    // 1. 初始化可分配的整数寄存器池
    allocable_int_regs = {
        PhysicalReg::T0, PhysicalReg::T1, PhysicalReg::T2, PhysicalReg::T3, PhysicalReg::T4, /* T5保留 */ PhysicalReg::T6,
        PhysicalReg::A0, PhysicalReg::A1, PhysicalReg::A2, PhysicalReg::A3, PhysicalReg::A4, PhysicalReg::A5, PhysicalReg::A6, PhysicalReg::A7,
        PhysicalReg::S1, PhysicalReg::S2, PhysicalReg::S3, PhysicalReg::S4, PhysicalReg::S5, PhysicalReg::S6, PhysicalReg::S7,
        PhysicalReg::S8, PhysicalReg::S9, PhysicalReg::S10, PhysicalReg::S11,
        // S0 是帧指针，不参与分配
    };
    K_int = allocable_int_regs.size();

    // 2. 初始化可分配的浮点寄存器池
    allocable_fp_regs = {
        PhysicalReg::F0, PhysicalReg::F1, PhysicalReg::F2, PhysicalReg::F3, PhysicalReg::F4, PhysicalReg::F5, PhysicalReg::F6, PhysicalReg::F7,
        PhysicalReg::F10, PhysicalReg::F11, PhysicalReg::F12, PhysicalReg::F13, PhysicalReg::F14, PhysicalReg::F15, PhysicalReg::F16, PhysicalReg::F17,
        PhysicalReg::F8, PhysicalReg::F9, PhysicalReg::F18, PhysicalReg::F19, PhysicalReg::F20, PhysicalReg::F21, PhysicalReg::F22,
        PhysicalReg::F23, PhysicalReg::F24, PhysicalReg::F25, PhysicalReg::F26, PhysicalReg::F27,
        PhysicalReg::F28, PhysicalReg::F29, PhysicalReg::F30, PhysicalReg::F31,
    };
    K_fp = allocable_fp_regs.size();

    // 3. 预着色所有物理寄存器
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    for (const auto& reg : allocable_int_regs) precolored.insert(offset + static_cast<unsigned>(reg));
    for (const auto& reg : allocable_fp_regs) precolored.insert(offset + static_cast<unsigned>(reg));
    precolored.insert(offset + static_cast<unsigned>(PhysicalReg::S0));
    precolored.insert(offset + static_cast<unsigned>(PhysicalReg::RA));
    precolored.insert(offset + static_cast<unsigned>(PhysicalReg::SP));
    precolored.insert(offset + static_cast<unsigned>(PhysicalReg::ZERO));
}

// 主入口: 迭代运行分配算法直到无溢出
bool RISCv64RegAlloc::run(std::shared_ptr<std::atomic<bool>> stop_flag) {
    if (DEBUG) std::cerr << "===== LLIR Before Running Graph Coloring Register Allocation " << MFunc->getName() << " =====\n";
    std::stringstream ss_before_reg_alloc;
    if (DEBUG) {
        RISCv64AsmPrinter printer_reg_alloc(MFunc);
        printer_reg_alloc.run(ss_before_reg_alloc, true);
        std::cout << ss_before_reg_alloc.str();
    }

    if (DEBUG) std::cerr << "===== Running Graph Coloring Register Allocation for function: " << MFunc->getName() << " =====\n";
    
    const int MAX_ITERATIONS = 50;
    int iteration = 0;
    
    while (iteration++ < MAX_ITERATIONS) {
        // std::cerr << "Iteration Step: " << iteration << "\n";
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        if (doAllocation()) {
            break;
        } else {
            rewriteProgram();
            if (stop_flag && stop_flag->load()) {
                // 如果从外部接收到停止信号
                std::cerr << "Info: IRC allocation cancelled due to timeout.\n";
                return false; // 提前退出，并返回失败
            }
            if (DEBUG) std::cerr << "--- Spilling detected, re-running allocation (iteration " << iteration << ") ---\n";
            
            if (iteration >= MAX_ITERATIONS) {
                return false;
            }
        }
    }

    applyColoring();
    
    MFunc->getFrameInfo().vreg_to_preg_map = this->color_map;
    collectUsedCalleeSavedRegs();
    if (DEBUG) std::cerr << "===== Finished Graph Coloring Register Allocation =====\n\n";
    return true;
}

// 单次分配的核心流程
bool RISCv64RegAlloc::doAllocation() {
    const int MAX_ITERATIONS = 50;
    int iteration = 0;
    initialize();
    precolorByCallingConvention();
    analyzeLiveness();
    build();
    protectCrossCallVRegs();
    makeWorklist();

    while (!simplifyWorklist.empty() || !worklistMoves.empty() || !freezeWorklist.empty() || !spillWorklist.empty()) {
        // if (DEBUG) std::cerr << "Inner Iteration Step: " << ++iteration << "\n";
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // if (DEEPDEBUG) dumpState("Loop Start");
        if (!simplifyWorklist.empty()) simplify();
        else if (!worklistMoves.empty()) coalesce();
        else if (!freezeWorklist.empty()) freeze();
        else if (!spillWorklist.empty()) selectSpill();
    }

    // if (DEEPDEBUG) dumpState("Before AssignColors");
    assignColors();
    return spilledNodes.empty();
}

void RISCv64RegAlloc::precolorByCallingConvention() {
    // 在处理前，先清空颜色相关的状态，确保重试时不会出错
    color_map.clear();
    coloredNodes.clear();

    Function* F = MFunc->getFunc();
    if (!F) return;

    // --- 部分1：处理函数传入参数的预着色 ---
    int int_arg_idx = 0;
    int float_arg_idx = 0;

    if (optLevel > 0)
    {
        for (const auto& pair : vreg_to_value_map) {
            unsigned vreg = pair.first;
            Value* val = pair.second;

            // 检查这个 Value* 是不是一个 Argument 对象
            if (auto arg = dynamic_cast<Argument*>(val)) {
                // 如果是，那么 vreg 就是最初分配给这个参数的 vreg
                int arg_idx = arg->getIndex();

                if (arg->getType()->isFloat()) {
                    if (arg_idx < 8) { // fa0-fa7
                        auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + arg_idx);
                        color_map[vreg] = preg;
                    }
                } else { // 整数或指针
                    if (arg_idx < 8) { // a0-a7
                        auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + arg_idx);
                        color_map[vreg] = preg;
                    }
                }
            }
        }
    } else {
        for (Argument* arg : F->getArguments()) {
            unsigned vreg = ISel->getVReg(arg);
            
            if (arg->getType()->isFloat()) {
                if (float_arg_idx < 8) { // fa0-fa7
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + float_arg_idx);
                    color_map[vreg] = preg;
                    float_arg_idx++;
                }
            } else { // 整数或指针
                if (int_arg_idx < 8) { // a0-a7
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + int_arg_idx);
                    color_map[vreg] = preg;
                    int_arg_idx++;
                }
            }
        }
    }

    // 将所有预着色的vreg视为已着色节点
    for(const auto& pair : color_map) {
        coloredNodes.insert(pair.first);
    }
    if (DEEPDEBUG) {
        std::cerr << "Precolored registers: { ";
        // 修改部分：将物理寄存器ID转换为其字符串名称
        for (unsigned v : precolored) std::cerr << regIdToString(v) << " ";
        std::cerr << "}\nColored nodes: { ";
        for (unsigned v : coloredNodes) std::cerr << "<%vreg" << v << ", " << regToString(color_map.at(v)) << "> ";
        std::cerr << "}\n";
    }
}

void RISCv64RegAlloc::protectCrossCallVRegs() {
    // 从ISel获取被标记为需要保护的参数副本vreg集合
    const auto& vregs_to_protect_potentially = MFunc->getProtectedArgumentVRegs();
    if (vregs_to_protect_potentially.empty()) {
        return; // 如果没有需要保护的vreg，直接返回
    }

    // VRegSet live_across_call_vregs;
    // // 遍历所有指令，找出哪些被标记的vreg其生命周期确实跨越了call指令
    // for (const auto& mbb_ptr : MFunc->getBlocks()) {
    //     for (const auto& instr_ptr : mbb_ptr->getInstructions()) {
    //         if (instr_ptr->getOpcode() == RVOpcodes::CALL) {
    //             const VRegSet& live_out_after_call = live_out_map.at(instr_ptr.get());
    //             for (unsigned vreg : vregs_to_protect_potentially) {
    //                 if (live_out_after_call.count(vreg)) {
    //                     live_across_call_vregs.insert(vreg);
    //                 }
    //             }
    //         }
    //     }
    // }

    // if (live_across_call_vregs.empty()) {
    //     return; // 如果被标记的vreg没有一个跨越call，也无需操作
    // }

    // if (DEEPDEBUG) {
    //     std::cerr << "--- [FIX] Applying protection for argument vregs that live across calls: ";
    //     for(unsigned v : live_across_call_vregs) std::cerr << regIdToString(v) << " ";
    //     std::cerr << "\n";
    // }

    // 获取所有调用者保存寄存器
    const auto& caller_saved_int = getCallerSavedIntRegs();
    const auto& caller_saved_fp = getCallerSavedFpRegs();
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);

    // 为每个确认跨越call的vreg，添加与所有调用者保存寄存器的冲突
    for (unsigned vreg : vregs_to_protect_potentially) {
        if (isFPVReg(vreg)) { // 如果是浮点vreg
            for (auto preg : caller_saved_fp) {
                addEdge(vreg, offset + static_cast<unsigned>(preg));
            }
        } else { // 如果是整数vreg
            for (auto preg : caller_saved_int) {
                addEdge(vreg, offset + static_cast<unsigned>(preg));
            }
        }
    }
}

// 初始化/重置所有数据结构
void RISCv64RegAlloc::initialize() {
    initial.clear();
    simplifyWorklist.clear();
    freezeWorklist.clear();
    spillWorklist.clear();
    spilledNodes.clear();
    coalescedNodes.clear();
    coloredNodes.clear();
    selectStack.clear();

    coalescedMoves.clear();
    constrainedMoves.clear();
    frozenMoves.clear();
    worklistMoves.clear();
    activeMoves.clear();

    adjList.clear();
    degree.clear();
    moveList.clear();
    alias.clear();
    color_map.clear();
}

// 活跃性分析（此部分为标准数据流分析，与现有版本类似但更精细）
void RISCv64RegAlloc::analyzeLiveness() {
    live_in_map.clear();
    live_out_map.clear();
    
    std::map<const MachineBasicBlock*, VRegSet> block_uses;
    std::map<const MachineBasicBlock*, VRegSet> block_defs;

    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        const MachineBasicBlock* mbb = mbb_ptr.get();
        VRegSet uses, defs;
        for (const auto& instr_ptr : mbb->getInstructions()) {
            VRegSet instr_use, instr_def;
            getInstrUseDef_Liveness(instr_ptr.get(), instr_use, instr_def);
            for (unsigned u : instr_use) {
                if (defs.find(u) == defs.end()) uses.insert(u);
            }
            defs.insert(instr_def.begin(), instr_def.end());
        }
        block_uses[mbb] = uses;
        block_defs[mbb] = defs;
    }

    bool changed = true;
    std::map<const MachineBasicBlock*, VRegSet> live_in, live_out;
    while (changed) {
        changed = false;
        for (auto it = MFunc->getBlocks().rbegin(); it != MFunc->getBlocks().rend(); ++it) {
            const auto& mbb_ptr = *it;
            const MachineBasicBlock* mbb = mbb_ptr.get();
            VRegSet new_out;
            for (auto succ : mbb->successors) {
                new_out.insert(live_in[succ].begin(), live_in[succ].end());
            }
            VRegSet new_in = block_uses[mbb];
            VRegSet out_minus_def = new_out;
            for (unsigned d : block_defs[mbb]) out_minus_def.erase(d);
            new_in.insert(out_minus_def.begin(), out_minus_def.end());
            if (live_out[mbb] != new_out || live_in[mbb] != new_in) {
                changed = true;
                live_out[mbb] = new_out;
                live_in[mbb] = new_in;
            }
        }
    }

    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        const MachineBasicBlock* mbb = mbb_ptr.get();
        VRegSet current_live = live_out[mbb];
        for (auto instr_it = mbb->getInstructions().rbegin(); instr_it != mbb->getInstructions().rend(); ++instr_it) {
            const MachineInstr* instr = instr_it->get();
            live_out_map[instr] = current_live;
            VRegSet use, def;

            getInstrUseDef_Liveness(instr, use, def);
            for(auto d : def) current_live.erase(d);
            for(auto u : use) current_live.insert(u);
            live_in_map[instr] = current_live;
        }
    }
}

void RISCv64RegAlloc::build() {
    initial.clear(); 
    RISCv64AsmPrinter printer_inside_build(MFunc);
    printer_inside_build.setStream(std::cerr);

    // 1. 收集所有待分配的（既非物理也非预着色）虚拟寄存器到 initial 集合
    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        for (const auto& instr_ptr : mbb_ptr->getInstructions()) {
            const MachineInstr* instr = instr_ptr.get();
            VRegSet use, def;
            getInstrUseDef_Liveness(instr, use, def);

            // 调试输出 use 和 def (保留您的调试逻辑)
            if (DEEPERDEBUG) {
                std::cerr << "Instr:";
                printer_inside_build.printInstruction(instr_ptr.get(), true);
                auto print_set = [this](const VRegSet& s, const std::string& name) {
                    std::cerr << "    " << name << ": { ";
                    for(unsigned v : s) std::cerr << regIdToString(v) << " ";
                    std::cerr << "}\n";
                };
                print_set(def, "Def     ");
                print_set(use, "Use     ");
            }

            for (unsigned v : use) {
                if (!coloredNodes.count(v) && !precolored.count(v)) {
                    initial.insert(v);
                } else if ((DEEPDEBUG && initial.size() < DEBUGLENGTH) || DEEPERDEBUG) {
                    // 这里的调试信息可以更精确
                    if (precolored.count(v)) {
                        std::cerr << "Skipping " << regIdToString(v) << " because it is a physical register.\n";
                    } else {
                        std::cerr << "Skipping " << regIdToString(v) << " because it is a pre-colored virtual register with " << regToString(color_map.at(v)) << "\n";
                    }
                }
            }
            for (unsigned v : def) {
                if (!coloredNodes.count(v) && !precolored.count(v)) {
                    initial.insert(v);
                } else if ((DEEPDEBUG && initial.size() < DEBUGLENGTH) || DEEPERDEBUG) {
                    if (precolored.count(v)) {
                        std::cerr << "Skipping " << regIdToString(v) << " because it is a physical register.\n";
                    } else {
                        std::cerr << "Skipping " << regIdToString(v) << " because it is a pre-colored virtual register with " << regToString(color_map.at(v)) << ".\n";
                    }
                }
            }
        }
    }

    if (DEEPDEBUG) {
        if (initial.size() > DEBUGLENGTH && !DEEPERDEBUG) {
            std::cerr << "Initial set too large, showing first " << DEBUGLENGTH << " elements:\n";
        }
        std::cerr << "Initial set (" << initial.size() << "): { ";
        unsigned count = 0;
        for (unsigned v : initial) {
            if (count++ >= DEBUGLENGTH && !DEEPERDEBUG) break; // 限制输出数量
            std::cerr << regIdToString(v) << " ";
        }
        if (count < initial.size()) {
            std::cerr << "... (total " << initial.size() << " elements)\n";
        } else {
            std::cerr << "}\n";
        }
    }
    
    // 2. 为所有参与图构建的虚拟寄存器（initial + coloredNodes）初始化数据结构
    VRegSet all_participating_vregs = initial;
    all_participating_vregs.insert(coloredNodes.begin(), coloredNodes.end());

    for (unsigned vreg : all_participating_vregs) {
        // 物理寄存器ID不应作为key，此检查是安全的双重保障
        const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
        if (vreg >= offset) {
            continue;
        }
        adjList[vreg] = {};
        degree[vreg] = 0;
    }

    // 3. 构建冲突图
    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        if (DEEPDEBUG) std::cerr << "\n--- Building Graph for Basic Block: " << mbb_ptr->getName() << " ---\n";
        for (const auto& instr_ptr : mbb_ptr->getInstructions()) {
            const MachineInstr* instr = instr_ptr.get();
            VRegSet use, def;
            getInstrUseDef_Liveness(instr, use, def);
            const VRegSet& live_out = live_out_map.at(instr);

            // 保留您的指令级调试输出
            if (DEEPERDEBUG) {
                RISCv64AsmPrinter temp_printer(MFunc);
                temp_printer.setStream(std::cerr);
                std::cerr << "Instr: ";
                temp_printer.printInstruction(const_cast<MachineInstr*>(instr), true);

                auto print_set = [this](const VRegSet& s, const std::string& name) {
                    std::cerr << "    " << name << ": { ";
                    for(unsigned v : s) std::cerr << regIdToString(v) << " ";
                    std::cerr << "}\n";
                };
                print_set(def, "Def     ");
                print_set(use, "Use     ");
                print_set(live_out, "Live_Out");
                std::cerr << "  ----------------\n";
            }
            
            bool is_move = instr->getOpcode() == RVOpcodes::MV;

            // 保留您处理 moveList 的逻辑
            if (is_move) {
                worklistMoves.insert(instr);
                VRegSet move_vregs;
                for(const auto& op : instr->getOperands()) {
                    if (op->getKind() == MachineOperand::KIND_REG) {
                        auto reg_op = static_cast<const RegOperand*>(op.get());
                        if(reg_op->isVirtual()) move_vregs.insert(reg_op->getVRegNum());
                    }
                }
                for (unsigned vreg : move_vregs) {
                    // 使用 operator[] 是安全的，如果vreg不存在会默认构造
                    moveList[vreg].insert(instr);
                }
            }

            VRegSet live = live_out;
            if (is_move) {
                for (unsigned u_op : use) {
                    live.erase(u_op);
                }
            }
            
            // --- 规则 1 & 2: Def 与 Live/Use 变量干扰 ---
            for (unsigned d : def) {
                // 新逻辑：对于指令定义的所有寄存器d（无论是虚拟寄存器还是像call指令那样
                // 隐式定义的物理寄存器），它都与该指令之后的所有活跃寄存器l冲突。
                // addEdge函数内部会正确处理 vreg-vreg 和 vreg-preg 的情况，
                // 并忽略 preg-preg 的情况。
                for (unsigned l : live) {
                    addEdge(d, l);
                }
                
                // 对于非传送指令, Def还和Use冲突。
                // 这个逻辑主要用于确保在同一条指令内，例如 sub t0, t1, t0,
                // 作为def的t0和作为use的t0被视为冲突。
                if (!is_move) {
                    for (unsigned u_op : use) {
                        addEdge(d, u_op);
                    }
                }
            }
            
            // --- 规则 3: Live_Out 集合内部的【虚拟寄存器】形成完全图 ---
            // 使用更高效的遍历，避免重复调用 addEdge(A,B) 和 addEdge(B,A)
            // 添加限制以防止过度密集的图
            const size_t MAX_LIVE_OUT_SIZE = 32; // 限制最大活跃变量数
            if (live_out.size() > MAX_LIVE_OUT_SIZE) {
                // 对于大量活跃变量，使用更保守的边添加策略
                // 只添加必要的边，而不是完全图
                for (auto it1 = live_out.begin(); it1 != live_out.end(); ++it1) {
                    unsigned l1 = *it1;
                    if (precolored.count(l1)) continue;
                    
                    // 只添加与定义变量相关的边
                    for (unsigned d : def) {
                        if (d != l1 && !precolored.count(d)) {
                            addEdge(l1, d);
                        }
                    }
                }
            } else {
                // 对于较小的集合，使用原来的完全图方法
                for (auto it1 = live_out.begin(); it1 != live_out.end(); ++it1) {
                    unsigned l1 = *it1;
                    if (precolored.count(l1)) continue;

                    for (auto it2 = std::next(it1); it2 != live_out.end(); ++it2) {
                        unsigned l2 = *it2;
                        addEdge(l1, l2);
                    }
                }
            }
        }
    }
}

// 将节点放入初始工作列表
void RISCv64RegAlloc::makeWorklist() {
    for (unsigned n : initial) {
        int K = isFPVReg(n) ? K_fp : K_int;
        if (degree.count(n) == 0) {
            std::cerr << "Error: degree not initialized for %vreg" << n << "\n";
            continue;
        }
        if ((DEEPDEBUG && initial.size() < DEBUGLENGTH) || DEEPERDEBUG) {
            std::cerr << "Assigning %vreg" << n << " (degree=" << degree.at(n)
                      << ", moveRelated=" << moveRelated(n) << ")\n";
        }
        if (degree.at(n) >= K) {
            spillWorklist.insert(n);
        } else if (moveRelated(n)) {
            freezeWorklist.insert(n);
        } else {
            simplifyWorklist.insert(n);
        }
    }
    if (DEEPDEBUG || DEEPERDEBUG) std::cerr << "--------------------------------\n";
    initial.clear();
}

// 简化阶段
void RISCv64RegAlloc::simplify() {
    unsigned n = *simplifyWorklist.begin();
    simplifyWorklist.erase(simplifyWorklist.begin());
    if (DEEPERDEBUG) std::cerr << "[Simplify] Popped %vreg" << n << ", pushing to stack.\n";
    selectStack.push_back(n);
    for (unsigned m : adjacent(n)) {
        decrementDegree(m);
    }
}

// 合并阶段
void RISCv64RegAlloc::coalesce() {
    const MachineInstr* move = *worklistMoves.begin();
    worklistMoves.erase(worklistMoves.begin());
    VRegSet use, def;
    getInstrUseDef_Liveness(move, use, def);
    unsigned x = getAlias(*def.begin());
    unsigned y = getAlias(*use.begin());
    unsigned u, v;

    // 总是将待合并的虚拟寄存器赋给 v，将合并目标赋给 u。
    // 优先级: 物理寄存器 (precolored) > 已着色的虚拟寄存器 (coloredNodes) > 普通虚拟寄存器。
    if (precolored.count(y)) {
        u = y;
        v = x;
    } else if (precolored.count(x)) {
        u = x;
        v = y;
    } else if (coloredNodes.count(y)) {
        u = y;
        v = x;
    } else {
        u = x;
        v = y;
    }
    
    // 防御性检查，处理物理寄存器之间的传送指令
    if (precolored.count(u) && precolored.count(v)) {
        constrainedMoves.insert(move);
        return;
    }

    if (DEEPERDEBUG) std::cerr << "[Coalesce] Processing move between " << regIdToString(x) 
                           << " and " << regIdToString(y) << " (aliases " << regIdToString(u) 
                           << ", " << regIdToString(v) << ").\n";

    if (u == v) {
        if (DEEPERDEBUG) std::cerr << "  -> Trivial coalesce (u == v).\n";
        coalescedMoves.insert(move);
        addWorklist(u);
        return;
    }
    
    bool is_conflicting = false;
    // 检查1：u 和 v 在冲突图中是否直接相连
    if ((adjList.count(v) && adjList.at(v).count(u)) || (adjList.count(u) && adjList.at(u).count(v))) {
        if (DEEPERDEBUG) std::cerr << "  -> [Check] Nodes interfere directly.\n";
        is_conflicting = true;
    } 
    // 检查2：如果节点不直接相连，则检查是否存在间接的颜色冲突
    else {
        // 获取 u 和 v 的颜色（如果它们有的话）
        unsigned u_color_id = 0, v_color_id = 0;
        if (precolored.count(u)) {
            u_color_id = u;
        } else if (coloredNodes.count(u) || color_map.count(u)) { // color_map.count(u) 是更可靠的检查
            u_color_id = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID) + static_cast<unsigned>(color_map.at(u));
        }

        if (precolored.count(v)) {
            v_color_id = v;
        } else if (coloredNodes.count(v) || color_map.count(v)) {
            v_color_id = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID) + static_cast<unsigned>(color_map.at(v));
        }

        // 如果 u 有颜色，检查 v 是否与该颜色代表的物理寄存器冲突
        if (u_color_id != 0 && adjList.count(v) && adjList.at(v).count(u_color_id)) {
            if (DEEPERDEBUG) std::cerr << "  -> [Check] Node " << regIdToString(v) << " interferes with the color of " << regIdToString(u) << " (" << regIdToString(u_color_id) << ").\n";
            is_conflicting = true;
        }
        // 如果 v 有颜色，检查 u 是否与该颜色代表的物理寄存器冲突
        else if (v_color_id != 0 && adjList.count(u) && adjList.at(u).count(v_color_id)) {
            if (DEEPERDEBUG) std::cerr << "  -> [Check] Node " << regIdToString(u) << " interferes with the color of " << regIdToString(v) << " (" << regIdToString(v_color_id) << ").\n";
            is_conflicting = true;
        }
    }

    if (is_conflicting) {
        if (DEEPERDEBUG) std::cerr << "  -> Constrained (nodes interfere directly or via pre-coloring).\n";
        constrainedMoves.insert(move);
        addWorklist(u);
        addWorklist(v);
        return;
    }

    bool u_is_colored = precolored.count(u) || coloredNodes.count(u);
    bool v_is_colored = precolored.count(v) || coloredNodes.count(v);

    if (u_is_colored && v_is_colored) {
        PhysicalReg u_color = precolored.count(u) 
            ? static_cast<PhysicalReg>(u - static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID)) 
            : color_map.at(u);
        PhysicalReg v_color = precolored.count(v) 
            ? static_cast<PhysicalReg>(v - static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID)) 
            : color_map.at(v);
        
        if (u_color != v_color) {
            if (DEEPERDEBUG) std::cerr << "  -> Constrained (move between two different precolored nodes: " 
                                   << regToString(u_color) << " and " << regToString(v_color) << ").\n";
            constrainedMoves.insert(move);
            return;
        } else {
            if (DEEPERDEBUG) std::cerr << "  -> Trivial coalesce (move between same precolored nodes).\n";
            coalescedMoves.insert(move);
            combine(u, v);
            addWorklist(u);
            return;
        }
    }

    // 类型检查 
    if (isFPVReg(u) != isFPVReg(v)) {
        if (DEEPERDEBUG) std::cerr << "  -> Constrained (type mismatch: " << regIdToString(u) << " is " 
                                << (isFPVReg(u) ? "float" : "int") << ", " << regIdToString(v) << " is "
                                << (isFPVReg(v) ? "float" : "int") << ").\n";
        constrainedMoves.insert(move);
        addWorklist(u);
        addWorklist(v);
        return;
    }
    
    // 启发式判断逻辑 
    bool u_is_effectively_precolored = precolored.count(u) || coloredNodes.count(u);
    bool can_coalesce = false;
    
    if (u_is_effectively_precolored) {
        if (DEEPERDEBUG) std::cerr << "  -> Trying George Heuristic (u is effectively precolored)...\n";
        
        VRegSet neighbors_of_v = adjacent(v);
        if (DEEPERDEBUG) {
            std::cerr << "      - Neighbors of " << regIdToString(v) << " to check are (" << neighbors_of_v.size() << "): { ";
            for (unsigned id : neighbors_of_v) std::cerr << regIdToString(id) << " ";
            std::cerr << "}\n";
        }
        
        bool george_ok = true;
        for (unsigned t : neighbors_of_v) {
            if (DEEPERDEBUG) std::cerr << "      - Checking neighbor " << regIdToString(t) << ":\n";

            unsigned u_phys_id = precolored.count(u) ? u : (static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID) + static_cast<unsigned>(color_map.at(u)));
            bool heuristic_result = georgeHeuristic(t, u_phys_id);
            
            if (DEEPERDEBUG) {
                std::cerr << "          - georgeHeuristic(" << regIdToString(t) << ", " << regIdToString(u_phys_id) << ") -> " << (heuristic_result ? "OK" : "FAIL") << "\n";
            }

            if (!heuristic_result) {
                george_ok = false;
                break;
            }
        }
        
        if (DEEPERDEBUG) std::cerr << "  -> George Heuristic final result: " << (george_ok ? "OK" : "FAIL") << "\n";
        if (george_ok) can_coalesce = true;

    } else {
        // --- 场景2：u和v都是未着色的虚拟寄存器，使用 Briggs 启发式 ---
        if (DEEPERDEBUG) std::cerr << "  -> Trying Briggs Heuristic (u and v are virtual)...\n";
        
        bool briggs_ok = briggsHeuristic(u, v);
        if (DEEPERDEBUG) std::cerr << "      - briggsHeuristic(" << regIdToString(u) << ", " << regIdToString(v) << ") -> " << (briggs_ok ? "OK" : "FAIL") << "\n";
        if (briggs_ok) can_coalesce = true;
    }

    if (can_coalesce) {
        if (DEEPERDEBUG) std::cerr << "  -> Heuristic OK. Combining " << regIdToString(v) << " into " << regIdToString(u) << ".\n";
        coalescedMoves.insert(move);
        combine(u, v);
        addWorklist(u);
    } else {
        if (DEEPERDEBUG) std::cerr << "  -> Heuristic failed. Adding to active moves.\n";
        activeMoves.insert(move);
    }
}

// 冻结阶段
void RISCv64RegAlloc::freeze() {
    unsigned u = *freezeWorklist.begin();
    freezeWorklist.erase(freezeWorklist.begin());
    if (DEEPERDEBUG) std::cerr << "[Freeze] Freezing %vreg" << u << " and moving to simplify list.\n";
    simplifyWorklist.insert(u);
    freezeMoves(u);
}

// 选择溢出节点
void RISCv64RegAlloc::selectSpill() {
    auto it = std::max_element(spillWorklist.begin(), spillWorklist.end(), 
        [&](unsigned a, unsigned b){ return degree.at(a) < degree.at(b); });
    unsigned m = *it;
    spillWorklist.erase(it);
    if (DEEPERDEBUG) std::cerr << "[Spill] Selecting %vreg" << m << " to spill.\n";
    simplifyWorklist.insert(m);
    freezeMoves(m);
}

void RISCv64RegAlloc::assignColors() {
    if (DEEPERDEBUG) std::cerr << "[AssignColors] Starting...\n";
    // 步骤 1: 为 selectStack 中的节点分配颜色 (此部分逻辑不变)
    while (!selectStack.empty()) {
        unsigned n = selectStack.back();
        selectStack.pop_back();
        bool is_fp = isFPVReg(n);
        const auto& available_regs = is_fp ? allocable_fp_regs : allocable_int_regs;
        std::set<PhysicalReg> ok_colors(available_regs.begin(), available_regs.end());

        if (adjList.count(n)) {
            for (unsigned w : adjList.at(n)) {
                unsigned w_alias = getAlias(w);
                
                if (coloredNodes.count(w_alias)) { // 邻居是已着色的vreg
                    ok_colors.erase(color_map.at(w_alias));
                } else if (precolored.count(w_alias)) { // 邻居是物理寄存器
                    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
                    ok_colors.erase(static_cast<PhysicalReg>(w_alias - offset));
                }
            }
        }

        if (ok_colors.empty()) {
            spilledNodes.insert(n);
            if (DEEPERDEBUG) std::cerr << "  -> WARNING: No color for %vreg" << n << " from selectStack. Spilling.\n";
        } else {
            PhysicalReg c = *ok_colors.begin();
            coloredNodes.insert(n);
            color_map[n] = c;
            if (DEEPERDEBUG) std::cerr << "  -> Colored %vreg" << n << " with " << regToString(c) << ".\n";
        }
    }

    // 步骤 2: 处理 coalescedNodes
    for (unsigned n : coalescedNodes) {
        unsigned root_alias = getAlias(n);
        
        // --- 处理所有三种可能性 ---

        // 情况 1: 别名本身就是物理寄存器 (修复当前bug)
        if (precolored.count(root_alias)) {
            const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
            color_map[n] = static_cast<PhysicalReg>(root_alias - offset);
            if (DEEPERDEBUG) std::cerr << "  -> Coalesced %vreg" << n << " gets color from PHYSICAL alias " << regIdToString(root_alias) << ".\n";
        }
        // 情况 2: 别名是被成功着色的虚拟寄存器
        else if (color_map.count(root_alias)) {
            color_map[n] = color_map.at(root_alias);
            if (DEEPERDEBUG) std::cerr << "  -> Coalesced %vreg" << n << " gets color from VIRTUAL alias " << regIdToString(root_alias) << ".\n";
        }
        // 情况 3: 别名是被溢出的虚拟寄存器
        else {
            spilledNodes.insert(n);
            if (DEEPERDEBUG) std::cerr << "  -> Alias " << regIdToString(root_alias) << " of %vreg" << n << " was SPILLED. Spilling %vreg" << n << " as well.\n";
        }
    }
}

// 重写程序，插入溢出代码
void RISCv64RegAlloc::rewriteProgram() {
    StackFrameInfo& frame_info = MFunc->getFrameInfo();
    // 使用 EFI Pass 确定的 locals_end_offset 作为溢出分配的基准。
    // locals_end_offset 本身是负数，代表局部变量区域的下边界地址。
    int spill_current_offset = frame_info.locals_end_offset - frame_info.spill_size;

    // 保存溢出区域的起始点，用于最后计算总的 spill_size
    const int spill_start_offset = frame_info.locals_end_offset;

    for (unsigned vreg : spilledNodes) {
        if (frame_info.spill_offsets.count(vreg)) continue;

        int size = 4;
        if (isFPVReg(vreg)) {
            size = 4; // float
        } else if (vreg_type_map.count(vreg) && vreg_type_map.at(vreg)->isPointer()) {
            size = 8; // pointer
        }

        // 在当前偏移基础上继续向下(地址变得更负)分配空间
        spill_current_offset -= size;
        
        // 对齐新的、更小的地址，RISC-V 要求8字节对齐
        spill_current_offset = spill_current_offset & ~7;

        // 将计算出的、不会冲突的正确偏移量存入 spill_offsets
        frame_info.spill_offsets[vreg] = spill_current_offset;
    }

    // 更新总的溢出区域大小。
    // spill_size = -(结束偏移 - 开始偏移)
    frame_info.spill_size = -(spill_current_offset - spill_start_offset);

    // 2. 遍历所有指令，重写代码
    for (auto& mbb : MFunc->getBlocks()) {
        std::vector<std::unique_ptr<MachineInstr>> new_instructions;
        
        for (auto& instr_ptr : mbb->getInstructions()) {
            std::map<unsigned, unsigned> use_remap;
            std::map<unsigned, unsigned> def_remap;

            VRegSet use, def;
            getInstrUseDef_Liveness(instr_ptr.get(), use, def);

            // a. 为每个溢出的 use 操作数创建新vreg并插入 load
            for (unsigned old_vreg : use) {
                if (spilledNodes.count(old_vreg)) {
                    Type* type = vreg_type_map.count(old_vreg) ? vreg_type_map.at(old_vreg) : Type::getIntType();
                    unsigned new_temp_vreg = ISel->getNewVReg(type);
                    use_remap[old_vreg] = new_temp_vreg;

                    RVOpcodes load_op;
                    if (isFPVReg(old_vreg)) load_op = RVOpcodes::FLW;
                    else if (type->isPointer()) load_op = RVOpcodes::LD;
                    else load_op = RVOpcodes::LW;

                    auto load = std::make_unique<MachineInstr>(load_op);
                    load->addOperand(std::make_unique<RegOperand>(new_temp_vreg));
                    load->addOperand(std::make_unique<MemOperand>(
                        std::make_unique<RegOperand>(PhysicalReg::S0),
                        std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))
                    ));
                    new_instructions.push_back(std::move(load));
                }
            }

            // b. 为每个溢出的 def 操作数创建新vreg
            for (unsigned old_vreg : def) {
                if (spilledNodes.count(old_vreg)) {
                    Type* type = vreg_type_map.count(old_vreg) ? vreg_type_map.at(old_vreg) : Type::getIntType();
                    unsigned new_temp_vreg = ISel->getNewVReg(type);
                    def_remap[old_vreg] = new_temp_vreg;
                }
            }

            // c. 创建一条全新的指令，用新vreg替换旧vreg
            auto new_instr = std::make_unique<MachineInstr>(instr_ptr->getOpcode());
            for (const auto& op : instr_ptr->getOperands()) {
                if (op->getKind() == MachineOperand::KIND_REG) {
                    auto reg_op = static_cast<RegOperand*>(op.get());
                    if (reg_op->isVirtual()) {
                        unsigned old_vreg = reg_op->getVRegNum();
                        if (use.count(old_vreg) && use_remap.count(old_vreg)) {
                            new_instr->addOperand(std::make_unique<RegOperand>(use_remap.at(old_vreg)));
                        } else if (def.count(old_vreg) && def_remap.count(old_vreg)) {
                            new_instr->addOperand(std::make_unique<RegOperand>(def_remap.at(old_vreg)));
                        } else {
                            new_instr->addOperand(std::make_unique<RegOperand>(old_vreg));
                        }
                    } else {
                        new_instr->addOperand(std::make_unique<RegOperand>(reg_op->getPReg()));
                    }
                } else if (op->getKind() == MachineOperand::KIND_MEM) {
                    auto mem_op = static_cast<MemOperand*>(op.get());
                    auto base_reg = mem_op->getBase();
                    unsigned old_vreg = base_reg->isVirtual() ? base_reg->getVRegNum() : -1;
                    
                    if (base_reg->isVirtual() && use_remap.count(old_vreg)) {
                        new_instr->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(use_remap.at(old_vreg)),
                            std::make_unique<ImmOperand>(mem_op->getOffset()->getValue())
                        ));
                    } else {
                         new_instr->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(*base_reg),
                            std::make_unique<ImmOperand>(mem_op->getOffset()->getValue())
                        ));
                    }
                } else { // 立即数、标签等直接复制
                    if(op->getKind() == MachineOperand::KIND_IMM)
                        new_instr->addOperand(std::make_unique<ImmOperand>(*static_cast<ImmOperand*>(op.get())));
                    else if (op->getKind() == MachineOperand::KIND_LABEL)
                        new_instr->addOperand(std::make_unique<LabelOperand>(*static_cast<LabelOperand*>(op.get())));
                }
            }
            new_instructions.push_back(std::move(new_instr));

            // d. 为每个溢出的 def 操作数，在原指令后插入 store 指令
            for (const auto& pair : def_remap) {
                unsigned old_vreg = pair.first;
                unsigned new_temp_vreg = pair.second;
                Type* type = vreg_type_map.count(old_vreg) ? vreg_type_map.at(old_vreg) : Type::getIntType();
                
                RVOpcodes store_op;
                if (isFPVReg(old_vreg)) store_op = RVOpcodes::FSW;
                else if (type->isPointer()) store_op = RVOpcodes::SD;
                else store_op = RVOpcodes::SW;

                auto store = std::make_unique<MachineInstr>(store_op);
                store->addOperand(std::make_unique<RegOperand>(new_temp_vreg));
                store->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(PhysicalReg::S0),
                    std::make_unique<ImmOperand>(frame_info.spill_offsets.at(old_vreg))
                ));
                new_instructions.push_back(std::move(store));
                if (DEEPERDEBUG) {
                    std::cerr << "[Spill] Inserted spill store for %vreg" << old_vreg 
                              << " at offset " << frame_info.spill_offsets.at(old_vreg) 
                              << " with new temp vreg " << regIdToString(new_temp_vreg) << ".\n";
                }
            }
        }
        mbb->getInstructions() = std::move(new_instructions);
    }
    
    // 清空溢出节点集合，为下一次迭代分配做准备
    spilledNodes.clear();
}

/**
 * @brief 获取一条指令完整的【虚拟】使用/定义寄存器集合
 * 这个函数将服务于图的构建（收集initial节点等）。
 */
void RISCv64RegAlloc::getInstrUseDef(const MachineInstr* instr, VRegSet& use, VRegSet& def) {
    auto opcode = instr->getOpcode();
    const auto& operands = instr->getOperands();

    static const std::map<RVOpcodes, std::pair<std::vector<int>, std::vector<int>>> op_info = {
        {RVOpcodes::ADD, {{0}, {1, 2}}}, {RVOpcodes::SUB, {{0}, {1, 2}}}, {RVOpcodes::MUL, {{0}, {1, 2}}},
        {RVOpcodes::DIV, {{0}, {1, 2}}}, {RVOpcodes::REM, {{0}, {1, 2}}}, {RVOpcodes::ADDW, {{0}, {1, 2}}},
        {RVOpcodes::SUBW, {{0}, {1, 2}}}, {RVOpcodes::MULW, {{0}, {1, 2}}}, {RVOpcodes::DIVW, {{0}, {1, 2}}},
        {RVOpcodes::REMW, {{0}, {1, 2}}}, {RVOpcodes::SLT, {{0}, {1, 2}}}, {RVOpcodes::SLTU, {{0}, {1, 2}}},
        {RVOpcodes::ADDI, {{0}, {1}}}, {RVOpcodes::ADDIW, {{0}, {1}}}, {RVOpcodes::XORI, {{0}, {1}}},
        {RVOpcodes::SLTI, {{0}, {1}}}, {RVOpcodes::SLTIU, {{0}, {1}}}, {RVOpcodes::LB, {{0}, {}}},
        {RVOpcodes::LH, {{0}, {}}}, {RVOpcodes::LW, {{0}, {}}}, {RVOpcodes::LD, {{0}, {}}},
        {RVOpcodes::LBU, {{0}, {}}}, {RVOpcodes::LHU, {{0}, {}}}, {RVOpcodes::LWU, {{0}, {}}},
        {RVOpcodes::FLW, {{0}, {}}}, {RVOpcodes::FLD, {{0}, {}}}, {RVOpcodes::SB, {{}, {0, 1}}},
        {RVOpcodes::SH, {{}, {0, 1}}}, {RVOpcodes::SW, {{}, {0, 1}}}, {RVOpcodes::SD, {{}, {0, 1}}},
        {RVOpcodes::FSW, {{}, {0, 1}}}, {RVOpcodes::FSD, {{}, {0, 1}}}, {RVOpcodes::BEQ, {{}, {0, 1}}},
        {RVOpcodes::BNE, {{}, {0, 1}}}, {RVOpcodes::BLT, {{}, {0, 1}}}, {RVOpcodes::BGE, {{}, {0, 1}}},
        {RVOpcodes::JALR, {{0}, {1}}}, {RVOpcodes::LI, {{0}, {}}}, {RVOpcodes::LA, {{0}, {}}},
        {RVOpcodes::MV, {{0}, {1}}}, {RVOpcodes::SEQZ, {{0}, {1}}}, {RVOpcodes::SNEZ, {{0}, {1}}},
        {RVOpcodes::RET, {{}, {}}}, {RVOpcodes::FADD_S, {{0}, {1, 2}}}, {RVOpcodes::FSUB_S, {{0}, {1, 2}}},
        {RVOpcodes::FMUL_S, {{0}, {1, 2}}}, {RVOpcodes::FDIV_S, {{0}, {1, 2}}}, {RVOpcodes::FEQ_S, {{0}, {1, 2}}},
        {RVOpcodes::FLT_S, {{0}, {1, 2}}}, {RVOpcodes::FLE_S, {{0}, {1, 2}}}, {RVOpcodes::FCVT_S_W, {{0}, {1}}},
        {RVOpcodes::FCVT_W_S, {{0}, {1}}}, {RVOpcodes::FMV_S, {{0}, {1}}}, {RVOpcodes::FMV_W_X, {{0}, {1}}},
        {RVOpcodes::FMV_X_W, {{0}, {1}}}, {RVOpcodes::FNEG_S, {{0}, {1}}}
    };
    
    auto get_vreg_id_if_virtual = [&](const MachineOperand* op, VRegSet& s) {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<const RegOperand*>(op);
            if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            auto mem_op = static_cast<const MemOperand*>(op);
            auto reg_op = mem_op->getBase();
            if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
        }
    };

    if (op_info.count(opcode)) {
        const auto& info = op_info.at(opcode);
        for (int idx : info.first) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), def);
        for (int idx : info.second) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), use);
        for (const auto& op : operands) if (op->getKind() == MachineOperand::KIND_MEM) get_vreg_id_if_virtual(op.get(), use);
    } else if (opcode == RVOpcodes::CALL) {
        if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[0].get(), def);
        for (size_t i = 1; i < operands.size(); ++i) if (operands[i]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[i].get(), use);
    }
}

/**
 * @brief 获取一条指令完整的、包含物理寄存器的Use/Def集合
 * 这个函数专门服务于活跃性分析，现已补全所有指令（包括伪指令）的逻辑。
 */
void RISCv64RegAlloc::getInstrUseDef_Liveness(const MachineInstr* instr, VRegSet& use, VRegSet& def) {
    auto opcode = instr->getOpcode();
    const auto& operands = instr->getOperands();

    // lambda表达式用于获取操作数的寄存器ID（虚拟或物理）
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    auto get_any_reg_id = [&](const MachineOperand* op) -> unsigned {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<const RegOperand*>(op);
            return reg_op->isVirtual() ? reg_op->getVRegNum() : (offset + static_cast<unsigned>(reg_op->getPReg()));
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            auto mem_op = static_cast<const MemOperand*>(op);
            auto reg_op = mem_op->getBase();
            return reg_op->isVirtual() ? reg_op->getVRegNum() : (offset + static_cast<unsigned>(reg_op->getPReg()));
        }
        return (unsigned)-1;
    };

    // --- 主要处理逻辑 ---
    if (op_info.count(opcode)) {
        const auto& info = op_info.at(opcode);
        for (int idx : info.first) if (idx < operands.size()) {
            unsigned reg_id = get_any_reg_id(operands[idx].get());
            if (reg_id != (unsigned)-1) def.insert(reg_id);
        }
        for (int idx : info.second) if (idx < operands.size()) {
            unsigned reg_id = get_any_reg_id(operands[idx].get());
            if (reg_id != (unsigned)-1) use.insert(reg_id);
        }
        // 对于所有内存操作，基址寄存器都必须是 use
        for (const auto& op : operands) {
            if (op->getKind() == MachineOperand::KIND_MEM) {
                 unsigned reg_id = get_any_reg_id(op.get());
                 if (reg_id != (unsigned)-1) use.insert(reg_id);
            }
        }
    } 
    // --- 特殊指令处理逻辑 ---
    else if (opcode == RVOpcodes::CALL) {
        // 返回值是Def
        if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) {
            def.insert(get_any_reg_id(operands[0].get()));
        }
        // 函数名后的所有寄存器参数都是Use
        for (size_t i = 1; i < operands.size(); ++i) {
            if (operands[i]->getKind() == MachineOperand::KIND_REG) {
                use.insert(get_any_reg_id(operands[i].get()));
            }
        }
        // 所有调用者保存寄存器(caller-saved)被隐式定义（因为它们的值被破坏了）
        for (auto preg : getCallerSavedIntRegs()) def.insert(offset + static_cast<unsigned>(preg));
        for (auto preg : getCallerSavedFpRegs()) def.insert(offset + static_cast<unsigned>(preg));
        // 返回地址寄存器RA也被隐式定义
        def.insert(offset + static_cast<unsigned>(PhysicalReg::RA));
    }
    else if (opcode == RVOpcodes::JALR) {
        // JALR rd, rs1, imm. Def: rd, Use: rs1. 
        // 同时也隐式定义了ra(x1)，但通常rd就是ra。为精确，我们只处理显式操作数。
        def.insert(get_any_reg_id(operands[0].get()));
        use.insert(get_any_reg_id(operands[1].get()));
    }
    else if (opcode == RVOpcodes::RET) {
        // 遵循调用约定，a0(整数/指针)和fa0(浮点)被隐式使用
        use.insert(offset + static_cast<unsigned>(PhysicalReg::A0));
        use.insert(offset + static_cast<unsigned>(PhysicalReg::F10)); // F10 is fa0
    }
    // 添加对 PSEUDO_KEEPALIVE 的处理
    else if (opcode == RVOpcodes::PSEUDO_KEEPALIVE) {
        // keepalive的所有操作数都是use，以确保它们的生命周期延续到该点
        for (const auto& op : operands) {
            if (op->getKind() == MachineOperand::KIND_REG) {
                unsigned reg_id = get_any_reg_id(op.get());
                if (reg_id != (unsigned)-1) use.insert(reg_id);
            }
        }
    }
}

void RISCv64RegAlloc::addEdge(unsigned u, unsigned v) {
    if (u == v) return;

    // 检查两个节点是否都是虚拟寄存器
    if (!precolored.count(u) && !precolored.count(v) && !coloredNodes.count(u) && !coloredNodes.count(v)) {
        // 只有当两个都是虚拟寄存器时，才为它们双方添加边和更新度数
        // 使用 operator[] 是安全的，如果键不存在，它会默认构造一个空的set
        if (adjList[u].find(v) == adjList[u].end()) {
            adjList[u].insert(v);
            adjList[v].insert(u);
            degree[u]++;
            degree[v]++;
        }
    }
    // 检查是否为 "虚拟-物理" 对
    else if (!precolored.count(u) && precolored.count(v) && !coloredNodes.count(u)) {
        // u是虚拟寄存器，v是物理寄存器，只更新u的邻接表和度数
        if (adjList[u].find(v) == adjList[u].end()) {
            adjList[u].insert(v);
            degree[u]++;
        }
    }
    // 检查是否为 "物理-虚拟" 对
    else if (precolored.count(u) && !precolored.count(v) && !coloredNodes.count(v)) {
        // u是物理寄存器，v是虚拟寄存器，只更新v的邻接表和度数
        if (adjList[v].find(u) == adjList[v].end()) {
            adjList[v].insert(u);
            degree[v]++;
        }
    }
    // 如果两个都是物理寄存器，则什么都不做，直接返回。
}

RISCv64RegAlloc::VRegSet RISCv64RegAlloc::adjacent(unsigned n) {
    // 仅在 DEEPDEBUG 模式下启用详细日志
    if (DEEPERDEBUG) {
        // 使用 regIdToString 打印节点 n，无论是物理还是虚拟
        std::cerr << "\n[adjacent] >>>>> Executing for node " << regIdToString(n) << " <<<<<\n";
    }

    // 1. 如果节点 n 是物理寄存器，它没有邻接表，直接返回空集
    if (precolored.count(n)) {
        if (DEEPERDEBUG) {
            std::cerr << "[adjacent] Node " << regIdToString(n) << " is precolored. Returning {}.\n";
        }
        return {};
    }

    // 安全检查：确保 n 在 adjList 中存在，防止 map::at 崩溃
    if (adjList.count(n) == 0) {
        if (DEEPERDEBUG) {
            std::cerr << "[adjacent] WARNING: Node " << regIdToString(n) << " not found in adjList. Returning {}.\n";
        }
        return {};
    }

    // 2. 获取 n 在冲突图中的所有邻居
    VRegSet result = adjList.at(n);
    
    if (DEEPERDEBUG) {
        // 定义一个局部的 lambda 方便打印集合
        auto print_set = [this](const VRegSet& s, const std::string& name) {
            std::cerr << "[adjacent] " << name << " (" << s.size() << "): { ";
            for (unsigned id : s) std::cerr << regIdToString(id) << " ";
            std::cerr << "}\n";
        };
        print_set(result, "Initial full neighbors");
    }

    // 3. 过滤掉那些已经在 selectStack 或 coalescedNodes 中的邻居
    //    这些节点被认为是“已移除”的，不参与当前的启发式判断

    // 3a. 从 selectStack 中移除
    VRegSet removed_from_stack; // 仅用于调试打印
    for (auto it = selectStack.rbegin(); it != selectStack.rend(); ++it) {
        if (result.count(*it)) {
            if (DEEPERDEBUG) removed_from_stack.insert(*it);
            result.erase(*it);
        }
    }
    if (DEEPERDEBUG && !removed_from_stack.empty()) {
        std::cerr << "[adjacent]   - Removed from selectStack: { ";
        for(unsigned id : removed_from_stack) std::cerr << regIdToString(id) << " ";
        std::cerr << "}\n";
    }

    // 3b. 从 coalescedNodes 中移除
    VRegSet removed_from_coalesced; // 仅用于调试打印
    for (unsigned cn : coalescedNodes) {
        if (result.count(cn)) {
            if (DEEPERDEBUG) removed_from_coalesced.insert(cn);
            result.erase(cn);
        }
    }
    if (DEEPERDEBUG && !removed_from_coalesced.empty()) {
        std::cerr << "[adjacent]   - Removed from coalescedNodes: { ";
        for(unsigned id : removed_from_coalesced) std::cerr << regIdToString(id) << " ";
        std::cerr << "}\n";
    }

    // 4. 返回最终的、过滤后的“有效”邻居集合
    if (DEEPERDEBUG) {
        std::cerr << "[adjacent] >>>>> Returning final adjacent set (" << result.size() << "): { ";
        for (unsigned id : result) std::cerr << regIdToString(id) << " ";
        std::cerr << "}\n\n";
    }

    return result;
}

RISCv64RegAlloc::VRegMoveSet RISCv64RegAlloc::nodeMoves(unsigned n) {
    if (precolored.count(n) || !moveList.count(n)) {
        return {};
    }
    
    VRegMoveSet result;
    const VRegMoveSet& moves = moveList.at(n);
    for (const auto& move : moves) {
        if (activeMoves.count(move) || worklistMoves.count(move)) {
            result.insert(move);
        }
    }
    return result;
}

bool RISCv64RegAlloc::moveRelated(unsigned n) {
    return !nodeMoves(n).empty();
}

void RISCv64RegAlloc::decrementDegree(unsigned m) {
    if (precolored.count(m)) {
        return;
    }

    int K = isFPVReg(m) ? K_fp : K_int;
    int d = degree.at(m);
    degree.at(m)--;
    if (d == K) {
        VRegSet nodes_to_enable = adjacent(m);
        nodes_to_enable.insert(m);
        enableMoves(nodes_to_enable);
        spillWorklist.erase(m);
        if (moveRelated(m)) {
            if (DEEPERDEBUG) {
                std::cerr << "[decrementDegree] Node " << regIdToString(m) << " has degree " << d << ", now decremented to " << degree.at(m) << ". Added to freezeWorklist.\n";
            }
            freezeWorklist.insert(m);
        } else {
            if (DEEPERDEBUG) {
                std::cerr << "[decrementDegree] Node " << regIdToString(m) << " has degree " << d << ", now decremented to " << degree.at(m) << ". Added to simplifyWorklist.\n";
            }
            simplifyWorklist.insert(m);
        }
    }
}

void RISCv64RegAlloc::enableMoves(const VRegSet& nodes) {
    for (unsigned n : nodes) {
        VRegMoveSet moves = nodeMoves(n);
        for (const auto& move : moves) {
            if (activeMoves.count(move)) {
                activeMoves.erase(move);
                worklistMoves.insert(move);
            }
        }
    }
}

unsigned RISCv64RegAlloc::getAlias(unsigned n) {
    if (precolored.count(n)) {
        return n;
    }
    if (alias.count(n)) {
        // 路径压缩
        alias.at(n) = getAlias(alias.at(n));
        return alias.at(n);
    }
    return n;
}

void RISCv64RegAlloc::addWorklist(unsigned u) {
    if (precolored.count(u) || color_map.count(u)) return;

    int K = isFPVReg(u) ? K_fp : K_int;
    if (!moveRelated(u) && degree.at(u) < K) {
        freezeWorklist.erase(u);
        simplifyWorklist.insert(u);
        if (DEEPERDEBUG) {
            std::cerr << "[addWorklist] Node " << regIdToString(u) << " added to simplifyWorklist (degree: " << degree.at(u) << ", K: " << K << ").\n";
        }
    }
}

// Briggs启发式
bool RISCv64RegAlloc::briggsHeuristic(unsigned u, unsigned v) {
    if (DEEPERDEBUG) {
        std::cerr << "\n[Briggs] >>>>> Checking coalesce between " << regIdToString(u) << " and " << regIdToString(v) << " <<<<<\n";
    }

    // 步骤 1: 分别获取 u 和 v 的邻居
    VRegSet u_adj = adjacent(u);
    VRegSet v_adj = adjacent(v);

    // 步骤 2: 合并两个邻居集合
    VRegSet all_adj = u_adj;
    all_adj.insert(v_adj.begin(), v_adj.end());

    if (DEEPERDEBUG) {
        auto print_set = [this](const VRegSet& s, const std::string& name) {
            std::cerr << "[Briggs] " << name << " (" << s.size() << "): { ";
            for (unsigned id : s) std::cerr << regIdToString(id) << " ";
            std::cerr << "}\n";
        };
        print_set(u_adj, "Neighbors of u");
        print_set(v_adj, "Neighbors of v");
        print_set(all_adj, "Combined neighbors");
    }

    // 步骤 3: 遍历合并后的邻居集合，计算度数 >= K 的节点数量
    int k = 0;
    if (DEEPERDEBUG) std::cerr << "[Briggs] Checking significance of combined neighbors:\n";
    for (unsigned n : all_adj) {
        // 关键修正：只考虑那些在工作集中的邻居节点 n
        if (degree.count(n) > 0) {
            int K = isFPVReg(n) ? K_fp : K_int;
            if (degree.at(n) >= K) {
                k++;
                if (DEEPERDEBUG) {
                    std::cerr << "[Briggs]   - Node " << regIdToString(n) << " is significant (degree " << degree.at(n) << " >= " << K << "). Count k is now " << k << ".\n";
                }
            }
        }
    }

    // 步骤 4: 比较 "重要" 邻居的数量是否小于 K
    int K_u = isFPVReg(u) ? K_fp : K_int;
    bool result = (k < K_u);

    if (DEEPERDEBUG) {
        std::cerr << "[Briggs] Final count of significant neighbors (k) = " << k << ".\n";
        std::cerr << "[Briggs] K value for node " << regIdToString(u) << " is " << K_u << ".\n";
        std::cerr << "[Briggs] >>>>> Result (k < K): " << (result ? "OK (can coalesce)" : "FAIL (cannot coalesce)") << "\n\n";
    }
    return result;
}

// George启发式
bool RISCv64RegAlloc::georgeHeuristic(unsigned t, unsigned u) {
    // 如果 t 不是一个待分配的虚拟寄存器（即它是物理寄存器），
    // 那么它已经被预着色，总是满足 George 启发式条件。
    // 我们通过检查 degree.count(t) 来判断 t 是否在我们的虚拟寄存器工作集中。
    if (degree.count(t) == 0) {
        return true;
    }
    
    int K = isFPVReg(t) ? K_fp : K_int;
    
    return degree.at(t) < K || adjList.at(t).count(u);
}

void RISCv64RegAlloc::combine(unsigned u, unsigned v) {
    freezeWorklist.erase(v);
    spillWorklist.erase(v);
    coalescedNodes.insert(v);
    alias[v] = u;
    if (moveList.count(u) && moveList.count(v)) {
        moveList.at(u).insert(moveList.at(v).begin(), moveList.at(v).end());
    } else if (moveList.count(v)) {
        moveList[u] = moveList.at(v);
    }
    enableMoves({v});
    for (unsigned t : adjList.at(v)) { 
        addEdge(t, u);
        decrementDegree(t);
    }

    if (!precolored.count(u)) {
        int K = isFPVReg(u) ? K_fp : K_int;
        if (degree.at(u) >= K && freezeWorklist.count(u)) {
            freezeWorklist.erase(u);
            spillWorklist.insert(u);
        }
    }
}

void RISCv64RegAlloc::freezeMoves(unsigned u) {
    if (precolored.count(u)) return;

    VRegMoveSet moves = nodeMoves(u);
    for (const auto& move : moves) {
        VRegSet use, def;
        getInstrUseDef_Liveness(move, use, def);
        unsigned x = *def.begin();
        unsigned y = *use.begin();
        unsigned v_alias;

        if (getAlias(y) == getAlias(u)) {
            v_alias = getAlias(x);
        } else {
            v_alias = getAlias(y);
        }

        activeMoves.erase(move);
        frozenMoves.insert(move);

        if (!precolored.count(v_alias) && !coloredNodes.count(v_alias) && nodeMoves(v_alias).empty() && degree.at(v_alias) < (isFPVReg(v_alias) ? K_fp : K_int)) {
            freezeWorklist.erase(v_alias);
            simplifyWorklist.insert(v_alias);
            if (DEEPERDEBUG) {
                std::cerr << "[freezeMoves] Node " << regIdToString(v_alias) << " moved to simplifyWorklist (degree: " << degree.at(v_alias) << ").\n";
            }
        }
    }
}

// 检查vreg是否为浮点类型
bool RISCv64RegAlloc::isFPVReg(unsigned vreg) const {
    // 1. 检查是否为虚拟寄存器
    if (vreg_type_map.count(vreg)) {
        return vreg_type_map.at(vreg)->isFloat();
    }
    
    // 2. 检查是否为物理寄存器 (ID >= 100000)
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    if (vreg >= offset) {
        // 先减去偏移，还原成原始的、小的枚举值
        unsigned raw_preg_id = vreg - offset;
        
        // 再用原始枚举值判断是否在浮点寄存器范围内
        if (raw_preg_id >= static_cast<unsigned>(PhysicalReg::F0) && raw_preg_id <= static_cast<unsigned>(PhysicalReg::F31)) {
            return true;
        }
    }

    // 3. 其他所有情况（如未知的vreg，或整数物理寄存器）都默认为整数
    return false;
}

// 收集被使用的被调用者保存寄存器
void RISCv64RegAlloc::collectUsedCalleeSavedRegs() {
    StackFrameInfo& frame_info = MFunc->getFrameInfo();
    frame_info.used_callee_saved_regs.clear();

    const auto& callee_saved_int = getCalleeSavedIntRegs();
    const auto& callee_saved_fp = getCalleeSavedFpRegs();
    std::set<PhysicalReg> callee_saved_set(callee_saved_int.begin(), callee_saved_int.end());
    callee_saved_set.insert(callee_saved_fp.begin(), callee_saved_fp.end());
    // s0总是被使用作为帧指针
    callee_saved_set.insert(PhysicalReg::S0);


    for(const auto& pair : color_map) {
        PhysicalReg preg = pair.second;
        if(callee_saved_set.count(preg)) {
            frame_info.used_callee_saved_regs.insert(preg);
        }
    }
}

/**
 * @brief 将最终的寄存器分配结果应用到所有机器指令上。
 * 遍历所有操作数，将虚拟寄存器替换为分配到的物理寄存器。
 */
void RISCv64RegAlloc::applyColoring() {
    for (auto& mbb : MFunc->getBlocks()) {
        for (auto& instr_ptr : mbb->getInstructions()) {
            for (auto& op_ptr : instr_ptr->getOperands()) {
                if (op_ptr->getKind() == MachineOperand::KIND_REG) {
                    auto reg_op = static_cast<RegOperand*>(op_ptr.get());
                    if (reg_op->isVirtual()) {
                        unsigned vreg = reg_op->getVRegNum();
                        if (color_map.count(vreg)) {
                            // 使用 setPReg 将虚拟寄存器转换为物理寄存器
                            reg_op->setPReg(color_map.at(vreg));
                        } else {
                            // 如果一个vreg在成功分配后仍然没有颜色，可能是紧急溢出
                            // std::cerr << "WARNING: Virtual register %vreg" << vreg << " has no color after allocation, treating as spilled\n";
                            // 在紧急溢出情况下，使用临时寄存器
                            reg_op->setPReg(PhysicalReg::T6);
                        }
                    }
                } else if (op_ptr->getKind() == MachineOperand::KIND_MEM) {
                    auto mem_op = static_cast<MemOperand*>(op_ptr.get());
                    auto reg_op = mem_op->getBase();
                    if (reg_op->isVirtual()) {
                        unsigned vreg = reg_op->getVRegNum();
                        if (color_map.count(vreg)) {
                            reg_op->setPReg(color_map.at(vreg));
                        } else {
                            // std::cerr << "WARNING: Virtual register in memory operand has no color, using T6\n";
                            reg_op->setPReg(PhysicalReg::T6);
                        }
                    }
                }
            }
        }
    }
}

void RISCv64RegAlloc::dumpState(const std::string& stage) {
    if (!DEEPDEBUG) return;
    std::cerr << "\n=============== STATE DUMP (" << stage << ") ===============\n";
    auto print_vreg_set = [&](const VRegSet& s, const std::string& name){
        if (s.size() > DEBUGLENGTH) {
            std::cerr << name << " (" << s.size() << ")\n";
        }
        else {
            std::cerr << name << " (" << s.size() << "): { ";
            for(unsigned v : s) std::cerr << "%vreg" << v << " ";
            std::cerr << "}\n";
        }
        
    };
    auto print_vreg_stack = [&](const VRegStack& s, const std::string& name){
        if (s.size() > DEBUGLENGTH) {
            std::cerr << name << " (" << s.size() << ")\n";
        }
        else {
            std::cerr << name << " (" << s.size() << "): { ";
            for(unsigned v : s) std::cerr << "%vreg" << v << " ";
            std::cerr << "}\n";
        }
        
    };
    print_vreg_set(simplifyWorklist, "SimplifyWorklist");
    print_vreg_set(freezeWorklist, "FreezeWorklist");
    print_vreg_set(spillWorklist, "SpillWorklist");
    print_vreg_set(coalescedNodes, "CoalescedNodes");
    print_vreg_set(spilledNodes, "SpilledNodes");

    print_vreg_stack(selectStack, "SelectStack");

    std::cerr << "WorklistMoves (" << worklistMoves.size() << ")\n";
    std::cerr << "ActiveMoves (" << activeMoves.size() << ")\n";

    size_t final_nodes = coalescedNodes.size() + spilledNodes.size() + selectStack.size();
    std::cerr << "Total Final Nodes: " << final_nodes << "\n";
    std::cerr << "=======================================================\n";
}

std::string RISCv64RegAlloc::regToString(PhysicalReg reg) {
    switch (reg) {
        case PhysicalReg::ZERO: return "x0";  case PhysicalReg::RA: return "ra";
        case PhysicalReg::SP: return "sp";    case PhysicalReg::GP: return "gp";
        case PhysicalReg::TP: return "tp";    case PhysicalReg::T0: return "t0";
        case PhysicalReg::T1: return "t1";    case PhysicalReg::T2: return "t2";
        case PhysicalReg::S0: return "s0";    case PhysicalReg::S1: return "s1";
        case PhysicalReg::A0: return "a0";    case PhysicalReg::A1: return "a1";
        case PhysicalReg::A2: return "a2";    case PhysicalReg::A3: return "a3";
        case PhysicalReg::A4: return "a4";    case PhysicalReg::A5: return "a5";
        case PhysicalReg::A6: return "a6";    case PhysicalReg::A7: return "a7";
        case PhysicalReg::S2: return "s2";    case PhysicalReg::S3: return "s3";
        case PhysicalReg::S4: return "s4";    case PhysicalReg::S5: return "s5";
        case PhysicalReg::S6: return "s6";    case PhysicalReg::S7: return "s7";
        case PhysicalReg::S8: return "s8";    case PhysicalReg::S9: return "s9";
        case PhysicalReg::S10: return "s10";  case PhysicalReg::S11: return "s11";
        case PhysicalReg::T3: return "t3";    case PhysicalReg::T4: return "t4";
        case PhysicalReg::T5: return "t5";    case PhysicalReg::T6: return "t6";
        case PhysicalReg::F0: return "f0";    case PhysicalReg::F1: return "f1";
        case PhysicalReg::F2: return "f2";    case PhysicalReg::F3: return "f3";
        case PhysicalReg::F4: return "f4";    case PhysicalReg::F5: return "f5";
        case PhysicalReg::F6: return "f6";    case PhysicalReg::F7: return "f7";
        case PhysicalReg::F8: return "f8";    case PhysicalReg::F9: return "f9";
        case PhysicalReg::F10: return "f10";  case PhysicalReg::F11: return "f11";
        case PhysicalReg::F12: return "f12";  case PhysicalReg::F13: return "f13";
        case PhysicalReg::F14: return "f14";  case PhysicalReg::F15: return "f15";
        case PhysicalReg::F16: return "f16";  case PhysicalReg::F17: return "f17";
        case PhysicalReg::F18: return "f18";  case PhysicalReg::F19: return "f19";
        case PhysicalReg::F20: return "f20";  case PhysicalReg::F21: return "f21";
        case PhysicalReg::F22: return "f22";  case PhysicalReg::F23: return "f23";
        case PhysicalReg::F24: return "f24";  case PhysicalReg::F25: return "f25";
        case PhysicalReg::F26: return "f26";  case PhysicalReg::F27: return "f27";
        case PhysicalReg::F28: return "f28";  case PhysicalReg::F29: return "f29";
        case PhysicalReg::F30: return "f30";  case PhysicalReg::F31: return "f31";
        default: return "UNKNOWN_REG";
    }
}

std::string RISCv64RegAlloc::regIdToString(unsigned id) {
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);

    if (id >= offset && precolored.count(id)) {
        // 先减去偏移量，得到原始的、小的枚举值
        PhysicalReg reg = static_cast<PhysicalReg>(id - offset);
        // 再将原始枚举值传给 regToString
        return regToString(reg);
    } else {
        return "%vreg" + std::to_string(id);
    }
}

} // namespace sysy