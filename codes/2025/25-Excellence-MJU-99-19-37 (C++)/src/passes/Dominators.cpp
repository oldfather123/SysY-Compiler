#include "Dominators.hpp"
#include "Function.hpp"
#include <fstream>
#include <vector>
#include <list>
#include <iostream>

void Dominators::run() {
    for(auto &f1 : m_->get_functions()) {
        auto f = &f1;
        if(f->is_declaration())
            continue;
        run_on_func(f);
    }
}

void Dominators::run_on_func(Function *f) {
    // 清空之前的数据
    post_order_vec_.clear();
    post_order_.clear();
    idom_.clear();
    dom_frontier_.clear();
    dom_tree_succ_blocks_.clear();
    dom_tree_L_.clear();
    dom_tree_R_.clear();
    dom_post_order_.clear();
    dom_dfs_order_.clear();
    
    post_order.clear();
    post_order_num.clear();
    post_order_map.clear();
    reverse_post_order_map.clear();
    
    // 初始化数据结构
    for(auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        idom_.insert({bb, nullptr});
        dom_frontier_.insert({bb, {}});
        dom_tree_succ_blocks_.insert({bb, {}});
    }
    
    // // 调试输出
    // std::cerr << "\n[Dominators] Function " << f->get_name() << ":\n";
    // std::cerr << "  Total basic blocks: " << f->get_basic_blocks().size() << "\n";
    // std::cerr << "  Entry block: " << f->get_entry_block()->get_name() << "\n";
    
    // 计算支配关系
    create_idom(f);
    create_dominance_frontier(f);
    create_dom_tree_succ(f);
    create_dom_dfs_order(f);
    
    
    // std::cerr << "  Dominator tree structure:\n";
    // for (auto &[bb, succs] : dom_tree_succ_blocks_) {
    //     if (!succs.empty()) {
    //         std::cerr << "    " << bb->get_name() << " dominates: ";
    //         for (auto *succ : succs) {
    //             std::cerr << succ->get_name() << " ";
    //         }
    //         //std::cerr << "\n";
    //     }
    // }
}

// 后序遍历 - 使用参考代码的实现
void Dominators::post_order_traversal(BasicBlock *bb, BBSet &visit) {
    visit.insert(bb);
    for (auto b : bb->get_succ_basic_blocks()) {
        if (visit.find(b) == visit.end())
            post_order_traversal(b, visit);
    }
    post_order_num[bb] = post_order.size();
    post_order.push_back(bb);
    post_order_map[bb->get_parent()].push_back(bb);
}

void Dominators::create_idom(Function *f) {
    f->set_instr_name();
    
    // visit数组
    BBSet visit;
    auto entry = f->get_entry_block();
    
    post_order.clear();
    post_order_num.clear();
    post_order_traversal(entry, visit);
    
    post_order.reverse();
    for(auto it : post_order) {
        reverse_post_order_map[it->get_parent()].push_back(it);
        idom_[it] = nullptr;
    }
    
    idom_[entry] = entry;
    
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto bb : post_order) {
            if (bb == entry)
                continue;
            BasicBlock *new_idom = nullptr;
            for (auto &it : bb->get_pre_basic_blocks()) {
                if (idom_.at(it) != nullptr) {
                    new_idom = it;
                    break;
                }
            }
            
            for (auto &p : bb->get_pre_basic_blocks()) {
                if (p == new_idom)
                    continue;
                if (idom_[p] != nullptr) {
                    auto finger1 = p;
                    auto finger2 = new_idom;
                    while (finger1 != finger2) {
                        while (post_order_num[finger1] < post_order_num[finger2])
                            finger1 = idom_[finger1];
                        while (post_order_num[finger2] < post_order_num[finger1])
                            finger2 = idom_[finger2];
                    }
                    new_idom = finger1;
                }
            }
            
            // 如果新计算的 idom 与之前的不同，更新并标记为已改变
            if (idom_[bb] != new_idom) {
                idom_[bb] = new_idom;
                changed = true;
            }
        }
    }
    
    // 调试输出
    //std::cerr << "  Final idom:\n";
    for (auto &bb1 : f->get_basic_blocks()) {
        auto *bb = &bb1;
        if (idom_.find(bb) != idom_.end() && idom_[bb]) {
           // std::cerr << "    " << bb->get_name() << " -> " 
                    //  
        } else {
           // std::cerr << "    " << bb->get_name() << " -> null\n";
        }
    }
}

void Dominators::create_dominance_frontier(Function *f) {
    for (auto &b : f->get_basic_blocks()) {
        if (b.get_pre_basic_blocks().size() >= 2) {
            for (auto &p : b.get_pre_basic_blocks()) {
                auto runner = p;
                while (runner != idom_.at(&b)) {
                    dom_frontier_[runner].insert(&b);
                    runner = idom_[runner];
                    if(runner == nullptr)
                        break;
                }
            }
        }
    }
}

void Dominators::create_dom_tree_succ(Function *f) {
    for (auto &b : f->get_basic_blocks()) {
        auto entry = f->get_entry_block();
        if (idom_[&b] == nullptr)
            dom_tree_succ_blocks_[entry].insert(&b);
        else if (idom_[&b] == &b)
            continue;
        else
            dom_tree_succ_blocks_[idom_[&b]].insert(&b);
    }
}

void Dominators::create_reverse_post_order(Function *f) {
    // 这个函数不再需要
}

void Dominators::dfs(BasicBlock *bb, std::set<BasicBlock *> &visited) {
    // 这个函数不再需要
}

BasicBlock *Dominators::intersect(BasicBlock *b1, BasicBlock *b2) {
    // intersect逻辑已经内嵌在create_idom中
    return nullptr;
}

void Dominators::create_dom_dfs_order(Function *f) {
    unsigned int order = 0;
    std::function<void(BasicBlock *)> dfs = [&](BasicBlock *bb) {
        dom_tree_L_[bb] = ++ order;
        dom_dfs_order_.push_back(bb);
        for (auto &succ : dom_tree_succ_blocks_[bb]) {
            dfs(succ);
        }
        dom_tree_R_[bb] = order;
    };
    dfs(f->get_entry_block());
    dom_post_order_ =
        std::vector(dom_dfs_order_.rbegin(), dom_dfs_order_.rend());
}

void Dominators::print_idom(Function *f) {
    f->get_parent()->set_print_name();
    int counter = 0;
    std::map<BasicBlock *, std::string> bb_id;
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_name().empty())
            bb_id[bb] = "bb" + std::to_string(counter);
        else
            bb_id[bb] = bb->get_name();
        counter++;
    }
    printf("Immediate dominance of function %s:\n", f->get_name().c_str());
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        std::string output;
        output = bb_id[bb] + ": ";
        if (get_idom(bb)) {
            output += bb_id[get_idom(bb)];
        } else {
            output += "null";
        }
        printf("%s\n", output.c_str());
    }
}

void Dominators::print_dominance_frontier(Function *f) {
    f->get_parent()->set_print_name();
    int counter = 0;
    std::map<BasicBlock *, std::string> bb_id;
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_name().empty())
            bb_id[bb] = "bb" + std::to_string(counter);
        else
            bb_id[bb] = bb->get_name();
        counter++;
    }
    printf("Dominance Frontier of function %s:\n", f->get_name().c_str());
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        std::string output;
        output = bb_id[bb] + ": ";
        if (get_dominance_frontier(bb).empty()) {
            output += "null";
        } else {
            bool first = true;
            for (auto df : get_dominance_frontier(bb)) {
                if (first) {
                    first = false;
                } else {
                    output += ", ";
                }
                output += bb_id[df];
            }
        }
        printf("%s\n", output.c_str());
    }
}

void Dominators::dump_cfg(Function *f) {
    f->get_parent()->set_print_name();
    if(f->is_declaration())
        return;
    std::vector<std::string> edge_set;
    bool has_edges = false;
    for (auto &bb : f->get_basic_blocks()) {
        auto succ_blocks = bb.get_succ_basic_blocks();
        if(!succ_blocks.empty())
            has_edges = true;
        for (auto succ : succ_blocks) {
            edge_set.push_back('\t' + bb.get_name() + "->" + succ->get_name() + ";\n");
        }
    }
    std::string digraph = "digraph G {\n";
    if (!has_edges && !f->get_basic_blocks().empty()) {
        auto &bb = f->get_basic_blocks().front();
        digraph += '\t' + bb.get_name() + ";\n";
    } else {
        for (auto &edge : edge_set) {
            digraph += edge;
        }
    }
    digraph += "}\n";
    std::ofstream file_output;
    file_output.open(f->get_name() + "_cfg.dot", std::ios::out);
    file_output << digraph;
    file_output.close();
    std::string dot_cmd = "dot -Tpng " + f->get_name() + "_cfg.dot" + " -o " + f->get_name() + "_cfg.png";
    std::system(dot_cmd.c_str());
}

void Dominators::dump_dominator_tree(Function *f) {
    f->get_parent()->set_print_name();
    if(f->is_declaration())
        return;

    std::vector<std::string> edge_set;
    bool has_edges = false;

    for (auto &b : f->get_basic_blocks()) {
        if (idom_.find(&b) != idom_.end() && idom_[&b] != &b) {
            edge_set.push_back('\t' + idom_[&b]->get_name() + "->" + b.get_name() + ";\n");
            has_edges = true;
        }
    }

    std::string digraph = "digraph G {\n";

    if (!has_edges && !f->get_basic_blocks().empty()) {
        auto &b = f->get_basic_blocks().front();
        digraph += '\t' + b.get_name() + ";\n";
    } else {
        for (auto &edge : edge_set) {
            digraph += edge;
        }
    }

    digraph += "}\n";

    std::ofstream file_output;
    file_output.open(f->get_name() + "_dom_tree.dot", std::ios::out);
    file_output << digraph;
    file_output.close();

    std::string dot_cmd = "dot -Tpng " + f->get_name() + "_dom_tree.dot" + " -o " + f->get_name() + "_dom_tree.png";
    std::system(dot_cmd.c_str());
}