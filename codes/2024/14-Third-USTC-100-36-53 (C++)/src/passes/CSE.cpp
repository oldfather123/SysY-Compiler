#include "CSE.hpp"

#include <unordered_set>
#include <algorithm>

void CSE::run(Function *func, AnalysisPassManager &APM) {
    func_info_ = APM.getResult<FuncInfo>(func->get_parent());
    dom_info_ = APM.getResult<Dominators>(func);

    insert_forward_load(func);

    // forward store
    do {
        for (auto &BB : func->get_basic_blocks())
            while (local_cse(&BB))
                ;
    } while (global_cse(func));

    // delete forward
    remove_forward_load(func);

    forward_loads_.clear();
    inst_to_delete_.clear();

    // just invalidate all, although it slow down everything
    APM.invalidateAll(func);
    APM.invalidateAll(func->get_parent());
}

bool CSE::local_cse(BasicBlock *bb) {
    // Use hash map / ilist_node::prev ?
    std::vector<Instruction *> pre_instr;
    std::vector<Instruction *> inst_to_delete;

    for (auto &I : bb->get_instructions()) {
        auto *instr = &I;
        pre_instr.push_back(instr);

        if (!can_delete(instr))
            continue;

        for (auto it = pre_instr.rbegin() + 1; it != pre_instr.rend(); ++it) {
            auto pre = *it;
            if (may_kill(pre, instr))
                break;
            if (trivial_common_expr(pre, instr)) {
                instr->replace_all_use_with(pre);
                pre_instr.pop_back();
                inst_to_delete.push_back(instr);
                break;
            }
        }
    }

    bool changed = !inst_to_delete.empty();
    for (auto inst : inst_to_delete)
        inst->get_parent()->erase_instr(inst);
    return changed;
}

bool CSE::global_cse(Function *func) {
    std::vector<Instruction *> all_expr;
    std::vector<LoadInst *> all_loads;
    // collect all loads
    for (auto &BB : func->get_basic_blocks()) {
        for (auto &I : BB.get_instructions()) {
            if (!can_delete(&I))
                continue;
            all_expr.push_back(&I);
            if (auto load = dynamic_cast<LoadInst *>(&I))
                all_loads.push_back(load);
        }
    }

    // just use vector?
    struct BlockInfo {
        std::vector<Instruction *> gens;
        std::vector<Instruction *> kills;
        std::vector<Instruction *> ins;
        std::vector<Instruction *> outs;
    };
    std::unordered_map<BasicBlock *, BlockInfo> block_info;

    // 2. gen, kill
    for (auto &BB : func->get_basic_blocks()) {
        std::vector<Instruction *> gens;
        std::vector<Instruction *> kills;

        for (auto &I : BB.get_instructions()) {
            auto *instr = &I;
            if (instr->is_store() || instr->is_call()) {
                for (auto load : all_loads)
                    if (may_kill(instr, load))
                        kills.push_back(load);
                for (auto it = gens.begin(); it != gens.end();)
                    if (may_kill(instr, *it))
                        it = gens.erase(it);
                    else
                        ++it;
            } else if (can_delete(instr))
                gens.push_back(instr);
        }

        std::sort(gens.begin(), gens.end());
        std::sort(kills.begin(), kills.end());
        block_info[&BB].gens = std::move(gens);
        block_info[&BB].kills = std::move(kills);
    }

    // 3. in, out
    std::sort(all_expr.begin(), all_expr.end());
    for (auto &BB : func->get_basic_blocks())
        block_info[&BB].outs = all_expr;

    bool changed;
    do {
        changed = false;

        for (auto &BB : func->get_basic_blocks()) {
            std::vector<Instruction *> ins;
            std::vector<Instruction *> outs;
            std::vector<Instruction *> tmp;

            // TODO: inplace intersect, difference, union
            auto &pres = BB.get_pre_basic_blocks();
            if (!pres.empty()) {
                ins = block_info[pres.front()].outs;
                for (auto it = std::next(pres.begin()); it != pres.end();
                     ++it) {
                    auto pre_outs = block_info[*it].outs;
                    std::set_intersection(ins.begin(), ins.end(),
                                          pre_outs.begin(), pre_outs.end(),
                                          std::back_inserter(tmp));
                    std::swap(tmp, ins);
                    tmp.clear();
                }
            }

            block_info[&BB].ins = ins;

            outs = std::move(ins);
            auto &kills = block_info[&BB].kills;
            std::set_difference(outs.begin(), outs.end(), kills.begin(),
                                kills.end(), std::back_inserter(tmp));
            std::swap(outs, tmp);
            tmp.clear();

            auto &gens = block_info[&BB].gens;
            std::set_union(outs.begin(), outs.end(), gens.begin(), gens.end(),
                           std::back_inserter(tmp));
            outs.erase(std::unique(outs.begin(), outs.end()), outs.end());
            outs = std::move(tmp);

            if (outs != block_info[&BB].outs) {
                block_info[&BB].outs = std::move(outs);
                changed = true;
            }
        }
    } while (changed);

    std::unordered_set<Instruction *> inst_delete_set;
    for (auto &BB : func->get_basic_blocks()) {
        std::vector<Instruction *> avail = block_info[&BB].ins;
        for (auto &I : BB.get_instructions()) {
            auto *instr = &I;
            if (instr->is_store() || instr->is_call()) {
                for (auto it = avail.begin(); it != avail.end();) {
                    if (may_kill(instr, *it))
                        it = avail.erase(it);
                    else
                        ++it;
                }
            }
            if (!can_delete(instr))
                continue;

            for (auto expr : avail) {
                if (expr == instr ||
                    inst_delete_set.find(expr) != inst_delete_set.end())
                    continue;
                if (trivial_common_expr(expr, instr)) {
                    instr->replace_all_use_with(expr);
                    inst_delete_set.insert(instr);
                    break;
                }
            }
        }
    }

    changed = !inst_delete_set.empty();
    for (auto inst : inst_delete_set) {
        inst->get_parent()->erase_instr(inst);
    }
    return changed;
}

void CSE::insert_forward_load(Function *func) {
    for (auto &BB : func->get_basic_blocks()) {
        for (auto &I : BB.get_instructions()) {
            if (auto *store = dynamic_cast<StoreInst *>(&I)) {
                auto lval = store->get_lval();
                auto load = LoadInst::create_load(lval, nullptr);
                load->insert_before(store->getNext());
                forward_loads_.emplace(load, store);
            }
        }
    }
}

void CSE::remove_forward_load(Function *func) {
    // forward loads should no be deleted by CSE
    for (auto &[load, store] : forward_loads_) {
        load->replace_all_use_with(store->get_rval());
        load->get_parent()->erase_instr(load);
    }
}

bool CSE::may_kill(Instruction *instr, Instruction *load) {
    if (!load->is_load())
        return false;

    if (!(instr->is_store() || instr->is_call()))
        return false;

    // TODO: alias analysis

    // alloca(local), global, argument
    auto *load_addr = static_cast<LoadInst *>(load)->get_lval();
    if (instr->is_store()) {
        auto *store_addr = static_cast<StoreInst *>(instr)->get_lval();

        // gep with different last index
        // assume no overflow / underflow
        auto load_addr_gep = dynamic_cast<GetElementPtrInst *>(load_addr);
        auto store_addr_gep = dynamic_cast<GetElementPtrInst *>(store_addr);
        if (load_addr_gep && store_addr_gep) {
            auto idx1 = load_addr_gep->get_operands().back();
            auto idx2 = store_addr_gep->get_operands().back();

            if (auto ci1 = dynamic_cast<ConstantInt *>(idx1)) {
                if (auto ci2 = dynamic_cast<ConstantInt *>(idx2)) {
                    if (ci1->get_value() != ci2->get_value())
                        return false;
                }
            }
        }

        auto *load_addr_base = get_base_addr(load_addr);
        auto *store_addr_base = get_base_addr(store_addr);
        if (load_addr_base == store_addr_base)
            return true;

        // local
        if (dynamic_cast<AllocaInst *>(load_addr_base) ||
            dynamic_cast<AllocaInst *>(store_addr_base))
            return false;

        // different global or non-array
        auto gbl1 = dynamic_cast<GlobalVariable *>(load_addr_base);
        auto gbl2 = dynamic_cast<GlobalVariable *>(store_addr_base);
        if (gbl1 && gbl2)
            return false;
        if (gbl1 && !gbl1->get_init()->get_type()->is_array_type())
            return false;
        if (gbl2 && !gbl2->get_init()->get_type()->is_array_type())
            return false;

        // argument or global array
        return true;
    }

    if (instr->is_call()) {
        auto *callee = static_cast<Function *>(instr->get_operand(0));
        if (func_info_->is_pure_function(callee))
            return false;

        // TODO: arguments write
        return true;
    }

    assert(false && "maybe case");
}

Value *CSE::get_base_addr(Value *val) {
    Value *p = val;
    while (auto *gep = dynamic_cast<GetElementPtrInst *>(p))
        p = gep->get_operand(0);
    return p;
}

bool CSE::trivial_common_expr(Instruction *instr1, Instruction *instr2) {
    if (instr1->get_instr_type() != instr2->get_instr_type())
        return false;
    auto &opd1 = instr1->get_operands();
    auto &opd2 = instr2->get_operands();

    if (opd1.size() != opd2.size())
        return false;

    if (instr1->is_communicative()) {
        return (opd1[0] == opd2[0] && opd1[1] == opd2[1]) ||
               (opd1[0] == opd2[1] && opd1[1] == opd2[0]);
    }

    // phi instruction?
    for (size_t i = 0; i < opd1.size(); ++i) {
        if (opd1[i] != opd2[i])
            return false;
    }
    return true;
}

bool CSE::can_delete(Instruction *instr) {
    if (instr->is_terminator() || instr->is_store() || instr->is_alloca())
        return false;
    if (instr->is_call()) {
        auto *callee = static_cast<Function *>(instr->get_operand(0));
        return func_info_->is_pure_function(callee);
    }
    return true;
}