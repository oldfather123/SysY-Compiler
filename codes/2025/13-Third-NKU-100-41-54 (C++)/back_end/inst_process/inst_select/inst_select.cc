#include"inst_select.h"
#include"machine_instruction.h"
#include <functional>

static int local_label_counter = 0;

// 所有模板特化声明
template <> void RiscV64Unit::ConvertAndAppend<Instruction>(Instruction inst);
template <> void RiscV64Unit::ConvertAndAppend<ArithmeticInstruction*>(ArithmeticInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<LoadInstruction*>(LoadInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<StoreInstruction*>(StoreInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<IcmpInstruction*>(IcmpInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<PhiInstruction*>(PhiInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<AllocaInstruction*>(AllocaInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<BrCondInstruction*>(BrCondInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<BrUncondInstruction*>(BrUncondInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<FcmpInstruction*>(FcmpInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<RetInstruction*>(RetInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<ZextInstruction*>(ZextInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<FptosiInstruction*>(FptosiInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<GetElementptrInstruction*>(GetElementptrInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<CallInstruction*>(CallInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<SitofpInstruction*>(SitofpInstruction* inst);
template <> void RiscV64Unit::ConvertAndAppend<BitCastInstruction*>(BitCastInstruction* inst);

void RiscV64Unit::SelectInstructionAndBuildCFG()
{
    global_def = IR->global_def;

    for (auto [defI,cfg] : IR->llvm_cfg) {
        cfg->BuildCFG();
        if (cfg && cfg->DomTree) {
            ((DominatorTree*)cfg->DomTree)->BuildDominatorTree();
        }
        if(cfg == nullptr){
            ERROR("LLVMIR CFG is Empty, you should implement BuildCFG in MidEnd first");
        }
        std::string name = cfg->function_def->GetFunctionName();

        cur_func = new RiscV64Function(name);
        cur_func->SetParent(this);
        functions.push_back(cur_func);

        auto cur_mcfg = new MachineCFG;
        cur_func->SetMachineCFG(cur_mcfg);
        cur_cfg = cfg;

        ClearFunctionSelectState();

        auto funcdefI = (FunctionDefineInstruction*)defI;
        int i32_cnt = 0;
        int f32_cnt = 0;

        for(int i = 0; i < funcdefI->formals_reg.size(); ++i){
            auto para = funcdefI->formals_reg[i];
            auto paratype = funcdefI->formals[i];
            // 添加参数寄存器Register, 方便riscv64_lowerframe.cc使用
            if(paratype == BasicInstruction::LLVMType::I32 || paratype == BasicInstruction::LLVMType::PTR){
				auto newpara = GetNewRegister(((RegOperand *)funcdefI->formals_reg[i])->GetRegNo(), INT64);
				cur_func->AddParameter(newpara);
            }else if(paratype == BasicInstruction::LLVMType::FLOAT32){
				auto newpara = GetNewRegister(((RegOperand *)funcdefI->formals_reg[i])->GetRegNo(), FLOAT64);
				cur_func->AddParameter(newpara);
            }else{
                ERROR("Unknown type");
            }
        }

        BuildPhiWeb(cfg);
        
        for (auto [id, block] : *(cfg->block_map)) {
            cur_block = new RiscV64Block(id);
            curblockmap[id] = cur_block;
            cur_mcfg->AssignEmptyNode(id, cur_block);
            cur_func->UpdateMaxLabel(id);

            cur_block->setParent(cur_func);
            cur_func->blocks.push_back(cur_block);

            block->dfs_id=0;
        }

		// icmp / fcmp not found error 
		// slove method1: select icmp_inst/fcmp_inst previously (no use)
		// try : 1. typeid(*instruction).name(), 2. dynamic_cast<>
		// slove method2: search the blocks in domtree
		DomtreeDfs((*cfg->block_map)[0], cfg);

        if (cur_offset % 8 != 0) {
            cur_offset = ((cur_offset + 7) / 8) * 8;
        }
		// 设置临时变量开辟的栈空间
		int new_offset = ((cur_offset + cur_func->GetParaSize() + 7) / 8) * 8;
        cur_func->SetStackSize(new_offset);

		// 当参数溢出的时候，会影响局部变量的位置
		// 难点.遍历完被调用的所有函数才能知道参数栈的偏移, 此时所有的指令都已经选择
		// 解决方案：先预存所有的局部变量alloca指令，在最后的时候加上参数栈偏移
		int paraSize = (cur_func->GetParaSize() + 7) / 8 * 8;
 		((RiscV64Function *)cur_func)->AddParameterSize(cur_func->GetParaSize());
		
        // 控制流图连边
        for (int i = 0; i < cfg->G.size(); i++) {
            const auto &arcs = cfg->G[i];
            for (auto arc : arcs) {
                cur_mcfg->MakeEdge(i, arc->block_id);
            }
        }

		// cur_mcfg->display();

        // 遍历完所有基本块后插入phi生成的额外指令
        for (auto [id, block] : *(cfg->block_map)) {
            cur_block = curblockmap[id];
            if(!phi_instr_map[id].empty()){
                std::deque<MachineBaseInstruction *> terminalI_que;
                auto nowI = cur_block->GetInsList().back();
                while(nowI != terminal_instr_map[id]){
                    terminalI_que.push_back(nowI);
                    cur_block->pop_back();
                    nowI = cur_block->GetInsList().back();
                }
                for(auto I : phi_instr_map[id]){
                    cur_block->push_back(I);
                }
                while(!terminalI_que.empty()){
                    cur_block->push_back(terminalI_que.back());
                    terminalI_que.pop_back();
                }
            }
        }

    //按照中端支配树建立后端支配树
    cur_mcfg->SetMachineDomTree((DominatorTree*)(cfg->DomTree));
    cur_mcfg->DomTree->C=cur_mcfg;
    }


    LowerFrame(); //最后调用LowerFrame
}
void RiscV64Unit::LowerFrame()
{
    // 在每个函数的开头处插入获取参数的指令
    for (auto func : functions) {
        cur_func = func;
        for (auto &b : func->blocks) {
            cur_block = b;
            if (cur_block->getLabelId() == 0) {    // 函数入口，需要插入获取参数的指令
                int i32_cnt = 0;
				int f32_cnt = 0;
				int arg_off = 0;
                for (auto para : func->GetParameters()) {   
					if (para.type.data_type == INT64.data_type) {	
                        if (i32_cnt < 8) {    // 插入使用寄存器传参的指令
                            cur_block->push_front(rvconstructor->ConstructR(RISCV_ADD, para, GetPhysicalReg(RISCV_a0 + i32_cnt),
                                                                    GetPhysicalReg(RISCV_x0)));
                        }
                        if (i32_cnt >= 8) {    // 插入使用内存传参的指令
							if (arg_off <= 2047 && arg_off >= -2048) {
								cur_block->push_front(rvconstructor->ConstructIImm(RISCV_LD, para, GetPhysicalReg(RISCV_fp), arg_off)); 
							} else {
								auto imm_reg = cur_func->GetNewRegister(INT64.data_type, INT64.data_length);
								auto offset_mid_reg = cur_func->GetNewRegister(INT64.data_type, INT64.data_length);
								cur_block->push_front(rvconstructor->ConstructIImm(RISCV_LD, para, offset_mid_reg, 0));
								cur_block->push_front(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_fp), imm_reg));
								cur_block->push_front(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
							}

                            arg_off += 8;
                        }
                        i32_cnt++;
                    } else if (para.type.data_type == FLOAT64.data_type) {    // 处理浮点数
						if (f32_cnt < 8) {    // 插入使用寄存器传参的指令
							cur_block->push_front(rvconstructor->ConstructR2(RISCV_FMV_S, para, GetPhysicalReg(RISCV_fa0 + f32_cnt)));
                        }
                        if (f32_cnt >= 8) {    // 插入使用内存传参的指令
							if (arg_off <= 2047 && arg_off >= -2048) {
								cur_block->push_front(rvconstructor->ConstructIImm(RISCV_FLD, para, GetPhysicalReg(RISCV_fp), arg_off)); 
							} else {
								auto imm_reg = cur_func->GetNewRegister(INT64.data_type, INT64.data_length);
								auto offset_mid_reg = cur_func->GetNewRegister(INT64.data_type, INT64.data_length);
								cur_block->push_front(rvconstructor->ConstructIImm(RISCV_FLD, para, offset_mid_reg, 0));
								cur_block->push_front(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_fp), imm_reg));
								cur_block->push_front(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
							}

                            arg_off += 8;
                        }
                        f32_cnt++;
                    } else {
                        ERROR("Unknown type");
                    }
                }

				if (arg_off != 0) {
                    cur_func->SetHasInParaInStack(true);
                }
            }
        }
    }
}

const int TotalRegs = 64;
extern bool optimize_flag;//在main.cc中定义
void RiscV64Unit::LowerStack()
{
    //逐一处理函数
    for(auto func:functions)
    {
        //一、前置工作->获取被调用者保存寄存器的定义块与读写块信息
        //1.设置当前函数
        cur_func=func;
        //2.设置"寄存器定义块（写）"与"寄存器读写块"的信息，一维代表寄存器编号，二维代表出现块的编号
        std::vector<std::vector<int>> saveregs_occurblockids,saveregs_rwblockids;
        //GatherUseSregs(func,saveregs_occurblockids,saveregs_rwblockids);
        saveregs_occurblockids.resize(TotalRegs);
        saveregs_rwblockids.resize(TotalRegs);
        //3.遍历当前函数的所有基本块来设置
        for(auto &b:func->blocks)
        {
            //1）保存"被调用者保存寄存器"s0-s11,fs0-fs11,ra，保证函数被调用后这些寄存器的值能恢复原样
            std::bitset<TotalRegs> RegNeedSaved;
            RegNeedSaved.set(RISCV_s0); RegNeedSaved.set(RISCV_s1); RegNeedSaved.set(RISCV_s2);
            RegNeedSaved.set(RISCV_s3); RegNeedSaved.set(RISCV_s4); RegNeedSaved.set(RISCV_s5);
            RegNeedSaved.set(RISCV_s6); RegNeedSaved.set(RISCV_s7); RegNeedSaved.set(RISCV_s8);
            RegNeedSaved.set(RISCV_s9); RegNeedSaved.set(RISCV_s10); RegNeedSaved.set(RISCV_s11);
            RegNeedSaved.set(RISCV_fs0); RegNeedSaved.set(RISCV_fs1); RegNeedSaved.set(RISCV_fs2);
            RegNeedSaved.set(RISCV_fs3); RegNeedSaved.set(RISCV_fs4); RegNeedSaved.set(RISCV_fs5);
            RegNeedSaved.set(RISCV_fs6); RegNeedSaved.set(RISCV_fs7); RegNeedSaved.set(RISCV_fs8);
            RegNeedSaved.set(RISCV_fs9); RegNeedSaved.set(RISCV_fs10); RegNeedSaved.set(RISCV_fs11);
            RegNeedSaved.set(RISCV_ra);

            //2）保存当前块中被访问过的寄存器
            std::bitset<TotalRegs> RegVisited; // 标记当前块中该寄存器是否已经记录过使用（读或写）
            std::bitset<TotalRegs> RegWritten; // 标记当前块中该寄存器是否已经记录过写（用于避免重复记录到occurblockids）
            for(auto ins:*b)
            {
                for (auto reg : ins->GetWriteReg())
                {
                    if (!reg->is_virtual && RegNeedSaved[reg->reg_no])
                    {
                        if (!RegVisited[reg->reg_no])
                        {
                            RegVisited[reg->reg_no] = true;
                            saveregs_rwblockids[reg->reg_no].push_back(b->getLabelId());
                        }
                        if (!RegWritten[reg->reg_no])
                        {
                            RegWritten[reg->reg_no] = true;
                            saveregs_occurblockids[reg->reg_no].push_back(b->getLabelId());
                        }
                    }
                }
                for (auto reg : ins->GetReadReg())
                {
                    if (!reg->is_virtual && RegNeedSaved[reg->reg_no] && !RegVisited[reg->reg_no])
                    {
                        RegVisited[reg->reg_no] = true;
                        saveregs_rwblockids[reg->reg_no].push_back(b->getLabelId());
                    }
                }
            }
        }
        //4.保存fp（s0）寄存器。如果有参数溢出到栈，则需要在入口块使用fp寄存器，把fp抬高，故标记。
        if (func->HasInParaInStack()) 
        {
            saveregs_occurblockids[RISCV_fp].push_back(0);//fp实际上就是s0寄存器，作为栈底位置。之后都通过fp+n来处理
            saveregs_rwblockids[RISCV_fp].push_back(0);
        }
        //二、栈空间分配阶段
        //1.设置辅助变量：需要store操作的基本块、需要load操作的基本块、需要恢复的偏移量
        std::vector<int> sd_blocks;
        std::vector<int> ld_blocks;
        std::vector<int> restore_offset;
        sd_blocks.resize(64);
        ld_blocks.resize(64);
        restore_offset.resize(64);
        int saveregnum = 0, cur_restore_offset = 0;
        //2.统计需要在栈中保存的寄存器信息：如果寄存器被修改且使用过，则需要单独保存这个寄存器
         for (int i = 0; i < saveregs_occurblockids.size(); i++) { // 遍历保存寄存器的出现块 ID
            auto &vld = saveregs_rwblockids[i]; // 获取与当前保存寄存器相关的读写块 ID
            if (!vld.empty()) { // 如果读写块不为空
                saveregnum++; // 保存寄存器数量加1
            }
        }
        //3.增加大小（到这里）
        func->AddStackSize(saveregnum*8);
        //2.恢复栈空间
        auto mcfg = func->getMachineCFG(); // 获取函数的机器控制流图
        bool restore_at_beginning = (-8 + func->GetStackSize()) >= 2048; // 如果函数栈空间太大，需要在开始时恢复寄存器
        if (!optimize_flag) { // 如果未启用优化标志
            restore_at_beginning = true; // 强制在开始时恢复寄存器
        }
        //1）如果无需在开始时恢复寄存器
        // if(!restore_at_beginning)
        // {//...
        // }

         for (auto &b : func->blocks) {
            cur_block = b;
            if (b->getLabelId() == 0) {
                if (func->GetStackSize() <= 2032) {
                    b->push_front(rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                               GetPhysicalReg(RISCV_sp),
                                                               -func->GetStackSize()));    // sub sp
                } else {
                    auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                    b->push_front(rvconstructor->ConstructR(RISCV_SUB, GetPhysicalReg(RISCV_sp),
                                                            GetPhysicalReg(RISCV_sp), stacksz_reg));
                    // 修复1：先执行lui, 再执行ori, lui负责清空低12位
					// auto addiw_instr1 = rvconstructor->ConstructUImm(RISCV_LUI, stacksz_reg,  (func->GetStackSize() + (1 << 11)) >> 12);
                    // auto addiw_instr2 = rvconstructor->ConstructIImm(RISCV_ORI, stacksz_reg, stacksz_reg,func->GetStackSize()& 0xfff);
                    // b->push_front(addiw_instr2);
					// b->push_front(addiw_instr1);
					
					// 修复2：即使修复了lui和ori的顺序，但是爆了一个如下非法指令，4048明明是在12位限制内的
					// test_output/example/temp.out.S:9: Error: illegal operands `ori t0,t0,4048'
					// test_output/example/temp.out.S:74: Error: illegal operands `ori t0,t0,4048'
					
                    b->push_front(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize()));
                }
                if (func->HasInParaInStack()) {
					b->push_front(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_fp), GetPhysicalReg(RISCV_sp), GetPhysicalReg(RISCV_x0)));    // fp = sp 栈帧切换
                }
                // fp should always be restored at beginning now
                if (restore_at_beginning) {
                    int offset = 0;
                    for (int i = 0; i < 64; i++) {
                        if (!saveregs_occurblockids[i].empty()) {
                            int regno = i;
                            offset -= 8;
                            if (regno >= RISCV_x0 && regno <= RISCV_x31) {
                                b->push_front(rvconstructor->ConstructSImm(RISCV_SD, GetPhysicalReg(regno),
                                                                           GetPhysicalReg(RISCV_sp), offset));
                            } else {
                                b->push_front(rvconstructor->ConstructSImm(RISCV_FSD, GetPhysicalReg(regno),
                                                                           GetPhysicalReg(RISCV_sp), offset));
                            }
                        }
                    }
                } else if (func->HasInParaInStack()) {
                    b->push_front(rvconstructor->ConstructSImm(RISCV_SD, GetPhysicalReg(RISCV_fp),
                                                               GetPhysicalReg(RISCV_sp), restore_offset[RISCV_fp]));
                }
            }
            auto y_ins = *(b->ReverseBegin());
            if(y_ins==nullptr){continue;}

            //Assert(y_ins->arch == MachineBaseInstruction::RiscV);
            auto riscv_y_ins = (RiscV64Instruction *)y_ins;
            if (riscv_y_ins->getOpcode() == RISCV_JALR) {
                if (riscv_y_ins->getRd() == GetPhysicalReg(RISCV_x0)) {
                    if (riscv_y_ins->getRs1() == GetPhysicalReg(RISCV_ra)) {
                        Assert(riscv_y_ins->getImm() == 0);
                        b->pop_back();
                        // b->push_back(rvconstructor->ConstructComment("Lowerstack: add sp\n"));
                        if (func->GetStackSize() <= 2032) {
                            b->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                                      GetPhysicalReg(RISCV_sp), func->GetStackSize()));
                        } else {
                            auto stacksz_reg = GetPhysicalReg(RISCV_t0);
							// bug 同上，故改用伪指令 li
                            // auto lui_inst = rvconstructor->ConstructUImm(RISCV_LUI, stacksz_reg,  (func->GetStackSize() + (1 << 11)) >> 12);
                            // auto ori_inst = rvconstructor->ConstructIImm(RISCV_ORI, stacksz_reg, stacksz_reg,func->GetStackSize()& 0xfff);
                            // b->push_back(lui_inst);
							// b->push_back(ori_inst);
                            b->push_back(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize()));
                            b->push_back(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_sp),
                                                                   GetPhysicalReg(RISCV_sp), stacksz_reg));
                        }
                        if (restore_at_beginning) {
                            int offset = 0;
                            for (int i = 0; i < 64; i++) {
                                if (!saveregs_occurblockids[i].empty()) {
                                    int regno = i;
                                    offset -= 8;
                                    if (regno >= RISCV_x0 && regno <= RISCV_x31) {
                                        b->push_back(rvconstructor->ConstructIImm(RISCV_LD, GetPhysicalReg(regno),
                                                                                  GetPhysicalReg(RISCV_sp), offset));
                                    } else {
                                        b->push_back(rvconstructor->ConstructIImm(RISCV_FLD, GetPhysicalReg(regno),
                                                                                  GetPhysicalReg(RISCV_sp), offset));
                                    }
                                }
                            }
                        }
                        b->push_back(riscv_y_ins);
                    }
                }
            }
         }
    }

}


void RiscV64Unit::ClearFunctionSelectState() { 
    llvmReg_offset_map.clear();
    resReg_cmpInst_map.clear();
    llvm2rv_regmap.clear();
    phi_instr_map.clear();
    terminal_instr_map.clear();
    phi_temp_phiseqmap.clear();
    phi_temp_registermap.clear();
    mem2reg_destruc_set.clear();
    curblockmap.clear();
	cur_offset = 0; 
}

Register RiscV64Unit::GetNewRegister(int llvm_regNo, MachineDataType type) {
	// 不存在llvm_reg到rv_reg的映射，则新建; 否则，直接返回。
	if (llvm2rv_regmap.find(llvm_regNo) == llvm2rv_regmap.end()) {
        llvm2rv_regmap[llvm_regNo] = cur_func->GetNewRegister(type.data_type, type.data_length);
    }
    return llvm2rv_regmap[llvm_regNo];
}

Register RiscV64Unit::GetNewTempRegister(MachineDataType type) {
	return cur_func->GetNewRegister(type.data_type, type.data_length);
}

void RiscV64Unit::InsertImmI32Instruction(Register resultRegister, Operand op, MachineBlock* insert_block, bool isPhiPost){
    // 负数处理: https://blog.csdn.net/qq_52505851/article/details/132085930
    // https://quantaly.github.io/riscv-li/
    // RISC-V handles 32-bit constants and addresses with instructions 
    // that set the upper 20 bits of a 32-bit register. Load upper 
    // immediate lui loads 20 bits into bits 31 through 12. Then a 
    // second instruction such as addi can set the bottom 12 bits. 
    // Small numbers or addresses can be formed by using the zero register instead of lui.
    auto immop = (ImmI32Operand*)op;
    auto imm = immop->GetIntImmVal();
    if(imm >= imm12Min && imm <= imm12Max){
        // addiw        %result, x0, imm
        auto addiw_instr = rvconstructor->ConstructIImm(RISCV_ADDIW, resultRegister, GetPhysicalReg(RISCV_x0), imm);
        if(isPhiPost){
            phi_instr_map[insert_block->getLabelId()].push_back(addiw_instr);
        }else{
            insert_block->push_back(addiw_instr);
        }
    }else{
        // lui          %temp, immHigh20
        // addiw		%result, %temp, immLow12
        auto immLow12 = (imm << 20) >> 20;
        auto immHigh20  = (imm >> 12);
        if(imm < imm12Min){
            immHigh20 = immHigh20 - (immLow12 >= 0);
            immHigh20 = -(immHigh20 ^ 0xFFFFF);
        }else{
            immHigh20 = immHigh20 + (immLow12 < 0);
        }
        auto tempregister = GetNewTempRegister(INT64);
        auto lui_instr = rvconstructor->ConstructUImm(RISCV_LUI, tempregister, immHigh20);
        auto addiw_instr = rvconstructor->ConstructIImm(RISCV_ADDIW, resultRegister, tempregister, immLow12);
        if(isPhiPost){
            phi_instr_map[insert_block->getLabelId()].push_back(lui_instr);
            phi_instr_map[insert_block->getLabelId()].push_back(addiw_instr);
        }else{
            insert_block->push_back(lui_instr);
            insert_block->push_back(addiw_instr);
        }
    }
}

static int double_floatConvert(long long doublebit){
    int origin_E = (doublebit << 1) >> 53;
    int E = (origin_E - ((1 << 10) - 1) + ((1 << 7) - 1)) & EIGHT_BITS;
    int F = (((doublebit << 12) >> 41) + ((1ll << 35) & doublebit)) & TWENTYTHERE_BITS;
    int S = (doublebit >> 63) & 1;
    int floatbit = F | (E << 23) | (S << 31);
    // std::cout << "E 十进制: " << E << " 转为十六进制大写: " 
    //           << std::hex << std::uppercase << E << std::endl;
    // std::cout << "F 十进制: " << F << " 转为十六进制大写: " 
    //           << std::hex << std::uppercase << F << std::endl;
    // std::cout << "S 十进制: " << S << " 转为十六进制大写: " 
    //           << std::hex << std::uppercase << S << std::endl;
    // std::cout << "floatbit 十进制: " << floatbit << " 转为十六进制大写: " 
    //           << std::hex << std::uppercase << floatbit << std::endl;
    return floatbit;
}

void RiscV64Unit::InsertImmFloat32Instruction(Register resultRegister, Operand op, MachineBlock* insert_block, bool isPhiPost){
    auto immop = (ImmF32Operand*)op;
    auto imm = immop->GetFloatByteVal();
    if(imm == 0){
        auto fmvwx_instr = rvconstructor->ConstructR2(RISCV_FMV_W_X, resultRegister, GetPhysicalReg(0));
        if(isPhiPost){
            phi_instr_map[insert_block->getLabelId()].push_back(fmvwx_instr);
        }else{
            insert_block->push_back(fmvwx_instr);
        }
        return;
    }
    auto bit_imm = double_floatConvert(immop->GetFloatByteVal());
    // 64位浮点数转32位
    // std::cout << "十进制: " << imm << " 转为十六进制大写: " 
    //           << std::hex << std::uppercase << imm << std::endl;
    auto bit_high32_high20_imm = bit_imm >> 12;
    auto bit_high32_low12_imm = (bit_imm << 20) >> 20;
    if(!bit_high32_low12_imm){
        // lui			%temp, imm
        // fmv.w.x      %ft0, %temp
        auto tempregister = GetNewTempRegister(INT64);
        auto lui_instr = rvconstructor->ConstructUImm(RISCV_LUI, tempregister, bit_high32_high20_imm);
        auto fmvwx_instr = rvconstructor->ConstructR2(RISCV_FMV_W_X, resultRegister, tempregister);
        
        if(isPhiPost){
            phi_instr_map[insert_block->getLabelId()].push_back(lui_instr);
            phi_instr_map[insert_block->getLabelId()].push_back(fmvwx_instr);
        }else{
            insert_block->push_back(lui_instr);
            insert_block->push_back(fmvwx_instr);
        }
    }else{
        // lui          %temp, immHigh20
        // addiw		%temp2, %temp1, immLow12
        // fmv.w.x      %resultRegister, %temp2
        auto immLow12 = bit_high32_low12_imm;
        auto immHigh20  = bit_high32_high20_imm;
        if(imm < imm12Min){
            immHigh20 = immHigh20 - (immLow12 >= 0);
            immHigh20 = -(immHigh20 ^ 0xFFFFF);
        }else{
            immHigh20 = immHigh20 + (immLow12 < 0);
        }
        auto tempregister1 = GetNewTempRegister(INT64);
        auto tempregister2 = GetNewTempRegister(INT64);
        auto lui_instr = rvconstructor->ConstructUImm(RISCV_LUI, tempregister1, immHigh20);
        auto addiw_instr = rvconstructor->ConstructIImm(RISCV_ADDIW, tempregister2, tempregister1, immLow12);
        auto fmvwx_instr = rvconstructor->ConstructR2(RISCV_FMV_W_X, resultRegister, tempregister2);

        if(isPhiPost){
            phi_instr_map[insert_block->getLabelId()].push_back(lui_instr);
            phi_instr_map[insert_block->getLabelId()].push_back(addiw_instr);
            phi_instr_map[insert_block->getLabelId()].push_back(fmvwx_instr);
        }else{
            insert_block->push_back(lui_instr);
            insert_block->push_back(addiw_instr);
            insert_block->push_back(fmvwx_instr);
        }
    }
}

Register RiscV64Unit::GetI32A(int a_num){
    Register registerop;
	if(a_num >= 0 && a_num <= 7) {
		registerop = GetPhysicalReg(RISCV_a0 + a_num);
	} else {
		registerop = GetPhysicalReg(RISCV_INVALID);
	}
    return registerop;
}

Register RiscV64Unit::GetF32A(int a_num){
    Register registerop;
	if(a_num >= 0 && a_num <= 7) {
		registerop = GetPhysicalReg(RISCV_fa0 + a_num);
	} else {
		registerop = GetPhysicalReg(RISCV_INVALID);
	}
    return registerop;
}

int RiscV64Unit::InsertArgInStack(BasicInstruction::LLVMType type, Operand arg_op, int &arg_off, MachineBlock* insert_block, int &nr_iarg, int &nr_farg) {
	if (type == BasicInstruction::I32 || type == BasicInstruction::PTR) {
		if (nr_iarg < 8) {
		} else {
			if (arg_op->GetOperandType() == BasicOperand::REG) {
				auto arg_regop = (RegOperand *)arg_op;
				auto arg_reg = GetNewRegister(arg_regop->GetRegNo(), INT64);
				if (llvmReg_offset_map.find(arg_regop->GetRegNo()) == llvmReg_offset_map.end()) {
					if(arg_off <= 2047 && arg_off >= -2048) {
						insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
					} else {
						auto imm_reg = GetNewTempRegister(INT64);
						auto offset_mid_reg = GetNewTempRegister(INT64);
						insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
						insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
						insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, arg_reg, offset_mid_reg, 0));
					}
				} else {
					auto sp_offset = llvmReg_offset_map[arg_regop->GetRegNo()];
					auto mid_reg = GetNewTempRegister(INT64);
					auto imm_reg = GetNewTempRegister(INT64);
					auto offset_mid_reg = GetNewTempRegister(INT64);
					if(sp_offset <= 2047 && sp_offset >= -2048) {
						insert_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, mid_reg, GetPhysicalReg(RISCV_sp), sp_offset));
					} else {
						insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, sp_offset));
						insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
					}
					if(sp_offset <= 2047 && sp_offset >= -2048) {
						insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, mid_reg, GetPhysicalReg(RISCV_sp), arg_off));
					} else {
						insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
						insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
						insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, mid_reg, offset_mid_reg, 0));
					}
				}
			} else if (arg_op->GetOperandType() == BasicOperand::IMMI32) {
				auto arg_immop = (ImmI32Operand *)arg_op;
				auto arg_imm_reg = GetNewTempRegister(INT64);
				insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, arg_imm_reg, arg_immop->GetIntImmVal()));
				if(arg_off <= 2047 && arg_off >= -2048) {
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, arg_imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
				} else {
					auto imm_reg = GetNewTempRegister(INT64);
					auto offset_mid_reg = GetNewTempRegister(INT64);
					insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
					insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, arg_imm_reg, offset_mid_reg, 0));
				}
			} else if (arg_op->GetOperandType() == BasicOperand::GLOBAL) {
				auto arg_glbop = (GlobalOperand *)arg_op;
				auto ptr_reg = GetNewTempRegister(INT64);
				auto la_inst = rvconstructor->ConstructULabel(RISCV_LA, ptr_reg, RiscVLabel(arg_glbop->GetName(), true));
				insert_block->push_back(la_inst);
				if(arg_off <= 2047 && arg_off >= -2048) {
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, ptr_reg, GetPhysicalReg(RISCV_sp), arg_off));
				} else {
					auto imm_reg = GetNewTempRegister(INT64);
					auto offset_mid_reg = GetNewTempRegister(INT64);
					insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
					insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, ptr_reg, offset_mid_reg, 0));
				}
			}
			arg_off += 8;
		}
		nr_iarg++;
	} else if (type == BasicInstruction::FLOAT32) { 
		if (nr_farg < 8) {
		} else {
			if (arg_op->GetOperandType() == BasicOperand::REG) {
				auto arg_regop = (RegOperand *)arg_op;
				auto arg_reg = GetNewRegister(arg_regop->GetRegNo(), FLOAT64);
				if(arg_off <= 2047 && arg_off >= -2048) {
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_FSD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
				} else {
					auto imm_reg = GetNewTempRegister(INT64);
					auto offset_mid_reg = GetNewTempRegister(INT64);
					insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
					insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_FSD, arg_reg, offset_mid_reg, 0));
				}
			} else if (arg_op->GetOperandType() == BasicOperand::IMMF32) {
				auto arg_immop = (ImmF32Operand *)arg_op;
				auto arg_imm = arg_immop->GetFloatVal();
				auto float_imm_reg = GetNewTempRegister(INT64);
				// get float_imm args in format i32
				InsertImmI32Instruction(float_imm_reg, arg_immop, insert_block);  
				if(arg_off <= 2047 && arg_off >= -2048) {
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, float_imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
				} else {
					auto imm_reg = GetNewTempRegister(INT64);
					auto offset_mid_reg = GetNewTempRegister(INT64);
					insert_block->push_back(rvconstructor->ConstructUImm(RISCV_LI, imm_reg, arg_off));
					insert_block->push_back(rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
					insert_block->push_back(rvconstructor->ConstructSImm(RISCV_SD, float_imm_reg, offset_mid_reg, 0));
				}
			} else {
				ERROR("Unexpected Operand type");
			}
			arg_off += 8;
		}
		nr_farg++;
	} else {
		ERROR("Unexpected parameter type %d", type);
		return -1;
	}
	return 0;
}

void RiscV64Unit::DomtreeDfs(BasicBlock* ubb, CFG *C){
    if (!ubb || !C || !C->DomTree ) {
        ERROR("Invalid parameters in DomtreeDfs");
        return;
    }
    if(ubb->dfs_id>0){return ;}
    ubb->dfs_id=1;

    auto ubbid = ubb->block_id;
	auto DomTree = (DominatorTree*)C->DomTree;
	auto domtree = DomTree->dom_tree;
	// DomTree->display();
    cur_block = curblockmap[ubbid];

    // std::cerr << "\n[指令选择] 开始处理基本块 B" << ubbid << std::endl;
    assert(!ubb->Instruction_list.empty());
    for (auto instruction : ubb->Instruction_list) {
        // std::cerr << "[指令选择] 处理指令: ";
        // instruction->PrintIR(std::cerr);
        assert(instruction!=nullptr);
        ConvertAndAppend<Instruction>(instruction);
    }

    //assert(!domtree[ubbid].empty());
    if(domtree[ubbid].empty()){return ;}
    
	for (auto vbb : domtree[ubbid]) {
        if(vbb==nullptr){continue;}
		DomtreeDfs(vbb, C);
	}
}


// 模板特化实现
template <> void RiscV64Unit::ConvertAndAppend<Instruction>(Instruction inst) {
    switch (inst->GetOpcode()) {
    case BasicInstruction::LOAD:
        ConvertAndAppend<LoadInstruction *>((LoadInstruction *)inst);
        break;
    case BasicInstruction::STORE:
        ConvertAndAppend<StoreInstruction *>((StoreInstruction *)inst);
        break;
    case BasicInstruction::ADD:
    case BasicInstruction::SUB:
    case BasicInstruction::MUL:
    case BasicInstruction::DIV:
    case BasicInstruction::FADD:
    case BasicInstruction::FSUB:
    case BasicInstruction::FMUL:
    case BasicInstruction::FDIV:
    case BasicInstruction::MOD:
    case BasicInstruction::SHL:
    case BasicInstruction::BITXOR:
        ConvertAndAppend<ArithmeticInstruction *>((ArithmeticInstruction *)inst);
        break;
    case BasicInstruction::ICMP:
        ConvertAndAppend<IcmpInstruction *>((IcmpInstruction *)inst);
        break;
    case BasicInstruction::FCMP:
        ConvertAndAppend<FcmpInstruction *>((FcmpInstruction *)inst);
        break;
    case BasicInstruction::ALLOCA:
        ConvertAndAppend<AllocaInstruction *>((AllocaInstruction *)inst);
        break;
    case BasicInstruction::BR_COND:
        ConvertAndAppend<BrCondInstruction *>((BrCondInstruction *)inst);
        break;
    case BasicInstruction::BR_UNCOND:
        ConvertAndAppend<BrUncondInstruction *>((BrUncondInstruction *)inst);
        break;
    case BasicInstruction::RET:
        ConvertAndAppend<RetInstruction *>((RetInstruction *)inst);
        break;
    case BasicInstruction::ZEXT:
        ConvertAndAppend<ZextInstruction *>((ZextInstruction *)inst);
        break;
    case BasicInstruction::FPTOSI:
        ConvertAndAppend<FptosiInstruction *>((FptosiInstruction *)inst);
        break;
    case BasicInstruction::SITOFP:
        ConvertAndAppend<SitofpInstruction *>((SitofpInstruction *)inst);
        break;
    case BasicInstruction::GETELEMENTPTR:
        ConvertAndAppend<GetElementptrInstruction *>((GetElementptrInstruction *)inst);
        break;
    case BasicInstruction::CALL:
        ConvertAndAppend<CallInstruction *>((CallInstruction *)inst);
        break;
    case BasicInstruction::PHI:
        ConvertAndAppend<PhiInstruction *>((PhiInstruction *)inst);
        break;
	case BasicInstruction::BITCAST:
		ConvertAndAppend<BitCastInstruction *>((BitCastInstruction *)inst);
		break;
    default:
        ERROR("Unknown LLVM IR instruction");
    }
}

/* (1) global var or const var
*  %t1 = load i32* @a ====> lui %1, %hi(a)
* 							lw %2,%lo(a)(%1)  #riscv
*
*  (2) Load a temporary variable from the stack into a register
*  %t1 = load i32* %t2 ====> lw %1, 0(%2) #riscv
*
*  where assume that the value in the %t2 register corresponds to the virtual register %2
*  (3) Load an array element (PTR)
* 	...
*/
template <> void RiscV64Unit::ConvertAndAppend<LoadInstruction*>(LoadInstruction* inst) {
	int op_code = (inst->GetDataType() == BasicInstruction::LLVMType::FLOAT32) ? RISCV_FLW :
				  (inst->GetDataType() == BasicInstruction::LLVMType::I32    ) ? RISCV_LW  :
				  (inst->GetDataType() == BasicInstruction::LLVMType::PTR    ) ? RISCV_LD  : -1 ;

	MachineDataType target_type = (inst->GetDataType() == BasicInstruction::LLVMType::FLOAT32) ? FLOAT64 :
								  (inst->GetDataType() == BasicInstruction::LLVMType::I32    ) ? INT64  :
								  (inst->GetDataType() == BasicInstruction::LLVMType::PTR    ) ? INT64  : INT64;
	if (inst->GetPointer()->GetOperandType() == BasicOperand::REG) {

        auto pointer_op = (RegOperand *)inst->GetPointer();
        auto rd_op = (RegOperand *)inst->GetResult();
		Register rd = GetNewRegister(rd_op->GetRegNo(), target_type);

		if (llvmReg_offset_map.find(pointer_op->GetRegNo()) == llvmReg_offset_map.end()) {
			auto ptr_reg = GetNewRegister(pointer_op->GetRegNo(), INT64);
			
			// lw rd, 0(ptr_reg)
			auto lw_inst = rvconstructor->ConstructIImm(op_code, rd, ptr_reg, 0);
			cur_block->push_back(lw_inst);
		} else { // case 2. pointer_op is a alloca_reg that pointed to stack
			auto stack_offset = llvmReg_offset_map[pointer_op->GetRegNo()];
            // do right here
			// lw rd, stack_offset(sp)
			auto lw_inst = rvconstructor->ConstructIImm(op_code, rd, GetPhysicalReg(RISCV_sp), stack_offset);
			cur_block->push_back(lw_inst);
			((RiscV64Function *)cur_func)->AddAllocaInst(lw_inst);
		}

    } else if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
		Register ptr_reg = GetNewTempRegister(INT64);
        auto global_op = (GlobalOperand *)inst->GetPointer();
        auto rd_op = (RegOperand *)inst->GetResult();
		Register rd = GetNewRegister(rd_op->GetRegNo(), target_type);

#if USE_AUIPC_LW_OPTIMIZATION
		// bug: (.text+0x20c): dangerous relocation: %pcrel_lo missing matching %pcrel_hi
		int local_id = ++local_label_counter;
		cur_block->push_back(rvconstructor->ConstructLocalLabel(local_id));
		auto auipc_inst = rvconstructor->ConstructUImm_pcrel_hi(RISCV_AUIPC, ptr_reg, RiscVLabel(global_op->GetName(), true));
		auto lw_inst = rvconstructor->ConstructIImm_pcrel_lo(op_code, rd, ptr_reg, RiscVLabel(local_id));

		cur_block->push_back(auipc_inst);
		cur_block->push_back(lw_inst);
#else
		auto la_inst = rvconstructor->ConstructULabel(RISCV_LA, ptr_reg, RiscVLabel(global_op->GetName(), true));
		auto lw_inst = rvconstructor->ConstructIImm(op_code, rd, ptr_reg, 0);

		cur_block->push_back(la_inst);
		cur_block->push_back(lw_inst);
#endif
    }
}

// StoreInstruction is same as LoadInstruction
template <> void RiscV64Unit::ConvertAndAppend<StoreInstruction*>(StoreInstruction* inst) {
    // TODO("Implement this if you need");
	Operand value_op = inst->GetValue();
	Register value_reg;

	// solve value (update msg in value_reg)
	if (value_op->GetOperandType() == BasicOperand::IMMI32){
		// create a tmp reg to store imm
		value_reg = GetNewTempRegister(INT64);
		InsertImmI32Instruction(value_reg, (ImmI32Operand *)inst->GetValue(), cur_block);
	} else if (value_op->GetOperandType() == BasicOperand::IMMF32) {
		value_reg = GetNewTempRegister(FLOAT64);
		InsertImmFloat32Instruction(value_reg, (ImmF32Operand *)inst->GetValue(), cur_block);
	} else if (value_op->GetOperandType() == BasicOperand::REG) {
		RegOperand *value_regop = (RegOperand *)inst->GetValue();
		if(inst->GetDataType() == BasicInstruction::LLVMType::I32){
			value_reg = GetNewRegister(value_regop->GetRegNo(), INT64);
		} else if (inst->GetDataType() == BasicInstruction::LLVMType::FLOAT32) {
			value_reg = GetNewRegister(value_regop->GetRegNo(), FLOAT64);
		}
	}

	// solve pointer (the address want to be store)
	int op_code = (inst->GetDataType() == BasicInstruction::LLVMType::FLOAT32) ? RISCV_FSW :
				  (inst->GetDataType() == BasicInstruction::LLVMType::I32) ? RISCV_SW : -1 ;

	if (inst->GetPointer()->GetOperandType() == BasicOperand::REG) {  // local var (store in stack)
		RegOperand* pointer_op = (RegOperand *)inst->GetPointer();
		// case 1. pointer_op is a ptr_reg pointed to address 
		if (llvmReg_offset_map.find(pointer_op->GetRegNo()) == llvmReg_offset_map.end()) {
			auto ptr_reg = GetNewRegister(pointer_op->GetRegNo(), INT64);

			// sw value_reg, 0(ptr_reg)
			auto store_inst = rvconstructor->ConstructSImm(op_code, value_reg, ptr_reg, 0);
			cur_block->push_back(store_inst);
		} else { // case 2. pointer_op is a alloca_reg that pointed to stack
			auto stack_offset = llvmReg_offset_map[pointer_op->GetRegNo()];

			// sw value_reg, stack_offset(sp)
			auto store_inst = rvconstructor->ConstructSImm(op_code, value_reg, GetPhysicalReg(RISCV_sp), stack_offset);
			cur_block->push_back(store_inst);
			((RiscV64Function *)cur_func)->AddAllocaInst(store_inst);
		}
	} else if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {  // global var  (address = hi + lo)
		GlobalOperand* global_op = (GlobalOperand *)inst->GetPointer();

		auto ptr_reg = GetNewTempRegister(INT64);
#if USE_AUIPC_LW_OPTIMIZATION
		int local_id = ++local_label_counter;
		cur_block->push_back(rvconstructor->ConstructLocalLabel(local_id));
		auto auipc_inst = rvconstructor->ConstructUImm_pcrel_hi(RISCV_AUIPC, ptr_reg, RiscVLabel(global_op->GetName(), true));
		cur_block->push_back(auipc_inst);
		auto sw_inst = rvconstructor->ConstructSLabel(op_code, value_reg, ptr_reg, RiscVLabel(local_id));
		cur_block->push_back(sw_inst);
#else
		auto la_inst = rvconstructor->ConstructULabel(RISCV_LA, ptr_reg, RiscVLabel(global_op->GetName(), true));
		auto sw_inst = rvconstructor->ConstructSImm(op_code, value_reg, ptr_reg, 0);

		cur_block->push_back(la_inst);
		cur_block->push_back(sw_inst);
#endif
	}
}

template <> void RiscV64Unit::ConvertAndAppend<ArithmeticInstruction *>(ArithmeticInstruction *inst) {
	bool op1_isreg = (inst->GetOperand1()->GetOperandType() == BasicOperand::REG);
	bool op2_isreg = (inst->GetOperand2()->GetOperandType() == BasicOperand::REG);
	auto *rd_op = (RegOperand *)inst->GetResult();

	auto *reg_op1 = (op1_isreg) ? (RegOperand *)inst->GetOperand1() : nullptr;
	auto *imm_op1 = (op1_isreg) ? nullptr : (ImmI32Operand *)inst->GetOperand1();

	auto *reg_op2 = (op2_isreg) ? (RegOperand *)inst->GetOperand2() : nullptr;
	auto *imm_op2 = (op2_isreg) ? nullptr : (ImmI32Operand *)inst->GetOperand2();

	switch (inst->GetOpcode()) {

		case BasicInstruction::LLVMIROpcode::ADD:
		 	// Imm + Imm 的情况, riscv64 指令集无法解决, 在窥孔优化里消除
			// 所有的 Imm 都需要存储在 Imm_reg 中，因为 addiw 等立即数运算有位宽限制
			if(op1_isreg && op2_isreg){  // reg + reg
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);

				auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, rd, rs, rt);
                cur_block->push_back(addw_instr);
			} else if (op1_isreg && !op2_isreg) { // reg + imm 
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op2, cur_block);

				auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, rd, rs, imm_reg);
				cur_block->push_back(addw_instr);
			} else if (op1_isreg || op2_isreg) {  // imm + reg 
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);
				
				auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, rd, imm_reg, rt);
				cur_block->push_back(addw_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_op1_reg = GetNewTempRegister(INT64);
				Register imm_op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmI32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(addw_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::SUB:
			if(op1_isreg && op2_isreg){  // reg - reg
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);

				auto subw_instr = rvconstructor->ConstructR(RISCV_SUBW, rd, rs, rt);
                cur_block->push_back(subw_instr);
			} else if (op1_isreg && !op2_isreg) { // reg - imm 
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op2, cur_block);

				auto subw_instr = rvconstructor->ConstructR(RISCV_SUBW, rd, rs, imm_reg);
				cur_block->push_back(subw_instr);
			} else if (op1_isreg || op2_isreg) {  // imm - reg 
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);
				
				auto subw_instr = rvconstructor->ConstructR(RISCV_SUBW, rd, imm_reg, rt);
				cur_block->push_back(subw_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_op1_reg = GetNewTempRegister(INT64);
				Register imm_op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmI32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto subw_instr = rvconstructor->ConstructR(RISCV_SUBW, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(subw_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::DIV:  // RISC-V 指令集架构中，没有专门针对 [除以立即数] 的指令
			if(op1_isreg && op2_isreg){  // reg / reg
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);

				auto divw_instr = rvconstructor->ConstructR(RISCV_DIVW, rd, rs, rt);
                cur_block->push_back(divw_instr);
			} else if (op1_isreg && !op2_isreg) { // reg / imm 
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op2, cur_block);

				auto divw_instr = rvconstructor->ConstructR(RISCV_DIVW, rd, rs, imm_reg);
				cur_block->push_back(divw_instr);
			} else if (op1_isreg || op2_isreg) {  // imm / reg 
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);
				
				auto divw_instr = rvconstructor->ConstructR(RISCV_DIVW, rd, imm_reg, rt);
				cur_block->push_back(divw_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_op1_reg = GetNewTempRegister(INT64);
				Register imm_op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmI32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto divw_instr = rvconstructor->ConstructR(RISCV_DIVW, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(divw_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::MUL:  // RISC-V 指令集架构中，没有专门针对 [乘以立即数] 的指令
			if(op1_isreg && op2_isreg){  // reg * reg
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);

				auto mulw_instr = rvconstructor->ConstructR(RISCV_MULW, rd, rs, rt);
                cur_block->push_back(mulw_instr);
			} else if (op1_isreg && !op2_isreg) { // reg * imm 
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op2, cur_block);

				auto mulw_instr = rvconstructor->ConstructR(RISCV_MULW, rd, rs, imm_reg);
				cur_block->push_back(mulw_instr);
			} else if (op1_isreg || op2_isreg) {  // imm * reg 
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);
				
				auto mulw_instr = rvconstructor->ConstructR(RISCV_MULW, rd, imm_reg, rt);
				cur_block->push_back(mulw_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_op1_reg = GetNewTempRegister(INT64);
				Register imm_op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmI32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto mulw_instr = rvconstructor->ConstructR(RISCV_MULW, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(mulw_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::MOD:
			if(op1_isreg && op2_isreg){  // reg % reg
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);

				auto remw_instr = rvconstructor->ConstructR(RISCV_REMW, rd, rs, rt);
                cur_block->push_back(remw_instr);
			} else if (op1_isreg && !op2_isreg) { // reg % imm 
                auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op2, cur_block);

				auto remw_instr = rvconstructor->ConstructR(RISCV_REMW, rd, rs, imm_reg);
				cur_block->push_back(remw_instr);
			} else if (op1_isreg || op2_isreg) {  // imm % reg 
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), INT64);
				
				auto remw_instr = rvconstructor->ConstructR(RISCV_REMW, rd, imm_reg, rt);
				cur_block->push_back(remw_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), INT64);
				Register imm_op1_reg = GetNewTempRegister(INT64);
				Register imm_op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmI32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto remw_instr = rvconstructor->ConstructR(RISCV_REMW, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(remw_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::FADD:
			if(op1_isreg && op2_isreg){  // reg + reg (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);

				auto fadd_d_instr = rvconstructor->ConstructR(RISCV_FADD_S, rd, rs, rt);
                cur_block->push_back(fadd_d_instr);
			} else if (op1_isreg && !op2_isreg) { // reg + imm (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op2, cur_block);

				auto fadd_d_instr = rvconstructor->ConstructR(RISCV_FADD_S, rd, rs, imm_reg);
				cur_block->push_back(fadd_d_instr);
			} else if (op1_isreg || op2_isreg) {  // imm + reg (double)
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);
				
				auto fadd_d_instr = rvconstructor->ConstructR(RISCV_FADD_S, rd, imm_reg, rt);
				cur_block->push_back(fadd_d_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_op1_reg = GetNewTempRegister(FLOAT64);
				Register imm_op2_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmFloat32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto fadd_d_instr = rvconstructor->ConstructR(RISCV_FADD_S, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(fadd_d_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::FSUB:
			if(op1_isreg && op2_isreg){  // reg + reg (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);

				auto fsub_d_instr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, rs, rt);
                cur_block->push_back(fsub_d_instr);
			} else if (op1_isreg && !op2_isreg) { // reg + imm (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op2, cur_block);

				auto fsub_d_instr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, rs, imm_reg);
				cur_block->push_back(fsub_d_instr);
			} else if (op1_isreg || op2_isreg) {  // imm - reg (double)
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);
				
				auto fsub_d_instr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, imm_reg, rt);
				cur_block->push_back(fsub_d_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_op1_reg = GetNewTempRegister(FLOAT64);
				Register imm_op2_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmFloat32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto fsub_d_instr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(fsub_d_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::FMUL:
			if(op1_isreg && op2_isreg){  // reg * reg (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);

				auto fmul_d_instr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, rs, rt);
                cur_block->push_back(fmul_d_instr);
			} else if (op1_isreg && !op2_isreg) { // reg * imm (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op2, cur_block);

				auto fmul_d_instr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, rs, imm_reg);
				cur_block->push_back(fmul_d_instr);
			} else if (op1_isreg || op2_isreg) {  // imm * reg (double)
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);
				
				auto fmul_d_instr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, imm_reg, rt);
				cur_block->push_back(fmul_d_instr);
			} else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_op1_reg = GetNewTempRegister(FLOAT64);
				Register imm_op2_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmFloat32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto fmul_d_instr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(fmul_d_instr);	
			}
			break;  

		case BasicInstruction::LLVMIROpcode::FDIV:
			if(op1_isreg && op2_isreg){  // reg / reg (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
                auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);

				auto fdiv_d_instr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, rs, rt);
                cur_block->push_back(fdiv_d_instr);
			} else if (op1_isreg && !op2_isreg) { // reg / imm (double)
                auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
                auto rs = GetNewRegister(reg_op1->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op2, cur_block);

				auto fdiv_d_instr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, rs, imm_reg);
				cur_block->push_back(fdiv_d_instr);
			} else if (op1_isreg || op2_isreg) {  // imm / reg (double)
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_reg, imm_op1, cur_block);
				auto rt = GetNewRegister(reg_op2->GetRegNo(), FLOAT64);
				
				auto fdiv_d_instr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, imm_reg, rt);
				cur_block->push_back(fdiv_d_instr);
			}  else {
				auto rd = GetNewRegister(rd_op->GetRegNo(), FLOAT64);
				Register imm_op1_reg = GetNewTempRegister(FLOAT64);
				Register imm_op2_reg = GetNewTempRegister(FLOAT64);
				InsertImmFloat32Instruction(imm_op1_reg, imm_op1, cur_block);
				InsertImmFloat32Instruction(imm_op2_reg, imm_op2, cur_block);
				
				auto fdiv_d_instr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, imm_op1_reg, imm_op2_reg);
				cur_block->push_back(fdiv_d_instr);	
			}
			break;  

		default:
			ERROR("Error: undefined opcode.");
	}
}
// inst1: %t2 = icmp ne i32 %t1, 0	   	     ====> bne %1, x0, .L3		
// inst2: br i1 %t2, label %B3, label %B4		   j .L4 			#riscv
// Tip: inst1 处的汇编代码生成 .L3 标签，但是这个标签只有指令选择执行到 inst2 的时候才能获取到
// 故需要预存 inst1, 通过关联寄存器 %t2, 使得 inst2 可以回溯到 inst1
template <> void RiscV64Unit::ConvertAndAppend<IcmpInstruction *>(IcmpInstruction *inst) {
    // TODO("Implement this if you need");
    auto res_op = (RegOperand *)inst->GetResult();
    auto res_reg = GetNewRegister(res_op->GetRegNo(), INT64);
    resReg_cmpInst_map[res_reg] = inst;
}

template <> void RiscV64Unit::ConvertAndAppend<FcmpInstruction *>(FcmpInstruction *inst) {
    // TODO("Implement this if you need");
	auto res_op = (RegOperand *)inst->GetResult();
    auto res_reg = GetNewRegister(res_op->GetRegNo(), INT64);
    resReg_cmpInst_map[res_reg] = inst;
}

// globalVar is no need to alloca and not in basicblock,
// globalVar is printed at riscv64_printasm.cc (GlobalVarDefineInstruction)
// localVar is need to alloca a space in stack, a llvm_reg is corresponding to a stack_offset
template <> void RiscV64Unit::ConvertAndAppend<AllocaInstruction *>(AllocaInstruction *inst) {
    // TODO("Implement this if you need");
	auto llvm_reg_op = (RegOperand *)inst->GetResult();

	// clac allocSz
	std::vector<int> dims = inst->GetDims();
	int allcoSz = 4;    // 4 bytes
    for (int dim : dims) allcoSz *= dim;
	
	// update llvmReg_offset_map
	// std::cerr << "offset of reg %r" << llvm_reg_op->GetRegNo() << " is " << cur_offset << std::endl;
	// std::cerr << "offset of reg %r" << llvm_reg_op->GetRegNo() << " is " << cur_offset + bias << std::endl;
    llvmReg_offset_map[llvm_reg_op->GetRegNo()] = cur_offset;
    // llvmReg_offset_map[llvm_reg_op->GetRegNo()] = cur_offset + bias;
    cur_offset += allcoSz;
}

// inst1: %t2 = icmp ne i32 %t1, 0	   	     ====> bne %1, x0, .L3		
// inst2: br i1 %t2, label %B3, label %B4		   j .L4 			#riscv
// Tip: inst1 处的汇编代码生成 .L3 标签，但是这个标签只有指令选择执行到 inst2 的时候才能获取到
// 故需要预存 inst1, 通过关联寄存器 %t2, 使得 inst2 可以回溯到 inst1
template <> void RiscV64Unit::ConvertAndAppend<BrCondInstruction *>(BrCondInstruction *inst) {
    // TODO("Implement this if you need");
    terminal_instr_map[cur_block->getLabelId()] = cur_block->GetInsList().back();
	// special case 
	// br i1 imm, label %B3, label %B4    ====> j .L3   (imm == 1)
	//											j .L4   (imm == 0)  
	// 此处是没有 Icmp 或者 Fcmp的过程的, resReg_cmpInst_map[res_reg] = null_ptr
	if (inst->GetCond()->GetOperandType() == BasicOperand::IMMI32) {
		auto cond_op = (ImmI32Operand *)inst->GetCond();  
		auto imm = cond_op->GetIntImmVal();

		auto true_label = (LabelOperand *)inst->GetTrueLabel();
		auto false_label = (LabelOperand *)inst->GetFalseLabel();

		// j .L3
		if (imm) {
			auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(true_label->GetLabelNo(), 0));
			cur_block->push_back(jal_inst);
		} else {
			auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo(), 0));
			cur_block->push_back(jal_inst);
		}
		return;   
	} else if (inst->GetCond()->GetOperandType() == BasicOperand::IMMF32) {
		auto cond_op = (ImmF32Operand *)inst->GetCond();  
		auto imm = cond_op->GetFloatVal();

		auto true_label = (LabelOperand *)inst->GetTrueLabel();
		auto false_label = (LabelOperand *)inst->GetFalseLabel();

		// j .L3
		if (imm) {
			auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(true_label->GetLabelNo(), 0));
			cur_block->push_back(jal_inst);
		} else {
			auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo(), 0));
			cur_block->push_back(jal_inst);
		}
		return;   
	}

    auto cond_op = (RegOperand *)inst->GetCond();  
    Register res_reg, cond_reg;
	res_reg = cond_reg = GetNewRegister(cond_op->GetRegNo(), INT64);  // res_reg == cond_reg
    auto it = resReg_cmpInst_map.find(res_reg);
	if (it == resReg_cmpInst_map.end()) {
		std::cout << "cmp_inst not found!" << std::endl;
		return;
	} 
	auto cmp_inst = resReg_cmpInst_map[res_reg]; 

    Register op1_reg, op2_reg;
    if (cmp_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::ICMP) {
        auto icmp_inst = (IcmpInstruction *)cmp_inst;
		// solve op1
        if (icmp_inst->GetOp1()->GetOperandType() == BasicOperand::REG) {
			auto op1 = (RegOperand *)icmp_inst->GetOp1();
            op1_reg = GetNewRegister(op1->GetRegNo(), INT64);
        } else if (icmp_inst->GetOp1()->GetOperandType() == BasicOperand::IMMI32) {
			auto op1 = (ImmI32Operand *)icmp_inst->GetOp1();
			op1_reg = GetNewTempRegister(INT64);
			InsertImmI32Instruction(op1_reg, op1, cur_block);
		}
		// solve op2
        if (icmp_inst->GetOp2()->GetOperandType() == BasicOperand::REG) {
			auto op2 = (RegOperand *)icmp_inst->GetOp2();
            op2_reg = GetNewRegister(op2->GetRegNo(), INT64);
        } else if (icmp_inst->GetOp2()->GetOperandType() == BasicOperand::IMMI32) {
			auto op2 = (ImmI32Operand *)icmp_inst->GetOp2();
			op2_reg = GetNewTempRegister(INT64);
			InsertImmI32Instruction(op2_reg, op2, cur_block);
		}

		RISCV_INST opcode = IcmpSelect_map[icmp_inst->GetCond()];
		auto true_label = (LabelOperand *)inst->GetTrueLabel();
		auto false_label = (LabelOperand *)inst->GetFalseLabel();

		// bne %1, x0, .L3
		auto br_inst = rvconstructor->ConstructBLabel(opcode, op1_reg, op2_reg, RiscVLabel(true_label->GetLabelNo(), 0));
		cur_block->push_back(br_inst);
		// j .L4
		auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo(), 0));
		cur_block->push_back(jal_inst);
	}

	// 对于浮点比较，需要先获取比较结果(set_inst), 再借助形如inst1、inst2的指令进行跳转
	else if (cmp_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::FCMP) {
        auto fcmp_inst = (FcmpInstruction *)cmp_inst;
		// solve op1
        if (fcmp_inst->GetOp1()->GetOperandType() == BasicOperand::REG) {
			auto op1 = (RegOperand *)fcmp_inst->GetOp1();
            op1_reg = GetNewRegister(op1->GetRegNo(), FLOAT64);
        } else if (fcmp_inst->GetOp1()->GetOperandType() == BasicOperand::IMMF32) {
			auto op1 = (ImmF32Operand *)fcmp_inst->GetOp1();
			op1_reg = GetNewTempRegister(FLOAT64);
			InsertImmFloat32Instruction(op1_reg, op1, cur_block);
		}
		// solve op2
        if (fcmp_inst->GetOp2()->GetOperandType() == BasicOperand::REG) {
			auto op2 = (RegOperand *)fcmp_inst->GetOp2();
            op2_reg = GetNewRegister(op2->GetRegNo(), FLOAT64);
        } else if (fcmp_inst->GetOp2()->GetOperandType() == BasicOperand::IMMF32) {
			auto op2 = (ImmF32Operand *)fcmp_inst->GetOp2();
			op2_reg = GetNewTempRegister(FLOAT64);
			InsertImmFloat32Instruction(op2_reg, op2, cur_block);
		}
		// solve res_reg
		if (FcmpSelect_map.find(fcmp_inst->GetCond()) != FcmpSelect_map.end()){
			RISCV_INST opcode = FcmpSelect_map[fcmp_inst->GetCond()];
			RISCV_INST brcode = Not_map[fcmp_inst->GetCond()];
			auto true_label = (LabelOperand *)inst->GetTrueLabel();
			auto false_label = (LabelOperand *)inst->GetFalseLabel();

			// feq.s res_reg, op1, op2 
			auto set_inst = rvconstructor->ConstructR(opcode, res_reg, op1_reg, op2_reg);
			cur_block->push_back(set_inst);
			// beq(bne) res_reg, x0, ture_label
			auto br_ins = rvconstructor->ConstructBLabel(brcode, res_reg, GetPhysicalReg(RISCV_x0), RiscVLabel(true_label->GetLabelNo(), 0));
			cur_block->push_back(br_ins);
			// j false_lable
			auto jal_inst = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo(), 0));
			cur_block->push_back(jal_inst);

		} else std::cerr << "Error: this fcmp_inst is not supported" << std::endl;
	}
    block_terminal_type[inst->GetBlockID()] = 1;
}

template <> void RiscV64Unit::ConvertAndAppend<BrUncondInstruction *>(BrUncondInstruction *inst) {
    terminal_instr_map[cur_block->getLabelId()] = cur_block->GetInsList().back();
    auto dest_label = RiscVLabel(((LabelOperand *)inst->GetDestLabel())->GetLabelNo(), 0);
    auto jal_instr = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), dest_label);

    cur_block->push_back(jal_instr);
    block_terminal_type[inst->GetBlockID()] = 0;
}

template <> void RiscV64Unit::ConvertAndAppend<CallInstruction *>(CallInstruction *inst) {
    // 如果函数的参数个数不超过 8 个, 则依次将参数存储在寄存器 a0/fa0 到 a7/fa7 中
    // 否则, 将超出部分放入栈(临时寄存器)
    // 函数返回值放在寄存器 a0/fa0 上
    auto paramlist = inst->GetParameterList();
    int now_int_num = 0;
    int now_float_num = 0;

	// 调用 llvm.memset.p0.i32，将数组的前 10 个字节设置为 42
    // call void @llvm.memset.p0.i32(ptr %ptr, i8 42, i32 10, i1 0)TODO
	// bug : 需要注意的是memset往往是给局部变量初始化，所以如果有参数溢出，需要在AddParameterSize函数添加偏移
	// 如果采用insertingi32函数，可以确保加载的立即数范围超过12位，但是很难使用AddParameterSize函数添加偏移
	// 解决方案：改用li指令，不需要再考虑4095的立即数限制，伪指令自动转换解析
    if (inst->GetFunctionName() == std::string("llvm.memset.p0.i32")) {
        // slove parameter 0
		auto ptrreg_op = (RegOperand *)inst->GetParameterList()[0].second;
		int ptrreg_no = ptrreg_op->GetRegNo();

		if (llvm2rv_regmap.find(ptrreg_no) != llvm2rv_regmap.end()) { // 不一定memset数组的初始位置 
			// std::cerr << ptrreg_no << std::endl;
			Register ptr_virReg = llvm2rv_regmap[ptrreg_no];
			auto assign_a0_inst = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_x0), ptr_virReg);
			cur_block->push_back(assign_a0_inst);
		} else {  // 数组的初始位置 sp + offset
			auto offset = llvmReg_offset_map[ptrreg_no];
            // auto offset_reg = GetNewTempRegister(INT64);
            // InsertImmI32Instruction(offset_reg, new ImmI32Operand(offset), cur_block);
			// auto assign_a0_inst = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_sp), offset_reg);

			// auto assign_a0_inst =rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_sp), offset);
			// cur_block->push_back(assign_a0_inst);
			// ((RiscV64Function *)cur_func)->AddAllocaInst(assign_a0_inst);

			auto offset_reg = GetNewTempRegister(INT64);
			auto li_inst = rvconstructor->ConstructUImm(RISCV_LI, offset_reg, offset);
			auto assign_a0_inst = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_sp), offset_reg);
			cur_block->push_back(li_inst);
			cur_block->push_back(assign_a0_inst);
			((RiscV64Function *)cur_func)->AddAllocaInst(li_inst);
		}
        
        // slove parameter 1
		auto imm_op1 = (ImmI32Operand *)(inst->GetParameterList()[1].second);
		InsertImmI32Instruction(GetPhysicalReg(RISCV_a1), imm_op1, cur_block);
	
        // slove paramter 2
        if (inst->GetParameterList()[2].second->GetOperandType() == BasicOperand::IMMI32) {
            auto imm_op2 = (ImmI32Operand *)(inst->GetParameterList()[2].second);
            InsertImmI32Instruction(GetPhysicalReg(RISCV_a2), imm_op2, cur_block);
        } else {
            auto SzReg = (RegOperand *)inst->GetParameterList()[2].second;
            int SzReg_no = SzReg->GetRegNo();
            if (llvm2rv_regmap.find(SzReg_no) != llvm2rv_regmap.end()) {
                Register Sz_virReg = llvm2rv_regmap[SzReg_no];
                auto assign_a2_inst = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a2), GetPhysicalReg(RISCV_x0), Sz_virReg);
                cur_block->push_back(assign_a2_inst);
            } else {
                auto it = llvmReg_offset_map.find(SzReg_no);
                if (it != llvmReg_offset_map.end()) {
                    auto offset = it->second;
                    auto offset_reg = GetNewTempRegister(INT64);
                    auto li_inst = rvconstructor->ConstructUImm(RISCV_LI, offset_reg, offset);
                    auto assign_a2_inst = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a2), GetPhysicalReg(RISCV_sp), offset_reg);
                    cur_block->push_back(li_inst);
                    cur_block->push_back(assign_a2_inst);
                    ((RiscV64Function *)cur_func)->AddAllocaInst(li_inst);
                } else {
                    // fallback: size 未知，置 0 防止未定义行为
                    auto zero_li = rvconstructor->ConstructUImm(RISCV_LI, GetPhysicalReg(RISCV_a2), 0);
                    cur_block->push_back(zero_li);
                }
            }
        }
        
        // call
        cur_block->push_back(rvconstructor->ConstructCall(RISCV_CALL, "memset", 3, 0));
        return;
    }


    for(auto [type, para] : paramlist){
        Register pararegister;
        if(para->GetOperandType() == BasicOperand::IMMI32){
            if(now_int_num < 8){
                pararegister = GetI32A(now_int_num++);
				InsertImmI32Instruction(pararegister, para, cur_block);
            }else{
                // pararegister = GetNewTempRegister(INT64);
				// do nothing but now_int_num++
				now_int_num++;
            }
        }else if(para->GetOperandType() == BasicOperand::IMMF32){
            if(now_float_num < 8){
                pararegister = GetF32A(now_float_num++);
				InsertImmFloat32Instruction(pararegister, para, cur_block);
            }else{
                // pararegister = GetNewTempRegister(FLOAT64);
				// do nothing but now_float_num++
				now_float_num++;
            }
        }else{
            if(type == BasicInstruction::I32 || type == BasicInstruction::PTR){
                if(now_int_num < 8){
					if(para->GetOperandType() == BasicOperand::REG){
						auto reg = (RegOperand*)para;
						auto regno = reg->GetRegNo();
						pararegister = GetI32A(now_int_num);
						if(llvmReg_offset_map.find(regno) != llvmReg_offset_map.end()){
							auto sp_offset = llvmReg_offset_map[regno];
							auto mid_reg = GetNewTempRegister(INT64);
							InsertImmI32Instruction(mid_reg, new ImmI32Operand(sp_offset), cur_block);
							auto addw_instr = rvconstructor->ConstructR(RISCV_ADD, pararegister, mid_reg, GetPhysicalReg(RISCV_sp));
							cur_block->push_back(addw_instr);
						}else{
							std::cerr << "regno: " << regno << " not found in llvmReg_offset_map" << std::endl;
							std::cerr << "now_int_num: " << now_int_num << std::endl;
							auto originpara_register = GetNewRegister(regno, INT64);
							auto addw_instr = rvconstructor->ConstructR(RISCV_ADD, pararegister, originpara_register, GetPhysicalReg(RISCV_x0));
							cur_block->push_back(addw_instr);
						}
					} else if(para->GetOperandType() == BasicOperand::GLOBAL){
						pararegister = GetI32A(now_int_num);
						auto para_global = (GlobalOperand *)para;
						auto la_inst = rvconstructor->ConstructULabel(RISCV_LA, pararegister, RiscVLabel(para_global->GetName(), true));
						cur_block->push_back(la_inst);
					}
					now_int_num++;
                }else{
                    // pararegister = GetNewTempRegister(INT64);
					// do nothing but now_int_num++
					now_int_num++;
                }
            } else if(type == BasicInstruction::FLOAT32){
                if(now_float_num < 8){
					if(para->GetOperandType() == BasicOperand::REG){
						auto reg = (RegOperand*)para;
						auto regno = reg->GetRegNo();
						auto originpara_register = GetNewRegister(regno, FLOAT64);
						pararegister = GetF32A(now_float_num++);
						auto fmvwx_instr = rvconstructor->ConstructR2(RISCV_FMV_S, pararegister, originpara_register);
						cur_block->push_back(fmvwx_instr);
					}
                }else{
                    // pararegister = GetNewTempRegister(FLOAT64);
					// do nothing but now_float_num++
					now_float_num++;
                }
            }
        }
    }

	// solve nr_para more than 8
	/* ---------------------------gobolt 测试程序-----------------------------------
	int foo(int arg1, int arg2,float arg3,int arg4,int arg5,float arg6,
			int arg7,int arg8,int arg9,int arg10, int arg11, int arg12)
	{
		return arg1 + arg2 + arg3 + arg4 + arg5 + arg6 
		     + arg7 + arg8 + arg9 + arg10 + arg11 + arg12;
	}

	int main(){
		foo(1, 2, 3.3, 4, 5, 6.6, 7, 8, 9, 10, 11, 12);
		return 0;
	} 
	---------------------------------------------------------------------------------*/

	/* ---------------------------caller 相关汇编截取 ----------------------------------
        li      a5,12
        sd      a5,8(sp)
        li      a5,11
        sd      a5,0(sp)
        li      a7,10
        li      a6,9
        li      a5,8
        li      a4,7
        fmv.s   fa1,fa4
        li      a3,5
        li      a2,4
        fmv.s   fa0,fa5
        li      a1,2
        li      a0,1
        call    foo(int, int, float, int, int, float, int, int, int, int, int, int)
		调用约定注意点:
	    1. 实际能依靠寄存器传输的最大参数个数为 16 个 (a0-a7, fa0-fa7, 浮点整数各自传输)
		2. 0(sp)存储第一个溢出的参数, 8(sp)存储第二个...
	---------------------------------------------------------------------------------*/


	/* ---------------------------callee 相关汇编截取 ----------------------------------
		; 功能为累加, 逻辑简单直观
		; return arg1 + arg2 + arg3 + arg4 + ... + arg9 + arg10 + arg11 + arg12;
		
	    lw      a5,-20(s0)   ; arg1  a0 (reg)
        mv      a4,a5
        lw      a5,-24(s0)   ; arg2  a1 (reg)
        addw    a5,a4,a5
        sext.w  a5,a5
        fcvt.s.w        fa4,a5
        flw     fa5,-28(s0)  ; arg3  fa0 (reg)
        fadd.s  fa4,fa4,fa5
        lw      a5,-32(s0)   ; arg4  a2 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,-36(s0)   ; arg5  a3 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        flw     fa5,-40(s0)  ; arg6  fa1 (reg)
        fadd.s  fa4,fa4,fa5
        lw      a5,-44(s0)   ; arg7  a4 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,-48(s0)   ; arg8  a5 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,-52(s0)   ; arg9  a6 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,-56(s0)   ; arg10 a7 (reg)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,0(s0)     ; arg11 (stack)
        fcvt.s.w        fa5,a5
        fadd.s  fa4,fa4,fa5
        lw      a5,8(s0)     ; arg12 (stack)
        fcvt.s.w        fa5,a5
        fadd.s  fa5,fa4,fa5
        fcvt.w.s a5,fa5,rtz
        sext.w  a5,a5
		调用约定注意点:
	    1. 参数传入之后作为临时变量, 预先存储在栈中 (lowerframe)
		2. 溢出的参数需要通过栈底s0, 也就是原先的栈顶来查找, 如arg11, arg12 
		   (lowerstack 也许需要存储栈底fp/s0)
	---------------------------------------------------------------------------------*/
	
	int nr_stkpara = std::max(0, now_int_num - 8) + std::max(0, now_float_num - 8);
	// std::cerr << now_int_num << " " << now_float_num << std::endl;

	if (nr_stkpara != 0) {
		now_int_num = now_float_num = 0;
        int arg_off = 0;
        for (auto [type, arg_op] : inst->GetParameterList()) {
			Assert(InsertArgInStack(type, arg_op, arg_off, cur_block, now_int_num, now_float_num) == 0);
        }
    }
	// 开辟参数空间
	cur_func->UpdateParaSize(nr_stkpara * 8);
	// std::cerr << "UpdateParaSize to " << nr_stkpara * 8 << std::endl;

    auto call_instr = rvconstructor->ConstructCall(RISCV_CALL, 
												   inst->GetFunctionName(), 
												   std::min(8, now_int_num), 
												   std::min(8, now_float_num));
    cur_block->push_back(call_instr);

	// solve return value
    if(inst->GetResult() != nullptr){
        auto resultop = inst->GetResult();
        auto resultreg = (RegOperand*)resultop;
        auto resultreno = resultreg->GetRegNo();
        Register resultregister;
        if(inst->GetRetType() == BasicInstruction::I32 || inst->GetRetType() == BasicInstruction::PTR){
            resultregister = GetNewRegister(resultreno, INT64);
            auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, resultregister, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_x0));
            cur_block->push_back(addw_instr);
        }else{
            resultregister = GetNewRegister(resultreno, FLOAT64);
            // std::cerr<<resultregister.get_reg_no()<<'\n';
            auto fmvwx_instr = rvconstructor->ConstructR2(RISCV_FMV_S, resultregister, GetPhysicalReg(RISCV_fa0));
            cur_block->push_back(fmvwx_instr);
        }
    }

}

template <> void RiscV64Unit::ConvertAndAppend<RetInstruction *>(RetInstruction *inst) {
    terminal_instr_map[cur_block->getLabelId()] = cur_block->GetInsList().back();
	// a0 寄存器设置为返回值
    if (inst->GetRetVal() != NULL) {
        if (inst->GetRetVal()->GetOperandType() == BasicOperand::IMMI32) {  		   // return imm i32
            InsertImmI32Instruction(GetPhysicalReg(RISCV_a0), inst->GetRetVal(), cur_block); 
        } else if (inst->GetRetVal()->GetOperandType() == BasicOperand::IMMF32) {   // return imm f32
            InsertImmFloat32Instruction(GetPhysicalReg(RISCV_fa0), inst->GetRetVal(), cur_block);
        } else if (inst->GetRetVal()->GetOperandType() == BasicOperand::REG) {
            if (inst->GetType() == BasicInstruction::LLVMType::I32) { 			   // return reg i32
                auto retreg_val = (RegOperand *)inst->GetRetVal();
                auto reg = GetNewRegister(retreg_val->GetRegNo(), INT64);

                auto retcopy_instr = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_a0), reg, GetPhysicalReg(RISCV_x0));
                cur_block->push_back(retcopy_instr);
            } else if (inst->GetType() == BasicInstruction::LLVMType::FLOAT32) {	   // return reg f32
                auto retreg_val = (RegOperand *)inst->GetRetVal();
                auto reg = GetNewRegister(retreg_val->GetRegNo(), FLOAT64);
                auto retcopy_instr = rvconstructor->ConstructR2(RISCV_FMV_S, GetPhysicalReg(RISCV_fa0), reg);
                cur_block->push_back(retcopy_instr);
            }
        }
    }

    auto ret_instr = rvconstructor->ConstructIImm(RISCV_JALR, GetPhysicalReg(RISCV_x0), GetPhysicalReg(RISCV_ra), 0);
    if (inst->GetType() == BasicInstruction::I32) {
        ret_instr->setRetType(1);
    } else if (inst->GetType() == BasicInstruction::FLOAT32) {
        ret_instr->setRetType(2);
    } else {
        ret_instr->setRetType(0);
    }
    cur_block->push_back(ret_instr);
    block_terminal_type[inst->GetBlockID()] = 0;
}
/* (1) type transformer imm to reg
 *  %t1 = fptosi float 3.14 to i32  ===> LI %temp, 3.14  # li imm should smaller than 2^12 (InsertImmFloat32Instruction)
 * 										 FCVT.W.S %1, %temp  
 * 
 * (2) type transformer reg to reg
 *  %t1 = sitofp i32 %t2 to float   ===> FCVT.S.W %1, %2  
*/
template <> void RiscV64Unit::ConvertAndAppend<FptosiInstruction *>(FptosiInstruction *inst) {
    // TODO("Implement this if you need");
    auto dst_op = (RegOperand *)inst->GetResult();
    if (inst->GetSrc()->GetOperandType() == BasicOperand::REG) {
        auto src_op = (RegOperand *)inst->GetSrc();
		Register src_reg = GetNewRegister(src_op->GetRegNo(), FLOAT64);
		Register dst_reg = GetNewRegister(dst_op->GetRegNo(), INT64);

		auto fcvt_w_s = rvconstructor->ConstructR2(RISCV_FCVT_W_S, dst_reg, src_reg);
        cur_block->push_back(fcvt_w_s);
    } else if (inst->GetSrc()->GetOperandType() == BasicOperand::IMMF32) {
        auto src_op = (ImmF32Operand *)inst->GetSrc();
		Register src_reg = GetNewTempRegister(FLOAT64);
		InsertImmFloat32Instruction(src_reg, src_op, cur_block);
		Register dst_reg = GetNewRegister(dst_op->GetRegNo(), INT64);

		auto fcvt_w_s = rvconstructor->ConstructR2(RISCV_FCVT_W_S, dst_reg, src_reg);
        cur_block->push_back(fcvt_w_s);
	}
}

// SitofpInstruction is same as FptosiInstruction.
template <> void RiscV64Unit::ConvertAndAppend<SitofpInstruction *>(SitofpInstruction *inst) {
    // TODO("Implement this if you need");
	auto dst_op = (RegOperand *)inst->GetResult();
    if (inst->GetSrc()->GetOperandType() == BasicOperand::REG) {
        auto src_op = (RegOperand *)inst->GetSrc();
		Register src_reg = GetNewRegister(src_op->GetRegNo(), INT64);
		Register dst_reg = GetNewRegister(dst_op->GetRegNo(), FLOAT64);

		auto fcvt_s_w = rvconstructor->ConstructR2(RISCV_FCVT_S_W, dst_reg, src_reg);
        cur_block->push_back(fcvt_s_w);
    } else if (inst->GetSrc()->GetOperandType() == BasicOperand::IMMI32) {
        auto src_op = (ImmI32Operand *)inst->GetSrc();
		Register src_reg = GetNewTempRegister(INT64);
		InsertImmI32Instruction(src_reg, src_op, cur_block);
		Register dst_reg = GetNewRegister(dst_op->GetRegNo(), FLOAT64);
		auto fcvt_s_w = rvconstructor->ConstructR2(RISCV_FCVT_S_W, dst_reg, src_reg);
        cur_block->push_back(fcvt_s_w);
	}
}


/* !a 特殊情况 （icmp / fcmp 不一定只和 cond br 绑定，也有可能和符号扩展绑定）
 * inst1: %r3 = icmp eq i32 %r2,0 	  ===>    sltu  %r3, zero, %r2  # 如果 %r2 != 0，%r3 = 1；否则 %r3 = 0   负数也有效
 *                                            xori  %r3, %r3, 1     # 对 %r3 取反：如果 %r2 == 0，%r3 = 1；否则 %r3 = 0 	  
 * inst2: zext i1 %r3 to i32		
 * 在第二条指令进行选择时，第二条指令负责生成第一条指令的riscv形式 (第二条指令无需翻译)
 * */
template <> void RiscV64Unit::ConvertAndAppend<ZextInstruction *>(ZextInstruction *inst) {

	// special case : %r13 = zext i1 0 to i32
	if(inst->GetSrc()->GetOperandType() == BasicOperand::IMMI32) {
		auto srcop = (ImmI32Operand *)inst->GetSrc();
		auto res_reg = GetNewTempRegister(INT64);
        llvm2rv_regmap[((RegOperand*)inst->GetResult())->GetRegNo()] = res_reg;
		InsertImmI32Instruction(res_reg, srcop, cur_block);
		return;
	} else if(inst->GetSrc()->GetOperandType() == BasicOperand::IMMF32){
		auto srcop = (ImmF32Operand *)inst->GetSrc();
		auto res_reg = GetNewTempRegister(FLOAT64);
        llvm2rv_regmap[((RegOperand*)inst->GetResult())->GetRegNo()] = res_reg;
		InsertImmFloat32Instruction(res_reg, srcop, cur_block);
		return;
	}
	/*
	 * %cmp1 = icmp eq i32 %x, %y    ->    subw t0, x1, x2      
	 * 								       sltiu t1, t0, 1      						
	 * %cmp2 = icmp ne i32 %x, %y    ->    subw t0, x1, x2      
	 * 								       sltu t1, zero, t0      
	 * %cmp3 = icmp ugt i32 %x, %y   ->   
	 * %cmp4 = icmp uge i32 %x, %y   ->    
	 * %cmp5 = icmp ult i32 %x, %y   ->   
	 * %cmp6 = icmp ule i32 %x, %y   ->    
	 * %cmp7 = icmp sgt i32 %x, %y   ->    slt t0, x2, x1 
	 * %cmp8 = icmp sge i32 %x, %y   ->    slt t0, x1, x2
	 *  								   xori t1, t0, 1
	 * %cmp9 = icmp slt i32 %x, %y   ->    slt t0, x1, x2
	 * %cmp10 = icmp sle i32 %x, %y  ->    slt t0, x2, x1
	 * 									   xori t1, t0, 1
	*/
	auto srcop = inst->GetSrc();
	auto srcreg = (RegOperand*)srcop;
	auto srcregno = srcreg->GetRegNo();
	auto srcregister = GetNewRegister(srcregno, INT64);

	auto resultop = inst->GetResult();
	auto resultreg = (RegOperand*)resultop;
	auto resultregno = resultreg->GetRegNo();
	auto resultregister = GetNewRegister(resultregno, INT64);
	

	auto cmp_inst = resReg_cmpInst_map[srcregister];
	Register op1_reg, op2_reg;
	auto tempregister = GetNewTempRegister(INT64);


    if(inst->GetFromType() == BasicInstruction::I1 && inst->GetToType() == BasicInstruction::I32){
		if (cmp_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::ICMP) {
			auto icmp_inst = (IcmpInstruction *)cmp_inst;
			auto cond = icmp_inst->GetCond();
            // std::cerr<<icmp_inst->GetOp1()->GetFullName()<<" "<<icmp_inst->GetOp2()->GetFullName()<<'\n';
			// solve op1
			if (icmp_inst->GetOp1()->GetOperandType() == BasicOperand::REG) {
				auto op1 = (RegOperand *)icmp_inst->GetOp1();
				op1_reg = GetNewRegister(op1->GetRegNo(), INT64);
                // inst->PrintIR(std::cerr);
			} else if (icmp_inst->GetOp1()->GetOperandType() == BasicOperand::IMMI32) {
				auto op1 = (ImmI32Operand *)icmp_inst->GetOp1();
				op1_reg = GetNewTempRegister(INT64);
                
				InsertImmI32Instruction(op1_reg, op1, cur_block);
			}
			// solve op2
			if (icmp_inst->GetOp2()->GetOperandType() == BasicOperand::REG) {
				auto op2 = (RegOperand *)icmp_inst->GetOp2();
				op2_reg = GetNewRegister(op2->GetRegNo(), INT64);
			} else if (icmp_inst->GetOp2()->GetOperandType() == BasicOperand::IMMI32) {
				auto op2 = (ImmI32Operand *)icmp_inst->GetOp2();
				op2_reg = GetNewTempRegister(INT64);
				InsertImmI32Instruction(op2_reg, op2, cur_block);
			}
			// gen code by cond
			switch (cond)
			{
				case BasicInstruction::IcmpCond::eq: 
				{
					auto subw_inst = rvconstructor->ConstructR(RISCV_SUBW, srcregister, op1_reg, op2_reg);
                    // std::cerr<<"eq = "<<srcregister.get_reg_no()<<'\n';
					auto sltu_inst = rvconstructor->ConstructIImm(RISCV_SLTIU, resultregister, srcregister, 1);
					cur_block->push_back(subw_inst);
					cur_block->push_back(sltu_inst);
					break;
				}
				case BasicInstruction::IcmpCond::ne:
				{
					auto subw_inst = rvconstructor->ConstructR(RISCV_SUBW, srcregister, op1_reg, op2_reg);
					auto sltu_inst = rvconstructor->ConstructR(RISCV_SLTU, resultregister, GetPhysicalReg(RISCV_x0), srcregister);
					cur_block->push_back(subw_inst);
					cur_block->push_back(sltu_inst);
					break;
				}
				case BasicInstruction::IcmpCond::sgt:
				{
					auto sltu_inst = rvconstructor->ConstructR(RISCV_SLT, resultregister, op2_reg, op1_reg);
					cur_block->push_back(sltu_inst);
					break;
				}
				case BasicInstruction::IcmpCond::sge:
				{
					auto slt_inst = rvconstructor->ConstructR(RISCV_SLT, srcregister, op1_reg, op2_reg);
					auto xori_inst = rvconstructor->ConstructIImm(RISCV_XORI, resultregister, srcregister, 1);
					cur_block->push_back(slt_inst);
					cur_block->push_back(xori_inst);
					break;
				}
				case BasicInstruction::IcmpCond::slt:
				{
					auto slt_inst = rvconstructor->ConstructR(RISCV_SLT, resultregister, op1_reg, op2_reg);
					cur_block->push_back(slt_inst);
					break;
				}
				case BasicInstruction::IcmpCond::sle:
				{
					auto slt_inst = rvconstructor->ConstructR(RISCV_SLT, srcregister, op2_reg, op1_reg);
					auto xori_inst = rvconstructor->ConstructIImm(RISCV_XORI, resultregister, srcregister, 1);
					cur_block->push_back(slt_inst);
					cur_block->push_back(xori_inst);
					break;
				}
				default:
				{
					std::cerr << "this icmpCond is not support at Zext." << std::endl;
					break;
				}
			}
		}

    }else{
        assert("Wrong Type Converse in ZextInstruction");
    }
}

template <> void RiscV64Unit::ConvertAndAppend<GetElementptrInstruction *>(GetElementptrInstruction *inst) {
	
    auto base_op = inst->GetPtrVal();
    // 1. 计算getelementptr的地址偏移
    auto dims = inst->GetDims();
    auto indexes = inst->GetIndexes();
    auto siz = indexes.size();
    int base = 4;
    int dimmax_base = 1;
    for(size_t i = 0; i < dims.size(); ++i){
		// std::cerr << dims[i] << std::endl;
        dimmax_base *= dims[i];
    }
    // std::cerr<<dims.size()<<'\n';
    Register last_register = GetPhysicalReg(RISCV_x0);
    for(size_t i = 0; i < siz; ++i){
        auto indexop = indexes[i];
        // auto dimnum = dims[i];
        // std::cerr<<i<<" : "<<indexop->GetFullName()<<" "<<dimnum<<" "<<dimmax_base<<'\n';
        bool isZeroFlag = indexop->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)indexop)->GetIntImmVal() == 0;
        if(!isZeroFlag){
            // 1.1 获取当前偏移基量, 存入 temp_offset_register
            auto temp_offset_register = GetNewTempRegister(INT64);
            InsertImmI32Instruction(temp_offset_register, new ImmI32Operand(dimmax_base), cur_block);
            Register AddRegister;
            // 1.2 获得下标 AddRegister
            if(indexop->GetOperandType() == BasicOperand::REG){
                auto indexregno = ((RegOperand*)indexop)->GetRegNo();
                AddRegister = GetNewRegister(indexregno, INT32);
				// std::cerr << "下标是一个寄存器 %" << AddRegister.get_reg_no() << std::endl;
            }else{
                // 立即数
				// std::cerr << "下标是一个立即数" << std::endl;
                auto imm = ((ImmI32Operand*)indexop)->GetIntImmVal();
                AddRegister = GetNewTempRegister(INT64);
                InsertImmI32Instruction(AddRegister, new ImmI32Operand(imm), cur_block);
            }
            // 1.3 算出实际偏移量 = 下标 * 偏移基量 tempresult_register
            auto mul_register = GetNewTempRegister(INT64);
            auto mul_instr = rvconstructor->ConstructR(RISCV_MUL, mul_register, AddRegister, temp_offset_register);
            cur_block->push_back(mul_instr);
            // 1.4 将实际偏移量加到总偏移量中
            if(last_register == GetPhysicalReg(RISCV_x0)){
                last_register = mul_register;
            }else{
                auto new_sum_register = GetNewTempRegister(INT64);
                // std::cerr<<"here : "<<new_sum_register.get_reg_no()<<"\n";
                auto add_instr = rvconstructor->ConstructR(RISCV_ADD, new_sum_register, mul_register, last_register);
                cur_block->push_back(add_instr);
                last_register = new_sum_register;
            }
        }
        if(i < dims.size()){
            dimmax_base /= dims[i];
        }
    }
    // 1.5 最终偏移量 = 总和偏移量 << 2
    Register final_offset_register;
    final_offset_register = GetNewTempRegister(INT64);
    auto slli_instr = rvconstructor->ConstructIImm(RISCV_SLLI, final_offset_register, last_register, 2);
    cur_block->push_back(slli_instr);
    
    // 2. 计算 base 地址(sp + offset)
    // 如果是函数参数, 则不需要计算
    if(base_op->GetOperandType() == BasicOperand::REG){
        // 局部数组
        auto base_regno = ((RegOperand*)base_op)->GetRegNo();
        auto offset_reg = GetNewTempRegister(INT64);
        if(llvm2rv_regmap.find(base_regno) != llvm2rv_regmap.end()){
			auto resultop = inst->GetResult();
            auto resultregno = ((RegOperand*)resultop)->GetRegNo();
            auto resultregister = GetNewRegister(resultregno, INT64);
            auto add_instr = rvconstructor->ConstructR(RISCV_ADD, resultregister, GetNewRegister(base_regno, INT64), final_offset_register);
            cur_block->push_back(add_instr);
        }else{
			Register addr_register = GetNewTempRegister(INT64);
			auto li_inst = rvconstructor->ConstructUImm(RISCV_LI, offset_reg, llvmReg_offset_map[base_regno]);
			cur_block->push_back(li_inst);
			((RiscV64Function *)cur_func)->AddAllocaInst(li_inst);
			auto addr_instr = rvconstructor->ConstructR(RISCV_ADD, addr_register, GetPhysicalReg(RISCV_sp), offset_reg);
			cur_block->push_back(addr_instr);
            auto resultop = inst->GetResult();
            auto resultregno = ((RegOperand*)resultop)->GetRegNo();
            auto resultregister = GetNewRegister(resultregno, INT64);
            auto add_instr = rvconstructor->ConstructR(RISCV_ADD, resultregister, addr_register, final_offset_register);
            cur_block->push_back(add_instr);
        }
        
    }else{
        // 全局数组
        GlobalOperand* global_op = (GlobalOperand *)inst->GetPtrVal();
		Register ptr_reg = GetNewTempRegister(INT64);
		auto la_inst = rvconstructor->ConstructULabel(RISCV_LA, ptr_reg, RiscVLabel(global_op->GetName(), true));
		cur_block->push_back(la_inst);
        auto resultop = inst->GetResult();
        auto resultregno = ((RegOperand*)resultop)->GetRegNo();
        auto add_instr = rvconstructor->ConstructR(RISCV_ADD, GetNewRegister(resultregno, INT64), final_offset_register, ptr_reg);
        cur_block->push_back(add_instr);
    }
    
}
template <> void RiscV64Unit::ConvertAndAppend<PhiInstruction *>(PhiInstruction *inst) {
    // TODO("Implement this if you need");
    // https://www.cnblogs.com/lixingyang/p/17721341.html
    // 1. 使用并查集构建 phi 网络, 对每个 phi 网络构建一个临时寄存器
    // 2. 遍历 phi 网络里的每一个phi, 在前驱基本块 philist.label 存取数值 philist.val 进临时寄存器, 在 phi 指令处取出
    auto resultop = inst->GetResult();
    auto resultreg = (RegOperand*)resultop;
    auto resultregno = resultreg->GetRegNo();
    auto rootphiseq = phi_temp_phiseqmap[resultregno];
    // std::cerr<<resultregno<<" "<<rootphiseq<<'\n';
    if(phi_temp_registermap.find(rootphiseq) == phi_temp_registermap.end()){
        if(inst->GetResultType() == BasicInstruction::I32 || inst->GetResultType() == BasicInstruction::PTR){
            phi_temp_registermap[rootphiseq] = GetNewTempRegister(INT64);
        }else{
            phi_temp_registermap[rootphiseq] = GetNewTempRegister(FLOAT64);
        }
        mem2reg_destruc_set.insert(phi_temp_registermap[rootphiseq]);
        // llvm2rv_regmap[resultreg->GetRegNo()] = phi_temp_registermap[rootphiseq];
    }

    // inst->PrintIR(std::cerr);
    auto tempregister = phi_temp_registermap[rootphiseq];
    for(auto [labelop, valop] : inst->GetPhiList()){
        auto labelno = ((LabelOperand*)labelop)->GetLabelNo();
        auto cur_mcfg = cur_func->getMachineCFG();
        auto bb = (cur_mcfg->block_map)[labelno];
        auto bblist = bb->Mblock->GetInsList();
        // MachineBaseInstruction* terminalI = nullptr;
        // MachineBaseInstruction* terminalI2 = nullptr;
        // if(bblist.size() > 0){
        //     terminalI = bblist.back();
        //     bb->Mblock->pop_back();
        // }
        // if(bblist.size() > 1 && block_terminal_type[labelno]){
        //     terminalI2 = *(----bblist.end());
        //     bb->Mblock->pop_back();
        // }
        MachineBaseInstruction *add_instr;
        // "do right here"
        if(inst->GetResultType() == BasicInstruction::I32 || inst->GetResultType() == BasicInstruction::PTR){
            // inst->PrintIR(std::cerr);
            // std::cerr<<inst->GetResultType()<<" "<<valop->GetFullName()<<" "<<valop->GetOperandType()<<'\n';
            if(valop->GetOperandType() == BasicOperand::REG){
                auto valreg = (RegOperand*)valop;
                auto valregno = valreg->GetRegNo();
                auto reg_register = GetNewRegister(valregno, INT64);
                add_instr = rvconstructor->ConstructR(RISCV_ADD, tempregister, reg_register, GetPhysicalReg(RISCV_x0));
            }else{
                auto valimm = (ImmI32Operand*)valop;
                auto imm = valimm->GetIntImmVal();
                auto temp_imm_register = GetNewTempRegister(INT64);
                InsertImmI32Instruction(temp_imm_register, valimm, bb->Mblock, 1);
                add_instr = rvconstructor->ConstructR(RISCV_ADD, tempregister, temp_imm_register, GetPhysicalReg(RISCV_x0));
            }
        }else/* if(inst->GetResultType() == BasicInstruction::FLOAT32)*/{
            if(valop->GetOperandType() == BasicOperand::REG){
                auto valreg = (RegOperand*)valop;
                auto valregno = valreg->GetRegNo();
                auto reg_register = GetNewRegister(valregno, FLOAT64);
                add_instr = rvconstructor->ConstructR2(RISCV_FMV_S, tempregister, reg_register);
            }else{
                auto valimm = (ImmF32Operand*)valop;
                auto imm = valimm->GetFloatVal();
                auto temp_imm_register = GetNewTempRegister(FLOAT64);
                InsertImmFloat32Instruction(temp_imm_register, valimm, bb->Mblock, 1);
                add_instr = rvconstructor->ConstructR2(RISCV_FMV_S, tempregister, temp_imm_register);
            }
        }
        // 将生成的指令放在所有基本块都生成后再生成
        phi_instr_map[bb->Mblock->getLabelId()].push_back(add_instr);
    }

    if(inst->GetResultType() == BasicInstruction::I32 || inst->GetResultType() == BasicInstruction::PTR){
        auto newreg = GetNewRegister(resultregno, INT64);
        llvm2rv_regmap[resultregno] = newreg;
        auto add_instr = rvconstructor->ConstructR(RISCV_ADD, newreg, tempregister, GetPhysicalReg(RISCV_x0));
        cur_block->push_back(add_instr);
    }else{
        auto newreg = GetNewRegister(resultregno, FLOAT64);
        llvm2rv_regmap[resultregno] = newreg;
        auto add_instr = rvconstructor->ConstructR2(RISCV_FMV_S, newreg, tempregister);
        cur_block->push_back(add_instr);
    }
    // std::cerr<<llvm2rv_regmap[resultreg->GetRegNo()].get_reg_no()<<'\n';
}

void RiscV64Unit::BuildPhiWeb(CFG *C){
    std::map<int, int> UnionFindMap;
    std::function<int(int)> UnionFind = [&](int RegToFindNo) -> int {
        if (UnionFindMap[RegToFindNo] == RegToFindNo)
            return RegToFindNo;
        return UnionFindMap[RegToFindNo] = UnionFind(UnionFindMap[RegToFindNo]);
    };
    auto Connect = [&](Operand resultOp, Operand replaceOp) -> void {
        auto Reg1 = (RegOperand *)resultOp;
        auto Reg1no = Reg1->GetRegNo();
        auto Reg0 = (RegOperand *)replaceOp;
        auto Reg0no = Reg0->GetRegNo();
        UnionFindMap[UnionFind(Reg1no)] = UnionFind(Reg0no);
    };
    for (int i = 0; i <= C->max_reg; ++i) {
        UnionFindMap[i] = i;
    }
    phiseq = -1;
    for(auto [id, bb] : *C->block_map){
        for(auto I : bb->Instruction_list){
            if(I->GetOpcode() != BasicInstruction::PHI){
                break;
            }
            auto phiI = (PhiInstruction*)I;
            auto resultop = phiI->GetResult();
            auto resultreg = (RegOperand*)resultop;
            auto resultregno =resultreg->GetRegNo();
            for(auto [labelop, valop] : phiI->GetPhiList()){
                if(valop->GetOperandType() != BasicOperand::REG){
                    continue;
                }
                auto valreg = (RegOperand*)valop;
                auto valregno = valreg->GetRegNo();
                Connect(resultop, valop);
            }
        }
    }

    for(auto [id, bb] : *C->block_map){
        for(auto I : bb->Instruction_list){
            if(I->GetOpcode() != BasicInstruction::PHI){
                break;
            }
            // I->PrintIR(std::cerr);
            auto phiI = (PhiInstruction*)I;
            auto resultop = phiI->GetResult();
            auto resultreg = (RegOperand*)resultop;
            auto resultregno =resultreg->GetRegNo();
            // auto rootregno = UnionFind(resultregno);
            auto rootregno = resultregno;
            if(phi_temp_phiseqmap.find(rootregno) == phi_temp_phiseqmap.end()){
                phiseq++;
                phi_temp_phiseqmap[rootregno] = phiseq;
                // std::cerr<<resultregno<<" "<<rootregno<<" "<<phiseq<<" "<<phi_temp_phiseqmap[rootregno]<<'\n';
            }
            phi_temp_phiseqmap[resultregno] = phi_temp_phiseqmap[rootregno];
			// std::cerr<<resultregno<<" "<<rootregno<<" "<<phiseq<<" "<<phi_temp_phiseqmap[rootregno]<<'\n';
        }
    }
}

template <> void RiscV64Unit::ConvertAndAppend<BitCastInstruction *>(BitCastInstruction *ins) {
    Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
	Assert(ins->GetFromType() == BasicInstruction::FLOAT32);
	Assert(ins->GetType() == BasicInstruction::I32);

    auto rd_op = (RegOperand *)ins->GetResult();
    auto result_reg = GetNewRegister(rd_op->GetRegNo(), INT64);
    auto src_op = ins->GetSrc();

    if (src_op->GetOperandType() == BasicOperand::REG) {
        auto rs_op = (RegOperand *)src_op;
        auto src_reg = GetNewRegister(rs_op->GetRegNo(), FLOAT64);
        auto fmv_inst = rvconstructor->ConstructR2(RISCV_FMV_X_W, result_reg, src_reg);
        cur_block->push_back(fmv_inst);
    } else if (src_op->GetOperandType() == BasicOperand::IMMF32) {
        auto float_imm = (ImmF32Operand *)src_op;
        float fimm = float_imm->GetFloatVal();
        int bit_pattern = *((int *)(&fimm));
		InsertImmI32Instruction(result_reg, new ImmI32Operand(bit_pattern), cur_block);
	}
}