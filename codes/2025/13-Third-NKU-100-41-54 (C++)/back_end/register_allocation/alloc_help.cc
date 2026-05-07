#include"alloc_help.h"
#include<algorithm>

template <class T> std::set<T> SetIntersect(const std::set<T> &a, const std::set<T> &b) {
    std::set<T> ret;
    for (auto x : b) {
        if (a.count(x) != 0) {
            ret.insert(x);
        }
    }
    return ret;
}

template <class T> std::set<T> SetUnion(const std::set<T> &a, const std::set<T> &b) {
    std::set<T> ret(a);
    for (auto x : b) {
        ret.insert(x);
    }
    return ret;
}

// a-b
template <class T> std::set<T> SetDiff(const std::set<T> &a, const std::set<T> &b) {
    std::set<T> ret(a);
    for (auto x : b) {
        ret.erase(x);
    }
    return ret;
}
// DEF[B]: 在基本块B中定义，并且定义前在B中没有被使用的变量集合（！表示在块内最早定义且后续可能被覆盖的变量（需传播到后续块））
// USE[B]: 在基本块B中使用，并且使用前在B中没有被定义的变量集合（！表示在块内使用前未被重新定义的变量（需从入口继承活跃性））
// 更新每个基本块的DEF（定义集合）和USE（使用集合）
void Liveness::UpdateDefUse() {
    //1.清空状态，重头计算（之前可能有函数计算过）
    DEF.clear();  
    USE.clear();  
    //2.获取当前函数的CFG
    auto mcfg = current_func->getMachineCFG(); 
    mcfg->seqscan_open();// 顺序遍历所有基本块（按控制流正向顺序）
    while (mcfg->seqscan_hasNext()) { // 循环处理每个基本块
        //3.获取基本块及对应ID
        auto node = mcfg->seqscan_next(); 
        int block_id = node->Mblock->getLabelId(); 
        // 4.清空当前块的状态，用更简洁的方式表示当前块的DEF与USE
        DEF[block_id].clear(); 
        USE[block_id].clear(); 
        auto &cur_def = DEF[block_id];
        auto &cur_use = USE[block_id];
        // 5.遍历基本块内的所有指令（按执行顺序）
        for (auto ins : *(node->Mblock)) { 
            // 1）处理指令的读寄存器（使用操作）
            for (auto reg_r : ins->GetReadReg()) { 
                Register reg = *reg_r; 
                // 如果该寄存器尚未被定义（不在DEF集合中）
                if (cur_def.find(reg) == cur_def.end()) { 
                    cur_use.insert(reg); // 添加到USE集合（使用前未定义）
                }
            }

            // 2）处理指令的写寄存器（定义操作）
            for (auto reg_w : ins->GetWriteReg()) { 
                Register reg = *reg_w; // 获取寄存器对象
                // 如果该寄存器尚未被使用（不在USE集合中）
                if (cur_use.find(reg) == cur_use.end()) { 
                    cur_def.insert(reg); // 添加到DEF集合（定义前未使用）
                }
            }
        }
    }
    //mcfg->seqscan_close(); // 关闭顺序遍历模式
}

//OUT[B] = ∪ IN[S] （S 是 B 的所有后继块） 在基本块 B 出口处 活跃的变量集合。（它们的值在进入该基本块后会被使用。）
//IN[B] = USE[B] ∪ (OUT[B] - DEF[B])  在基本块 B 入口处 活跃的变量集合（离开该基本块后会被后续的基本块使用）
//OUT代表在基本块之外被使用（B的后继块的IN的并集）
//IN的话代表活跃区间要延续到块B之后：代表在基本块内被使用,比如USE[B](使用前未重新定义)。或者OUT[B] - DEF[B]（在后继块被使用，且未被块B重新定义）
void Liveness::Execute() {
    //1.计算所有基本块的 DEF 和 USE 集合
    UpdateDefUse(); 
    //2.清空OUT,IN
    OUT.clear();    
    IN.clear();     
    //3.获取当前函数的控制流图
    auto mcfg = current_func->getMachineCFG(); 
    bool changed = 1; // 标记是否发生数据流变化（需继续迭代）

    // 4.迭代执行数据流分析，直到结果稳定（不动点）
    while (changed) {
        //1)初始假设本轮无变化
        changed = 0; 

        //2)顺序遍历所有基本块（正向遍历）
        mcfg->seqscan_open(); 
        while (mcfg->seqscan_hasNext()) {
            //3)获取基本块及对应ID
            auto node = mcfg->seqscan_next(); 
            int cur_id = node->Mblock->getLabelId(); 

            // 4）计算 OUT[cur_id] = U IN[succ] （所有后继块的 IN 集合的并集）
            std::set<Register> out;
            for (auto succ : mcfg->GetSuccessorsByBlockId(cur_id)) {
                int succ_id = succ->Mblock->getLabelId();
                out = SetUnion<Register>(out, IN[succ_id]); // 集合并操作
            }

            // 步骤2.1.2：若 OUT 集合变化，更新并标记后续需继续迭代
            if (out != OUT[cur_id]) {
                OUT[cur_id] = out;
            }

            // 步骤2.1.3：计算 IN[cur_id] = USE[cur_id] U (OUT[cur_id] - DEF[cur_id])
            std::set<Register> in = SetUnion<Register>(
                USE[cur_id], 
                SetDiff<Register>(OUT[cur_id], DEF[cur_id]) // 集合差操作
            );

            // 步骤2.1.4：若 IN 集合变化，标记 changed 为 true
            if (in != IN[cur_id]) {
                changed = 1; // 数据流发生变化，需要继续迭代
                IN[cur_id] = in; // 更新 IN 集合
            }
        }
        mcfg->seqscan_close(); // 关闭顺序遍历模式
    } // 结束迭代（当 changed 保持 0 时退出）
}
bool PhysicalRegistersAllocTools::OccupyReg(int phy_id, LiveInterval interval) {
    // 你需要保证interval不与phy_id已有的冲突
    // 或者增加判断分配失败返回false的代码
    phy_occupied[phy_id].push_back(interval);
    return true;
}

bool PhysicalRegistersAllocTools::ReleaseReg(int phy_id, LiveInterval interval) { 
    //TODO("ReleaseReg"); 
    for(auto it=phy_occupied[phy_id].begin();it!=phy_occupied[phy_id].end();++it)
    {
        if((*it).getReg() == interval.getReg())//找到要释放的活跃区间
        {
            phy_occupied[phy_id].erase(it);
            return true;
        }
    }
    return false; 
}

bool PhysicalRegistersAllocTools::OccupyMem(int offset, LiveInterval interval) {
    //TODO("OccupyMem");
    auto cur_vreg=interval.getReg();
    auto size=cur_vreg.getDataWidth()/4;
    for(int i=offset;i<offset+size;i++)
    {
        while(i>=mem_occupied.size()){mem_occupied.push_back({});}
        mem_occupied[i].push_back(interval);
    }
    return true;
}
bool PhysicalRegistersAllocTools::ReleaseMem(int offset, LiveInterval interval) {
    //TODO("ReleaseMem");
    auto cur_vreg=interval.getReg();
    auto size=cur_vreg.getDataWidth()/4;
    for(int i=offset;i<offset+size;i++)
    { 
        for(auto it=mem_occupied[i].begin();it!=mem_occupied[i].end();++it)
        {
            if((*it).getReg() == interval.getReg())
            {
                mem_occupied[i].erase(it);
                break;
            }
        }
    }
     return true;
}

int PhysicalRegistersAllocTools::getIdleReg(LiveInterval interval) {
    for(auto i:getValidRegs(interval))
    {
        int flag=true;
        for(auto conflict_j:getAliasRegs(i))//其实只是获取自己
        {
            for(auto other_interval:phy_occupied[conflict_j])//自己的活跃区间不会重叠
            {
                if(interval&other_interval)//检查活跃区间是否重叠
                {
                    flag=false;
                    break;
                }
            }
        }
        if(flag){return i;}
    }

    return -1;
}
int PhysicalRegistersAllocTools::getIdleReg(LiveInterval interval, std::vector<int> preferred_regs) {
    // 检查寄存器是否与任何别名寄存器的活跃区间冲突
    auto isRegisterConflicting = [this, &interval](int reg) {
        for (auto conflict_j : getAliasRegs(reg)) {
            for (auto& other_interval : phy_occupied[conflict_j]) {
                if (interval & other_interval) {
                    return true;
                }
            }
        }
        return false;
    };
    // 第一阶段：尝试首选寄存器
    for (int reg : preferred_regs) {
        if (!isRegisterConflicting(reg)) {
            return reg;
        }
    }
    // 收集已尝试的寄存器
    std::unordered_set<int> tried_regs(preferred_regs.begin(), preferred_regs.end());
    // 第二阶段：尝试有效寄存器（排除已尝试的）
    std::vector<int> valid_regs = getValidRegs(interval);
    for (int reg : valid_regs) {
        if (tried_regs.count(reg) == 0 && !isRegisterConflicting(reg)) {
            return reg;
        }
    }
    

    return -1; // 无可用寄存器
}

//reference:https://github.com/yuhuifishash/SysY/target/common/machine_passes/register_alloc/physical_register.cc line128-line152
int PhysicalRegistersAllocTools::getIdleMem(LiveInterval interval) //{ TODO("getIdleMem"); }
{
    std::vector<bool> ok;
    ok.resize(mem_occupied.size(),true);
    for(int i=0;i<mem_occupied.size();i++)
    {
        ok[i]=true;
        for(auto other_interval:mem_occupied[i])
        {
            if(interval&other_interval)
            {
                ok[i]=false;//该内存处不可使用
                break;
            }
        }
    }
    int free_cnt=0;
    for(int offset=0;offset<ok.size();offset++)
    {
        if(ok[offset]){free_cnt++;}
        else{free_cnt=0;}
        if(free_cnt==interval.getReg().getDataWidth()/4)
        {return offset-free_cnt+1;}//返回可用块的起始偏移量
    }
    return mem_occupied.size()-free_cnt;//返回最后的可用偏移
}

int PhysicalRegistersAllocTools::swapRegspill(int p_reg1, LiveInterval interval1, int offset_spill2, int size,
                                              LiveInterval interval2) {

    //TODO("swapRegspill");
    ReleaseReg(p_reg1,interval1);
    ReleaseMem(offset_spill2,interval2);
    OccupyReg(p_reg1,interval2);
    return 0;
}
std::vector<LiveInterval> PhysicalRegistersAllocTools::getConflictIntervals(LiveInterval interval) {
    std::vector<LiveInterval> result;
    for (auto phy_intervals : phy_occupied) {
        for (auto other_interval : phy_intervals) {
            if (interval.getReg().type == other_interval.getReg().type && (interval & other_interval)) {
                result.push_back(other_interval);
            }
        }
    }
    return result;
}
// 获取可分配的寄存器列表（不考虑区间冲突）
std::vector<int> RiscV64RegisterAllocTools::getValidRegs(LiveInterval interval) {
    if (interval.getReg().type.data_type == MachineDataType::INT) {
        return std::vector<int>({
        RISCV_t0, RISCV_t1, RISCV_t2, RISCV_t3, RISCV_t4, RISCV_t5,  RISCV_t6,  RISCV_a0, RISCV_a1, RISCV_a2,
        RISCV_a3, RISCV_a4, RISCV_a5, RISCV_a6, RISCV_a7, RISCV_s0,  RISCV_s1,  RISCV_s2, RISCV_s3, RISCV_s4,
        RISCV_s5, RISCV_s6, RISCV_s7, RISCV_s8, RISCV_s9, RISCV_s10, RISCV_s11, RISCV_ra,
        });
    } else if (interval.getReg().type.data_type == MachineDataType::FLOAT) {
        return std::vector<int>({
        RISCV_f0,  RISCV_f1,  RISCV_f2,  RISCV_f3,  RISCV_f4,  RISCV_f5,  RISCV_f6,  RISCV_f7,
        RISCV_f8,  RISCV_f9,  RISCV_f10, RISCV_f11, RISCV_f12, RISCV_f13, RISCV_f14, RISCV_f15,
        RISCV_f16, RISCV_f17, RISCV_f18, RISCV_f19, RISCV_f20, RISCV_f21, RISCV_f22, RISCV_f23,
        RISCV_f24, RISCV_f25, RISCV_f26, RISCV_f27, RISCV_f28, RISCV_f29, RISCV_f30, RISCV_f31,
        });
    } else {
        //ERROR("Unsupported Reg data type");
        return std::vector<int>();
    }
}    