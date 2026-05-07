#include "globalopt.h"

//#define GLOBALOPT_DEBUG

#ifdef GLOBALOPT_DEBUG
#define GLOBALOPT_DEBUG_PRINT(x) do { x; } while(0)
#else
#define GLOBALOPT_DEBUG_PRINT(x) do {} while(0)
#endif

void GlobalOptPass::Execute(){
    NoModRefGlobals.clear();
    RefOnlyGlobals.clear();
    ModOnlyGlobals.clear();
    ModRefGlobals.clear();
    global_map.clear();

    GlobalValTypeDef();

    // //std::cout<<"---------------------"<<std::endl;
    // //std::cout<<"NoModRef: ";
    // for(auto &inst:NoModRefGlobals){
    //     //std::cout<<((GlobalVarDefineInstruction*)inst)->GetName()<<"  ";
    // }//std::cout<<std::endl;

    // //std::cout<<"Ref: ";
    // for(auto &inst:RefOnlyGlobals){
    //     //std::cout<<((GlobalVarDefineInstruction*)inst)->GetName()<<"  ";
    // }//std::cout<<std::endl;

    // //std::cout<<"Mod: ";
    // for(auto &inst:ModOnlyGlobals){
    //     //std::cout<<((GlobalVarDefineInstruction*)inst)->GetName()<<"  ";
    // }//std::cout<<std::endl;

    // //std::cout<<"ModRef: ";
    // for(auto &inst:ModRefGlobals){
    //     //std::cout<<((GlobalVarDefineInstruction*)inst)->GetName()<<"  ";
    // }//std::cout<<std::endl;
    // //std::cout<<"---------------------"<<std::endl;

    ProcessGlobals();

}

void GlobalOptPass::GlobalValTypeDef(){
    auto main_cfg = llvmIR->llvm_cfg[llvmIR->FunctionNameTable["main"]];
    auto global_info = AA->globalmap[main_cfg];
    for(auto &inst:llvmIR->global_def){
        if(((GlobalVarDefineInstruction*)inst)->IsArray()){continue;}
        std::string global_name = "@"+((GlobalVarDefineInstruction*)inst)->GetName();
        if(global_info->ref_ops.count(global_name)){
            if(global_info->mod_ops.count(global_name)){
                ModRefGlobals.insert(inst);
            }else{
                RefOnlyGlobals.insert(inst);
            }
        }else if(global_info->mod_ops.count(global_name)){
            ModOnlyGlobals.insert(inst);
        }else{
            NoModRefGlobals.insert(inst);
        }
    }
    return ;
}

void GlobalOptPass::ProcessGlobals(){
    // 【1】NoModRef: delete
    for(auto &inst:NoModRefGlobals){
        llvmIR->global_def.erase(inst);
    }

    // 【2】RefOnly: 只读说明必有初值，用imm替换global var 的load/store
    std::unordered_map<std::string,Operand> ref_global_replace;
    for(auto &inst:RefOnlyGlobals){
        Operand init_val = ((GlobalVarDefineInstruction*)inst)->GetInitVal();
        assert(init_val->GetOperandType()==BasicOperand::IMMI32||init_val->GetOperandType()==BasicOperand::IMMF32);
        llvmIR->global_def.erase(inst);
        ref_global_replace["@"+((GlobalVarDefineInstruction*)inst)->GetName()]=init_val;
    }

    // 【3】ModOnly: 只写是没有意义的，删除global和相关操作
    std::unordered_set<std::string> mod_global_delete;
    for(auto &inst:ModOnlyGlobals){
        mod_global_delete.insert("@"+((GlobalVarDefineInstruction*)inst)->GetName());
    }

    // 【2】【3】执行替换、删除操作
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();){
                auto inst=*it;
                if(inst->GetOpcode()==BasicInstruction::LOAD){
                    auto ptr_name=((LoadInstruction*)inst)->GetPointer()->GetFullName();
                    if(ref_global_replace.count(ptr_name)){
                        int res_regno=((LoadInstruction*)inst)->GetDefRegno();
                        Operand imm=ref_global_replace[ptr_name];
                        //替换所有使用此处值的指令
                        for(auto &use_inst:cfg->use_map[res_regno]){
                            auto operands=use_inst->GetNonResultOperands();
                            for(auto &op:operands){
                                if(op->GetOperandType()==BasicOperand::REG){
                                    if(((RegOperand*)op)->GetRegNo()==res_regno){
                                        op=imm->Clone();
                                    }
                                }
                            }use_inst->SetNonResultOperands(operands);
                        }
                        it=block->Instruction_list.erase(it);
                        continue;
                    }
                    ++it;
                }else if(inst->GetOpcode()==BasicInstruction::STORE){//若store的值是冗余的，后续通过ADCE清除
                    auto ptr_name=((StoreInstruction*)inst)->GetPointer()->GetFullName();
                    if(mod_global_delete.count(ptr_name)){
                        it=block->Instruction_list.erase(it);
                        continue;
                    }
                    ++it;
                }else{
                    ++it;
                }
            }
        }
    }

    // 【4】ModRef的global，做类mem2reg：仅当函数调用前后进行load/store，其余时间在同一函数内部直接用寄存器
    for(auto &inst:ModRefGlobals){
        auto globaldef_inst=(GlobalVarDefineInstruction*)inst;
        auto global_name="@"+globaldef_inst->GetName();
        auto global_type=globaldef_inst->GetDataType();
        auto res=std::make_pair(global_type,globaldef_inst);
        global_map[global_name]=res;
    }

    ApproxiMem2reg();

    return ;
}


// 全局变量、数组指针存在冗余的load，删除 --> 同一block内对同一def不会有重复的load； 保证同一block内store后不再有load; 
/*
优化目标: 删除【块内】冗余load指令
    - 同一def不会有重复的load
    - store后不再有load 
    KPI：在保守与激进之间寻求平衡：
        - global / local ptr：如果是不同的ptr，一定不会发生别名冲突          ---> store后不再load，直接用store的value
        - param ptr: 根据调用时传参情况，有可能不同形参对应的实参是同一个指针  ---> 保守策略，不改变
        
*/
void GlobalOptPass::EliminateRedundantLS(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        for(auto &[id,block]:*(cfg->block_map)){
            std::unordered_map<std::string,LSInfo*> load_store_map;// ptr全名 ~ < 上一次是否为store, 上一次load的结果reg>

            for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();){
                    auto &inst=*it;
                    if(inst->GetOpcode()==BasicInstruction::STORE){
                        std::string ptr=((StoreInstruction*)inst)->GetPointer()->GetFullName();
                        auto value = ((StoreInstruction*)inst)->GetValue();
                        load_store_map[ptr]=new LSInfo(true,value);

                    }else if(inst->GetOpcode()==BasicInstruction::LOAD){
                        //跳过形参
                        auto info=AA->GetPtrInfo(((LoadInstruction*)inst)->GetPointer(),cfg);
                        if(info->type==PtrInfo::types::Param || info->type==PtrInfo::types::Undefed){
                            ++it;
                            continue;
                        }
                        std::string ptr=((LoadInstruction*)inst)->GetPointer()->GetFullName();
                        if(load_store_map.count(ptr)){
                            //【1】已经load过，删去重复的load 【2】刚刚store过，可用store的value替换load
                            if(!load_store_map[ptr]->defed ||
                               load_store_map[ptr]->defed && load_store_map[ptr]->load_res!=nullptr){
                                Operand value = load_store_map[ptr]->load_res;
                                int regno_todlt=inst->GetDefRegno();

                                auto use_insts=cfg->use_map[regno_todlt];
                                for(auto &use_inst:use_insts){
                                    if(use_inst==nullptr){continue;}
                                    auto operands=use_inst->GetNonResultOperands();
                                    for(auto &operand:operands){
                                        if(operand->GetOperandType()==BasicOperand::REG && 
                                            ((RegOperand*)operand)->GetRegNo()==regno_todlt){
                                            operand = value->Clone();
                                            if(value->GetOperandType()==BasicOperand::REG){
                                                cfg->use_map[((RegOperand*)value)->GetRegNo()].push_back(use_inst);
                                            }
                                        }
                                    } 
                                    use_inst->SetNonResultOperands(operands);
                                }
                                cfg->use_map.erase(regno_todlt);
                                it=block->Instruction_list.erase(it);
                                continue;
                            }
                            delete load_store_map[ptr];
                        }
                        load_store_map[ptr]=new LSInfo(false,((LoadInstruction*)inst)->GetResult());//保留此次load
                        
                    }else if(inst->GetOpcode()==BasicInstruction::CALL){
                        for(auto &[str,info]:load_store_map){
                            auto effect = AA->QueryCallGlobalModRef((CallInstruction*)inst,str);
                            if(effect==Mod || effect==ModRef){
                                info->defed=true;
                                info->load_res=nullptr;
                            }else{
                            }
                        }
                    }
                    ++it;
            }
        }
    }

   return ;
}

/*
- 0 def, multi uses : 仅做一次load，放置在入口块处，其余load删除
- 1 def, multi uses : 不删def，所有load均被替换删除 【考虑连襟的tradeoff】
    *load通过后续的ADCE删除
*/
void GlobalOptPass::OneDefDomAllUses(CFG* cfg){
    std::unordered_map<int,Operand> replace_map; // old_regno ~ new_Operand
    //std::unordered_set<std::string> globals_to_mem2reg;
    //std::cout<<" ------------- In Function "<<cfg->function_def->GetFunctionName()<<"------------"<<std::endl;
    //【1】识别one def 的global，记录替换map
    for(auto &[global,id_pairs]:def_blocks){
        //【1.1】Only Use ：将一个load提到最前，其余load删除
        if(id_pairs.size()==0){ 
            //std::cout<<global<<" is only used !"<<std::endl;
            auto global_name=global.substr(1);//去掉"@"
            cfg->max_reg+=1;
            auto def_value = GetNewRegOperand(cfg->max_reg);
            auto load_inst = new LoadInstruction(global_map[global].first,GetNewGlobalOperand(global_name),def_value);
            cfg->GetBlockWithId(0)->InsertInstruction(0,load_inst);

            //记录替换map
            auto use_pairs=use_blocks[global];
            for(auto &[use_id,inst]:use_pairs){
                if(inst->GetOpcode()==BasicInstruction::LOAD){
                    int use_regno=inst->GetDefRegno();
                    replace_map[use_regno]=def_value;
                }
            }
        }
        //【1.2】One Def
        else if(id_pairs.size()==1){
            int def_id=id_pairs.begin()->first; //the block id which def inst is in
            Instruction def_inst = (*id_pairs.begin()).second;
            //get def value
            Operand def_value;
            if(def_inst->GetOpcode()==BasicInstruction::STORE){
                def_value = ((StoreInstruction*)(*id_pairs.begin()).second)->GetValue();
                //std::cout<<" def "<<global<<" by STORE inst"<<std::endl;
            }else if(def_inst->GetOpcode()==BasicInstruction::CALL){
                auto global_name=global.substr(1);//去掉"@"
                cfg->max_reg+=1;
                def_value = GetNewRegOperand(cfg->max_reg);
                auto load_inst=new LoadInstruction(global_map[global].first,GetNewGlobalOperand(global_name),def_value);//可能产生冗余LOAD，可之后再进行一次ERLS!!!
                auto last_inst=cfg->GetBlockWithId(def_id)->Instruction_list.back();
                cfg->GetBlockWithId(def_id)->Instruction_list.pop_back();
                cfg->GetBlockWithId(def_id)->InsertInstruction(1,load_inst);
                cfg->GetBlockWithId(def_id)->InsertInstruction(1,last_inst);
                //std::cout<<" def "<<global<<" by CALL inst "<<((CallInstruction*)def_inst)->GetFunctionName()<<std::endl;
            }

            // register use value to replace
            auto use_pairs=use_blocks[global];
            for(auto &[use_id,inst]:use_pairs){
                if(cfg->getDomTree()->dominates(def_id,use_id)&&def_id!=use_id){
                    if(inst->GetOpcode()==BasicInstruction::LOAD){//支配且非同一块，才替换
                        int use_regno=inst->GetDefRegno();
                        replace_map[use_regno]=def_value;
                        ////std::cout<<"需要将 "<<use_regno<<" 的寄存器替换为 "<<def_value->GetFullName()<<std::endl;
                    }
                    // }else if(inst->GetOpcode()==BasicInstruction::CALL){//若同一块，则不删store了；非同一块，添加store
                    // }
                }
            }
        }
    }
    //【2】用store的value替换use之处
    for(auto &[id,block]:*(cfg->block_map)){
        for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();){
            auto inst=*it;
            if(inst->GetOpcode()!=BasicInstruction::STORE && inst->GetOpcode()!=BasicInstruction::LOAD)
            {
                auto operands=inst->GetNonResultOperands();
                for(auto &op:operands){
                    if(op->GetOperandType()==BasicOperand::REG){
                        int regno=((RegOperand*)op)->GetRegNo();
                        if(replace_map.count(regno)){
                            op=replace_map[regno]->Clone();
                        }
                    }
                }
                inst->SetNonResultOperands(operands);
            }
            ++it;
        }
    }
}

/*
函数内部将global val的访存提升为寄存器操作，当遇到函数调用时：
    - 若子函数Ref此global val，则调用前进行store(前提：call前修改过global val，否则不添加)   
    - 若子函数Mod此global val, 则调用后进行load（前提：call后仍需使用global val，否则不添加）--- 都添加，后续可通过ADCE消除
*/
void GlobalOptPass::ApproxiMem2reg(){

    //【0】处理DefUseInSameBlock ，为【2】做铺垫
    /*
           %r1 = load  @a           %r1 = load  @a
           store %r2-->@a     =>    store %r2-->@a
           %r3 = load  @a           
           use %r3                  use %r2
    */
    ////std::cout<<"<================= EliminateRedundantLS =====================>"<<std::endl<<std::endl;
    EliminateRedundantLS();

    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        
        ////std::cout<<"<=================    Gather Infomation =====================>"<<std::endl<<std::endl;
        //【1】收集信息
        def_blocks.clear();
        use_blocks.clear();
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::LOAD){
                    auto ptr=((LoadInstruction*)inst)->GetPointer();
                    if(ptr->GetOperandType()==BasicOperand::GLOBAL){
                        use_blocks[ptr->GetFullName()].push_back({id,inst});
                    }
                }else if(inst->GetOpcode()==BasicInstruction::STORE){
                    auto ptr=((StoreInstruction*)inst)->GetPointer();
                    if(ptr->GetOperandType()==BasicOperand::GLOBAL){
                        def_blocks[ptr->GetFullName()].push_back({id,inst});
                    }
                }else if(inst->GetOpcode()==BasicInstruction::CALL){
                    auto func_name=((CallInstruction*)inst)->GetFunctionName();
                    if(!lib_function_names.count(func_name)){
                        auto son_cfg=llvmIR->llvm_cfg[llvmIR->FunctionNameTable[func_name]];
                        //std::cout<<"[in globalopt.cc] func_name: "<<func_name<<std::endl;
                        assert(son_cfg!=nullptr);
                        auto info=AA->globalmap[son_cfg];
                        assert(info!=nullptr);
                        for(auto &op:info->ref_ops){
                            if(global_map.count(op)){//info中的信息当前已经删去了RefOnly，因此这里需要用ModRefGlobals过滤
                                use_blocks[op].push_back({id,inst});
                            }
                        }
                        for(auto &op:info->mod_ops){
                            if(global_map.count(op)){//info中的信息当前已经删去了RefOnly，因此这里需要用ModRefGlobals过滤
                                def_blocks[op].push_back({id,inst});
                            }
                        }
                    }
                }
            }
        }
        //补全def_blocks、use_blocks：只处理两种情况：def且use了  and 未def仅use
        for(auto &[global,uses]:use_blocks){
            if(!def_blocks.count(global)){
                std::vector<std::pair<int,Instruction>> null_set;
                def_blocks[global]=null_set;
            }
        }

        GLOBALOPT_DEBUG_PRINT(std::cerr << "=== GlobalOpt def/use Analysis ===" << std::endl);
        GLOBALOPT_DEBUG_PRINT(std::cerr << "def_blocks: "<< std::endl);
        for(auto &[op,set]:def_blocks){
            GLOBALOPT_DEBUG_PRINT(std::cerr << op<<" is defed in block ");
            for(auto &id_pair:set){
                GLOBALOPT_DEBUG_PRINT(std::cerr << id_pair.first<<" ");
            }
            GLOBALOPT_DEBUG_PRINT(std::cerr<<std::endl);
        }
        GLOBALOPT_DEBUG_PRINT(std::cerr << "use_blocks: "<< std::endl);
        for(auto &[op,set]:use_blocks){
            GLOBALOPT_DEBUG_PRINT(std::cerr << op<<" is used in block ");
            for(auto &id_pair:set){
                GLOBALOPT_DEBUG_PRINT(std::cerr << id_pair.first<<" ");
            }
            GLOBALOPT_DEBUG_PRINT(std::cerr<<std::endl);
        }

        //【2】处理OneDefDomAllUses : 用该def的value替换所有load处的res
        ////std::cout<<"<================= OneDefDomAllUses =====================>"<<std::endl<<std::endl;
        OneDefDomAllUses(cfg);
    }

    return ;
}