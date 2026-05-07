#include "rv_reg_allocator.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <stack>
#include <utility>
#include <vector>

#include "log.hpp"
#include "rv_basic_block.hpp"
#include "rv_function.hpp"
#include "rv_instruction.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

// 析构函数，释放 live_info_map 中的 LiveInfo 指针
RVRegAllocator::~RVRegAllocator() {
    for (auto &kv : live_info_map) {
        delete kv.second;
    }
    live_info_map.clear();
}

RVFunction *current_function;

void RVRegAllocator::allocate(RVModule &module) {
    // 遍历所有函数，逐个分配寄存器
    for (const auto &kv : module.get_functions()) {
        RVFunction *func = kv.second;
        if (!func->get_blocks().empty()) {
            allocate_function(func);
        }
    }
}

void RVRegAllocator::allocate_function(RVFunction *func) {
    // logger::INFO(func->get_name() + "::allocate_function");
    current_function = func;
    phase = Phase::NONE;
    while (true) {
        if (phase == Phase::NONE) {
            phase = Phase::FLOAT;
        } else if (phase == Phase::FLOAT) {
            phase = Phase::INT;
        } else {
            break;
        }

        // ---- 插入保护指令逻辑 ----
        if (func->get_name() != "@main") {
            // logger::INFO(func->get_name());
            // 获取第一个基本块的指令 vector
            auto *first_block = func->get_blocks().front();
            auto &first_insts = first_block->get_insts();
            RVInstruction *head_inst = first_insts.front();
            // 收集所有返回块的 RVJr 指令插入点
            std::vector<RVInstruction *> tail_nodes;
            for (auto *block : func->get_blocks()) {
                auto &insts = block->get_insts();
                if (!insts.empty()) {
                    // 逆序查找 RVJr
                    for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
                        // logger::INFO((*it)->to_string());
                        if (dynamic_cast<RVJr *>(*it)) {
                            // 记录 RVJr 指令本身
                            tail_nodes.push_back(*it);
                        }
                    }
                }
            }

            gen_protection_move(head_inst, tail_nodes, func);
        }

        // 清空溢出内存指令记录
        spilled_memory_instructions.clear();

        bool has_tail_call = true;
        while (has_tail_call) {
            initialize(func);
            has_tail_call = false;
            build_graph(func);
            make_worklist(func);

            main_loop();
            assign_color();

            if (!spilled_nodes.empty()) {
                // logger::INFO("Spilling ", spilled_nodes.size(), " nodes, rewriting program");
                // for (auto *reg : spilled_nodes) {
                //     logger::INFO(reg->to_string());
                // }
                rewrite_program(func);
                has_tail_call = true;
            } else {
                // logger::INFO("no Spilling");
            }
        }

        // 替换虚拟寄存器为物理寄存器
        // 只对虚拟寄存器分配和替换颜色，物理寄存器直接跳过
        for (auto *block : func->get_blocks()) {
            auto &insts = block->get_insts();
            for (auto *inst : insts) {
                // logger::INFO(inst->to_string());
                std::map<RVReg *, RVReg *> replace_map;
                for (auto *operand : inst->get_operands()) {
                    auto *vir_op = dynamic_cast<RVVirReg *>(operand);
                    if (vir_op && color.find(vir_op) != color.end()) {
                        bool is_current_type = false;
                        // logger::INFO("ok4");
                        if (phase == Phase::INT && vir_op->get_reg_type() == RVVirReg::RegType::INT_TYPE) {
                            is_current_type = true;
                        } else if (phase == Phase::FLOAT && vir_op->get_reg_type() == RVVirReg::RegType::FLOAT_TYPE) {
                            is_current_type = true;
                        }
                        if (is_current_type) {
                            if (vir_op->get_reg_type() == RVVirReg::RegType::INT_TYPE && phase == Phase::INT) {
                                // logger::INFO("ok1");
                                auto *phy_reg = reg_info::get_cpu_reg(color[vir_op]);
                                replace_map[vir_op] = phy_reg;
                            } else if (vir_op->get_reg_type() == RVVirReg::RegType::FLOAT_TYPE &&
                                       phase == Phase::FLOAT) {
                                // logger::INFO("ok5");
                                auto *phy_reg = reg_info::get_fpu_reg(color[vir_op]);
                                replace_map[vir_op] = phy_reg;
                            }
                        }
                    }
                }
                // 执行替换
                for (const auto &kv : replace_map) {
                    // logger::INFO("ok6");
                    inst->replace_regs(kv.first, kv.second);
                }
            }
        }
        // logger::INFO("888");
    }
}

void RVRegAllocator::initialize(RVFunction *func) {
    // logger::INFO(func->get_name() + "::initialize");
    // 先进行活跃信息分析，填充live_info_map
    func->liveness_analysis();
    for (auto &kv : live_info_map) {
        delete kv.second;
    }
    live_info_map.clear();
    for (auto *block : func->get_blocks()) {
        auto *info = new BlockLiveInfo();
        // 拷贝BlockLiveInfo到LiveInfo
        const auto &blk_info = block->get_live_info();
        info->live_use.insert(blk_info.live_use.begin(), blk_info.live_use.end());
        info->live_def.insert(blk_info.live_def.begin(), blk_info.live_def.end());
        info->live_in.insert(blk_info.live_in.begin(), blk_info.live_in.end());
        info->live_out.insert(blk_info.live_out.begin(), blk_info.live_out.end());
        live_info_map[block] = info;
    }
    // 清空所有数据结构
    simplify_worklist.clear();
    freeze_worklist.clear();
    spill_worklist.clear();
    spilled_nodes.clear();
    coalesced_nodes.clear();
    colored_nodes.clear();

    while (!select_stack.empty()) {
        select_stack.pop();
    }

    coalesced_moves.clear();
    constrained_moves.clear();
    frozen_moves.clear();
    worklist_moves.clear();
    active_moves.clear();

    adj_set.clear();
    adj_list.clear();
    degree.clear();
    move_list.clear();
    alias.clear();
    color.clear();

    // 初始化预着色寄存器
    auto cpu_regs = reg_info::get_all_cpu_regs();
    auto fpu_regs = reg_info::get_all_fpu_regs();

    for (auto *reg : cpu_regs) {
        colored_nodes.insert(reg);
        color[reg] = reg->get_phys_id();
        degree[reg] = 0x7fffffff;  // INF
    }

    for (auto *reg : fpu_regs) {
        colored_nodes.insert(reg);
        color[reg] = reg->get_phys_id();
        degree[reg] = 0x7fffffff;  // INF
    }
}

void RVRegAllocator::gen_protection_move(RVInstruction *head_inst,
                                         const std::vector<RVInstruction *> &tail_nodes,
                                         RVFunction *func) {
    // logger::INFO("RVRegAllocator::gen_protection_move");
    // 需要保护的寄存器列表：s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11
    std::vector<int> protected_regs = {8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
    for (int index : protected_regs) {
        if (phase == Phase::INT) {
            // 整数寄存器保护
            auto *phy_reg = reg_info::get_cpu_reg(index);
            auto *vir_reg = RVVirReg::create(func->get_next_vreg_index(), RVVirReg::RegType::INT_TYPE, func);
            // logger::INFO(vir_reg->to_string());
            auto *mv_inst = RVMv::create(vir_reg, phy_reg);
            head_inst->insert_inst_before_self(mv_inst);
            for (auto *jr_inst : tail_nodes) {
                auto *mv_inst2 = RVMv::create(phy_reg, vir_reg);
                jr_inst->insert_inst_before_self(mv_inst2);
            }
        } else {
            // 浮点寄存器保护
            auto *phy_reg = reg_info::get_fpu_reg(index);
            auto *vir_reg = RVVirReg::create(func->get_next_vreg_index(), RVVirReg::RegType::FLOAT_TYPE, func);
            // logger::INFO(vir_reg->to_string());
            auto *fmv_inst = RVFmv_s::create(vir_reg, phy_reg);
            head_inst->insert_inst_before_self(fmv_inst);
            for (auto *jr_inst : tail_nodes) {
                auto *fmv_inst2 = RVFmv_s::create(phy_reg, vir_reg);
                jr_inst->insert_inst_before_self(fmv_inst2);
            }
        }
    }
}

void RVRegAllocator::build_graph(RVFunction *func) {
    // logger::INFO(func->get_name() + "::Building graph");
    // 遍历所有基本块
    for (auto *block : func->get_blocks()) {
        // 获取该块的 live_out
        auto live_out = live_info_map[block]->live_out;

        // 逆序遍历指令
        const auto &insts = block->get_insts();
        for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
            const auto &inst = *it;

            // logger::INFO(inst->to_string());

            // 处理 move 指令
            if (dynamic_cast<RVMv *>(inst) || dynamic_cast<RVFmv_s *>(inst)) {
                auto *dst = inst->get_def();
                auto *src = inst->get_uses()[0];

                if (src && dst) {
                    live_out.erase(src);

                    if (move_list.find(src) == move_list.end()) {
                        move_list[src] = std::set<RVInstruction *>();
                    }
                    if (move_list.find(dst) == move_list.end()) {
                        move_list[dst] = std::set<RVInstruction *>();
                    }

                    move_list[src].insert(inst);
                    move_list[dst].insert(inst);
                    worklist_moves.insert(inst);
                }
            }

            // 处理定义寄存器
            if (auto *const def = inst->get_def()) {
                std::vector<RVReg *> def_regs = {def};

                // 如果是调用指令，添加调用约定中的定义寄存器
                if (dynamic_cast<RVCall *>(inst)) {
                    auto call_defs = get_call_defs();
                    for (auto *call_def : call_defs) {
                        def_regs.push_back(call_def);
                    }
                    // def_regs.insert(def_regs.end(), call_defs.begin(), call_defs.end());
                }

                // 添加所有定义寄存器到 live_out
                for (auto *def_reg : def_regs) {
                    live_out.insert(def_reg);
                }

                // 为每个定义寄存器添加冲突边
                for (auto *def_reg : def_regs) {
                    for (auto *live_reg : live_out) {
                        add_edge(live_reg, def_reg);
                    }
                }

                // 从 live_out 中移除定义寄存器
                for (auto *def_reg : def_regs) {
                    live_out.erase(def_reg);
                }
            }

            // 处理使用寄存器
            for (auto *use : inst->get_uses()) {
                live_out.insert(use);
            }
        }
    }

    // for (auto& kv : adj_list) {
    //     logger::INFO("build_grapg start print");
    //     std::string s = kv.first->to_string() + ": ";
    //     for (auto* n : kv.second) s += n->to_string() + " ";
    //     logger::INFO("adj_list[" + s + "]");
    // }
}

void RVRegAllocator::make_worklist(RVFunction *func) {
    // logger::INFO(func->get_name() + "::make_worklist");
    // 首先处理 move 指令，检查源和目标是否冲突
    for (auto *block : func->get_blocks()) {
        const auto &insts = block->get_insts();
        for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
            const auto &inst = *it;
            if (dynamic_cast<RVMv *>(inst) || dynamic_cast<RVFmv_s *>(inst)) {
                auto *const def = inst->get_def();
                auto *src = inst->get_uses()[0];

                if (def && src) {
                    if (adj_list.find(def) == adj_list.end()) {
                        adj_list[def] = OperandSet();  // 先插入空集合
                    }
                    if (adj_list[def].find(src) != adj_list[def].end()) {
                        worklist_moves.erase(inst);
                        constrained_moves.insert(inst);
                    }
                }
            }
        }
    }

    // 分类所有虚拟寄存器，直接遍历集合
    for (auto *vir_reg : func->get_all_vir_reg_used()) {
        bool is_current_type = false;
        if (phase == Phase::INT && vir_reg->get_reg_type() == RVVirReg::RegType::INT_TYPE) {
            is_current_type = true;
        } else if (phase == Phase::FLOAT && vir_reg->get_reg_type() == RVVirReg::RegType::FLOAT_TYPE) {
            is_current_type = true;
        }
        if (is_current_type) {
            if (degree.find(vir_reg) != degree.end() && degree[vir_reg] >= K) {
                spill_worklist.insert(vir_reg);
            } else if (move_related(vir_reg)) {
                freeze_worklist.insert(vir_reg);
            } else {
                simplify_worklist.insert(vir_reg);
            }
        }
    }
}

void RVRegAllocator::main_loop() {
    while (!simplify_worklist.empty() || !worklist_moves.empty() || !freeze_worklist.empty() ||
           !spill_worklist.empty()) {
        if (!simplify_worklist.empty()) {
            // logger::INFO("simplify_worklist");
            simplify();
        } else if (!worklist_moves.empty()) {
            // logger::INFO("worklist_moves");
            coalesce();
        } else if (!freeze_worklist.empty()) {
            // logger::INFO("freeze_worklist");
            freeze();
        } else if (!spill_worklist.empty()) {
            // logger::INFO("spill_worklist");
            select_spill();
        }
    }
}

void RVRegAllocator::simplify() {
    // 弹出一个节点，压入 select_stack
    auto *n = *simplify_worklist.begin();
    simplify_worklist.erase(n);
    select_stack.push(n);
    // 对所有邻接节点调用 decrement_degree
    for (auto *m : adjacent(n)) {
        decrement_degree(m);
    }
}

void RVRegAllocator::coalesce() {
    auto *m = *worklist_moves.begin();
    // logger::INFO(m->to_string());
    worklist_moves.erase(m);

    auto *x = get_alias(m->get_def());
    auto *y = get_alias(m->get_uses()[0]);

    RVReg *u, *v;
    if (y->is_precolored()) {
        u = y;
        v = x;
    } else {
        u = x;
        v = y;
    }

    if (u == v) {
        coalesced_moves.insert(m);
        add_worklist(u);
    } else if (v->is_precolored() || adj_set.find({u, v}) != adj_set.end()) {
        constrained_moves.insert(m);
        add_worklist(u);
        add_worklist(v);
    } else {
        bool flag = true;
        if (u->is_precolored()) {
            for (auto *t : adjacent(v)) {
                flag = flag && ok(t, u);
            }
        } else {
            OperandSet computed_nodes;
            auto adj_u = adjacent(u);
            auto adj_v = adjacent(v);
            for (auto &it : adj_u) computed_nodes.insert(it);
            for (auto &it : adj_v) computed_nodes.insert(it);
            flag = conservative(computed_nodes);
        }

        if (flag) {
            coalesced_moves.insert(m);
            combine(u, v);
            add_worklist(u);
        } else {
            active_moves.insert(m);
        }
    }
}

void RVRegAllocator::freeze() {
    auto *u = *freeze_worklist.begin();
    freeze_worklist.erase(u);
    simplify_worklist.insert(u);
    freeze_moves(u);
}

void RVRegAllocator::select_spill() {
    // 计算所有等待溢出寄存器的溢出代价
    build_spill_cost(spill_worklist);

    // 获取溢出代价最小的寄存器
    RVReg *m = get_min_cost_spill_reg();

    if (!m && !spill_worklist.empty()) {
        // 如果没有计算出的溢出代价，使用原来的方法
        m = *spill_worklist.begin();
    }

    if (m) {
        spill_worklist.erase(m);
        simplify_worklist.insert(m);
        freeze_moves(m);
    }
}

void RVRegAllocator::assign_color() {
    // logger::INFO("assign_color");
    while (!select_stack.empty()) {
        auto *n = select_stack.top();
        select_stack.pop();

        auto ok_colors = init_ok_colors();

        if (adj_list.find(n) == adj_list.end()) {
            adj_list[n] = OperandSet();
        }

        for (auto *w : adj_list[n]) {
            auto *alias_w = get_alias(w);
            if (alias_w->is_precolored() || colored_nodes.find(alias_w) != colored_nodes.end()) {
                ok_colors.remove(color[alias_w]);
            }
        }

        if (ok_colors.empty()) {
            spilled_nodes.insert(n);
            // logger::INFO("spilled_nodes.insert: ", n->to_string());
        } else {
            colored_nodes.insert(n);
            auto c = *ok_colors.begin();
            color[n] = c;

            // logger::INFO(n->to_string() + " -- " + std::to_string(c));
        }
    }

    for (auto *n : coalesced_nodes) {
        color[n] = color[get_alias(n)];
    }

    // logger::INFO("assign_color start print");
    // for (auto x : color) {
    //     logger::INFO("color[" + x.first->to_string() + "] = " + std::to_string(x.second));
    // }
}

void RVRegAllocator::rewrite_program(RVFunction *func) {
    if (spilled_nodes.empty()) return;

    for (auto spilled_node : spilled_nodes) {
        auto vir_reg = dynamic_cast<RVVirReg *>(spilled_node);
        if (!vir_reg) continue;

        // 为溢出寄存器分配栈空间
        func->allocate_stack_space(vir_reg);

        // 获取所有使用该寄存器的指令
        auto users = spilled_node->get_graph_uses();

        for (auto *user : users) {
            // 处理定义寄存器
            if (user->get_def() && user->get_def() == spilled_node) {
                auto *new_def_reg = RVVirReg::create(func->get_next_vreg_index(), vir_reg->get_reg_type(), func);

                func->remap_virtual_register(new_def_reg, vir_reg);
                int offset = func->get_stack_offset(new_def_reg);

                // assert(offset % 8 == 0);
                if (offset > -2048 && offset <= 2048) {
                    // 直接使用偏移量
                    RVInstruction *store_inst;
                    if (vir_reg->get_reg_type() == RVVirReg::RegType::FLOAT_TYPE) {
                        store_inst = RVFsd::create(reg_info::get_sp(), new_def_reg, RVImmediate::create(-offset));
                    } else {
                        store_inst = RVSd::create(reg_info::get_sp(), new_def_reg, RVImmediate::create(-offset));
                    }

                    // 记录这个溢出相关的store指令
                    spilled_memory_instructions.insert(store_inst);

                    // logger::INFO(store_inst->to_string());
                    // 插入 store 指令
                    user->insert_inst_after_self(store_inst);
                    user->replace_def(new_def_reg);
                    func->replace_virtual_register(new_def_reg, vir_reg);
                } else {
                    // 需要辅助寄存器计算地址
                    auto *assist_reg = RVVirReg::create(func->get_next_vreg_index(), RVVirReg::RegType::INT_TYPE, func);

                    // 计算地址：assist_reg = sp + offset
                    auto *li_inst = RVLi::create(assist_reg, RVImmediate::create(-offset));
                    auto *add_inst = RVAdd::create(assist_reg, reg_info::get_sp(), assist_reg);

                    // 存储：store new_def_reg, 0(assist_reg)
                    RVInstruction *store_inst;
                    if (vir_reg->get_reg_type() == RVVirReg::RegType::FLOAT_TYPE) {
                        store_inst = RVFsd::create(assist_reg, new_def_reg, RVImmediate::create(0));
                    } else {
                        store_inst = RVSd::create(assist_reg, new_def_reg, RVImmediate::create(0));
                    }

                    // 记录这些溢出相关的指令
                    spilled_memory_instructions.insert(li_inst);
                    spilled_memory_instructions.insert(add_inst);
                    spilled_memory_instructions.insert(store_inst);

                    // logger::INFO(store_inst->to_string());
                    // 插入指令并维护use-def链, 由于逐条插入, 要倒序插入
                    user->insert_inst_after_self(store_inst);
                    user->insert_inst_after_self(add_inst);
                    user->insert_inst_after_self(li_inst);
                    user->replace_def(new_def_reg);
                    func->replace_virtual_register(new_def_reg, vir_reg);
                }
            }

            for (auto *use : user->get_uses()) {
                if (use == spilled_node) {
                    auto *new_def_reg = RVVirReg::create(func->get_next_vreg_index(), vir_reg->get_reg_type(), func);

                    func->remap_virtual_register(new_def_reg, vir_reg);
                    int offset = func->get_stack_offset(new_def_reg);

                    // assert(offset % 8 == 0);
                    if (vir_reg->get_reg_type() == RVVirReg::RegType::INT_TYPE) {
                        if (offset > -2048 && offset <= 2048) {
                            // 直接加载
                            auto *load_inst =
                                RVLd::create(new_def_reg, reg_info::get_sp(), RVImmediate::create(-offset));

                            // 记录这个溢出相关的load指令
                            spilled_memory_instructions.insert(load_inst);

                            // logger::INFO(load_inst->to_string());
                            user->insert_inst_before_self(load_inst);
                            user->replace_uses(spilled_node, new_def_reg);
                        } else {
                            auto *li_inst = RVLi::create(new_def_reg, RVImmediate::create(-offset));
                            auto *add_inst = RVAdd::create(new_def_reg, reg_info::get_sp(), new_def_reg);
                            auto *load_inst = RVLd::create(new_def_reg, new_def_reg, RVImmediate::create(0));

                            // 记录这些溢出相关的指令
                            spilled_memory_instructions.insert(li_inst);
                            spilled_memory_instructions.insert(add_inst);
                            spilled_memory_instructions.insert(load_inst);

                            // logger::INFO(load_inst->to_string());
                            user->insert_inst_before_self(li_inst);
                            user->insert_inst_before_self(add_inst);
                            user->insert_inst_before_self(load_inst);
                            user->replace_uses(spilled_node, new_def_reg);
                        }
                    } else {
                        if (offset > -2048 && offset <= 2048) {
                            // 直接加载浮点
                            auto *load_inst =
                                RVFld::create(new_def_reg, reg_info::get_sp(), RVImmediate::create(-offset));

                            // 记录这个溢出相关的load指令
                            spilled_memory_instructions.insert(load_inst);

                            // logger::INFO(load_inst->to_string());
                            user->insert_inst_before_self(load_inst);
                            user->replace_uses(spilled_node, new_def_reg);
                        } else {
                            // 需要辅助寄存器
                            auto *assist_reg =
                                RVVirReg::create(func->get_next_vreg_index(), RVVirReg::RegType::INT_TYPE, func);

                            auto *li_inst = RVLi::create(assist_reg, RVImmediate::create(-offset));
                            auto *add_inst = RVAdd::create(assist_reg, reg_info::get_sp(), assist_reg);
                            auto *load_inst = RVFld::create(new_def_reg, assist_reg, RVImmediate::create(0));

                            // 记录这些溢出相关的指令
                            spilled_memory_instructions.insert(li_inst);
                            spilled_memory_instructions.insert(add_inst);
                            spilled_memory_instructions.insert(load_inst);

                            // logger::INFO(load_inst->to_string());
                            user->insert_inst_before_self(li_inst);
                            user->insert_inst_before_self(add_inst);
                            user->insert_inst_before_self(load_inst);
                            user->replace_uses(spilled_node, new_def_reg);
                        }
                    }
                    user->replace_uses(spilled_node, new_def_reg);  // 这里为什么还要再替换一次
                }
            }
        }
        // 清空溢出节点的use链，防止重复处理
        spilled_node->clear_uses();
    }
}

void RVRegAllocator::decrement_degree(RVReg *m) {
    degree[m]--;
    if (degree[m] == K - 1) {
        adjacent(m).insert(m);
        spill_worklist.erase(m);
        if (move_related(m)) {
            freeze_worklist.insert(m);
        } else {
            simplify_worklist.insert(m);
        }
    }
}

bool RVRegAllocator::move_related(RVReg *n) { return !node_moves(n).empty(); }

std::set<RVInstruction *> RVRegAllocator::node_moves(RVReg *n) {
    std::set<RVInstruction *> result;
    if (move_list.find(n) != move_list.end()) {
        for (auto *x : move_list[n]) {
            result.insert(x);
        }
    }

    std::set<RVInstruction *> tmp_set;
    for (auto *x : active_moves) {
        tmp_set.insert(x);
    }
    for (auto &worklist_move : worklist_moves) tmp_set.insert(worklist_move);

    std::set<RVInstruction *> intersection;
    for (auto *m : result) {
        if (tmp_set.find(m) != tmp_set.end()) {
            intersection.insert(m);
        }
    }
    return intersection;
}

RVRegAllocator::OperandSet RVRegAllocator::adjacent(RVReg *n) {
    OperandSet result;
    if (adj_list.find(n) != adj_list.end()) {
        for (auto *x : adj_list[n]) {
            result.insert(x);
        }
    }

    std::stack<RVReg *> tmp_stack = select_stack;
    while (!tmp_stack.empty()) {
        // logger::INFO(select_stack.size());
        result.erase(tmp_stack.top());
        tmp_stack.pop();
    }

    for (auto *x : coalesced_nodes) {
        result.erase(x);
    }
    return result;
}

RVReg *RVRegAllocator::get_alias(RVReg *n) {
    if (coalesced_nodes.find(n) != coalesced_nodes.end()) {
        return get_alias(alias[n]);
    } else {
        return n;
    }
}

void RVRegAllocator::add_worklist(RVReg *u) {
    if (!u->is_precolored() && !move_related(u) && (degree.find(u) != degree.end() ? degree[u] : 0) < K) {
        freeze_worklist.erase(u);
        simplify_worklist.insert(u);
    }
}

bool RVRegAllocator::ok(RVReg *t, RVReg *r) {
    return (degree.find(t) != degree.end() ? degree[t] : 0) < K || t->is_precolored() ||
           adj_set.find({t, r}) != adj_set.end();
}

bool RVRegAllocator::conservative(const OperandSet &nodes) {
    int k = 0;
    for (auto *n : nodes) {
        if (degree.find(n) != degree.end() && degree[n] >= K) {
            k++;
        }
        if (k == K) {
            return false;
        }
    }
    return true;
}

void RVRegAllocator::combine(RVReg *u, RVReg *v) {
    if (freeze_worklist.find(v) != freeze_worklist.end()) {
        freeze_worklist.erase(v);
    } else {
        spill_worklist.erase(v);
    }

    coalesced_nodes.insert(v);
    alias[v] = u;

    if (move_list.find(u) != move_list.end() && move_list.find(v) != move_list.end()) {
        for (auto it = move_list[v].begin(); it != move_list[v].end(); ++it) move_list[u].insert(*it);
    }

    OperandSet v_nodes;
    v_nodes.insert(v);
    enable_moves(v_nodes);

    if (adj_list.find(v) != adj_list.end()) {
        for (auto *t : adj_list[v]) {
            add_edge(t, u);
            decrement_degree(t);
        }
    }

    if (degree.find(u) != degree.end() && degree[u] >= K && freeze_worklist.find(u) != freeze_worklist.end()) {
        freeze_worklist.erase(u);
        spill_worklist.insert(u);
    }
}

void RVRegAllocator::freeze_moves(RVReg *u) {
    auto moves = node_moves(u);
    for (auto *m : moves) {
        auto *x = m->get_def();
        auto *y = dynamic_cast<RVReg *>(m->get_operands()[1]);
        RVReg *v;

        if (get_alias(y) == get_alias(u)) {
            v = get_alias(x);
        } else {
            v = get_alias(y);
        }

        active_moves.erase(m);
        frozen_moves.insert(m);

        if (node_moves(v).empty() && (degree.find(v) != degree.end() ? degree[v] : 0) < K) {
            freeze_worklist.erase(v);
            simplify_worklist.insert(v);
        }
    }
}

double RVRegAllocator::get_prio_for_spill(RVReg *r) {
    // TODO: 实现更复杂的启发式算法，考虑循环因子等
    return 1.0;  // 简单实现，后续可优化
}

std::list<int> RVRegAllocator::init_ok_colors() const {
    std::list<int> ok_colors;
    if (phase == Phase::INT) {
        // 整数寄存器: 5-31 (跳过zero,ra,sp,gp,tp)
        for (int i = 5; i <= 31; i++) {
            ok_colors.push_back(i);
        }
    } else {
        // 浮点寄存器: 0-31
        for (int i = 0; i <= 31; i++) {
            ok_colors.push_back(i);
        }
    }

    // for (auto x : ok_colors) {
    //     logger::INFO("ok_colors: ", x);
    // }

    return ok_colors;
}

void RVRegAllocator::enable_moves(const OperandSet &nodes) {
    for (auto *n : nodes) {
        auto moves = node_moves(n);
        for (auto *m : moves) {
            if (active_moves.find(m) != active_moves.end()) {
                active_moves.erase(m);
                worklist_moves.insert(m);
            }
        }
    }
}

void RVRegAllocator::add_edge(RVReg *u, RVReg *v) {
    // 确保u v是同一类型的寄存器（float或者int）
    bool u_is_int_type = false, v_is_int_type = false;

    // u
    if (auto *u_reg = dynamic_cast<RVPhyReg *>(u)) {
        u_is_int_type = (u_reg->get_type() == RVPhyReg::Type::INT);
    } else if (auto *u_vreg = dynamic_cast<RVVirReg *>(u)) {
        u_is_int_type = (u_vreg->get_reg_type() == RVVirReg::RegType::INT_TYPE);
    }
    // v
    if (auto *v_reg = dynamic_cast<RVPhyReg *>(v)) {
        v_is_int_type = (v_reg->get_type() == RVPhyReg::Type::INT);
    } else if (auto *v_vreg = dynamic_cast<RVVirReg *>(v)) {
        v_is_int_type = (v_vreg->get_reg_type() == RVVirReg::RegType::INT_TYPE);
    }

    bool is_same_type = u_is_int_type == v_is_int_type;

    if (is_same_type && adj_set.find({u, v}) == adj_set.end() && u != v) {
        if (!((u_is_int_type && phase == Phase::INT) || (!u_is_int_type && phase == Phase::FLOAT))) {
            return;
        }

        adj_set.insert({u, v});
        adj_set.insert({v, u});

        if (!u->is_precolored()) {
            if (adj_list.find(u) == adj_list.end()) {
                adj_list[u] = OperandSet();
            }
            adj_list[u].insert(v);
            if (degree.find(u) == degree.end()) {
                degree[u] = 1;
            } else {
                degree[u]++;
            }
        }

        if (!v->is_precolored()) {
            if (adj_list.find(v) == adj_list.end()) {
                adj_list[v] = OperandSet();
            }
            adj_list[v].insert(u);
            if (degree.find(v) == degree.end()) {
                degree[v] = 1;
            } else {
                degree[v]++;
            }
        }
    }
}

std::vector<RVPhyReg *> RVRegAllocator::get_call_defs() const {
    std::vector<RVPhyReg *> result;

    if (phase == Phase::INT) {
        // 整数寄存器：ra, t0-t6, a0-a7, s0-s11, t3-t6
        std::vector<int> int_regs = {1, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};
        std::vector<int> float_regs = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};

        for (int i : float_regs) {
            result.push_back(reg_info::get_fpu_reg(i));
        }
        for (int i : int_regs) {
            result.push_back(reg_info::get_cpu_reg(i));
        }
    } else {
        // 浮点寄存器：f0-f7, f10-f17, f28-f31
        std::vector<int> float_regs = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};

        for (int i : float_regs) {
            result.push_back(reg_info::get_fpu_reg(i));
        }
    }

    return result;
}

// 计算寄存器溢出代价相关方法实现

void RVRegAllocator::build_spill_cost(const OperandSet &regs) {
    reg_spill_cost_map.clear();
    for (auto *reg : regs) {
        calculate_reg_cost(reg);
    }
}

std::vector<RVReg *> RVRegAllocator::get_spill_array() {
    std::vector<RVReg *> regs;
    for (const auto &kv : reg_spill_cost_map) {
        regs.push_back(kv.first);
    }

    // 按溢出代价从低到高排序
    std::sort(
        regs.begin(), regs.end(), [this](RVReg *a, RVReg *b) { return reg_spill_cost_map[a] < reg_spill_cost_map[b]; });

    // 如果寄存器数量超过20个，只返回前一半（代价最低的）
    if (regs.size() > 20) {
        regs.resize(regs.size() / 2);
    }

    // logger::INFO("spill_array");
    // for (auto *reg : regs) {
    //     logger::INFO(reg->to_string());
    // }
    return regs;
}

RVReg *RVRegAllocator::get_min_cost_spill_reg() {
    if (reg_spill_cost_map.empty()) {
        return nullptr;
    }

    // 找到溢出代价最小的寄存器
    RVReg *min_cost_reg = nullptr;
    int min_cost = std::numeric_limits<int>::max();

    for (const auto &kv : reg_spill_cost_map) {
        if (kv.second < min_cost) {
            min_cost = kv.second;
            min_cost_reg = kv.first;
        }
    }

    return min_cost_reg;
}

void RVRegAllocator::calculate_reg_cost(RVReg *reg) {
    int cost = 0;

    // 获取寄存器的所有使用点
    auto uses = reg->get_graph_uses();

    for (auto *use : uses) {
        // 获取循环深度
        int loop_depth = use->get_parent_block()->get_loop_depth() + 1;

        // 检查是否是加载/存储指令
        if (use->is_memory_ins()) {
            // 检查该寄存器是否是该指令的val寄存器（load的目标或store的源）
            if (use->get_memory_val() == reg) {
                // 检查指令是否已经溢出（通过检查是否在spilled_memory_instructions中）
                bool is_already_spilled = (spilled_memory_instructions.find(use) != spilled_memory_instructions.end());

                if (is_already_spilled) {
                    // 已经溢出的指令，代价极高
                    cost += 5 * 2000 * loop_depth;
                } else {
                    // 普通的内存访问指令
                    cost += 5 * 100 * loop_depth;
                }
            } else {
                // 基址寄存器，代价较低
                cost += 5 * 50 * loop_depth;
            }
        } else {
            // 非内存访问指令
            cost += 1 * 100 * loop_depth;
        }
    }

    // 如果是预着色寄存器，增加额外代价
    if (reg->is_precolored()) {
        cost += 50;
    }

    reg_spill_cost_map[reg] = cost;
}

}  // namespace backend::riscv
