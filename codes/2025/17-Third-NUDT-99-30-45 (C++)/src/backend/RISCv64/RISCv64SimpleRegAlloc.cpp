#include "RISCv64SimpleRegAlloc.h"
#include "RISCv64AsmPrinter.h"
#include "RISCv64Info.h"
#include <algorithm>
#include <iostream>
#include <cassert>

// 外部调试级别控制变量的定义
// 假设这些变量在其他地方定义，例如主程序或一个通用的cpp文件
extern int DEBUG;
extern int DEEPDEBUG;

namespace sysy {

RISCv64SimpleRegAlloc::RISCv64SimpleRegAlloc(MachineFunction* mfunc) : MFunc(mfunc), ISel(mfunc->getISel()) {
    // 1. 初始化可分配的整数寄存器池
    // T5 被大立即数传送逻辑保留
    // T2, T3, T4 被本分配器保留为专用的溢出/临时寄存器
    allocable_int_regs = {
        PhysicalReg::T0, PhysicalReg::T1, /* T2,T3,T4,T5,T6 reserved */
        PhysicalReg::A0, PhysicalReg::A1, PhysicalReg::A2, PhysicalReg::A3, PhysicalReg::A4, PhysicalReg::A5, PhysicalReg::A6, PhysicalReg::A7,
        PhysicalReg::S1, PhysicalReg::S2, PhysicalReg::S3, PhysicalReg::S4, PhysicalReg::S5, PhysicalReg::S6, PhysicalReg::S7,
        PhysicalReg::S8, PhysicalReg::S9, PhysicalReg::S10, PhysicalReg::S11,
    };

    // 2. 初始化可分配的浮点寄存器池
    // F0, F1, F2 被本分配器保留为专用的溢出/临时寄存器
    allocable_fp_regs = {
        /* F0,F1,F2 reserved */ PhysicalReg::F3, PhysicalReg::F4, PhysicalReg::F5, PhysicalReg::F6, PhysicalReg::F7,
        PhysicalReg::F10, PhysicalReg::F11, PhysicalReg::F12, PhysicalReg::F13, PhysicalReg::F14, PhysicalReg::F15, PhysicalReg::F16, PhysicalReg::F17,
        PhysicalReg::F8, PhysicalReg::F9, PhysicalReg::F18, PhysicalReg::F19, PhysicalReg::F20, PhysicalReg::F21, PhysicalReg::F22,
        PhysicalReg::F23, PhysicalReg::F24, PhysicalReg::F25, PhysicalReg::F26, PhysicalReg::F27,
        PhysicalReg::F28, PhysicalReg::F29, PhysicalReg::F30, PhysicalReg::F31,
    };
    
    // 3. 映射所有物理寄存器到特殊的虚拟寄存器ID (保持不变)
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    for (unsigned i = 0; i < static_cast<unsigned>(PhysicalReg::INVALID); ++i) {
        auto preg = static_cast<PhysicalReg>(i);
        preg_to_vreg_id_map[preg] = offset + i;
    }
}

// 寄存器分配的主入口点
void RISCv64SimpleRegAlloc::run() {
    if (DEBUG) std::cerr << "===== Running Simple Graph Coloring Allocator for function: " << MFunc->getName() << " =====\n";
    
    // 实例化一个AsmPrinter用于调试输出，避免重复创建
    RISCv64AsmPrinter printer(MFunc);
    printer.setStream(std::cerr);

    if (DEBUG) {
        std::cerr << "\n===== LLIR after VReg Unification =====\n";
        printer.run(std::cerr, true);
        std::cerr << "===== End of Unified LLIR =====\n\n";
    }

    // 阶段 1: 处理函数调用约定（参数寄存器预着色）
    handleCallingConvention();    
    if (DEBUG) {
        std::cerr << "--- After HandleCallingConvention ---\n";
        std::cerr << "Pre-colored vregs:\n";
        for (const auto& pair : color_map) {
            std::cerr << "  %vreg" << pair.first << " -> " << printer.regToString(pair.second) << "\n";
        }
    }

    // 阶段 2: 活跃性分析
    analyzeLiveness();            
    
    // 阶段 3: 构建干扰图
    buildInterferenceGraph();     
    
    // 阶段 4: 图着色算法分配物理寄存器
    colorGraph();                 
    if (DEBUG) {
        std::cerr << "\n--- After GraphColoring ---\n";
        std::cerr << "Assigned colors:\n";
        for (const auto& pair : color_map) {
            std::cerr << "  %vreg" << pair.first << " -> " << printer.regToString(pair.second) << "\n";
        }
        std::cerr << "Spilled vregs:\n";
        if (spilled_vregs.empty()) {
            std::cerr << "  (None)\n";
        } else {
            for (unsigned vreg : spilled_vregs) {
                std::cerr << "  %vreg" << vreg << "\n";
            }
        }
    }
    
    // 阶段 5: 重写函数（插入溢出/填充代码，替换虚拟寄存器为物理寄存器）
    rewriteFunction();
    
    // 将最终的寄存器分配结果保存到MachineFunction的帧信息中，供后续Pass使用
    MFunc->getFrameInfo().vreg_to_preg_map = this->color_map;

    if (DEBUG) {
        std::cerr << "\n===== Final LLIR after Simple Register Allocation =====\n";
        printer.run(std::cerr, false); // 使用false来打印最终的物理寄存器
        std::cerr << "===== Finished Simple Graph Coloring Allocator =====\n\n";
    }
}

/**
 * @brief [新增] 虚拟寄存器统一预处理
 * 扫描函数，找到通过栈帧传递的参数，并将后续从该栈帧加载的VReg统一为原始的参数VReg。
 */
void RISCv64SimpleRegAlloc::unifyArgumentVRegs() {
    if (MFunc->getBlocks().size() < 2) return; // 至少需要入口和函数体两个块

    std::map<int, unsigned> stack_slot_to_vreg; // 映射: <栈偏移, 原始参数vreg>
    MachineBasicBlock* entry_block = MFunc->getBlocks().front().get();

    // 步骤 1: 扫描入口块，找到所有参数的“家（home）”在栈上的位置
    for (const auto& instr : entry_block->getInstructions()) {
        // 我们寻找 sw %vreg_arg, 0(%vreg_addr) 的模式
        if (instr->getOpcode() == RVOpcodes::SW || instr->getOpcode() == RVOpcodes::SD || instr->getOpcode() == RVOpcodes::FSW) {
            auto& operands = instr->getOperands();
            if (operands.size() == 2 && operands[0]->getKind() == MachineOperand::KIND_REG && operands[1]->getKind() == MachineOperand::KIND_MEM) {
                auto src_reg_op = static_cast<RegOperand*>(operands[0].get());
                auto mem_op = static_cast<MemOperand*>(operands[1].get());
                unsigned addr_vreg = mem_op->getBase()->getVRegNum();
                
                // 查找定义这个地址vreg的addi指令，以获取偏移量
                for (const auto& prev_instr : entry_block->getInstructions()) {
                    if (prev_instr->getOpcode() == RVOpcodes::ADDI && prev_instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
                        auto def_op = static_cast<RegOperand*>(prev_instr->getOperands().front().get());
                        if (def_op->isVirtual() && def_op->getVRegNum() == addr_vreg) {
                            int offset = static_cast<ImmOperand*>(prev_instr->getOperands()[2].get())->getValue();
                            stack_slot_to_vreg[offset] = src_reg_op->getVRegNum();
                            break;
                        }
                    }
                }
            }
        }
    }

    if (stack_slot_to_vreg.empty()) return; // 没有找到参数存储，无需处理

    // 步骤 2: 扫描函数体，构建本地vreg到参数vreg的重映射表
    std::map<unsigned, unsigned> vreg_remap; // 映射: <本地vreg, 原始参数vreg>
    MachineBasicBlock* body_block = MFunc->getBlocks()[1].get();

    for (const auto& instr : body_block->getInstructions()) {
        if (instr->getOpcode() == RVOpcodes::LW || instr->getOpcode() == RVOpcodes::LD || instr->getOpcode() == RVOpcodes::FLW) {
            auto& operands = instr->getOperands();
            if (operands.size() == 2 && operands[0]->getKind() == MachineOperand::KIND_REG && operands[1]->getKind() == MachineOperand::KIND_MEM) {
                auto dest_reg_op = static_cast<RegOperand*>(operands[0].get());
                auto mem_op = static_cast<MemOperand*>(operands[1].get());
                unsigned addr_vreg = mem_op->getBase()->getVRegNum();
                
                // 同样地，查找定义地址的addi指令
                for (const auto& prev_instr : body_block->getInstructions()) {
                     if (prev_instr->getOpcode() == RVOpcodes::ADDI && prev_instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
                        auto def_op = static_cast<RegOperand*>(prev_instr->getOperands().front().get());
                        if (def_op->isVirtual() && def_op->getVRegNum() == addr_vreg) {
                            int offset = static_cast<ImmOperand*>(prev_instr->getOperands()[2].get())->getValue();
                            if (stack_slot_to_vreg.count(offset)) {
                                unsigned old_vreg = dest_reg_op->getVRegNum();
                                unsigned new_vreg = stack_slot_to_vreg.at(offset);
                                vreg_remap[old_vreg] = new_vreg;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    if (vreg_remap.empty()) return;

    // 步骤 3: 遍历所有指令，应用重映射
    // 定义一个lambda函数来替换vreg，避免代码重复
    auto replace_vreg_in_operand = [&](MachineOperand* op) {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<RegOperand*>(op);
            if (reg_op->isVirtual() && vreg_remap.count(reg_op->getVRegNum())) {
                reg_op->setVRegNum(vreg_remap.at(reg_op->getVRegNum()));
            }
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            auto base_reg_op = static_cast<MemOperand*>(op)->getBase();
            if (base_reg_op->isVirtual() && vreg_remap.count(base_reg_op->getVRegNum())) {
                base_reg_op->setVRegNum(vreg_remap.at(base_reg_op->getVRegNum()));
            }
        }
    };

    for (auto& mbb : MFunc->getBlocks()) {
        for (auto& instr : mbb->getInstructions()) {
            for (auto& op : instr->getOperands()) {
                replace_vreg_in_operand(op.get());
            }
        }
    }
}

void RISCv64SimpleRegAlloc::handleCallingConvention() {
    Function* F = MFunc->getFunc();
    if (!F) return;

    // --- 1. 处理函数传入参数的预着色 ---
    int int_arg_idx = 0;
    int float_arg_idx = 0;

    for (Argument* arg : F->getArguments()) {
        unsigned vreg = ISel->getVReg(arg);
        if (arg->getType()->isFloat()) {
            if (float_arg_idx < 8) { // fa0-fa7
                auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + float_arg_idx);
                color_map[vreg] = preg;
            }
            float_arg_idx++;
        } else {
            if (int_arg_idx < 8) { // a0-a7
                auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + int_arg_idx);
                color_map[vreg] = preg;
            }
            int_arg_idx++;
        }
    }
}

void RISCv64SimpleRegAlloc::analyzeLiveness() {
    if (DEBUG) std::cerr << "\n--- Starting Liveness Analysis ---\n";
    
    // === 阶段 1: 预计算每个基本块的 use 和 def 集合 ===
    std::map<const MachineBasicBlock*, LiveSet> block_uses;
    std::map<const MachineBasicBlock*, LiveSet> block_defs;
    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        const MachineBasicBlock* mbb = mbb_ptr.get();
        LiveSet uses, defs;
        for (const auto& instr_ptr : mbb->getInstructions()) {
            LiveSet instr_use, instr_def;
            getInstrUseDef_Liveness(instr_ptr.get(), instr_use, instr_def);
            // use[B] = use[B] U (instr_use - def[B])
            for (unsigned u : instr_use) {
                if (defs.find(u) == defs.end()) {
                    uses.insert(u);
                }
            }
            // def[B] = def[B] U instr_def
            defs.insert(instr_def.begin(), instr_def.end());
        }
        block_uses[mbb] = uses;
        block_defs[mbb] = defs;
    }

    // === 阶段 2: 在“块”粒度上进行迭代数据流分析，直到收敛 ===
    std::map<const MachineBasicBlock*, LiveSet> block_live_in;
    std::map<const MachineBasicBlock*, LiveSet> block_live_out;
    bool changed = true;
    while (changed) {
        changed = false;
        // 逆序遍历基本块，加速收敛
        for (auto it = MFunc->getBlocks().rbegin(); it != MFunc->getBlocks().rend(); ++it) {
            const auto& mbb_ptr = *it;
            const MachineBasicBlock* mbb = mbb_ptr.get();
            
            // 2.1 计算 live_out[B] = U_{S in succ(B)} live_in[S]
            LiveSet new_live_out;
            for (auto succ : mbb->successors) {
                new_live_out.insert(block_live_in[succ].begin(), block_live_in[succ].end());
            }

            // 2.2 计算 live_in[B] = use[B] U (live_out[B] - def[B])
            LiveSet live_out_minus_def = new_live_out;
            for (unsigned d : block_defs.at(mbb)) {
                live_out_minus_def.erase(d);
            }
            LiveSet new_live_in = block_uses.at(mbb);
            new_live_in.insert(live_out_minus_def.begin(), live_out_minus_def.end());

            // 2.3 检查是否达到不动点
            if (block_live_out[mbb] != new_live_out || block_live_in[mbb] != new_live_in) {
                changed = true;
                block_live_out[mbb] = new_live_out;
                block_live_in[mbb] = new_live_in;
            }
        }
    }

    // === 阶段 3: 进行一次指令粒度的遍历，填充最终的 live_in_map 和 live_out_map ===
    for (const auto& mbb_ptr : MFunc->getBlocks()) {
        const MachineBasicBlock* mbb = mbb_ptr.get();
        LiveSet live_out = block_live_out.at(mbb);

        for (auto instr_it = mbb->getInstructions().rbegin(); instr_it != mbb->getInstructions().rend(); ++instr_it) {
            const MachineInstr* instr = instr_it->get();
            live_out_map[instr] = live_out;

            LiveSet use, def;
            getInstrUseDef_Liveness(instr, use, def);

            LiveSet live_in = use;
            LiveSet diff = live_out;
            for (auto vreg : def) {
                diff.erase(vreg);
            }
            live_in.insert(diff.begin(), diff.end());
            live_in_map[instr] = live_in;
            
            // 更新 live_out，为块内的上一条指令做准备
            live_out = live_in;
        }
    }
}

void RISCv64SimpleRegAlloc::buildInterferenceGraph() {
    if (DEBUG) std::cerr << "\n--- Starting Interference Graph Construction ---\n";
    RISCv64AsmPrinter printer(MFunc);
    printer.setStream(std::cerr);

    // 1. 收集所有图中需要出现的节点 (所有虚拟寄存器和物理寄存器)
    std::set<unsigned> all_nodes;
    for (const auto& mbb : MFunc->getBlocks()) {
        for(const auto& instr : mbb->getInstructions()) {
            LiveSet use, def;
            getInstrUseDef_Liveness(instr.get(), use, def);
            all_nodes.insert(use.begin(), use.end());
            all_nodes.insert(def.begin(), def.end());
        }
    }
    // 确保所有物理寄存器节点也存在
    for (const auto& pair : preg_to_vreg_id_map) {
        all_nodes.insert(pair.second);
    }

    // 2. 初始化干扰图邻接表
    for (unsigned vreg : all_nodes) { interference_graph[vreg] = {}; }

    // 3. 遍历指令，添加冲突边
    for (const auto& mbb : MFunc->getBlocks()) {
        if (DEEPDEBUG) std::cerr << "--- Building Graph for Basic Block: " << mbb->getName() << " ---\n";
        for (const auto& instr_ptr : mbb->getInstructions()) {
            const MachineInstr* instr = instr_ptr.get();
            if (DEEPDEBUG) {
                std::cerr << "  Instr: ";
                printer.printInstruction(const_cast<MachineInstr*>(instr), true);
            }
            
            LiveSet def, use;
            getInstrUseDef_Liveness(instr, def, use); // 注意Use/Def顺序
            const LiveSet& live_out = live_out_map.at(instr);
            
            if (DEEPDEBUG) {
                printLiveSet(use, "Use     ", std::cerr, printer);
                printLiveSet(def, "Def     ", std::cerr, printer);
                printLiveSet(live_out, "Live_Out", std::cerr, printer);
            }

            // 规则1: 指令的定义(def)与该指令之后的所有活跃变量(live_out)冲突
            for (unsigned d : def) {
                for (unsigned l : live_out) {
                    if (d != l) {
                        if (DEEPDEBUG && interference_graph.at(d).find(l) == interference_graph.at(d).end()) {
                           std::cerr << "    Edge (Def-LiveOut): " << regIdToString(d, printer) << " <-> " << regIdToString(l, printer) << "\n";
                        }
                        interference_graph[d].insert(l);
                        interference_graph[l].insert(d);
                    }
                }
            }
            
            // 规则2: 对于非MV指令, def与use也冲突
            if (instr->getOpcode() != RVOpcodes::MV) {
                for (unsigned d : def) {
                    for (unsigned u : use) {
                        if (d != u) {
                            if (DEEPDEBUG && interference_graph.at(d).find(u) == interference_graph.at(d).end()) {
                                std::cerr << "    Edge (Def-Use): " << regIdToString(d, printer) << " <-> " << regIdToString(u, printer) << "\n";
                            }
                            interference_graph[d].insert(u);
                            interference_graph[u].insert(d);
                        }
                    }
                }
            }

            // 所有在某一点上同时活跃的寄存器（即live_out集合中的所有成员），
            // 它们之间必须两两互相干扰。
            std::vector<unsigned> live_out_vec(live_out.begin(), live_out.end());
            for (size_t i = 0; i < live_out_vec.size(); ++i) {
                for (size_t j = i + 1; j < live_out_vec.size(); ++j) {
                    unsigned u = live_out_vec[i];
                    unsigned v = live_out_vec[j];
                    if (DEEPDEBUG && interference_graph[u].find(v) == interference_graph[u].end()) {
                        std::cerr << "    Edge (Live-Live): %vreg" << u << " <-> %vreg" << v << "\n";
                    }
                    interference_graph[u].insert(v);
                    interference_graph[v].insert(u);
                }
            }

            // 规则3: CALL指令会破坏所有调用者保存(caller-saved)寄存器
            if (instr->getOpcode() == RVOpcodes::CALL) {
                const auto& caller_saved_int = getCallerSavedIntRegs();
                const auto& caller_saved_fp = getCallerSavedFpRegs();

                for (unsigned live_vreg : live_out) {
                    auto [type, size] = getTypeAndSize(live_vreg);
                    if (type == Type::kFloat) {
                        for (PhysicalReg cs_reg : caller_saved_fp) {
                            unsigned cs_vreg_id = preg_to_vreg_id_map.at(cs_reg);
                            if (live_vreg != cs_vreg_id) {
                                interference_graph[live_vreg].insert(cs_vreg_id);
                                interference_graph[cs_vreg_id].insert(live_vreg);
                            }
                        }
                    } else {
                        for (PhysicalReg cs_reg : caller_saved_int) {
                            unsigned cs_vreg_id = preg_to_vreg_id_map.at(cs_reg);
                            if (live_vreg != cs_vreg_id) {
                                interference_graph[live_vreg].insert(cs_vreg_id);
                                interference_graph[cs_vreg_id].insert(live_vreg);
                            }
                        }
                    }
                }
            } // end if CALL
            if (DEEPDEBUG) std::cerr << "  ----------------\n";
        } // end for instr
    } // end for mbb
}

void RISCv64SimpleRegAlloc::colorGraph() {
    // 1. 收集所有需要着色的虚拟寄存器
    std::vector<unsigned> vregs_to_color;
    for (auto const& [vreg, neighbors] : interference_graph) {
        // 只为未预着色的、真正的虚拟寄存器进行着色
        if (color_map.find(vreg) == color_map.end() && vreg < static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID)) {
            vregs_to_color.push_back(vreg);
        }
    }

    // 2. 按冲突度从高到低排序，进行贪心着色
    std::sort(vregs_to_color.begin(), vregs_to_color.end(), [&](unsigned a, unsigned b) {
        return interference_graph.at(a).size() > interference_graph.at(b).size();
    });

    // 3. 遍历并着色
    for (unsigned vreg : vregs_to_color) {
        std::set<PhysicalReg> used_colors;
        // 收集所有邻居的颜色
        for (unsigned neighbor_id : interference_graph.at(vreg)) {
            // A. 邻居是已着色的vreg
            if (color_map.count(neighbor_id)) {
                used_colors.insert(color_map.at(neighbor_id));
            } 
            // B. 邻居是物理寄存器本身
            else if (neighbor_id >= static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID)) {
                PhysicalReg neighbor_preg = static_cast<PhysicalReg>(neighbor_id - static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID));
                used_colors.insert(neighbor_preg);
            }
        }
        
        // 根据vreg类型选择寄存器池
        auto [type, size] = getTypeAndSize(vreg);
        const auto& allocable_regs = (type == Type::kFloat) ? allocable_fp_regs : allocable_int_regs;
        
        bool colored = false;
        for (PhysicalReg preg : allocable_regs) {
            if (used_colors.find(preg) == used_colors.end()) {
                color_map[vreg] = preg;
                colored = true;
                break;
            }
        }
        
        if (!colored) {
            spilled_vregs.insert(vreg);
        }
    }
}

void RISCv64SimpleRegAlloc::rewriteFunction() {
    if (DEBUG) std::cerr << "\n--- Starting Function Rewrite (Spilling & Substitution) ---\n";
    StackFrameInfo& frame_info = MFunc->getFrameInfo();

    // 步骤 1: 为所有溢出的vreg计算唯一的栈偏移量 (此部分逻辑正确，予以保留)
    int current_offset = frame_info.locals_end_offset;
    for (unsigned vreg : spilled_vregs) {
        if (frame_info.spill_offsets.count(vreg)) continue;
        auto [type, size] = getTypeAndSize(vreg);
        current_offset -= size;
        current_offset = current_offset & ~7;
        frame_info.spill_offsets[vreg] = current_offset;
    }
    frame_info.spill_size = -(current_offset - frame_info.locals_end_offset);

    // 步骤 2: 遍历所有指令，对CALL指令做简化处理
    for (auto& mbb : MFunc->getBlocks()) {
        std::vector<std::unique_ptr<MachineInstr>> new_instructions;
        for (auto& instr_ptr : mbb->getInstructions()) {

            if (instr_ptr->getOpcode() != RVOpcodes::CALL) {
                std::vector<PhysicalReg> int_spill_pool = {PhysicalReg::T2, PhysicalReg::T3, PhysicalReg::T4, /*PhysicalReg::T5,*/ PhysicalReg::T6};
                std::vector<PhysicalReg> fp_spill_pool = {PhysicalReg::F0, PhysicalReg::F1, PhysicalReg::F2, PhysicalReg::F3};
                std::map<unsigned, PhysicalReg> vreg_to_preg_map_for_this_instr;
                LiveSet use, def;
                getInstrUseDef(instr_ptr.get(), use, def);
                LiveSet all_vregs_in_instr = use;
                all_vregs_in_instr.insert(def.begin(), def.end());
                for(unsigned vreg : all_vregs_in_instr) {
                    if (spilled_vregs.count(vreg)) {
                        auto [type, size] = getTypeAndSize(vreg);
                        if (type == Type::kFloat) {
                            assert(!fp_spill_pool.empty() && "FP spill pool exhausted for generic instruction!");
                            vreg_to_preg_map_for_this_instr[vreg] = fp_spill_pool.front();
                            fp_spill_pool.erase(fp_spill_pool.begin());
                        } else {
                            assert(!int_spill_pool.empty() && "Int spill pool exhausted for generic instruction!");
                            vreg_to_preg_map_for_this_instr[vreg] = int_spill_pool.front();
                            int_spill_pool.erase(int_spill_pool.begin());
                        }
                    }
                }
                for (unsigned vreg : use) {
                    if (spilled_vregs.count(vreg)) {
                        PhysicalReg target_preg = vreg_to_preg_map_for_this_instr.at(vreg);
                        auto [type, size] = getTypeAndSize(vreg);
                        RVOpcodes load_op = (type == Type::kFloat) ? RVOpcodes::FLW : ((type == Type::kPointer) ? RVOpcodes::LD : RVOpcodes::LW);
                        auto load = std::make_unique<MachineInstr>(load_op);
                        load->addOperand(std::make_unique<RegOperand>(target_preg));
                        load->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(vreg))
                        ));
                        new_instructions.push_back(std::move(load));
                    }
                }
                auto new_instr = std::make_unique<MachineInstr>(instr_ptr->getOpcode());
                for (const auto& op : instr_ptr->getOperands()) {
                    const RegOperand* reg_op = nullptr;
                    if (op->getKind() == MachineOperand::KIND_REG) reg_op = static_cast<const RegOperand*>(op.get());
                    else if (op->getKind() == MachineOperand::KIND_MEM) reg_op = static_cast<const MemOperand*>(op.get())->getBase();
                    if (reg_op) {
                        PhysicalReg final_preg;
                        if (reg_op->isVirtual()) {
                            unsigned vreg = reg_op->getVRegNum();
                            if (spilled_vregs.count(vreg)) {
                                final_preg = vreg_to_preg_map_for_this_instr.at(vreg);
                            } else {
                                assert(color_map.count(vreg));
                                final_preg = color_map.at(vreg);
                            }
                        } else {
                            final_preg = reg_op->getPReg();
                        }
                        auto new_reg_op = std::make_unique<RegOperand>(final_preg);
                        if (op->getKind() == MachineOperand::KIND_REG) {
                            new_instr->addOperand(std::move(new_reg_op));
                        } else {
                            auto mem_op = static_cast<const MemOperand*>(op.get());
                            new_instr->addOperand(std::make_unique<MemOperand>(std::move(new_reg_op), std::make_unique<ImmOperand>(*mem_op->getOffset())));
                        }
                    } else {
                        if(op->getKind() == MachineOperand::KIND_IMM) new_instr->addOperand(std::make_unique<ImmOperand>(*static_cast<const ImmOperand*>(op.get())));
                        else if (op->getKind() == MachineOperand::KIND_LABEL) new_instr->addOperand(std::make_unique<LabelOperand>(*static_cast<const LabelOperand*>(op.get())));
                    }
                }
                new_instructions.push_back(std::move(new_instr));
                for (unsigned vreg : def) {
                    if (spilled_vregs.count(vreg)) {
                        PhysicalReg src_preg = vreg_to_preg_map_for_this_instr.at(vreg);
                        auto [type, size] = getTypeAndSize(vreg);
                        RVOpcodes store_op = (type == Type::kFloat) ? RVOpcodes::FSW : ((type == Type::kPointer) ? RVOpcodes::SD : RVOpcodes::SW);
                        auto store = std::make_unique<MachineInstr>(store_op);
                        store->addOperand(std::make_unique<RegOperand>(src_preg));
                        store->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(vreg))
                        ));
                        new_instructions.push_back(std::move(store));
                    }
                }
            } else {
                // --- 对于CALL指令，只处理其自身和返回值，不再处理参数 ---
                const PhysicalReg INT_TEMP_REG = PhysicalReg::T6;
                const PhysicalReg FP_TEMP_REG = PhysicalReg::F7;

                // 1. 克隆CALL指令本身，只保留标签操作数
                auto new_call = std::make_unique<MachineInstr>(RVOpcodes::CALL);
                for (const auto& op : instr_ptr->getOperands()) {
                    if (op->getKind() == MachineOperand::KIND_LABEL) {
                        new_call->addOperand(std::make_unique<LabelOperand>(*static_cast<const LabelOperand*>(op.get())));
                        // 注意：只添加第一个标签，防止ISel的错误导致多个标签
                        break; 
                    }
                }
                new_instructions.push_back(std::move(new_call));

                // 2. 只处理返回值(def)的溢出和移动
                auto& operands = instr_ptr->getOperands();
                if (!operands.empty() && operands.front()->getKind() == MachineOperand::KIND_REG) {
                    unsigned def_vreg = static_cast<RegOperand*>(operands.front().get())->getVRegNum();
                    auto [type, size] = getTypeAndSize(def_vreg);
                    PhysicalReg result_reg_abi = type == Type::kFloat ? PhysicalReg::F10 : PhysicalReg::A0;

                    if (spilled_vregs.count(def_vreg)) {
                        // 返回值被溢出：a0/fa0 -> temp -> 溢出槽
                        PhysicalReg temp_reg = type == Type::kFloat ? FP_TEMP_REG : INT_TEMP_REG;
                        RVOpcodes store_op = (type == Type::kFloat) ? RVOpcodes::FSW : ((type == Type::kPointer) ? RVOpcodes::SD : RVOpcodes::SW);
                        
                        auto mv_from_abi = std::make_unique<MachineInstr>(type == Type::kFloat ? RVOpcodes::FMV_S : RVOpcodes::MV);
                        mv_from_abi->addOperand(std::make_unique<RegOperand>(temp_reg));
                        mv_from_abi->addOperand(std::make_unique<RegOperand>(result_reg_abi));
                        new_instructions.push_back(std::move(mv_from_abi));

                        auto store = std::make_unique<MachineInstr>(store_op);
                        store->addOperand(std::make_unique<RegOperand>(temp_reg));
                        store->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(frame_info.spill_offsets.at(def_vreg))
                        ));
                        new_instructions.push_back(std::move(store));
                    } else {
                        // 返回值未溢出：a0/fa0 -> 已着色的物理寄存器
                        auto mv_to_dest = std::make_unique<MachineInstr>(type == Type::kFloat ? RVOpcodes::FMV_S : RVOpcodes::MV);
                        mv_to_dest->addOperand(std::make_unique<RegOperand>(color_map.at(def_vreg)));
                        mv_to_dest->addOperand(std::make_unique<RegOperand>(result_reg_abi));
                        new_instructions.push_back(std::move(mv_to_dest));
                    }
                }
            }
        }
        mbb->getInstructions() = std::move(new_instructions);
    }
}

// --- 辅助函数实现 ---

void RISCv64SimpleRegAlloc::getInstrUseDef_Liveness(const MachineInstr* instr, LiveSet& use, LiveSet& def) {
    auto opcode = instr->getOpcode();
    const auto& operands = instr->getOperands();
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    
    auto get_any_reg_id = [&](const MachineOperand* op) -> unsigned {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<const RegOperand*>(op);
            return reg_op->isVirtual() ? reg_op->getVRegNum() : (offset + static_cast<unsigned>(reg_op->getPReg()));
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            auto reg_op = static_cast<const MemOperand*>(op)->getBase();
            return reg_op->isVirtual() ? reg_op->getVRegNum() : (offset + static_cast<unsigned>(reg_op->getPReg()));
        }
        return (unsigned)-1;
    };
    
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
        for (const auto& op : operands) {
            if (op->getKind() == MachineOperand::KIND_MEM) {
                 unsigned reg_id = get_any_reg_id(op.get());
                 if (reg_id != (unsigned)-1) use.insert(reg_id);
            }
        }
    } 
    else if (opcode == RVOpcodes::CALL) {
        if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) {
            def.insert(get_any_reg_id(operands[0].get()));
        }
        for (size_t i = 1; i < operands.size(); ++i) {
            if (operands[i]->getKind() == MachineOperand::KIND_REG) {
                use.insert(get_any_reg_id(operands[i].get()));
            }
        }
        for (auto preg : getCallerSavedIntRegs()) def.insert(offset + static_cast<unsigned>(preg));
        for (auto preg : getCallerSavedFpRegs()) def.insert(offset + static_cast<unsigned>(preg));
        def.insert(offset + static_cast<unsigned>(PhysicalReg::RA));
    }
    else if (opcode == RVOpcodes::RET) {
        use.insert(offset + static_cast<unsigned>(PhysicalReg::A0));
        use.insert(offset + static_cast<unsigned>(PhysicalReg::F10)); // fa0
    }
}

std::pair<Type::Kind, unsigned> RISCv64SimpleRegAlloc::getTypeAndSize(unsigned vreg) {
    const auto& vreg_type_map = ISel->getVRegTypeMap();
    if (vreg_type_map.count(vreg)) {
        Type* type = vreg_type_map.at(vreg);
        if (type->isFloat()) return {Type::kFloat, 4};
        if (type->isPointer()) return {Type::kPointer, 8};
    }
    // 默认或未知类型按32位整数处理
    return {Type::kInt, 4};
}

std::string RISCv64SimpleRegAlloc::regIdToString(unsigned id, const RISCv64AsmPrinter& printer) const {
    const unsigned offset = static_cast<unsigned>(PhysicalReg::PHYS_REG_START_ID);
    if (id >= offset) {
        PhysicalReg reg = static_cast<PhysicalReg>(id - offset);
        return printer.regToString(reg);
    } else {
        return "%vreg" + std::to_string(id);
    }
}

void RISCv64SimpleRegAlloc::printLiveSet(const LiveSet& s, const std::string& name, std::ostream& os, const RISCv64AsmPrinter& printer) {
    os << "    " << name << " (" << s.size() << "): { ";
    for (unsigned vreg : s) {
        os << regIdToString(vreg, printer) << " ";
    }
    os << "}\n";
}

} // namespace sysy