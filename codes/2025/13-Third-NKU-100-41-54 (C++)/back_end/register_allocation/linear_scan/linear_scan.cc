#include"linear_scan.h"
#include"../../inst_process/machine_instruction.h"
extern std::map<Register, std::set<Register>> reg_to_reg;
void FastLinearScan::VirtualRegisterRewrite() {
    for (auto func : unit->functions) {
        current_func = func;
        RewriteInFunc();
    }
}

void FastLinearScan::RewriteInFunc() {
    auto func = current_func;
    auto mcfg = func->getMachineCFG();
    mcfg->seqscan_open();
    while (mcfg->seqscan_hasNext()) {
        auto block = mcfg->seqscan_next()->Mblock;
        for (auto it = block->begin(); it != block->end(); ++it) {
            auto ins = *it;
            // 根据alloc_result将ins的虚拟寄存器重写为物理寄存器
        for(auto reg:ins->GetReadReg())//遍历当前指令中所有读寄存器
            {
                if(reg->is_virtual==false)//如果不是虚拟寄存器
                {
                    //检查分配结果中是否找不到该寄存器
                    Assert(alloc_result.find(func)==alloc_result.end()
                    ||alloc_result.find(func)->second.find(*reg)==alloc_result.find(func)->second.end());
                    continue;
                }
                //当前函数，当前寄存器对应的alloc_result
                auto result=alloc_result.find(func)->second.find(*reg)->second;
                if(result.in_mem==true){ERROR("shoudn't reach here");}//如果已经分配了栈空间，那么不再需要分配寄存器
                else{
                    reg->is_virtual=false;
                    reg->reg_no=result.phy_reg_no;
                }
            }
            for(auto reg:ins->GetWriteReg())//遍历当前指令的所有写寄存器
            {
                if(reg->is_virtual==false)
                {
                    Assert(alloc_result.find(func)==alloc_result.end()
                    ||alloc_result.find(func)->second.find(*reg)==alloc_result.find(func)->second.end());
                    continue;
                }
                auto result=alloc_result.find(func)->second.find(*reg)->second;
                if(result.in_mem==true){ERROR("shoudn't reach here");}
                else{
                    reg->is_virtual=false;//在内存中的为虚拟寄存器，不在内存中的为非虚拟
                    reg->reg_no=result.phy_reg_no;
                }
            }
        }
    }
   mcfg->seqscan_close();
}

bool IntervalsPrioCmp(LiveInterval a, LiveInterval b) { return a.begin()->begin > b.begin()->begin; }

bool FastLinearScan::DoAllocInCurrentFunc() {
    //一、前置工作
    //1.设置spilled标签
    bool spilled = false;
    //2.指定函数
    auto mfun = current_func;
    //MachineFunction* mfun;
    // std::cerr<<"FastLinearScan: "<<mfun->getFunctionName()<<"\n";
    //3.清空tool
    phy_regs_tools->clear();
    //4.遍历所有活跃区间
    for (auto interval : intervals) {
        Assert(interval.first == interval.second.getReg());
        //1)对于虚拟寄存器，压入unalloc_queue队列。Q：在哪里确定的是否虚拟？
        if (interval.first.is_virtual) {
            unalloc_queue.push(interval.second);
            //std::cout<<"Unalloc Reg:"<<interval.first.reg_no<<"\n";
        //2)对于物理寄存器，占用它
        } else {
            //std::cout<<"Pre Occupy Physical Reg "<<interval.first.reg_no<<"\n";
            // 预先占用已经存在的物理寄存器
            phy_regs_tools->OccupyReg(interval.first.reg_no, interval.second);
        }
    }
    // TODO: 进行线性扫描寄存器分配, 为每个虚拟寄存器选择合适的物理寄存器或者将其溢出到合适的栈地址中
    // 该函数中只需正确设置alloc_result，并不需要实际生成溢出代码
    //TODO("LinearScan");
    //5.遍历unalloc_queue中的虚拟寄存器
     while(!unalloc_queue.empty())
    {
        //1）获取待处理的活跃区间，及其对应的寄存器（编号）
        auto interval=unalloc_queue.top();
        unalloc_queue.pop();
        auto cur_vreg=interval.getReg();
        std::vector<int> preferred_regs;
        for (auto reg : reg_to_reg[cur_vreg]) {
            if(!reg.is_virtual)
            {
                preferred_regs.push_back(reg.reg_no);//偏好物理寄存器
            }
            else if(reg.is_virtual&&(alloc_result[mfun].find(reg) != alloc_result[mfun].end())&&alloc_result[mfun][reg].in_mem == false)
            {
                preferred_regs.push_back(alloc_result[mfun][reg].phy_reg_no);
            }
        }
        //2）尝试获取空闲物理寄存器（通过活跃区间）
        //int phy_reg_id = phy_regs_tools->getIdleReg(interval);
        int phy_reg_id = phy_regs_tools->getIdleReg(interval, preferred_regs);
        //std::cout<<"reg_no="<<cur_vreg.reg_no<<" , idlereg="<<phy_reg_id<<"\n";
        //A.如果有空闲的物理寄存器，占用它
        if (phy_reg_id >= 0) {
            phy_regs_tools->OccupyReg(phy_reg_id, interval);
            AllocPhyReg(mfun, cur_vreg, phy_reg_id);
        //B.如果没有空闲的，标记spilled为true,需要获取空闲内存
        } else {
            //a.标记spilled
            spilled = true;
            //b.获取空闲内存并占用
            int mem = phy_regs_tools->getIdleMem(interval);
            phy_regs_tools->OccupyMem(mem, interval);
            // volatile int mem_ = mem;
            // volatile int mem__ = mem_+current_func->GetStackOffset();
            //c.分配栈空间
            AllocStack(mfun, cur_vreg, mem);
            //d.计算溢出权重
            double spill_weight = CalculateSpillWeight(interval);
            auto spill_interval = interval;
            //e.选择溢出权重最大的溢出
            for (auto other : phy_regs_tools->getConflictIntervals(interval)) {
                double other_weight = CalculateSpillWeight(other);
                if (spill_weight > other_weight && other.getReg().is_virtual) {
                    spill_weight = other_weight;
                    spill_interval = other;
                }
            }
            auto spill_reg=spill_interval.getReg();
            //std::cout<<"spill to mem="<<mem<<" , spill_reg="<<spill_reg.reg_no<<", cur_vreg="<<cur_vreg.reg_no<<"\n";
            if (!(cur_vreg==spill_reg)) {
                phy_regs_tools->swapRegspill(getAllocResultInReg(mfun, spill_interval.getReg()), spill_interval, mem,
                                        cur_vreg.getDataWidth(), interval);
                swapAllocResult(mfun, interval.getReg(), spill_interval.getReg());
                // alloc_result[mfun].erase(spill_interval.getReg());
                // unalloc_queue.push(spill_interval);
                int spill_mem = phy_regs_tools->getIdleMem(spill_interval);
                phy_regs_tools->OccupyMem(spill_mem, spill_interval);
                AllocStack(mfun, spill_interval.getReg(), spill_mem);
            }
            //ShowAllAllocResult();
            
        }
    }
    // std::cout << "函数: " << mfun->getFunctionName() << std::endl;
    // for (auto &reg_pair : alloc_result[mfun]) {
    //     auto vreg = reg_pair.first;
    //     auto res = reg_pair.second;
    //     std::cout << "  虚拟寄存器: " << vreg.reg_no << " -> ";
    //     if (res.in_mem) {
    //         std::cout << "栈偏移: " << res.stack_offset << std::endl;
    //     } else {
    //         std::cout << "物理寄存器: " << res.phy_reg_no << std::endl;
    //     }
    // }
    return spilled;
}

//reference:https://github.com/yuhuifishash/SysY/target/common/machine_instruction_structures/machine.cc  line 308~333
std::set<MachineBlock *> FindNodesInLoop(MachineCFG *C, MachineBlock *n, MachineBlock *d)    // backedge n->d
{
    std::set<MachineBlock *> loop_nodes;

    std::stack<MachineBlock *> S;

    loop_nodes.insert(n);
    loop_nodes.insert(d);

    if (n == d) {
        return loop_nodes;
    }

    S.push(n);
    while (!S.empty()) {
        MachineBlock *x = S.top();
        S.pop();
        for (auto preBB : C->GetPredecessorsByBlockId(x->getLabelId())) {
            if (loop_nodes.find(preBB->Mblock) == loop_nodes.end()) {
                loop_nodes.insert(preBB->Mblock);
                S.push(preBB->Mblock);
            }
        }
    }
    return loop_nodes;
}

void FastLinearScan::ComputeLoopDepth(MachineCFG* C) {

    // (1) 收集块并清零 loop_depth
    std::vector<MachineBlock*> blocks;
    {
        C->seqscan_open();
        while (C->seqscan_hasNext()) {
            auto *block = C->seqscan_next()->Mblock;
            block->loop_depth = 0;
            blocks.push_back(block);
        }
    }

    // (2) 以 header 为键，聚合(并集)该 header 的自然环节点
    //    这样同一 header 由多条回边产生的环不会被重复计数
    std::unordered_map<MachineBlock*, std::unordered_set<MachineBlock*>> header2nodes;

    for (auto *tail : blocks) {
        auto tail_id = tail->getLabelId();
        for (auto succ : C->GetSuccessorsByBlockId(tail_id)) { // tail -> head 候选回边
            auto *head = succ->Mblock;
            if (C->DomTree->IsDominate(head->getLabelId(), tail_id)) {
                // 自然环节点（以该回边为依据）
                auto nodes = FindNodesInLoop(C, tail, head);
                auto &bucket = header2nodes[head]; // 并集累加
                for (auto *n : nodes) bucket.insert(n);
            }
        }
    }

    // (3) 每个 header 的环并集算作一个 loop：对其中每个块 loop_depth += 1
    for (auto &kv : header2nodes) {
        const auto &nodes = kv.second;
        for (auto *bb : nodes) {
            bb->loop_depth += 1;
        }
    }
}

// 计算溢出权重
double FastLinearScan::CalculateSpillWeight(LiveInterval interval) {
    return (double)interval.getReferenceCount() / interval.getIntervalLength();
}

void FastLinearScan::Execute() {
    // 需要保证此时不存在phi指令
    for (auto func : unit->functions) {
        not_allocated_funcs.push(func);
    }
    // 计算循环深度，为UpdateIntervalsInCurrentFunc中计算引用次数做准备
    for (auto &func: unit->functions){
        auto C = func->getMachineCFG();
        ComputeLoopDepth(C);
    }
    
    //1.逐一处理函数
	int spill_size=0;
    while (!not_allocated_funcs.empty()) {
        current_func = not_allocated_funcs.front();
        numbertoins.clear();
        //2.对每条指令进行编号
        InstructionNumber(unit, numbertoins).ExecuteInFunc(current_func);

        //3.清除之前分配的结果
        alloc_result[current_func].clear();
        not_allocated_funcs.pop();

        //4.计算活跃区间并消除冗余复制指令
        UpdateIntervalsInCurrentFunc();
        if (DoAllocInCurrentFunc()) {    // 5.尝试进行分配
            // 6.如果发生溢出，插入spill指令后将所有物理寄存器退回到虚拟寄存器，重新分配
            SpillCodeGen(current_func, &alloc_result[current_func]);    // 生成溢出代码
			// std::cerr<<"spill size:"<<phy_regs_tools->getSpillSize()<<std::endl;
			spill_size+=phy_regs_tools->getSpillSize();
			current_func->AddStackSize(phy_regs_tools->getSpillSize());                 // 调整栈的大小
            not_allocated_funcs.push(current_func);                               // 重新分配直到不再spill
        }
    }
    // INSERT_YOUR_CODE
    // 文件输出，id, spillsize，id从0开始，随着文件末尾递增
    {
        FILE* fp = fopen("spillsize.txt", "a");
        if (fp) {
            fprintf(fp, "%d\n", spill_size);
            fclose(fp);
        }
    }
    // 重写虚拟寄存器，全部转换为物理寄存器
    VirtualRegisterRewrite();
}

void InstructionNumber::Execute() {
    for (auto func : unit->functions) {
        ExecuteInFunc(func);
    }
}

void InstructionNumber::ExecuteInFunc(MachineFunction *func) {
    // 对每个指令进行编号(用于计算活跃区间)
    int count_begin = 0;
    //current_func = func;
    // Note: If Change to DFS Iterator, FastLinearScan::UpdateIntervalsInCurrentFunc() Also need to be
    // changed
    auto mcfg = func->getMachineCFG();
    mcfg->bfs_open();
    while (mcfg->bfs_hasNext()) {
        auto mcfg_node = mcfg->bfs_next();
        auto mblock = mcfg_node->Mblock;
        // Update instruction number
        // 每个基本块开头会占据一个编号
        this->numbertoins[count_begin] = InstructionNumberEntry(nullptr, true);
        count_begin++;
        for (auto ins : *mblock) {
            this->numbertoins[count_begin] = InstructionNumberEntry(ins, false);
            ins->setNumber(count_begin++);
        }
    }
    //mcfg->bfs_close();
}

// 更新当前函数中所有寄存器的活跃区间（Live Interval）
void FastLinearScan::UpdateIntervalsInCurrentFunc() {
    //1.清空之前的活跃区间数据（以函数为单位，可能处理过其他函数）
    intervals.clear(); 
    reg_to_reg.clear();//清空寄存器复制情况
    //2.获取当前函数，当前函数对应的控制流图
    auto mfun = current_func; 
    auto mcfg = mfun->getMachineCFG(); 
    //3.对当前函数进行活跃分析
    Liveness liveness(mfun); 

    // 4.准备反向遍历控制流图（逆向数据流分析，从函数出口遍历到函数入口）
    mcfg->bfs_close(); 
    mcfg->reverse_open(); 
    
    // 记录寄存器最后一次定义和使用的位置（指令编号）
    std::map<Register, int> last_def, last_use;

    // 开始反向遍历所有基本块（逆控制流方向）
    while (mcfg->reverse_hasNext()) {
        auto mcfg_node = mcfg->reverse_next(); // 获取下一个基本块节点
        auto mblock = mcfg_node->Mblock; // 当前基本块对象
        auto cur_id = mblock->getLabelId(); // 基本块标签ID

        // 处理基本块的OUT集合（出口处活跃的寄存器）
        for (auto reg : liveness.GetOUT(cur_id)) {
            // 若该寄存器尚未记录，创建新的活跃区间
            if (intervals.find(reg) == intervals.end()) {
                intervals[reg] = LiveInterval(reg); // 初始化区间对象
            }
            
            // 扩展或新增区间段
            if (last_use.find(reg) == last_use.end()) {
                // 情况1：无前驱使用记录，创建新区间段（从块入口到出口）
                intervals[reg].PushFront(
                    mblock->getBlockInNumber(), // 基本块起始指令编号
                    mblock->getBlockOutNumber() // 基本块结束指令编号
                );
            } else {
                // 情况2：已有前驱使用记录，合并区间（仍会创建新区间段）
                intervals[reg].PushFront(
                    mblock->getBlockInNumber(),
                    mblock->getBlockOutNumber()
                );
            }
            last_use[reg] = mblock->getBlockOutNumber(); // 记录最后一次使用位置
        }

        // 反向遍历当前基本块内的指令（从最后一条到第一条）
        for (auto reverse_it = mblock->ReverseBegin(); 
             reverse_it != mblock->ReverseEnd(); 
             ++reverse_it) 
        {
            auto ins = *reverse_it; // 获取当前指令对象
            //统计寄存器复制关系
            if(ins->arch==MachineBaseInstruction::RiscV)
            {
                auto riscv_ins=static_cast<RiscV64Instruction*>(ins);
                auto op=riscv_ins->getOpcode();
                auto rs1=riscv_ins->getRs1();
                auto rs2=riscv_ins->getRs2();
                auto rd=riscv_ins->getRd();
                if(op==RISCV_ADD||op==RISCV_ADDW)
                {
                    if(rs1.reg_no==RISCV_x0)
                    {
                        reg_to_reg[rs2].insert(rd);
                        reg_to_reg[rd].insert(rs2);
                    }
                    else if(rs2.reg_no==RISCV_x0)
                    {
                        reg_to_reg[rs1].insert(rd);
                        reg_to_reg[rd].insert(rs1);
                    }
                }
                else if(op==RISCV_FMV_S)
                {
                    reg_to_reg[rs1].insert(rd);
                    reg_to_reg[rd].insert(rs1);
                }
            }
            // 处理指令的写寄存器（定义操作）
            for (auto reg : ins->GetWriteReg()) {
                // 更新最后一次定义位置
                last_def[*reg] = ins->getNumber(); // 记录指令编号

                // 初始化未记录的寄存器区间
                if (intervals.find(*reg) == intervals.end()) {
                    intervals[*reg] = LiveInterval(*reg);
                }

                // 检查是否存在未处理的最后一次使用
                if (last_use.find(*reg) != last_use.end()) {
                    // 情况1：存在后续使用，分割当前区间
                    last_use.erase(*reg); // 清除使用记录
                    intervals[*reg].SetMostBegin(ins->getNumber()); // 设置区间起点为定义位置
                } else {
                    // 情况2：无后续使用，创建单指令区间（仅定义点）
                    intervals[*reg].PushFront(
                        ins->getNumber(), // 起始
                        ins->getNumber()   // 结束（定义即结束）
                    );
                }
                intervals[*reg].IncreaseReferenceCount(mblock->loop_depth);//引用计数（用于溢出优先级）
            }

            // 处理指令的读寄存器（使用操作）
            for (auto reg : ins->GetReadReg()) {
                // 初始化未记录的寄存器区间
                if (intervals.find(*reg) == intervals.end()) {
                    intervals[*reg] = LiveInterval(*reg);
                }

                // 检查是否存在未处理的最后一次使用
                if (last_use.find(*reg) == last_use.end()) {
                    // 情况：无后续使用，创建从块入口到当前指令的区间
                    intervals[*reg].PushFront(
                        mblock->getBlockInNumber(), // 块起始
                        ins->getNumber()            // 当前指令
                    );
                }
                last_use[*reg] = ins->getNumber(); // 更新最后一次使用位置
                intervals[*reg].IncreaseReferenceCount(mblock->loop_depth);
            }
        } // 结束指令遍历

        // 清空临时记录，避免跨基本块污染
        last_use.clear();
        last_def.clear();
    } // 结束基本块遍历

    //区间合并算法：将相邻或可兼容的活跃区间合并，减少寄存器分配时的冲突可能性
    UnionFind uf;
    uf.initialize(intervals);

    // 合并操作
    for (const auto& [reg, interval] : intervals) {
        if (!reg.is_virtual) continue;
        
        for (auto other : reg_to_reg[reg]) {
            if (!other.is_virtual) continue;
            
            Register rootReg = uf.findRoot(reg);
            Register rootOther = uf.findRoot(other);
            
            // 跳过相同集合或重叠区间
            if (rootReg == rootOther || (intervals[rootReg] & intervals[rootOther])) 
                continue;
            
            // 合并活跃区间
            intervals[rootReg] =intervals[rootReg]|intervals[rootOther];
            
            // 转移复制关系
            for (auto src : reg_to_reg[rootOther]) {
                if (!src.is_virtual) {
                    reg_to_reg[src].insert(rootReg);
                    reg_to_reg[rootReg].insert(src);
                }
            }
            
            // 执行并查集合并
            uf.unionSets(rootReg, rootOther);
            intervals.erase(rootOther);
        }
    }

    // 更新指令寄存器引用
    auto updateRegisterRef = [&](Register& reg) {
        if (reg.is_virtual) 
            reg = uf.findRoot(reg);
    };

    for (auto block : current_func->blocks) {
        for (auto ins : *block) {
            for (auto& reg : ins->GetReadReg()) 
                updateRegisterRef(*reg);
            for (auto& reg : ins->GetWriteReg()) 
                updateRegisterRef(*reg);
        }
    }
}

const int MAX_TEMP_REGS=11;
void FastLinearScan::SpillCodeGen(MachineFunction *function, std::map<Register, AllocResult> *alloc_result) {
    //std::cout<<"Optimized SpillCodeGen\n";
    auto mcfg = function->getMachineCFG();
    mcfg->seqscan_open();
    
    // 跟踪当前使用的临时寄存器数量
    int temp_reg_count = 0;
    // 用于存储频繁访问的栈值的临时寄存器
    std::map<int, Register> temp_reg_cache;
    //1.顺序逐个访问函数的基本块
    while (mcfg->seqscan_hasNext()) {
        cur_block = mcfg->seqscan_next()->Mblock;
        //2.顺序访问基本块的各个指令
        for (auto it = cur_block->begin(); it != cur_block->end(); ++it) {
            auto ins = *it;
            std::set<Register> processed_regs;//已处理的寄存器集合（避免重复处理）
            
        for (auto reg : ins->GetReadReg()) {
                if (reg->is_virtual == false)
                    continue;
                auto result = alloc_result->find(*reg)->second;
                if (result.in_mem == true) {
                    // Spill Code Gen
                    *reg = GenerateReadCode(it, result.stack_offset * 4, reg->type);
                }
            }
            for (auto reg : ins->GetWriteReg()) {
                if (reg->is_virtual == false)
                    continue;
                auto result = alloc_result->find(*reg)->second;
                if (result.in_mem == true) {
                    // Spill Code Gen
                    *reg = GenerateWriteCode(it, result.stack_offset * 4, reg->type);
                }
            }
        }
    }
}
// 显示alloc_result的全部内容（allocres）
void FastLinearScan::ShowAllAllocResult() {
	std::cout << "======== alloc_result 全部内容 ========" << std::endl;
	for (auto &func_pair : alloc_result) {
		auto *mfun = func_pair.first;
		std::cout << "函数: " << mfun->getFunctionName() << std::endl;
		for (auto &reg_pair : func_pair.second) {
			auto vreg = reg_pair.first;
			auto res = reg_pair.second;
			std::cout << "  虚拟寄存器: " << vreg.reg_no << " -> ";
			if (res.in_mem) {
				std::cout << "栈偏移: " << res.stack_offset << std::endl;
			} else {
				std::cout << "物理寄存器: " << res.phy_reg_no << std::endl;
			}
		}
	}
	std::cout << "======================================" << std::endl;
}

Register FastLinearScan::GenerateReadCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
                                          MachineDataType type) {
    auto read_mid_reg = current_func->GetNewRegister(type.data_type, type.data_length);
    // missing lowerimm
    // missing stack size adjust
    int offset = raw_stk_offset + current_func->GetStackOffset();
    // cur_block->insert(it, rvconstructor->ConstructComment("Read Spill\n"));
    if (offset <= 2047 && offset >= -2048) {
        if (type == INT64) {
            cur_block->insert(it, rvconstructor->ConstructIImm(RISCV_LD, read_mid_reg, GetPhysicalReg(RISCV_sp),
                                                               offset));    // insert load
        } else if (type == FLOAT64) {
            cur_block->insert(it,
                              rvconstructor->ConstructIImm(RISCV_FLD, read_mid_reg, GetPhysicalReg(RISCV_sp), offset));
        }
    } else {
        auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
        auto offset_mid_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
        cur_block->insert(it, rvconstructor->ConstructUImm(RISCV_LI, imm_reg, offset));
        cur_block->insert(it, rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
        if (type == INT64) {
            cur_block->insert(it, rvconstructor->ConstructIImm(RISCV_LD, read_mid_reg, offset_mid_reg, 0));
        } else if (type == FLOAT64) {
            cur_block->insert(it, rvconstructor->ConstructIImm(RISCV_FLD, read_mid_reg, offset_mid_reg, 0));
        }
    }
    return read_mid_reg;
}

Register FastLinearScan::GenerateWriteCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
                                           MachineDataType type) {
    auto write_mid_reg = current_func->GetNewRegister(type.data_type, type.data_length);
    int offset = raw_stk_offset + current_func->GetStackOffset();
    ++it;
    // cur_block->insert(it, rvconstructor->ConstructComment("Write Spill\n"));
    if (offset <= 2047 && offset >= -2048) {
        if (type == INT64) {
            cur_block->insert(it, rvconstructor->ConstructSImm(RISCV_SD, write_mid_reg, GetPhysicalReg(RISCV_sp),
                                                               offset));    // insert store
        } else if (type == FLOAT64) {
            cur_block->insert(it, rvconstructor->ConstructSImm(RISCV_FSD, write_mid_reg, GetPhysicalReg(RISCV_sp),
                                                               offset));    // insert store
        }
    } else {
        auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
        auto offset_mid_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
        cur_block->insert(it, rvconstructor->ConstructUImm(RISCV_LI, imm_reg, offset));
        cur_block->insert(it, rvconstructor->ConstructR(RISCV_ADD, offset_mid_reg, GetPhysicalReg(RISCV_sp), imm_reg));
        if (type == INT64) {
            cur_block->insert(it, rvconstructor->ConstructSImm(RISCV_SD, write_mid_reg, offset_mid_reg, 0));
        } else if (type == FLOAT64) {
            cur_block->insert(it, rvconstructor->ConstructSImm(RISCV_FSD, write_mid_reg, offset_mid_reg, 0));
        }
    }
    --it;
    return write_mid_reg;
}
