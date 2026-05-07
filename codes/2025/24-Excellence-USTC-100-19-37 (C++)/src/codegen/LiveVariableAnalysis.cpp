#include "LiveVariableAnalysis.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "Value.hpp"
#include "User.hpp"
#include "BasicBlock.hpp"

#include <algorithm>


void LiveVariableAnalysis::clear() {
    live_ranges.clear();
    visited.clear();
    circles.clear();
    circle_intervals.clear();
    bb_order.clear();
    prev_bb = nullptr;
    inst_to_index.clear();
}

void LiveVariableAnalysis::run() {
    clear();
    for (auto &func : m->get_functions()) {
        analyze_function(&func);
    }
}

// 给无跳转的基本块加上后继（它的下一个块）
void LiveVariableAnalysis::prepare_bb(Function *f) {
    auto &bbs = f->get_basic_blocks();
    for (auto it = bbs.begin(); it != bbs.end(); ++it) {
        BasicBlock *bb = &*it;
        if (bb->is_terminated()) continue;
        auto next_it = it;
        ++next_it;
        if (next_it != bbs.end()) {
            BasicBlock *next_bb = &*next_it;
            bb->add_succ_basic_block(next_bb);
            next_bb->add_pre_basic_block(bb);
        }
    }
}

// dfs 遍历基本块
void LiveVariableAnalysis::dfs_basic_block(BasicBlock *bb) {
    if (visited[bb]) {
        auto cir = std::pair<BasicBlock *, BasicBlock *>(bb, prev_bb);
        if (std::find(circles.begin(), circles.end(), cir) == circles.end()) {
            circles.push_back(cir);
        }
    }
    visited[bb] = true;
    for (auto &succ : bb->get_succ_basic_blocks()) {
        prev_bb = bb;
        dfs_basic_block(succ);
    }
    bb_order.push_back(bb);
}

// dfs 遍历函数的基本块
void LiveVariableAnalysis::dfs_function(Function *f) {
    prepare_bb(f);
    clear();
    auto bb = f->get_entry_block();
    dfs_basic_block(bb);
    std::reverse(bb_order.begin(), bb_order.end());
    make_inst_index();

}
    
// 遍历指令，创建索引
void LiveVariableAnalysis::make_inst_index() {
    int index = 0;
    for (auto &bb : bb_order) {
        for (auto &inst : bb->get_instructions()) {
            inst_to_index[&inst] = index++;
        }
    }
}

// circle -> interval
void LiveVariableAnalysis::make_circle_intervals() {
    circle_intervals.clear();
    for (auto &circle : circles) {
        auto start_bb = circle.first;
        auto end_bb = circle.second;
        int start = -1, end = -1;
        auto start_inst = start_bb->get_instructions().begin();
        auto end_inst = end_bb->get_instructions().end();
        start = inst_to_index[&*start_inst];
        end = inst_to_index[&*--end_inst];
        start *= 2;
        end = end * 2 + 1;
        circle_intervals.push_back({start, end});
    }
}

void LiveVariableAnalysis::analyze_function(Function *f) {
    dfs_function(f);
    make_circle_intervals();
    // 给循环区间排序
    // 按 pair 的第一个元素升序排序（从小到大）,即按起始点排序
    std::sort(circle_intervals.begin(), circle_intervals.end(),
        [](const auto& a, const auto& b) {
            if (a.first == b.first) {
                // 如果起始点相同，按结束点升序排序，从小到大
                return a.second < b.second;
            }
            return a.first < b.first;
        }
    );
    for (auto &bb : bb_order) {
        for (auto it = bb->get_instructions().begin();
             it != bb->get_instructions().end(); ++it) {
            // analyze_instruction(&*it);
            auto inst = &*it;
            // if phi, merge
            // if (inst->is_phi()) {
            //     merge(inst);
            // }
            // get use list
            std::vector<int> use_indices;
            int start = inst_to_index[inst] * 2 + 1;
            int end = inst_to_index[inst] * 2 + 1;
            for (auto &use : inst->get_use_list()) {
                auto user = use.val_;
                if (user->is<Instruction>()) {
                    auto user_inst = user->as<Instruction>();
                    auto use_idx = inst_to_index[inst];
                    use_indices.push_back(use_idx * 2);
                    end = std::max(end, use_idx * 2);

                }
                
            }
            if (start == end) {
                // 如果没有使用，跳过
                continue;
            }
            // 检测循环，调整起止点、优先度
            LiveRange live_range = {start, end, 0, use_indices};
            // 检查循环
            check_instruction(inst);
            live_ranges[inst] = {start, end, 0, use_indices};
            
        }
    }
}

// 指令检测循环，调整起止点、优先度
void LiveVariableAnalysis::check_instruction(Instruction *inst) {
    auto it = live_ranges.find(inst);
    if (it == live_ranges.end()) {
        return; // 指令不在索引中
    }
    auto &live_range = it->second;
    int start = live_range.start;
    int end = live_range.end;
    auto uses = live_range.uses;
    int last_circle_end = -1;
    int priority_circle = 1;
    for (const auto &interval : circle_intervals) {
        if (interval.first > end) {
            break; // 之后的循环区间都不可能影响当前指令
        }
        if (interval.second < start) {
            continue; // 之前的循环区间不影响当前指令
        }
        // 如果循环区间与指令的 uses 有重叠
        bool used = false;
        for (const auto &use : uses) {
            if (use >= interval.first && use <= interval.second) {
                used = true;
                break;
            }
        }
        if (!used) {
            continue; // 如果循环区间没有使用该指令，跳过
        }
        start = std::min(start, interval.first);
        end = std::max(end, interval.second);

        // 检测是否是内层循环
        if (last_circle_end != -1 && interval.first <= last_circle_end) {
            // 如果当前循环的起始点小于等于上一个循环的结束点，说明是内层循环
            priority_circle *= 10; // 优先级乘以10
        } else {
            priority_circle = 1; // 重置优先级
        }
        live_range.priority += priority_circle; // 优先级增加
        last_circle_end = interval.second;
    }
    // 更新起止点
    live_range.start = start;
    live_range.end = end;
}