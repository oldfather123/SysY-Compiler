#include "Check.hh"
#include "Module.hh"
#include "logging.hh"
//// #include "ArriveDef.hh"

void Check::execute() {
    SuccCheck();
    PreCheck();
    InstParentCheck();
    //// UsedefCheck();
    DefuseCheck();
    if(FindError == false){
        LOG(INFO) <<"Check通过!";
    }
}

void Check::SuccCheck(){
    LOG(DEBUG)<<"进入后继关系检查";
    for(auto fun:m_->get_functions()){
        for(auto block:fun->get_basic_blocks()){
            Instruction *inst = block->get_terminator();
            if(!inst){
                LOG(ERROR) <<"基本块"<<block->get_name()<<"以非终止指令结尾";
                FindError = true;
                continue;
            }
            if(inst->is_br()){
                int ops = inst->get_num_operands();
                auto succList = block->get_succ_basic_blocks();
                if(ops == 3){//cond_br
                    if(succList.size() != 2){
                        //实际后继数量与succ_bbs_中不符合，括号内第一项为实际后继数，第二项为succ_bbs_中后继数
                        LOG(ERROR) <<"基本块"<<block->get_name()<<"后继的数量错误("<<succList.size()<<"/2)";
                        FindError = true;
                        continue;
                    }
                    BasicBlock *Truebb = dynamic_cast<BasicBlock *>(dynamic_cast<BranchInst *>(inst)->get_operand(1));
                    BasicBlock *Falsebb = dynamic_cast<BasicBlock *>(dynamic_cast<BranchInst *>(inst)->get_operand(2));
                    if(find(succList.begin(),succList.end(),Truebb) == succList.end()){
                        LOG(ERROR) <<"未找到基本块"<<block->get_name()<<"的后继:"<<Truebb->get_name();
                        FindError = true;
                        continue;
                    }
                    if(find(succList.begin(),succList.end(),Falsebb) == succList.end()){
                        LOG(ERROR) <<"未找到基本块"<<block->get_name()<<"的后继:"<<Falsebb->get_name();
                        FindError = true;
                        continue;
                    }
                }
                else if(ops == 1){//br
                    if(succList.size() != 1){
                        LOG(ERROR) <<"基本块"<<block->get_name()<<"后继的数量错误("<<succList.size()<<"/1)";
                        FindError = true;
                        continue;
                    }
                    BasicBlock *bb = dynamic_cast<BasicBlock *>(dynamic_cast<BranchInst *>(inst)->get_operand(0));
                    if(find(succList.begin(),succList.end(),bb) == succList.end()){
                        LOG(ERROR) <<"未找到基本块"<<block->get_name()<<"的后继:"<<bb->get_name();
                        FindError = true;
                        continue;
                    }
                }
            }
        }
    }
}

void Check::PreCheck(){
    LOG(DEBUG)<<"进入前驱关系检查";

    if(FindError == true){
        LOG(WARNING)<<"后继关系错误，跳过前驱关系检查";
        return; 
    }

    for(auto fun:m_->get_functions()){
        for(auto block:fun->get_basic_blocks()){//验证每个块的后继块是否也保存了该前驱后继关系
            auto succList = block->get_succ_basic_blocks();//获取后继表
            for(auto bb:succList){
                auto preList = bb->get_pre_basic_blocks();//获取前驱表
                if(find(preList.begin(),preList.end(),block) == preList.end()){
                    
                    LOG(ERROR) <<bb->get_name()<<"中缺失前驱关系:"<<block->get_name()<<"->"<<bb->get_name();
                    FindError = true;
                }
            }
        }
    }

    for(auto fun:m_->get_functions()){
        for(auto block:fun->get_basic_blocks()){
            std::set<BasicBlock *> visited;//记录哪些基本块已经与本基本块有了前驱关系，防止重复
            auto preList = block->get_pre_basic_blocks();//获取前驱表
            for(auto bb:preList){//依次查找每个前驱关系是否正确
                if(visited.find(bb) != visited.end()){
                    LOG(ERROR) <<"该前驱关系重复:"<<bb->get_name()<<"->"<<block->get_name();
                    FindError = true;
                    continue;
                }
                auto succList = bb->get_succ_basic_blocks();//获取block前驱的后继表
                visited.insert(bb);
                if(find(succList.begin(),succList.end(),block) == succList.end()){//未在其前驱中找到对应的后继关系
                    LOG(ERROR) <<"前驱关系不正确:"<<bb->get_name()<<"->"<<block->get_name();
                    FindError = true;
                }
            }
        }
    }
}

/* 暂时无用
void Check::UsedefCheck(){
    LOG(DEBUG) << "进入引用前是否有定值的检查";

    if(FindError){
        LOG(WARNING) << "前驱后继关系错误，跳过引用前是否有定值的检查";
        return;
    }
    Arrivedef arrdef(m_);
    for(auto func : m_->get_functions()){
        arrdef.bb_gen_set.clear();
        arrdef.bb_in_set.clear();
        arrdef.bb_out_set.clear();
        arrdef.get_gen_kill(func);
        arrdef.get_in_out(func);
        LOG(DEBUG) << "进入" << func->get_name() << "的引用前是否有定值检查";
        for(auto bb : func->get_basic_blocks()) {
            auto def_set = arrdef.bb_in_set[bb];
            for(auto inst : bb->get_instructions()){
                for(auto inst_2 : bb->get_instructions()) {
                    if(inst_2 == inst){
                        break;
                    }
                    if(!inst_2->is_void()){//只将那些有左操作数的加入到def_set中
                        def_set.insert(inst_2);
                    }
                }
                //判断引用之前是否定值
                for(auto op : inst->get_operands()) {
                    bool flag = false;
                    if(dynamic_cast<Instruction *>(op) == inst){
                        continue;
                    }
                    for(auto arg : func->get_args()) {
                        if(dynamic_cast<Value *>(arg) == op){
                            flag = true;
                            break;
                        }
                    }
                    if(flag) continue;
                    if(dynamic_cast<ConstantFloat*>(op) || dynamic_cast<ConstantInt*>(op) || dynamic_cast<Function*>(op) || dynamic_cast<BasicBlock*>(op)|| dynamic_cast<GlobalVariable*>(op)){
                        continue;
                    }
                    if(def_set.find(dynamic_cast<Instruction *>(op)) == def_set.end()){
                        //未找到定值
                        LOG(DEBUG) << func->get_name() << ":" << bb->get_name() << ":" << op->get_name() << "无定值";
                        FindError = true;
                    }
                }
            }
        }

    }

}
*/

void Check::DefuseCheck(){
    LOG(DEBUG) << "进入du链的检查";

    if(FindError){
        LOG(WARNING) << "前驱后继关系错误或者引用前无定值，跳过du链的检查";
        return;
    }
    
    for(auto func : m_->get_functions()){
        LOG(DEBUG) << "进入" << func->get_name() << "的du链的检查";
        std::map<Instruction *, std::set<Instruction *>>  Instr_use;
        //初始化
        for(auto bb : func->get_basic_blocks()){
            for(auto inst : bb->get_instructions()){
                if(!inst->is_void()){
                    Instr_use.insert({inst, {}});
                }
            }
        }
        //创建du链
        for(auto bb : func->get_basic_blocks()){
            for(auto inst : bb->get_instructions()){
                for(auto op : inst->get_operands()) {
                    bool flag = false;
                    for(auto arg : func->get_args()) {
                        if(dynamic_cast<Value *>(arg) == op){
                            flag = true;
                            break;
                        }
                    }
                    if(flag) continue;
                    if(dynamic_cast<ConstantFP*>(op) || dynamic_cast<ConstantInt*>(op) || dynamic_cast<Function*>(op) || dynamic_cast<BasicBlock*>(op)|| dynamic_cast<GlobalVariable*>(op)){
                        continue;
                    }
                    Instr_use[dynamic_cast<Instruction *>(op)].insert(inst);
                }
            }
        }
        //检查du链
        for(auto bb : func->get_basic_blocks()){
            for(auto inst : bb->get_instructions()){
                if(!inst->is_void()){
                    //开始检查
                    if(Instr_use[inst].size() > inst->get_use_list().size()){
                        LOG(ERROR) << "指令" << inst->get_name() << "的引用数量有误" << Instr_use[inst].size() << "/" << inst->get_use_list().size(); 
                        FindError = true;
                    }
                    for(auto struct_use : inst->get_use_list()){
                        if(Instr_use[inst].find(dynamic_cast<Instruction *>(struct_use.val_)) == Instr_use[inst].end()){//未找到相应的引用
                            FindError = true;
                            LOG(ERROR) << "指令" << inst->get_name() << "的引用" << struct_use.val_->get_name() << "未找到";
                        }
                    }
                }
            }
        }

    }
}


void Check::InstParentCheck()
{
    LOG(DEBUG) << "进入Inst-Parent的检查";
    for(auto func : m_->get_functions())
    {
        for(auto bb : func->get_basic_blocks())
        {
            std::string bbName = bb->get_name();
            for(auto inst : bb->get_instructions())
            {
                if(inst->is_alloca())
                {
                    continue;
                }

                if(inst->get_parent()->get_name() != bbName)
                {
                    FindError = true;
                    LOG(WARNING) << "(" << func->get_name() << ")指令" << inst->get_name() << "的Parent基本块不正确！" ;
                    LOG(WARNING) << "应为："  << bbName;
                    LOG(WARNING) << "实际："  << inst->get_parent()->get_name();
                }
            }
        }
    }
}