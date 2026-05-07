#include "Mem2Reg.hpp"
#include "IRBuilder.hpp"
#include "Value.hpp"
#include "Constant.hpp"
#include <memory>

#include <queue>

void Mem2Reg::run()
{
    dominators_ = std::make_unique<Dominators>(m_);
    dominators_->run();
    for (auto &f : m_->get_functions())
    {
        if (f.is_declaration())
            continue;
        func_ = &f;
        if (func_->get_basic_blocks().size() >= 1)
        {
            generate_phi();
            rename(func_->get_entry_block());
        }
    }
    // allocas_.clear();
    // store_blocks_.clear();
    // phi_nodes_.clear();
    // def_stack_.clear();

    // for (auto *alloca : allocas_) {
    //     auto &bb = *alloca->get_parent();
    //     auto &insts = bb.get_instructions();
    //     auto it = std::find_if(insts.begin(), insts.end(),
    //                            [&](Instruction &i) { return &i == alloca; });
    //     if (it != insts.end()) {
    //         insts.erase(it);
    //     }
    // }
}

void Mem2Reg::generate_phi()
{
    allocas_.clear();
    store_blocks_.clear();
    phi_nodes_.clear();

    // 1. 找出 alloca 和 store
    for (auto &bb : func_->get_basic_blocks())
    {
        for (auto &inst : bb.get_instructions())
        {
            // for (auto &inst : bb.get_instructions()) {
            //     std::cerr << "Inst: " << inst.get_instr_op_name(); // 打印指令名
            //     // 打印指令指针或者操作数
            //     for (int i = 0; i < inst.get_num_operands(); i++) {
            //         Value* op = inst.get_operand(i);
            //         std::cerr << "  operand[" << i << "] type: " << op->get_type()->print() << "\n";
            //     }
            //     std::cerr << "\n";
            // }
            if (auto alloca = dynamic_cast<AllocaInst *>(&inst))
            {
                if (alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
                    alloca->get_type()->get_pointer_element_type()->is_float_type() || 
                    alloca->get_type()->get_pointer_element_type()->is_pointer_type())
                {
                    // std::cerr << "Found alloca at: " << static_cast<void*>(alloca) << "\n";
                    allocas_.push_back(alloca);
                }
            }
            if (auto store = dynamic_cast<StoreInst *>(&inst))
            {
                auto ptr = store->get_lval();
                if (auto alloca = dynamic_cast<AllocaInst *>(ptr))
                {
                    if (alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
                        alloca->get_type()->get_pointer_element_type()->is_float_type() || 
                        alloca->get_type()->get_pointer_element_type()->is_pointer_type())
                    {
                        // std::cerr << "  -> It's an alloca: " << static_cast<void*>(alloca) << "\n";
                        store_blocks_[alloca].insert(&bb);
                    }
                }
            }
        }
    }

    // 2. 为每个 alloca 变量计算 phi 函数应插入的位置
    // for (auto *alloca : allocas_) {
    //     std::set<BasicBlock*> worklist = store_blocks_[alloca];
    //     std::set<BasicBlock*> visited;
    //     while (!worklist.empty()) {
    //         BasicBlock *bb = *worklist.begin();
    //         worklist.erase(worklist.begin());

    //         for (auto df_bb : dominators_->get_dominance_frontier(bb)) {
    //             if (phi_nodes_[df_bb].count(alloca) == 0) {
    //                 // 创建与 alloca 指向类型相同的 phi 指令
    //                 auto phi_type = alloca->get_type()->get_pointer_element_type();
    //                 auto phi = PhiInst::create_phi(phi_type, df_bb);
    //                 phi_nodes_[df_bb][alloca] = phi;

    //                 // df_bb->get_instructions().insert(df_bb->get_instructions().begin(), phi);
    //                 df_bb->add_instr_begin(phi);
    //                 std::cerr << phi <<"    " <<df_bb << "\n";
    //             }
    //         }
    //     }
    // }
    for (auto *alloca : allocas_)
    {
        std::set<BasicBlock *> worklist = store_blocks_[alloca];
        std::set<BasicBlock *> visited;

        while (!worklist.empty())
        {
            BasicBlock *bb = *worklist.begin();
            worklist.erase(worklist.begin());

            for (auto df_bb : dominators_->get_dominance_frontier(bb))
            {
                // 如果该 alloca 在 df_bb 还没有插入 phi
                if (phi_nodes_[df_bb].count(alloca) == 0)
                {
                    // 插入 phi
                    auto phi_type = alloca->get_type()->get_pointer_element_type();
                    auto phi = PhiInst::create_phi(phi_type, df_bb);
                    phi_nodes_[df_bb][alloca] = phi;
                    df_bb->add_instr_begin(phi);
                    // std::cerr << "Insert phi for alloca " << alloca << " in block " << df_bb << "\n";

                    // 避免重复加入
                    if (visited.count(df_bb) == 0)
                    {
                        worklist.insert(df_bb);
                        visited.insert(df_bb);
                    }
                }
            }
        }
    }
}

void Mem2Reg::rename(BasicBlock *bb)
{
    // std::cerr << "rename on BB " << bb->get_name() << " " << bb << std::endl;
    std::vector<std::pair<AllocaInst *, Value *>> new_defs;

    // 1. 处理 phi 指令
    for (auto &inst : bb->get_instructions())
    {
        // std::cerr << inst.get_instr_op_name() << " " << &inst << std::endl;
        if (auto phi = dynamic_cast<PhiInst *>(&inst))
        {
            for (auto &[alloca, phi_inst] : phi_nodes_[bb])
            {
                if (phi_inst == phi)
                {
                    def_stack_[alloca].push_back(phi);
                    new_defs.emplace_back(alloca, phi);
                    break;
                }
            }
        }
    }

    // 2. 替换 load 和记录 store
    auto &instrs = bb->get_instructions();
    for (auto it = instrs.begin(); it != instrs.end();)
    {
        Instruction *inst = &(*it);

        if (auto load = dynamic_cast<LoadInst *>(inst))
        {
            // auto ptr = load->get_lval();
            // std::cerr << "Processing Load: " << load << " from " << ptr << "\n";
            if (auto alloca = dynamic_cast<AllocaInst *>(load->get_lval()))
            {
                if ((alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
                     alloca->get_type()->get_pointer_element_type()->is_float_type() ||
                      alloca->get_type()->get_pointer_element_type()->is_pointer_type()) &&
                    !def_stack_[alloca].empty())
                {
                    Value *latest = def_stack_[alloca].back();
                    load->replace_all_use_with(latest);
                    it = instrs.erase(it);
                    continue;
                }
            }
        }
        else if (auto store = dynamic_cast<StoreInst *>(inst))
        {
            // auto ptr = store->get_lval();
            // std::cerr << "Processing Store: " << store << " to " << ptr << "\n";
            if (auto alloca = dynamic_cast<AllocaInst *>(store->get_lval()))
            {
                if (alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
                    alloca->get_type()->get_pointer_element_type()->is_float_type() ||
                     alloca->get_type()->get_pointer_element_type()->is_pointer_type())
                {
                    Value *val = store->get_rval();
                    def_stack_[alloca].push_back(val);
                    new_defs.emplace_back(alloca, val);
                    it = instrs.erase(it);
                    continue;
                }
            }
        }
        ++it;
    }

    // // 3. 处理后继基本块的 phi 指令
    Instruction *term = &bb->get_instructions().back();
    std::vector<BasicBlock *> successors;
    if (auto br = dynamic_cast<BranchInst *>(term))
    {
        if (br->is_cond_br())
        {
            successors.push_back(dynamic_cast<BasicBlock *>(br->get_operand(1)));
            successors.push_back(dynamic_cast<BasicBlock *>(br->get_operand(2)));
        }
        else
        {
            successors.push_back(dynamic_cast<BasicBlock *>(br->get_operand(0)));
        }
    }

    for (auto succ : successors)
    {
        for (auto &phi_pair : phi_nodes_[succ])
        {
            AllocaInst *alloca = phi_pair.first;
            PhiInst *phi = phi_pair.second;

            Value *val = def_stack_[alloca].empty()
                             ? UndefValue::get(alloca->get_type()->get_pointer_element_type(), m_)
                             : def_stack_[alloca].back();
            phi->add_phi_pair_operand(val, bb);
        }
    }

    // // 4. 递归处理支配树中的子节点
    for (auto child : dominators_->get_dom_tree_succ_blocks(bb))
    {
        rename(child);
    }

    // 5. 恢复 def_stack
    for (auto it = new_defs.rbegin(); it != new_defs.rend(); ++it)
    {
        def_stack_[it->first].pop_back();
    }
}

// void Mem2Reg::rename(BasicBlock *bb) {
//     std::vector<std::pair<AllocaInst *, Value *>> new_defs;

//     // 1. 处理 phi 指令
//     for (auto &inst : bb->get_instructions()) {
//         if (auto phi = dynamic_cast<PhiInst *>(&inst)) {
//             for (auto &[alloca, phi_inst] : phi_nodes_[bb]) {
//                 if (phi_inst == phi) {
//                     def_stack_[alloca].push_back(phi);
//                     new_defs.emplace_back(alloca, phi);
//                     break;
//                 }
//             }
//         }
//     }

//     // 2. 替换 load 和记录 store
//     auto &instrs = bb->get_instructions();
//     for (auto it = instrs.begin(); it != instrs.end();) {
//         Instruction *inst = &(*it);

//         if (auto load = dynamic_cast<LoadInst *>(inst)) {
//             // auto ptr = load->get_lval();
//             // std::cerr << "Processing Load: " << load << " from " << ptr << "\n";
//             if (auto alloca = dynamic_cast<AllocaInst *>(load->get_lval())) {
//                 if ((alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
//                      alloca->get_type()->get_pointer_element_type()->is_float_type()||alloca->get_type()->get_pointer_element_type()->is_pointer_type()) &&
//                     !def_stack_[alloca].empty()) {
//                     Value *latest = def_stack_[alloca].back();
//                     load->replace_all_use_with(latest);
//                     it = instrs.erase(it);
//                     continue;
//                 }
//             }
//         } else if (auto store = dynamic_cast<StoreInst *>(inst)) {
//             // auto ptr = store->get_lval();
//             // std::cerr << "Processing Store: " << store << " to " << ptr << "\n";
//             if (auto alloca = dynamic_cast<AllocaInst *>(store->get_lval())) {
//                 if (alloca->get_type()->get_pointer_element_type()->is_integer_type() ||
//                     alloca->get_type()->get_pointer_element_type()->is_float_type()||alloca->get_type()->get_pointer_element_type()->is_pointer_type()) {
//                     Value *val = store->get_rval();
//                     def_stack_[alloca].push_back(val);
//                     new_defs.emplace_back(alloca, val);
//                     it = instrs.erase(it);
//                     continue;
//                 }
//             }
//         }
//         ++it;
//     }

//     // 3. 处理后继基本块的 phi 指令
//     Instruction *term = &bb->get_instructions().back();
//     std::vector<BasicBlock *> successors;
//     if (auto br = dynamic_cast<BranchInst *>(term)) {
//         if (br->is_cond_br()) {
//             successors.push_back(dynamic_cast<BasicBlock*>(br->get_operand(1)));
//             successors.push_back(dynamic_cast<BasicBlock*>(br->get_operand(2)));
//         } else {
//             successors.push_back(dynamic_cast<BasicBlock*>(br->get_operand(0)));
//         }
//     }

//     for (auto succ : successors) {
//         for (auto &phi_pair : phi_nodes_[succ]) {
//             AllocaInst *alloca = phi_pair.first;
//             PhiInst *phi = phi_pair.second;

//             Value *val = def_stack_[alloca].empty()
//                          ? UndefValue::get(alloca->get_type()->get_pointer_element_type(), m_)
//                          : def_stack_[alloca].back();
//             phi->add_phi_pair_operand(val, bb);
//         }
//     }

//     // 4. 递归处理支配树中的子节点
//     for (auto child : dominators_->get_dom_tree_succ_blocks(bb)) {
//         rename(child);
//     }

//     // 5. 恢复 def_stack
//     for (auto it = new_defs.rbegin(); it != new_defs.rend(); ++it) {
//         def_stack_[it->first].pop_back();
//     }
// }