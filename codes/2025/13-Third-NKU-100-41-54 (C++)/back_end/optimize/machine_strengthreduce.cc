#include "machine_strengthreduce.h"

void MachineStrengthReducePass::Execute() {
    ScalarStrengthReduction();
    GepStrengthReduction();
    //LSOffsetCompute();
}

// 仅保留乘法的优化
// 除法、取模操作的强度削弱仅适用于无符号数
void MachineStrengthReducePass::ScalarStrengthReduction() {
    for (auto &func : unit->functions) {
        for (auto &block : func->blocks) {
            for (auto it = block->instructions.begin(); it != block->instructions.end();) {
                if((*it)->arch != MachineBaseInstruction::RiscV){ //跳过LOCAL_LABAEL
                    it = std::next(it);
                    continue;
                }
                auto inst = (RiscV64Instruction*)(*it);
                //[1] 乘/除/模 的削弱
                if(inst->getOpcode() == RISCV_ADDI || inst->getOpcode() == RISCV_ADDIW ||
                   inst->getOpcode() == RISCV_LI || inst->getOpcode() == RISCV_LUI ) {
                    auto next_it = std::next(it);
                    if (next_it != block->instructions.end()) {
                        auto next_inst = (RiscV64Instruction*)(*next_it);
                        /*
                        (1) 乘法--->左移
                        addiw		t0,x0,k
                        mul			t0,s1,t0    ---> slli t0,s1,k （目前方案：仅当k=2^n时才进行转换）
                        */
                        if(next_inst->getOpcode() == RISCV_MUL || next_inst->getOpcode() == RISCV_MULW) {
                            bool flag=false;  int k=0;
                            if(inst->getOpcode() == RISCV_ADDI || inst->getOpcode() == RISCV_ADDIW){
                                if(next_inst->getRs2().reg_no == inst->getRd().reg_no && !inst->getRs1().is_virtual && inst->getRs1().reg_no == 0){
                                    flag=true;
                                    k = inst->getImm();
                                }
                            }else if(next_inst->getRs2().reg_no == inst->getRd().reg_no){
                                flag=true;
                                k = inst->getImm();
                                if(inst->getOpcode() == RISCV_LUI){
                                    k=(k<<12);
                                }
                            }
                            if(flag) {
                                if(k > 0 && (k & (k - 1)) == 0 && k!=1) { // k是2的幂
                                    int shift_amount = __builtin_ctz(k);  // 获取k的二进制表示中最低位1的索引
                                    RiscV64Instruction *slli_inst = new RiscV64Instruction();
                                    if(next_inst->getOpcode() == RISCV_MUL) {
                                        slli_inst->setOpcode(RISCV_SLLI,false);
                                    } else if(next_inst->getOpcode() == RISCV_MULW) {
                                        slli_inst->setOpcode(RISCV_SLLIW,false);
                                    }
                                    slli_inst->setRd(next_inst->getRd());
                                    slli_inst->setRs1(next_inst->getRs1());
                                    slli_inst->setImm(shift_amount);

                                    it = block->instructions.erase(it);
                                    *it = slli_inst;
                                    ++it;
                                    continue;
                                }
                            }
                        }
                        /*
                        (2) 除法--->右移
                        addiw		t0,x0,k
                        div			t0,s1,t0    ---> srai t0,s1,k （目前方案：仅当k=2^n时才进行转换）
                        */
                        //else 
                        // if(next_inst->getOpcode() == RISCV_DIV || next_inst->getOpcode() == RISCV_DIVW){
                        //     if(next_inst->getRs2().reg_no == inst->getRd().reg_no && !inst->getRs1().is_virtual && inst->getRs1().reg_no == 0) {
                        //         int k = inst->getImm();
                        //         if(k > 0 && (k & (k - 1)) == 0 && k!=1) { // k是2的幂
                        //             int shift_amount = __builtin_ctz(k);  // 获取k的二进制表示中最低位1的索引
                        //             RiscV64Instruction *srai_inst = new RiscV64Instruction();
                        //             if(next_inst->getOpcode() == RISCV_DIV) {
                        //                 srai_inst->setOpcode(RISCV_SRAI,false);
                        //             } else if(next_inst->getOpcode() == RISCV_DIVW) {
                        //                 srai_inst->setOpcode(RISCV_SRAIW,false);
                        //             }
                        //             srai_inst->setRd(next_inst->getRd());
                        //             srai_inst->setRs1(next_inst->getRs1());
                        //             srai_inst->setImm(shift_amount);

                        //             it = block->instructions.erase(it);
                        //             *it = srai_inst;
                        //             ++it;
                        //             continue;
                        //         }
                        //     }
                        // }
                        /*
                        (1) 取模--->位与
                        addiw		t0,x0,k
                        rem			t0,s1,t0    ---> and t0,s1,k （目前方案：仅当k=2^n时才进行转换）
                        */
                        // else 
                        // if(next_inst->getOpcode() == RISCV_REM || next_inst->getOpcode() == RISCV_REMW){
                        //     if(next_inst->getRs2().reg_no == inst->getRd().reg_no && !inst->getRs1().is_virtual && inst->getRs1().reg_no == 0) {
                        //         int k = inst->getImm();
                        //         if(k > 0 && (k & (k - 1)) == 0 && k!=1) { // k是2的幂
                        //             int mod_amount = k - 1; // 计算模数
                        //             RiscV64Instruction *and_inst = new RiscV64Instruction();
                        //             and_inst->setOpcode(RISCV_ANDI,false);
                        //             and_inst->setRd(next_inst->getRd());
                        //             and_inst->setRs1(next_inst->getRs1());
                        //             and_inst->setImm(mod_amount);

                        //             it = block->instructions.erase(it);
                        //             *it = and_inst;
                        //             ++it;
                        //             continue;
                        //         }
                        //     }
                        // }
                    }
                }//[2] 加法的强度削弱
                else if(inst->getOpcode() == RISCV_ADD || inst->getOpcode() == RISCV_ADDW){
                    auto inst = (RiscV64Instruction*)(*it);
                    if(inst->getRd().reg_no == inst->getRs1().reg_no && inst->getRd().reg_no==inst->getRs2().reg_no ) {
                        // add x, x, x ---> x*2 ---> x<<1
                        RiscV64Instruction *slli_inst = new RiscV64Instruction();
                        if(inst->getOpcode() == RISCV_ADD) {
                            slli_inst->setOpcode(RISCV_SLLI,false);
                        } else if(inst->getOpcode() == RISCV_ADDW) {
                            slli_inst->setOpcode(RISCV_SLLIW,false);
                        }
                        slli_inst->setRd(inst->getRd());
                        slli_inst->setRs1(inst->getRs1());
                        slli_inst->setImm(1);

                        *it = slli_inst;
                        ++it;
                        continue;
                    }
                }
                ++it;
            }
        }
    }
}

/*
GEP指令冗余消除： 
（1）%r236 = getelementptr i32, ptr %r92, i32 1 
    - 若ptr为普通指针变量，则模式为addiw-addiw-mul-slli-add
    - 若ptr为sp保存的数组基址，会多一条从sp取值的指令，即模式为addiw-addiw-mul-slli-addi-add
（2）%r235 = getelementptr i32, ptr %r10, i32 0
    基本模式：slli xx,x0,2 -- addi(sp)  -- add
*/


void MachineStrengthReducePass::GepStrengthReduction() {
    for (auto &func : unit->functions) {
        for (auto &block : func->blocks) {
            auto old_block = block->instructions;
            block->instructions.clear();

            auto it = old_block.begin();
            while (it != old_block.end()) {
                auto inst1 = (RiscV64Instruction*)(*it);

                //一次取6条指令
                auto it2 = std::next(it);
                auto it3 = (it2 != old_block.end()) ? std::next(it2) : old_block.end();
                auto it4 = (it3 != old_block.end()) ? std::next(it3) : old_block.end();
                auto it5 = (it4 != old_block.end()) ? std::next(it4) : old_block.end();
                auto it6 = (it5 != old_block.end()) ? std::next(it5) : old_block.end();

                bool matched = false;
                if (it2 != old_block.end() && it3 != old_block.end() && it4 != old_block.end() 
                    && it5 != old_block.end() &&it6 != old_block.end()) {
                    auto inst2 = (RiscV64Instruction*)(*it2);
                    auto inst3 = (RiscV64Instruction*)(*it3);
                    auto inst4 = (RiscV64Instruction*)(*it4);
                    auto inst5 = (RiscV64Instruction*)(*it5);
                    auto inst6 = (RiscV64Instruction*)(*it6);

                    //【1】%r236 = getelementptr i32, ptr %r92, i32 1 基址指针
                    if (inst1->getOpcode() == RISCV_ADDIW && inst1->getRs1().reg_no == 0 &&
                        inst2->getOpcode() == RISCV_ADDIW && inst2->getRs1().reg_no == 0 &&
                        inst3->getOpcode() == RISCV_MUL &&
                            inst3->getRs1().reg_no == inst2->getRd().reg_no &&
                            inst3->getRs2().reg_no == inst1->getRd().reg_no &&
                        inst4->getOpcode() == RISCV_SLLI &&
                            inst4->getRs1().reg_no == inst3->getRd().reg_no &&
                            inst4->getImm() == 2 &&
                        inst5->getOpcode() == RISCV_ADDI &&
                            inst5->getRs1().reg_no == RISCV_sp &&
                        inst6->getOpcode() == RISCV_ADD &&
                            inst6->getRs1().reg_no == inst5->getRd().reg_no &&
                            inst6->getRs2().reg_no == inst4->getRd().reg_no ) 
                    {
                        // 完全匹配：做冗余消除
                        int op1 = inst1->getImm();
                        int op2 = inst2->getImm();
                        int index = 4 * op1 * op2;
                        int offset = index + inst5->getImm();

                        // ADDI 立即数需在 [-2048, 2047] 范围内
                        if (offset < -2048 || offset > 2047) {
                            // 需要先li立即数到寄存器，再 add
                            RiscV64Instruction* li_inst = new RiscV64Instruction();
                            li_inst->setOpcode(RISCV_LI, false);
                            li_inst->setRd(inst4->getRd()); 
                            li_inst->setImm(offset);
                            block->instructions.push_back(li_inst); // li   tx,  offset
                            inst6->setRs1(inst5->getRs1());          
                            block->instructions.push_back(inst6);   // add  res,sp, tx
                        } else {
                            // 直接用 ADDI
                            RiscV64Instruction* new_addi = new RiscV64Instruction();
                            new_addi->setOpcode(RISCV_ADDI, false);
                            new_addi->setRd(inst6->getRd());
                            new_addi->setRs1(inst5->getRs1());
                            new_addi->setImm(offset);
                            block->instructions.push_back(new_addi);//addi res, sp, offset
                        }

                        // 跳过这 6 条原始指令
                        std::advance(it, 6);
                        matched = true;
                    }
                    //【2】%r236 = getelementptr i32, ptr %r92, i32 1 普通指针
                    else if (inst1->getOpcode() == RISCV_ADDIW && inst1->getRs1().reg_no == 0 &&
                        inst2->getOpcode() == RISCV_ADDIW && inst2->getRs1().reg_no == 0 &&
                        inst3->getOpcode() == RISCV_MUL &&
                            inst3->getRs1().reg_no == inst2->getRd().reg_no &&
                            inst3->getRs2().reg_no == inst1->getRd().reg_no &&
                        inst4->getOpcode() == RISCV_SLLI &&
                            inst4->getRs1().reg_no == inst3->getRd().reg_no &&
                            inst4->getImm() == 2 &&
                        inst5->getOpcode() == RISCV_ADD &&
                            inst5->getRs2().reg_no == inst4->getRd().reg_no) 
                    {
                        // 完全匹配：做 strength reduction
                        int op1 = inst1->getImm();
                        int op2 = inst2->getImm();
                        int index = 4 * op1 * op2;

                        // ADDI 立即数需在 [-2048, 2047] 范围内
                        if (index < -2048 || index > 2047) {
                            // 需要先li立即数到寄存器，再 add
                            RiscV64Instruction* li_inst = new RiscV64Instruction();
                            li_inst->setOpcode(RISCV_LI, false);
                            li_inst->setRd(inst4->getRd()); 
                            li_inst->setImm(index);
                            block->instructions.push_back(li_inst);

                            block->instructions.push_back(inst5);
                        } else {
                            // 直接用 ADDI
                            RiscV64Instruction* new_addi = new RiscV64Instruction();
                            new_addi->setOpcode(RISCV_ADDI, false);
                            new_addi->setRd(inst5->getRd());
                            new_addi->setRs1(inst5->getRs1());
                            new_addi->setImm(index);
                            block->instructions.push_back(new_addi);
                        }

                        // 跳过这 5 条原始指令
                        std::advance(it, 5);
                        matched = true;
                    }
                }
                // 【3】%r235 = getelementptr i32, ptr %r10, i32 0
                /*
                slli		t0,x0,2
	            addi		t1,sp,128
	            add			t0,t1,t0   ---> add t0, sp, 128
                */
                if(!matched && it2 != old_block.end() && it3 != old_block.end()){
                    auto inst2 = (RiscV64Instruction*)(*it2);
                    auto inst3 = (RiscV64Instruction*)(*it3);

                    if(inst1->getOpcode() == RISCV_SLLI && inst1->getRs1().reg_no == RISCV_x0 &&
                       inst2->getOpcode() == RISCV_ADDI && inst2->getRs1().reg_no == RISCV_sp &&
                       inst3->getOpcode() == RISCV_ADD && 
                       inst3->getRs1().reg_no == inst2->getRd().reg_no &&
                       inst3->getRs2().reg_no == inst1->getRd().reg_no){
                        
                        // 完全匹配：做冗余消除
                        RiscV64Instruction* new_addi = new RiscV64Instruction();
                        new_addi->setOpcode(RISCV_ADDI, false);
                        new_addi->setRd(inst3->getRd());
                        new_addi->setRs1(inst2->getRs1());
                        new_addi->setImm(inst2->getImm());
                        block->instructions.push_back(new_addi);

                        // 跳过这 3 条原始指令
                        std::advance(it, 3);
                        matched = true;
                    }
                }

                if (!matched) {
                    // 没匹配上：原样搬运当前这一条
                    block->instructions.push_back(inst1);
                    ++it;
                }
            }
        }
    }
}

//访存地址强度削弱
/*
sw xx, 0(t0)                 sw xx, 0(t0)
...
addi t0, t0,4         
...
sw xx, 0(t0)         ==>     sw xx, 4(t0) 
...
addi t0, t0,4
...
sw xx, 0(t0)                 sw xx, 8(t0) 
*/
/*
1. 目前实现了栈上内存访问的指令合并，（sp的值在函数开始即设置好，不会更改，不存在连锁效应）
2. 大多数自增访问存在连锁反应
*/
void MachineStrengthReducePass::LSOffsetCompute(){
    // for (auto &func : unit->functions) {
    //     std::cout<<"-----------function: "<<func->getFunctionName()<<std::endl;
    //     for (auto &block : func->blocks) {
    //         int offset = 0;
    //         int ba_regno=0;
    //         std::vector<RiscV64Instruction*> insts_todlt;
    //         std::cout<<" --- block "<<block->getLabelId()<<std::endl;
    //         for(auto it = block->instructions.begin(); it!=block->instructions.end();){
    //             auto inst=(RiscV64Instruction*)(*it);
    //             if(inst->getOpcode()==RISCV_SW){
    //                 offset=0;
    //                 ba_regno=inst->getRs2().reg_no;
    //                 auto look_ahead=std::next(it);
    //                 bool matched=false;
    //                 while(look_ahead!=block->instructions.end()){
    //                     auto next_inst=(RiscV64Instruction*)(*look_ahead);
    //                     if((next_inst->getRd().reg_no!=ba_regno&&next_inst->getOpcode()!=RISCV_SW)){
    //                         ++look_ahead;
    //                     }else if(next_inst->getOpcode()==RISCV_ADDI && // addi t0, t0, imm
    //                            next_inst->getRd().reg_no==next_inst->getRs1().reg_no){

    //                     }else if(next_inst->getOpcode()==RISCV_SW){

    //                     }else{


    //                     }
    //                 }
    //             }else if(inst->getOpcode()==RISCV_LW){

    //             }

    //             ++it;
    //         }
    //     }
    // }


    for (auto &func : unit->functions) {
        std::cout<<"-----------function: "<<func->getFunctionName()<<std::endl;
        for (auto &block : func->blocks) {
            std::cout<<" --- block "<<block->getLabelId()<<std::endl;
            for(auto it = block->instructions.rbegin(); it!=block->instructions.rend();){
                if((*it)->arch != MachineBaseInstruction::RiscV){ //跳过LOCAL_LABAEL
                    it = std::next(it);
                    continue;
                }
                auto inst = (RiscV64Instruction*)(*it); 
                //std::cout<<OpTable.at(inst->getOpcode()).name<<std::endl;

                if(inst->getOpcode() == RISCV_SW && !inst->getUseLabel() ){
                    int basic_address = inst->getRs2().reg_no;
                    auto look_ahead = std::next(it);
                    while(look_ahead!=block->instructions.rend() &&
                          ((RiscV64Instruction*)(*look_ahead))->getRd().reg_no!=basic_address){
                        ++look_ahead;
                    }
                    //若basic_address的计算不在此Block中，放弃
                    if(look_ahead==block->instructions.rend()){
                        break;
                    }
                    //若找到，看basic_address的计算是否可以合并到sw指令中
                    auto next_it =look_ahead;
                    auto next_inst = (RiscV64Instruction*)(*next_it);
                    //std::cout<<OpTable.at(next_inst->getOpcode()).name<<std::endl;
                    if((next_inst->getOpcode()==RISCV_ADDI || next_inst->getOpcode()==RISCV_ADDIW)&&
                        next_inst->getRs1().reg_no == RISCV_sp ){
                        inst->setRs2(next_inst->getRs1());
                        inst->setImm(inst->getImm()+next_inst->getImm());
                        auto base = next_it.base();
                        --base;
                        it = std::reverse_iterator<decltype(base)>(block->instructions.erase(base));
                        continue;
                    }
                }else if(inst->getOpcode() == RISCV_LW && !inst->getUseLabel() ){
                    int basic_address = inst->getRs1().reg_no;
                    auto look_ahead = std::next(it);
                    while(look_ahead!=block->instructions.rend() &&
                          ((RiscV64Instruction*)(*look_ahead))->getRd().reg_no!=basic_address){
                        ++look_ahead;
                    }
                    //若basic_address的计算不在此Block中，放弃
                    if(look_ahead==block->instructions.rend()){
                        break;
                    }
                    //若找到，看basic_address的计算是否可以合并到sw指令中
                    auto next_it =look_ahead;
                    auto next_inst = (RiscV64Instruction*)(*next_it);
                    //std::cout<<OpTable.at(next_inst->getOpcode()).name<<std::endl;
                    if((next_inst->getOpcode()==RISCV_ADDI || next_inst->getOpcode()==RISCV_ADDIW)&&
                        next_inst->getRs1().reg_no == RISCV_sp){
                        inst->setRs1(next_inst->getRs1());
                        inst->setImm(inst->getImm()+next_inst->getImm());
                        auto base = next_it.base();
                        --base;
                        it = std::reverse_iterator<decltype(base)>(block->instructions.erase(base));
                        continue;
                    }
                }
                it = std::next(it);
            }
        }
    }
}