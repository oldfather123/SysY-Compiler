#include "RegisterAllocation.hpp"
#include "LivenessAnalysis.hpp"
#include "MachineBasicBlock.hpp"
#include "MachineFunction.hpp"
#include "Operand.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>

size_t RegisterAllocation::type_k(Register::RegisterType type) {
    switch (type) {
    case Register::RegisterType::General:
        return general_K;
    case Register::RegisterType::Float:
        return float_K;
    }
    ASSERT(false && "Unknown register type");
    return 0;
}
void RegisterAllocation::run_on_func(std::shared_ptr<MachineFunction> func) {
    this->func = func;
    initial.clear();
    coalesced_nodes.clear();
    colored_nodes.clear();
    spilled_nodes.clear();
    edges.clear();

    bool is_spill = false;
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &instr : bb->get_instrs()) {
            for (auto &def : instr->get_def()) {
                if (def->is_virtual_reg())
                    initial.insert(
                        std::dynamic_pointer_cast<VirtualRegister>(def));
                degree[def] = 0;
                move_list[def] = MoveSet();
                graph[def] = RegisterSet();
                alias[def] = def;
            }
            for (auto &use : instr->get_use()) {
                if (use->is_virtual_reg())
                    initial.insert(
                        std::dynamic_pointer_cast<VirtualRegister>(use));
                degree[use] = 0;
                move_list[use] = MoveSet();
                graph[use] = RegisterSet();
                alias[use] = use;
            }
        }
    }
    for (auto &reg : initial) {
        degree[reg] = 0;
        move_list[reg] = MoveSet();
        graph[reg] = RegisterSet();
        alias[reg] = reg;
    }
    do {
        edges.clear();
        for (auto &reg : initial) {
            degree[reg] = 0;
            move_list[reg] = MoveSet();
            graph[reg] = RegisterSet();
            alias[reg] = reg;
        }
        liveness_analysis->run_on_func(func);
        build_graph();
        make_worklist();
        do {
            if (simplify_worklist.size()) {
                simplify();
            } else if (worklist_moves.size()) {
                coalesce();
            } else if (freeze_worklist.size()) {
                freeze();
            } else if (spill_worklist.size()) {
                select_spill();
            }
        } while (simplify_worklist.size() || worklist_moves.size() ||
                 freeze_worklist.size() || spill_worklist.size());

        assign_colors();
        if (spilled_nodes.size()) {
            rewrite_program();
            is_spill = true;
        } else
            is_spill = false;
    } while (is_spill);

    for (auto &bb : func->get_basic_blocks()) {
        for (auto &instr : bb->get_instrs()) {
            instr->colorize();
        }
    }
}

void RegisterAllocation::run() {
    liveness_analysis = std::make_shared<LivenessAnalysis>(module);
    for (auto &func : module->get_functions()) {
        run_on_func(func);
    }
}
void RegisterAllocation::add_edge(std::shared_ptr<Register> u,
                                  std::shared_ptr<Register> v) {
    if (u->get_type() != v->get_type())
        return;
    if (u == v)
        return;
    if (edges.find(std::make_pair(u, v)) != edges.end())
        return;
    edges.insert(std::make_pair(u, v));
    edges.insert(std::make_pair(v, u));
    if (!u->is_physical_reg()) {
        graph[u].insert(v);
        degree[u]++;
    }
    if (!v->is_physical_reg()) {
        graph[v].insert(u);
        degree[v]++;
    }
}

void RegisterAllocation::build_graph() {
    for (auto &block : func->get_basic_blocks()) {
        auto live = liveness_analysis->get_live_out(block);
        for (auto it = block->get_instrs().rbegin();
             it != block->get_instrs().rend(); ++it) {
            auto instr = *it;
            if (instr->get_tag() == MachineInstr::Tag::MOV) {
                auto def =
                    std::dynamic_pointer_cast<Register>(instr->get_operand(0));
                auto use =
                    std::dynamic_pointer_cast<Register>(instr->get_operand(1));
                live.erase(use);
                move_list[def].insert(instr);
                move_list[use].insert(instr);
                worklist_moves.insert(instr);
            }
            auto defs = instr->get_def();
            live.insert(defs.begin(), defs.end());
            for (auto &reg : instr->get_def()) {
                for (auto &l : live) {
                    add_edge(reg, l);
                }
            }

            for (auto &reg : instr->get_def()) {
                live.erase(reg);
            }
            auto uses = instr->get_use();
            live.insert(uses.begin(), uses.end());

            // count use times
            for (auto &reg : instr->get_all_reg()) {
                if (reg->is_physical_reg())
                    continue;
                if (use_times.find(reg) == use_times.end())
                    use_times[reg] = 1;
                else
                    use_times[reg]++;
            }
        }
    }
}

bool RegisterAllocation::move_related(std::shared_ptr<Register> reg) {
    for (auto &m : move_list[reg]) {
        if (active_moves.find(m) != active_moves.end() ||
            worklist_moves.find(m) != worklist_moves.end()) {
            return true;
        }
    }
    return false;
}

void RegisterAllocation::make_worklist() {
    for (auto &reg : initial) {
        if (degree[reg] >= type_k(reg->get_type())) {
            spill_worklist.insert(reg);
        } else if (move_related(reg)) {
            freeze_worklist.insert(reg);
        } else {
            simplify_worklist.insert(reg);
        }
    }
    initial.clear();
}

RegisterSet RegisterAllocation::adjacent(std::shared_ptr<Register> reg) {
    RegisterSet ret = graph[reg];
    auto copy = ret;
    for (auto &v : copy) {
        if (coalesced_nodes.find(v) != coalesced_nodes.end()) {
            ret.erase(v);
        }
        if (in_select_stack.find(v) != in_select_stack.end())
            ret.erase(v);
    }
    return ret;
}

MoveSet RegisterAllocation::node_moves(std::shared_ptr<Register> reg) {
    MoveSet ret;
    for (auto &m : move_list[reg]) {
        if (active_moves.find(m) != active_moves.end() ||
            worklist_moves.find(m) != worklist_moves.end()) {
            ret.insert(m);
        }
    }
    return ret;
}

void RegisterAllocation::simplify() {
    auto reg = *simplify_worklist.begin();
    simplify_worklist.erase(reg);
    select_stack.push_back(reg);
    in_select_stack.insert(reg);
    for (auto &v : adjacent(reg)) {
        decrement_degree(v);
    }
}

void RegisterAllocation::decrement_degree(std::shared_ptr<Register> reg) {
    auto d = degree[reg];
    degree[reg]--;
    if (d == type_k(reg->get_type())) {
        RegisterSet nodes = adjacent(reg);
        nodes.insert(reg);
        enable_moves(nodes);
        spill_worklist.erase(reg);
        if (move_related(reg)) {
            freeze_worklist.insert(reg);
        } else {
            simplify_worklist.insert(reg);
        }
    }
}

void RegisterAllocation::enable_moves(RegisterSet &nodes) {
    for (auto &reg : nodes) {
        for (auto &m : node_moves(reg)) {
            if (active_moves.find(m) != active_moves.end()) {
                active_moves.erase(m);
                worklist_moves.insert(m);
            }
        }
    }
}

std::shared_ptr<Register>
RegisterAllocation::get_alias(std::shared_ptr<Register> reg) {
    if (coalesced_nodes.find(reg) != coalesced_nodes.end()) {
        return alias[reg] = get_alias(alias[reg]);
    }
    return reg;
}

void RegisterAllocation::coalesce() {
    auto m = *worklist_moves.begin();
    worklist_moves.erase(m);
    auto x = get_alias(std::dynamic_pointer_cast<Register>(m->get_operand(0)));
    auto y = get_alias(std::dynamic_pointer_cast<Register>(m->get_operand(1)));
    std::shared_ptr<Register> u, v;
    if (y->is_physical_reg()) {
        u = y;
        v = x;
    } else {
        u = x;
        v = y;
    }
    if (u == v) {
        coalesced_moves.insert(m);
        add_worklist(u);
    } else if (v->is_physical_reg() ||
               edges.find(std::make_pair(u, v)) != edges.end()) {
        constrained_moves.insert(m);
        add_worklist(u);
        add_worklist(v);
    } else if ((u->is_physical_reg() && check_colored_adj(u, v)) ||
               (!u->is_physical_reg() && conservative(u, v))) {
        coalesced_moves.insert(m);
        combine(u, v);
        add_worklist(u);
    } else {
        active_moves.insert(m);
    }
}

bool RegisterAllocation::check_colored_adj(std::shared_ptr<Register> u,
                                           std::shared_ptr<Register> v) {
    for (auto &t : adjacent(v)) {
        if (degree[t] >= type_k(t->get_type()) && !(t->is_physical_reg()) &&
            edges.find(std::make_pair(t, u)) == edges.end()) {
            return false;
        }
    }
    return true;
}

bool RegisterAllocation::conservative(std::shared_ptr<Register> u,
                                      std::shared_ptr<Register> v) {
    RegisterSet res = adjacent(u);
    for (auto &t : adjacent(v)) {
        res.insert(t);
    }
    size_t k = 0;
    for (auto &t : res) {
        if (degree[t] >= type_k(t->get_type())) {
            k++;
        }
    }
    return k < type_k(u->get_type());
}

void RegisterAllocation::add_worklist(std::shared_ptr<Register> reg) {
    if (!reg->is_physical_reg() && !move_related(reg) &&
        degree[reg] < type_k(reg->get_type())) {
        freeze_worklist.erase(reg);
        simplify_worklist.insert(reg);
    }
}

void RegisterAllocation::combine(std::shared_ptr<Register> u,
                                 std::shared_ptr<Register> v) {
    if (freeze_worklist.find(v) != freeze_worklist.end()) {
        freeze_worklist.erase(v);
    } else {
        spill_worklist.erase(v);
    }
    coalesced_nodes.insert(v);
    alias[v] = u;
    move_list[u].insert(move_list[v].begin(), move_list[v].end());
    for (auto &t : adjacent(v)) {
        add_edge(t, u);
        decrement_degree(t);
    }
    if (degree[u] >= type_k(u->get_type()) &&
        freeze_worklist.find(u) != freeze_worklist.end()) {
        freeze_worklist.erase(u);
        spill_worklist.insert(u);
    }
}

void RegisterAllocation::freeze() {
    auto u = *freeze_worklist.begin();
    freeze_worklist.erase(u);
    simplify_worklist.insert(u);
    freeze_moves(u);
}

void RegisterAllocation::freeze_moves(std::shared_ptr<Register> u) {
    for (auto &m : node_moves(u)) {
        std::shared_ptr<Register> x =
            std::dynamic_pointer_cast<Register>(m->get_operand(0));
        std::shared_ptr<Register> y =
            std::dynamic_pointer_cast<Register>(m->get_operand(1));
        std::shared_ptr<Register> v = (x == u) ? y : x;
        if (active_moves.find(m) != active_moves.end())
            active_moves.erase(m);
        else
            worklist_moves.erase(m);
        frozen_moves.insert(m);
        if (node_moves(v).empty() && degree[v] < type_k(v->get_type())) {
            freeze_worklist.erase(v);
            simplify_worklist.insert(v);
        }
    }
}

void RegisterAllocation::select_spill() {
    auto m = *spill_worklist.begin();
    // TODO: add better heuristic to select spill node
    // size_t cost_m = -1; // which means infinity
    // for (auto &reg : spill_worklist) {
    //     if (use_times[reg] < cost_m) {
    //         m = reg;
    //         cost_m = use_times[reg];
    //     }
    // }
    spill_worklist.erase(m);
    simplify_worklist.insert(m);
    freeze_moves(m);
}

void RegisterAllocation::assign_colors() {
    Register::color.clear();
    while (select_stack.size()) {
        auto reg = select_stack.back();
        select_stack.pop_back();
        RegisterSet ok_colors;
        switch (reg->get_type()) {
        case Register::RegisterType::General:
            ok_colors = PhysicalRegister::allocatable_general();
            break;
        case Register::RegisterType::Float:
            ok_colors = PhysicalRegister::allocatable_float();
            break;
        }
        for (auto &w : graph[reg]) {
            auto alias_w = get_alias(w);
            if (alias_w->is_physical_reg()) {
                ok_colors.erase(alias_w);
            } else if (colored_nodes.find(alias_w) != colored_nodes.end()) {
                ok_colors.erase(
                    Register::color[std::dynamic_pointer_cast<VirtualRegister>(
                        alias_w)]);
            }
        }
        if (ok_colors.empty()) {
            spilled_nodes.insert(reg);
        } else {
            colored_nodes.insert(reg);
            Register::color[std::dynamic_pointer_cast<VirtualRegister>(reg)] =
                std::dynamic_pointer_cast<PhysicalRegister>(*ok_colors.begin());
        }
    }
    for (auto &reg : coalesced_nodes) {
        auto alias_reg = get_alias(reg);
        if (alias_reg->is_physical_reg())
            Register::color[std::dynamic_pointer_cast<VirtualRegister>(reg)] =
                std::dynamic_pointer_cast<PhysicalRegister>(alias_reg);
        else
            Register::color[std::dynamic_pointer_cast<VirtualRegister>(reg)] =
                Register::color[std::dynamic_pointer_cast<VirtualRegister>(
                    get_alias(alias_reg))];
    }
    in_select_stack.clear();
}

void RegisterAllocation::rewrite_program() {
    initial.clear();
    Register::temp_regs.clear();
    for (auto &reg : spilled_nodes) {
        func->frame_scheduler->insert_reg(reg);
    }
    for (auto &block : func->get_basic_blocks()) {
        // auto instrs = block->get_instrs();
        // block->clear_instrs();

        for (auto it = block->get_instrs().begin();
             it != block->get_instrs().end(); it++) {
            auto instr = *it;
            for (auto &reg : instr->get_use()) {
                if (spilled_nodes.find(reg) == spilled_nodes.end())
                    continue;
                builder->set_insert_point(block, it);
                auto vreg = VirtualRegister::create(reg->get_type());

                instr->replace_use(reg, vreg);

                size_t offset = func->frame_scheduler->get_reg_offset(reg);
                auto stack_reg = reg->is_using_sp_as_frame_reg()
                                     ? PhysicalRegister::sp()
                                     : PhysicalRegister::fp();

                switch (reg->get_type()) {
                case Register::RegisterType::General:
                    if (Immediate::create(offset)->is_imm_length(12)) {
                        builder->insert_instr(
                            MachineInstr::Tag::LD,
                            {vreg, stack_reg,
                             Immediate::create(
                                 func->frame_scheduler->get_reg_offset(reg))},
                            MachineInstr::Suffix::DWORD);
                    } else {
                        auto offset_reg = VirtualRegister::create(
                            Register::RegisterType::General);
                        builder->add_int_to_reg(offset_reg, stack_reg, offset);
                        builder->insert_instr(
                            MachineInstr::Tag::LD,
                            {vreg, offset_reg, Immediate::create(0)},
                            MachineInstr::Suffix::DWORD);
                    }
                    break;
                case Register::RegisterType::Float:
                    if (Immediate::create(offset)->is_imm_length(12)) {
                        builder->insert_instr(
                            MachineInstr::Tag::FLW,
                            {vreg, stack_reg,
                             Immediate::create(
                                 func->frame_scheduler->get_reg_offset(reg))});
                    } else {
                        auto offset_reg = VirtualRegister::create(
                            Register::RegisterType::General);
                        builder->add_int_to_reg(offset_reg, stack_reg, offset);
                        builder->insert_instr(
                            MachineInstr::Tag::FLW,
                            {vreg, offset_reg, Immediate::create(0)});
                    }
                    break;
                }
            }

            for (auto &reg : instr->get_def()) {
                if (spilled_nodes.find(reg) == spilled_nodes.end())
                    continue;
                builder->set_insert_point(block, std::next(it));
                auto vreg = VirtualRegister::create(reg->get_type());

                instr->replace_def(reg, vreg);

                size_t offset = func->frame_scheduler->get_reg_offset(reg);
                auto stack_reg = reg->is_using_sp_as_frame_reg()
                                     ? PhysicalRegister::sp()
                                     : PhysicalRegister::fp();
                switch (reg->get_type()) {
                case Register::RegisterType::General:
                    if (Immediate::create(offset)->is_imm_length(12)) {
                        builder->insert_instr(
                            MachineInstr::Tag::ST,
                            {vreg, stack_reg,
                             Immediate::create(
                                 func->frame_scheduler->get_reg_offset(reg))},
                            MachineInstr::Suffix::DWORD);
                    } else {
                        auto offset_reg = VirtualRegister::create(
                            Register::RegisterType::General);
                        builder->add_int_to_reg(offset_reg, stack_reg, offset);
                        builder->insert_instr(
                            MachineInstr::Tag::ST,
                            {vreg, offset_reg, Immediate::create(0)},
                            MachineInstr::Suffix::DWORD);
                    }
                    break;
                case Register::RegisterType::Float:
                    if (Immediate::create(offset)->is_imm_length(12)) {
                        builder->insert_instr(
                            MachineInstr::Tag::FSW,
                            {vreg, stack_reg,
                             Immediate::create(
                                 func->frame_scheduler->get_reg_offset(reg))});
                    } else {
                        auto offset_reg = VirtualRegister::create(
                            Register::RegisterType::General);
                        builder->add_int_to_reg(offset_reg, stack_reg, offset);
                        builder->insert_instr(
                            MachineInstr::Tag::FSW,
                            {vreg, offset_reg, Immediate::create(0)});
                    }
                    break;
                }
            }
        }
    }
    rewrite_prologue();

    initial.insert(coalesced_nodes.begin(), coalesced_nodes.end());
    initial.insert(colored_nodes.begin(), colored_nodes.end());
    initial.insert(Register::temp_regs.begin(), Register::temp_regs.end());
    spilled_nodes.clear();
    coalesced_nodes.clear();
    colored_nodes.clear();
}

void RegisterAllocation::rewrite_prologue() {
    auto prologue = func->get_prologue_block();
    auto it =
        prologue->find_instr([](auto &instr) { return instr->is_frame_set(); });
    while (it != prologue->get_instrs_end() && (*it)->is_frame_set()) {
        it = prologue->erase_instr(it);
    }

    builder->set_flag(MachineInstr::Flag::IS_FRAME_SET);
    builder->set_insert_point(prologue, it);

    builder->add_int_to_reg(PhysicalRegister::sp(), PhysicalRegister::sp(),
                            -func->frame_scheduler->get_frame_size());
    // add a nop instruction to mark the pos of frame set
    // which will be removed later
    builder->set_flag(MachineInstr::Flag::IS_FRAME_SET |
                      MachineInstr::Flag::IS_RESERVED);
    builder->insert_instr(MachineInstr::Tag::MOV,
                          {PhysicalRegister::zero(), PhysicalRegister::zero()});
    builder->set_flag(0);
}
