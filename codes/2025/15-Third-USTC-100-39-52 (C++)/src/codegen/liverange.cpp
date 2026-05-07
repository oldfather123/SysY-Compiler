#include "liverange.hpp"

#include "BasicBlock.hpp"
#include "Function.hpp"
#include "logging.hpp"

#include <deque>
#include <unordered_set>
#include <algorithm>
#include <iostream>
using namespace std;
using namespace LRA;
enum class Color { White, Gray, Black };
std::map<BasicBlock*, Color> colors;
void LiveRangeAnalyzer::clear()
{
    // TODO:每次活跃变量分析前清空定义的变量
    intervalmap.clear();
    IN.clear();
    OUT.clear();
    BB_DFS_Order.clear();
    liveIntervals.clear();
    phi_map.clear();      // 清空 phi_map
    instr_id.clear();     // 清空指令 ID
    circle_range.clear(); // 清空循环区间
    circles.clear();      // 清空循环信息
    colors.clear();       // 清空颜色标记
    cntline = 0;
    call_info.clear();
    phi_defs.clear();
}

LiveSet LiveRangeAnalyzer::joinFor(BasicBlock *bb) {
    LiveSet out;

    // 合并所有后继块的 IN 集合
    // for (auto succ : bb->get_succ_basic_blocks()) {
    //     auto &irs = succ->get_instructions();
    //     auto it = irs.begin();
    //     while (it != irs.end() && it->is_phi()) {
    //         ++it;
    //     }
    //     assert(it != irs.end());
    //     auto *first_instr = &(*it);
    //     int in_id = instr_id[first_instr] - 1;
    //     // std::cout << "[joinFor] Current block: " << bb->get_name()
    //     //       << ", Successor: " << succ->get_name()
    //     //       << ", First non-phi instr: " << first_instr->get_name()
    //     //       << ", in_id: " << in_id << "\n";
    //    // printf("%d\n",in_id);

    //     union_ip(out, IN[in_id]);
    // }
    // 合并所有后继块的 IN 集合（去除 phi 使用变量）
    for (auto succ : bb->get_succ_basic_blocks()) {
        auto &irs = succ->get_instructions();
        auto it = irs.begin();
        
        // 收集所有 phi 指令使用的变量
        //std::unordered_set<Value*> phi_used_vars;
        while (it != irs.end() && it->is_phi()) {
            ++it;
        }

        // 找到第一个非 phi 指令，获取其 in_id
        assert(it != irs.end());
        auto *first_instr = &(*it);
        int in_id = instr_id[first_instr] - 1;

        // 创建一个新的临时集合，存放非-phi变量
        LiveSet filtered_in;
        for (auto val : IN[in_id]) {
            if (phi_defs.count(val) == 0) {
                filtered_in.insert(val);
            }
        }

        // 合并进 out 集合
        union_ip(out, filtered_in);
    }
    // 加入当前块作为前驱的 phi-use，只加入后继块是当前块后继的 use
    //std::cout << "[joinFor] checking phi_map[" << bb->get_name() << "]\n";
    auto phi_it = phi_map.find(bb);
    if (phi_it != phi_map.end()) {
        for (auto &[def, use, succ] : phi_it->second) {
            // 只有当后继是当前块的后继时才加入
            if (std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ)
                != bb->get_succ_basic_blocks().end()) {
                    if (use != nullptr && !use->get_name().empty()) {
                        out.insert(use);
                        out.insert(def);
                        // std::cout << "  phi use: " << use->get_name()
                        // << " (for def " << def->get_name()
                        // << ", succ " << succ->get_name() << ")\n";
                    } else {
                        // std::cout << "  phi use is empty or null, skipped (for def " << def->get_name() << ", succ " << succ->get_name() << ")\n";
                    }
            }
        }
    }

    return out;
}

void LiveRangeAnalyzer::make_id(Function *func)
{
    /*TODO: 按排序后的BB为所有的指令标定行号，这里建议指令行号是指令数的两倍，这样便于分辨指令的IN和OUT.
    由于已经进行了copystmt,所以phi语句无需进行标定*/
    cntline = 0; // 指令编号的基数
    for (auto bb : BB_DFS_Order) {
        for (auto &instr : bb->get_instructions()) {
            // if (instr.is_phi()){
            //     continue; // phi 已转化为 copy-stmt，跳过
            // }
            cntline++;
            instr_id[&instr] = cntline * 2; // OUT: 2n, IN: 2n - 1
            //std::cout << "[make_id] instr: " << instr.get_name() << " => ID: " << instr_id[&instr] << "\n";

            // 检查是否为调用指令
            if (instr.is_call()) {
                // 记录调用指令的位置和参数个数
                int num_args = instr.get_num_operand();  // 假设 get_num_args() 返回调用指令的参数个数
                call_info[&instr] = {cntline*2-1, num_args};  // 存储行号和参数个数
                //int a=cntline
                //printf("%d\n",cntline);
            }
        }
    }
}

void LiveRangeAnalyzer::reverse_post_order(BasicBlock *bb, BasicBlock *father) {
    if (colors[bb] == Color::Gray) {
        circles.emplace_back(bb, father);
        return;
    }
    if (colors[bb] == Color::Black) {
        return;
    }
    colors[bb] = Color::Gray;

    // for (auto succ : bb->get_succ_basic_blocks()) {
    //     reverse_post_order(succ, bb);
    // }
    for (auto succ_iter = bb->get_succ_basic_blocks().rbegin(); succ_iter != bb->get_succ_basic_blocks().rend(); ++succ_iter){
        //左 右 中，左是ret 右是circle
        //把顺序反过来，先中间，再circle，再return
        //拓扑序
        reverse_post_order(*succ_iter,bb);
    }
    colors[bb] = Color::Black;
    BB_DFS_Order.push_back(bb);
}

void LiveRangeAnalyzer::get_dfs_order(Function *func) {
    colors.clear();
    BB_DFS_Order.clear();

    reverse_post_order(func->get_entry_block(), nullptr);
    std::reverse(BB_DFS_Order.begin(), BB_DFS_Order.end());

    // std::cout << "[RPO] Order: ";
    // for (auto *bb : BB_DFS_Order) {
    //     std::cout << bb->get_name() << " ";
    // }
    // std::cout << std::endl;
}



bool LiveRangeAnalyzer::in_loop(int instr_id) {
    for (auto &[start, end] : circle_range) {
        if (instr_id >= start && instr_id <= end)
            return true;
    }
    return false;
}



LiveSet LiveRangeAnalyzer::transferFunction(Instruction *instr)
{
    /*TODO: 在平时讲述的活跃变量分析中我们是对每个块计算 IN[B] = use + (OUT[B] - def), 但在具体程序流分析中我们可以对每条语句进行计算来达到等价的效果
    这个函数就用于对每条语句计算instr的 in = use + out - def
    思考对于instr来说它的use和def是什么?这样做相对于对整个BasicBlock计算有什么好处?
    */
    LiveSet in, use, def;

    // 如果这条指令有结果值（非 void），它定义了这个结果
    if (!instr->is_void()) {
        def.insert(instr);
    }

    // 遍历所有操作数，构建 use 集合
    for (auto &operand : instr->get_operands()) {
        Value *val = operand;
        // 忽略常量、函数名、全局变量、基本块标签等非变量引用
        if (dynamic_cast<Function *>(val) ||
            dynamic_cast<GlobalVariable *>(val) ||
            dynamic_cast<Constant *>(val) ||
            dynamic_cast<BasicBlock *>(val)) {
            continue;
        }
        use.insert(val);
    }

    // 计算 OUT - DEF
    std::set_difference(
        OUT[instr_id[instr]].begin(), OUT[instr_id[instr]].end(),
        def.begin(), def.end(),
        std::inserter(in, in.begin()));

    // 再将 USE 合入 IN
    union_ip(in, use);

    return in;
}


void LiveRangeAnalyzer::build_phi_map(Function *func) {
    //std::cout << "[build_phi_map] called\n";
    for (auto &bb : func->get_basic_blocks()) {
        //std::cout << "  block: " << bb.get_name() << "\n";
        for (auto &instr : bb.get_instructions()) {
            // std::cout << "    instr: " << instr.get_name()
            //   << ", is_phi: " << instr.is_phi()
            //   << ", typeid: " << typeid(instr).name() << "\n";

            if (!instr.is_phi()) {
                //std::cout << "    non-phi instr: " << instr.get_name() << ", break\n";
                break;
            }
            auto *phi = dynamic_cast<PhiInst *>(&instr);
            assert(phi && "phi instruction must be PhiInst");
            //std::cout << "phi incoming size: " << phi->get_incoming().size() << "\n";
            Value *def = phi;
            //printf("\n22\n");
            // for (auto &[val, pred_bb] : phi->get_incoming()) {
            //     //printf("\n33\n");
            //     phi_map[pred_bb].emplace_back(def, val, &bb);
            //     std::cout << "    [phi_map] pred=" << pred_bb->get_name()
            //               << ", def=" << def->get_name()
            //               << ", use=" << val->get_name()
            //               << ", succ=" << bb.get_name() << "\n";
            // }
            for(int i=0;i<phi->get_num_operand();i+=2){
                auto *val=phi->get_operand(i);
                auto *pred_bb=dynamic_cast<BasicBlock *>(phi->get_operand(i+1));
                phi_map[pred_bb].emplace_back(def, val, &bb);
                // std::cout << "    [phi_map] pred=" << pred_bb->get_name()
                //           << ", def=" << def->get_name()
                //           << ", use=" << val->get_name()
                //           << ", succ=" << bb.get_name() << "\n";
            }
            phi_defs.insert(def);
        }
    }
}

void LiveRangeAnalyzer::run(Function *func)
{
    clear();
    get_dfs_order(func);
    //make_id(func);       // 给指令分配编号：OUT = 2n，IN = 2n-1
    build_phi_map(func);
    make_id(func);
     // 打印 phi_map 内容，方便调试
    // std::cout << "===== phi_map 内容 =====\n";
    // for (auto &[pred, vec] : phi_map) {
    //     std::cout << "phi_map for pred block: " << pred->get_name() << "\n";
    //     for (auto &[def, use, succ] : vec) {
    //         std::cout << "  def=" << def->get_name()
    //                   << ", use=" << use->get_name()
    //                   << ", succ=" << succ->get_name() << "\n";
    //     }
    // }
    // std::cout << "======================\n"; 

    bool cont = true;
    while (cont)
    {
        cont = false;

        // 逆序遍历基本块和指令（活跃分析是逆向）
        for (auto rit_bb = BB_DFS_Order.rbegin(); rit_bb != BB_DFS_Order.rend(); ++rit_bb)
        {
            auto bb = *rit_bb;
            auto &instrs = bb->get_instructions();
            auto* phiend =&(*instrs.rbegin());
            Instruction *first_inst = &instrs.front();
            for (auto rit = instrs.rbegin(); rit != instrs.rend(); ++rit)
            {
                Instruction *instr = &(*rit);
                if (instr->is_phi())
                    continue;  // phi 已转为copy-stmt，不参与此处分析
                //std::cout << "[Run] instr: " << instr->get_name() << std::endl;
                int out_id = instr_id.at(instr);    // 指令的 OUT 编号，偶数
                int in_id = out_id - 1;              // 指令的 IN 编号，奇数

                if (instr->is_ret() || instr->is_br())
                {
                    // OUT = 后继指令的IN合并（joinFor）
                    OUT[out_id] = joinFor(bb);
                    // // 特殊处理 copy-stmt 对phi定义的影响
                    // auto phi_it = phi_map.find(bb);
                    // if (phi_it != phi_map.end())
                    // {
                    //     for (auto &[def, use] : phi_it->second)
                    //     {
                    //         OUT[out_id].insert(use);
                    //         OUT[out_id].erase(def);
                    //     }
                    // }
                   // printf("\n999\n");
                }
                else
                {
                    // 普通指令：OUT = 下一条指令的IN
                    // 下一条指令的OUT编号是 out_id + 2，IN编号是 out_id + 1
                    OUT[out_id] = IN[out_id + 1];
                }

                // 计算当前指令的IN集合（活跃变量传递）
                LiveSet new_in = transferFunction(instr);
                if (IN[in_id] != new_in)
                {
                    IN[in_id] = new_in;
                    cont = true;
                }

                if(rit == first_inst)
                {
                  //  printf("8765");
                    for (auto val : IN[in_id]){
                        intervalmap[val].i = std::min(intervalmap[val].i == UNINITIAL ? in_id : intervalmap[val].i, in_id);
                        if(auto* end=dynamic_cast<PhiInst *>(val)){
                            OUT[out_id].insert(val);
                            //std::cout  << "use6666=" << val->get_name() << "\n";
                        }
                    }
                    for (auto val : OUT[out_id]){
                        if(auto* end=dynamic_cast<PhiInst *>(val)){
                        // intervalmap[val].j = std::max(intervalmap[val].j, out_id);
                            int phiend1 = instr_id.at(phiend);
                            intervalmap[val].j = std::max(intervalmap[val].j, phiend1);
                            //std::cout  << "use=" << val->get_name() << "\n";
                           // printf("%d\n",phiend1);
                        }
                        else{
                            intervalmap[val].j = std::max(intervalmap[val].j, out_id);
                        }
                    }
                }else{
                    // 更新活跃区间的起止点
                    for (auto val : IN[in_id])
                        intervalmap[val].i = std::min(intervalmap[val].i == UNINITIAL ? in_id : intervalmap[val].i, in_id);
                    for (auto val : OUT[out_id]){
                        if(auto* end=dynamic_cast<PhiInst *>(val)){
                        // intervalmap[val].j = std::max(intervalmap[val].j, out_id);
                            int phiend1 = instr_id.at(phiend);
                            intervalmap[val].j = std::max(intervalmap[val].j, phiend1);
                            //std::cout  << "use=" << val->get_name() << "\n";
                            //printf("%d\n",phiend1);
                        }
                        else{
                            intervalmap[val].j = std::max(intervalmap[val].j, out_id);
                        }
                    }
                }
                // std::cout << "[ANALYZE] instr_id: " << out_id << " (" << instr->get_name() << ")\n";
                // std::cout << "  IN[" << in_id << "] = " << print_liveSet(IN[in_id]) << "\n";
                // std::cout << "  OUT[" << out_id << "] = " << print_liveSet(OUT[out_id]) << "\n";
            }
        }
    }

    // 将函数参数加入IN[0]
    assert(IN.find(0) == IN.end() && OUT.find(0) == OUT.end() && "no instr_id mapped to 0");
    IN[0] = OUT[0] = {};
    for (auto &arg : func->get_args())
        IN[0].insert(&arg);
    // for (auto &[val, interval] : intervalmap)
    //     {
    //      std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
    // }
    make_interval(func);
    // for (auto &[val, interval] : intervalmap)
    // {
    //      std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
    // }
}
bool instr_is_within_interval(int id, const Interval& interval) {
    //int instr_position = instr->get_position();  // 获取指令的位置信息

    // 判断指令的位置是否在区间 [i, j] 之间
    return (((id >= interval.i||(id >= (interval.i-1))) && id <= interval.j));
}

void LiveRangeAnalyzer::make_interval(Function *func)
{
    //intervalmap.clear();
    //std::cout << "[make_interval] called, cntline=" << cntline << std::endl;

    // 遍历每条指令，统计 IN 和 OUT 的活跃集合
    for (int i = 1; i <= cntline; ++i)
    {
        int out_id = 2 * i;       // OUT编号，偶数
        int in_id = out_id - 1;   // IN编号，奇数

        //std::cout << "[make_interval] i=" << i << ", IN[" << in_id << "] size=" << IN[in_id].size()
        //          << ", OUT[" << out_id << "] size=" << OUT[out_id].size() << std::endl;

        for (auto &val : IN[in_id])
        {
            if (intervalmap.find(val) == intervalmap.end()){
                intervalmap[val] = Interval(in_id, in_id);
            }
            else{
                intervalmap[val].j = std::max(intervalmap[val].j, in_id);
                // std::cout  << "use1=" << val->get_name()<<" " << to_string(intervalmap[val].j) << "\n";
            }
        }

        for (auto &val : OUT[out_id])
        {
            if (intervalmap.find(val) == intervalmap.end()){
                intervalmap[val] = Interval(out_id, out_id);
            } else{
                intervalmap[val].j = std::max(intervalmap[val].j, out_id);
               // std::cout  << "use=" << val->get_name() <<" "<< to_string(intervalmap[val].j) << "\n";
            } 
        }
    }
    // for (auto &[val, interval] : intervalmap)
    // {
    //      std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
    // }
     for (auto &[val, interval] : intervalmap)
    {
        if (auto phi=dynamic_cast<PhiInst *>(val)) {
                int num=phi->get_num_operand()/2;
                int id[num],i=0;
                // for (auto &[val, pred_bb] : phi->get_incoming()) {
                //     //phi_map[pred_bb].emplace_back(def, val, &bb);
                //     auto &instrs = pred_bb->get_instructions();
                //     auto rit = instrs.rbegin();
                //    // auto val1=dynamic_cast<Instruction *>(val);
                //     Instruction *instr = &(*rit);
                //     id[i]=instr_id[instr]-1;
                //     string pass=to_string(interval.i);
                //     i++;
                // }
                for(int j=0;j<phi->get_num_operand();j+=2){
                    //auto *val=phi->get_operand(i);
                    auto *pred_bb=dynamic_cast<BasicBlock *>(phi->get_operand(j+1));
                    auto &instrs = pred_bb->get_instructions();
                    auto rit = instrs.rbegin();
                   // auto val1=dynamic_cast<Instruction *>(val);
                    Instruction *instr = &(*rit);
                    id[i]=instr_id[instr]-1;
                    i++;
                    //string pass=to_string(interval.i);
                    //i++;
                }
                if(intervalmap[phi].i!=-1) {
                    //int arr[] = {5, 2, 8, 1, 4};
                    int n = sizeof(id) / sizeof(id[0]);
                    int min_val = *min_element(id, id + n);
                    intervalmap[phi].i = min_val;
                   // printf(" %d\n",intervalmap[phi].j);
                    int j=0;
                    while(j<num){
                       // printf("%d\n",id[j]);
                        if(id[j]>intervalmap[phi].j){
                            intervalmap[phi].j=id[j];
                        }
                        j++;
                    }
                    //printf(" %d\n",intervalmap[phi].j);
                    // if(id[0]>intervalmap[phi].j){
                    //     intervalmap[phi].j=id[0];
                    // }
                    // if(id[1]>intervalmap[phi].j){
                    //     intervalmap[phi].j=id[1];
                    // }
                    //intervalmap[phi].j+=2;
                }
                //intervalmap[&instr].j = id; // 初步范围，后续会再更新 j
        }
          //std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
    }
    // 延长循环中的变量活跃区间
    for (auto &[loop_header, loop_exit] : circles)
    {
        auto first_ir = &(*loop_header->get_instructions().begin());
        // while (first_ir->is_phi())
        //     first_ir++;  // 跳过 phi
        auto last_ir = &(*loop_exit->get_instructions().rbegin());

        int loop_start = instr_id[first_ir] - 1;  // IN编号
        int loop_end = instr_id[last_ir];         // OUT编号

        circle_range.emplace_back(loop_start, loop_end);
    }

    // 延长循环内的活跃变量区间
    for (auto &[val, interval] : intervalmap)
    {
        for (auto &[c_start, c_end] : circle_range)
        {
            if (interval.i < c_start && interval.j >= c_start && interval.j < c_end)
            {
                interval.j = c_end;
            }
        }
    }

    // ✅ 修正：裁剪 interval.j，不允许超过程序内最大非 phi 指令编号
    // int max_instr_id = 0;
    // for (auto &[val, id] : instr_id)
    // {
    //     if (!val->is_phi()) {
    //         max_instr_id = std::max(max_instr_id, id);
    //     }
    // }

    //std::cout << "[make_interval] max_instr_id = " << max_instr_id << "\n";

    // for (auto &[val, interval] : intervalmap)
    // {
    //     if (interval.j > max_instr_id)
    //     {
    //         // std::cout << "[Adjust] " << val->get_name()
    //         //           << " 活跃区间 j=" << interval.j
    //         //           << " 超过 max_instr_id=" << max_instr_id
    //         //           << "，已裁剪为 " << max_instr_id << "\n";
    //         interval.j = max_instr_id;
    //     }
    // }
    // // 构建最终集合
    // liveIntervals.clear();
    // for (auto &[val, itv] : intervalmap) {
    //     // 特殊处理phi指令：活跃区间从定义点开始
    //     auto *instr =dynamic_cast<Instruction*>(val);
    //     if (instr && instr->is_phi()) {
    //         // 找到定义该phi的基本块的第一条非phi指令
    //         auto *bb = instr->get_parent();
    //         for (auto &instr_in_bb : bb->get_instructions()) {
    //             if (!instr_in_bb.is_phi()) {
    //                 int start_id = instr_id[&instr_in_bb] - 1;
    //                 itv.i = std::max(itv.i, start_id);
    //                 break;
    //             }
    //         }
    //     }
        
    //     liveIntervals.insert({itv, val});
    // }
    // 构建最终集合
    // for (auto &[val, interval] : intervalmap)
    // {
    //     liveIntervals.insert({interval, val});
    // }
    for (auto &[val, interval] : intervalmap) {
        int max_param_value = 0; // 用来保存当前活跃区间中所有 call 指令的最大参数值
        interval.k=0;
        // 检查活跃区间是否包含 call 指令
        for (auto &[instr, args] : call_info) {
            // 假设 call_info 存储了每个指令的参数数量和最大参数值
            if (instr_is_within_interval(args.first, interval)) { // 判断 call 指令是否在活跃区间内
                // 获取当前 call 指令的参数最大值
                int current_max = args.second;
                max_param_value = std::max(max_param_value, current_max);
            }
        }
        
        // 更新 Interval 结构中的 k 值
        interval.k = max_param_value;
        liveIntervals.insert({interval, val});
        //std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +","+to_string(interval.k)<< "\n";
    }
    //std::cout << "[make_interval] intervalmap size = " << intervalmap.size() << std::endl;
    // for (auto liveint : liveIntervals) 
    // {
    //     std::cout << "[intervalmap] " << liveint.second->get_name() << " : " << "<" + to_string(liveint.first.i) + ", " + to_string(liveint.first.j) + ">" +","+to_string(liveint.first.k)<< "\n";
    // }
    
}


// // // TODO: 对框架不满可尽情修改
// #include "liverange.hpp"

// #include "BasicBlock.hpp"
// #include "Function.hpp"
// #include "logging.hpp"

// #include <deque>

// using namespace LRA;
// enum class Color { White, Gray, Black };
// std::map<BasicBlock*, Color> colors;
// void LiveRangeAnalyzer::clear()
// {
//     // TODO:每次活跃变量分析前清空定义的变量
//     intervalmap.clear();
//     IN.clear();
//     OUT.clear();
//     BB_DFS_Order.clear();
//     liveIntervals.clear();
//     phi_map.clear();      // 清空 phi_map
//     instr_id.clear();     // 清空指令 ID
//     circle_range.clear(); // 清空循环区间
//     circles.clear();      // 清空循环信息
//     colors.clear();       // 清空颜色标记
//     cntline = 0;
//     call_info.clear();
// }

// LiveSet LiveRangeAnalyzer::joinFor(BasicBlock *bb) {
//     LiveSet out;

//     // 合并所有后继块的 IN 集合
//     for (auto succ : bb->get_succ_basic_blocks()) {
//         auto &irs = succ->get_instructions();
//         auto it = irs.begin();
//         while (it != irs.end() && it->is_phi()) {
//             ++it;
//         }
//         assert(it != irs.end());
//         auto *first_instr = &(*it);
//         int in_id = instr_id[first_instr] - 1;
//         union_ip(out, IN[in_id]);
//     }

//     // // 加入当前块作为前驱的 phi-use，只加入后继块是当前块后继的 use
//     auto phi_it = phi_map.find(bb);
//     if (phi_it != phi_map.end()) {
//         for (auto &[def, use, succ] : phi_it->second) {
//             // 只有当后继是当前块的后继时才加入
//             if (std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ)
//                 != bb->get_succ_basic_blocks().end()) {
//                     if (use != nullptr && !use->get_name().empty()) {
//                         out.insert(use);
//                     }
//             }
//         }
//     }

//     return out;
// }

// void LiveRangeAnalyzer::make_id(Function *func)
// {
//     /*TODO: 按排序后的BB为所有的指令标定行号，这里建议指令行号是指令数的两倍，这样便于分辨指令的IN和OUT.
//     由于已经进行了copystmt,所以phi语句无需进行标定*/
//     cntline = 0; // 指令编号的基数
//     for (auto bb : BB_DFS_Order) {
//         for (auto &instr : bb->get_instructions()) {
//             // if (instr.is_phi()){
//             //     continue; // phi 已转化为 copy-stmt，跳过
//             // }
//             cntline++;
//             instr_id[&instr] = cntline * 2; // OUT: 2n, IN: 2n - 1

//             // 检查是否为调用指令
//             if (instr.is_call()) {
//                 // 记录调用指令的位置和参数个数
//                 int num_args = instr.get_num_operand();  // 假设 get_num_args() 返回调用指令的参数个数
//                 call_info[&instr] = {cntline*2-1, num_args};  // 存储行号和参数个数

//             }
//         }
//     }
// }

// void LiveRangeAnalyzer::reverse_post_order(BasicBlock *bb, BasicBlock *father) {
//     if (colors[bb] == Color::Gray) {
//         circles.emplace_back(bb, father);
//         return;
//     }
//     if (colors[bb] == Color::Black) {
//         return;
//     }
//     colors[bb] = Color::Gray;

//     for (auto succ : bb->get_succ_basic_blocks()) {
//         reverse_post_order(succ, bb);
//     }
//     colors[bb] = Color::Black;
//     BB_DFS_Order.push_back(bb);
// }

// void LiveRangeAnalyzer::get_dfs_order(Function *func) {
//     colors.clear();
//     BB_DFS_Order.clear();

//     reverse_post_order(func->get_entry_block(), nullptr);
//     std::reverse(BB_DFS_Order.begin(), BB_DFS_Order.end());

// }



// bool LiveRangeAnalyzer::in_loop(int instr_id) {
//     for (auto &[start, end] : circle_range) {
//         if (instr_id >= start && instr_id <= end)
//             return true;
//     }
//     return false;
// }



// LiveSet LiveRangeAnalyzer::transferFunction(Instruction *instr)
// {
//     /*TODO: 在平时讲述的活跃变量分析中我们是对每个块计算 IN[B] = use + (OUT[B] - def), 但在具体程序流分析中我们可以对每条语句进行计算来达到等价的效果
//     这个函数就用于对每条语句计算instr的 in = use + out - def
//     思考对于instr来说它的use和def是什么?这样做相对于对整个BasicBlock计算有什么好处?
//     */
//     LiveSet in, use, def;

//     // 如果这条指令有结果值（非 void），它定义了这个结果
//     if (!instr->is_void()) {
//         def.insert(instr);
//     }

//     // 遍历所有操作数，构建 use 集合
//     for (auto &operand : instr->get_operands()) {
//         Value *val = operand;
//         // 忽略常量、函数名、全局变量、基本块标签等非变量引用
//         if (dynamic_cast<Function *>(val) ||
//             dynamic_cast<GlobalVariable *>(val) ||
//             dynamic_cast<Constant *>(val) ||
//             dynamic_cast<BasicBlock *>(val)) {
//             continue;
//         }
//         use.insert(val);
//     }

//     // 计算 OUT - DEF
//     std::set_difference(
//         OUT[instr_id[instr]].begin(), OUT[instr_id[instr]].end(),
//         def.begin(), def.end(),
//         std::inserter(in, in.begin()));

//     // 再将 USE 合入 IN
//     union_ip(in, use);

//     return in;
// }


// void LiveRangeAnalyzer::build_phi_map(Function *func) {
//     for (auto &bb : func->get_basic_blocks()) {
//         for (auto &instr : bb.get_instructions()) {
//             if (!instr.is_phi()) {
//                 break;
//             }
//             auto *phi = dynamic_cast<PhiInst *>(&instr);
//             assert(phi && "phi instruction must be PhiInst");
//             Value *def = phi;
//             //printf("\n22\n");
//             for (auto &[val, pred_bb] : phi->get_incoming()) {
//                 phi_map[pred_bb].emplace_back(def, val, &bb);

//             }
//         }
//     }
// }

// void LiveRangeAnalyzer::run(Function *func)
// {
//     clear();
//     get_dfs_order(func);
//     //make_id(func);       // 给指令分配编号：OUT = 2n，IN = 2n-1
//     build_phi_map(func);
//     make_id(func);
//     bool cont = true;
//     //bool in_cg=false;
//    // bool out_cg=false;
//     while (cont)
//     {
//        // printf("1957\n");
//         cont = false;
//         // 逆序遍历基本块和指令（活跃分析是逆向）
//         for (auto rit_bb = BB_DFS_Order.rbegin(); rit_bb != BB_DFS_Order.rend(); ++rit_bb)
//         {
//             auto bb = *rit_bb;
//             auto &instrs = bb->get_instructions();
//             for (auto rit = instrs.rbegin(); rit != instrs.rend(); ++rit)
//             {
//                 bool in_cg=false;
//                 //bool out_cg=false;
//                 Instruction *instr = &(*rit);
//                 if (instr->is_phi())
//                     continue;  // phi 已转为copy-stmt，不参与此处分析
//                 int out_id = instr_id.at(instr);    // 指令的 OUT 编号，偶数
//                 int in_id = out_id - 1;              // 指令的 IN 编号，奇数
//                 if (instr->is_ret() || instr->is_br())
//                 {
//                     // OUT = 后继指令的IN合并（joinFor）
//                     OUT[out_id] = joinFor(bb);
//                     // // 特殊处理 copy-stmt 对phi定义的影响
//                 }
//                 else
//                 {
//                     // 普通指令：OUT = 下一条指令的IN
//                     // 下一条指令的OUT编号是 out_id + 2，IN编号是 out_id + 1
//                     OUT[out_id] = IN[out_id + 1];
//                 }
//                 // 计算当前指令的IN集合（活跃变量传递）
//                 LiveSet new_in = transferFunction(instr);
//                 if (IN[in_id] != new_in)
//                 {
//                     IN[in_id] = new_in;
//                     cont = true;
//                     in_cg=true;
//                     //printf("222\n");
//                 }
//                 if(in_cg){
//                 // 更新活跃区间的起止点
//                 for (auto val : IN[in_id])
//                      intervalmap[val].i = std::min(intervalmap[val].i == UNINITIAL ? in_id : intervalmap[val].i, in_id);
//                 }
//                 for (auto val : OUT[out_id]){
//                     intervalmap[val].j = std::max(intervalmap[val].j, out_id);
//                 }
                
//             }
//         }
//     }
//     // 将函数参数加入IN[0]
//     assert(IN.find(0) == IN.end() && OUT.find(0) == OUT.end() && "no instr_id mapped to 0");
//     IN[0] = OUT[0] = {};
//     for (auto &arg : func->get_args())
//         IN[0].insert(&arg);
//    // printf("9999\n");
//     // for (auto &[val, interval] : intervalmap)
//     // {
//     //      std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
//     // }
//     make_interval(func);
// }

// bool instr_is_within_interval(int id, const Interval& interval) {

//     // 判断指令的位置是否在区间 [i, j] 之间
//     return (((id >= interval.i||(id >= (interval.i-1))) && id <= interval.j));
// }

// void LiveRangeAnalyzer::make_interval(Function *func)
// {
//     intervalmap.clear();
//    // printf("8888\n");
//     // 遍历每条指令，统计 IN 和 OUT 的活跃集合
//     for (int i = 1; i <= cntline; ++i)
//     {
//         int out_id = 2 * i;       // OUT编号，偶数
//         int in_id = out_id - 1;   // IN编号，奇数

//         for (auto &val : IN[in_id])
//         {
//             if (intervalmap.find(val) == intervalmap.end())
//                 intervalmap[val] = Interval(in_id, in_id);
//             else
//                 intervalmap[val].j = std::max(intervalmap[val].j, in_id);
//         }

//         for (auto &val : OUT[out_id])
//         {
//             if (intervalmap.find(val) == intervalmap.end())
//                 intervalmap[val] = Interval(out_id, out_id);
//             else
//                 intervalmap[val].j = std::max(intervalmap[val].j, out_id);
//         }
//     }
//     // for (auto &bb : func->get_basic_blocks()) {
//     //     for (auto &instr : bb.get_instructions()) {
//     //         if (instr.is_phi()) {
//     //             auto id = instr_id[&instr]; // 即使 phi 不生成指令也可以假设个编号
//     //             if(intervalmap[&instr].i!=-1) intervalmap[&instr].i = id - 1;
//     //             //intervalmap[&instr].j = id; // 初步范围，后续会再更新 j
//     //         }
//     //     }
//     // }
//     for (auto &[val, interval] : intervalmap)
//     {
//         if (auto phi=dynamic_cast<PhiInst *>(val)) {
//                 int id[2],i=0;
//                 for (auto &[val, pred_bb] : phi->get_incoming()) {
//                     //phi_map[pred_bb].emplace_back(def, val, &bb);
//                     auto &instrs = pred_bb->get_instructions();
//                     auto rit = instrs.rbegin();
//                    // auto val1=dynamic_cast<Instruction *>(val);
//                     Instruction *instr = &(*rit);
//                     id[i]=instr_id[instr]-1;
//                     // std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(id[i])<< "\n";
//                     string pass=to_string(interval.i);
//                     i++;
//                 }
//                 if(intervalmap[phi].i!=-1) intervalmap[phi].i = std::min(id[0],id[1]);
//                 //intervalmap[&instr].j = id; // 初步范围，后续会再更新 j
//         }
//         //  std::cout << "[intervalmap] " << val->get_name() << " : " << "<" + to_string(interval.i) + ", " + to_string(interval.j) + ">" +to_string(interval.k)<< "\n";
//     }
//    // printf("4444\n");
//     // 延长循环中的变量活跃区间
//     for (auto &[loop_header, loop_exit] : circles)
//     {   
//        // printf("1958");
//         auto first_ir = &(*loop_header->get_instructions().begin());
//         while (first_ir->is_phi())
//             first_ir++;  // 跳过 phi
//         auto last_ir = &(*loop_exit->get_instructions().rbegin());

//         int loop_start = instr_id[first_ir] - 1;  // IN编号
//         int loop_end = instr_id[last_ir];         // OUT编号

//         circle_range.emplace_back(loop_start, loop_end);
//     }
//     //printf("3333\n");
//     // 延长循环内的活跃变量区间
//     for (auto &[val, interval] : intervalmap)
//     {
//         for (auto &[c_start, c_end] : circle_range)
//         {
//             if (interval.i < c_start && interval.j >= c_start && interval.j < c_end)
//             {
//                 interval.j = c_end;
//             }
//         }
//     }
//     //printf("2222\n");
//     for (auto &[val, interval] : intervalmap) {
//         int max_param_value = 0; // 用来保存当前活跃区间中所有 call 指令的最大参数值
//         interval.k=0;
//         // 检查活跃区间是否包含 call 指令
//         for (auto &[instr, args] : call_info) {
//             // 假设 call_info 存储了每个指令的参数数量和最大参数值
//             if (instr_is_within_interval(args.first, interval)) { // 判断 call 指令是否在活跃区间内
//                 // 获取当前 call 指令的参数最大值
//                 int current_max = args.second;
//                 max_param_value = std::max(max_param_value, current_max);
//             }
//         }
        
//         // 更新 Interval 结构中的 k 值
//         interval.k = max_param_value;
//         liveIntervals.insert({interval, val});
//     }
//    // printf("1111\n");
// //    for (const auto& interval : liveIntervals) {
// //         std::cout << "[intervalmap] " << interval.second->get_name() << " : " << "<" + to_string(interval.first.i) + ", " + to_string(interval.first.j) + ">" +to_string(interval.first.k)<< "\n";
// //     }
// }


// // TODO: 对框架不满可尽情修改
