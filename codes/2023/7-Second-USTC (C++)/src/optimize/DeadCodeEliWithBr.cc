#include <string.h>
#include<algorithm>
#include<list>

#include "DeadCodeEliWithBr.hh"
#include "RDominateTree.hh"
#include "DominateTree.hh"

#include "logging.hh"


void DeadCodeEliWithBr::execute(){
    RDominateTree rdom_tree(m_);
    rdom_tree.execute();        //& 获得反向支配树
    DominateTree dom_tree(m_);
    dom_tree.execute();
    //& 找出被引用的函数
    find_func();
    delete_unused_func();
    //& 以函数为单位进行优化
    for(auto &func : func_list) {
        if(func->get_basic_blocks().empty()){
            continue;
        }
        else{
            func_ = func;
            //& 初始化
            inst_mark.clear();
            bb_mark.clear();
            for(auto &bb : func->get_basic_blocks()) {
                bb_mark.insert({bb, false});
                for(auto inst : bb->get_instructions()){
                    inst_mark.insert({inst, false});
                }
            }
            //& 进行mark操作
            mark();
            //& 进行sweep操作
            sweep();
            delete_unreachable_bb();
            delete_unused_control();
        }
    }

}

void DeadCodeEliWithBr::find_func(){
    std::list<Function*> func_work_list;
    //& 首先找到main函数
    for(auto &func : m_->get_functions()) {
        if(func == m_->get_main_function()){
            main_func = func;
            func_mark.insert({func, true});
            func_list.push_back(func);
            func_work_list.push_back(func);
        }
        else{
            func_mark.insert({func, false});
        }
    }
    //& 从main函数开始依次找到被调用的函数
    while(!func_work_list.empty()) {
        auto func = func_work_list.front();
        func_work_list.pop_front();
        for(auto bb : func->get_basic_blocks()) {
            for(auto inst : bb->get_instructions()) {
                if(inst->is_call()){
                    auto call_func = (Function *)(inst->get_operand(0));
                    LOG(DEBUG) << func->get_name();
                    if(!func_mark[call_func]){
                        func_mark[call_func] = true;
                        func_list.push_back(call_func);
                        func_work_list.push_back(call_func);
                    }
                }
            }
        }
    }
    
}
void DeadCodeEliWithBr::delete_unused_func(){
    //& 错误
    auto function_list = m_->get_functions();
    for(auto func : function_list){
        if(func_mark[func] == false){
            m_->get_functions().remove(func);
        }
    }
}

void DeadCodeEliWithBr::mark(){
    std::list<Instruction*> worklist = {};
    //& 进行第一次遍历函数中的所有指令，将关键指令标记。
    for(auto bb : func_->get_basic_blocks()){
        for(auto inst : bb->get_instructions()){
            if(is_critical_inst(inst)){
                inst_mark.find(inst)->second = true;
                worklist.push_back(inst);
            }
        }
    }
    while(!worklist.empty()){
        auto inst = worklist.front();
        worklist.pop_front();
        //& 如果指令所在的基本块未被标记，则标记
        if(!bb_mark[inst->get_parent()]){
            bb_mark[inst->get_parent()] = true;
        }
        //& 对指令引用变量的定值进行标记
        for(auto operand : inst->get_operands()){
            //&判断操作数是否是变量，是变量对其的定值做标记
            if(dynamic_cast<Instruction*>(operand)){
                auto inst_to_mark = dynamic_cast<Instruction*>(operand);
                if(!inst_mark[inst_to_mark]){
                    inst_mark[inst_to_mark] = true;
                    worklist.push_back(inst_to_mark);
                }
            }
            //&判断从worklist中提出的变量指令是否是phi指令
            //&如果是，则对phi指令中的基本块的跳转指令进行标记
            if(inst->is_phi()){
                if(dynamic_cast<BasicBlock *>(operand)){    //&并且操作数是基本块
                    auto phi_bb = dynamic_cast<BasicBlock *>(operand);
                    worklist.push_back(phi_bb->get_instructions().back());
                }
            }

        }
        //& 对该指令所在基本块的反向支配边界中的块的跳转指令，进行标记
        for(auto bb : inst->get_parent()->get_rdom_frontier()){
            auto br = bb->get_instructions().back();
            if(br->is_extend_br()){
                if(!inst_mark[br]){
                    inst_mark[br] = true;
                    worklist.push_back(br);
                }
            }
        }
    }
    
}

BasicBlock * DeadCodeEliWithBr::get_irdom(BasicBlock * bb){
    BasicBlock * irdom = nullptr;
    for(auto rdombb : bb->get_rdoms()) {
        if(rdombb == bb){
            continue;
        }
        if(irdom == nullptr){
            irdom = rdombb;
        }
        if(rdombb->get_rdoms().find(irdom) != rdombb->get_rdoms().end()){
            irdom = rdombb;
        }
    }
    return irdom;
}


void DeadCodeEliWithBr::sweep(){
    //&删除无用的函数
    std::list<Instruction*> delete_list = {};
    //&获取没有标记的指令的list
    for(auto bb : func_->get_basic_blocks()){
        for(auto inst : bb->get_instructions()) {
            if(!inst_mark[inst]){
                delete_list.push_back(inst);
            }
        }
    }
    //&处理未标记的指令
    for(auto inst : delete_list){
        if(!(inst->is_void() || inst->is_call())){
            inst->get_parent()->delete_instr(inst);     //&删除指令
            /*
            //&更新使用者列表
            //& 错误
            // for(auto operand : inst->get_operands()){//
            //     if(dynamic_cast<ConstantInt *>(operand)){
            //         continue;    //& 常数不用管
            //     }
            //     if(dynamic_cast<Instruction*>(operand)){
            //         auto oper_inst = dynamic_cast<Instruction *>(operand);   //& 操作数对应的指令
            //         oper_inst->remove_use(dynamic_cast<Value *>(inst));
            //     }
            // }
            */
        }
        else if(inst->is_extend_br()){
            if(inst->is_extend_cond_br())
            {
                BasicBlock *nearest_marked_postdom = get_irdom(inst->get_parent());
                while(!bb_mark[nearest_marked_postdom]){
                    nearest_marked_postdom = get_irdom(nearest_marked_postdom);
                    //& 错误
                    if(nearest_marked_postdom == nullptr){
                        for(auto ret_bb : func_->get_basic_blocks()){
                            if(ret_bb->get_name() == "label_ret"){
                                nearest_marked_postdom = ret_bb;
                                break;
                            }
                        }
                    }
                    break;
                }
                //& 将br指令所在的基本块的后驱节点删除，增加nearest_marked_postdom为唯一的后驱节点
                //& 将br指令所在的基本块从其后驱节点的前驱节点列表中删除
                auto succ_bb_list = inst->get_parent()->get_succ_basic_blocks();
                for(auto succ_bb : succ_bb_list){
                    succ_bb->remove_pre_basic_block(inst->get_parent());
                    inst->get_parent()->remove_succ_basic_block(succ_bb);
                }
                //& 错误
                BranchInst::create_br(nearest_marked_postdom, inst->get_parent());
                inst->get_parent()->delete_instr(inst);
                /*
                // inst->get_parent()->add_succ_basic_block(nearest_marked_postdom);
                //& 设置inst->get_parent为nearest_marked_postdom的前驱
                // nearest_marked_postdom->add_pre_basic_block(inst->get_parent());
                */
            }
        }
    }
}
bool DeadCodeEliWithBr::is_critical_inst(Instruction *inst) {
    return (inst->is_ret() || inst->is_store() || inst->is_call() || inst->is_storeoffset() || inst->is_memset());
}


void DeadCodeEliWithBr::delete_unreachable_bb(){
    std::map<BasicBlock *, bool> reachable_bb;
    std::list<BasicBlock *> worklist;
    //& 初始化
    for(auto bb : func_->get_basic_blocks()) {
        reachable_bb.insert({bb, false});
    }
    //& 开始遍历可以到达的基本块
    worklist.push_back(func_->get_entry_block());
    reachable_bb[func_->get_entry_block()] = true;
    while(!worklist.empty()){
        auto current_bb = worklist.front();
        worklist.pop_front();
        for(auto succ_bb : current_bb->get_succ_basic_blocks()) {
            if(!reachable_bb[succ_bb]){     //& 未被扫描过
                reachable_bb[succ_bb] = true;
                worklist.push_back(succ_bb);
            }
        }
    }
    //& 错误
    auto bb_list = func_->get_basic_blocks();
    for(auto bb : bb_list){
        if(!reachable_bb[bb]){      //& 无法到达的块
            for(auto succ_bb : bb->get_succ_basic_blocks()){
                succ_bb->remove_pre_basic_block(bb);
            }
            func_->get_basic_blocks().remove(bb);
        }
    }
}

void DeadCodeEliWithBr::delete_unused_control(){
    DominateTree dom_tree(m_);
    bool changed = true;
    while(changed){
        changed = false;
        dom_tree.get_reverse_post_order(func_);//& compute postorder
        auto postorder = dom_tree.reverse_post_order;
        postorder.reverse();
        for(auto bb_i : postorder){
            //& case1 合并冗余分支指令
            auto br = bb_i->get_terminator();
            //& 错误
            if(bb_i->get_name() == "label_ret"){
                continue;
            }
            if(br->is_extend_cond_br()){//& 如果是条件分支
                auto succ_bb_1 = br->get_operand(1);
                auto succ_bb_2 = br->get_operand(2);
                if(br->is_cmpbr() || br->is_fcmpbr())
                {
                    succ_bb_1 = br->get_operand(2);
                    succ_bb_2 = br->get_operand(3);
                }
                if(succ_bb_1 == succ_bb_2){
                    //& 如果分支指令的两个基本块相同，则用jump来代替br
                    //& 首先将br指令，从第一个参数的引用连中删除
                    //& 错误
                    changed = true;
                    if(dynamic_cast<Instruction*>(br->get_operand(0))){
                        auto oper_inst = dynamic_cast<Instruction *>(br->get_operand(0));
                        //& 操作数对应的指令
                        oper_inst->remove_use(dynamic_cast<Value *>(br));
                    }
                    //& 删除branch创建jump
                    //& create会添加前驱和后继
                    //& 错误
                    (dynamic_cast<BasicBlock*>(succ_bb_1))->remove_pre_basic_block(br->get_parent());
                    br->get_parent()->remove_succ_basic_block(dynamic_cast<BasicBlock*>(succ_bb_1));
                    BranchInst::create_br(dynamic_cast<BasicBlock*>(succ_bb_1), br->get_parent());
                    br->get_parent()->delete_instr(br);
                    /*
                    //& 更新使用者列表
                    //& 错误
                    // if(dynamic_cast<Instruction*>(br->get_operand(0))){
                    //     auto curinst = dynamic_cast<Instruction *>(br->get_operand(0));//操作数对应的指令
                    //     curinst->remove_use(dynamic_cast<Value *>(br));
                    // }
                    */
                }
            }
            //& case 2 3 4
            br = bb_i->get_terminator();
            //& 错误 跳转
            if(br->is_br() && !br->is_extend_cond_br()){
                auto succ_bb = br->get_parent()->get_succ_basic_blocks().front();
                //& 获得唯一的后继节点
                //& case 2 删除空的程序块
                if(br->get_parent()->get_num_of_instrs() == 1){
                    //& 只有一个跳转指令
                    //& 错误
                    bool phi_flag = false;
                    for(auto inst : succ_bb->get_instructions()){
                        if(inst->is_phi()){
                            phi_flag = true;
                        }
                    }
                    if(phi_flag){
                        continue;
                    }
                    if (strcmp(br->get_parent()->get_name().c_str(), "label_entry")){
                        //&不是entry块
                        changed = true;
                        auto pre_bb_list = br->get_parent()->get_pre_basic_blocks();
                        for(auto pre_bb : pre_bb_list){
                            //&修改前驱节点的
                            auto pre_bb_br = pre_bb->get_terminator();
                            //&错误 pre_bb_br
                            if(pre_bb_br->is_extend_cond_br()){
                                //&如果是条件跳转指令
                                auto succ_bb_1 = 1;
                                auto succ_bb_2 = 2;
                                if(pre_bb_br->is_cmpbr() || pre_bb_br->is_fcmpbr())
                                {
                                    succ_bb_1 = 2;
                                    succ_bb_2 = 3;
                                }

                                if(pre_bb_br->get_operand(succ_bb_1) == br->get_parent()){
                                    pre_bb_br->set_operand(succ_bb_1, succ_bb);
                                }
                                else{
                                    pre_bb_br->set_operand(succ_bb_2, succ_bb);
                                }
                            }
                            else{
                                //&如果是非条件跳转指令
                                pre_bb_br->set_operand(0, succ_bb);
                            }
                            //& 错误
                            pre_bb->remove_succ_basic_block(br->get_parent());
                            std::list<BasicBlock *>::iterator it = std::find(pre_bb->get_succ_basic_blocks().begin(), pre_bb->get_succ_basic_blocks().end(),succ_bb);
                            if(it == pre_bb->get_succ_basic_blocks().end()){
                                pre_bb->add_succ_basic_block(succ_bb);
                                succ_bb->add_pre_basic_block(pre_bb);
                            }
                        }
                        //& 对删除的后继节点进行处理
                        //& 首先将br->parent将从succ_bb中移除
                        succ_bb->remove_pre_basic_block(br->get_parent());
                        /*
                        //接下来处理succ_bb块中的phi指令
                        // for(auto inst : succ_bb->get_instructions()){
                        //     if(inst->is_phi()){
                        //         for(size_t i = 1; i < inst->get_num_operand(); i = i + 2){
                        //             if(inst->get_operand(i) == br->get_parent()){
                        //                 //删除包含br->get_parent()的部分
                        //                 Value *temp = inst->get_operand(i-1);
                        //                 inst->remove_operands(i-1, i);
                        //                 //更新phi的参数
                        //                 for(auto pre_bb : pre_bb_list){
                        //                     inst->add_operand(temp);
                        //                     inst->add_operand(pre_bb);
                        //                 }
                        //             }
                        //         }
                        //     }
                        // }
                        */
                        //& 将br->get_parent()基本块删除
                        func_->get_basic_blocks().remove(br->get_parent());
                        continue;
                    }
                }
                //& case3 合并程序块
                if(succ_bb->get_pre_basic_blocks().size() == 1){ //& 后继节点只有一个前驱节点
                    //& 将bb_i的指令移动到succ_bb中
                    //&  changed = true;
                    if(bb_i->get_name() != "label_entry")
                    {
                        auto inst_list = bb_i->get_instructions();
                        inst_list.reverse();
                        for(auto inst : inst_list){
                            succ_bb->add_instr_begin(inst);
                        }
                        succ_bb->delete_instr(br);
                        auto pre_bb_list = bb_i->get_pre_basic_blocks();
                        for(auto pre_bb : pre_bb_list){//修改前驱节点的
                            auto pre_bb_br = pre_bb->get_terminator();
                            if(pre_bb_br->is_extend_cond_br()){
                                //& 如果是条件跳转指令
                                auto succ_bb_1 = 1;
                                auto succ_bb_2 = 2;
                                if(pre_bb_br->is_cmpbr() || pre_bb_br->is_fcmpbr())
                                {
                                    succ_bb_1 = 2;
                                    succ_bb_2 = 3;
                                }

                                if(pre_bb_br->get_operand(succ_bb_1) == br->get_parent()){
                                    pre_bb_br->set_operand(succ_bb_1, succ_bb);
                                }
                                else{
                                    pre_bb_br->set_operand(succ_bb_2, succ_bb);
                                }
                            }
                            else{
                                //& 如果是非条件跳转指令
                                pre_bb_br->set_operand(0, succ_bb);
                            }
                            pre_bb->remove_succ_basic_block(br->get_parent());
                            pre_bb->add_succ_basic_block(succ_bb);
                            succ_bb->add_pre_basic_block(pre_bb);
                        }
                        //& 对删除的后继节点进行处理
                        //& 首先将br->parent将从succ_bb中移除
                        succ_bb->remove_pre_basic_block(br->get_parent());
                        //& 将br->get_parent()基本块删除
                        func_->get_basic_blocks().remove(br->get_parent());

                        //& NEW: 设置移动后的指令的Parent
                        for(auto inst : inst_list){
                            inst->set_parent(succ_bb);
                        }
                        continue;
                    }
                    else
                    {
                        auto inst_list = succ_bb->get_instructions();
                        for(auto inst : inst_list){
                            bb_i->add_instruction(inst);
                        }
                        bb_i->delete_instr(br);

                        //& 维护前后继关系
                        auto succ_bb_list = succ_bb->get_succ_basic_blocks();
                        for(auto succ_bb_ : succ_bb_list)
                        {
                            succ_bb_->remove_pre_basic_block(succ_bb);
                            succ_bb_->add_pre_basic_block(bb_i);
                            bb_i->add_succ_basic_block(succ_bb_);
                        }
                        succ_bb->replace_all_use_with(bb_i);
                        bb_i->remove_succ_basic_block(succ_bb);
                        func_->get_basic_blocks().remove(succ_bb);

                        //NEW: 设置移动后的指令的Parent
                        for(auto inst : inst_list){
                            inst->set_parent(bb_i);
                        }
                        continue;
                    }
                }
                //& case4 提升分支指令
                if(succ_bb->get_num_of_instrs() == 1)
                {
                    if(succ_bb->get_name() == "label_ret"){
                        continue;
                    }
                    //& 错误
                    bool phi_flag = false;
                    for(auto succ_succ_bb : succ_bb->get_succ_basic_blocks()){
                        for(auto inst : succ_succ_bb->get_instructions()){
                            if(inst->is_phi()){
                                phi_flag = true;
                                break;
                            }
                        }
                        if(phi_flag){
                            break;
                        }
                    }
                    if(phi_flag){
                        continue;
                    } 
                    auto succ_br = succ_bb->get_terminator();
                    if(succ_br->is_cmpbr() || succ_br->is_fcmpbr())
                    {
                        continue;
                    }
                    changed = true;
                    if(dynamic_cast<BranchInst *>(succ_br)->is_cond_br()){
                        //& 将br指令(即jump指令删除)替换成succ_br的副本
                        Value *cond = succ_br->get_operand(0);
                        BasicBlock *if_true = dynamic_cast<BasicBlock *>(succ_br->get_operand(1));
                        BasicBlock *if_false = dynamic_cast<BasicBlock *>(succ_br->get_operand(2));
                        BranchInst::create_cond_br(cond, if_true, if_false, br->get_parent());
                        //& 调整前驱后继的关系
                        br->get_parent()->remove_succ_basic_block(succ_bb);
                        succ_bb->remove_pre_basic_block(br->get_parent());
                        br->get_parent()->delete_instr(br);
                    }
                }
            }

        }
    }
}
