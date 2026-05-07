#include "basic_cse.h"
#include <algorithm>
#include <cassert>
#include<functional>
#include <stack>

//#define CSE_DEBUG

#ifdef CSE_DEBUG
#define CSE_DEBUG_PRINT(x) do { x; } while(0)
#else
#define CSE_DEBUG_PRINT(x)
#endif

//记得修改cse_init!!!以及注意GetCSEInfo
//DomAnalysis *domtrees;
//AliasAnalysisPass *alias_analyser;
extern std::unordered_map<std::string, CFG*> cfgTable;
// 获取指令的CSE特征信息
static InstCSEInfo GetCSEInfo(Instruction inst) {
    InstCSEInfo info;
    info.opcode = inst->GetOpcode();
    auto operands = inst->GetNonResultOperands();
    std::string op1, op2;
    
    switch(info.opcode) {
        case BasicInstruction::LOAD: {
            auto load_inst = static_cast<LoadInstruction*>(inst);
            auto ptr = load_inst->GetPointer();
            
            // 检查是否是对全局变量的load
            if (ptr->GetOperandType() == BasicOperand::GLOBAL) {
                // 对于全局变量的load，添加一个特殊标识，避免CSE优化
                info.operand_list.push_back("GLOBAL_LOAD_" + ptr->GetFullName());
                // 添加指令的唯一标识，确保每个全局变量load都被视为不同的
                info.operand_list.push_back("INST_" + std::to_string(reinterpret_cast<uintptr_t>(inst)));
            } else {
                // 对于非全局变量的load，正常处理
                info.operand_list.push_back(ptr->GetFullName());
            }
            break;
        }
        case BasicInstruction::CALL: {
            auto call_inst = static_cast<CallInstruction*>(inst);
            info.operand_list.push_back(call_inst->GetFunctionName());
            for (auto op : operands) {
                info.operand_list.push_back(op->GetFullName());
            }
            break;
        }
        case BasicInstruction::ADD:    // 操作数可交换
        case BasicInstruction::FADD:
        case BasicInstruction::FMUL:
        case BasicInstruction::MUL: {  // 操作数可交换
            assert(operands.size() == 2);
            
            // 创建排序后的操作数列表
            op1 = operands[0]->GetFullName();
            op2 = operands[1]->GetFullName();
            
            if (op1 > op2) 
                std::swap(op1, op2);
            
            info.operand_list.push_back(std::move(op1));
            info.operand_list.push_back(std::move(op2));
            break;
        }
        case BasicInstruction::GETELEMENTPTR: {

            auto gep_inst = static_cast<GetElementptrInstruction*>(inst);
        
            // 添加基本操作数
            for (auto op : operands) {
                info.operand_list.push_back(op->GetFullName());
            }
            
            // 添加维度信息
            for (auto dim : gep_inst->GetDims()) {
                info.operand_list.push_back(std::to_string(dim));
            }
            break;
        }
        case BasicInstruction::ICMP:
            {auto IcmpI = (IcmpInstruction *)inst;
            info.cond = IcmpI->GetCompareCondition(); // 记录比较条件
            }
        case BasicInstruction::FCMP:
            {auto FcmpI = (FcmpInstruction *)inst;
            info.cond = FcmpI->GetCompareCondition(); // 记录比较条件;
            }
        default:
            info.operand_list.reserve(operands.size());
            for (auto op : operands) {
                info.operand_list.push_back(op->GetFullName());
            }
            break;
    }  
    return info;
}

int GetResultRegNo(Instruction inst)
{
    auto result = (RegOperand*)inst->GetResult();
    if(result!=nullptr){return result->GetRegNo();}
    return -1;
}

// InstCSEInfo的比较运算符实现
bool InstCSEInfo::operator<(const InstCSEInfo &other) const {
    if (opcode != other.opcode) 
        {return opcode < other.opcode;}
    
    if (cond != other.cond) 
        {return cond < other.cond;}
    
    
    if (operand_list.size() != other.operand_list.size())
        {return operand_list.size() < other.operand_list.size();}
    
    return std::lexicographical_compare(
        operand_list.begin(), operand_list.end(),
        other.operand_list.begin(), other.operand_list.end()
    );
}
bool BasicBlockCSEOptimizer::IsValChanged(Instruction I1, Instruction I2, CFG *C) {
    //1.获取指令1的内存指向
    Operand ptr1,ptr2;
    if (I1->GetOpcode() == BasicInstruction::LOAD) {
        ptr1 = ((LoadInstruction *)I1)->GetPointer();
    } else if (I1->GetOpcode() == BasicInstruction::STORE) {
        ptr1 = ((StoreInstruction *)I1)->GetPointer();
    }
    if (I2->GetOpcode() == BasicInstruction::LOAD) {
        ptr2 = ((LoadInstruction *)I2)->GetPointer();
    } else if (I2->GetOpcode() == BasicInstruction::STORE) {
        ptr2 = ((StoreInstruction *)I2)->GetPointer();
    }
    assert(ptr1==ptr2);
    //2.获取I1正向遍历，I2反向遍历的基本块交集（？）
    int id1=I1->GetBlockID();
    int id2=I2->GetBlockID();
    std::set<int> blocks;
    assert(id1 != id2);
    std::set<int> id1_tos, to_id2s;

    std::queue<int> q;
    std::vector<int> vis;
    vis.resize(C->max_label + 1);
    q.push(id1);
    //1.BFS编号为id1的基本块的所有后继块，所有可达块记录在id1_tos中
    while (!q.empty()) {
        int x = q.front();
        q.pop();
        if (vis[x]) {
            continue;
        }
        id1_tos.insert(x);
        vis[x] = 1;
        for (auto v : C->GetSuccessor(x)) {
            q.push(v->block_id);
        }
    }

    vis.clear();
    vis.resize(C->max_label + 1);
    q.push(id2);
    //2.BFS编号为id2的基本块的所有前驱块，所有可达块记录在id2_tos中（不记录id1的前驱块）
    while (!q.empty()) {
        int x = q.front();
        q.pop();
        if (vis[x]) {
            continue;
        }
        to_id2s.insert(x);
        vis[x] = 1;
        if (x == id1) {
            continue;
        }
        for (auto v : C->GetPredecessor(x)) {
            q.push(v->block_id);
        }
    }
    //3.记录两组路径的基本块交集
    for (int i = 0; i <= C->max_label; ++i) {
        if (id1_tos.find(i) != id1_tos.end() && to_id2s.find(i) != to_id2s.end()) {
            blocks.insert(i);
        }
    }

    if (blocks.size() == 0) {
        return false;
    }
    //3.遍历交集基本块
    for (auto id : blocks) {
        auto bb = (*C->block_map)[id];
        int st = 0, ed = bb->Instruction_list.size();
        //1)找到I1所在基本块及I1所在位置
        if (id == I1->GetBlockID()) {
            int Iindex = -1;
            auto IBB = (*C->block_map)[I1->GetBlockID()];
            for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
                auto tmpI = IBB->Instruction_list[i];
                if (tmpI == I1) {
                    Iindex = i;
                    break;
                }
            }
            assert(Iindex != -1);
            st = Iindex + 1;
        //2)找到I2所在基本块及指令所在位置
        } else if (id == I2->GetBlockID()) {
            int Iindex = -1;
            auto IBB = (*C->block_map)[I2->GetBlockID()];
            for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
                auto tmpI = IBB->Instruction_list[i];
                if (tmpI == I2) {
                    Iindex = i;
                    break;
                }
            }
            assert(Iindex != -1);
            ed = Iindex;
        }
        //3)遍历I1之后的指令，I2之前的指令，其他块中的指令
        for (int i = st; i < ed; ++i) {
            auto I = bb->Instruction_list[i];
            if(I->GetOpcode()==BasicInstruction::BR_COND||I->GetOpcode()==BasicInstruction::BR_UNCOND||I->GetOpcode()==BasicInstruction::OTHER||I->GetOpcode()==BasicInstruction::RET)
            {
                continue;
            }
            if(alias_analyser->QueryInstModRef(I,ptr1,C)==Mod||alias_analyser->QueryInstModRef(I,ptr1,C)==ModRef)
            {return true;}
            if(I->GetResult()==ptr1){return true;}
            
        }
    }
    // std::cerr<<"NoStore\n";
    return false;
}
// CSE初始化函数实现
void SimpleCSEPass::CSEInit(CFG* cfg) {
    // 1. 把store立即数转换成store寄存器：add reg,val,0;store reg mem
    // 2.设置blockID
    for (auto& block_pair : *(cfg->block_map)) {
        BasicBlock* bb = block_pair.second;
        std::deque<Instruction> new_instrs;
        
        for (BasicInstruction* inst : bb->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::STORE) {
                StoreInstruction* store_inst = static_cast<StoreInstruction*>(inst);
                BasicOperand* val = store_inst->GetValue();
                auto type = val->GetOperandType();
                // store_inst->PrintIR(std::cerr);
                // std::cerr<<"store_type="<<type<<"\n";
                // 只处理支持的两种类型
                if (type == BasicOperand::IMMI32 || type ==BasicOperand::IMMF32 ) {
                    // std::cerr<<"存在立即数store\n";
                    // store_inst->PrintIR(std::cerr);
                    // 创建新寄存器
                    BasicOperand* new_reg = GetNewRegOperand(++cfg->max_reg);
                    auto new_intr = (type == BasicOperand::IMMI32) ? 
                        new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, val, new ImmI32Operand(0), new_reg) :
                        new ArithmeticInstruction(BasicInstruction::FADD, BasicInstruction::FLOAT32, val, new ImmF32Operand(0), new_reg);
                    new_intr->SetBlockID(block_pair.first);
                    new_instrs.push_back(new_intr);
                   // new_intr->PrintIR(std::cerr);
                    // 更新store操作数
                    store_inst->SetValue(new_reg);
                   // store_inst->PrintIR(std::cerr);
                }
            }
            inst->SetBlockID(block_pair.first);
            new_instrs.push_back(inst); // 添加原始指令
        }
        // 替换原指令列表
        bb->Instruction_list = new_instrs;
    }
    for (auto [id, bb] : *cfg->block_map) {
        for (auto I : bb->Instruction_list) {
            I->SetBlockID(id);
        }
    }
}

// 判断指令是否不考虑CSE
bool CSENotConsider(Instruction I) {
    // 跳过控制流/内存分配/比较等指令
    if (I->GetOpcode() == BasicInstruction::PHI || I->GetOpcode() == BasicInstruction::BR_COND || 
        I->GetOpcode() == BasicInstruction::BR_UNCOND || I->GetOpcode() == BasicInstruction::ALLOCA ||
        I->GetOpcode() == BasicInstruction::RET || I->GetOpcode() == BasicInstruction::ICMP || 
        I->GetOpcode() == BasicInstruction::FCMP) {
        return false;
    }
    return true;
}
bool BranchCSENotConsider(Instruction I) {
    // 跳过控制流/内存分配/比较等指令
    if (I->GetOpcode() == BasicInstruction::PHI || I->GetOpcode() == BasicInstruction::BR_COND || 
        I->GetOpcode() == BasicInstruction::BR_UNCOND || I->GetOpcode() == BasicInstruction::ALLOCA ||
        I->GetOpcode() == BasicInstruction::RET) {
        return false;
    }
    return true;
}

void BasicBlockCSEOptimizer::NoMemoptimize() {
    hasMemOp=false;
    optimize();
    hasMemOp=true;
}
// BasicBlockCSEOptimizer成员函数实现
bool BasicBlockCSEOptimizer::optimize() {
    // 多次迭代直到没有变化
    while (true) {
        reset();
        processAllBlocks();
        removeDeadInstructions();
        applyRegisterReplacements();
        
        if (!changed) break;
    }
    return changed;
}

void BasicBlockCSEOptimizer::reset() {
    changed = false;
    reg_replace_map.clear();
    erase_set.clear();
}
void BasicBlockCSEOptimizer::inst_clear()
{
    CallInstMap.clear();
    LoadInstMap.clear();
    LoadInstSet.clear();
    CallInstSet.clear();
}
void BasicBlockCSEOptimizer::clearAllCSEInfo() {
    inst_map.clear();
    InstSet.clear();
    LoadInstMap.clear();
    LoadInstSet.clear();
    CallInstMap.clear();
    CallInstSet.clear();
}
void BasicBlockCSEOptimizer::processAllBlocks() {
    for (auto [id, bb] : *C->block_map) {
        clearAllCSEInfo();
        changed|=processBlock(bb);
    }
}

bool BasicBlockCSEOptimizer::processBlock(LLVMBlock bb) {
    auto instructions = bb->Instruction_list;
    flag=changed;
    for (auto I : instructions) {
        if (!CSENotConsider(I)) continue;
        
        switch (I->GetOpcode()) {
            case BasicInstruction::CALL:
                if(hasMemOp){processCallInstruction(static_cast<CallInstruction*>(I));}
                break;
            case BasicInstruction::STORE:
                if(hasMemOp){processStoreInstruction(static_cast<StoreInstruction*>(I));}
                break;
            case BasicInstruction::LOAD:
                if(hasMemOp){processLoadInstruction(static_cast<LoadInstruction*>(I));}
                break;
            default:
                processRegularInstruction(I);
                break;
        }
    }
    return flag;
}

void BasicBlockCSEOptimizer::processCallInstruction(CallInstruction* CallI) {
    //std::cout<<"[CSE] Processing call instruction: "<<CallI->GetFunctionName()<<std::endl;
    //1.不处理外部调用（lib_func），清理即可
    if (cfgTable.find(CallI->GetFunctionName()) == cfgTable.end()) {
        //std::cout<<"[CSE] External call detected, clearing all CSE info"<<std::endl;
        clearAllCSEInfo();
        return;    // external call, clear all instructions
    }
    //2.获取cfg与cfg对应的读写内存信息
    auto cfg = cfgTable[CallI->GetFunctionName()];
    if(cfg==nullptr){return;}
    //std::cout<<"getcfg\n";
    auto& rwmap = alias_analyser->GetRWMap();
    auto it = rwmap.find(cfg);
    if (it == rwmap.end()) {
        return;
    }
    const RWInfo& rwinfo = it->second;
    //std::cout<<"getrwinfo\n";
	if(alias_analyser->HasSideEffect(cfg))
    // if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) // write memory, can not CSE, and will kill some Load and Call
    {
        //std::cout<<"[CSE] Call has side effects - WriteRoots: "<<rwinfo.WriteRoots.size()<<", has_lib_func_call: "<<rwinfo.has_lib_func_call<<std::endl;
        inst_map.clear();//新增，清除普通指令
        InstSet.clear();
        //std::cout<<"write call\n";
        //CallI->PrintIR(std::cerr);
        //1.如果是外部函数调用，清理后直接返回
        if (cfgTable.find(CallI->GetFunctionName()) == cfgTable.end()) {
            // for simple, we do not consider independent call there, this can be CSE in DomTreeWalkCSE
            // I->PrintIR(std::cerr);std::cerr<<"kill everything\n";
            inst_clear();
            return;    // external call
        }
        auto cfg = cfgTable[CallI->GetFunctionName()];
        assert(cfg!=nullptr);
        auto& rwmap = alias_analyser->GetRWMap();
        auto it = rwmap.find(cfg);
        assert(it!= rwmap.end());
        const RWInfo& rwinfo = it->second;
        //2.如果当前控制流图中有外部函数调用，也清理后直接返回
        if (rwinfo.has_lib_func_call) {
            //std::cout<<"[CSE] Call contains lib function call, clearing all CSE info"<<std::endl;
            // for simple, we do not consider independent call there, this can be CSE in DomTreeWalkCSE
            // I->PrintIR(std::cerr);std::cerr<<"kill everything\n";
            inst_clear();
            return;
        }
        //3.当前控制流图中没有外部函数调用，遍历Load集合
        for (auto it = LoadInstSet.begin(); it != LoadInstSet.end();) {
            //1)获取load指令指向的内存位置PTR
            assert((*it)->GetOpcode() == BasicInstruction::LOAD);
            auto LoadI = (LoadInstruction *)(*it);
            auto ptr = LoadI->GetPointer();
            //2)查询当前call指令是否修改了内存PTR
            auto result = alias_analyser->QueryInstModRef(CallI, ptr, C);
            //3)如果当前call指令修改了某load指令的内存，则从映射和集合中删除该load指令
            if (result == ModRefStatus::Mod || result ==ModRefStatus::ModRef) {
                // I->PrintIR(std::cerr);std::cerr<<"kill ";(*it)->PrintIR(std::cerr);

                LoadInstMap.erase(GetCSEInfo(*it));
                it = LoadInstSet.erase(it);
            //4)反之遍历下一条指令
            } else {
                ++it;
            }
        }
        //4.获取call指令修改的内存集合，逐一遍历，存储实际写入的指针
        auto writeptrs = rwinfo.WriteRoots;
        std::vector<Operand> real_writeptrs;//存储实际写入地址
        // for (auto ptr : writeptrs) {
        //     real_writeptrs.push_back(ptr);
        // }
        auto& glmap = alias_analyser->GetPtrMap();
        auto glptrs=glmap[cfg];
        for(auto [regno,ptrinfo]:glptrs)
        {
            for(auto ptr:ptrinfo->AliasOps)
            {
                if (ptr->GetOperandType() == BasicOperand::GLOBAL) {
                    real_writeptrs.push_back(ptr);
                }
                else if (ptr->GetOperandType() == BasicOperand::REG) {
                    int ptr_regno = ((RegOperand *)ptr)->GetRegNo();
                    //std::cerr<<"ptr_regno= "<<ptr_regno<<"; call_param_size= "<< CallI->GetParameterList().size()<<"\n";
                    //assert(ptr_regno < CallI->GetParameterList().size());
                    if(ptr_regno< CallI->GetParameterList().size())
                    {
                        auto [type, real_ptr2] = CallI->GetParameterList()[ptr_regno];
                        real_writeptrs.push_back(real_ptr2);
                    }
                    else
                    {
                        real_writeptrs.push_back(ptr);
                    }
                } else {    // should not reach here
                    assert(false);
                }
            }
        }
        for (auto ptr : writeptrs) {
            //1)如果为全局变量，直接加入向量
            if (ptr->GetOperandType() == BasicOperand::GLOBAL) {
                real_writeptrs.push_back(ptr);
            //2)如果为局部变量（寄存器），那么一定在参数列表，获取调用时传入的实际指针(别名分析已经获取实际指针)
            //如果有修改函数参数列表的优化，需要在此处优化之后进行
            } else if (ptr->GetOperandType() == BasicOperand::REG) {
                int ptr_regno = ((RegOperand *)ptr)->GetRegNo();
                //std::cerr<<"ptr_regno= "<<ptr_regno<<"; call_param_size= "<< CallI->GetParameterList().size()<<"\n";
                //assert(ptr_regno < CallI->GetParameterList().size());
                if(ptr_regno< CallI->GetParameterList().size())
                {
                    auto [type, real_ptr2] = CallI->GetParameterList()[ptr_regno];
                    real_writeptrs.push_back(real_ptr2);
                }
                else
                {
                    real_writeptrs.push_back(ptr);
                }
            } else {    // should not reach here
                assert(false);
            }
        }
        //5.遍历call指令集合
        for (auto it = CallInstSet.begin(); it != CallInstSet.end();) {
            assert((*it)->GetOpcode() == BasicInstruction::CALL);
            bool is_needkill = false;
            //1）如果存在call指令修改了当前call指令的内存，则标记冲突（？自己和自己不会冲突吗）
            for (auto ptr : real_writeptrs) {
                if (alias_analyser->QueryInstModRef(*it, ptr, C) != ModRefStatus::NoModRef) {
                    is_needkill = true;
                    break;
                }
            }
            //2)将冲突的call指令从callinstmap中删除
            if (is_needkill) {
                //std::cout<<"[CSE] Killing conflicting call instruction due to memory conflict"<<std::endl;
                // I->PrintIR(std::cerr);std::cerr<<"kill ";(*it)->PrintIR(std::cerr);
                CallInstMap.erase(GetCSEInfo(*it));
                it = CallInstSet.erase(it);
            } else {
                ++it;
            }
        }
    }
    else    // only read memory, we can CSE
    {
        //std::cout<<"[CSE] Call is read-only, attempting CSE"<<std::endl;
        //std::cout<<"read call\n";
        auto Info = GetCSEInfo(CallI);
        auto CSEiter = CallInstMap.find(Info);
        if (CSEiter != CallInstMap.end()) {
            //std::cout<<"[CSE] *** CSE ELIMINATION: Eliminating duplicate call instruction ***"<<std::endl;
            //std::cout<<"[CSE] Original call result reg: "<<CSEiter->second<<", Current call result reg: "<<GetResultRegNo(CallI)<<std::endl;
            erase_set.insert(CallI);
            reg_replace_map[GetResultRegNo(CallI)] = CSEiter->second;
            flag= true;
        } else {
            //std::cout<<"[CSE] Adding call to CSE map for future elimination"<<std::endl;
            CallInstSet.insert(CallI);
            CallInstMap.insert({Info, GetResultRegNo(CallI)});
        }
    } 

}

void BasicBlockCSEOptimizer::processStoreInstruction(StoreInstruction* StoreI) {
    // store instructions, this will kill some loads
    //1.删除与当前store指令涉及同一处内存的，call指令与load指令
    auto ptr =StoreI->GetPointer();
    
    //2.遍历load指令集合
    for (auto it = LoadInstSet.begin(); it != LoadInstSet.end();) {
        //1)如果存在load指令加载了此处内存，则从load指令集合中删除该条load指令
        auto result = alias_analyser->QueryInstModRef(*it, ptr, C);
        auto ptr2=((LoadInstruction*)(*it))->GetPointer();
        if ((result == ModRefStatus::Ref||result==ModRefStatus::ModRef)||alias_analyser->QueryAlias(ptr,ptr2,C)==MustAlias) {    // if load instruction ref the ptr of store instruction
            // I->PrintIR(std::cerr);std::cerr<<"kill ";(*it)->PrintIR(std::cerr);

            LoadInstMap.erase(GetCSEInfo(*it));
            it = LoadInstSet.erase(it);
        //2)反之遍历下一条即可
        } else {
            ++it;
        }
    }
    //3.遍历call指令集合
    for (auto it = CallInstSet.begin(); it != CallInstSet.end();) {
        //1)如果call指令读取或修改了此处内存，则从映射和集合中删除该条call指令
        auto result = alias_analyser->QueryInstModRef(*it, ptr, C);
        if (result != ModRefStatus::NoModRef) {
            // I->PrintIR(std::cerr);std::cerr<<"kill ";(*it)->PrintIR(std::cerr);

            CallInstMap.erase(GetCSEInfo(*it));
            it = CallInstSet.erase(it);
        } else {
            ++it;
        }
    }

    /* then the store can generate a new load value
        store %rx -> ptr %p0
        %ry = load ptr %p0
        this will be optimized to %ry = %rx
    */
   //2.确保已经把所有store 立即数的指令转换成 store 寄存器的指令
    assert(StoreI->GetValue()->GetOperandType() == BasicOperand::REG);
    //3.获取寄存器编号
    int val_regno = ((RegOperand *)StoreI->GetValue())->GetRegNo();
    
    if (reg_replace_map.find(val_regno) != reg_replace_map.end()) {
        val_regno = reg_replace_map[val_regno];
    }
    for(auto it = InstSet.begin(); it !=InstSet.end();)
    {
        auto ins=*it;
        int f=false;
        for(auto regno:ins->GetUseRegno())
        {
            if(regno==val_regno)
            {
                f=true;
            }
        }
        if(f)
        {
            inst_map.erase(GetCSEInfo(*it));
            it = InstSet.erase(it);
        }
        else
        {
            ++it;
        }
        
    }
    //StoreI->PrintIR(std::cerr);
    //4.对store生成对应的load指令，插入到load映射与集合中
    auto LoadI = new LoadInstruction(StoreI->GetDataType(), StoreI->GetPointer(), GetNewRegOperand(val_regno));
    auto Info = GetCSEInfo(LoadI);
    auto it=LoadInstMap.find(Info);
    if(it!=LoadInstMap.end())
    {
        LoadInstSet.erase(it->second);
        LoadInstMap.erase(it);
    }
    //LoadI->PrintIR(std::cerr);
    LoadInstSet.insert(LoadI);
    LoadInstMap.insert({Info, LoadI});
}

void BasicBlockCSEOptimizer::processLoadInstruction(LoadInstruction* loadI) {
    //1.根据cse_info寻找对应的load指令
    auto Info = GetCSEInfo(loadI);
    auto CSEiter = LoadInstMap.find(Info);
    //2.如果在load映射中找到：
    if (CSEiter != LoadInstMap.end()) {
        if(!memdep_analyser->isLoadSameMemory(loadI,CSEiter->second,C)){return;}
        //1)把当前load指令插入erase_set
        erase_set.insert(loadI);
        // I->PrintIR(std::cerr);
        //2)把当前load指令的寄存器编号修改成映射中存储的寄存器编号
        reg_replace_map[GetResultRegNo(loadI)] =GetResultRegNo( CSEiter->second);
        //3)标记flag为true(?)
        flag = true;
        
    //3.如果没有在load映射中找到cse信息，则插入集合与映射中
    } else {
        LoadInstSet.insert(loadI);
        LoadInstMap.insert({Info, loadI});
    }
}

void BasicBlockCSEOptimizer::processRegularInstruction(BasicInstruction* I) {
    InstCSEInfo info = GetCSEInfo(I);
    
    if (inst_map.find(info) != inst_map.end()) {
        erase_set.insert(I);
        reg_replace_map[GetResultRegNo(I)] = GetResultRegNo(inst_map[info]);
        flag = true;
    } else {
        inst_map[info] = (I);
        InstSet.insert(I);
    }
}

void BasicBlockCSEOptimizer::removeDeadInstructions() {
    // std::cerr<<"erase_set:\n";
    // for(auto ins:erase_set)
    // {
    //     ins->PrintIR(std::cerr);
    // }
    // std::cerr<<"======\n";
    for (auto [id, bb] : *C->block_map) {
        std::deque<Instruction> new_instructions; 
        for (auto I : bb->Instruction_list) {
            if (erase_set.find(I) == erase_set.end()) {
                new_instructions.push_back(I);
            }
        }
        
        bb->Instruction_list = new_instructions;
    }
}

void BasicBlockCSEOptimizer::applyRegisterReplacements() {
    // 规范化寄存器替换映射
    // std::cerr<<"reg_replace_map:\n";
    // for (auto& [o_id, n_id] : reg_replace_map) {
    //     std::cerr<<"o_id="<<o_id<<" ,n_id="<<n_id<<"\n";
    //     while (reg_replace_map.find(n_id) != reg_replace_map.end()) {
    //         n_id = reg_replace_map[n_id];
    //     }
    // }
    // std::cerr<<"======\n";
    // 应用寄存器替换
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            I->ChangeResult(reg_replace_map);
            I->ChangeReg(reg_replace_map,reg_replace_map);
        }
    }
}

// 基本块内CSE优化入口
void SimpleCSEPass::SimpleBlockCSE(CFG* cfg,LLVMIR *IR) {
    BasicBlockCSEOptimizer optimizer(cfg,IR,alias_analyser,domtrees,memdep_analyser);
    optimizer.optimize();
}
//DFS版本的BlockDef出错，暂沿用BFS版本
// 检查基本块是否满足"无定义使用"条件
static bool BlockDefNoUseCheck(CFG *C, int bb_id, int st_id) {
    //std::cout<<"begin BlockDefNoUseCheck\n";
    
    // 安全地获取基本块
    if (C->block_map->find(bb_id) == C->block_map->end() || 
        C->block_map->find(st_id) == C->block_map->end()) {
        return false;
    }
    
    auto bb = (*C->block_map)[bb_id];
    std::set<Operand> defs;  // 存储bb中定义的操作数
    
    // 收集bb中的定义
    for (auto I : bb->Instruction_list) {
        if (I->GetOpcode() == BasicInstruction::PHI) { // PHI指令使优化复杂化
            return false;
        } else if (I->GetResult() != nullptr) {
            defs.insert(I->GetResult()); // 记录定义
        }
    }
    
    // 检查是否存在存储或调用（可能有副作用）
    for (auto I : bb->Instruction_list) {
        if (I->GetOpcode() == BasicInstruction::STORE || I->GetOpcode() == BasicInstruction::CALL) {
            return false;
        }
    }

    // 检查目标块是否包含PHI指令
    auto bb2 = (*C->block_map)[st_id];
    for (auto I : bb2->Instruction_list) {
        if (I->GetOpcode() == BasicInstruction::PHI) {
            return false;
        }
    }

    // BFS遍历从st_id开始的所有路径（跳过bb_id）
    std::vector<int> vis;
    std::queue<int> q;
    vis.resize(C->max_label + 1);
    q.push(st_id);

    while (!q.empty()) {
        auto x = q.front();
        q.pop();
        if (vis[x]) continue;
        vis[x] = true;
        if (x == bb_id) continue;  // 跳过定义块本身
        
        // 安全地获取基本块
        if (C->block_map->find(x) == C->block_map->end()) {
            continue;
        }
        
        auto bbx = (*C->block_map)[x];
        // 检查操作数是否使用bb中的定义
        for (auto I : bbx->Instruction_list) {
            for (auto op : I->GetNonResultOperands()) {
                if (defs.find(op) != defs.end()) {
                    return false;  // 发现使用，不满足条件
                }
            }
        }

        // 后继入队
        for (auto bb : C->GetSuccessor(x)) {
            q.push(bb->block_id);
        }
    }
    //std::cout<<"end BlockDefNoUseCheck\n";
    return true;  // 满足"无定义使用"条件
}
// static bool EmptyBlockJumping(CFG *C) {
//     bool optimized = false;
//     auto dom_tree = domtrees->GetDomTree(C);
    
//     // 使用工作列表处理候选块
//     std::vector<std::pair<LLVMBlock, LLVMBlock>> candidates;
//     for (auto& [id, bb] : *C->block_map) {
//         if (bb->Instruction_list.size() < 2) continue;
        
//         auto cmp_instr = bb->Instruction_list.end() - 2;
//         for (auto succ : C->GetSuccessor(id)) {
//             if (succ->Instruction_list.size() >= 2 && 
//                 dom_tree->dominates(bb, succ) && 
//                 bb != succ) {
//                 candidates.emplace_back(bb, succ);
//             }
//         }
//     }

//     for (auto& [bb, bb2] : candidates) {
//         auto cmp1 = *(bb->Instruction_list.end() - 2);
//         auto cmp2 = *(bb2->Instruction_list.end() - 2);
        
//         // 比较指令匹配检查
//         if (cmp1->GetOpcode() != cmp2->GetOpcode()) continue;
        
//         Operand op1_1, op1_2, op2_1, op2_2;
//         int cond1 = 0, cond2 = 0;
        
//         if (auto icmp = dynamic_cast<IcmpInstruction*>(cmp1)) {
//             op1_1 = icmp->GetOp1(); op1_2 = icmp->GetOp2(); cond1 = icmp->GetCompareCondition();
//             auto icmp2 = dynamic_cast<IcmpInstruction*>(cmp2);
//             if (!icmp2) continue;
//             op2_1 = icmp2->GetOp1(); op2_2 = icmp2->GetOp2(); cond2 = icmp2->GetCompareCondition();
//         } 
//         else if (auto fcmp = dynamic_cast<FcmpInstruction*>(cmp1)) {
//             op1_1 = fcmp->GetOp1(); op1_2 = fcmp->GetOp2(); cond1 = fcmp->GetCompareCondition();
//             auto fcmp2 = dynamic_cast<FcmpInstruction*>(cmp2);
//             if (!fcmp2) continue;
//             op2_1 = fcmp2->GetOp1(); op2_2 = fcmp2->GetOp2(); cond2 = fcmp2->GetCompareCondition();
//         }
//         else continue;
        
//         if (op1_1->GetFullName() != op2_1->GetFullName() ||
//             op1_2->GetFullName() != op2_2->GetFullName() ||
//             cond1 != cond2) continue;
//         if(bb->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
//         if(bb2->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
//         auto br1 = static_cast<BrCondInstruction*>(bb->Instruction_list.back());
//         auto br2 = static_cast<BrCondInstruction*>(bb2->Instruction_list.back());
        
//         int target_id = (static_cast<LabelOperand*>(br1->GetTrueLabel())->GetLabelNo() == bb2->block_id)
//             ? static_cast<LabelOperand*>(br2->GetTrueLabel())->GetLabelNo()
//             : static_cast<LabelOperand*>(br2->GetFalseLabel())->GetLabelNo();
        
//         if (!BlockDefNoUseCheck(C, bb2->block_id, target_id)) continue;
        
//         // 优化处理
//         if (static_cast<LabelOperand*>(br1->GetTrueLabel())->GetLabelNo() == bb2->block_id) {
//             br1->SetTrueLabel(br2->GetTrueLabel());
//             bb2->Instruction_list.pop_back();
//             bb2->Instruction_list.pop_back();
//             auto new_cmp = new IcmpInstruction(BasicInstruction::I32, 
//                 new ImmI32Operand(1), new ImmI32Operand(0), 
//                 BasicInstruction::eq, cmp2->GetResult());
//             bb2->InsertInstruction(bb2->Instruction_list.size(), new_cmp);
//             bb2->InsertInstruction(bb2->Instruction_list.size(), br2);
//         } else {
//             br1->SetFalseLabel(br2->GetFalseLabel());
//             bb2->Instruction_list.pop_back();
//             bb2->Instruction_list.pop_back();
//             auto new_cmp = new IcmpInstruction(BasicInstruction::I32, 
//                 new ImmI32Operand(1), new ImmI32Operand(1), 
//                 BasicInstruction::eq, cmp2->GetResult());
//             bb2->InsertInstruction(bb2->Instruction_list.size(), new_cmp);
//             bb2->InsertInstruction(bb2->Instruction_list.size(), br2);
//         }
//         optimized = true;
//     }
//     return optimized;
// }

// static bool EmptyBlockJumping(CFG *C) {
//     bool optimized = false;
//     auto dom_tree = domtrees->GetDomTree(C);
    
//     // 使用工作列表处理候选块
//     std::vector<std::pair<LLVMBlock, LLVMBlock>> candidates;
//     for (auto& [id, bb] : *C->block_map) {
//         if (bb->Instruction_list.size() < 2) continue;
        
//         auto cmp_instr = bb->Instruction_list.end() - 2;
//         for (auto succ : C->GetSuccessor(id)) {
//             if (succ->Instruction_list.size() >= 2 && 
//                 dom_tree->dominates(bb, succ) && 
//                 bb != succ) {
//                 candidates.emplace_back(bb, succ);
//             }
//         }
//     }

//     for (auto& [bb, bb2] : candidates) {
//         auto cmp1 = *(bb->Instruction_list.end() - 2);
//         auto cmp2 = *(bb2->Instruction_list.end() - 2);
        
//         // 比较指令匹配检查
//         if (cmp1->GetOpcode() != cmp2->GetOpcode()) continue;
        
//         Operand op1_1, op1_2, op2_1, op2_2;
//         int cond1 = 0, cond2 = 0;
        
//         if (auto icmp = dynamic_cast<IcmpInstruction*>(cmp1)) {
//             op1_1 = icmp->GetOp1(); op1_2 = icmp->GetOp2(); cond1 = icmp->GetCompareCondition();
//             auto icmp2 = dynamic_cast<IcmpInstruction*>(cmp2);
//             if (!icmp2) continue;
//             op2_1 = icmp2->GetOp1(); op2_2 = icmp2->GetOp2(); cond2 = icmp2->GetCompareCondition();
//         } 
//         else if (auto fcmp = dynamic_cast<FcmpInstruction*>(cmp1)) {
//             op1_1 = fcmp->GetOp1(); op1_2 = fcmp->GetOp2(); cond1 = fcmp->GetCompareCondition();
//             auto fcmp2 = dynamic_cast<FcmpInstruction*>(cmp2);
//             if (!fcmp2) continue;
//             op2_1 = fcmp2->GetOp1(); op2_2 = fcmp2->GetOp2(); cond2 = fcmp2->GetCompareCondition();
//         }
//         else continue;
        
//         if (op1_1->GetFullName() != op2_1->GetFullName() ||
//             op1_2->GetFullName() != op2_2->GetFullName() ||
//             cond1 != cond2) continue;
//         if(bb->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
//         if(bb2->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
//         auto br1 = static_cast<BrCondInstruction*>(bb->Instruction_list.back());
//         auto br2 = static_cast<BrCondInstruction*>(bb2->Instruction_list.back());
        
//         int target_id = (static_cast<LabelOperand*>(br1->GetTrueLabel())->GetLabelNo() == bb2->block_id)
//             ? static_cast<LabelOperand*>(br2->GetTrueLabel())->GetLabelNo()
//             : static_cast<LabelOperand*>(br2->GetFalseLabel())->GetLabelNo();
        
//         if (!BlockDefNoUseCheck(C, bb2->block_id, target_id)) continue;
        
//         // 优化处理
//         if (static_cast<LabelOperand*>(br1->GetTrueLabel())->GetLabelNo() == bb2->block_id) {
//             br1->SetTrueLabel(br2->GetTrueLabel());
//             bb2->Instruction_list.pop_back();
//             bb2->Instruction_list.pop_back();
//             auto new_cmp = new IcmpInstruction(BasicInstruction::I32, 
//                 new ImmI32Operand(1), new ImmI32Operand(0), 
//                 BasicInstruction::eq, cmp2->GetResult());
//             bb2->InsertInstruction(bb2->Instruction_list.size(), new_cmp);
//             bb2->InsertInstruction(bb2->Instruction_list.size(), br2);
//         } else {
//             br1->SetFalseLabel(br2->GetFalseLabel());
//             bb2->Instruction_list.pop_back();
//             bb2->Instruction_list.pop_back();
//             auto new_cmp = new IcmpInstruction(BasicInstruction::I32, 
//                 new ImmI32Operand(1), new ImmI32Operand(1), 
//                 BasicInstruction::eq, cmp2->GetResult());
//             bb2->InsertInstruction(bb2->Instruction_list.size(), new_cmp);
//             bb2->InsertInstruction(bb2->Instruction_list.size(), br2);
//         }
//         optimized = true;
//     }
//     return optimized;
// }
// 优化空块的条件跳转
bool DomTreeCSEOptimizer::EmptyBlockJumping(CFG *C) {
    bool flag = false;  // 优化发生标志
    for (auto [id, bb] : *C->block_map) {
        if (bb->Instruction_list.size() < 2) continue;  // 跳过指令不足的块
        
        Operand op1_1, op1_2;
        int cond1 = 0;
        auto I = *(bb->Instruction_list.end() - 2);  // 倒数第二条指令（比较指令）
        
        // 提取整型比较信息
        if (I->GetOpcode() == BasicInstruction::ICMP) {
            auto IcmpI = (IcmpInstruction *)I;
            op1_1 = IcmpI->GetOp1();
            op1_2 = IcmpI->GetOp2();
            cond1 = IcmpI->GetCompareCondition();
        } 
        // 提取浮点比较信息
        else if (I->GetOpcode() == BasicInstruction::FCMP) {
            auto FcmpI = (FcmpInstruction *)I;
            op1_1 = FcmpI->GetOp1();
            op1_2 = FcmpI->GetOp2();
            cond1 = FcmpI->GetCompareCondition();
        } else {
            continue;  // 非比较指令跳过
        }
        // 遍历后继块
        for (auto bb2 : C->GetSuccessor(id)) {
            // 支配性检查
            if (!domtrees->GetDomTree(C)->dominates(bb, bb2) || bb2 == bb) continue;
            if (bb2->Instruction_list.size() < 2) continue;
            
            Operand op2_1, op2_2;
            int cond2 = 0;
            auto I2 = *(bb2->Instruction_list.end() - 2);
            // 指令类型匹配检查
            if (I->GetOpcode() != I2->GetOpcode()) continue;
            
            // 提取后继块的比较信息
            if (I2->GetOpcode() == BasicInstruction::ICMP) {
                auto IcmpI2 = (IcmpInstruction *)I2;
                op2_1 = IcmpI2->GetOp1();
                op2_2 = IcmpI2->GetOp2();
                cond2 = IcmpI2->GetCompareCondition();
            } else if (I2->GetOpcode() == BasicInstruction::FCMP) {
                auto FcmpI2 = (FcmpInstruction *)I2;
                op2_1 = FcmpI2->GetOp1();
                op2_2 = FcmpI2->GetOp2();
                cond2 = FcmpI2->GetCompareCondition();
            } else continue;
            
            // 操作数和条件匹配检查
            if (op1_1->GetFullName() != op2_1->GetFullName() || 
                op1_2->GetFullName() != op2_2->GetFullName() || 
                cond1 != cond2) {
                continue;
            }
            
            if(bb->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
            if(bb2->Instruction_list.back()->GetOpcode()!=BasicInstruction::BR_COND ){continue;}
            auto brI1 = (BrCondInstruction *)(bb->Instruction_list.back());
            auto brI2 = (BrCondInstruction *)(bb2->Instruction_list.back());
           
            // True分支优化
            if (((LabelOperand *)brI1->GetTrueLabel())->GetLabelNo() == bb2->block_id) {
                // 检查目标块是否可优化
                if (!BlockDefNoUseCheck(C, bb2->block_id, ((LabelOperand *)brI2->GetTrueLabel())->GetLabelNo())) {
                    continue;
                }
                // 重定向True分支
                brI1->SetTrueLabel(((LabelOperand *)brI2->GetTrueLabel()));
                // 替换后继块中的指令为常量比较
                bb2->Instruction_list.pop_back();
                bb2->Instruction_list.pop_back();
                auto NIcmpI2 = new IcmpInstruction(BasicInstruction::I32, new ImmI32Operand(1), new ImmI32Operand(0), BasicInstruction::eq, I2->GetResult());
                bb2->InsertInstruction(1, NIcmpI2);  // 插入恒假比较
                bb2->InsertInstruction(1, brI2);     // 放回分支指令
                flag = true;  // 标记优化发生
            } 
            // False分支优化
            else {
                if (!BlockDefNoUseCheck(C, bb2->block_id, ((LabelOperand *)brI2->GetFalseLabel())->GetLabelNo())) {
                    continue;
                }
                brI1->SetFalseLabel(((LabelOperand *)brI2->GetFalseLabel()));
                bb2->Instruction_list.pop_back();
                bb2->Instruction_list.pop_back();
                auto NIcmpI2 = new IcmpInstruction(BasicInstruction::I32, new ImmI32Operand(1), new ImmI32Operand(1), BasicInstruction::eq, I2->GetResult());
                bb2->InsertInstruction(1, NIcmpI2);  // 插入恒真比较
                bb2->InsertInstruction(1, brI2);
                flag = true;
            }
        }
    }
    return flag;  // 返回优化状态
}

// 使用BFS检查基本块bb1是否能到达bb2
static bool CanReach(int bb1_id, int bb2_id, CFG *C) {
    std::vector<int> vis;      // 访问标记数组
    std::queue<int> q;         // BFS队列

    vis.resize(C->max_label + 1); // 根据最大标签初始化
    q.push(bb1_id);            // 起始块入队

    while (!q.empty()) {
        auto x = q.front();
        q.pop();
        if (x == bb2_id) {     // 找到目标块
            return true;
        }
        if (vis[x]) continue;  // 已访问跳过
        vis[x] = true;         // 标记已访问

        // 遍历所有后继块
        for (auto bb : C->GetSuccessor(x)) {
            q.push(bb->block_id);
        }
    }
    return false;  // 未找到路径
}

// 检查条件分支是否可优化（用于CSE）
static bool CanJump(bool isleft, int x1_id, int x2_id, CFG *C) {
    // 前置条件：x1支配x2
    //assert(C->dominates(x1_id, x2_id));
    
    // 安全地获取基本块
    if (C->block_map->find(x1_id) == C->block_map->end() || 
        C->block_map->find(x2_id) == C->block_map->end()) {
        return false;
    }
    
    auto x1 = (*C->block_map)[x1_id];
    auto x2 = (*C->block_map)[x2_id];
    
    // 检查x1是否有指令
    if (x1->Instruction_list.empty()) {
        return false;
    }
    
    auto BrI1 = (BrCondInstruction *)(*(x1->Instruction_list.end() - 1)); // 获取分支指令
    
    // 检查分支指令的有效性
    if (!BrI1 || BrI1->GetOpcode() != BasicInstruction::BR_COND) {
        return false;
    }
    
    // 安全地获取标签操作数
    auto trueLabel = BrI1->GetTrueLabel();
    auto falseLabel = BrI1->GetFalseLabel();
    if (!trueLabel || !falseLabel || 
        trueLabel->GetOperandType() != BasicOperand::LABEL ||
        falseLabel->GetOperandType() != BasicOperand::LABEL) {
        return false;
    }
    
    int trueLabelNo = ((LabelOperand *)trueLabel)->GetLabelNo();
    int falseLabelNo = ((LabelOperand *)falseLabel)->GetLabelNo();
    
    // 安全地获取目标块
    if (C->block_map->find(trueLabelNo) == C->block_map->end() || 
        C->block_map->find(falseLabelNo) == C->block_map->end()) {
        return false;
    }
    
    auto xT = (*C->block_map)[trueLabelNo];
    auto xF = (*C->block_map)[falseLabelNo];

    if (isleft) {  // 处理True分支情况
        // 检查路径存在性
        if (!CanReach(xT->block_id, x2->block_id, C) && CanReach(xF->block_id, x2->block_id, C)) {
            // 检查x2是否有足够的指令
            if (x2->Instruction_list.size() < 2) {
                return false;
            }
            // 修改x2的指令：用常量比较替换原比较
            auto tmpI1 = *(x2->Instruction_list.end() - 1);
            auto tmpI2 = *(x2->Instruction_list.end() - 2);
            x2->Instruction_list.pop_back();
            x2->Instruction_list.pop_back();
            // 创建恒假比较 (0==1)
            auto IcmpI = new IcmpInstruction(BasicInstruction::I32, new ImmI32Operand(0), new ImmI32Operand(1), BasicInstruction::eq, tmpI2->GetResult());
            x2->InsertInstruction(1, IcmpI); // 插入新指令
            x2->InsertInstruction(1, tmpI1);  // 放回原分支指令
            return true;  // 优化成功
        }
    } else {  // 处理False分支情况
        if (CanReach(xT->block_id, x2->block_id, C) && !CanReach(xF->block_id, x2->block_id, C)) {
            // 检查x2是否有足够的指令
            if (x2->Instruction_list.size() < 2) {
                return false;
            }
            // 创建恒真比较 (1==1)
            auto tmpI1 = *(x2->Instruction_list.end() - 1);
            auto tmpI2 = *(x2->Instruction_list.end() - 2);
            x2->Instruction_list.pop_back();
            x2->Instruction_list.pop_back();
            auto IcmpI = new IcmpInstruction(BasicInstruction::I32, new ImmI32Operand(1), new ImmI32Operand(1), BasicInstruction::eq, tmpI2->GetResult());
            x2->InsertInstruction(1, IcmpI);
            x2->InsertInstruction(1, tmpI1);
            return true;
        }
    }
    return false;  // 无优化
}
// // 使用DFS检查基本块可达性
// static bool CanReach(int bb1_id, int bb2_id, CFG *C) {
//     if (bb1_id == bb2_id) return true;
//     std::vector<bool> visited(C->max_label + 1, false);
//     std::stack<int> stack;
//     stack.push(bb1_id);
//     visited[bb1_id] = true;

//     while (!stack.empty()) {
//         int current = stack.top();
//         stack.pop();
//         for (auto bb : C->GetSuccessor(current)) {
//             int next_id = bb->block_id;
//             if (next_id == bb2_id) return true;
//             if (!visited[next_id]) {
//                 visited[next_id] = true;
//                 stack.push(next_id);
//             }
//         }
//     }
//     return false;
// }

// // 封装指令修改操作
// static void ReplaceInstructions(LLVMBlock x2, bool constant_value) {
//     auto& instr_list = x2->Instruction_list;
//     auto last_instr = instr_list.back();
//     auto cmp_instr = *(instr_list.end() - 2);
//     instr_list.pop_back();
//     instr_list.pop_back();

//     auto op1 = new ImmI32Operand(constant_value ? 1 : 0);
//     auto op2 = new ImmI32Operand(1);
//     auto new_cmp = new IcmpInstruction(
//         BasicInstruction::I32, op1, op2, 
//         BasicInstruction::eq, cmp_instr->GetResult()
//     );

//     x2->InsertInstruction(instr_list.size(), new_cmp);
//     x2->InsertInstruction(instr_list.size() + 1, last_instr);
// }

// // 优化条件分支
// static bool CanJump(bool isleft, int x1_id, int x2_id, CFG *C) {
//     auto blk1 = (*C->block_map)[x1_id];
//     auto blk2 = (*C->block_map)[x2_id];
//     auto br_instr = static_cast<BrCondInstruction*>(blk1->Instruction_list.back());
    
//     int true_target = static_cast<LabelOperand*>(br_instr->GetTrueLabel())->GetLabelNo();
//     int false_target = static_cast<LabelOperand*>(br_instr->GetFalseLabel())->GetLabelNo();

//     bool true_path = CanReach(true_target, x2_id, C);
//     bool false_path = CanReach(false_target, x2_id, C);

//     if (isleft && !true_path && false_path) {
//         ReplaceInstructions(blk2, false); // false condition
//         return true;
//     }
//     if (!isleft && true_path && !false_path) {
//         ReplaceInstructions(blk2, true); // true condition
//         return true;
//     }
//     return false;
// }

// DomTreeCSEOptimizer成员函数实现
void DomTreeCSEOptimizer::optimize() {

    while (changed) {
        changed = false;
        tmp=changed;
        currentRecursionDepth=0;
		CSE_DEBUG_PRINT(std::cout<<"changed"<<std::endl);
        dfs(0);
		CSE_DEBUG_PRINT(std::cout<<"dfs"<<std::endl);
        removeDeadInstructions();
		CSE_DEBUG_PRINT(std::cout<<"removeDeadInstructions"<<std::endl);
        applyRegisterReplacements();
        changed=tmp;
    }

}
void DomTreeCSEOptimizer::branch_optimize()
{
    changed=true;
    while (changed) {
        CmpMap.clear();
        changed = false;
        currentRecursionDepth = 0;
        branch_dfs(0);
        branch_end();
    }

}
void DomTreeCSEOptimizer::branch_end()
{
    changed |= EmptyBlockJumping(C);  // 合并优化结果
    // 重建CFG和支配树
    C->BuildCFG();
    DomInfo[C] = new DominatorTree(C);
    DomInfo[C]->BuildDominatorTree(false);
    C->DomTree = DomInfo[C]; 
}
// void DomTreeCSEOptimizer::dfs(int bbid) {
//     LLVMBlock now = (*C->block_map)[bbid];
//     std::set<InstCSEInfo> regularCseSet;
//     std::map<InstCSEInfo, int> tmpLoadNumMap;
//     std::set<InstCSEInfo> cmpCseSet;
//     int ins_size=now->Instruction_list.size();
//     for(int i=0;i<ins_size-2;++i)
//     {
//         auto I=now->Instruction_list[i];
//         if (!CSENotConsider(I)) continue;

//         switch (I->GetOpcode()) {
//             case BasicInstruction::LOAD:
//                 processLoadInstruction(static_cast<LoadInstruction*>(I), tmpLoadNumMap);
//                 break;
//             case BasicInstruction::STORE:
//                 processStoreInstruction(static_cast<StoreInstruction*>(I), tmpLoadNumMap);
//                 break;
//             case BasicInstruction::CALL:
//                 processCallInstruction(static_cast<CallInstruction*>(I));
//                 break;
//             default:
//                 processRegularInstruction(I, regularCseSet);
//                 break;
//         }
//     }
//     if(ins_size>=2)
//     {
//         for(int i=ins_size-2;i<ins_size;++i)
//         {
//             auto I=now->Instruction_list[i];
//             if (!BranchCSENotConsider(I)) continue;

//             switch (I->GetOpcode()) {
//                 case BasicInstruction::LOAD:
//                     processLoadInstruction(static_cast<LoadInstruction*>(I), tmpLoadNumMap);
//                     break;
//                 case BasicInstruction::STORE:
//                     processStoreInstruction(static_cast<StoreInstruction*>(I), tmpLoadNumMap);
//                     break;
//                 case BasicInstruction::CALL:
//                     processCallInstruction(static_cast<CallInstruction*>(I));
//                     break;
//                 case BasicInstruction::ICMP:
//                     if(i==ins_size-2){processIcmpInstruction(static_cast<IcmpInstruction*>(I),cmpCseSet);}
//                     break;
//                 case BasicInstruction::FCMP:
//                     if(i==ins_size-2){processFcmpInstruction(static_cast<FcmpInstruction*>(I),cmpCseSet);}
//                     break;
//                 default:
//                     processRegularInstruction(I, regularCseSet);
//                     break;
//             }
//         }
//     }
    
    
//     for (auto v : domtrees->GetDomTree(C)->dom_tree[bbid]) {
//         dfs(v->block_id);
//     }

//     //cleanupTemporaryEntries(regularCseSet, tmpLoadNumMap);
//     cleanupTemporaryEntries(regularCseSet,cmpCseSet);
// }

void DomTreeCSEOptimizer::dfs(int bbid) {
    // 检查递归深度
    if (currentRecursionDepth > MAX_RECURSION_DEPTH) {
        std::cerr << "递归深度超过限制，可能出现死循环。退出优化过程。" << std::endl;
        return;
    }

    currentRecursionDepth++; // 进入递归，深度增加
    CSE_DEBUG_PRINT(std::cout<<"dfs bbid: "<<bbid<<std::endl);
    LLVMBlock now = (*C->block_map)[bbid];
    std::set<InstCSEInfo> tmpCSESet;
    std::map<InstCSEInfo, int> tmpLoadNumMap;
    for (auto I : now->Instruction_list) {
        if (!CSENotConsider(I)) continue;

        switch (I->GetOpcode()) {
            case BasicInstruction::LOAD:
                CSE_DEBUG_PRINT(std::cout<<"load"<<std::endl);
                processLoadInstruction(static_cast<LoadInstruction*>(I), tmpLoadNumMap);
                CSE_DEBUG_PRINT(std::cout<<"load end"<<std::endl);
                break;
            case BasicInstruction::STORE:
                CSE_DEBUG_PRINT(std::cout<<"store"<<std::endl);
                processStoreInstruction(static_cast<StoreInstruction*>(I), tmpLoadNumMap);
                CSE_DEBUG_PRINT(std::cout<<"store end"<<std::endl);
                break;
            case BasicInstruction::CALL:
                CSE_DEBUG_PRINT(std::cout<<"call"<<std::endl);
                processCallInstruction(static_cast<CallInstruction*>(I),tmpCSESet);
                CSE_DEBUG_PRINT(std::cout<<"call end"<<std::endl);
                break;
            default:
                processRegularInstruction(I, tmpCSESet);
                break;
        }
    }

    CSE_DEBUG_PRINT(std::cout<<"dfs bbid: "<<bbid<<" end"<<std::endl);

    for (auto v : C->getDomTree()->dom_tree[bbid]) {
        dfs(v->block_id);
    }

    cleanupTemporaryEntries(tmpCSESet, tmpLoadNumMap);
    currentRecursionDepth--; // 退出递归，深度减少
}

void DomTreeCSEOptimizer::branch_dfs(int bbid)
{
    // 检查递归深度
    if (currentRecursionDepth > MAX_RECURSION_DEPTH) {
        std::cerr << "递归深度超过限制，可能出现死循环。退出优化过程。" << std::endl;
        return;
    }

    currentRecursionDepth++; // 进入递归，深度增加
    // 安全地获取基本块
    if (C->block_map->find(bbid) == C->block_map->end()) {
        return;
    }
    
    LLVMBlock now = (*C->block_map)[bbid];
    std::set<InstCSEInfo> cmpCseSet;  // 临时存储当前块的CSE信息
    
    // 1）检查当前块尾部是否存在比较指令
    if (now->Instruction_list.size() >= 2) {
        auto I = *(now->Instruction_list.end() - 2);
        //2)如果存在比较指令，提取CSE信息
        if (I->GetOpcode() == BasicInstruction::FCMP || I->GetOpcode() == BasicInstruction::ICMP) {
            auto info = GetCSEInfo(I);  // 提取CSE信息
            bool isConstCmp = false;
            
            //3）检查是否为整型常量比较（？不可消除）
            if (I->GetOpcode() == BasicInstruction::ICMP) {
                auto IcmpI = (IcmpInstruction *)I;
                if (IcmpI->GetOp1()->GetOperandType() == BasicOperand::IMMI32 &&
                    IcmpI->GetOp2()->GetOperandType() == BasicOperand::IMMI32) {
                    isConstCmp = true;
                }
            }
            
            bool isCSE = false;
            //4）查找可用的公共表达式（非常量cmp且已经在cmpmap中记录）
            if (!isConstCmp && CmpMap.find(info) != CmpMap.end()) {
                for (auto I2 : CmpMap[info]) {
                    // 检查支配关系下的跳转可能性
                    if (CanJump(1, I2->GetBlockID(), I->GetBlockID(), C) || 
                        CanJump(0, I2->GetBlockID(), I->GetBlockID(), C)) {
                        isCSE = true;
                        break;
                    }
                }
            }
            // 5）非常量比较且无CSE时加入映射
            if (!isCSE && !isConstCmp) {
                cmpCseSet.insert(info);
                CmpMap[info].push_back(I);
            }
        }
    }
    

    // 6）递归处理支配树中的子节点
    for (auto v : domtrees->GetDomTree(C)->dom_tree[bbid]) {
        dfs(v->block_id);
    }

    //7） 回溯：移除当前块添加的CSE信息
    for (auto info : cmpCseSet) {
        CmpMap[info].pop_back();
    }
     currentRecursionDepth--; // 退出递归，深度减少
}


void DomTreeCSEOptimizer::processIcmpInstruction(IcmpInstruction* IcmpI,std::set<InstCSEInfo>& cmpCseSet)
{
    auto info = GetCSEInfo(IcmpI);  // 提取CSE信息
    bool isConstCmp = false;
    //检查是否为整型常量比较（？不可消除）(是否需要扩展成浮点数常量比较？)
    if (IcmpI->GetOp1()->GetOperandType() == BasicOperand::IMMI32 &&
                IcmpI->GetOp2()->GetOperandType() == BasicOperand::IMMI32) {
        isConstCmp = true;
    }  
    bool isCSE = false;
    //4）查找可用的公共表达式（非常量cmp且已经在cmpmap中记录）
    if (!isConstCmp && CmpMap.find(info) != CmpMap.end()) {
        for (auto I2 : CmpMap[info]) {
            // 检查支配关系下的跳转可能性
            if (CanJump(1, I2->GetBlockID(), IcmpI->GetBlockID(), C) || 
                CanJump(0, I2->GetBlockID(), IcmpI->GetBlockID(), C)) {
                isCSE = true;
                break;
            }
        }
    }
    // 5）非常量比较且无CSE时加入映射
    if (!isCSE && !isConstCmp) {
        cmpCseSet.insert(info);
        CmpMap[info].push_back(IcmpI);
    }
}
void DomTreeCSEOptimizer::processFcmpInstruction(FcmpInstruction* FcmpI,std::set<InstCSEInfo>& cmpCseSet)
{
    auto info = GetCSEInfo(FcmpI);  // 提取CSE信息
    bool isConstCmp = false;
    //检查是否为整型常量比较（？不可消除）(是否需要扩展成浮点数常量比较？)
    if (FcmpI->GetOp1()->GetOperandType() == BasicOperand::IMMF32 &&
                FcmpI->GetOp2()->GetOperandType() == BasicOperand::IMMF32) {
        isConstCmp = true;
    }  
    bool isCSE = false;
    //4）查找可用的公共表达式（非常量cmp且已经在cmpmap中记录）
    if (!isConstCmp && CmpMap.find(info) != CmpMap.end()) {
        for (auto I2 : CmpMap[info]) {
            // 检查支配关系下的跳转可能性
            if (CanJump(1, I2->GetBlockID(), FcmpI->GetBlockID(), C) || 
                CanJump(0, I2->GetBlockID(), FcmpI->GetBlockID(), C)) {
                isCSE = true;
                break;
            }
        }
    }
    // 5）非常量比较且无CSE时加入映射
    if (!isCSE && !isConstCmp) {
        cmpCseSet.insert(info);
        CmpMap[info].push_back(FcmpI);
    }

}
void DomTreeCSEOptimizer::processLoadInstruction(LoadInstruction* LoadI, std::map<InstCSEInfo, int>& tmpLoadNumMap) {
    //1.查找是否有等价的load指令
    auto info = GetCSEInfo(LoadI);
    auto CSEiter = LoadCSEMap.find(info);
    bool is_cse = false;
    //2.如果存在等价的load指令
    if (CSEiter != LoadCSEMap.end()) {
        //3.遍历映射中存储的等价load指令，记为I2
        for (auto I2 : LoadCSEMap[info]) {
            if(memdep_analyser->isLoadSameMemory(LoadI, I2, C) == false){continue;}
			//LoadI->PrintIR(//std::cout);I2->PrintIR(//std::cout);
			//std::cout<<"-----------------------------\n";
            //4.如果I2与该条load2加载同一块内存
            //1)该条load指令插入删除集合
            eraseSet.insert(LoadI);
            // I->PrintIR(std::cerr);
            // I2->PrintIR(std::cerr);
            // std::cerr<<"-----------------------------\n";
            //2)如果I2是store指令，更改替代的reg;load指令同理
            if (I2->GetOpcode() == BasicInstruction::STORE) {
                // I->PrintIR(std::cerr);
                auto StoreI2 = (StoreInstruction *)I2;
                int val_regno = ((RegOperand *)StoreI2->GetValue())->GetRegNo();
                if (regReplaceMap.find(val_regno) != regReplaceMap.end()) {
                    val_regno = regReplaceMap[val_regno];
                }
                regReplaceMap[GetResultRegNo(LoadI)] = val_regno;

            } else if (I2->GetOpcode() == BasicInstruction::LOAD) {

                regReplaceMap[GetResultRegNo(LoadI)] = GetResultRegNo(I2);
            } else {    // should not reach here
                assert(false);
            }

            tmp|= true;//很可能需要改成flag
            is_cse = true;
            break;
        }
    }
    if(!is_cse)
    {
        LoadCSEMap[info].push_back(LoadI);
        if(tmpLoadNumMap.find(info)==tmpLoadNumMap.end())
        {
            tmpLoadNumMap[info]=0;
        }
        tmpLoadNumMap[info] += 1;
    }
}

void DomTreeCSEOptimizer::processStoreInstruction(StoreInstruction* StoreI, std::map<InstCSEInfo, int>& tmpLoadNumMap) {
    //1.确保此时只剩下store reg的指令（通过block_cse的初始化，消除立即数版本的store)
    assert(StoreI->GetValue()->GetOperandType() == BasicOperand::REG);
    //2.如果当前寄存器已经在替换列表中，直接替换即可
    int val_regno = ((RegOperand *)StoreI->GetValue())->GetRegNo();
    if (regReplaceMap.find(val_regno) != regReplaceMap.end()) {
        val_regno = regReplaceMap[val_regno];
    }
    //3.创建store对应的load指令,用它提取info,存储store指令（？为什么得用load的info)
    auto LoadI =
    new LoadInstruction(StoreI->GetDataType(), StoreI->GetPointer(), GetNewRegOperand(val_regno));
    auto info = GetCSEInfo(LoadI);
    LoadCSEMap[info].push_back(StoreI);
    if(tmpLoadNumMap.find(info)==tmpLoadNumMap.end())
    {
        tmpLoadNumMap[info]=0;
    }
    tmpLoadNumMap[info] += 1;
    return;
}

void DomTreeCSEOptimizer::processCallInstruction(CallInstruction* CallI, std::set<InstCSEInfo>& regularCseSet) {
    //1.如果是库函数调用，无法处理，直接返回
    if (cfgTable.find(CallI->GetFunctionName()) == cfgTable.end()) {
        return;    // external call
    }
    auto cfg = cfgTable[CallI->GetFunctionName()];
    if(cfg==nullptr){return;}
    auto& rwmap = alias_analyser->GetRWMap();
    auto it = rwmap.find(cfg);
    if (it == rwmap.end()) {
            return;
    }
    const RWInfo& rwinfo = it->second;
    //2.如果存在读写内存的操作，无法处理，直接返回
    // we only CSE independent call in this Pass
	if(alias_analyser->HasSideEffect(cfg)|| rwinfo.ReadRoots.size() !=0){return;}
    // if( (rwinfo.has_lib_func_call) || rwinfo.ReadRoots.size() !=0 || rwinfo.WriteRoots.size() != 0) {
    //     return;
    // }
    auto info = GetCSEInfo(CallI);
    auto cseIter = instCSEMap.find(info);
    //3.如果存在同等的call指令，加入删除队列中，并替换寄存器的值；如果不存在，存入信息中
    if (cseIter != instCSEMap.end()) {
        eraseSet.insert(CallI);
        regReplaceMap[GetResultRegNo(CallI)] = cseIter->second;
        tmp|= true;
    } else {
        instCSEMap[info] = GetResultRegNo(CallI);
        regularCseSet.insert(info);
    }
}

void DomTreeCSEOptimizer::processRegularInstruction(BasicInstruction* I, std::set<InstCSEInfo>& regularCseSet) {
    auto info = GetCSEInfo(I);
    auto cseIter = instCSEMap.find(info);

    if (cseIter != instCSEMap.end()) {
        eraseSet.insert(I);
        regReplaceMap[GetResultRegNo(I)] = cseIter->second;
        tmp|= true;
    } else {
        instCSEMap[info] = GetResultRegNo(I);
        regularCseSet.insert(info);
    }
}

void DomTreeCSEOptimizer::cleanupTemporaryEntries(const std::set<InstCSEInfo>& regularCseSet, 
                                                const std::map<InstCSEInfo, int>& tmpLoadNumMap) {
    for (auto info : regularCseSet) {
        instCSEMap.erase(info);
    }

    // for (auto [info, num] : tmpLoadNumMap) {
    //     auto& loadList = LoadCSEMap[info];
    //     loadList.erase(loadList.end() - num, loadList.end());
    //     if (loadList.empty()) {
    //         LoadCSEMap.erase(info);
    //     }
    // }
    for (auto [info, num] : tmpLoadNumMap) {
        for (int i = 0; i < num; ++i) {
            LoadCSEMap[info].pop_back();
        }
    }
}
// void DomTreeCSEOptimizer::cleanupTemporaryEntries(const std::set<InstCSEInfo>& regularCseSet,const std::set<InstCSEInfo>& cmpCseSet) {
//     for (auto info : regularCseSet) {
//         instCSEMap.erase(info);
//     }
//     for (auto info : cmpCseSet) {
//         CmpMap[info].pop_back();
//     }
// }
// void DomTreeCSEOptimizer::cleanupTemporaryEntries(const std::set<InstCSEInfo>& regularCseSet) {
//     for (auto info : regularCseSet) {
//         instCSEMap.erase(info);
//     }
// }
void DomTreeCSEOptimizer::removeDeadInstructions() {
    for (auto [id, bb] : *C->block_map) {
        std::deque<Instruction> newInstructions;
        for (auto I : bb->Instruction_list) {
            if (!eraseSet.count(I)) {
                newInstructions.push_back(I);
            }
        }
        bb->Instruction_list = newInstructions;
    }
    eraseSet.clear();
}

void DomTreeCSEOptimizer::applyRegisterReplacements() {
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            I->ChangeResult(regReplaceMap);
            I->ChangeReg(regReplaceMap,regReplaceMap);
        }
    }
    regReplaceMap.clear();
}

// 支配树CSE优化入口
void SimpleCSEPass::SimpleDomTreeWalkCSE(CFG* C) {
    DomTreeCSEOptimizer optimizer(C,alias_analyser,memdep_analyser,domtrees);
    optimizer.optimize();
}

// 整体CSE优化入口
void SimpleCSEPass::Execute() {
    memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);
    cfgTable.clear();
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    
    // 构建函数调用映射，检测多指针参数函数
    buildFuncCallMap();
    
		CSE_DEBUG_PRINT(std::cout<<"SimpleCSEPass::Execute"<<std::endl);
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		CSE_DEBUG_PRINT(std::cout<<"defI: "<<defI->GetFunctionName()<<std::endl);
        CSEInit(cfg);
		CSE_DEBUG_PRINT(std::cout<<"CSEInit"<<std::endl);
        BasicBlockCSEOptimizer optimizer(cfg,llvmIR,alias_analyser,domtrees,memdep_analyser);
        // 根据函数名设置是否进行内存优化
        std::string funcName = defI->GetFunctionName();
        //std::cout<<"funcName: "<<funcName<<std::endl;
        //std::cout<<"canMemOp(funcName): "<<canMemOp(funcName)<<std::endl;
        optimizer.setMemOp(canMemOp(funcName));
        optimizer.optimize();
        CSE_DEBUG_PRINT(std::cout<<"optimize"<<std::endl);
        DomTreeCSEOptimizer optimizer2(cfg,alias_analyser,memdep_analyser,domtrees);
        optimizer2.setMemOp(canMemOp(funcName));
        optimizer2.optimize();
		CSE_DEBUG_PRINT(std::cout<<"optimize2"<<std::endl);
    }
	CSE_DEBUG_PRINT(std::cout<<"SimpleCSEPass::Execute2"<<std::endl);
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //CSEInit(cfg);
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(id);
            }
        }
        DomTreeCSEOptimizer optimizer2(cfg,alias_analyser,memdep_analyser,domtrees);
        std::string funcName = defI->GetFunctionName();
        optimizer2.setMemOp(canMemOp(funcName));
        optimizer2.branch_optimize();
    }
	CSE_DEBUG_PRINT(std::cout<<"SimpleCSEPass::Execute3"<<std::endl);
}
void SimpleCSEPass::BNExecute() {
    memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);
    cfgTable.clear();
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    
    // 构建函数调用映射，检测多指针参数函数
    buildFuncCallMap();
    
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        CSEInit(cfg);
        BasicBlockCSEOptimizer optimizer(cfg,llvmIR,alias_analyser,domtrees,memdep_analyser);
        optimizer.NoMemoptimize();
    }
}
void SimpleCSEPass::BlockExecute() {
    memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);
    cfgTable.clear();
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    
    // 构建函数调用映射，检测多指针参数函数
    buildFuncCallMap();
    
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        CSEInit(cfg);
        BasicBlockCSEOptimizer optimizer(cfg,llvmIR,alias_analyser,domtrees,memdep_analyser);
        // 根据函数名设置是否进行内存优化
        std::string funcName = defI->GetFunctionName();

        optimizer.setMemOp(canMemOp(funcName));
        optimizer.optimize();
    }
}
void SimpleCSEPass::DomtreeExecute() {
    memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);
    cfgTable.clear();
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(id);
            }
        }
        //CSEInit(cfg);
        DomTreeCSEOptimizer optimizer2(cfg,alias_analyser,memdep_analyser,domtrees);
        std::string funcName = defI->GetFunctionName();
        optimizer2.setMemOp(canMemOp(funcName));
        optimizer2.optimize();
    }
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(id);
            }
        }
        //CSEInit(cfg);
        DomTreeCSEOptimizer optimizer2(cfg,alias_analyser,memdep_analyser,domtrees);
        optimizer2.branch_optimize();
    }
}

// 新增方法实现
void SimpleCSEPass::buildFuncCallMap() {
    funcCallMap.clear();
    canMemOpMap.clear();
    
    // 初始化所有函数的调用列表和内存优化标志
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        std::string funcName = defI->GetFunctionName();
        funcCallMap[funcName] = std::vector<CallInstruction*>();
        canMemOpMap[funcName] = true; // 默认可以进行内存优化
    }
    
    // 一次遍历所有CFG，收集所有call指令并检查指针参数别名
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                if (I->GetOpcode() == BasicInstruction::CALL) {
                    auto callInst = static_cast<CallInstruction*>(I);
                    std::string calledFuncName = callInst->GetFunctionName();
                    
                    // 如果被调用的函数在我们的CFG中，添加到其调用列表中
                    if (funcCallMap.find(calledFuncName) != funcCallMap.end()) {
                        funcCallMap[calledFuncName].push_back(callInst);
                        
                        // 在这里直接检查指针参数别名，避免后续重复遍历
                        if (canMemOpMap[calledFuncName]) { // 只有还可以优化的函数才需要检查
                            auto paramList = callInst->GetParameterList();
                            std::vector<Operand> ptrArgs;
                            
                            // 收集所有指针参数
                            for (auto [type, operand] : paramList) {
                                if (type == BasicInstruction::PTR) {
                                    ptrArgs.push_back(operand);
                                }
                            }
                            
                            // 检查是否有相同的指针参数
                            bool hasSamePtrArgs = false;
                            for (size_t i = 0; i < ptrArgs.size() && !hasSamePtrArgs; i++) {
                                for (size_t j = i + 1; j < ptrArgs.size(); j++) {
                                    // 使用当前CFG（调用者的CFG）进行别名分析
                                    auto aliasResult = alias_analyser->QueryAlias(ptrArgs[i], ptrArgs[j], cfg);
                                    if (aliasResult == MustAlias) {
                                        hasSamePtrArgs = true;
                                        break;
                                    }
                                }
                            }
                            
                            // 如果发现相同指针参数，禁用该函数的内存优化
                            if (hasSamePtrArgs) {
                                canMemOpMap[calledFuncName] = false;
                            }
                        }
                    }
                }
            }
        }
    }

	// for (auto [funcName, calls] : funcCallMap) {
	// 	//std::cout<<"funcName: "<<funcName<<std::endl;
	// 	for (auto callInst : calls) {
	// 		//std::cout<<"callInst: "; callInst->PrintIR(//std::cout);
	// 	}
	// }
}

bool SimpleCSEPass::canMemOp(const std::string& funcName) {
    auto it = canMemOpMap.find(funcName);
    if (it != canMemOpMap.end()) {
        return it->second;
    }
    // 如果函数不在映射中，默认可以进行内存优化
    return true;
}


