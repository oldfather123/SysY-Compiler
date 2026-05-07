#include "mem2reg.h"
#include <tuple>

std::map<int, int> store_record_map; // 记录store的regno实际来自于哪一个reg，用于load/store的清除 <pointer_regno, value_regno>
std::map<int, int> load_record_map; // 记录load的regno实际来自于哪一个reg，用于load/store的清除 <value_regno, pointer_regno>

std::set<int> visited_blocks; // 记录在cfg深度优先搜索时已经访问过的块id
std::map<int, std::map<int, int>> store_bb_map; // 重命名专用变量，记录每个block的store的regno实际来自哪个reg
std::map<int, std::map<int, int>> load_bb_map; // 重命名专用变量，记录每个block的load的regno实际来自哪个reg

// 检查该条alloca指令是否可以被mem2reg
// eg. 数组不可以mem2reg
// eg. 如果该指针直接被使用不可以mem2reg(在SysY一般不可能发生,SysY不支持指针语法)
bool Mem2RegPass::IsPromotable(Instruction AllocaInst) {
    return (((AllocaInstruction*)AllocaInst)->GetDims().empty());
}
/*
    int a1 = 5,a2 = 3,a3 = 11,b = 4
    return b // a1,a2,a3 is useless
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;a1
    %r1 = alloca i32 ;a2
    %r2 = alloca i32 ;a3
    %r3 = alloca i32 ;b
    store 5 -> %r0 ;a1 = 5
    store 3 -> %r1 ;a2 = 3
    store 11 -> %r2 ;a3 = 11
    store 4 -> %r3 ;b = 4
    %r4 = load i32 %r3
    ret i32 %r4
--------------------------------------------------
%r0,%r1,%r2只有store, 但没有load,所以可以删去
优化后的IR(pseudo)为:
    %r3 = alloca i32
    store 4 -> %r3
    %r4 - load i32 %r3
    ret i32 %r4
*/

// vset is the set of alloca regno that load at least once
void Mem2RegPass::Mem2RegNoUseAlloca(CFG *C, std::unordered_set<int> &vset) {
    for(auto &[label, block]: *(C->block_map)){
        std::deque<Instruction> old_Intrs = block->Instruction_list;
        block->Instruction_list.clear();
        for(auto &intr: old_Intrs){
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::ALLOCA){
                if(IsPromotable((AllocaInstruction*)intr)){
                    Operand operand = ((AllocaInstruction*)intr)->GetResult();
                    int regno = ((RegOperand*)operand)->GetRegNo();
                    if(vset.find(regno)==vset.end())continue; // 如果从来没有load过，抛弃alloca
                }
            }
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
                Operand operand = ((StoreInstruction*)intr)->GetPointer();
                // 检查寄存器是否用于store全局变量（因为全局变量不需要load）
                if(operand->GetOperandType()!=BasicOperand::GLOBAL){
                    if(operand->GetOperandType()==BasicOperand::REG){
                        int regno = ((RegOperand*)operand)->GetRegNo();
                        if(vset.find(regno)==vset.end())
                            if(array_regno.find(regno)==array_regno.end())
                                continue; // 如果从来没有load过，并且不是数组，抛弃store
                    }
                }
            }
            block->Instruction_list.push_back(intr);
        }
    }
}

/*
    int b = getint();
    b = b + 10
    return b // def and use of b are in same block
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;b
    %r1 = call getint()
    store %r1 -> %r0
    %r2 = load i32 %r0
    %r3 = %r2 + 10
    store %r3 -> %r0
    %r4 = load i32 %r0
    ret i32 %r4
--------------------------------------------------
%r0的所有load和store都在同一个基本块内
优化后的IR(pseudo)为:
    %r1 = call getint()
    %r3 = %r1 + 10
    ret %r3

对于每一个load，我们只需要找到最近的store,然后用store的值替换之后load的结果即可
*/

// vset is the set of alloca regno that load and store are all in the BB block_id
void Mem2RegPass::Mem2RegUseDefInSameBlock(CFG *C, std::unordered_set<int> &vset, int block_id) {
    // 临时用来删除不要的alloca，后面再优化
    LLVMBlock zero_block = (*(C->block_map))[0];
    std::deque<Instruction> zero_block_Intrs = zero_block->Instruction_list;
    zero_block->Instruction_list.clear();
    for(auto &intr: zero_block_Intrs){
        if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::ALLOCA){
            Operand operand = ((AllocaInstruction*)intr)->GetResult();
            int regno = ((RegOperand*)operand)->GetRegNo();
            if(vset.find(regno)!=vset.end()){
                continue;
            }
        }
        zero_block->Instruction_list.push_back(intr);
    }

    LLVMBlock block = (*(C->block_map))[block_id];
    std::deque<Instruction> old_Intrs = block->Instruction_list;
    block->Instruction_list.clear();
    for(auto &intr: old_Intrs){
        if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
            Operand p_operand = ((StoreInstruction*)intr)->GetPointer();
            Operand v_operand = ((StoreInstruction*)intr)->GetValue();
            if(p_operand->GetOperandType()==BasicOperand::REG){
                int p_regno = ((RegOperand*)p_operand)->GetRegNo();
                // 假如不是目标寄存器，就保存指令并退出
                if(vset.find(p_regno)==vset.end()){
                    intr->ChangeReg(store_record_map, load_record_map);
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                // 例外是数组指针，它不应该在目标内，store必须保留
                if(array_regno.find(p_regno)!=array_regno.end()){
                    intr->ChangeReg(store_record_map, load_record_map);
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                intr->ChangeReg(store_record_map, load_record_map);
                p_operand = ((StoreInstruction*)intr)->GetPointer();
                v_operand = ((StoreInstruction*)intr)->GetValue();
                int v_regno = ((RegOperand*)v_operand)->GetRegNo();
                p_regno = ((RegOperand*)p_operand)->GetRegNo();
                store_record_map[p_regno] = v_regno; // 更新def map
            }
            else if(p_operand->GetOperandType()==BasicOperand::GLOBAL){
                intr->ChangeReg(store_record_map, load_record_map);
                block->Instruction_list.push_back(intr);
                continue;
            }
        }
        else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
            Operand p_operand = ((LoadInstruction*)intr)->GetPointer();
            Operand r_operand = ((LoadInstruction*)intr)->GetResult();
            if(p_operand->GetOperandType()==BasicOperand::REG){
                int p_regno = ((RegOperand*)p_operand)->GetRegNo();
                // 假如不是目标寄存器，就把指令压入栈并退出
                if(vset.find(p_regno)==vset.end()){
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                // 例外是数组指针，它不应该在目标内，load必须保留
                if(array_regno.find(p_regno)!=array_regno.end()){
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                intr->ChangeReg(store_record_map, load_record_map);
                p_operand = ((LoadInstruction*)intr)->GetPointer();
                int r_regno = ((RegOperand*)r_operand)->GetRegNo();
                p_regno = ((RegOperand*)p_operand)->GetRegNo();
                load_record_map[r_regno] = p_regno; // 更新use map
            }
            else if(p_operand->GetOperandType()==BasicOperand::GLOBAL){
                block->Instruction_list.push_back(intr);
                continue;
            }
        }
        else{
            intr->ChangeReg(store_record_map, load_record_map);
            block->Instruction_list.push_back(intr);
        }
    }
}

// vset is the set of alloca regno that one store dominators all load instructions
// 保证时间复杂度小于等于O(nlognlogn)
// 只要仅在一个块内store就行
void Mem2RegPass::Mem2RegOneDefDomAllUses(CFG *C, std::unordered_set<int> &vset, int block_id) {
    Mem2RegUseDefInSameBlock(C,vset,block_id);
    for(auto &[label, block]: *(C->block_map)){
        std::deque<Instruction> old_Intrs = block->Instruction_list;
        block->Instruction_list.clear();
        for(auto &intr: old_Intrs){
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
                Operand p_operand = ((LoadInstruction*)intr)->GetPointer();
                Operand r_operand = ((LoadInstruction*)intr)->GetResult();
                if(p_operand->GetOperandType()==BasicOperand::REG){
                    int p_regno = ((RegOperand*)p_operand)->GetRegNo();
                    // 假如不是目标寄存器，就保留指令并退出
                    if(vset.find(p_regno)==vset.end()){
                        block->Instruction_list.push_back(intr);
                        continue;
                    }
                    // 例外是数组指针，它不应该在目标内，load必须保留
                    if(array_regno.find(p_regno)!=array_regno.end()){
                        block->Instruction_list.push_back(intr);
                        continue;
                    }
                    //剩余的情况：one def dom all uses, 删除
                    intr->ChangeReg(store_record_map, load_record_map);
                    Operand p_operand = ((LoadInstruction*)intr)->GetPointer();
                    Operand r_operand = ((LoadInstruction*)intr)->GetResult();
                    p_regno = ((RegOperand*)p_operand)->GetRegNo();
                    int r_regno = ((RegOperand*)r_operand)->GetRegNo();
                    load_record_map[r_regno] = p_regno; // 更新use map
                }
                else if(p_operand->GetOperandType()==BasicOperand::GLOBAL){
                    block->Instruction_list.push_back(intr);
                }
            }
            else{
                intr->ChangeReg(store_record_map, load_record_map);
                block->Instruction_list.push_back(intr);
            }
        }
    }
}

// 不参与phi运算的简单优化情况
// 关于数组的处理的解释，以这个指令为例: %r5 = getelementptr [1005 x i32], ptr @que, i32 0, i32 %r4
// %r5不可以被替换，它作为ptr的load和store也都不可以被删除
// %r4可以被替换
void Mem2RegPass::BasicMem2Reg(CFG *C){
    // 遍历所有指令，初始化loaded_regno，use_map，def_map， phi_def_map
    for(auto &[label, block]: *(C->block_map)){
        for(auto &intr: block->Instruction_list){
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
                Operand operand = ((LoadInstruction*)intr)->GetPointer();
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno = ((RegOperand*)operand)->GetRegNo();
                    loaded_regno.insert(regno);
                    use_map[regno].insert(block->block_id);
                }
            }
            else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
                Operand operand = ((StoreInstruction*)intr)->GetPointer();
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno = ((RegOperand*)operand)->GetRegNo();
                    def_map[regno].insert(block->block_id);
                }
            }
            // 添加数组使用的寄存器下标
            else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::GETELEMENTPTR){
                Operand operand = ((GetElementptrInstruction*)intr)->GetResult();
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno = ((RegOperand*)operand)->GetRegNo();
                    array_regno.insert(regno);
                }
            }
        }
    }
    // 删除除了数组空间分配以外的所有alloca指令
    Mem2RegNoUseAlloca(C, loaded_regno);

    // 临时变量，记录一个块内有哪些regno可以优化掉load和store
    std::map<int, std::unordered_set<int>> sameblock_bb_regno_map;//< block_id，这样的instructions>
    std::map<int, std::unordered_set<int>> onedef_bb_regno_map;   

    // 以use_map为基准遍历，顺便为phi插入收集需要处理的regno
    for(auto &[regno, bb_use_set]: use_map){
        // 一个regno的load仅出现在一个块内，且其store也只出现在同一个块内，说明可以在单个块内完成代码优化(basic情况)
        if(bb_use_set.size()==1){
            if(def_map.find(regno)!=def_map.end()){
                std::unordered_set<int> bb_def_set = def_map[regno];
                if(bb_def_set.size()==1 && *(bb_def_set.begin())==*(bb_use_set.begin())){
                    int bb_label = *(bb_def_set.begin());
                    sameblock_bb_regno_map[bb_label].insert(regno);
                }
            }
        }
        // 一个regno的load最早在多个块内都有出现，但store只在一个块内存在，说明所有load都可以被优化
        else{
            if(def_map.find(regno)!=def_map.end()){
               std::unordered_set<int> bb_def_set = def_map[regno];
                if(bb_def_set.size()==1){
                    int bb_label = *(bb_def_set.begin());
                    onedef_bb_regno_map[bb_label].insert(regno);
                }
            }
        }
    }

    for(auto &[bb, vset]: sameblock_bb_regno_map){
        store_record_map.clear();
        load_record_map.clear();
        Mem2RegUseDefInSameBlock(C, vset, bb);
    }

    for(auto &[bb, vset]: onedef_bb_regno_map){
        store_record_map.clear();
        load_record_map.clear();
        Mem2RegOneDefDomAllUses(C, vset, bb);
    }
}

void Mem2RegPass::InsertPhi(CFG *C, int& max_reg) {
    LLVMBlock zero_block = (*(C->block_map))[0];
    std::deque<Instruction> old_Intrs = zero_block->Instruction_list;
    zero_block->Instruction_list.clear();

    // 开始遍历每一个可以插入phi的regno(这里采用遍历alloca的方式进行)
    for(auto &intr: old_Intrs){
        if(intr->GetOpcode()!=BasicInstruction::LLVMIROpcode::ALLOCA){
            zero_block->Instruction_list.push_back(intr);
            continue;
        }
        // 跳过数组的alloca
        if(((AllocaInstruction*)intr)->GetDims().empty()==false){
            zero_block->Instruction_list.push_back(intr);
            continue;
        }
        Operand operand = ((AllocaInstruction*)intr)->GetResult();
        int regno = ((RegOperand*)operand)->GetRegNo();

        // 开始插入phi指令
        std::unordered_set<int> worklist = def_map[regno];
        std::unordered_set<int> historylist;//记录此regno已经插入了phi的block，避免重复
        //std::cout<<"regno: "<<regno<<std::endl;
        while(worklist.size()!=0){
            auto it = worklist.begin();
            int id = *it;
            //std::cout<<"block_id: "<<id<<std::endl;
            worklist.erase(it);

            std::set<int> DF = domtrees->GetDomTree(C)->GetDF(id);
            for(auto& df: DF){
                //std::cout<<"df: "<<df<<std::endl;
                if(historylist.find(df)==historylist.end()){
                    LLVMBlock block = (*(C->block_map))[df];
                    Instruction phi_instr = new PhiInstruction(((AllocaInstruction*)intr)->GetDataType(), GetNewRegOperand(++max_reg), regno);
                    block->InsertInstruction(0, phi_instr);
                    historylist.insert(df);
                    if(def_map[regno].find(df)==def_map[regno].end()){
                        worklist.insert(df);
                    }
                }
            }
        }
    }
}

// 变量重命名步骤：
// 1.对本block的所有指令进行store,load,phi的处理（alloca之前就已经被删掉了）
// 2.对本block的所有后继进行phi指令填充
// 3.依次对后继进行DFS
void Mem2RegPass::DFS(CFG *C, int bbid){
    if(visited_blocks.find(bbid)!=visited_blocks.end()){//保证不重
        return;
    }
    
    LLVMBlock block = (*C->block_map)[bbid];
    std::deque<Instruction> old_Intrs = block->Instruction_list;
    block->Instruction_list.clear();
    
    //std::cout<<bbid<<"\n";
    //【1】对本block的所有指令进行store,load,phi的处理（alloca之前就已经被删掉了）【即记录store/load_bb_map】
    for(auto &intr: old_Intrs){
        if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
            Operand p_operand = ((StoreInstruction*)intr)->GetPointer();
            Operand v_operand = ((StoreInstruction*)intr)->GetValue();
            if(p_operand->GetOperandType()==BasicOperand::REG){
                int p_regno = ((RegOperand*)p_operand)->GetRegNo();
                
                // 例外是数组指针，它不应该在目标内，store必须保留
                if(array_regno.find(p_regno)!=array_regno.end()){
                    intr->ChangeReg(store_bb_map[bbid], load_bb_map[bbid]);
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                intr->ChangeReg(store_bb_map[bbid], load_bb_map[bbid]);
                p_operand = ((StoreInstruction*)intr)->GetPointer();
                v_operand = ((StoreInstruction*)intr)->GetValue();
                int v_regno = ((RegOperand*)v_operand)->GetRegNo();
                p_regno = ((RegOperand*)p_operand)->GetRegNo();
                store_bb_map[bbid][p_regno] = v_regno; // 更新def map
                //std::cout<<"store: "<<p_regno<<" "<<v_regno<<std::endl;
            }
            else if(p_operand->GetOperandType()==BasicOperand::GLOBAL){
                intr->ChangeReg(store_bb_map[bbid], load_bb_map[bbid]);
                block->Instruction_list.push_back(intr);
                continue;
            }
        }else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::LOAD){
            Operand p_operand = ((LoadInstruction*)intr)->GetPointer();
            Operand r_operand = ((LoadInstruction*)intr)->GetResult();
            if(p_operand->GetOperandType()==BasicOperand::REG){
                int p_regno = ((RegOperand*)p_operand)->GetRegNo();
                
                // 例外是数组指针，它不应该在目标内，load必须保留
                if(array_regno.find(p_regno)!=array_regno.end()){
                    block->Instruction_list.push_back(intr);
                    continue;
                }
                intr->ChangeReg(store_bb_map[bbid], load_bb_map[bbid]);
                p_operand = ((LoadInstruction*)intr)->GetPointer();
                r_operand = ((LoadInstruction*)intr)->GetResult();
                int r_regno = ((RegOperand*)r_operand)->GetRegNo();
                p_regno = ((RegOperand*)p_operand)->GetRegNo();
                load_bb_map[bbid][r_regno] = p_regno; // 更新use map
                //std::cout<<"load: "<<r_regno<<" "<<p_regno<<std::endl;
            }
            else if(p_operand->GetOperandType()==BasicOperand::GLOBAL){
                block->Instruction_list.push_back(intr);
                continue;
            }
        }else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
            Operand r_operand = ((PhiInstruction*)intr)->GetResult();
            //std::cout<<"phi: "<<((PhiInstruction*)intr)->GetRegno()<<std::endl;
            int r_regno = ((RegOperand*)r_operand)->GetRegNo();
            store_bb_map[bbid][((PhiInstruction*)intr)->GetRegno()] = r_regno;
            block->Instruction_list.push_back(intr);
        }else{
            intr->ChangeReg(store_bb_map[bbid], load_bb_map[bbid]);
            block->Instruction_list.push_back(intr);
        }
    }

    //【2】对本block的所有后继进行phi指令填充
    std::set<LLVMBlock> succ = C->GetSuccessor(bbid);
    for(auto & succ_block: succ){
        // 处理后继节点的phi
         for(auto &intr: succ_block->Instruction_list){
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
                int regno = ((PhiInstruction*)intr)->GetRegno();
                if(store_bb_map[bbid].find(regno)!=store_bb_map[bbid].end())
                    ((PhiInstruction*)intr)->AddPhi({GetNewLabelOperand(bbid), GetNewRegOperand(store_bb_map[bbid][regno])});
                //std::cout<<"set phi: "<<succ[i]->block_id<<" "<<regno<<" "<<store_bb_map[bbid][regno]<<std::endl;
            }
            else{
                break;
            }
        }

        // 向后继节点传递变量
        store_bb_map[succ_block->block_id].insert(store_bb_map[bbid].begin(), store_bb_map[bbid].end());
        load_bb_map[succ_block->block_id].insert(load_bb_map[bbid].begin(), load_bb_map[bbid].end());
    }

    //【3】依次对后继进行DFS
    visited_blocks.insert(bbid);
     for(auto & succ_block: succ){
        DFS(C, succ_block->block_id);
    }
}


void Mem2RegPass::VarRename(CFG *C) {
    store_bb_map.clear();
    load_bb_map.clear();
    visited_blocks.clear();
    DFS(C, 0);

}

void Mem2RegPass::Mem2Reg(CFG *C, int& max_reg) {
    BasicMem2Reg(C);
    InsertPhi(C, max_reg);
    VarRename(C);
}

void Mem2RegPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        Mem2Reg(cfg, llvmIR->function_max_reg[defI]);
		cfg->max_reg = llvmIR->function_max_reg[defI];
        loaded_regno.clear();
        array_regno.clear();
        use_map.clear();
        def_map.clear();
    }
}
