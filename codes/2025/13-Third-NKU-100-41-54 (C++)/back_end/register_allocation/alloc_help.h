#ifndef ALLOC_HELP_H
#define ALLOC_HELP_H
#include"../basic/machine.h"
#include<set>
#include"../inst_process/inst_select/inst_select.h"
//指令类，记录单条机器指令及其是否在块首
//机器指令有ARM,RISCV,PHI
struct InstructionNumberEntry {
public:
    MachineBaseInstruction *ins;
    bool is_block_begin;
    InstructionNumberEntry() : ins(nullptr), is_block_begin(true) {}
    InstructionNumberEntry(MachineBaseInstruction *ins, bool isbegin) : ins(ins), is_block_begin(isbegin) {}
};

// 记录单项分配结果
//记录物理寄存器编号、是否在内存中，以及相对偏移量
struct AllocResult {
    bool in_mem;
    union {
        int phy_reg_no;
        int stack_offset;    // 这里的offset表示偏移, 相对什么位置及单位可以自行决定
    };
    AllocResult() : in_mem(false) { phy_reg_no = 0; }
    AllocResult(const struct AllocResult &other) {
        in_mem = other.in_mem;
        phy_reg_no = other.phy_reg_no;
    }
};
class LiveInterval {
private:
    Register reg;
    // 当begin和end不同时, 活跃区间为[begin,end), 即左闭右开
    // 当begin和end相同时, 表示[begin,end], 即一个单点 (这么做的原因是方便活跃区间计算)
    // 注意特殊判断begin和end相同时的情况
    struct LiveSegment {
        int begin;
        int end;
        bool inside(int pos) const {
            if (begin == end) return begin == pos;
            return begin <= pos && pos < end; 
        }
        bool operator&(const struct LiveSegment &that) const {
            return this->inside(that.begin) || this->inside(that.end - 1 > that.begin ? that.end - 1 : that.begin) ||
                    that.inside(this->begin) || that.inside(this->end - 1 > this->begin ? this->end - 1 : this->begin);
        }
        bool operator==(const struct LiveSegment &that) const {
            return this->begin == that.begin && this->end == that.end;
        }
    };
    std::list<LiveSegment> segments{};
    int reference_count;

public:
    // 检测两个活跃区间是否重叠
    // 保证两个活跃区间各个段各自都是不降序（升序）排列的
    // reference https://github.com/yuhuifishash/SysY/target/common/machine_passes/register_alloc/liveinterval.h line 64-line93
    bool operator&(const LiveInterval &that) const {
        //1.其中任一活跃区间为空，直接返回不重叠
        if(segments.empty()||that.segments.empty()){return false;}
        auto it=segments.begin();
        auto jt=that.segments.begin();
        while(1)
        {
            if(*it&*jt){return true;}//如果两个段有重叠
            if(it->end <= jt->begin)//it的活跃段早于jt的活跃段
            {
                ++it;//判断it的下一段
                if(it==segments.end()){return false;}
            }
            else if(jt->end<=it->begin)
            {
                ++jt;
                if(jt==that.segments.end()){return false;}
            }
            else{
                ERROR("LiveInterval::operator&: Error");
            }
        }
        return false;
    }
    LiveInterval operator|(const LiveInterval &that) const {
        LiveInterval ret(this->reg);
        ret.reference_count = this->reference_count + that.reference_count - 2;
        auto it = segments.begin();
        auto jt = that.segments.begin();
        while (1) {
            if (it == segments.end() && jt == that.segments.end()) {
                break;
            }
            if (it == segments.end()) {
                ret.segments.push_back(*jt);
                ++jt;
                continue;
            }
            if (jt == that.segments.end()) {
                ret.segments.push_back(*it);
                ++it;
                continue;
            }
            if (it->begin < jt->begin) {
                ret.segments.push_back(*it);
                ++it;
            } else {
                ret.segments.push_back(*jt);
                ++jt;
            }
        }
        return ret;
    }

    // 更新引用计数
    void IncreaseReferenceCount(int loop_depth) { 
        if(loop_depth==0){ 
            reference_count+=1;
        }else{
            reference_count += ((1 << (std::min(5,loop_depth))) + loop_depth +1);
        }
    }
    int getReferenceCount() { return reference_count; }
    // 返回活跃区间长度
    int getIntervalLength() {
        int ret = 0;
        for (auto seg : segments) {
            ret += (seg.end - seg.begin + 1);
        }
        return ret;
    }
    Register getReg() { return reg; }
    LiveInterval() : reference_count(0) {}    // Temp
    LiveInterval(Register reg) : reg(reg), reference_count(0) {}

    void PushFront(int begin, int end) { segments.push_front({begin = begin, end = end}); }
    void SetMostBegin(int begin) { segments.begin()->begin = begin; }

    // 可以直接 for(auto segment : liveinterval)
    decltype(segments.begin()) begin() { return segments.begin(); }
    decltype(segments.end()) end() { return segments.end(); }
};

class Liveness {
private:
    MachineFunction *current_func;
    // 更新所有块DEF和USE集合
    void UpdateDefUse();
    // Key: Block_Number
    // 存储活跃变量分析的结果
    std::map<int, std::set<Register>> IN{}, OUT{}, DEF{}, USE{};

public:
    // 对所有块进行活跃变量分析并保存结果
    void Execute();
    Liveness(MachineFunction *mfun, bool calculate = true) : current_func(mfun) {
        if (calculate) {
            Execute();
        }
    }
    // 获取基本块的IN/OUT/DEF/USE集合
    std::set<Register> GetIN(int bid) { return IN[bid]; }
    std::set<Register> GetOUT(int bid) { return OUT[bid]; }
    std::set<Register> GetDef(int bid) { return DEF[bid]; }
    std::set<Register> GetUse(int bid) { return USE[bid]; }
};
// 维护物理寄存器以及溢出寄存器对内存的占用情况
class PhysicalRegistersAllocTools {
private:
protected:
    // 物理寄存器占用情况
    // phy_occupied[phy_id] = {interval1, interval2, ...} 表示物理寄存器phy_id在interval1, interval2...中被占用
    std::vector<std::vector<LiveInterval>> phy_occupied;

    // 内存占用情况
    // mem_occupied[offset] = {interval1, interval2, ...} 表示内存offset在interval1, interval2...中被占用
    std::vector<std::vector<LiveInterval>> mem_occupied;

    // 由区间获取所有合法的物理寄存器(不考虑活跃区间是否冲突)
    virtual std::vector<int> getValidRegs(LiveInterval interval) = 0;

    // 由物理寄存器获取所有的别名物理寄存器
    // 例子1：RISCV中，a0是a0的别名；RISCV中，只有寄存器和寄存器自己构成别名
    // 例子2：x86中，eax的别名有：eax, ax, al, ah
    // 例子3：ARM中，q0的别名有：q0,d0,d1,s0,s1,s2,s3
    virtual std::vector<int> getAliasRegs(int phy_id) = 0;

public:
    // 清空占用状态
    virtual void clear() {
        phy_occupied.clear();
        mem_occupied.clear();
    }

    // 将区间inteval分配到物理寄存器phy_id,返回是否成功
    bool OccupyReg(int phy_id, LiveInterval interval);
    // 释放物理寄存器,返回是否成功
    bool ReleaseReg(int phy_id, LiveInterval interval);

    // 将区间inteval分配到内存,返回是否成功
    bool OccupyMem(int offset, LiveInterval interval);
    // 释放内存,返回是否成功
    bool ReleaseMem(int offset, LiveInterval interval);

    // 获取空闲的（活跃区间不冲突的）物理寄存器, 返回物理寄存器编号
    int getIdleReg(LiveInterval interval);
    int getIdleReg(LiveInterval interval, std::vector<int> preferred_regs);
    //暂时修改
    // int getIdleReg(LiveInterval interval, std::vector<int> preferd_regs,
    //                               std::vector<int> noprefer_regs);
    // 获取空闲的（活跃区间不冲突的）内存, 返回栈上的offset
    int getIdleMem(LiveInterval interval);

    // 交换分配结果（必须是一个溢出，一个不溢出), 用于在线性扫描的过程中选择溢出寄存器
    int swapRegspill(int p_reg1, LiveInterval interval1, int offset_spill2, int size, LiveInterval interval2);

    // 获得所有活跃区间与interval冲突的、已经分配在同数据类型的物理寄存器中的活跃区间
    // 用于在寄存器分配过程中选择溢出区间
    std::vector<LiveInterval> getConflictIntervals(LiveInterval interval);

    // 获取所有溢出寄存器占用内存大小之和
    int getSpillSize() {
        return mem_occupied.size()*4;
    }
};
class RiscV64RegisterAllocTools : public PhysicalRegistersAllocTools {
protected:
    std::vector<int> getValidRegs(LiveInterval interval);
    std::vector<int> getAliasRegs(int phy_id) { return std::vector<int>({phy_id}); }

public:
    RiscV64RegisterAllocTools() { phy_occupied.resize(64); }
    void clear() {
        phy_occupied.clear();
        Assert(phy_occupied.empty());
        phy_occupied.resize(64);
        mem_occupied.clear();
        Assert(mem_occupied.empty());
    }
};
    
#endif

