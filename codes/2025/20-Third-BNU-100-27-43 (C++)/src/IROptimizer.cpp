#include "IROptimizer.h"
#include <algorithm>
#include <set>
#include <vector>
#include <list>
#include <queue>
#include <functional>
#include <stack>

namespace MyIR
{
    bool areConstantsEqual(Constant *c1, Constant *c2)
    {
        if (c1 == c2)
            return true;
        if (!c1 || !c2)
            return false;
        if (c1->get_type()->get_type_id() != c2->get_type()->get_type_id())
            return false;

        if (auto c1_int = dynamic_cast<ConstantInt *>(c1))
        {
            if (auto c2_int = dynamic_cast<ConstantInt *>(c2))
            {
                return c1_int->get_value() == c2_int->get_value();
            }
        }
        if (auto c1_float = dynamic_cast<ConstantFloat *>(c1))
        {
            if (auto c2_float = dynamic_cast<ConstantFloat *>(c2))
            {
                return c1_float->get_value() == c2_float->get_value();
            }
        }

        return c1 == c2;
    }

    bool FunctionPass::run(IRUnit *unit)
    {
        bool changed = false;
        for (auto &func : unit->functions)
        {
            if (!func->is_declaration())
            {
                changed |= runOnFunction(func.get());
            }
        }
        return changed;
    }

    void CFGAnalysis::run(Function *func)
    {
        predecessors_map_.clear();
        successors_map_.clear();

        for (auto &bb : func->get_basic_blocks())
        {
            auto terminator = bb->get_instructions().back().get();
            if (auto *br = dynamic_cast<BranchInst *>(terminator))
            {
                if (br->is_conditional())
                {
                    BasicBlock *true_dest = br->get_true_dest();
                    BasicBlock *false_dest = br->get_false_dest();
                    successors_map_[bb.get()].push_back(true_dest);
                    successors_map_[bb.get()].push_back(false_dest);
                    predecessors_map_[true_dest].push_back(bb.get());
                    predecessors_map_[false_dest].push_back(bb.get());
                }
                else
                {
                    BasicBlock *dest = br->get_uncond_dest();
                    successors_map_[bb.get()].push_back(dest);
                    predecessors_map_[dest].push_back(bb.get());
                }
            }
        }
    }

    const std::vector<BasicBlock *> &CFGAnalysis::getPredecessors(BasicBlock *bb) const
    {
        auto it = predecessors_map_.find(bb);
        if (it != predecessors_map_.end())
        {
            return it->second;
        }
        return empty_blocks_;
    }

    const std::vector<BasicBlock *> &CFGAnalysis::getSuccessors(BasicBlock *bb) const
    {
        auto it = successors_map_.find(bb);
        if (it != successors_map_.end())
        {
            return it->second;
        }
        return empty_blocks_;
    }

    void DominatorTree::run(Function *func, const CFGAnalysis &cfg)
    {
        idom_map_.clear();
        dom_children_map_.clear();
        dominance_frontier_map_.clear();

        if (func->get_basic_blocks().empty())
        {
            return;
        }

        computeIdom(func, cfg);
        computeDF(func, cfg);
    }

    bool DominatorTree::dominates(BasicBlock *a, BasicBlock *b) const
    {
        if (a == b)
        {
            return true;
        }
        BasicBlock *current = b;
        while (current)
        {
            auto it = idom_map_.find(current);
            if (it == idom_map_.end())
            {
                return false;
            }
            current = it->second;
            if (current == a)
            {
                return true;
            }
        }
        return false;
    }

    BasicBlock *DominatorTree::getIdom(BasicBlock *bb) const
    {
        auto it = idom_map_.find(bb);
        return (it != idom_map_.end()) ? it->second : nullptr;
    }

    const std::vector<BasicBlock *> &DominatorTree::getDomChildren(BasicBlock *bb) const
    {
        auto it = dom_children_map_.find(bb);
        return (it != dom_children_map_.end()) ? it->second : empty_children_;
    }

    const std::set<BasicBlock *> &DominatorTree::getDominanceFrontier(BasicBlock *bb) const
    {
        auto it = dominance_frontier_map_.find(bb);
        return (it != dominance_frontier_map_.end()) ? it->second : empty_frontier_;
    }

    void DominatorTree::computeIdom(Function *func, const CFGAnalysis &cfg)
    {
        BasicBlock *entry = func->get_basic_blocks().front().get();

        std::vector<BasicBlock *> rpo;
        std::set<BasicBlock *> visited;
        std::vector<BasicBlock *> worklist = {entry};
        visited.insert(entry);
        size_t head = 0;
        while (head < worklist.size())
        {
            BasicBlock *u = worklist[head++];
            rpo.push_back(u);
            for (BasicBlock *succ : cfg.getSuccessors(u))
            {
                if (visited.find(succ) == visited.end())
                {
                    visited.insert(succ);
                    worklist.push_back(succ);
                }
            }
        }
        std::reverse(rpo.begin(), rpo.end());

        std::map<BasicBlock *, int> post_order_map;
        for (size_t i = 0; i < rpo.size(); ++i)
        {
            post_order_map[rpo[i]] = i;
        }

        std::reverse(rpo.begin(), rpo.end());

        idom_map_[entry] = entry;

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (BasicBlock *b : rpo)
            {
                if (b == entry)
                    continue;

                BasicBlock *new_idom = nullptr;
                const auto &predecessors = cfg.getPredecessors(b);

                for (BasicBlock *p : predecessors)
                {
                    if (idom_map_.count(p))
                    {
                        if (new_idom == nullptr)
                        {
                            new_idom = p;
                        }
                        else
                        {
                            BasicBlock *f1 = p;
                            BasicBlock *f2 = new_idom;
                            while (f1 != f2)
                            {
                                while (post_order_map.at(f1) < post_order_map.at(f2))
                                {
                                    f1 = idom_map_.at(f1);
                                }
                                while (post_order_map.at(f2) < post_order_map.at(f1))
                                {
                                    f2 = idom_map_.at(f2);
                                }
                            }
                            new_idom = f1;
                        }
                    }
                }

                if (!idom_map_.count(b) || idom_map_.at(b) != new_idom)
                {
                    idom_map_[b] = new_idom;
                    changed = true;
                }
            }
        }
        idom_map_[entry] = nullptr;

        for (auto const &[node, idom] : idom_map_)
        {
            if (idom)
                dom_children_map_[idom].push_back(node);
        }
    }

    void DominatorTree::computeDF(Function *func, const CFGAnalysis &cfg)
    {
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            BasicBlock *b = bb_ptr.get();
            const auto &predecessors = cfg.getPredecessors(b);
            if (predecessors.size() >= 2)
            {
                for (auto *p : predecessors)
                {
                    BasicBlock *runner = p;
                    while (runner != getIdom(b))
                    {
                        dominance_frontier_map_[runner].insert(b);
                        runner = getIdom(runner);
                        if (!runner)
                            break;
                    }
                }
            }
        }
    }

    bool Mem2Reg::runOnFunction(Function *func)
    {
        current_function_ = func;
        allocas_to_promote_.clear();
        defining_blocks_.clear();
        phi_locations_.clear();
        phi_to_alloca_.clear();
        var_stacks_.clear();
        replacements_.clear();
        zero_constants_map_.clear();

        std::vector<AllocaInst *> all_scalar_allocas;
        BasicBlock *entry_block = func->get_basic_blocks().front().get();
        for (auto &inst_ptr : entry_block->get_instructions())
        {
            if (auto *alloca = dynamic_cast<AllocaInst *>(inst_ptr.get()))
            {
                auto elem_type = alloca->get_allocated_type();
                if (Type::is_integer_ty(elem_type.get()) || Type::is_float_ty(elem_type.get()))
                {
                    all_scalar_allocas.push_back(alloca);
                }
            }
        }

        std::set<AllocaInst *> live_allocas;
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                if (auto *load = dynamic_cast<LoadInst *>(inst_ptr.get()))
                {
                    if (auto *alloca = dynamic_cast<AllocaInst *>(load->get_operand(0)))
                    {
                        if (std::find(all_scalar_allocas.begin(), all_scalar_allocas.end(), alloca) != all_scalar_allocas.end())
                        {
                            live_allocas.insert(alloca);
                        }
                    }
                }
                else if (auto *store = dynamic_cast<StoreInst *>(inst_ptr.get()))
                {
                    if (auto *alloca = dynamic_cast<AllocaInst *>(store->get_operand(1)))
                    {
                        if (std::find(all_scalar_allocas.begin(), all_scalar_allocas.end(), alloca) != all_scalar_allocas.end())
                        {
                            live_allocas.insert(alloca);
                        }
                    }
                }
            }
        }

        for (auto *alloca : all_scalar_allocas)
        {
            if (live_allocas.count(alloca))
            {
                allocas_to_promote_.push_back(alloca);
            }
        }

        if (allocas_to_promote_.empty())
            return false;

        cfg_.run(func);
        dom_tree_.run(func, cfg_);

        int phi_counter = 0;
        for (auto *alloca : allocas_to_promote_)
        {
            std::queue<BasicBlock *> worklist_q;
            std::set<BasicBlock *> worklist_s;

            for (auto &bb_ptr : func->get_basic_blocks())
            {
                for (auto &inst_ptr : bb_ptr->get_instructions())
                {
                    if (auto *store = dynamic_cast<StoreInst *>(inst_ptr.get()))
                    {
                        if (store->get_operand(1) == alloca)
                        {
                            if (worklist_s.find(bb_ptr.get()) == worklist_s.end())
                            {
                                worklist_q.push(bb_ptr.get());
                                worklist_s.insert(bb_ptr.get());
                            }
                        }
                    }
                }
            }

            while (!worklist_q.empty())
            {
                BasicBlock *b = worklist_q.front();
                worklist_q.pop();
                for (BasicBlock *df_node : dom_tree_.getDominanceFrontier(b))
                {
                    if (phi_locations_[alloca].find(df_node) == phi_locations_[alloca].end())
                    {
                        std::string phi_name = "phi." + std::to_string(phi_counter++);
                        auto phi = std::make_shared<Instruction>(alloca->get_allocated_type(), Opcode::PHI, 0, phi_name);
                        phi->set_parent(df_node);
                        df_node->get_instructions().push_front(phi);
                        phi_locations_[alloca].insert(df_node);
                        phi_to_alloca_[phi.get()] = alloca;
                        if (worklist_s.find(df_node) == worklist_s.end())
                        {
                            worklist_q.push(df_node);
                            worklist_s.insert(df_node);
                        }
                    }
                }
            }
        }

        for (auto *alloca : allocas_to_promote_)
        {
            var_stacks_[alloca].push_back(getUndefValFor(alloca->get_allocated_type()));
        }
        rename(func->get_basic_blocks().front().get());

        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                for (size_t i = 0; i < inst_ptr->get_num_operands(); ++i)
                {
                    Value *final_replacement = findReplacement(inst_ptr->get_operand(i));
                    if (final_replacement != inst_ptr->get_operand(i))
                    {
                        inst_ptr->set_operand(i, final_replacement);
                    }
                }
            }
        }

        std::vector<Instruction *> to_remove;
        to_remove.insert(to_remove.end(), allocas_to_promote_.begin(), allocas_to_promote_.end());
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                if (auto *store = dynamic_cast<StoreInst *>(inst_ptr.get()))
                {
                    if (auto *alloca = dynamic_cast<AllocaInst *>(store->get_operand(1)))
                    {
                        if (std::find(allocas_to_promote_.begin(), allocas_to_promote_.end(), alloca) != allocas_to_promote_.end())
                        {
                            to_remove.push_back(store);
                        }
                    }
                }
                else if (auto *load = dynamic_cast<LoadInst *>(inst_ptr.get()))
                {
                    if (auto *alloca = dynamic_cast<AllocaInst *>(load->get_operand(0)))
                    {
                        if (std::find(allocas_to_promote_.begin(), allocas_to_promote_.end(), alloca) != allocas_to_promote_.end())
                        {
                            to_remove.push_back(load);
                        }
                    }
                }
            }
        }

        for (auto *inst : to_remove)
        {
            inst->get_parent()->get_instructions().remove_if([inst](const auto &p)
                                                             { return p.get() == inst; });
        }

        return !allocas_to_promote_.empty();
    }

    void Mem2Reg::rename(BasicBlock *bb)
    {
        std::map<AllocaInst *, int> pushed_counts;

        for (auto &inst_ptr : bb->get_instructions())
        {
            Instruction *inst = inst_ptr.get();
            if (inst->get_opcode() == Opcode::PHI)
            {
                if (phi_to_alloca_.count(inst))
                {
                    AllocaInst *alloca = phi_to_alloca_.at(inst);
                    var_stacks_[alloca].push_back(inst);
                    pushed_counts[alloca]++;
                }
            }
            else if (auto *store = dynamic_cast<StoreInst *>(inst))
            {
                if (auto *alloca = dynamic_cast<AllocaInst *>(store->get_operand(1)))
                {
                    if (std::find(allocas_to_promote_.begin(), allocas_to_promote_.end(), alloca) != allocas_to_promote_.end())
                    {
                        var_stacks_[alloca].push_back(store->get_operand(0));
                        pushed_counts[alloca]++;
                    }
                }
            }
            else if (auto *load = dynamic_cast<LoadInst *>(inst))
            {
                if (auto *alloca = dynamic_cast<AllocaInst *>(load->get_operand(0)))
                {
                    if (std::find(allocas_to_promote_.begin(), allocas_to_promote_.end(), alloca) != allocas_to_promote_.end())
                    {
                        if (var_stacks_.count(alloca) && !var_stacks_[alloca].empty())
                        {
                            replacements_[load] = var_stacks_[alloca].back();
                        }
                        else
                        {
                            replacements_[load] = getUndefValFor(alloca->get_allocated_type());
                        }
                    }
                }
            }
        }

        for (BasicBlock *succ : cfg_.getSuccessors(bb))
        {
            for (auto &inst_ptr : succ->get_instructions())
            {
                if (inst_ptr->get_opcode() == Opcode::PHI)
                {
                    Instruction *phi = inst_ptr.get();
                    if (phi_to_alloca_.count(phi))
                    {
                        AllocaInst *alloca = phi_to_alloca_.at(phi);
                        Value *incoming_val;
                        if (var_stacks_.count(alloca) && !var_stacks_[alloca].empty())
                        {
                            incoming_val = var_stacks_[alloca].back();
                        }
                        else
                        {
                            incoming_val = getUndefValFor(alloca->get_allocated_type());
                        }
                        phi->add_operand(incoming_val);
                        // 风险提示: 下面的 reinterpret_cast 要求 BasicBlock* 和 Value* 之间存在安全的转换关系
                        // (例如, BasicBlock 继承自 Value), 类似LLVM的设计。
                        // 如果不是这样, 这里可能导致未定义行为和错误的IR输出。
                        phi->add_operand(reinterpret_cast<Value *>(bb));
                    }
                }
            }
        }

        for (auto *child : dom_tree_.getDomChildren(bb))
        {
            rename(child);
        }

        for (auto const &[alloca, count] : pushed_counts)
        {
            for (int i = 0; i < count; ++i)
            {
                if (var_stacks_.count(alloca) && !var_stacks_[alloca].empty())
                {
                    var_stacks_[alloca].pop_back();
                }
            }
        }
    }

    Value *Mem2Reg::getUndefValFor(std::shared_ptr<Type> type)
    {
        TypeID id = type->get_type_id();
        if (zero_constants_map_.find(id) == zero_constants_map_.end())
        {
            Constant *new_const = nullptr;
            switch (type->get_type_id())
            {
            case TypeID::Integer:
            {
                auto int_type = std::static_pointer_cast<IntegerType>(type);
                new_const = new ConstantInt(int_type, 0);
                break;
            }
            case TypeID::Float:
            {
                auto float_type = std::static_pointer_cast<FloatType>(type);
                new_const = new ConstantFloat(float_type, 0.0);
                break;
            }
            default:
                return nullptr;
            }
            zero_constants_map_[id] = new_const;
        }
        return zero_constants_map_.at(id);
    }

    Value *Mem2Reg::findReplacement(Value *v)
    {
        if (replacements_.count(v))
        {
            Value *rep = replacements_.at(v);
            Value *final_rep = findReplacement(rep);
            if (final_rep != rep)
            {
                replacements_[v] = final_rep;
            }
            return final_rep;
        }
        return v;
    }

    bool SCCP::runOnFunction(Function *func)
    {
        lattice_values_.clear();
        executable_blocks_.clear();
        ssa_worklist_.clear();
        cfg_worklist_.clear();

        if (func->get_basic_blocks().empty())
            return false;

        add_to_cfg_worklist(func->get_basic_blocks().front().get());

        for (auto &arg : func->get_arguments())
        {
            setLatticeValue(arg.get(), {OVERDEFINED, nullptr});
        }

        while (!cfg_worklist_.empty() || !ssa_worklist_.empty())
        {
            if (!cfg_worklist_.empty())
            {
                BasicBlock *bb = cfg_worklist_.back();
                cfg_worklist_.pop_back();

                for (auto &inst_ptr : bb->get_instructions())
                {
                    visit(inst_ptr.get());
                }
            }
            if (!ssa_worklist_.empty())
            {
                Value *v = ssa_worklist_.back();
                ssa_worklist_.pop_back();

                // This is a simplification; in a full SSA-based pass, we'd use use-def chains.
                // Iterating all instructions is less efficient but works for this structure.
                for (auto &bb_ptr : func->get_basic_blocks())
                {
                    for (auto &inst_ptr : bb_ptr->get_instructions())
                    {
                        for (size_t i = 0; i < inst_ptr->get_num_operands(); ++i)
                        {
                            if (inst_ptr->get_operand(i) == v)
                            {
                                visit(inst_ptr.get());
                                break;
                            }
                        }
                    }
                }
            }
        }

        bool changed = false;
        std::map<Value *, Value *> replacements;
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                auto lval = getLatticeValue(inst_ptr.get());
                if (lval.type == CONSTANT && static_cast<Value *>(inst_ptr.get()) != static_cast<Value *>(lval.value))
                {
                    replacements[inst_ptr.get()] = lval.value;
                }
            }
        }

        if (!replacements.empty())
        {
            changed = true;
            for (auto &bb_ptr : func->get_basic_blocks())
            {
                for (auto &inst_ptr : bb_ptr->get_instructions())
                {
                    for (size_t i = 0; i < inst_ptr->get_num_operands(); ++i)
                    {
                        if (replacements.count(inst_ptr->get_operand(i)))
                        {
                            inst_ptr->set_operand(i, replacements.at(inst_ptr->get_operand(i)));
                        }
                    }
                }
            }
        }

        for (auto &bb_ptr : func->get_basic_blocks())
        {
            if (executable_blocks_.find(bb_ptr.get()) == executable_blocks_.end())
            {
                continue;
            }

            // The terminator is the last instruction.
            auto terminator = bb_ptr->get_instructions().back().get();
            if (auto br = dynamic_cast<BranchInst *>(terminator))
            {
                if (br->is_conditional())
                {
                    auto lval = getLatticeValue(br->get_operand(0));
                    if (lval.type == CONSTANT)
                    {
                        auto *const_int = dynamic_cast<ConstantInt *>(lval.value);
                        if (const_int)
                        {
                            BasicBlock *taken_dest = (const_int->get_value() != 0) ? br->get_true_dest() : br->get_false_dest();
                            BasicBlock *untaken_dest = (const_int->get_value() != 0) ? br->get_false_dest() : br->get_true_dest();

                            // --- START OF FIX ---
                            // A branch from bb_ptr to untaken_dest is being removed.
                            // We must update any PHI nodes in untaken_dest.
                            for (auto &succ_inst_ptr : untaken_dest->get_instructions())
                            {
                                if (succ_inst_ptr->get_opcode() == Opcode::PHI)
                                {
                                    // Iterate backwards to safely remove operands.
                                    for (int i = succ_inst_ptr->get_num_operands() - 2; i >= 0; i -= 2)
                                    {
                                        if (succ_inst_ptr->get_operand(i + 1) == reinterpret_cast<Value *>(bb_ptr.get()))
                                        {
                                            succ_inst_ptr->remove_operands(i, 2);
                                        }
                                    }
                                }
                                else
                                {
                                    // PHI nodes must be the first instructions in a block.
                                    break;
                                }
                            }
                            // --- END OF FIX ---

                            // Replace the conditional branch with an unconditional one.
                            auto new_br = std::make_shared<BranchInst>(taken_dest, br->get_parent());

                            bb_ptr->get_instructions().pop_back();
                            bb_ptr->get_instructions().push_back(new_br);
                            changed = true;
                        }
                    }
                }
            }
        }

        return changed;
    }

    void SCCP::add_to_cfg_worklist(BasicBlock *bb)
    {
        if (executable_blocks_.find(bb) == executable_blocks_.end())
        {
            executable_blocks_.insert(bb);
            cfg_worklist_.push_back(bb);
        }
    }

    void SCCP::add_to_ssa_worklist(Value *v)
    {
        // To prevent duplicates, could use a set
        ssa_worklist_.push_back(v);
    }

    SCCP::LatticeValue SCCP::getLatticeValue(Value *v)
    {
        if (!v)
        {
            return {OVERDEFINED, nullptr};
        }
        if (auto c = dynamic_cast<Constant *>(v))
        {
            return {CONSTANT, c};
        }
        if (lattice_values_.count(v))
        {
            return lattice_values_[v];
        }
        return {UNDEFINED, nullptr};
    }

    void SCCP::setLatticeValue(Value *v, SCCP::LatticeValue lval)
    {
        LatticeValue old_lval = getLatticeValue(v);
        bool changed = false;
        if (old_lval.type != lval.type)
        {
            changed = true;
        }
        else if (old_lval.type == CONSTANT)
        {
            if (!areConstantsEqual(old_lval.value, lval.value))
            {
                changed = true;
            }
        }

        if (changed)
        {
            lattice_values_[v] = lval;
            add_to_ssa_worklist(v);
        }
    }

    void SCCP::visit(Value *v)
    {
        if (!dynamic_cast<Instruction *>(v))
        {
            return;
        }

        if (getLatticeValue(v).type == OVERDEFINED)
        {
            return;
        }

        if (auto bin_inst = dynamic_cast<BinaryInst *>(v))
        {
            visitBinaryInst(bin_inst);
        }
        else if (auto cmp_inst = dynamic_cast<CmpInst *>(v))
        {
            visitCmpInst(cmp_inst);
        }
        else if (auto cast_inst = dynamic_cast<CastInst *>(v))
        {
            visitCastInst(cast_inst);
        }
        else if (auto load_inst = dynamic_cast<LoadInst *>(v))
        {
            visitLoadInst(load_inst);
        }
        else if (auto phi = dynamic_cast<Instruction *>(v); phi && phi->get_opcode() == Opcode::PHI)
        {
            visitPhiNode(phi);
        }
        else if (auto br = dynamic_cast<BranchInst *>(v))
        {
            visitBranchInst(br);
        }
        else if (auto ret = dynamic_cast<ReturnInst *>(v))
        {
            visitReturnInst(ret);
        }
        else if (auto inst = dynamic_cast<Instruction *>(v))
        {
            if (inst->get_type()->get_type_id() != TypeID::Void)
            {
                setLatticeValue(inst, {OVERDEFINED, nullptr});
            }
        }
    }

    void SCCP::visitPhiNode(Instruction *phi)
    {
        if (getLatticeValue(phi).type == OVERDEFINED)
        {
            return;
        }

        LatticeValue new_phi_lval = {UNDEFINED, nullptr};

        for (size_t i = 0; i < phi->get_num_operands(); i += 2)
        {
            Value *incoming_value = phi->get_operand(i);
            LatticeValue incoming_lval = getLatticeValue(incoming_value);

            if (incoming_lval.type == UNDEFINED)
            {
                continue;
            }
            if (new_phi_lval.type == UNDEFINED)
            {
                new_phi_lval = incoming_lval;
                continue;
            }

            if (new_phi_lval.type == OVERDEFINED || incoming_lval.type == OVERDEFINED)
            {
                new_phi_lval.type = OVERDEFINED;
                new_phi_lval.value = nullptr;
            }
            else if (new_phi_lval.type == CONSTANT && incoming_lval.type == CONSTANT)
            {
                if (!areConstantsEqual(new_phi_lval.value, incoming_lval.value))
                {
                    new_phi_lval.type = OVERDEFINED;
                    new_phi_lval.value = nullptr;
                }
            }
        }

        setLatticeValue(phi, new_phi_lval);
    }

    void SCCP::visitLoadInst(LoadInst *inst)
    {
        auto ptr = inst->get_operand(0);

        if (auto *gv = dynamic_cast<GlobalVariable *>(ptr))
        {
            if (gv->get_linkage() == LinkageType::Constant)
            {
                setLatticeValue(inst, {CONSTANT, gv->get_initializer().get()});
                return;
            }
        }

        setLatticeValue(inst, {OVERDEFINED, nullptr});
    }

    void SCCP::visitBranchInst(BranchInst *inst)
    {
        if (inst->is_conditional())
        {
            LatticeValue lval = getLatticeValue(inst->get_operand(0));
            if (lval.type == CONSTANT)
            {
                auto *c_int = dynamic_cast<ConstantInt *>(lval.value);
                if (c_int)
                {
                    if (c_int->get_value() != 0)
                    {
                        add_to_cfg_worklist(inst->get_true_dest());
                    }
                    else
                    {
                        add_to_cfg_worklist(inst->get_false_dest());
                    }
                }
            }
            else
            {
                add_to_cfg_worklist(inst->get_true_dest());
                add_to_cfg_worklist(inst->get_false_dest());
            }
        }
        else
        {
            add_to_cfg_worklist(inst->get_uncond_dest());
        }
    }

    void SCCP::visitReturnInst(ReturnInst *inst)
    {
        if (inst->has_return_value())
        {
            getLatticeValue(inst->get_operand(0));
        }
    }

    void SCCP::visitBinaryInst(BinaryInst *inst)
    {
        auto lval1 = getLatticeValue(inst->get_operand(0));
        auto lval2 = getLatticeValue(inst->get_operand(1));

        if (lval1.type == UNDEFINED || lval2.type == UNDEFINED)
        {
            setLatticeValue(inst, {UNDEFINED, nullptr});
            return;
        }

        if (lval1.type == CONSTANT && lval2.type == CONSTANT)
        {
            Constant *result_const = nullptr;
            bool is_float_op = Type::is_float_ty(inst->get_type().get());

            if (is_float_op)
            {
                auto const1 = dynamic_cast<ConstantFloat *>(lval1.value);
                auto const2 = dynamic_cast<ConstantFloat *>(lval2.value);
                if (const1 && const2)
                {
                    double v1 = const1->get_value();
                    double v2 = const2->get_value();
                    double result = 0.0;
                    bool overdefined = false;

                    switch (inst->get_opcode())
                    {
                    case Opcode::FAdd:
                        result = v1 + v2;
                        break;
                    case Opcode::FSub:
                        result = v1 - v2;
                        break;
                    case Opcode::FMul:
                        result = v1 * v2;
                        break;
                    case Opcode::FDiv:
                        if (v2 == 0.0)
                            overdefined = true;
                        else
                            result = v1 / v2;
                        break;
                    default:
                        overdefined = true;
                        break;
                    }

                    if (!overdefined)
                    {
                        result_const = new ConstantFloat(std::static_pointer_cast<FloatType>(inst->get_type()), result);
                    }
                }
            }
            else
            {
                auto const1 = dynamic_cast<ConstantInt *>(lval1.value);
                auto const2 = dynamic_cast<ConstantInt *>(lval2.value);
                if (const1 && const2)
                {
                    int64_t v1 = const1->get_value();
                    int64_t v2 = const2->get_value();
                    int64_t result = 0;
                    bool overdefined = false;

                    switch (inst->get_opcode())
                    {
                    case Opcode::Add:
                        result = v1 + v2;
                        break;
                    case Opcode::Sub:
                        result = v1 - v2;
                        break;
                    case Opcode::Mul:
                        result = v1 * v2;
                        break;
                    case Opcode::SDiv:
                        if (v2 == 0)
                            overdefined = true;
                        else
                            result = v1 / v2;
                        break;
                    case Opcode::SRem:
                        if (v2 == 0)
                            overdefined = true;
                        else
                            result = v1 % v2;
                        break;
                    default:
                        overdefined = true;
                        break;
                    }

                    if (!overdefined)
                    {
                        result_const = new ConstantInt(std::static_pointer_cast<IntegerType>(inst->get_type()), result);
                    }
                }
            }

            if (result_const)
            {
                setLatticeValue(inst, {CONSTANT, result_const});
            }
            else
            {
                setLatticeValue(inst, {OVERDEFINED, nullptr});
            }
            return;
        }

        setLatticeValue(inst, {OVERDEFINED, nullptr});
    }

    void SCCP::visitCmpInst(CmpInst *inst)
    {
        auto lval1 = getLatticeValue(inst->get_operand(0));
        auto lval2 = getLatticeValue(inst->get_operand(1));

        if (lval1.type == OVERDEFINED || lval2.type == OVERDEFINED)
        {
            setLatticeValue(inst, {OVERDEFINED, nullptr});
            return;
        }
        if (lval1.type == UNDEFINED || lval2.type == UNDEFINED)
        {
            setLatticeValue(inst, {UNDEFINED, nullptr});
            return;
        }

        bool result = false;
        bool evaluated = false;

        if (inst->get_opcode() == Opcode::FCmp)
        {
            auto const1 = dynamic_cast<ConstantFloat *>(lval1.value);
            auto const2 = dynamic_cast<ConstantFloat *>(lval2.value);
            if (const1 && const2)
            {
                double v1 = const1->get_value();
                double v2 = const2->get_value();
                evaluated = true;
                switch (inst->get_predicate())
                {
                case CmpPredicate::OEQ:
                    result = (v1 == v2);
                    break;
                case CmpPredicate::ONE:
                    result = (v1 != v2);
                    break;
                case CmpPredicate::OGT:
                    result = (v1 > v2);
                    break;
                case CmpPredicate::OGE:
                    result = (v1 >= v2);
                    break;
                case CmpPredicate::OLT:
                    result = (v1 < v2);
                    break;
                case CmpPredicate::OLE:
                    result = (v1 <= v2);
                    break;
                default:
                    evaluated = false;
                    break;
                }
            }
        }
        else
        {
            auto const1 = dynamic_cast<ConstantInt *>(lval1.value);
            auto const2 = dynamic_cast<ConstantInt *>(lval2.value);
            if (const1 && const2)
            {
                int64_t v1 = const1->get_value();
                int64_t v2 = const2->get_value();
                evaluated = true;
                switch (inst->get_predicate())
                {
                case CmpPredicate::EQ:
                    result = (v1 == v2);
                    break;
                case CmpPredicate::NE:
                    result = (v1 != v2);
                    break;
                case CmpPredicate::SGT:
                    result = (v1 > v2);
                    break;
                case CmpPredicate::SGE:
                    result = (v1 >= v2);
                    break;
                case CmpPredicate::SLT:
                    result = (v1 < v2);
                    break;
                case CmpPredicate::SLE:
                    result = (v1 <= v2);
                    break;
                default:
                    evaluated = false;
                    break;
                }
            }
        }

        if (evaluated)
        {
            auto int_type = std::dynamic_pointer_cast<IntegerType>(inst->get_type());
            setLatticeValue(inst, {CONSTANT, new ConstantInt(int_type, result ? 1 : 0)});
        }
        else
        {
            setLatticeValue(inst, {OVERDEFINED, nullptr});
        }
    }

    void SCCP::visitCastInst(CastInst *inst)
    {
        auto operand_lval = getLatticeValue(inst->get_operand(0));

        if (operand_lval.type == OVERDEFINED)
        {
            setLatticeValue(inst, {OVERDEFINED, nullptr});
            return;
        }
        if (operand_lval.type == UNDEFINED)
        {
            setLatticeValue(inst, {UNDEFINED, nullptr});
            return;
        }

        Constant *in_const = operand_lval.value;
        Constant *out_const = nullptr;
        auto opcode = inst->get_opcode();
        auto dest_ty = inst->get_type();

        switch (opcode)
        {
        case Opcode::SIToFP:
            if (auto ci = dynamic_cast<ConstantInt *>(in_const))
            {
                out_const = new ConstantFloat(std::static_pointer_cast<FloatType>(dest_ty), static_cast<double>(ci->get_value()));
            }
            break;
        case Opcode::FPToSI:
            if (auto cf = dynamic_cast<ConstantFloat *>(in_const))
            {
                out_const = new ConstantInt(std::static_pointer_cast<IntegerType>(dest_ty), static_cast<int64_t>(cf->get_value()));
            }
            break;
        case Opcode::ZExt:
            if (auto ci = dynamic_cast<ConstantInt *>(in_const))
            {
                out_const = new ConstantInt(std::static_pointer_cast<IntegerType>(dest_ty), ci->get_value());
            }
            break;
        case Opcode::Bitcast:
            break;
        default:
            break;
        }

        if (out_const)
        {
            setLatticeValue(inst, {CONSTANT, out_const});
        }
        else
        {
            setLatticeValue(inst, {OVERDEFINED, nullptr});
        }
    }

    bool DeadCodeElimination::isDead(Instruction *inst)
    {
        if (inst->is_terminator() || inst->get_opcode() == Opcode::PHI ||
            dynamic_cast<StoreInst *>(inst) ||
            dynamic_cast<AllocaInst *>(inst) ||
            dynamic_cast<CallInst *>(inst))
        {
            return false;
        }

        for (auto &bb : inst->get_parent()->get_parent()->get_basic_blocks())
        {
            for (auto &other_inst : bb->get_instructions())
            {
                for (size_t i = 0; i < other_inst->get_num_operands(); ++i)
                {
                    if (other_inst->get_operand(i) == inst)
                        return false;
                }
            }
        }
        return true;
    }

    bool DeadCodeElimination::runOnFunction(Function *func)
    {
        bool total_changed = false;

        std::set<BasicBlock *> reachable_blocks;
        std::queue<BasicBlock *> worklist_bb;
        if (!func->get_basic_blocks().empty())
        {
            BasicBlock *entry = func->get_basic_blocks().front().get();
            worklist_bb.push(entry);
            reachable_blocks.insert(entry);
        }
        while (!worklist_bb.empty())
        {
            BasicBlock *bb = worklist_bb.front();
            worklist_bb.pop();
            auto term = bb->get_instructions().back().get();
            if (auto br = dynamic_cast<BranchInst *>(term))
            {
                if (br->is_conditional())
                {
                    if (reachable_blocks.find(br->get_true_dest()) == reachable_blocks.end())
                    {
                        reachable_blocks.insert(br->get_true_dest());
                        worklist_bb.push(br->get_true_dest());
                    }
                    if (reachable_blocks.find(br->get_false_dest()) == reachable_blocks.end())
                    {
                        reachable_blocks.insert(br->get_false_dest());
                        worklist_bb.push(br->get_false_dest());
                    }
                }
                else
                {
                    if (reachable_blocks.find(br->get_uncond_dest()) == reachable_blocks.end())
                    {
                        reachable_blocks.insert(br->get_uncond_dest());
                        worklist_bb.push(br->get_uncond_dest());
                    }
                }
            }
        }

        std::vector<BasicBlock *> unreachable_blocks;
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            if (reachable_blocks.find(bb_ptr.get()) == reachable_blocks.end())
            {
                unreachable_blocks.push_back(bb_ptr.get());
            }
        }

        if (!unreachable_blocks.empty())
        {
            total_changed = true;
            for (auto *bb : unreachable_blocks)
            {
                auto term = bb->get_instructions().back().get();
                std::vector<BasicBlock *> successors;
                if (auto br = dynamic_cast<BranchInst *>(term))
                {
                    if (br->is_conditional())
                    {
                        successors.push_back(br->get_true_dest());
                        successors.push_back(br->get_false_dest());
                    }
                    else
                    {
                        successors.push_back(br->get_uncond_dest());
                    }
                }

                for (auto *succ : successors)
                {
                    if (reachable_blocks.find(succ) == reachable_blocks.end())
                        continue;
                    for (auto &inst_ptr : succ->get_instructions())
                    {
                        if (inst_ptr->get_opcode() == Opcode::PHI)
                        {
                            for (int i = inst_ptr->get_num_operands() - 2; i >= 0; i -= 2)
                            {
                                if (inst_ptr->get_operand(i + 1) == bb)
                                {
                                    inst_ptr->remove_operands(i, 2);
                                }
                            }
                        }
                    }
                }
            }

            func->get_basic_blocks().remove_if([&](const auto &p)
                                               { return std::find(unreachable_blocks.begin(), unreachable_blocks.end(), p.get()) != unreachable_blocks.end(); });
        }

        bool iter_changed = true;
        while (iter_changed)
        {
            iter_changed = false;
            std::vector<Instruction *> dead_instructions;
            std::set<Instruction *> live_instructions;
            std::queue<Instruction *> worklist_inst;

            for (auto &bb_ptr : func->get_basic_blocks())
            {
                for (auto &inst_ptr : bb_ptr->get_instructions())
                {
                    if (inst_ptr->is_terminator() ||
                        dynamic_cast<StoreInst *>(inst_ptr.get()) ||
                        dynamic_cast<CallInst *>(inst_ptr.get()))
                    {
                        if (live_instructions.find(inst_ptr.get()) == live_instructions.end())
                        {
                            worklist_inst.push(inst_ptr.get());
                            live_instructions.insert(inst_ptr.get());
                        }
                    }
                }
            }

            while (!worklist_inst.empty())
            {
                Instruction *inst = worklist_inst.front();
                worklist_inst.pop();

                for (size_t i = 0; i < inst->get_num_operands(); ++i)
                {
                    if (auto *op_inst = dynamic_cast<Instruction *>(inst->get_operand(i)))
                    {
                        if (live_instructions.find(op_inst) == live_instructions.end())
                        {
                            live_instructions.insert(op_inst);
                            worklist_inst.push(op_inst);
                        }
                    }
                }
            }

            for (auto &bb_ptr : func->get_basic_blocks())
            {
                for (auto &inst_ptr : bb_ptr->get_instructions())
                {
                    if (live_instructions.find(inst_ptr.get()) == live_instructions.end() && !dynamic_cast<AllocaInst *>(inst_ptr.get()))
                    {
                        dead_instructions.push_back(inst_ptr.get());
                    }
                }
            }

            if (!dead_instructions.empty())
            {
                iter_changed = true;
                total_changed = true;
                for (auto *inst : dead_instructions)
                {
                    inst->get_parent()->get_instructions().remove_if([inst](const auto &p)
                                                                     { return p.get() == inst; });
                }
            }
        }
        return total_changed;
    }

    IROptimizer::IROptimizer()
    {
        passes_.push_back(std::make_unique<AllocaHoisting>());
        passes_.push_back(std::make_unique<Mem2Reg>());
        passes_.push_back(std::make_unique<FunctionInlining>());
        passes_.push_back(std::make_unique<LoopInvariantCodeMotion>());
        passes_.push_back(std::make_unique<SCCP>());
        passes_.push_back(std::make_unique<StrengthReduction>());
        passes_.push_back(std::make_unique<DeadCodeElimination>());
    }

    void IROptimizer::run(IRUnit *unit)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto &pass : passes_)
            {
                changed |= pass->run(unit);
            }
        }

        for (auto &func : unit->functions)
        {
            if (func->is_declaration())
            {
                continue;
            }

            int temp_reg_counter = 0;
            for (auto &bb : func->get_basic_blocks())
            {
                for (auto &inst_ptr : bb->get_instructions())
                {
                    if (inst_ptr->get_type()->get_type_id() != MyIR::TypeID::Void)
                    {
                        inst_ptr->set_name(std::to_string(temp_reg_counter++));
                    }
                }
            }
        }
    }

    bool StrengthReduction::isPowerOfTwo(int64_t n, int &exponent)
    {
        if (n <= 0)
        {
            return false;
        }
        if ((n & (n - 1)) != 0)
        {
            return false;
        }
        exponent = 0;
        int64_t temp = n;
        while (temp > 1)
        {
            temp >>= 1;
            exponent++;
        }
        return true;
    }

    bool StrengthReduction::runOnFunction(Function *func)
    {
        bool changed = false;
        std::map<Instruction *, Value *> replacements;
        std::vector<Instruction *> to_remove;

        constexpr int COST_THRESHOLD = 4;

        for (auto &bb_ptr : func->get_basic_blocks())
        {
            auto &instructions = bb_ptr->get_instructions();
            for (auto it = instructions.begin(); it != instructions.end(); ++it)
            {
                Instruction *inst = it->get();
                auto bin_inst = dynamic_cast<BinaryInst *>(inst);

                if (!bin_inst || (bin_inst->get_opcode() != Opcode::Mul && bin_inst->get_opcode() != Opcode::SDiv))
                {
                    continue;
                }

                if (!Type::is_integer_ty(bin_inst->get_type().get()))
                {
                    continue;
                }

                Value *operand1 = bin_inst->get_operand(0);
                Value *operand2 = bin_inst->get_operand(1);
                ConstantInt *const_operand = nullptr;
                Value *var_operand = nullptr;

                if (bin_inst->get_opcode() == Opcode::Mul)
                {
                    if (auto c = dynamic_cast<ConstantInt *>(operand1))
                    {
                        const_operand = c;
                        var_operand = operand2;
                    }
                    else if (auto c = dynamic_cast<ConstantInt *>(operand2))
                    {
                        const_operand = c;
                        var_operand = operand1;
                    }
                }
                else // SDiv
                {
                    if (auto c = dynamic_cast<ConstantInt *>(operand2))
                    {
                        const_operand = c;
                        var_operand = operand1;
                    }
                }

                if (!const_operand)
                {
                    continue;
                }

                int64_t C = const_operand->get_value();
                Value *new_final_value = nullptr;
                std::list<std::shared_ptr<Instruction>> generated_insts;
                auto int_type = std::static_pointer_cast<IntegerType>(var_operand->get_type());

                if (bin_inst->get_opcode() == Opcode::Mul)
                {
                    if (C == 0)
                    {
                        new_final_value = new ConstantInt(int_type, 0);
                    }
                    else if (C == 1)
                    {
                        new_final_value = var_operand;
                    }
                    else if (C == -1)
                    {
                        auto zero = new ConstantInt(int_type, 0);
                        auto sub = std::make_shared<BinaryInst>(Opcode::Sub, zero, var_operand, "", inst->get_parent());
                        generated_insts.push_back(sub);
                        new_final_value = sub.get();
                    }
                    else
                    {
                        int64_t C_abs = std::abs(C);
                        int exponent;
                        if (isPowerOfTwo(C_abs, exponent))
                        {
                            auto shift_amount = new ConstantInt(int_type, exponent);
                            auto shl = std::make_shared<BinaryInst>(Opcode::Shl, var_operand, shift_amount, "", inst->get_parent());
                            generated_insts.push_back(shl);
                            new_final_value = shl.get();
                        }
                        else if (isPowerOfTwo(C_abs - 1, exponent))
                        {
                            auto shift_amount = new ConstantInt(int_type, exponent);
                            auto shl = std::make_shared<BinaryInst>(Opcode::Shl, var_operand, shift_amount, "", inst->get_parent());
                            generated_insts.push_back(shl);
                            auto add = std::make_shared<BinaryInst>(Opcode::Add, shl.get(), var_operand, "", inst->get_parent());
                            generated_insts.push_back(add);
                            new_final_value = add.get();
                        }
                        else if (isPowerOfTwo(C_abs + 1, exponent))
                        {
                            auto shift_amount = new ConstantInt(int_type, exponent);
                            auto shl = std::make_shared<BinaryInst>(Opcode::Shl, var_operand, shift_amount, "", inst->get_parent());
                            generated_insts.push_back(shl);
                            auto sub = std::make_shared<BinaryInst>(Opcode::Sub, shl.get(), var_operand, "", inst->get_parent());
                            generated_insts.push_back(sub);
                            new_final_value = sub.get();
                        }
                        else
                        {
                            int popcount = 0;
                            for (int i = 0; i < 63; ++i)
                                if ((C_abs >> i) & 1)
                                    popcount++;
                            int cost = (popcount > 1) ? (2 * popcount - 1) : popcount;

                            if (C < 0)
                                cost++;

                            if (cost <= COST_THRESHOLD)
                            {
                                Value *running_sum = nullptr;
                                for (int i = 0; i < 63; ++i)
                                {
                                    if ((C_abs >> i) & 1)
                                    {
                                        auto shift_amount = new ConstantInt(int_type, i);
                                        auto term = std::make_shared<BinaryInst>(Opcode::Shl, var_operand, shift_amount, "", inst->get_parent());
                                        generated_insts.push_back(term);

                                        if (running_sum == nullptr)
                                        {
                                            running_sum = term.get();
                                        }
                                        else
                                        {
                                            auto add = std::make_shared<BinaryInst>(Opcode::Add, running_sum, term.get(), "", inst->get_parent());
                                            generated_insts.push_back(add);
                                            running_sum = add.get();
                                        }
                                    }
                                }
                                new_final_value = running_sum;
                            }
                        }

                        if (C < 0 && new_final_value && generated_insts.size() < COST_THRESHOLD)
                        {
                            auto zero = new ConstantInt(int_type, 0);
                            auto negate = std::make_shared<BinaryInst>(Opcode::Sub, zero, new_final_value, "", inst->get_parent());
                            generated_insts.push_back(negate);
                            new_final_value = negate.get();
                        }
                    }
                }
                else // SDiv
                {
                    int exponent;
                    if (C > 0 && isPowerOfTwo(C, exponent) && exponent > 0)
                    {
                        auto thirty_one = new ConstantInt(int_type, 31);
                        auto magic_number = new ConstantInt(int_type, C - 1);
                        auto shift_amount = new ConstantInt(int_type, exponent);

                        auto sign_bit = std::make_shared<BinaryInst>(Opcode::AShr, var_operand, thirty_one, "", inst->get_parent());
                        generated_insts.push_back(sign_bit);

                        auto add_val = std::make_shared<BinaryInst>(Opcode::And, sign_bit.get(), magic_number, "", inst->get_parent());
                        generated_insts.push_back(add_val);

                        auto sum = std::make_shared<BinaryInst>(Opcode::Add, var_operand, add_val.get(), "", inst->get_parent());
                        generated_insts.push_back(sum);

                        auto result = std::make_shared<BinaryInst>(Opcode::AShr, sum.get(), shift_amount, "", inst->get_parent());
                        generated_insts.push_back(result);

                        new_final_value = result.get();
                    }
                }

                if (new_final_value)
                {
                    changed = true;

                    if (auto new_inst_final = dynamic_cast<Instruction *>(new_final_value))
                    {
                        new_inst_final->set_name(inst->get_name());
                    }

                    for (const auto &new_i_ptr : generated_insts)
                    {
                        instructions.insert(it, new_i_ptr);
                    }

                    replacements[inst] = new_final_value;
                    to_remove.push_back(inst);
                }
            }
        }

        if (!changed)
        {
            return false;
        }

        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                if (std::find(to_remove.begin(), to_remove.end(), inst_ptr.get()) != to_remove.end())
                {
                    continue;
                }
                for (size_t i = 0; i < inst_ptr->get_num_operands(); ++i)
                {
                    if (auto op_inst = dynamic_cast<Instruction *>(inst_ptr->get_operand(i)))
                    {
                        if (replacements.count(op_inst))
                        {
                            inst_ptr->set_operand(i, replacements.at(op_inst));
                        }
                    }
                }
            }
        }

        for (auto *inst_to_remove : to_remove)
        {
            inst_to_remove->get_parent()->get_instructions().remove_if(
                [inst_to_remove](const auto &p)
                { return p.get() == inst_to_remove; });
        }

        int temp_reg_counter = 0;
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                if (inst_ptr->get_type()->get_type_id() != TypeID::Void)
                {
                    inst_ptr->set_name(std::to_string(temp_reg_counter++));
                }
            }
        }

        return true;
    }

    bool FunctionInlining::run(IRUnit *unit)
    {
        bool total_changed = false;
        std::set<Function *> modified_functions;

        while (true)
        {
            bool changed_this_iteration = false;
            std::vector<CallInst *> candidates;

            for (auto &func : unit->functions)
            {
                if (func->is_declaration())
                {
                    continue;
                }
                for (auto &bb : func->get_basic_blocks())
                {
                    for (auto &inst : bb->get_instructions())
                    {
                        if (auto call = dynamic_cast<CallInst *>(inst.get()))
                        {
                            candidates.push_back(call);
                        }
                    }
                }
            }

            for (CallInst *call : candidates)
            {
                Function *caller = call->get_parent()->get_parent();
                Value *callee_val = call->get_callee();
                Function *callee = dynamic_cast<Function *>(callee_val);

                if (callee && shouldInline(caller, callee))
                {
                    if (doInline(call, unit))
                    {
                        changed_this_iteration = true;
                        modified_functions.insert(caller);
                        break;
                    }
                }
            }

            if (changed_this_iteration)
            {
                total_changed = true;
            }
            else
            {
                break;
            }
        }

        if (total_changed)
        {
            for (Function *func : modified_functions)
            {
                int temp_reg_counter = 0;
                for (auto &bb : func->get_basic_blocks())
                {
                    for (auto &inst_ptr : bb->get_instructions())
                    {
                        if (inst_ptr->get_type()->get_type_id() != TypeID::Void)
                        {
                            inst_ptr->set_name(std::to_string(temp_reg_counter++));
                        }
                    }
                }
            }
        }

        return total_changed;
    }

    bool FunctionInlining::shouldInline(Function *caller, Function *callee)
    {
        if (callee->is_declaration())
        {
            return false;
        }

        if (caller == callee)
        {
            return false;
        }

        for (auto &bb : callee->get_basic_blocks())
        {
            for (auto &inst : bb->get_instructions())
            {
                if (auto call = dynamic_cast<CallInst *>(inst.get()))
                {
                    if (call->get_callee() == callee)
                    {
                        return false;
                    }
                }
            }
        }

        if (function_size_cache_.find(callee) == function_size_cache_.end())
        {
            int size = 0;
            for (auto &bb : callee->get_basic_blocks())
            {
                size += bb->get_instructions().size();
            }
            function_size_cache_[callee] = size;
        }

        return function_size_cache_[callee] < instruction_threshold_;
    }

    void FunctionInlining::replaceAllUsesWith(Value *old_val, Value *new_val, Function *scope)
    {
        if (old_val == new_val)
            return;
        for (auto &bb : scope->get_basic_blocks())
        {
            for (auto &inst : bb->get_instructions())
            {
                for (size_t i = 0; i < inst->get_num_operands(); ++i)
                {
                    if (inst->get_operand(i) == old_val)
                    {
                        inst->set_operand(i, new_val);
                    }
                }
            }
        }
    }

    BasicBlock *FunctionInlining::splitBasicBlock(BasicBlock *bb, Instruction *split_point_inst)
    {
        Function *F = bb->get_parent();
        auto &BBlis = F->get_basic_blocks();
        auto &I_lis = bb->get_instructions();

        auto split_point_it = I_lis.begin();
        while (split_point_it != I_lis.end())
        {
            if (split_point_it->get() == split_point_inst)
            {
                break;
            }
            ++split_point_it;
        }

        auto new_bb = std::make_shared<BasicBlock>(bb->get_name() + ".split", F);
        auto bb_it_in_func = std::find_if(BBlis.begin(), BBlis.end(), [bb](const auto &p)
                                          { return p.get() == bb; });
        auto new_bb_it = BBlis.insert(std::next(bb_it_in_func), new_bb);

        new_bb->get_instructions().splice(new_bb->get_instructions().begin(), I_lis, split_point_it, I_lis.end());

        for (auto &inst : new_bb->get_instructions())
        {
            inst->set_parent(new_bb.get());
        }

        Instruction *old_terminator = new_bb->get_instructions().back().get();
        if (auto br = dynamic_cast<BranchInst *>(old_terminator))
        {
            std::vector<BasicBlock *> successors;
            if (br->is_conditional())
            {
                successors.push_back(br->get_true_dest());
                successors.push_back(br->get_false_dest());
            }
            else
            {
                successors.push_back(br->get_uncond_dest());
            }

            for (auto *succ : successors)
            {
                for (auto &phi_inst_ptr : succ->get_instructions())
                {
                    if (phi_inst_ptr->get_opcode() == Opcode::PHI)
                    {
                        for (size_t i = 0; i < phi_inst_ptr->get_num_operands(); i += 2)
                        {
                            if (phi_inst_ptr->get_operand(i + 1) == reinterpret_cast<Value *>(bb))
                            {
                                phi_inst_ptr->set_operand(i + 1, reinterpret_cast<Value *>(new_bb.get()));
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        auto new_br_to_split = std::make_shared<BranchInst>(new_bb.get(), bb);
        bb->get_instructions().push_back(new_br_to_split);

        return new_bb.get();
    }

    bool FunctionInlining::doInline(CallInst *call, IRUnit *unit)
    {
        Function *caller = call->get_parent()->get_parent();
        BasicBlock *call_block = call->get_parent();
        Function *callee = dynamic_cast<Function *>(call->get_callee());

        BasicBlock *after_call_block = splitBasicBlock(call_block, call);
        call_block->get_instructions().pop_back();

        std::map<Value *, Value *> value_map;

        auto remap = [&](Value *v) -> Value *
        {
            if (!v)
                return nullptr;
            if (auto it = value_map.find(v); it != value_map.end())
            {
                return it->second;
            }
            return v;
        };

        auto callee_args = callee->get_arguments();
        for (size_t i = 0; i < callee_args.size(); ++i)
        {
            value_map[callee_args[i].get()] = call->get_operand(i + 1);
        }

        std::map<BasicBlock *, BasicBlock *> bb_map;
        std::vector<std::pair<Value *, BasicBlock *>> return_phis;
        int temp_reg_counter = 0;
        for (auto &bb : caller->get_basic_blocks())
        {
            for (auto &inst : bb->get_instructions())
            {
                const std::string &name = inst->get_name();
                if (!name.empty() && isdigit(name[0]))
                {
                    try
                    {
                        int num = std::stoi(name);
                        if (num >= temp_reg_counter)
                        {
                            temp_reg_counter = num + 1;
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        auto find_bb_iter = [&](BasicBlock *bb)
        {
            for (auto it = caller->get_basic_blocks().begin(); it != caller->get_basic_blocks().end(); ++it)
            {
                if (it->get() == bb)
                    return it;
            }
            return caller->get_basic_blocks().end();
        };
        auto after_call_block_iter = find_bb_iter(after_call_block);

        for (auto &bb : callee->get_basic_blocks())
        {
            std::string new_bb_name = callee->get_name() + "." + bb->get_name() + ".inlined." + std::to_string(inline_instance_counter_);
            auto new_bb = std::make_shared<BasicBlock>(new_bb_name, caller);
            caller->get_basic_blocks().insert(after_call_block_iter, new_bb);
            bb_map[bb.get()] = new_bb.get();
            value_map[bb.get()] = new_bb.get();
        }
        inline_instance_counter_++;

        for (auto &bb : callee->get_basic_blocks())
        {
            BasicBlock *new_bb = bb_map[bb.get()];
            for (auto &inst : bb->get_instructions())
            {
                std::shared_ptr<Instruction> new_inst;
                if (auto alloca_inst = dynamic_cast<AllocaInst *>(inst.get()))
                {
                    new_inst = std::make_shared<AllocaInst>(alloca_inst->get_allocated_type(), "", new_bb, alloca_inst->get_align());
                }
                else if (auto ret = dynamic_cast<ReturnInst *>(inst.get()))
                {
                    if (ret->has_return_value())
                    {
                        return_phis.push_back({remap(ret->get_operand(0)), new_bb});
                    }
                    new_inst = std::make_shared<BranchInst>(after_call_block, new_bb);
                }
                else if (auto bin_inst = dynamic_cast<BinaryInst *>(inst.get()))
                {
                    new_inst = std::make_shared<BinaryInst>(bin_inst->get_opcode(), remap(bin_inst->get_operand(0)), remap(bin_inst->get_operand(1)), "", new_bb);
                }
                else if (auto cmp_inst = dynamic_cast<CmpInst *>(inst.get()))
                {
                    new_inst = std::make_shared<CmpInst>(cmp_inst->get_type(), cmp_inst->get_predicate(), remap(cmp_inst->get_operand(0)), remap(cmp_inst->get_operand(1)), "", new_bb);
                }
                else if (auto cast_inst = dynamic_cast<CastInst *>(inst.get()))
                {
                    new_inst = std::make_shared<CastInst>(cast_inst->get_opcode(), remap(cast_inst->get_operand(0)), cast_inst->get_type(), "", new_bb);
                }
                else if (auto gep_inst = dynamic_cast<GetElementPtrInst *>(inst.get()))
                {
                    std::vector<Value *> indices;
                    for (size_t i = 1; i < gep_inst->get_num_operands(); ++i)
                    {
                        indices.push_back(remap(gep_inst->get_operand(i)));
                    }
                    new_inst = std::make_shared<GetElementPtrInst>(remap(gep_inst->get_operand(0)), indices, gep_inst->is_inbounds(), "", new_bb);
                }
                else if (auto load_inst = dynamic_cast<LoadInst *>(inst.get()))
                {
                    new_inst = std::make_shared<LoadInst>(remap(load_inst->get_operand(0)), "", new_bb, load_inst->get_align());
                }
                else if (auto store_inst = dynamic_cast<StoreInst *>(inst.get()))
                {
                    new_inst = std::make_shared<StoreInst>(remap(store_inst->get_operand(0)), remap(store_inst->get_operand(1)), new_bb, store_inst->get_align());
                }
                else if (auto br_inst = dynamic_cast<BranchInst *>(inst.get()))
                {
                    if (br_inst->is_conditional())
                    {
                        new_inst = std::make_shared<BranchInst>(remap(br_inst->get_operand(0)), bb_map.at(br_inst->get_true_dest()), bb_map.at(br_inst->get_false_dest()), new_bb);
                    }
                    else
                    {
                        new_inst = std::make_shared<BranchInst>(bb_map.at(br_inst->get_uncond_dest()), new_bb);
                    }
                }
                else if (inst->get_opcode() == Opcode::PHI)
                {
                    new_inst = std::make_shared<Instruction>(inst->get_type(), inst->get_opcode(), 0, "", new_bb);
                }
                else if (auto call_inst = dynamic_cast<CallInst *>(inst.get()))
                {
                    std::vector<Value *> args;
                    for (size_t i = 1; i < call_inst->get_num_operands(); ++i)
                    {
                        args.push_back(remap(call_inst->get_operand(i)));
                    }
                    new_inst = std::make_shared<CallInst>(remap(call_inst->get_callee()), args, "", new_bb);
                }

                if (new_inst)
                {
                    if (new_inst->get_type()->get_type_id() != TypeID::Void)
                    {
                        new_inst->set_name(std::to_string(temp_reg_counter++));
                    }
                    new_bb->get_instructions().push_back(new_inst);
                    value_map[inst.get()] = new_inst.get();
                }
            }
        }

        for (auto &bb_ptr : callee->get_basic_blocks())
        {
            BasicBlock *new_bb = bb_map[bb_ptr.get()];
            auto new_inst_it = new_bb->get_instructions().begin();
            for (auto &inst_ptr : bb_ptr->get_instructions())
            {
                if (inst_ptr->get_opcode() == Opcode::PHI)
                {
                    Instruction *new_phi = new_inst_it->get();
                    for (size_t i = 0; i < inst_ptr->get_num_operands(); i += 2)
                    {
                        Value *val = remap(inst_ptr->get_operand(i));
                        Value *pred_bb = remap(inst_ptr->get_operand(i + 1));
                        new_phi->add_operand(val);
                        new_phi->add_operand(pred_bb);
                    }
                    ++new_inst_it;
                }
            }
        }

        auto branch_to_callee = std::make_shared<BranchInst>(bb_map.at(callee->get_basic_blocks().front().get()), call_block);
        call_block->get_instructions().push_back(branch_to_callee);

        if (call->get_type()->get_type_id() != TypeID::Void)
        {
            if (return_phis.size() == 1)
            {
                replaceAllUsesWith(call, return_phis[0].first, caller);
            }
            else if (return_phis.size() > 1)
            {
                auto phi = std::make_shared<Instruction>(call->get_type(), Opcode::PHI, 0, std::to_string(temp_reg_counter++), after_call_block);
                for (auto &pair : return_phis)
                {
                    phi->add_operand(pair.first);
                    phi->add_operand(reinterpret_cast<Value *>(pair.second));
                }
                after_call_block->get_instructions().push_front(phi);
                replaceAllUsesWith(call, phi.get(), caller);
            }
        }

        after_call_block->get_instructions().remove_if([&](const auto &p)
                                                       { return p.get() == call; });

        return true;
    }

    bool Loop::isLoopInvariant(Value *val, const std::set<Instruction *> &invariant_instructions) const
    {
        if (dynamic_cast<Constant *>(val) || dynamic_cast<Argument *>(val) || dynamic_cast<GlobalVariable *>(val))
        {
            return true;
        }

        if (auto inst = dynamic_cast<Instruction *>(val))
        {
            if (invariant_instructions.count(inst))
            {
                return true;
            }
            if (!contains(inst->get_parent()))
            {
                return true;
            }
        }

        return false;
    }

    Loop *LoopAnalysis::getLoopFor(BasicBlock *bb) const
    {
        auto it = block_to_loop_map_.find(bb);
        if (it != block_to_loop_map_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void LoopAnalysis::run(Function *func, const CFGAnalysis &cfg, const DominatorTree &dom_tree)
    {
        loops_.clear();
        block_to_loop_map_.clear();
        findLoops(func, cfg, dom_tree);

        for (auto &loop : loops_)
        {
            for (BasicBlock *bb : loop->getBlocks())
            {
                block_to_loop_map_[bb] = loop.get();
                for (BasicBlock *succ : cfg.getSuccessors(bb))
                {
                    if (!loop->contains(succ))
                    {
                        loop->addExitBlock(bb);
                        break;
                    }
                }
            }
        }

        std::vector<Loop *> loops_to_process;
        for (auto &loop_ptr : loops_)
        {
            loops_to_process.push_back(loop_ptr.get());
        }

        for (Loop *loop : loops_to_process)
        {
            BasicBlock *header = loop->getHeader();
            std::vector<BasicBlock *> outside_predecessors;
            for (BasicBlock *pred : cfg.getPredecessors(header))
            {
                if (!loop->contains(pred))
                {
                    outside_predecessors.push_back(pred);
                }
            }

            if (outside_predecessors.empty())
            {
                continue;
            }

            if (outside_predecessors.size() == 1)
            {
                BasicBlock *p = outside_predecessors[0];
                if (cfg.getSuccessors(p).size() == 1)
                {
                    loop->setPreheader(p);
                    continue;
                }
            }

            std::string preheader_name = header->get_name() + ".preheader";
            auto preheader_bb_ptr = std::make_shared<BasicBlock>(preheader_name, func);
            BasicBlock *preheader_bb = preheader_bb_ptr.get();
            loop->setPreheader(preheader_bb);

            preheader_bb->get_instructions().push_back(std::make_shared<BranchInst>(header, preheader_bb));

            for (BasicBlock *pred : outside_predecessors)
            {
                auto terminator = pred->get_instructions().back();
                for (size_t i = 0; i < terminator->get_num_operands(); ++i)
                {
                    if (terminator->get_opcode() == Opcode::Br && reinterpret_cast<Value *>(header) == terminator->get_operand(i))
                    {
                        terminator->set_operand(i, reinterpret_cast<Value *>(preheader_bb));
                    }
                }
            }

            for (auto &inst_ptr : header->get_instructions())
            {
                if (inst_ptr->get_opcode() != Opcode::PHI)
                {
                    break;
                }
                Instruction *phi = inst_ptr.get();
                std::vector<std::pair<Value *, BasicBlock *>> incoming_from_outside;

                for (BasicBlock *pred : outside_predecessors)
                {
                    for (size_t i = 0; i < phi->get_num_operands(); i += 2)
                    {
                        if (phi->get_operand(i + 1) == reinterpret_cast<Value *>(pred))
                        {
                            incoming_from_outside.push_back({phi->get_operand(i), pred});
                            break;
                        }
                    }
                }

                if (incoming_from_outside.empty())
                    continue;

                Value *value_from_preheader;
                if (incoming_from_outside.size() > 1)
                {
                    auto preheader_phi = std::make_shared<Instruction>(phi->get_type(), Opcode::PHI, 0, phi->get_name() + ".pre");
                    preheader_phi->set_parent(preheader_bb);
                    for (const auto &pair : incoming_from_outside)
                    {
                        preheader_phi->add_operand(pair.first);
                        preheader_phi->add_operand(reinterpret_cast<Value *>(pair.second));
                    }
                    preheader_bb->get_instructions().push_front(preheader_phi);
                    value_from_preheader = preheader_phi.get();
                }
                else
                {
                    value_from_preheader = incoming_from_outside[0].first;
                }

                for (int i = phi->get_num_operands() - 2; i >= 0; i -= 2)
                {
                    auto incoming_bb = reinterpret_cast<BasicBlock *>(phi->get_operand(i + 1));
                    if (std::find(outside_predecessors.begin(), outside_predecessors.end(), incoming_bb) != outside_predecessors.end())
                    {
                        phi->remove_operands(i, 2);
                    }
                }

                if (value_from_preheader)
                {
                    phi->add_operand(value_from_preheader);
                    phi->add_operand(reinterpret_cast<Value *>(preheader_bb));
                }
            }

            auto header_it = std::find_if(func->get_basic_blocks().begin(), func->get_basic_blocks().end(),
                                          [&](const auto &p)
                                          { return p.get() == header; });
            func->get_basic_blocks().insert(header_it, preheader_bb_ptr);
        }
    }

    void LoopAnalysis::findLoops(Function *func, const CFGAnalysis &cfg, const DominatorTree &dom_tree)
    {
        for (auto &bb_ptr : func->get_basic_blocks())
        {
            BasicBlock *bb = bb_ptr.get();
            for (BasicBlock *succ : cfg.getSuccessors(bb))
            {
                if (dom_tree.dominates(succ, bb))
                {
                    BasicBlock *header = succ;
                    bool found = false;
                    for (const auto &loop : loops_)
                    {
                        if (loop->getHeader() == header)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        continue;

                    auto loop = std::make_unique<Loop>(header);
                    std::stack<BasicBlock *> worklist;
                    loop->addBlock(header);
                    if (header != bb)
                    {
                        loop->addBlock(bb);
                        worklist.push(bb);
                    }

                    while (!worklist.empty())
                    {
                        BasicBlock *current = worklist.top();
                        worklist.pop();
                        for (auto pred : cfg.getPredecessors(current))
                        {
                            if (!loop->contains(pred))
                            {
                                loop->addBlock(pred);
                                worklist.push(pred);
                            }
                        }
                    }
                    loops_.push_back(std::move(loop));
                }
            }
        }
    }

    void LoopAnalysis::createPreheaders(Function *func, CFGAnalysis &cfg)
    {
        std::vector<Loop *> loops_to_process;
        for (auto &loop_ptr : loops_)
        {
            loops_to_process.push_back(loop_ptr.get());
        }

        for (Loop *loop : loops_to_process)
        {
            BasicBlock *header = loop->getHeader();
            std::vector<BasicBlock *> outside_predecessors;
            for (BasicBlock *pred : cfg.getPredecessors(header))
            {
                if (!loop->contains(pred))
                {
                    outside_predecessors.push_back(pred);
                }
            }

            if (outside_predecessors.size() == 0)
            {
                continue;
            }

            if (outside_predecessors.size() == 1)
            {
                BasicBlock *p = outside_predecessors[0];
                if (cfg.getSuccessors(p).size() == 1)
                {
                    loop->setPreheader(p);
                    continue;
                }
            }

            std::string preheader_name = header->get_name() + ".preheader";
            auto preheader_bb_ptr = std::make_shared<BasicBlock>(preheader_name, func);
            BasicBlock *preheader_bb = preheader_bb_ptr.get();
            loop->setPreheader(preheader_bb);

            preheader_bb->get_instructions().push_back(std::make_shared<BranchInst>(header, preheader_bb));

            for (BasicBlock *pred : outside_predecessors)
            {
                auto terminator = pred->get_instructions().back();
                for (size_t i = 0; i < terminator->get_num_operands(); ++i)
                {
                    if (terminator->get_opcode() == Opcode::Br && reinterpret_cast<Value *>(header) == terminator->get_operand(i))
                    {
                        terminator->set_operand(i, reinterpret_cast<Value *>(preheader_bb));
                    }
                }
            }

            for (auto &inst_ptr : header->get_instructions())
            {
                if (inst_ptr->get_opcode() == Opcode::PHI)
                {
                    Instruction *phi = inst_ptr.get();
                    Value *val_from_preheader = nullptr;

                    std::vector<BasicBlock *> preds_to_reroute;
                    for (BasicBlock *p : outside_predecessors)
                    {
                        preds_to_reroute.push_back(p);
                    }

                    for (int i = phi->get_num_operands() - 2; i >= 0; i -= 2)
                    {
                        auto incoming_bb = reinterpret_cast<BasicBlock *>(phi->get_operand(i + 1));
                        auto it = std::find(preds_to_reroute.begin(), preds_to_reroute.end(), incoming_bb);
                        if (it != preds_to_reroute.end())
                        {
                            if (!val_from_preheader)
                            {
                                val_from_preheader = phi->get_operand(i);
                            }
                            phi->remove_operands(i, 2);
                        }
                    }

                    if (val_from_preheader)
                    {
                        phi->add_operand(val_from_preheader);
                        phi->add_operand(reinterpret_cast<Value *>(preheader_bb));
                    }
                }
                else
                {
                    break;
                }
            }

            auto header_it = std::find_if(func->get_basic_blocks().begin(), func->get_basic_blocks().end(),
                                          [&](const auto &p)
                                          { return p.get() == header; });
            func->get_basic_blocks().insert(header_it, preheader_bb_ptr);
        }
    }

    bool LoopInvariantCodeMotion::runOnFunction(Function *func)
    {
        bool changed = false;
        cfg_.run(func);
        dom_tree_.run(func, cfg_);
        loop_analysis_.run(func, cfg_, dom_tree_);

        // The CFG might have been changed by preheader creation. For the dominance checks during
        // motion to be correct, the analyses must be up-to-date.
        // So, we re-run them if preheaders were potentially created.
        bool need_reanalysis = false;
        for (const auto &loop : loop_analysis_.getLoops())
        {
            if (loop->getPreheader())
            {
                // A very conservative check. If any loop has a preheader, we assume CFG could have changed.
                need_reanalysis = true;
                break;
            }
        }

        if (need_reanalysis)
        {
            cfg_.run(func);
            dom_tree_.run(func, cfg_);
        }

        std::vector<Loop *> loops;
        for (const auto &loop_ptr : loop_analysis_.getLoops())
        {
            loops.push_back(loop_ptr.get());
        }

        std::sort(loops.begin(), loops.end(), [](Loop *a, Loop *b)
                  { return a->getBlocks().size() > b->getBlocks().size(); });

        for (Loop *loop : loops)
        {
            changed |= runOnLoop(loop, func);
        }

        return changed;
    }

    bool LoopInvariantCodeMotion::runOnLoop(Loop *loop, Function *func)
    {
        BasicBlock *preheader = loop->getPreheader();
        if (!preheader)
        {
            return false;
        }

        bool changed_this_loop = false;
        std::set<Instruction *> invariant_instructions;
        std::vector<Instruction *> motion_candidates;

        for (auto *bb : loop->getBlocks())
        {

            if (loop_analysis_.getLoopFor(bb) == loop)
            {
                for (auto &inst_ptr : bb->get_instructions())
                {
                    motion_candidates.push_back(inst_ptr.get());
                }
            }
        }

        bool iter_changed = true;
        while (iter_changed)
        {
            iter_changed = false;
            std::vector<Instruction *> newly_found_invariants;
            for (Instruction *inst : motion_candidates)
            {
                bool operands_are_invariant = true;
                for (size_t i = 0; i < inst->get_num_operands(); ++i)
                {
                    if (!loop->isLoopInvariant(inst->get_operand(i), invariant_instructions))
                    {
                        operands_are_invariant = false;
                        break;
                    }
                }

                if (operands_are_invariant)
                {
                    Opcode op = inst->get_opcode();
                    if (op != Opcode::Store && op != Opcode::Call && op != Opcode::Ret && op != Opcode::Br && op != Opcode::PHI && op != Opcode::Load)
                    {
                        newly_found_invariants.push_back(inst);
                    }
                }
            }

            if (!newly_found_invariants.empty())
            {
                iter_changed = true;
                for (Instruction *inst : newly_found_invariants)
                {
                    invariant_instructions.insert(inst);
                    motion_candidates.erase(std::remove(motion_candidates.begin(), motion_candidates.end(), inst), motion_candidates.end());
                }
            }
        }

        if (!invariant_instructions.empty())
        {
            std::vector<Instruction *> instructions_to_move;
            for (auto *bb : loop->getBlocks())
            {
                if (loop_analysis_.getLoopFor(bb) == loop)
                {
                    for (auto &inst_ptr : bb->get_instructions())
                    {
                        Instruction *inst = inst_ptr.get();
                        if (invariant_instructions.count(inst))
                        {
                            BasicBlock *inst_block = inst->get_parent();
                            bool dominates_all_exits = true;
                            const auto &exit_blocks = loop->getExitBlocks();

                            if (!exit_blocks.empty())
                            {
                                for (BasicBlock *exit_block : exit_blocks)
                                {
                                    if (!dom_tree_.dominates(inst_block, exit_block))
                                    {
                                        dominates_all_exits = false;
                                        break;
                                    }
                                }
                            }

                            if (dominates_all_exits)
                            {
                                instructions_to_move.push_back(inst);
                            }
                        }
                    }
                }
            }

            if (!instructions_to_move.empty())
            {
                auto &preheader_insts = preheader->get_instructions();
                auto terminator_it = std::prev(preheader_insts.end());

                for (Instruction *inst : instructions_to_move)
                {
                    auto &source_block_insts = inst->get_parent()->get_instructions();
                    auto inst_it = std::find_if(source_block_insts.begin(), source_block_insts.end(),
                                                [inst](const auto &p)
                                                { return p.get() == inst; });

                    if (inst_it != source_block_insts.end())
                    {
                        preheader_insts.splice(terminator_it, source_block_insts, inst_it);
                        inst->set_parent(preheader);
                        changed_this_loop = true;
                    }
                }
            }
        }

        return changed_this_loop;
    }

    bool AllocaHoisting::runOnFunction(Function *func)
    {
        if (func->get_basic_blocks().empty())
        {
            return false;
        }

        BasicBlock *entry_block = func->get_basic_blocks().front().get();
        std::vector<std::shared_ptr<Instruction>> allocas_to_move;
        bool changed = false;

        for (auto &bb_ptr : func->get_basic_blocks())
        {
            if (bb_ptr.get() == entry_block)
            {
                continue;
            }

            auto &instructions = bb_ptr->get_instructions();
            for (auto it = instructions.begin(); it != instructions.end();)
            {
                if (dynamic_cast<AllocaInst *>(it->get()))
                {
                    allocas_to_move.push_back(*it);
                    it = instructions.erase(it);
                    changed = true;
                }
                else
                {
                    ++it;
                }
            }
        }

        if (changed)
        {
            auto &entry_instructions = entry_block->get_instructions();
            auto first_non_alloca = entry_instructions.begin();
            while (first_non_alloca != entry_instructions.end() &&
                   dynamic_cast<AllocaInst *>(first_non_alloca->get()))
            {
                ++first_non_alloca;
            }

            for (auto &alloca_ptr : allocas_to_move)
            {
                alloca_ptr->set_parent(entry_block);
                entry_instructions.insert(first_non_alloca, alloca_ptr);
            }
        }

        return changed;
    }

} // namespace MyIR