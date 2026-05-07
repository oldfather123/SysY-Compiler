#include "regalloc.hpp"

#include "Function.hpp"
#include "Instruction.hpp"
#include "liverange.hpp"

#include <algorithm>

using std::for_each;

using namespace RA;

#define ASSERT_CMPINST_USED_ONCE(cmpinst) (assert(cmpinst->get_use_list().size() <= 1))

// int get_arg_id(Argument *arg) {
//     int id = 0;
//     for(auto &arg_test : arg->get_parent()->get_args()){
//         if(&arg_test == arg)
//             break;
//         ++id;
//     }
//     return id;
// }
int RegAllocator::get_arg_id(Argument *arg,bool is_float) {
    if(is_float){
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args()) {
            if (arg_test.get_name() == arg->get_name()) // 更保险
                break;
            if(arg_test.get_type()->is_float_type()) ++id;
        }
        return id;
    }else{
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args()) {
            if (arg_test.get_name() == arg->get_name()) // 更保险
                break;
            if(!arg_test.get_type()->is_float_type())++id;
        }
        return id;
    }
    return 0;
}
int get_arg_id_std(Argument *arg,bool is_float) {
    if(is_float){
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args()) {
            if (arg_test.get_name() == arg->get_name()) // 更保险
                break;
            if(arg_test.get_type()->is_float_type()) ++id;
        }
        return id;
    }else{
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args()) {
            if (arg_test.get_name() == arg->get_name()) // 更保险
                break;
            if(!arg_test.get_type()->is_float_type())++id;
        }
        return id;
    }
    return 0;
}

bool RegAllocator::no_reg_alloca(Value *v) {
    auto instr = dynamic_cast<Instruction *>(v);
    auto arg = dynamic_cast<Argument *>(v);
    int id;
    if (instr) {
        //TODO: 判断哪些指令需要分配寄存器
        switch (instr->get_instr_type()) {
        case Instruction::ret:
        case Instruction::br:
        case Instruction::store:
        case Instruction::alloca:
        //case Instruction::phi:
            return true;
        default:
            return false;
    }
    }
    if (arg) { // 只为前8个参数分配寄存器
        if(arg->get_type()->is_float_type()) id = get_arg_id_std(arg,true);
        else id = get_arg_id_std(arg,false);
        //std::cout << "[no_reg_alloca] " << arg->get_name() << " -> id: " << id << "\n";
        return id > ARG_MAX_R;
    } else
        assert(false && "only instruction and argument's LiveInterval exits");
}

void RegAllocator::reset(Function *func) {
    //TODO: 对相关参数进行初始化
    cur_func = func;
    regmap.clear();
    active.clear();
    std::fill(used, used + MAXR + 1, false);
    stack_slot.clear();
    stack_offset = 0;
    spill_count = 0;
}

void RegAllocator::ReserveForArg(const LVITS &liveints) {
    int float_arg_id = 1;
    int int_arg_id = 1;

    for (const auto &liveint : liveints) {
        auto *arg = dynamic_cast<Argument *>(liveint.second);
        
        //printf("\n666676\n");
        if (!arg)
            continue;
        //printf("\n66666\n");
        // --- 对参数分类只处理一次 ---
        //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" << "\n";
        if (FLOAT) {
            // 只处理真正 float 类型（不要包含 float*）
            if (!arg->get_type()->is_float_type())
                continue;
            int a=get_arg_id(arg,true)+1;
            if (float_arg_id <= ARG_MAX_R&&a<=8) {
                
                regmap[arg] = a;
                used[a] = true;
                active.insert(liveint);
                float_arg_id++;
            } else {
                stack_slot[arg] = stack_offset;
                stack_offset += 8;
            }
            //float_arg_id++;

        } else {
            // 整数寄存器处理 int 和 pointer 类型
            if (!arg->get_type()->is_integer_type() &&
                !arg->get_type()->is_pointer_type())
                continue;
            int a=get_arg_id(arg,false)+1;
            //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" << "\n";
            if (int_arg_id <= ARG_MAX_R&&a<=8) {
                
                regmap[arg] = a;
                used[a] = true;
                active.insert(liveint);
                int_arg_id++;
            } else {
                stack_slot[arg] = stack_offset;
                stack_offset += 8;
            }
            //printf("\n7776666\n");
            //printf("%d",int_arg_id);
            //int_arg_id++;
        }
    }
}


// input set is sorted by increasing start point
void RegAllocator::LinearScan(const LVITS &liveints, Function *func) {
    reset(func);
    ReserveForArg(liveints);
    int reg;
    for (auto liveint : liveints) {
        //TODO: 考虑有哪些情况的liveint不需要进行分析
        //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" << "\n";
        if (regmap.count(liveint.second)) {continue;}
        if (dynamic_cast<Argument *>(liveint.second)){continue;} 
        if (no_reg_alloca(liveint.second)) {continue;}
        if (is_spilled(liveint.second)) {continue;}
        if(FLOAT){
            if(!liveint.second->get_type()->is_float_type()){
                continue;
            }
        }else{           
            if(liveint.second->get_type()->is_float_type()){
                continue;
            }
        }
        //printf("1958");
        // if(auto *arg = dynamic_cast<Argument *>(liveint.second))
       // std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
       // printf("1958");
        ExpireOldIntervals(liveint);
        //printf("1958");
        //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
        if (active.size() == R){
            //printf("1958");
            SpillAtInterval(liveint);
            //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
        }else{
            //printf("1958");
            //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
            // auto instr = dynamic_cast<Instruction *>(liveint.second);
            // if (instr && instr->is_phi()) {
            // // phi 变量：不分配寄存器，直接分配栈空间
            //     stack_slot[liveint.second] = stack_offset;
            //     // std::cout << "[LinearScan] new alloc for " << liveint.second->get_name()
            //     //  << " → reg: " << "\n";
            //     stack_offset += 8;
            //     spill_count++;
            //     //printf("\n1958\n");
            //     // std::cout << "[LinearScan] phi spilled to stack: " << liveint.second->get_name()
            //     //         << ", offset: " << stack_slot[liveint.second] << "\n";
            //     continue;  // 不再往下处理
            // }
            reg=1;
            while ((reg <= R && used[reg])||liveint.first.k >= reg) {
                ++reg;
            }
            if (reg <= R) {  // 找到空闲寄存器
                used[reg] = true;
                regmap[liveint.second] = reg;
                active.insert(liveint);
                // std::cout << "[LinearScan] new alloc for " << liveint.second->get_name()
                //  << " → reg: " << reg << "\n";
            }else{
               // std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
                //printf("1958");
                only_SpillAtInterval(liveint);
            }
        }
       // std::cout << "[intervalmap] " << "19999" << " : " << "<" + to_string(22) + ">" +","<< "\n";
    }
    //std::cout << "[intervalmap] " << "19999" << " : " << "<" + to_string(22) + ">" +","<< "\n";
}

void RegAllocator::ExpireOldIntervals(LiveInterval liveint) {
    //TODO: 清除与当前的liveint已无交集的活跃变量
    //printf("1958");
    for (auto now=active.begin();now!=active.end();){
        const LiveInterval &iv = *now;
        //printf("%d and %d\n",iv.first.j,liveint.first.i);
        if (iv.first.j < liveint.first.i){
            int pass = regmap[iv.second];
            //printf("123456\n");
            used[pass] = false;
            now = active.erase(now);
        }else{
            ++now;
        }
    }
}

void RegAllocator::SpillAtInterval(LiveInterval liveint) {
    //TODO: 处理溢出情况
    //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
    auto spill = *active.rbegin(); // 最后一个，结束最晚
//std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
    if (spill.first.j > liveint.first.j) {
        // spill 原来的
        int r = regmap[spill.second];
        regmap.erase(spill.second);
        regmap[liveint.second] = r;
        active.erase(spill);
        active.insert(liveint);
        // spill spill.second 到栈上
        stack_slot[spill.second] = stack_offset;
        stack_offset += 8;
        spill_count++;
    } else {
        // spill 当前 liveint
        stack_slot[liveint.second] = stack_offset;
        stack_offset += 8;
        spill_count++;
        // 不分配寄存器，也不插入 active
    }
    //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
}
void RegAllocator::only_SpillAtInterval(LiveInterval liveint) {
    //TODO: 处理溢出情况
    //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
    //auto spill = *active.rbegin(); // 最后一个，结束最晚
    //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
    // spill 当前 liveint
        stack_slot[liveint.second] = stack_offset;
        stack_offset += 8;
        spill_count++;
     // 不分配寄存器，也不插入 active
    //std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
   
}
bool RegAllocator::is_spilled(Value *v){
    return stack_slot.find(v) != stack_slot.end();
}

int RegAllocator::get_stack_offset(Value *v) {
    auto it = stack_slot.find(v);
    if (it != stack_slot.end())
        return it->second;
    else {
        assert(false && "Variable has not been assigned a stack slot!");
        return -1;
    }
}
int RegAllocator::get_free_reg() {
    for (int r = 1; r <= R; ++r) {
        if (!used[r])
            return r;
    }
    return -1;  // 没有空闲寄存器
}
void RegAllocator::alloc_stack_slot(Value *v) {
    // 若未分配则分配（避免重复）
    if (stack_slot.find(v) == stack_slot.end()) {
        stack_slot[v] = stack_offset;
        stack_offset += 8;  // 默认一个变量 8 字节，可按类型调整
    }
}
int RegAllocator::get_stack_size() const {
    int max_offset = 0;
    for (const auto& [val, offset] : stack_slot) {
        if (offset > max_offset) {
            max_offset = offset;
        }
    }
    return max_offset;
}


//TODO: 对框架不满可尽情修改