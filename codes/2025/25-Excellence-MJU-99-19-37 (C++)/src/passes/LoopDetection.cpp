#include "LoopDetection.hpp"
#include "Dominators.hpp"
#include <memory>
#include <set>
#include <algorithm>
#include <queue>

void LoopDetection::run() {
    dominators_ = std::make_unique<Dominators>(m_);
    all_loops_.clear();  // 添加一个成员变量来存储所有函数的循环
    
    for (auto &f1 : m_->get_functions()) {
        auto f = &f1;
        if (f->is_declaration())
            continue;
        func_ = f;
        run_on_func(f);
        
        // 保存每个函数的循环
        for (auto &loop : loops_) {
            all_loops_.push_back(loop);
        }
    }
    
    // 交换到总的循环列表
    loops_ = all_loops_;
    print();
}

void LoopDetection::discover_loop_and_sub_loops(BasicBlock *header, const BBset &latches, std::shared_ptr<Loop> loop) {
    std::queue<BasicBlock*> work_list;
    std::set<BasicBlock*> visited;
    
    // 首先将header加入循环（如果还没有的话）
    if (bb_to_loop_.find(header) == bb_to_loop_.end()) {
        bb_to_loop_[header] = loop;
    }
    
    // 将所有latch加入工作队列和循环
    for (auto latch : latches) {
        if (latch != header) {
            work_list.push(latch);
            visited.insert(latch);
            loop->add_latch(latch);
            
            // 如果latch还不属于任何循环，加入当前循环
            if (bb_to_loop_.find(latch) == bb_to_loop_.end()) {
                loop->add_block(latch);
                bb_to_loop_[latch] = loop;
            }
        }
    }
    
    // 从latch开始，逆向遍历找到所有循环内的块
    while (!work_list.empty()) {
        auto bb = work_list.front();
        work_list.pop();
        
        if (bb == header) continue;
        
        // 处理前驱块
        for (auto pred : bb->get_pre_basic_blocks()) {
            if (visited.find(pred) == visited.end()) {
                visited.insert(pred);
                
                // 检查pred是否被header支配（确保在循环内）
                if (dominators_->is_dominate(header, pred)) {
                    auto it = bb_to_loop_.find(pred);
                    if (it != bb_to_loop_.end()) {
                        // pred已经属于另一个循环
                        auto other_loop = it->second;
                        if (other_loop != loop) {
                            // 检查是否是子循环关系
                            bool is_nested = false;
                            
                            if (dominators_->is_dominate(header, other_loop->get_header())) {
                                if (!other_loop->get_parent()) {
                                    loop->add_sub_loop(other_loop);
                                    other_loop->set_parent(loop);
                                }
                                is_nested = true;
                            }
                            
                            // 如果不是嵌套关系，可能是并列循环，不处理
                            if (!is_nested) {
                                continue;
                            }
                        }
                    } else {
                        // pred还不属于任何循环，加入当前循环
                        loop->add_block(pred);
                        bb_to_loop_[pred] = loop;
                    }
                    
                    // 继续处理pred的前驱
                    work_list.push(pred);
                }
            }
        }
    }
}

void LoopDetection::run_on_func(Function *f) {
    dominators_->run_on_func(f);
    bb_to_loop_.clear();
    loops_.clear();
    
    // 按照支配树的后序遍历顺序处理，确保内层循环先被发现
    auto post_order = dominators_->get_dom_post_order();
    
    for (auto bb : post_order) {
        BBset latches;
        
        // 查找回边：pred被bb支配
        for (auto pred : bb->get_pre_basic_blocks()) {
            if (dominators_->is_dominate(bb, pred)) {
                latches.insert(pred);
            }
        }
        
        if (latches.empty()) continue;

        // 创建新循环
        auto loop = std::make_shared<Loop>(bb);
        loops_.push_back(loop);
        
        // 发现循环和子循环
        discover_loop_and_sub_loops(bb, latches, loop);
        
        // 查找并设置preheader
        find_and_set_preheader(loop);
    }
    
    // 移除重复的块
    remove_duplicate_blocks();
    
    // 计算循环深度
    calculate_loop_depth();
}

void LoopDetection::find_and_set_preheader(std::shared_ptr<Loop> loop) {
    auto header = loop->get_header();
    BasicBlock *preheader = nullptr;
    std::vector<BasicBlock*> outside_preds;
    
    // 查找所有不是从循环内部来的前驱
    for (auto pred : header->get_pre_basic_blocks()) {
        bool is_in_loop = false;
        
        // 检查pred是否在当前循环中
        for (auto block : loop->get_blocks()) {
            if (block == pred) {
                is_in_loop = true;
                break;
            }
        }
        
        // 也要检查是否是latch
        if (loop->get_latches().count(pred)) {
            is_in_loop = true;
        }
        
        if (!is_in_loop) {
            outside_preds.push_back(pred);
        }
    }
    
    
    if (outside_preds.size() == 1) {
        preheader = outside_preds[0];
        loop->set_preheader(preheader);
    }
   
}

void LoopDetection::remove_duplicate_blocks() {
    for (auto &loop : loops_) {
        BBvec unique_blocks;
        std::set<BasicBlock *> seen;
        
        // 确保header在第一个位置
        if (loop->get_header() && seen.find(loop->get_header()) == seen.end()) {
            unique_blocks.push_back(loop->get_header());
            seen.insert(loop->get_header());
        }
        
        // 添加其他块
        for (auto bb : loop->get_blocks()) {
            if (seen.find(bb) == seen.end()) {
                unique_blocks.push_back(bb);
                seen.insert(bb);
            }
        }
        
        loop->blocks_ = unique_blocks;
    }
}

void LoopDetection::calculate_loop_depth() {
    // 首先将所有循环的深度初始化为1
    for (auto &loop : loops_) {
        loop->set_depth(1);
    }
    
    // 根据父子关系计算深度
    for (auto &loop : loops_) {
        if (!loop->get_parent()) {
            calculate_depth_recursive(loop, 1);
        }
    }
}

void LoopDetection::calculate_depth_recursive(std::shared_ptr<Loop> loop, int depth) {
    loop->set_depth(depth);
    for (auto &sub_loop : loop->get_sub_loops()) {
        calculate_depth_recursive(sub_loop, depth + 1);
    }
}

bool LoopDetection::is_loop_invariant(Value *val, std::shared_ptr<Loop> loop) {
    // 常量总是循环不变的
    if (dynamic_cast<Constant *>(val)) {
        return true;
    }
    
    // 如果是指令，检查定义它的基本块是否在循环外
    if (auto inst = dynamic_cast<Instruction *>(val)) {
        auto bb = inst->get_parent();
        return !loop->contains(bb);
    }
    
    // 函数参数是循环不变的
    if (dynamic_cast<Argument *>(val)) {
        return true;
    }
    
    // 全局变量是循环不变的
    if (dynamic_cast<GlobalVariable *>(val)) {
        return true;
    }
    
    return false;
}

void LoopDetection::print() {
    m_->set_print_name();
    // std::cerr << "Loop Detection Result:" << std::endl;
    
    // 按函数分组打印
    std::map<Function*, std::vector<std::shared_ptr<Loop>>> func_loops;
    
    for (auto &loop : loops_) {
        auto func = loop->get_header()->get_parent();
        func_loops[func].push_back(loop);
    }
    
    for (auto &[func, loops] : func_loops) {
       // std::cerr << "Function: " << func->get_name() << std::endl;
        for (auto &loop : loops) {
            if (!loop->get_parent()) {
                print_loop(loop, 1);
            }
        }
    }
}
//调试输出
void LoopDetection::print_loop(std::shared_ptr<Loop> loop, int indent) {
    std::string prefix(indent * 2, ' ');
    
   // std::cerr << prefix << "Loop header: " << loop->get_header()->get_name() << std::endl;
    
    // if (loop->get_preheader()) {
    //     std::cerr << prefix << "Preheader: " << loop->get_preheader()->get_name() << std::endl;
    // }
    
   // std::cerr << prefix << "Loop depth: " << loop->get_depth() << std::endl;
    
    // std::cerr << prefix << "Loop blocks: ";
    // for (auto &bb : loop->get_blocks()) {
    //     std::cerr << bb->get_name() << " ";
    // }
    // std::cerr << std::endl;
    
   // std::cerr << prefix << "Loop latches: ";
    // for (auto &latch : loop->get_latches()) {
    //     std::cerr << latch->get_name() << " ";
    // }
    // std::cerr << std::endl;
    
    if (!loop->get_sub_loops().empty()) {
        //std::cerr << prefix << "Sub loops:" << std::endl;
        for (auto &sub_loop : loop->get_sub_loops()) {
            print_loop(sub_loop, indent + 1);
        }
    }
    
}