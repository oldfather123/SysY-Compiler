#include "AliasAnalysis.h"

std::unordered_map<std::string,ModRefStatus> lib_function_status ;

void AliasAnalysisPass::DefineLibFuncStatus(){
    lib_function_status["getint"]=Mod;
    lib_function_status["getch"]=Mod;
    lib_function_status["getfloat"]=Mod;
    lib_function_status["getarray"]=Mod;
    lib_function_status["getfarray"]=Mod;

    lib_function_status["putint"]=Ref;
    lib_function_status["putch"]=Ref;
    lib_function_status["putfloat"]=Ref;
    lib_function_status["putarray"]=Ref;
    lib_function_status["putfarray"]=Ref;

    lib_function_status["_sysy_starttime"]=NoModRef;
    lib_function_status["_sysy_stoptime"]=NoModRef;
    lib_function_status["llvm.memset.p0.i32"]=NoModRef;
    lib_function_status["llvm.umax.i32"]=NoModRef;
    lib_function_status["llvm.umin.i32"]=NoModRef;
    lib_function_status["llvm.smax.i32"]=NoModRef;
    lib_function_status["llvm.smin.i32"]=NoModRef;

    lib_function_status["___parallel_loop_constant_100"]=ModRef;
}   


//取指针操作数对应的PtrInfo结构体
PtrInfo* AliasAnalysisPass::GetPtrInfo(Operand op, CFG* cfg){
    // std::cout<<"[GetPtrInfo] In Function "<<cfg->function_def->GetFunctionName()<<" GetPtrInfo of op : ";
    // if(op==nullptr){
    //     std::cout<<"nullptr  .";
    // }else{
    //     std::cout<<op->GetFullName()<<"  .";
    // }
    
    if(op->GetOperandType()==BasicOperand::REG){
        int regno=((RegOperand*)op)->GetRegNo();
        return ptrmap[cfg][regno];
    }else if(op->GetOperandType()==BasicOperand::GLOBAL){
        auto global_info =new PtrInfo(PtrInfo::types::Global);
        global_info->root=op;
        return global_info;
    }else{
        return nullptr;
    }
}

//判定两个GEP指令是否具有相同的ptr、constIndexes
bool AliasAnalysisPass::IsSameArraySameConstIndex(GetElementptrInstruction* inst1,GetElementptrInstruction* inst2,double offset1,double offset2){
    //SysY未支持指针的数据类型，因此部分数组赋值仅存在于函数调用传参时；
    //即过程内别名分析不存在二级数组，故GEP指令中的ptr一定就是初次alloc时的ptr
    auto ptr1=inst1->GetPtrVal();
    auto ptr2=inst2->GetPtrVal();
    if(ptr1!=ptr2){ return false;}

    int index1=inst1->ComputeIndex();
    int index2=inst2->ComputeIndex();
    if(index1!=-1 && index2!=-1 && offset1!=-1 && offset2!=-1){
        if((index1+offset1)!=(index2+offset2)){
            return false;
        }
    }
    return true;//对于索引为reg的情况，无法判定，我们都认为存在别名冲突风险
}


/*
 过程内别名分析
1.GroundTruth：
    - 同数组不同静态索引的指针一定不存在别名冲突
    - SysY中产生指针的指令只有Getelementptr（源）和phi；产生数组基址的只有Global/Param/Alloc
2.Analysis:
    - case 1. 不属于同一array，一定不存在别名冲突
    - case 2. Global / Param / Local 彼此属于不同内存，不存在别名冲突
    - case 3. 整个array与内部元素一定存在别名冲突
    - case 4. 根对应的GEP指令同ptr、同ConstIndex --> MustAlias（衍生自位置不同但内容相同的GEP指令各自产生的源ptr)
    - 其它情况 --> NoAlias
*/
AliasStatus AliasAnalysisPass::QueryAlias(Operand op1, Operand op2, CFG* cfg){
    if(op1==op2 || op1 == nullptr || op2 == nullptr){
        return MustAlias;
    }

    // 对于立即数类型（IMMI32, IMMF32, IMMI64），它们不是指针，返回 NoAlias
    if(op1->GetOperandType() == BasicOperand::IMMI32 || 
       op1->GetOperandType() == BasicOperand::IMMF32 || 
       op1->GetOperandType() == BasicOperand::IMMI64 ||
       op2->GetOperandType() == BasicOperand::IMMI32 || 
       op2->GetOperandType() == BasicOperand::IMMF32 || 
       op2->GetOperandType() == BasicOperand::IMMI64){
        return NoAlias;
    }

    PtrInfo* info1=GetPtrInfo(op1,cfg);
    PtrInfo* info2=GetPtrInfo(op2,cfg); 
    // 对于非指针类型的REG，返回NoAlias
    if(info1 == nullptr || info2 == nullptr) { return NoAlias ;}
    
    // case 1. 不属于同一array，一定不存在别名冲突
    if(info1->root!=info2->root){
        return NoAlias;
    }

    // case 2. Global / Param / Local 彼此属于不同内存，不存在别名冲突
    if(info1->type!=PtrInfo::types::Undefed && info2->type!=PtrInfo::types::Undefed && info1->type!=info2->type){
        return NoAlias;
    }
    
    // case 3. 整个array与内部元素一定存在别名冲突
    if(info1->AliasOps.empty()||info2->AliasOps.empty()){
        if(info1->root==info2->root){
            return MustAlias;
        }
    }

    // case 4. 同类型同Array
    // case 4.1 若二者的别名集中存在重叠，则必由同一个指针别名而来
    if(info1->se_gep_offset==info2->se_gep_offset && info1->se_gep_offset!=INFINITY)
    for(auto &op:info1->AliasOps){
        if(info2->AliasOps.count(op)){
            return MustAlias;
        }
    }

    GetElementptrInstruction* inst1=info1->gep;
    GetElementptrInstruction* inst2=info2->gep;

	// 不存在 gep 指令表明为 op 为 alloca 指令的结果，表示为偏移为 0 的 gep 指令。
	// s.t. a 表示成 a[0]
    if (inst1 == nullptr && op1 != nullptr) {
        std::vector<int> dim; 
        std::vector<Operand> idxs = { new ImmI32Operand(0) };
        auto idx_typ = BasicInstruction::LLVMType::I32;
		auto typ = BasicInstruction::LLVMType::I32;  // 类型不影响后续判断
        inst1 = new GetElementptrInstruction(typ, op1, op1, dim, idxs, idx_typ);
    }
    if (inst2 == nullptr && op2 != nullptr) {
        std::vector<int> dim;
        std::vector<Operand> idxs = { new ImmI32Operand(0) };
        auto idx_typ = BasicInstruction::LLVMType::I32;
		auto typ = BasicInstruction::LLVMType::I32;  
        inst2 = new GetElementptrInstruction(typ, op2, op2, dim, idxs, idx_typ);
    }

    if(IsSameArraySameConstIndex(inst1, inst2 ,info1->se_gep_offset, info2->se_gep_offset)){
        return MustAlias;
    }
    //其它
    return NoAlias;

}



Operand AliasAnalysisPass::CalleeParamToCallerArgu(Operand op, CFG* callee_cfg, CallInstruction* CallI){
    if(op->GetOperandType()==BasicOperand::REG){
        auto callee_name = callee_cfg->function_def->GetFunctionName();
        auto callee_info = ReCallGraph[callee_name];
        if(callee_info.param_order.count(op)){//是形参，转换为caller的实参返回
            int order=callee_info.param_order[op];
            Operand argu=CallI->GetParameterList()[order].second;
            return argu;
        }else{//是local的，不予理会
            return nullptr;
        }
    }else if(op->GetOperandType()==BasicOperand::GLOBAL){//全局变量，直接返回
        return op;
    }
    return nullptr;
}

ModRefStatus AliasAnalysisPass::QueryInstModRef(Instruction inst, Operand op, CFG* cfg){
    if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
        LoadInstruction* load_inst = (LoadInstruction*)inst;
        Operand ptr=load_inst->GetPointer();
        if(QueryAlias(ptr,op,cfg)==MustAlias){
            return Ref;
        }
        return NoModRef;

    }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
        StoreInstruction* store_inst = (StoreInstruction*)inst;
        Operand ptr=store_inst->GetPointer();
        if(QueryAlias(ptr,op,cfg)==MustAlias){
            return Mod;
        }
        return NoModRef;

    }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::CALL){
        CallInstruction* callI=(CallInstruction*)inst;  
        //【1】若op为全局变量
        if(op->GetOperandType()==BasicOperand::GLOBAL){
            auto global_name = op->GetFullName();
            return QueryCallGlobalModRef(callI, global_name);
        }

        //【2】若op为其它变量
        auto callee_name = callI->GetFunctionName();
        // 【2.1】若是库函数，特殊处理
        if(lib_function_names.count(callee_name)){
            for(auto &[type,argu]:callI->GetParameterList()){
                if(type==BasicInstruction::LLVMType::PTR){
                    if(QueryAlias(op,argu,cfg)){
                        return lib_function_status[callee_name];
                    }
                }
            }
            return NoModRef;
        }
        //【2.2】非库函数，考虑该函数的读写情况
        auto callee_defI=llvmIR->FunctionNameTable[callee_name];
        CFG* callee_cfg=llvmIR->llvm_cfg[callee_defI];
        RWInfo rwinfo=rwmap[callee_cfg];
        int read=0,write=0;

        for(auto &rop:rwinfo.ReadRoots){
            auto rop2=CalleeParamToCallerArgu(rop,callee_cfg,callI);
            if(rop2!=nullptr){
                //std::cout<<"[in QueryInstModRef CALL]  rop: "<<rop2->GetFullName()<<" op: "<<op->GetFullName()<<std::endl;
                if(QueryAlias(rop2,op,cfg)==MustAlias){
                    read=1;
                    break;
                }   
            }
            
        }
        for(auto &wop:rwinfo.WriteRoots){
            auto wop2=CalleeParamToCallerArgu(wop,callee_cfg,callI);
            if(wop2!=nullptr){
                //std::cout<<"[in QueryInstModRef CALL]  wop: "<<wop2->GetFullName()<<" op: "<<op->GetFullName()<<std::endl;
                if(QueryAlias(wop2,op,cfg)==MustAlias){
                    write=2;
                    break;
                }
            }
        }

        return static_cast<ModRefStatus>(read+write);
    }
    return NoModRef;
}



//根据SSA def-use链，从前往后，将别名汇聚
void AliasAnalysisPass::GatherPtrInfos(CFG* cfg, int regno){
    PtrInfo* info=ptrmap[cfg][regno];
    std::vector<Instruction> use_insts = cfg->use_map[regno];
    for(auto &inst:use_insts){
        if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
            assert(((PhiInstruction*)inst)->GetType()==BasicInstruction::LLVMType::PTR);
            int res_regno=inst->GetDefRegno();
            PtrInfo* res_info=ptrmap[cfg][res_regno];
            if(res_info->root==nullptr){//对每个声明了的ptrinfo，只需记录一次
                res_info->AddAliass(info->AliasOps);//别名汇总
                res_info->type=info->type;//类型传递
                res_info->root=info->root;//祖宗传递
                res_info->se_gep_offset=info->se_gep_offset;//偏移传递
                res_info->gep=info->gep;//一代GEP指令传递
                GatherPtrInfos(cfg,res_regno);
            }
        }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::GETELEMENTPTR){
            int res_regno=inst->GetDefRegno();
            PtrInfo* res_info=ptrmap[cfg][res_regno];
            if(res_info->root==nullptr){//对每个声明了的ptrinfo，只需记录一次
                res_info->AddAliass(info->AliasOps);//别名汇总
                res_info->type=info->type;//类型传递
                res_info->root=info->root;//祖宗传递
                res_info->gep=info->gep;//一代GEP指令传递
                //累计偏移计算
                int offset=((GetElementptrInstruction*)inst)->ComputeIndex();
                if(offset==-1 || info->se_gep_offset == INFINITY){// INFINITY 代表二次偏移量为reg类型，无法获知
                    res_info->se_gep_offset = INFINITY;
                }else{
                    res_info->se_gep_offset= offset + info->se_gep_offset;
                }
                GatherPtrInfos(cfg,res_regno);
            }
        }
    }
    return ;
}

//别名分析信息收集：沿数据流图记录并汇聚别名集
void AliasAnalysisPass::PtrPropagationAnalysis(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        //std::cout<<"[in PtrPropagationAnalysis]  Now we analyse the function: "<<defI->GetFunctionName()<<std::endl;
        //【0】记录数组性质
        CallInfo callinfo;
        // (1)函数参数
        auto params = defI->GetNonResultOperands();
        auto param_types = defI->formals;
        for(int i=0;i<params.size();i++){
            if(param_types[i]==BasicInstruction::LLVMType::PTR){
                int regno = ((RegOperand*)params[i])->GetRegNo();
                auto info = new PtrInfo(PtrInfo::types::Param);
                info->root=params[i];
                ptrmap[cfg][regno]=info;//维护ptrmap

                callinfo.param_order[params[i]]=i;//记录函数形参与序号的映射
                //std::cout<<"[in PtrPropagationAnalysis]  Record a param PtrInfo: "<<params[i]->GetFullName()<<std::endl;
            }
        }
        ReCallGraph[defI->GetFunctionName()]=callinfo;
        //（2）函数内定义
        bool meet_alloca=false;
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst: block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::ALLOCA){
                    assert(((AllocaInstruction*)inst)->GetDims().size()>0);
                    Operand PtrOp = inst->GetResult();
                    int regno = ((RegOperand*)PtrOp)->GetRegNo();
                    auto info = new PtrInfo(PtrInfo::types::Local);
                    info->root=PtrOp;
                    ptrmap[cfg][regno]=info;
                    meet_alloca=true;
                }else{
                    if(meet_alloca){
                        break;
                    }
                }
            }
            if(meet_alloca){
                break;
            }
        }

        //【1】遍历所有cfg，记录会访问的ptr操作数（即由GEP、PHI指令产生）的信息
        for(auto &[regno,inst]:cfg->def_map){
            if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::GETELEMENTPTR){
                Operand ptr = ((GetElementptrInstruction*)inst)->GetPtrVal();
                PtrInfo* ptr_info = GetPtrInfo(ptr,cfg);
                if(ptr_info!=nullptr){//初次遍历，仅记录一代GEP指令;对于n代GEP指令，仅做初始化
                    //类型传递
                    auto info = new PtrInfo(PtrInfo::sources::Gep, ptr_info->type ,inst->GetResult());
                    info->root=ptr_info->root;
                    info->gep=(GetElementptrInstruction*)inst;
                    ptrmap[cfg][regno]=info;
                }else{
                    auto info = new PtrInfo(PtrInfo::sources::Gep,PtrInfo::types::Undefed,inst->GetResult());
                    ptrmap[cfg][regno]=info;
                }
            }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){//phi指令，仅做初始化
                if(((PhiInstruction*)inst)->GetResultType()==BasicInstruction::LLVMType::PTR){
                    auto info=new PtrInfo(PtrInfo::sources::Phi,PtrInfo::types::Undefed,inst->GetResult());
                    ptrmap[cfg][regno]=info;
                }
            }
        }
        //【2】汇聚非根ptr的所有别名——从初代GEP指令开始
        for(auto &[regno,info]:ptrmap[cfg]){
			if(info==nullptr){
				//std::cout<<"[in PtrPropagationAnalysis]  info is nullptr"<<std::endl;
				continue;
			}
            if(info->source==PtrInfo::sources::Gep && info->root!=nullptr){
                GatherPtrInfos(cfg,regno);
            }
        }
    }
}

//分析每个CFG的读写情况；并通过调用流汇总
void AliasAnalysisPass::RWInfoAnalysis(){
    //【1】收集每个CFG的读写信息，同时建立反向CallGraph
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        LeafFuncs.insert(defI->GetFunctionName());
        RWInfo rwinfo;
        rwinfo.has_lib_func_call = false;
        auto globalinfo = new GlobalValInfo;
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
                    Operand ptr=((LoadInstruction*)inst)->GetPointer();
                    if(ptr->GetOperandType()==BasicOperand::GLOBAL){//非array的全局变量
                        globalinfo->ref_ops.insert(ptr->GetFullName());
                    }else{
                        PtrInfo* info=GetPtrInfo(ptr,cfg);
                        if(info->type==PtrInfo::types::Global||info->type==PtrInfo::types::Param){//array的全局变量/参数
                            rwinfo.AddRead(info->root);
                        }
                    }
                }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
                    Operand ptr=((StoreInstruction*)inst)->GetPointer();
                    if(ptr->GetOperandType()==BasicOperand::GLOBAL){
                        globalinfo->mod_ops.insert(ptr->GetFullName());
                    }else{
                        PtrInfo* info=GetPtrInfo(ptr,cfg);
                        if(info->type==PtrInfo::types::Global||info->type==PtrInfo::types::Param){
                            rwinfo.AddWrite(info->root);
                        }
                    }
                }else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::CALL){//call指令暂时不记录，在汇总时分析
                    auto func_name=((CallInstruction*)inst)->GetFunctionName();
                    if(lib_function_names.count(func_name)){//库函数，必为leaf
                        ReCallGraph[func_name].callers[cfg]=(*(CallInstruction*)inst);//构建反向CallGraph
                        rwinfo.has_lib_func_call = true;
                    } else if(func_name!=defI->GetFunctionName()){//递归函数只记一次
                        ReCallGraph[func_name].callers[cfg]=(*(CallInstruction*)inst);//构建反向CallGraph
                        LeafFuncs.erase(defI->GetFunctionName());//调用了其他函数的函数不能作为leaf
                    }
                }
            }
        }
        rwmap[cfg]=rwinfo;
        globalmap[cfg]=globalinfo;
    }

    //【2】将子函数的读写信息汇聚到父函数中
    for(auto &func_name:LeafFuncs){
        GatherRWInfos(func_name);
    }
}

//将子函数的读写情况汇总到父函数中，作为call指令mod/ref的依据
void AliasAnalysisPass::GatherRWInfos(std::string func_name){
    //【1】库函数，无cfg，直接考虑参数
    if(lib_function_names.count(func_name)){
        CallInfo* callinfo=&ReCallGraph[func_name];
        for(auto &[caller_cfg,callI]:callinfo->callers){
            RWInfo* caller_rwinfo=&rwmap[caller_cfg];
            auto arguments=callI.GetParameterList();
            if(lib_function_status[func_name]==Ref){
                for(auto &[type,op]:arguments){
                    if(type==BasicInstruction::LLVMType::PTR){//仅对ptr处理
                        PtrInfo* caller_ptrinfo=GetPtrInfo(op,caller_cfg);//记录实参的root
                        caller_rwinfo->AddRead(caller_ptrinfo->root);
                    }
                }
            }else if(lib_function_status[func_name]==Mod){
                for(auto &[type,op]:arguments){
                    if(type==BasicInstruction::LLVMType::PTR){
                        PtrInfo* caller_ptrinfo=GetPtrInfo(op,caller_cfg);//记录实参的root
                        caller_rwinfo->AddWrite(caller_ptrinfo->root);
                    }
                }
            }
            //沿调用链反向传播汇总
            GatherRWInfos(caller_cfg->function_def->GetFunctionName());
        }
        return ;
    }
    //【2】非库函数
    else{
        auto callee_cfg=llvmIR->llvm_cfg[llvmIR->FunctionNameTable[func_name]];
        RWInfo rwinfo=rwmap[callee_cfg];
        CallInfo* callinfo=&ReCallGraph[func_name];
        for(auto &[caller_cfg,callI]:callinfo->callers){
            //【2.1】不含global var的rw信息汇总
            RWInfo* caller_rwinfo=&rwmap[caller_cfg];
            auto arguments=callI.GetParameterList();
            //read   只要全局变量和函数参数中的PTR
            for(auto &rop:rwinfo.ReadRoots){
                assert(rop!=nullptr);
                if(rop->GetOperandType()==BasicOperand::GLOBAL){
                    caller_rwinfo->AddRead(rop);
                }else if(rop->GetOperandType()==BasicOperand::REG){
                    PtrInfo* ptrinfo=GetPtrInfo(rop,callee_cfg);
                    if(ptrinfo->type==PtrInfo::types::Param){
                        // 只处理指针类型参数，i32 类型直接跳过（返回 NoAlias）
                        if(arguments[callinfo->param_order[rop]].first==BasicInstruction::LLVMType::PTR){
                            Operand arguOp=arguments[callinfo->param_order[rop]].second;//形参映射到实参
                            PtrInfo* caller_ptrinfo=GetPtrInfo(arguOp,caller_cfg);//记录实参的root
                                caller_rwinfo->AddRead(caller_ptrinfo->root);
                        } 
                    }
                }
            }
            //write
            for(auto &wop:rwinfo.WriteRoots){
                assert(wop!=nullptr);
                if(wop->GetOperandType()==BasicOperand::GLOBAL){
                    caller_rwinfo->AddWrite(wop);
                }else if(wop->GetOperandType()==BasicOperand::REG){
                    PtrInfo* ptrinfo=GetPtrInfo(wop,callee_cfg);
                    if(ptrinfo->type==PtrInfo::types::Param){
                        // 只处理指针类型参数，i32 类型直接跳过（返回 NoAlias）
                        if(arguments[callinfo->param_order[wop]].first==BasicInstruction::LLVMType::PTR){
                            Operand arguOp=arguments[callinfo->param_order[wop]].second;//形参映射到实参
                            PtrInfo* caller_ptrinfo=GetPtrInfo(arguOp,caller_cfg);//记录实参的root
                                caller_rwinfo->AddWrite(caller_ptrinfo->root);
                        } 
                    }
                }
            }

            //【2.2】global var的信息汇总
            globalmap[caller_cfg]->AddInfo(globalmap[callee_cfg]);

            //沿调用链反向传播汇总
            GatherRWInfos(caller_cfg->function_def->GetFunctionName());
        }
        return ;
    }
}


//寻找phi传递的ptr
void AliasAnalysisPass::FindPhi(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
                    auto phi=(PhiInstruction*)inst;
                    if(phi->GetResultType()==BasicInstruction::LLVMType::PTR){
                        //std::cout<<" phi in function "<<defI->GetFunctionName()<<std::endl;
                        return ;
                    }

                }
            }
        }
    }
    //std::cout<<"No Phi"<<std::endl;
    return ;
}


void AliasAnalysisPass::Execute(){
    ptrmap.clear();
    rwmap.clear();
    globalmap.clear();
    ReCallGraph.clear();
    LeafFuncs.clear();
    PtrPropagationAnalysis();
    RWInfoAnalysis();

    // for(auto &[defI,cfg]:llvmIR->llvm_cfg){
    //     std::cout<<"========================================"<<std::endl;
    //     std::cout<<"all the ptrmap info in this cfg: ";
    //     for(auto &[regno,ptrinfo]:ptrmap[cfg]){
    //         std::cout<<regno<<" ";
    //         if(ptrinfo==nullptr){std::cout<<"!! ";}
    //     }std::cout<<std::endl;
    //     std::cout<<"========================================"<<std::endl;
    //     if(ptrmap[cfg].count(2)){
    //         std::cout<<"ptrmap[cfg][2].root: "<<std::endl;
    //     }
    //     std::cout<<"========================================"<<std::endl;
    // }

    // test
    // PrintAAResult();
    // Test();
}


ModRefStatus AliasAnalysisPass::QueryCallGlobalModRef(CallInstruction* callI, std::string global_name){
    int res=0;
    auto func_name=callI->GetFunctionName();
    if(lib_function_names.count(func_name)){return NoModRef; } //库函数不对全局变量进行Mod/Ref 【传参/函数返回值方式的ModRef属于父函数】
    auto son_cfg=llvmIR->llvm_cfg[llvmIR->FunctionNameTable[func_name]];
    auto global_info=globalmap[son_cfg];
    if(global_info->ref_ops.count(global_name)){
        res+=1;
    }
    if(global_info->mod_ops.count(global_name)){
        res+=2;
    }
    return static_cast<ModRefStatus>(res);
}

/* -------------------------------  TEST ----------------------------------*/

std::string sour[3]={"Undef","Gep","Phi"};
std::string tys[4]={"Undefed","Global","Param","Local"};
std::string alias_status[2]={"NoAlias","MustAlias"};
std::string modref_status[4]={"NoModRef","Ref","Mod","ModRef"};

void PtrInfo::PrintDebugInfo(){
    //std::cout<<"source: "<<sour[source]<<" type: "<<tys[type]<<" root: ";
    if(root!=nullptr){
        //std::cout<<root->GetFullName() <<std::endl;
    }else{
        //std::cout<<"nullptr "<<std::endl;
    }
    //std::cout<<"Aliases: "<<std::endl;
    for(auto& op:AliasOps){
        //std::cout<<"         "<<op->GetFullName()<<std::endl;
    }
    //std::cout<<"se_gep_offset: "<<se_gep_offset<<std::endl;

}


void AliasAnalysisPass::PrintAAResult() {

    //std::cout<<" ----------- CallInfo ------------- "<<std::endl;
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //defI->PrintIR(//std::cout);
        auto callinfo=ReCallGraph[defI->GetFunctionName()];

        //std::cout<<"para_order:  "<<std::endl;
        for(auto&[op,order]:callinfo.param_order){
            //std::cout<<"Operand: "<<op->GetFullName()<<" order: "<<std::to_string(order)<<std::endl;
        }

        //std::cout<<"callers:  "<<std::endl;
        for(auto&[caller,xx]:callinfo.callers){
            //std::cout<<caller->function_def->GetFunctionName()<<"    ";
        }//std::cout<<std::endl;
        //std::cout<<"-------------" <<std::endl;

    }

    //std::cout<<" ----------- PtrInfo ------------- "<<std::endl;
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //defI->PrintIR(std::cout);
        for (auto [regno, info] : ptrmap[cfg]) {
            //std::cout << "REG: %r" << regno << "\n";
            info->PrintDebugInfo();
            //std::cout<<"-----------"<<std::endl;
        }
    }

    //std::cout<<" ----------- RWInfo -------------- "<<std::endl;
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //defI->PrintIR(std::cout);
        auto cfgrwinfo = rwmap[cfg];

        //std::cout << "read :   ";
        for (auto op : cfgrwinfo.ReadRoots) {
            //std::cout << op << " ";
        }
        //std::cout << "\n";

        //std::cout << "write :   ";
        for (auto op : cfgrwinfo.WriteRoots) {
            //std::cout << op << " ";
        }
        //std::cout << "\n" <<"\n";
    }

    //std::cout<<" ----------- Leafs -------------- "<<std::endl;
    for(auto func_name:LeafFuncs){
        //std::cout << func_name<<std::endl;
    }

    //std::cout<<" ----------- Globalvar Info -------------- "<<std::endl;
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //defI->PrintIR(std::cout);

        auto globalinfo=globalmap[cfg];
        //std::cout << "read :   ";
        for(auto op:globalinfo->ref_ops){
            //std::cout<<op<<" ";
        }//std::cout << "\n";

        //std::cout << "write :   ";
        for(auto op:globalinfo->mod_ops){
            //std::cout<<op<<" ";
        }//std::cout << "\n";
    }
    //std::cout<<" ----------------------------------------- "<<std::endl;
}


void AliasAnalysisPass::Test() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {

        if(defI->GetFunctionName()!="insert"){
            continue;
        }

        //defI->PrintIR(std::cout);

        //std::cout<<" ----------- AliasResult -------------- "<<std::endl;
        std::set<Operand> ptrset;
        for (auto [id, bb] : *(cfg->block_map)) {
            for (auto I : bb->Instruction_list) {
                if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::GETELEMENTPTR) {
                    ptrset.insert(I->GetResult());
                } else if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
                    ptrset.insert(((LoadInstruction *)I)->GetPointer());
                } else if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                    ptrset.insert(((StoreInstruction *)I)->GetPointer());
                }
            }
        }
        for (auto ptr1 : ptrset) {
            for (auto ptr2 : ptrset) {
                auto res=QueryAlias(ptr1, ptr2, cfg);
                if(res==MustAlias){
                    //std::cout << ptr1->GetFullName() << " " << ptr2->GetFullName() << " " << alias_status[res] << "\n";
                }
                
            }
        }

        //std::cout<<" ----------- ModRefResult -------------- "<<std::endl;
        for (auto ptr : ptrset) {
            PtrInfo ptrinfo=*GetPtrInfo(ptr,cfg);
            if(ptrinfo.source==PtrInfo::sources::Undef&&ptrinfo.type!=PtrInfo::types::Global){
                continue;
            }
            for (auto [id, bb] : *(cfg->block_map)) {
                for (auto I : bb->Instruction_list) {
                    if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::CALL) {
                        if(((CallInstruction*)I)->GetFunctionName()=="radixSort"){
                            I->PrintIR(std::cout);
                            std::cout << ptr->GetFullName() << " " << modref_status[QueryInstModRef(I, ptr, cfg)] << "\n";
                        }
                        if(!lib_function_names.count(((CallInstruction*)I)->GetFunctionName())){
                            I->PrintIR(std::cout);
                            std::cout << ptr->GetFullName() << " " << modref_status[QueryInstModRef(I, ptr, cfg)] << "\n";
                        }


                    } else if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD) {
                        I->PrintIR(std::cout);
                        std::cout << ptr->GetFullName() << " " << modref_status[QueryInstModRef(I, ptr, cfg)] << "\n";
                    } else if (I->GetOpcode() == BasicInstruction::LLVMIROpcode::STORE) {
                        I->PrintIR(std::cout);
                        std::cout << ptr->GetFullName() << " " << modref_status[QueryInstModRef(I, ptr, cfg)] << "\n";
                    }
                }
            }
        }
    }
}   

// 判断函数是否有副作用（side effect）
bool AliasAnalysisPass::HasSideEffect(CFG* cfg) {
    if (cfg == nullptr) {
        return false;
    }
    
    // 检查 RWInfo 中的写操作
    auto& rwinfo = rwmap[cfg];
    if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) {
        return true;
    }
    
    // 检查全局变量的写操作
    auto global_info = globalmap[cfg];
    if (global_info != nullptr && !global_info->mod_ops.empty()) {
        return true;
    }
    
    return false;
}

bool AliasAnalysisPass::HasSideEffect(const std::string& func_name) {
    // 检查是否是库函数
    if (lib_function_names.count(func_name)) {
        // 库函数根据其状态判断是否有副作用
        auto status = lib_function_status[func_name];
        return (status == Mod || status == ModRef);
    }
    
    // 查找对应的 CFG
    auto func_defI = llvmIR->FunctionNameTable.find(func_name);
    if (func_defI == llvmIR->FunctionNameTable.end()) {
        return false; // 找不到函数定义，保守地认为有副作用
    }
    
    auto cfg = llvmIR->llvm_cfg[func_defI->second];
    return HasSideEffect(cfg);
}   