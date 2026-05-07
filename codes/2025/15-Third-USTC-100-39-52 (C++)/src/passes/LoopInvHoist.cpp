#include "LoopInvHoist.hpp"

#include "LoopSearch.hpp"
#include "logging.hpp"

#include <algorithm>
#include <vector>

void LoopInvHoist::run() {
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    LOG(INFO) << "====== Loop invariant motion started ======";

    LoopTree loop_tree;
    // TODO: construct loop tree and do loop invariant hoisting
    for (auto &func1 : m_->get_functions()) {
        auto func = &func1;
        auto loops = loop_searcher.get_loops_in_func(func);

        LOG(INFO) << "LoopInvHoist: Get func " << func->get_name() << ", addr = " << func;
        for (auto loop : loops) {
            auto parent = loop_searcher.get_parent_loop(loop);  // 返回包含此 loop 的上层 loop
            if (parent) {
                loop_tree[parent].insert(loop);  // 建立“父 -> 子”映射
            }
        }
    }
    
    BBset_t vis; 
    for (auto &func : m_->get_functions()) {
        auto loops = loop_searcher.get_loops_in_func(&func);

        for (auto loop : loops) {
            if (loop_searcher.get_parent_loop(loop) == nullptr) {
                hoist(loop, loop_tree, loop_searcher, vis);
            }
        }
    }
    LOG(INFO) << "====== Loop invariant motion ended ======";
}

// bool is_self_increment(Instruction *instr, BBset_t *loop) {
//     if (!(instr->is_add() || instr->is_fadd()) || instr->get_num_operand()!=2)
//         return false;

//     // 只有当一个 operand “正好等于” instr 本身时，才算 i = i + c
//     for (int i = 0; i < 2; i++) {
//         if (instr->get_operand(i) == dynamic_cast<Value *>(instr))
//             return true;
//     }
//     return false;
// }


bool LoopInvHoist::is_self_increment(Instruction *instr, BBset_t *loop) {
    return false;
    // // 只处理整数/浮点加法
    // if (!(instr->is_add() || instr->is_fadd()))
    //     return false;

    // // 操作数必须正好两个
    // if (instr->get_num_operand() != 2)
    //     return false;

    // auto *op1 = instr->get_operand(0);
    // auto *op2 = instr->get_operand(1);

    // // 任意一个是常量或 loop-invariant，另一个是之前的版本
    // // 判定是否是 x = x + c（x 是 loop 内某 SSA 值）

    // Instruction *def_instr1 = dynamic_cast<Instruction *>(op1);
    // Instruction *def_instr2 = dynamic_cast<Instruction *>(op2);

    // bool op1_in_loop = def_instr1 && loop->count(def_instr1->get_parent());
    // bool op2_in_loop = def_instr2 && loop->count(def_instr2->get_parent());

    // // 至少有一个是来自 loop 的定义（递归依赖）
    // if (!(op1_in_loop || op2_in_loop))
    //     return false;

    // // 另一个必须是常量或 loop-invariant
    // if (!(dynamic_cast<ConstantInt *>(op1) || is_inv(op1, loop) ||
    //       dynamic_cast<ConstantInt *>(op2) || is_inv(op2, loop)))
    //     return false;

    // return true;
}

// Optimize from leaf nodes on the loop tree up to the root nodes.
void LoopInvHoist::hoist(BBset_t *loop,
                         LoopTree &loop_tree,
                         LoopSearch &loop_searcher,
                         BBset_t &vis) {
    for (auto subloop : loop_tree[loop]) {
        hoist(subloop, loop_tree, loop_searcher, vis);
    }

    if (!loop) {
        return;
    }

    auto base = loop_searcher.get_loop_base(loop);
    std::vector<Instruction *> loop_invs;
    // TODO: find loop invariants, insert them into loop_invs

    // 简单的修改：将循环内的基本块按地址顺序排序，通常对应支配关系
    std::vector<BasicBlock *> ordered_blocks;
    for (auto bb : *loop) {
        ordered_blocks.push_back(bb);
    }
    
    // 按地址排序，这通常对应基本块在函数中的定义顺序
    std::sort(ordered_blocks.begin(), ordered_blocks.end());
    
    for (auto bb : ordered_blocks) {
        for (auto &instr : bb->get_instructions()) {
            if (can_move(&instr,loop)) {
                bool all_ope = true;
                for (int i = 0; i < instr.get_num_operand(); ++i) {
                    bool op_inv = is_inv(instr.get_operand(i), loop);
                    LOG(INFO) << "Operand " << i << " is_inv: " << op_inv;
                    if (!op_inv) {
                        all_ope = false;
                        break;
                    }
                }
                LOG(INFO) << "Instr " << instr.print() << " all_ope = " << all_ope;

                // ✅ 新增：如果 instr 被作为 br 的 condition，不能 hoist
                bool used_by_br = false;
                for (auto &user : instr.get_use_list()) {
                    auto *use_inst = dynamic_cast<Instruction*>(user.val_);
                    if (use_inst && use_inst->is_br()) {
                        used_by_br = true;
                        break;
                    }
                }


                bool self_inc = is_self_increment(&instr, loop);

                if (all_ope && !used_by_br && !self_inc) {
                    loop_invs.push_back(&instr);
                }
            }

        }
    }
    
    if (!loop_invs.empty()) {
        // Insert to the block just before the base block.
        BasicBlock *dest = nullptr;
        for (auto prec : base->get_pre_basic_blocks()) {
            if (!loop->count(prec)) {
                dest = prec;
                break;
            }
        }
        if (dest) {
            Instruction *terminator = dest->get_terminator();
            auto &inst_list = dest->get_instructions();

            // ✅ 手动查找 terminator 的 iterator
            auto termIt = inst_list.begin();
            for (; termIt != inst_list.end(); ++termIt) {
                if (&*termIt == terminator)
                    break;
            }
            assert(termIt != inst_list.end() && "Terminator not found in instruction list");

            for (auto instr : loop_invs) {
                auto parent_bb = instr->get_parent();
                parent_bb->get_instructions().remove(instr);

                instr->set_parent(dest);
                LOG(DEBUG) << "Hoisting instr: " << instr->print();

                inst_list.insert_before(termIt, instr);
            }
        }

        else {
            LOG(ERROR) << "This loop doesn't have an entry block?!";
        }
    }

    // Mark this loop body as analyzed
    for (auto bb : *loop) {
        vis.insert(bb);
    }
}

// A instruction can be moved <= no side effects (memory stores included)
// PHIs are excluded because we don't want to modify them.
bool LoopInvHoist::can_move(Instruction *instr, BBset_t *loop) {
    if (instr->is_store() || instr->is_call() || instr->is_phi() || instr->is_alloca()|| instr->is_gep())
        return false;
    // if (instr->is_load()) {
    //     // load 语句需要进一步分析其地址是否不变，是否被修改
    //     return memory_address_is_loop_invariant(instr) &&
    //            memory_not_modified_in_loop(instr);
    // }
    if (auto *ld = dynamic_cast<LoadInst *>(instr)) {
        Value *addr = ld->get_operand(0);
        // 2.1 地址必须循环不变
        if (!is_inv(addr, loop)) 
            return false;
        // 2.2 循环内不能有 store 写过同一地址
        if (is_modified_in_loop(addr, loop))
            return false;
        // // 2.3 如果 load 本身在循环体内，也不要动
        // if (loop->count(ld->get_parent()))
        //     return false;
    }
    return instr->isBinary() || instr->is_si2fp() || instr->is_fp2si() || instr->is_zext() ||
           instr->is_cmp() || instr->is_fcmp()||instr->is_load() ;
}

// Returns false if instr involves any value that is assigned inside loop.
bool LoopInvHoist::is_inv(Value *v, BBset_t *loop, std::set<Value*> &visited) {
    if (v == nullptr) return false;
    if (visited.count(v)) return true;
    visited.insert(v);

    // 常量一定是 invariant
    if (dynamic_cast<ConstantInt *>(v) || dynamic_cast<ConstantFP *>(v)) {
        return true;
    }

    // PhiInst 特殊处理
    if (auto *phi = dynamic_cast<PhiInst *>(v)) {
        LOG(INFO) << "hi: ";
        LOG(INFO) << "the phi instr is"  <<phi->print();
        for (int i = 0; i < phi->get_num_operand(); i += 2) {
            auto *val = phi->get_operand(i);
            auto *from_bb = dynamic_cast<BasicBlock *>(phi->get_operand(i + 1));
            if (!from_bb || loop->count(from_bb) || !is_inv(val, loop, visited)) {
                return false;
            }
        }
        return true;
    }

    if (auto *gep = dynamic_cast<GetElementPtrInst *>(v)) {
        for (int i = 1; i < gep->get_num_operand(); ++i)
            if (!is_inv(gep->get_operand(i), loop, visited))
                return false;
        return true;
    }

    if (auto *ld = dynamic_cast<LoadInst *>(v)) {
        Value *addr = ld->get_operand(0);
        // 首先判断地址是否 invariant（e.g., GEP 索引都不变）
        // LOG(INFO) << "try hoist " << ", addr = " << addr->get_name();
        if (is_inv(addr, loop, visited) ){
            if(!is_modified_in_loop(addr, loop)) {
                // LOG(INFO) << "yes " << ", addr = " << addr->get_name();
                return true;
            } else {
                // LOG(INFO) << "no " << ", addr = " << addr->get_name();
                return false;
            }
            if (dynamic_cast<GlobalVariable *>(ld->get_operand(0)))
                return false;
        } 
        return false;     
    }

    if (auto *instr = dynamic_cast<Instruction *>(v)) {
        if (instr->is_si2fp())
        {
            LOG(INFO) << "the fk instr is";
        }
        
        
        if ( instr->is_call())
            return false;
        auto *bb = instr->get_parent();
        if (!loop->count(bb)) return true;

        for (int i = 0; i < instr->get_num_operand(); ++i) {
            if (!is_inv(instr->get_operand(i), loop, visited)) return false;
        }
        return true;
    }

    
    return true; // 默认安全
}


bool LoopInvHoist::is_inv(Value *v, BBset_t *loop) {
    std::set<Value*> visited;
    return is_inv(v, loop, visited);
}

bool LoopInvHoist::is_modified_in_loop(Value *addr, BBset_t *loop) {
    auto gv = dynamic_cast<GlobalVariable *>(addr);
    if (!gv) return true; // 如果不是全局变量，保守起见认为可能修改

    // LOG(INFO) << "judgeing " << ", gv = " << addr->get_name()<<", in bigloop : " << loop->size();
    std::string target_gv_name = gv->get_name();
    for (auto bb : *loop) {
        for (auto &instr : bb->get_instructions()) {
            if (instr.is_store()) {
                Value *store_addr = instr.get_operand(1);
                std::string store_addr_name = store_addr->get_name();
                
                // 1. 直接存储到全局变量（按名称比较）
                if (store_addr_name == target_gv_name) {
                    // LOG(INFO) << "Direct store to global: " << target_gv_name;
                    return true;
                }

                // GEP 情况
                auto *gep = dynamic_cast<GetElementPtrInst *>(store_addr);
                if (gep && gep->get_operand(0) == gv)
                    return true;
            }

            // LOG(INFO) << "before judge call: ";
            if (instr.is_call()) {
                // LOG(INFO) << "in ";
                auto *call = dynamic_cast<CallInst *>(&instr);
                if (!call) continue;

                Function *callee = dynamic_cast<Function *>(call->get_operand(0));
                // LOG(INFO) << "in func: " << bb->get_parent()->get_name();
                // LOG(INFO) << "call 的内容 " << call->get_function();
                if (!callee) continue;

                if (function_writes_to_global(callee, gv)){
                    // LOG(INFO) << "yes_writes " << ", gv = " << gv->get_name()<<", in func : " << callee->get_name();
                
                    return true;
            
                }else{
                    // LOG(INFO) << "no_write " << ", store_addr = " << gv->get_name()<<", gv = " << callee->get_name();
                }
                
            }
        }
    }

    return false;
}

bool LoopInvHoist::function_writes_to_global(Function *f, GlobalVariable *gv,
                                             std::set<Function *> &visited) {
    if (visited.count(f)) return false; // 避免递归循环
    visited.insert(f);

    std::string target_gv_name = gv->get_name();

    for (auto &bb : f->get_basic_blocks()) {
        for (auto &instr : bb.get_instructions()) {
            if (instr.is_store()) {
                Value *store_addr = instr.get_operand(1);
                std::string store_addr_name = store_addr->get_name();

                // 1. 直接 store 到全局变量
                if (store_addr_name == target_gv_name) {
                    LOG(INFO) << "Direct store to global: " << target_gv_name;
                    return true;
                }

                // 2. store %gep
                if (auto *gep = dynamic_cast<GetElementPtrInst *>(store_addr)) {
                    Value *base_addr = gep->get_operand(0);
                    if (base_addr->get_name() == target_gv_name) {
                        LOG(INFO) << "Store via GEP to global: " << target_gv_name;
                        return true;
                    }
                }

                // 3. store %load
                if (auto *load = dynamic_cast<LoadInst *>(store_addr)) {
                    Value *src = load->get_operand(0);
                    if (src->get_name() == target_gv_name) {
                        LOG(INFO) << "Store via pointer to global: " << target_gv_name;
                        return true;
                    }
                }
            }

            // ✅ 4. 递归检查 call 指令的目标函数
            if (instr.is_call()) {
                Function *callee = dynamic_cast<Function *>(instr.get_operand(0));
                if (callee && function_writes_to_global(callee, gv, visited)) {
                    LOG(INFO) << "Indirect global store via call to: " << callee->get_name();
                    return true;
                }
            }
        }
    }

    return false;
}

bool LoopInvHoist::function_writes_to_global(Function *f, GlobalVariable *gv) {
    std::set<Function *> visited;
    return function_writes_to_global(f,gv, visited);
}

