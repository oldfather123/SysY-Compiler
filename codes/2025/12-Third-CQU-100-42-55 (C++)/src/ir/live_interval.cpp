#include "../../include/ir/live_interval.h"
#include "../../debug.h"
#include "live_analysis.h"
#include <algorithm>
#include <set>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace llvm_ir {

void LiveInterval::add_range(int s, int e) {
    if (s >= e) return;
    
    // 强制合并策略：确保一个变量只有一个连续的活跃区间
    if (ranges.empty()) {
        ranges.emplace_back(s, e);
        return;
    }
    
    // 找到全局的最小起点和最大终点
    int global_start = s;
    int global_end = e;
    
    for (const auto& r : ranges) {
        global_start = std::min(global_start, r.start);
        global_end = std::max(global_end, r.end);
    }
    
    // 清空现有区间，只保留一个合并后的区间
    ranges.clear();
    ranges.emplace_back(global_start, global_end);
}

LiveIntervalMap compute_live_intervals(Function* func) {
    LiveIntervalMap intervals;
    
    // 1. 对所有指令做DFN编号（深度优先遍历保证顺序）
    std::unordered_map<Instruction*, int> dfn;
    std::unordered_map<BasicBlock*, int> bb_start_pos; // 基本块起始位置
    std::unordered_map<BasicBlock*, int> bb_end_pos;   // 基本块结束位置
    int idx = 0;
    
    // 预分配空间
    dfn.reserve(func->basicBlocks.size() * 10); // 预估每个基本块平均10条指令
    bb_start_pos.reserve(func->basicBlocks.size());
    bb_end_pos.reserve(func->basicBlocks.size());
    
#ifdef LIVEDEBUG
    std::cout << "[DEBUG] Starting DFN numbering..." << std::endl;
#endif
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue;
        
        bb_start_pos[bb] = idx;
#ifdef LIVEDEBUG
        std::cout << "[DEBUG] Block " << bb->label << " starts at position " << idx << std::endl;
#endif
        
        // 为普通指令分配编号
        for (auto& inst_ptr : bb->instructions) {
            if (inst_ptr) {
                dfn[inst_ptr.get()] = idx++;
#ifdef LIVEDEBUG
                std::cout << "[DEBUG] Inst " << inst_ptr->name << " at position " << dfn[inst_ptr.get()] << std::endl;
#endif
            }
        }
        
        bb_end_pos[bb] = idx - 1;
#ifdef LIVEDEBUG
        std::cout << "[DEBUG] Block " << bb->label << " ends at position " << bb_end_pos[bb] << std::endl;
#endif
    }
    
    // 2. 执行活跃变量分析
#ifdef LIVEDEBUG
    std::cout << "[DEBUG] Starting live variable analysis..." << std::endl;
#endif
    LiveInfo info = live_variable_analysis(func);
    
#ifdef LIVEDEBUG
    // 3. 输出活跃变量分析结果用于调试
    std::cout << "[DEBUG] Live variable analysis results:" << std::endl;
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue;
        
        std::cout << "[DEBUG] Block " << bb->label << ":" << std::endl;
        std::cout << "[DEBUG]   IN: ";
        auto in_it = info.in.find(bb);
        if (in_it != info.in.end()) {
            for (auto* v : in_it->second) {
                if (v) std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[DEBUG]   OUT: ";
        auto out_it = info.out.find(bb);
        if (out_it != info.out.end()) {
            for (auto* v : out_it->second) {
                if (v) std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
    }
#endif
    
    // 4. 基于活跃变量分析结果计算活跃区间
    std::unordered_map<Value*, std::vector<std::pair<int, int>>> var_ranges;
    
    // 预分配空间
    var_ranges.reserve(info.live_in.size() / 2); // 预估变量数量
    
#ifdef LIVEDEBUG
    // 5. 正确地基于活跃变量分析结果计算活跃区间
    std::cout << "[DEBUG] Computing live intervals from live analysis results..." << std::endl;
#endif
    
    // 使用活跃变量分析的指令级结果
    for (const auto& [inst, live_in_set] : info.live_in) {
        int inst_pos = dfn[inst];
        
        // 处理live_in：在指令开始时活跃的变量
        for (auto* var : live_in_set) {
            if (!var || dynamic_cast<Constant*>(var) || dynamic_cast<UndefValue*>(var)) continue;
            var_ranges[var].emplace_back(inst_pos, inst_pos + 1);
            
#ifdef LIVEDEBUG
            if (var->name.find("tmp_t_9_phi_0_1") != std::string::npos) {
                std::cout << "[DEBUG] Variable " << var->name << " live-in at instruction " 
                          << inst->toString() << " pos " << inst_pos << std::endl;
            }
#endif
        }
    }
    
    for (const auto& [inst, live_out_set] : info.live_out) {
        int inst_pos = dfn[inst];
        
        // 处理live_out：在指令结束后仍活跃的变量
        for (auto* var : live_out_set) {
            if (!var || dynamic_cast<Constant*>(var) || dynamic_cast<UndefValue*>(var)) continue;
            var_ranges[var].emplace_back(inst_pos + 1, inst_pos + 2);
            
#ifdef LIVEDEBUG
            if (var->name.find("tmp_t_9_phi_0_1") != std::string::npos) {
                std::cout << "[DEBUG] Variable " << var->name << " live-out at instruction " 
                          << inst->toString() << " pos " << inst_pos + 1 << std::endl;
            }
#endif
        }
    }
    
    
    // 5. 改进的循环处理：基于控制流图检测循环并扩展活跃区间
#ifdef LIVEDEBUG
    std::cout << "[DEBUG] Starting loop detection..." << std::endl;
#endif
    std::unordered_set<BasicBlock*> loop_headers;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> loop_blocks;
    
    // 预分配空间
    loop_headers.reserve(func->basicBlocks.size() / 4); // 预估循环头数量
    loop_blocks.reserve(func->basicBlocks.size() / 4);
    
    // 检测所有循环头
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue;
        
        // 检查是否有回边指向此基本块
        for (size_t i = 0; i < bb->predecessors.size(); ++i) {
            BasicBlock* pred = bb->predecessors[i];
            if (!pred) continue;
            
            // 如果前驱块的起始位置大于当前块，说明是回边
            if (bb_start_pos[pred] > bb_start_pos[bb]) {
                loop_headers.insert(bb);
#ifdef LIVEDEBUG
                std::cout << "[DEBUG] Loop header detected: " << bb->label 
                          << " (back edge from " << pred->label << ")" << std::endl;
#endif
                break;
            }
        }
    }
    
    // 对于每个循环头，计算循环体包含的所有基本块
    for (BasicBlock* header : loop_headers) {
#ifdef LIVEDEBUG
        std::cout << "[DEBUG] Processing loop with header: " << header->label << std::endl;
#endif
        
        std::unordered_set<BasicBlock*> visited;
        std::vector<BasicBlock*> worklist;
        std::vector<BasicBlock*> loop_bbs;
        
        // 预分配空间
        visited.reserve(func->basicBlocks.size() / 2);
        worklist.reserve(func->basicBlocks.size() / 4);
        loop_bbs.reserve(func->basicBlocks.size() / 4);
        
        // 找到所有回边的来源作为循环体的结束块
        for (size_t i = 0; i < header->predecessors.size(); ++i) {
            BasicBlock* pred = header->predecessors[i];
            if (pred && bb_start_pos[pred] > bb_start_pos[header]) {
                worklist.push_back(pred);
#ifdef LIVEDEBUG
                std::cout << "[DEBUG]   Back edge from: " << pred->label << std::endl;
#endif
            }
        }
        
        // 通过回溯找到整个循环体
        loop_bbs.push_back(header);
        visited.insert(header);
        
        while (!worklist.empty()) {
            BasicBlock* current = worklist.back();
            worklist.pop_back();
            
            if (visited.count(current)) continue;
            visited.insert(current);
            loop_bbs.push_back(current);
#ifdef LIVEDEBUG
            std::cout << "[DEBUG]   Loop includes block: " << current->label << std::endl;
#endif
            
            // 添加当前块的前驱（除了已访问的）
            for (size_t i = 0; i < current->predecessors.size(); ++i) {
                BasicBlock* pred = current->predecessors[i];
                if (pred && !visited.count(pred) && bb_start_pos[pred] >= bb_start_pos[header]) {
                    worklist.push_back(pred);
                }
            }
        }
        
        loop_blocks[header] = std::move(loop_bbs);
        
        // 计算循环的整体活跃区间
        int loop_start = bb_start_pos[header];
        int loop_end = bb_end_pos[header];
        
        for (BasicBlock* bb : loop_blocks[header]) {
            loop_end = std::max(loop_end, bb_end_pos[bb]);
        }
        
#ifdef LIVEDEBUG
        std::cout << "[DEBUG] Loop spans from " << loop_start << " to " << loop_end << std::endl;
#endif
        
        // 对于在循环头同时出现在入口和出口活跃集合的变量，扩展其活跃区间
        auto in_it = info.in.find(header);
        auto out_it = info.out.find(header);
        
        if (in_it != info.in.end() && out_it != info.out.end()) {
            for (auto* var : in_it->second) {
                if (!var || dynamic_cast<Constant*>(var) || dynamic_cast<UndefValue*>(var)) continue;
                
                if (out_it->second.count(var)) {
                    // 这个变量在循环中，需要在整个循环期间保持活跃
                    var_ranges[var].emplace_back(loop_start, loop_end + 1);
                    
#ifdef LIVEDEBUG
                    std::cout << "[DEBUG] Loop variable detected: " << var->name 
                              << " in loop [" << loop_start << ", " << loop_end + 1 << ")" << std::endl;
#endif
                }
            }
        }
        
        // 特殊处理：对于循环中使用的变量，即使不在循环头的in/out中，也要扩展其活跃区间
        for (BasicBlock* loop_bb : loop_blocks[header]) {
            std::vector<Instruction*> all_insts;
            all_insts.reserve(loop_bb->instructions.size());
            
            for (auto& inst_ptr : loop_bb->instructions) {
                if (inst_ptr) all_insts.push_back(inst_ptr.get());
            }
            
            for (Instruction* inst : all_insts) {
                if (!inst) continue;
                
                // 检查指令的操作数，如果是其他指令的结果且在循环中被多次使用
                for (auto* operand : inst->operands) {
                    if (!operand || dynamic_cast<Constant*>(operand) || dynamic_cast<UndefValue*>(operand)) continue;
                    
                    // 如果操作数是在循环外定义但在循环内使用的，扩展其活跃区间
                    auto* operand_inst = dynamic_cast<Instruction*>(operand);
                    if (operand_inst) {
                        // 检查operand是否在循环外定义
                        bool defined_outside = true;
                        for (BasicBlock* check_bb : loop_blocks[header]) {
                            if (operand_inst->parent == check_bb) {
                                defined_outside = false;
                                break;
                            }
                        }
                        
                        if (defined_outside) {
                            // 变量在循环外定义，在循环内使用，扩展活跃区间覆盖整个循环
                            var_ranges[operand].emplace_back(loop_start, loop_end + 1);
#ifdef LIVEDEBUG
                            std::cout << "[DEBUG] External variable used in loop: " << operand->name 
                                      << " extended to [" << loop_start << ", " << loop_end + 1 << ")" << std::endl;
#endif
                        }
                    }
                }
            }
        }
    }
    
    
    // 6. 更精确地处理活跃区间，基于活跃变量分析的结果
#ifdef LIVEDEBUG
    std::cout << "[DEBUG] Final interval merging..." << std::endl;
#endif
    
    // 预分配intervals空间
    intervals.reserve(var_ranges.size());
    
    for (auto& [var, ranges] : var_ranges) {
        if (ranges.empty()) continue;
        
#ifdef LIVEDEBUG
        std::cout << "[DEBUG] Processing variable: " << var->name << std::endl;
        std::cout << "[DEBUG]   Raw ranges: ";
        for (auto& range : ranges) {
            std::cout << "[" << range.first << "," << range.second << ") ";
        }
        std::cout << std::endl;
#endif
        
        // 排序所有区间
        std::sort(ranges.begin(), ranges.end());
        
        // 合并重叠和相邻的区间
        std::vector<std::pair<int, int>> merged;
        merged.reserve(ranges.size()); // 预分配空间
        
        for (auto& range : ranges) {
            if (merged.empty() || merged.back().second < range.first) {
                merged.push_back(range);
            } else {
                merged.back().second = std::max(merged.back().second, range.second);
            }
        }
        
#ifdef LIVEDEBUG
        std::cout << "[DEBUG]   Merged ranges: ";
        for (auto& range : merged) {
            std::cout << "[" << range.first << "," << range.second << ") ";
        }
        std::cout << std::endl;
#endif
        
        // 对于循环变量，我们需要确保活跃区间覆盖整个循环
        // 这里使用启发式方法：如果变量有多个分离的区间，且跨度较大，可能是循环变量
        if (!merged.empty()) {
            int global_start = merged.front().first;
            int global_end = merged.back().second;
            
            if (merged.size() > 1) {
                int total_span = global_end - global_start;
                int active_span = 0;
                for (auto& range : merged) {
                    active_span += (range.second - range.first);
                }
                
#ifdef LIVEDEBUG
                std::cout << "[DEBUG]   Total span: " << total_span << ", Active span: " << active_span << std::endl;
                
                // 如果活跃跨度与总跨度比例较高，说明可能是循环变量，合并所有区间
                if (active_span * 2 > total_span) {  // 超过50%的跨度都是活跃的
                    std::cout << "[DEBUG]   High activity ratio, merging all ranges" << std::endl;
                }
#endif
            }
            
            intervals[var].add_range(global_start, global_end);
#ifdef LIVEDEBUG
            std::cout << "[DEBUG]   Final interval: [" << global_start << "," << global_end << ")" << std::endl;
#endif
        }
    }
    
#ifdef LIVEDEBUG
    std::cout << "[DEBUG] Live interval computation completed" << std::endl;
#endif
    return intervals;
}

}
