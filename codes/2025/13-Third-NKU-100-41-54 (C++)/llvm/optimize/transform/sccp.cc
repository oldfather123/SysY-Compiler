#include "sccp.h"

// Wegman & Zadeck SCCP Algorithm
// ─────────────────────────────────────────────────────────────────────────────
// Algorithm 8.1: Sparse data-flow propagation
//
// 1. Initialization
//    • Every edge in the CFG is marked not executable;
//    • The CFGWorkList is seeded with the outgoing edges of the CFG’s start node;
//    • The SSAWorkList is empty.
//
// 2. Remove the top element of one of the two work lists.
//
// 3. If the element is a control-flow edge, proceed as follows:
//    • Mark the edge as executable;
//    • Visit every φ-operation associated with the target node (Algorithm 8.2);
//    • If the target node was reached the first time via the CFGWorkList, visit all its
//      operations (Algorithm 8.2);
//    • If the target node has a single, non-executable outgoing edge, append that edge to
//      the CFGWorkList.
//
// 4. If the element is an edge from the SSA graph, process the target operation as follows:
//    a. When the target operation is a φ-operation visit that φ-operation;
//    b. For other operations, examine the executable flag of the incoming edges of the
//       respective CFG node; Visit the operation if any of the edges is executable.
//
// 5. Continue with step 2 until both work lists become empty.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Algorithm 8.2: Visiting an operation
//
// 1. Propagate data-flow information depending on the operation’s kind:
//    a. φ-operations:
//       Combine the data-flow information from the node’s operands where the
//       corresponding control-flow edge is executable.
//    b. Conditional branches:
//       Examine the branch’s condition(s) using the data-flow information of its operands;
//       Determine all outgoing edges of the branch’s CFG node whose condition is
//       potentially satisfied; Append the CFG edges that were non-executable to the
//       CFGWorkList.
//    c. Other operations:
//       Update the operation’s data-flow information by applying its transfer function.
//
// 2. Whenever the data-flow information of an operation changes, append all outgoing SSA
//    graph edges of the operation to the SSAWorkList.
// ─────────────────────────────────────────────────────────────────────────────


// 1. 构建SSA Graph 【分析、记录def-use】
void SCCPPass::buildSSAGraph(CFG* cfg){
        for(auto &[id,block] : *(cfg->block_map)) {
            for(auto &inst : block->Instruction_list) {
                    int DefRegno = inst->GetDefRegno();
                    if(DefRegno != -1){
                        //std::cout<<"build: --regno: " << DefRegno << std::endl;
                        cfg->def_instr_map[DefRegno] = {id,inst};//维护def_instr_map
                        ValueLattice[DefRegno] = std::make_unique<Lattice>();//初始化Lattice
                        for(auto &UseRegno : inst->GetUseRegno()) {
                            cfg->SSA_Graph[UseRegno].insert(DefRegno);//维护SSA_Graph
                            //std::cout<<"SSA_Graph["<<UseRegno<<"] insertion! "<<std::endl;
                        }
                    }
                
            }
        }
    
}

// 2. 主算法函数【算法主流程、指令替换】
void SCCPPass::SCCP(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        //(1)Initialization
        for(auto &[id,block] : *(cfg->block_map)) {
            block->dfs_id = 0;//用dfs_id标记block的可达性,0不可达，其余表示访问次数
        }
        ValueLattice.clear();
        CFGWorkList.clear();
        SSAWorkList.clear();
        edges_to_remove.clear();
        cfg->def_instr_map.clear();
        for(auto &block:cfg->GetSuccessor(0)){
            CFGWorkList.push_back(block);
        }
        //std::cout<<" ---------------------- A NEW CFG ---------------------- "<<std::endl;
        buildSSAGraph(cfg);
        //补充FuncDef的参数的Lattice定义信息
        //std::cout<<"build: --func_def" << std::endl;
        auto params = defI->GetNonResultOperands();
        for(auto &param : params) {
            if(param->GetOperandType() == BasicOperand::REG) {
                int regno = ((RegOperand*)param)->GetRegNo();
                ValueLattice[regno] = std::make_unique<Lattice>();
                ValueLattice[regno]->setLattice(LatticeStatus::OVERDEF,0);
            }
        }

        // (2)Remove the top element of one of the two work lists
        while(!CFGWorkList.empty()){
            LLVMBlock top = CFGWorkList.front();
            CFGWorkList.pop_front();

            top->dfs_id ++; // 标记为可达，访问次数+1 
            //(2.1)访问指令进行常量传播
            for(auto &inst:top->Instruction_list){
                if(inst->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI){
                    visit_phi(cfg,(PhiInstruction*)inst,top->block_id);
                }else if(top->dfs_id ==1){
                    // 仅在第一次访问block时才访问其它操作
                    visit_other_operations(cfg,inst,top->block_id);
                }
            }
            std::set<LLVMBlock> succ = cfg->GetSuccessor(top);
            if(succ.size() == 1){
                LLVMBlock next = *(succ.begin());
                if(next->dfs_id == 0){
                    CFGWorkList.push_back(next);
                }
            }
            //(2.2) remove ssa tops
            while(!SSAWorkList.empty()){
                int ssa_top = SSAWorkList.front();
                SSAWorkList.pop_front();

                int block_id = cfg->def_instr_map[ssa_top].first;
                Instruction inst = cfg->def_instr_map[ssa_top].second;
                if(inst->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI){
                    visit_phi(cfg,(PhiInstruction*)inst,block_id);
                }else{
                    std::set<LLVMBlock> preds=cfg->GetPredecessor(block_id);
                    for(auto &pred:preds){
                        if(pred->dfs_id>0){
                            visit_other_operations(cfg,inst,block_id);
                            break;
                        }
                    }
                }
            }
        }

        // (3) replace llvm ir instructions
        /*
            下面对所有指令进行常量替换，即已经识别为const的RegOperand替换为ImmOperand；
                - 如果一个指令的作用不在于计算值（即def_regno=-1,如store/br/ret/void call)，必须保留这些指令，只需将其use_operand替换为imm
                - 如果一个指令的作用在于计算出值（如Arithmetic等），若其result已知结果，则此指令无意义，删除；result未知则保留，同时替换use_operand
        */
        for(auto &[id,block]:*(cfg->block_map)){
            if(block->dfs_id == 0) continue; //不可达块
            auto old_instructions = block->Instruction_list;
            block->Instruction_list.clear();
            for(auto &inst:old_instructions){
                int def_no=inst->GetDefRegno();
                //[1]非定义reg型指令，一定要有
                if(def_no==-1){
                    //[1.1] br_uncond指令：若条件为常量，则直接替换为uncond
                    if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_COND){
                        BrCondInstruction* br_inst=(BrCondInstruction*)inst;
                        int true_label = ((LabelOperand*)br_inst->GetTrueLabel())->GetLabelNo();
                        int false_label = ((LabelOperand*)br_inst->GetFalseLabel())->GetLabelNo();
                        if(true_label==false_label){
                            inst = new BrUncondInstruction(GetNewLabelOperand(true_label));
                        }
                    }
                    //[1.2] 返回void的call指令 ：将参数替换为常量
                    else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::CALL){
                        CallInstruction* call_inst=(CallInstruction*)inst;
                        assert(call_inst->GetRetType()==BasicInstruction::LLVMType::VOID);
                        std::vector<Operand> args=call_inst->GetNonResultOperands();
                        for(auto &arg_operand:args){
                            if(arg_operand->GetOperandType()==BasicOperand::REG){
                                int regno = ((RegOperand*)(arg_operand))->GetRegNo();
                                if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::INT){
                                    int arg_value = ValueLattice[regno]->val.intVal;
                                    arg_operand = new ImmI32Operand(arg_value);
                                }else if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::FLOAT){                               
                                    float arg_value = ValueLattice[regno]->val.floatVal;
                                    arg_operand=new ImmF32Operand(arg_value);
                                }
                            }
                        }
                        call_inst->SetNonResultOperands(args);
                    }
                    //[1.3] ret指令：将返回值替换为常量
                    else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::RET){
                        RetInstruction* ret_inst=(RetInstruction*)inst;
                        if(ret_inst->GetType()!=BasicInstruction::LLVMType::VOID){
                            if(ret_inst->GetRetVal()->GetOperandType()==BasicOperand::REG){
                                int regno = ((RegOperand*)(ret_inst->GetRetVal()))->GetRegNo();
                                if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::INT){
                                    int ret_value = ValueLattice[regno]->val.intVal;
                                    std::vector<Operand> ops;
                                    ops.push_back(new ImmI32Operand(ret_value));
                                    ret_inst->SetNonResultOperands(ops);

                                }else if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::FLOAT){                               
                                    float ret_value = ValueLattice[regno]->val.floatVal;
                                    std::vector<Operand> ops;
                                    ops.push_back(new ImmF32Operand(ret_value));
                                    ret_inst->SetNonResultOperands(ops);
                                }
                            }
                        }
                    }
                    //[1.4] store指令：将返回值替换为常量
                    else if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE){
                        StoreInstruction* store_inst=(StoreInstruction*)inst;
                        if(store_inst->GetValue()->GetOperandType()==BasicOperand::REG){
                            int regno = ((RegOperand*)(store_inst->GetValue()))->GetRegNo();
                            if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::INT){
                                int store_value = ValueLattice[regno]->val.intVal;
                                store_inst->SetValue(new ImmI32Operand(store_value));
                            }else if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::FLOAT){                               
                                float store_value = ValueLattice[regno]->val.floatVal;
                                store_inst->SetValue(new ImmF32Operand(store_value));
                            }
                        }
                    }

                    block->Instruction_list.push_back(inst);
                }else {
                    //[2]定义reg的指令，若结果已为const，则删除此inst
                    //[3]定义reg的指令，若结果为变量/不确定，则保留此inst，并更新其操作数
                    if(ValueLattice[def_no]->status==LatticeStatus::OVERDEF||ValueLattice[def_no]->status==LatticeStatus::UNDEF){
                        std::vector<Operand> operands = inst->GetNonResultOperands();
                        std::vector<Operand> new_operands; new_operands.reserve(operands.size());
                        for(auto &operand:operands){
                            if(operand->GetOperandType()==BasicOperand::REG){
                                int regno=((RegOperand*)operand)->GetRegNo();
                                if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::INT){
                                    int use_value = ValueLattice[regno]->val.intVal;
                                    new_operands.emplace_back(new ImmI32Operand(use_value));

                                }else if(ValueLattice[regno]->status==LatticeStatus::CONST&&ValueLattice[regno]->valtype==Lattice::FLOAT){     
                                    float use_value = ValueLattice[regno]->val.floatVal;
                                    new_operands.emplace_back(new ImmF32Operand(use_value));
                                }else{
                                    new_operands.emplace_back(operand);
                                }
                            }else{
                                new_operands.emplace_back(operand);
                            }
                        }
                        inst->SetNonResultOperands(new_operands);
                        block->Instruction_list.push_back(inst);
                    }
                }
            }
        }
    }
}

// 3. visit Phi 
void SCCPPass::visit_phi(CFG* cfg, PhiInstruction* phi, int block_id){
    int def_regno = phi->GetDefRegno();
    Lattice origin_lattice = *ValueLattice[def_regno];

    //【0】剪枝
    if(edges_to_remove.count(block_id)){
        int label_to_remove=edges_to_remove[block_id];
        auto phi_list0 = phi->GetPhiList();
        std::vector<std::pair<Operand, Operand>> phi_list;
        for(auto &phi_pair:phi_list0){
            int label_no=((LabelOperand*)phi_pair.first)->GetLabelNo();
            if(label_no!=label_to_remove){
                phi_list.push_back(phi_pair);
            }
        }
        phi->SetPhiList(phi_list);
    }
    //【1】常量传播
    auto phi_list = phi->GetPhiList();
    auto operands = phi->GetNonResultOperands();
    //[1] int 
    if(phi->GetResultType() == BasicInstruction::I32){
        LatticeStatus def_status = LatticeStatus::CONST;
        std::vector<int> operand_values; 
        operand_values.resize(operands.size());
        // [1.1] 常量状态传播
        for(int i=0;i<operands.size();i++){
            if(operands[i]->GetOperandType() == BasicOperand::REG){//reg
                int regno = ((RegOperand*)operands[i])->GetRegNo();
                def_status = Intersect(def_status,ValueLattice[regno]->status);
                operand_values[i] = ValueLattice[regno]->val.intVal;
                
                //若部分const，则将那些const的reg替换为imm
                if(ValueLattice[regno]->status==LatticeStatus::CONST){
                    auto new_phi_pair=std::make_pair(phi_list[i].first,new ImmI32Operand(operand_values[i]));
                    phi->ChangePhiPair(i,new_phi_pair);
                }

            }else {//imm
                def_status = Intersect(def_status,LatticeStatus::CONST);
                operand_values[i] = ((ImmI32Operand*)operands[i])->GetIntImmVal();
            }
        }
        //[1.2]若为常量，则判断是否为同一值，同则记录
        if(def_status == LatticeStatus::CONST){
            int const_Value=0;
            for(int i=0;i<operands.size();i++){
                if(i==0){
                    const_Value = operand_values[i];
                }else{
                    if(operand_values[i] != const_Value){
                        def_status = LatticeStatus::OVERDEF;
                        const_Value = 0;
                        break;
                    }
                }
            }
            ValueLattice[def_regno] -> setLattice(def_status,const_Value);
        }else{
            ValueLattice[def_regno] -> setLattice(def_status,0);
        }
    //[2] float
    }else if(phi->GetResultType() == BasicInstruction::FLOAT32){
        LatticeStatus def_status = LatticeStatus::CONST;
        std::vector<float> operand_values; 
        operand_values.resize(operands.size());
        // [1.1] 常量状态传播
        for(int i=0;i<operands.size();i++){
            if(operands[i]->GetOperandType() == BasicOperand::REG){//reg
                int regno = ((RegOperand*)operands[i])->GetRegNo();
                def_status = Intersect(def_status,ValueLattice[regno]->status);
                operand_values[i] = ValueLattice[regno]->val.floatVal;

                //若部分const，则将那些const的reg替换为imm
                if(ValueLattice[regno]->status==LatticeStatus::CONST){
                    auto new_phi_pair=std::make_pair(phi_list[i].first,new ImmF32Operand(operand_values[i]));
                    phi->ChangePhiPair(i,new_phi_pair);
                }
            }else {//imm
                def_status = Intersect(def_status,LatticeStatus::CONST);
                operand_values[i] = ((ImmF32Operand*)operands[i])->GetFloatVal();
            }
        }
        //[1.2]若为常量，则判断是否为同一值，同则记录
        if(def_status == LatticeStatus::CONST){
            float const_Value=0;
            for(int i=0;i<operands.size();i++){
                if(i==0){
                    const_Value = operand_values[i];
                }else{
                    if(operand_values[i] != const_Value){
                        def_status = LatticeStatus::OVERDEF;
                        const_Value = 0;
                        break;
                    }
                }
            }
            ValueLattice[def_regno] -> setLattice(def_status,const_Value);
        }else{
            ValueLattice[def_regno] -> setLattice(def_status,0);
        }
    }

 
    // 【3】传递更新结果，即将SSA后继加入SSAWorkList
    if(ValueLattice[phi->GetDefRegno()]->NE(origin_lattice)){
        std::set<int> succs;
        cfg->GetSSAGraphAllSucc(succs,def_regno);
        for(auto &s:succs){
            SSAWorkList.push_back(s);
        }
    }
    return ;
}

// 5. visit Other Operations
void SCCPPass::visit_other_operations(CFG* cfg,Instruction inst, int block_id){
    int def_regno = inst->GetDefRegno();
    Lattice origin_lattice;
    if(def_regno!=-1){
        //std::cout<<"BOTTOM  def_regno: "<<def_regno<<std::endl;
        assert(ValueLattice[def_regno]!=nullptr);
        origin_lattice = *ValueLattice[def_regno];
    }
    //std::cout<<"other inst opcode: "<<inst->GetOpcode()<<std::endl;

        switch (inst->GetOpcode()){
            //[4]Arithmetic(reg/imm)
            case BasicInstruction::LLVMIROpcode::ADD:
            case BasicInstruction::LLVMIROpcode::SUB:
            case BasicInstruction::LLVMIROpcode::MUL:
            case BasicInstruction::LLVMIROpcode::DIV:
            case BasicInstruction::LLVMIROpcode::MOD:{
                ArithmeticInstruction* arith_inst = (ArithmeticInstruction*)inst;
                Operand op1 = (arith_inst)->GetOperand1(),op2= (arith_inst)->GetOperand2();
                int regno1=0,regno2=0,value1=0,value2=0;
                LatticeStatus def_status = LatticeStatus::UNDEF;
                LatticeStatus op1_status = LatticeStatus::UNDEF,op2_status = LatticeStatus::UNDEF;
                // deal with op1
                if(op1->GetOperandType() == BasicOperand::REG){
                    regno1 = ((RegOperand*)op1)->GetRegNo();
                    if(ValueLattice[regno1]->status== LatticeStatus::CONST){
                        value1 = ValueLattice[regno1]->val.intVal;
                    }
                    op1_status = ValueLattice[regno1]->status;
                }else if(op1->GetOperandType() == BasicOperand::IMMI32){
                    value1 = ((ImmI32Operand*)op1)->GetIntImmVal();
                    op1_status = LatticeStatus::CONST;
                }
                // deal with op2
                if(op2->GetOperandType() == BasicOperand::REG){
                    regno2 = ((RegOperand*)op2)->GetRegNo();
                    if(ValueLattice[regno2]->status== LatticeStatus::CONST){
                        value2 = ValueLattice[regno2]->val.intVal;
                    }
                    op2_status = ValueLattice[regno2]->status;
                }else if(op2->GetOperandType() == BasicOperand::IMMI32){
                    value2 = ((ImmI32Operand*)op2)->GetIntImmVal();
                    op2_status = LatticeStatus::CONST;
                }
                // comp def
                def_status = Intersect(op1_status,op2_status);
                if(def_status == LatticeStatus::CONST){
                    int def_val=arith_inst->CompConst(value1,value2);
                    ValueLattice[def_regno]->setLattice(LatticeStatus::CONST,def_val);
                }else{
                    ValueLattice[def_regno]->setLattice(def_status,0);
                }
                break;
            }
            case BasicInstruction::LLVMIROpcode::FADD:
            case BasicInstruction::LLVMIROpcode::FSUB:
            case BasicInstruction::LLVMIROpcode::FMUL:
            case BasicInstruction::LLVMIROpcode::FDIV:{
                ArithmeticInstruction* arith_inst = (ArithmeticInstruction*)inst;
                Operand op1 = (arith_inst)->GetOperand1(),op2= (arith_inst)->GetOperand2();
                int regno1=0,regno2=0;
                float value1=0,value2=0;
                LatticeStatus def_status = LatticeStatus::UNDEF;
                LatticeStatus op1_status = LatticeStatus::UNDEF,op2_status = LatticeStatus::UNDEF;
                // deal with op1
                if(op1->GetOperandType() == BasicOperand::REG){
                    regno1 = ((RegOperand*)op1)->GetRegNo();
                    if(ValueLattice[regno1]->status== LatticeStatus::CONST){
                        value1 = ValueLattice[regno1]->val.floatVal;
                    }
                    op1_status = ValueLattice[regno1]->status;
                }else if(op1->GetOperandType() == BasicOperand::IMMF32){
                    value1 = ((ImmF32Operand*)op1)->GetFloatVal();
                    op1_status = LatticeStatus::CONST;
                }
                // deal with op2
                if(op2->GetOperandType() == BasicOperand::REG){
                    regno2 = ((RegOperand*)op2)->GetRegNo();
                    if(ValueLattice[regno2]->status== LatticeStatus::CONST){
                        value2 = ValueLattice[regno2]->val.floatVal;
                    }
                    op2_status = ValueLattice[regno2]->status;
                }else if(op2->GetOperandType() == BasicOperand::IMMF32){
                    value2 = ((ImmF32Operand*)op2)->GetFloatVal();
                    op2_status = LatticeStatus::CONST;
                }
                // comp def
                def_status = Intersect(op1_status,op2_status);
                if(def_status == LatticeStatus::CONST){
                    float def_val=arith_inst->CompConst(value1,value2);
                    ValueLattice[def_regno]->setLattice(LatticeStatus::CONST,def_val);
                }else{
                    ValueLattice[def_regno]->setLattice(def_status,0);
                }
                break;
            }
            //[3] cmp (reg/imm)
            case BasicInstruction::LLVMIROpcode::ICMP:{
                IcmpInstruction* icmp_inst = (IcmpInstruction*)inst;
                Operand op1 = (icmp_inst)->GetOp1(),op2= (icmp_inst)->GetOp2();
                int regno1=0,regno2=0,value1=0,value2=0;
                LatticeStatus def_status = LatticeStatus::UNDEF;
                LatticeStatus op1_status = LatticeStatus::UNDEF,op2_status = LatticeStatus::UNDEF;
                // deal with op1
                if(op1->GetOperandType() == BasicOperand::REG){
                    regno1 = ((RegOperand*)op1)->GetRegNo();
                    if(ValueLattice[regno1]->status== LatticeStatus::CONST){
                        value1 = ValueLattice[regno1]->val.intVal;
                    }
                    op1_status = ValueLattice[regno1]->status;
                }else if(op1->GetOperandType() == BasicOperand::IMMI32){
                    value1 = ((ImmI32Operand*)op1)->GetIntImmVal();
                    op1_status = LatticeStatus::CONST;
                }
                // deal with op2
                if(op2->GetOperandType() == BasicOperand::REG){
                    regno2 = ((RegOperand*)op2)->GetRegNo();
                    if(ValueLattice[regno2]->status== LatticeStatus::CONST){
                        value2 = ValueLattice[regno2]->val.intVal;
                    }
                    op2_status = ValueLattice[regno2]->status;
                }else if(op2->GetOperandType() == BasicOperand::IMMI32){
                    value2 = ((ImmI32Operand*)op2)->GetIntImmVal();
                    op2_status = LatticeStatus::CONST;
                }
                // comp def
                def_status = Intersect(op1_status,op2_status);
                if(def_status == LatticeStatus::CONST){
                    int def_val=icmp_inst->CompConst(value1,value2);
                    ValueLattice[def_regno]->setLattice(LatticeStatus::CONST,def_val);
                }else{
                    ValueLattice[def_regno]->setLattice(def_status,0);
                }
                break;
            }
            case BasicInstruction::LLVMIROpcode::FCMP:{
                FcmpInstruction* fcmp_inst = (FcmpInstruction*)inst;
                Operand op1 = (fcmp_inst)->GetOp1(),op2= (fcmp_inst)->GetOp2();
                int regno1=0,regno2=0;
                float value1=0,value2=0;
                LatticeStatus def_status = LatticeStatus::UNDEF;
                LatticeStatus op1_status = LatticeStatus::UNDEF,op2_status = LatticeStatus::UNDEF;
                // deal with op1
                if(op1->GetOperandType() == BasicOperand::REG){
                    regno1 = ((RegOperand*)op1)->GetRegNo();
                    if(ValueLattice[regno1]->status== LatticeStatus::CONST){
                        value1 = ValueLattice[regno1]->val.floatVal;
                    }
                    op1_status = ValueLattice[regno1]->status;
                }else if(op1->GetOperandType() == BasicOperand::IMMF32){
                    value1 = ((ImmF32Operand*)op1)->GetFloatVal();
                    op1_status = LatticeStatus::CONST;
                }
                // deal with op2
                if(op2->GetOperandType() == BasicOperand::REG){
                    regno2 = ((RegOperand*)op2)->GetRegNo();
                    if(ValueLattice[regno2]->status== LatticeStatus::CONST){
                        value2 = ValueLattice[regno2]->val.floatVal;
                    }
                    op2_status = ValueLattice[regno2]->status;
                }else if(op2->GetOperandType() == BasicOperand::IMMF32){
                    value2 = ((ImmF32Operand*)op2)->GetFloatVal();
                    op2_status = LatticeStatus::CONST;
                }
                // comp def
                def_status = Intersect(op1_status,op2_status);
                if(def_status == LatticeStatus::CONST){
                    float def_val=fcmp_inst->CompConst(value1,value2);
                    ValueLattice[def_regno]->setLattice(LatticeStatus::CONST,def_val);
                }else{
                    ValueLattice[def_regno]->setLattice(def_status,0);
                }
                break;
            }
            //[0] call
            case BasicInstruction::LLVMIROpcode::CALL:{
                if(((CallInstruction*)inst)->GetRetType() != BasicInstruction::LLVMType::VOID){
                   ValueLattice[def_regno]->setLattice(LatticeStatus::OVERDEF,0);
                }
                break;
            }
            case BasicInstruction::LLVMIROpcode::GETELEMENTPTR:{
                ValueLattice[def_regno]->setLattice(LatticeStatus::OVERDEF,0);
                break;
            }
            //[1] br_cond
            case BasicInstruction::LLVMIROpcode::LOAD:{
                ValueLattice[def_regno]->setLattice(LatticeStatus::OVERDEF,0);
                break;
            }
            case BasicInstruction::LLVMIROpcode::BR_COND:{
                BrCondInstruction* brcond_inst = (BrCondInstruction*)inst;
                //常量传播
                Operand cond = brcond_inst->GetCond();
                LatticeStatus def_status = LatticeStatus::UNDEF;
                bool is_true=false;
                if(cond->GetOperandType() == BasicOperand::REG){
                    int regno = ((RegOperand*)cond)->GetRegNo();
                    def_status = ValueLattice[regno]->status;
                    if(def_status == LatticeStatus::CONST){
                        is_true = ValueLattice[regno]->val.intVal;
                    }
                }else if(cond->GetOperandType() == BasicOperand::IMMI32){
                    def_status = LatticeStatus::CONST;
                    is_true = ((ImmI32Operand*)cond)->GetIntImmVal();
                }

                //剪枝
                int true_label = ((LabelOperand*)brcond_inst->GetTrueLabel())->GetLabelNo();
                int false_label = ((LabelOperand*)brcond_inst->GetFalseLabel())->GetLabelNo();
                if(def_status == LatticeStatus::CONST){
                    if(is_true){//若cond为常量，确定了target，直接替换为uncond
                        CFGWorkList.push_back((*cfg->block_map)[true_label]);
                        brcond_inst->ChangeFalseLabel(GetNewLabelOperand(true_label));
                        edges_to_remove[false_label]=block_id;
                    }else{
                        CFGWorkList.push_back((*cfg->block_map)[false_label]);
                        brcond_inst->ChangeTrueLabel(GetNewLabelOperand(false_label));
                        edges_to_remove[true_label]=block_id;
                    }
                }else{
                    CFGWorkList.push_back((*cfg->block_map)[true_label]);
                    CFGWorkList.push_back((*cfg->block_map)[false_label]);
                }
                break;
            }
            //[2] trans (reg)
            case BasicInstruction::LLVMIROpcode::FPTOSI:{
                FptosiInstruction* fptosi_inst = (FptosiInstruction*)inst;
                LatticeStatus def_status; int src_value=0;
                if((fptosi_inst)->GetSrc()->GetOperandType() == BasicOperand::REG){
                    int src_regno = ((RegOperand*)((fptosi_inst)->GetSrc()))->GetRegNo();
                    def_status = ValueLattice[src_regno]->status;
                    if(def_status == LatticeStatus::CONST){
                        src_value = ValueLattice[src_regno]->val.floatVal;
                    }
                }else if((fptosi_inst)->GetSrc()->GetOperandType() == BasicOperand::IMMF32){
                    src_value = (float)((ImmF32Operand*)((fptosi_inst)->GetSrc()))->GetFloatVal();
                    def_status = LatticeStatus::CONST;
                }
                ValueLattice[def_regno]->setLattice(def_status,src_value);
                break;
            }
            case BasicInstruction::LLVMIROpcode::SITOFP:{
                SitofpInstruction* sitofp_inst = (SitofpInstruction*)inst;
                LatticeStatus def_status; float src_value=0;
                if((sitofp_inst)->GetSrc()->GetOperandType() == BasicOperand::REG){
                    int src_regno = ((RegOperand*)((sitofp_inst)->GetSrc()))->GetRegNo();
                    def_status = ValueLattice[src_regno]->status;
                    if(def_status == LatticeStatus::CONST){
                        src_value = ValueLattice[src_regno]->val.intVal;
                    }
                }else if((sitofp_inst)->GetSrc()->GetOperandType() == BasicOperand::IMMI32){
                    src_value = (float)((ImmI32Operand*)((sitofp_inst)->GetSrc()))->GetIntImmVal();
                    def_status = LatticeStatus::CONST;
                }
                ValueLattice[def_regno]->setLattice(def_status,src_value);
                break;
            }
            case BasicInstruction::LLVMIROpcode::ZEXT:{
                ZextInstruction* zext_inst = (ZextInstruction*)inst;
                LatticeStatus def_status; int src_value=0;
                if((zext_inst)->GetSrc()->GetOperandType() == BasicOperand::REG){
                    int src_regno = ((RegOperand*)((zext_inst)->GetSrc()))->GetRegNo();
                    def_status = ValueLattice[src_regno]->status;
                    src_value = ValueLattice[src_regno]->val.intVal;
                }else if((zext_inst)->GetSrc()->GetOperandType() == BasicOperand::IMMI32){
                    def_status = LatticeStatus::CONST;
                    src_value = ((ImmI32Operand*)((zext_inst)->GetSrc()))->GetIntImmVal();
                }

                ValueLattice[def_regno]->setLattice(def_status,src_value);
                break;
            }

            default:
                break;
        }
    
    if(def_regno != -1){
        // 传递更新结果，即将SSA后继加入SSAWorkList
        Lattice new_lattice = *ValueLattice[def_regno];
        if(new_lattice.NE(origin_lattice)){ //若def_reg的状态改变，则需要继续后传
            std::set<int> succs;
            cfg->GetSSAGraphAllSucc(succs,def_regno);
            for(auto &s:succs){
            SSAWorkList.push_back(s);
        }
    }
    }
    return ;
}

void SCCPPass::ConstGlobalReplace() {
    // 若有Const的全局变量，直接用立即数替换代码
    std::unordered_map<std::string,Operand> global_const_map;
    for (auto &defI : llvmIR->global_def) {
        if(defI->GetOpcode() == BasicInstruction::LLVMIROpcode::GLOBAL_VAR){
            GlobalVarDefineInstruction* global_var = (GlobalVarDefineInstruction*)defI;
            if(global_var->IsConst()&&!global_var->IsArray()){
                std::string var_name = global_var->GetName();
                global_const_map[var_name] = global_var->GetInitVal();
            }
        }
    }

    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        for(auto &block:*(cfg->block_map)){
            for(auto &inst:block.second->Instruction_list){
                if(inst->GetOpcode() == BasicInstruction::LLVMIROpcode::LOAD){
                    LoadInstruction* load_inst = (LoadInstruction*)inst;
                    if(load_inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL){
                        std::string var_name = ((GlobalOperand*)load_inst->GetPointer())->GetName();
                        if(global_const_map.count(var_name)){
                            //如果是加载一个全局常量，则直接替换为立即数
                            Operand const_value = global_const_map[var_name];

                            auto new_inst = new ArithmeticInstruction();
                            if(const_value->GetOperandType() == BasicOperand::IMMI32){
                                new_inst->SetOpcode(BasicInstruction::LLVMIROpcode::ADD);
                                new_inst->SetType(BasicInstruction::I32);
                                new_inst->SetOperand1(new ImmI32Operand(((ImmI32Operand*)const_value)->GetIntImmVal()));
                                new_inst->SetOperand2(new ImmI32Operand(0));
                            }else if(const_value->GetOperandType() == BasicOperand::IMMF32){    
                                new_inst->SetOpcode(BasicInstruction::LLVMIROpcode::FADD);
                                new_inst->SetType(BasicInstruction::FLOAT32);
                                new_inst->SetOperand1(new ImmF32Operand(((ImmF32Operand*)const_value)->GetFloatVal()));
                                new_inst->SetOperand2(new ImmF32Operand(0));
                            }
                            new_inst->SetResult(GetNewRegOperand(load_inst->GetDefRegno()));
                            inst = new_inst; // 替换原来的load指令
                        }
                    }
                }
            }
        }
    }
}

void SCCPPass::Execute() {
    ConstGlobalReplace();
    SCCP();
}
