#include "SimplifyCFG.hpp"

bool SimplifyCFG::check_useless_jump(BasicBlock* bb) {
    // 假定已经检验 bb->inst_list.size() == 1 and Typeid = unconditionalBranch
    bool isOk = false;
    Value* jumpPos = bb->inst_list.front()->operands[0];
    for (auto pre: bb->pre_bbs) {
        // 获取 pre 的跳转指令
        if(pre->inst_list.empty()) {
            continue; // 如果前驱基本块没有指令，则跳过
        }
        auto preBr = pre->get_terminator();
        // 判断是否为有条件跳转指令
        if (preBr->is_br() && preBr->operands.size() == 3) {
            Value* jumpPosa = preBr->operands[1];
            Value* jumpPosb = preBr->operands[2];
            if (jumpPosa == bb) {
                if (jumpPosb == jumpPos) {
                    return false;
                }
            } else if (jumpPosb == bb) {
                if (jumpPosa == jumpPos) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool SimplifyCFG::delete_relay_blocks(Function* func) {
    std::set<BasicBlock*> delBB;
    bool changed = false;
    for (auto bb : func->bb_list) {
        if (bb->name == "label_entry" || bb->name == "label_ret") { // 排除label_entry, label_ret
            continue;
        }
        if (delBB.find(bb) != delBB.end()) {
            continue;
        }
        if (bb->inst_list.size() == 1) { // 仅仅包含一条语句
            Instruction* onlyInstr = bb->inst_list.front();
            // 需要满足唯一的一条指令为 无条件 指令
            if (onlyInstr->is_br() && onlyInstr->operands.size() == 1 && check_useless_jump(bb)) {
                bool isOk = true;
                bool hasPhi = false;
                std::vector<int> phiArgNos;
                std::vector<PhiInst*> phis;
                for (auto use : bb->use_list) {
                    if (dynamic_cast<PhiInst*>(use.val)) {
                        hasPhi = true;
                        phis.push_back(dynamic_cast<PhiInst*>(use.val));
                        phiArgNos.push_back(use.index);
                        if(!dynamic_cast<ConstInt*>(phis.back()->operands[use.index - 1]) && !dynamic_cast<ConstInt*>(phis.back()->operands[use.index - 1])) {
                            isOk = false;
                            break;
                        }
                    }
                }
                
                if(isOk == false)
                    continue;

                if (hasPhi == false) {
                    // 删除前驱后继关系
                    BasicBlock* nextBB = static_cast<BasicBlock*>(onlyInstr->operands[0]);
                    nextBB->remove_pre_basic_block(bb);
                    bb->remove_succ_basic_block(nextBB);
                    // 将该基本块给剔除
                    bb->delete_inst(bb->inst_list.front());
                    bb->replace_use_with(nextBB);
                    // 建立新的前驱后继关系
                    for (auto pre : bb->pre_bbs) {
                        pre->add_succ_basic_block(nextBB);
                        nextBB->add_pre_basic_block(pre);
                    }
                    // [Terribly]:删除前驱后继关系
                    // TODO: 这里删除逻辑的错误，可能导致循环内删除迭代器，下面给出一个临时修改方案
                    unordered_set<BasicBlock*> bb_set;
                    for (auto pre : bb->pre_bbs) {
                        pre->remove_succ_basic_block(bb);
                        // bb->remove_pre_basic_block(pre);
                        bb_set.insert(pre);
                    }
                    for(auto bb: bb_set) {
                        bb->remove_pre_basic_block(bb);
                    }
                    delBB.insert(bb);
                } else {
                    for (int i = 0; i < phis.size(); i++) {
                        int phiArgNo = phiArgNos[i];
                        PhiInst* phi = phis[i];
                        // 首先第一步：获取Value
                        Value* ConstValue = phi->operands[phiArgNo - 1];
                        // assert(ConstValue->is_const() && (dynamic_cast<ConstInt*>(ConstValue) || dynamic_cast<ConstFloat*>(ConstValue)));
                        // 然后第二步：将这两个操作数删除
                        phi->remove_ops(phiArgNo - 1, phiArgNo);
                        // 最后第三步：添加这个关系
                        for (auto pre : bb->pre_bbs) {
                            if (delBB.find(pre) != delBB.end())
                                continue;
                            if (ConstValue->type->tid == TypeID::IntegerTy) {
                                auto newConst = new ConstInt(static_cast<IntegerType*>(ConstValue->type), dynamic_cast<ConstInt*>(ConstValue)->val);
                                phi->add_ops(newConst, pre);
                            } else if (ConstValue->type->tid == TypeID::FloatTy) {
                                auto newConst = new ConstFloat(ConstValue->type, dynamic_cast<ConstFloat*>(ConstValue)->val);
                                phi->add_ops(newConst, pre);
                            }
                        }
                    }

                    // 删除前驱后继关系
                    BasicBlock* nextBB = static_cast<BasicBlock*>(onlyInstr->operands[0]);
                    nextBB->remove_pre_basic_block(bb);
                    bb->remove_succ_basic_block(nextBB);
                    // 将该基本块给剔除
                    bb->delete_inst(bb->inst_list.front());
                    bb->replace_use_with(nextBB);
                    // 建立新的前驱后继关系
                    for (auto pre : bb->pre_bbs) {
                        pre->add_succ_basic_block(nextBB);
                        nextBB->add_pre_basic_block(pre);
                    }
                    // [Terribly]：删除前驱后继关系应该放在这里
                    // TODO: 这里删除逻辑的错误，可能导致循环内删除迭代器，下面给出一个临时修改方案
                    unordered_set<BasicBlock*> bb_set;
                    for (auto pre : bb->pre_bbs) {
                        pre->remove_succ_basic_block(bb);
                        // bb->remove_pre_basic_block(pre);
                        bb_set.insert(pre);
                    }
                    for(auto bb: bb_set) {
                        bb->remove_pre_basic_block(bb);
                    }
                    delBB.insert(bb);
                }
            }
        }
    }
    for (auto delbb : delBB) {
        func->remove_bb(delbb);
    }
    changed = delBB.size() != 0;
    return changed;
}

bool SimplifyCFG::merge_single_way_blocks(Function* func) {
    std::set<BasicBlock*> delBB;
    bool changed = false;
    for (auto bb: func->bb_list) {
        if (delBB.find(bb) != delBB.end()) {
            continue;
        }
        if (bb->succ_bbs.size() == 1) { // 需要满足后继基本块的数量为1
            auto succ_bb = bb->succ_bbs[0]; // 获取后继基本块
            if (succ_bb->pre_bbs.size() != 1 || delBB.find(succ_bb) != delBB.end()) { // 需要满足后续基本块的前驱基本块的数量为1
                continue;
            }
            // assert(bb == succ_bb->pre_bbs[0]);
            changed = true;
            // 删除前驱基本块的跳转指令
            auto preBr = bb->get_terminator();
            bb->delete_inst(preBr);
            // 移除前驱后继关系
            bb->remove_succ_basic_block(succ_bb);
            succ_bb->remove_pre_basic_block(bb);
            // 将后续块中的指令添加至前驱块中
            for (auto it = succ_bb->inst_list.begin(); it != succ_bb->inst_list.end();) {
                auto instr = *it;
                it++; // 提前递增，确保迭代器不失效
                succ_bb->remove_inst(instr);
                bb->add_inst_back(instr);
            }
            // 修改前驱后继关系
            for (auto succ: succ_bb->succ_bbs) {
                bb->add_succ_basic_block(succ);
                succ->remove_pre_basic_block(succ_bb);
                succ->add_pre_basic_block(bb);
            }
            // 将所有的后续块的使用转换为对前驱块的引用
            succ_bb->replace_use_with(bb);
            // 将该基本块删除，暂时加入delBB
            delBB.insert(succ_bb);
        }
    }
    for (auto delbb : delBB) {
        func->remove_bb(delbb);
    }
    return changed;
}

void SimplifyCFG::delete_rebundant_phi(Function* func) {
    std::vector<Instruction*> delInstr;
    for (auto bb : func->bb_list) {
        if (bb->pre_bbs.size() == 1) {
            for (auto instr : bb->inst_list) {
                if (instr->is_phi()) {
                    // 查找需要替换的值
                    Value* val = nullptr;
                    for (int i = 0; i < instr->operands.size(); i += 2) {
                        if (dynamic_cast<ValUndef*>(instr->operands[i]))
                            continue;
                        if (dynamic_cast<BasicBlock*>(instr->operands[i + 1]) != bb->pre_bbs[0])
                            continue;
                        // assert(dynamic_cast<BasicBlock*>(instr->operands[i + 1]) == bb->pre_bbs[0]);
                        val = instr->operands[i];
                        break;
                    }
                    if (val != nullptr) {
                        instr->replace_use_with(val);
                        delInstr.push_back(instr);
                    }
                }
            }
        }
    }
    for (auto instr : delInstr) {
        auto bb = instr->parent;
        bb->delete_inst(instr);
    }
}

void SimplifyCFG::execute() {
    for (auto func : m->func_list) {
        if (func->bb_list.empty())
            continue;
        delete_unreachable_blocks(func);
        delete_rebundant_phi(func);
        while (merge_single_way_blocks(func));
        while (delete_relay_blocks(func));
    }
}