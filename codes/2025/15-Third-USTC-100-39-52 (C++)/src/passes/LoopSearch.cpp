#include "LoopSearch.hpp"
#include "logging.hpp"

#include <fstream>
#include <iostream>
#include <unordered_set>

struct CFGNode {
    std::unordered_set<CFGNodePtr> succs;
    std::unordered_set<CFGNodePtr> prevs;
    BasicBlock *bb;
    int index;   // the index of the node in CFG
    int lowlink; // the min index of the node in the strongly connected componets
    bool onStack;
};

void LoopSearch::build_cfg(Function *func, std::unordered_set<CFGNode *> &result) {
    // TODO: build control flow graph used in loop search pass
    std::unordered_map<BasicBlock *, CFGNode *> bb2node;

    for (auto &bb : func->get_basic_blocks()) {
        auto node = new CFGNode();
        node->bb = &bb;
        node->index = -1;
        node->lowlink = -1;
        node->onStack = false;

        result.insert(node);
        bb2node[&bb] = node;
    }

    for (auto node : result) {
        auto bb = node->bb;

        for (auto succ_bb : bb->get_succ_basic_blocks()) {
            auto succ_node = bb2node[succ_bb];
            node->succs.insert(succ_node);
        }

        for (auto prev_bb : bb->get_pre_basic_blocks()) {
            auto prev_node = bb2node[prev_bb];
            node->prevs.insert(prev_node);
        }
    }
    
}

// Tarjan algorithm
bool LoopSearch::strongly_connected_components(CFGNodePtrSet &nodes,
                                               std::unordered_set<CFGNodePtrSet *> &result) {
    index_count = 0;
    stack.clear();
    for (auto n : nodes) {
        if (n->index == -1) {
            traverse(n, result);
        }
    }
    return result.size() != 0;
}

void LoopSearch::traverse(CFGNodePtr n, std::unordered_set<CFGNodePtrSet *> &result) {
    n->index   = index_count++;
    n->lowlink = n->index;
    stack.push_back(n);
    n->onStack = true;

    for (auto su : n->succs) {
        // has not visited su
        if (su->index == -1) {
            traverse(su, result);
            n->lowlink = std::min(su->lowlink, n->lowlink);
        }
        // has visited su
        else if (su->onStack) {
            n->lowlink = std::min(su->index, n->lowlink);
        }
    }

    if (n->index == n->lowlink) {
        // TODO: pop out the nodes in the same strongly connected component from stack
        auto *scc = new CFGNodePtrSet();  
        CFGNodePtr w=nullptr;

        do {
            w = stack.back();
            stack.pop_back();
            w->onStack = false;
            scc->insert(w);               
        } while (w != n);

        if (scc->size() > 1) {
            result.insert(scc);          // 只加入非单点 SCC（含有环）
        } else {
            delete scc;                  
        }
    }
}

CFGNodePtr LoopSearch::find_loop_base(CFGNodePtrSet *set, CFGNodePtrSet &reserved) {
    // TODO: find the loop base node
    for (auto base : *set) {
        for (auto prev : base->prevs) {
            if (set->count(prev) == 0 && reserved.count(base) == 0) {
                // 该 node 有来自 SCC 外的前驱，且不在保留区
                return base;
            }
        }
    }
    return nullptr;
}
void LoopSearch::run() {
    auto &func_list = m_->get_functions();
    for (auto &func1 : func_list) {
        auto func = &func1;
        if (func->get_basic_blocks().size() == 0) {
            continue;
        } else {
            CFGNodePtrSet nodes;
            CFGNodePtrSet reserved;
            std::unordered_set<CFGNodePtrSet *> sccs;

            // step 1: build cfg
            // TODO
            // dump graph
            build_cfg(func, nodes);
            dump_graph(nodes, func->get_name());
            // step 2: find strongly connected graph from external to internal
            do {
                strongly_connected_components(nodes, sccs);
                LOG(INFO) << "detected scc number: " << sccs.size();
                reserved.clear();

                for (auto sig_sccs : sccs) {
                    LOG(INFO) << "sig_scs's nodes number: " << sig_sccs->size();
                    auto base = find_loop_base(sig_sccs, reserved);
                    if (!base) continue;
                    reserved.insert(base);

                    auto *loop = new BBset_t();
                    for (auto node : *sig_sccs) {
                        loop->insert(node->bb);
                        bb2base[node->bb] = base->bb;
                    }

                    loop2base[loop] = base->bb;
                    base2loop[base->bb] = loop;
                    loop_set.insert(loop);
                    LOG(INFO) << "loop number: " << loop_set.size();
                    func2loop[func].insert(loop);

                    std::string blocks_in_loop;
                    for (auto bb : *loop) {
                        auto name = bb->get_name();
                        if (name.empty()) name = "<unnamed>";
                        blocks_in_loop += name + " ";
                    }
                    LOG(INFO) << "Detected loop with blocks: " << blocks_in_loop;
                    nodes.erase(base);
                    LOG(INFO) << "nodes number: " << nodes.size();
                }
                sccs.clear();
                for (auto node : nodes) {
                    node->index = -1;
                    node->lowlink = -1;
                    node->onStack = false;
                }

            } while (!reserved.empty());

            for (auto node : nodes) {
                delete node;
            }
            nodes.clear();
        }

    }
}

void LoopSearch::dump_graph(CFGNodePtrSet &nodes, std::string title) {
    if (!dump) return;

    std::unordered_map<BasicBlock *, std::string> bb_names;
    static int unnamed_id = 0;

    // 为每个 BB 分配合法名称
    for (auto node : nodes) {
        auto bb = node->bb;
        std::string name = bb->get_name();
        if (name.empty()) {
            name = "unnamed_" + std::to_string(++unnamed_id);
        }
        bb_names[bb] = name;
    }

    std::vector<std::string> edge_set;
    for (auto node : nodes) {
        auto name = bb_names[node->bb];

        if (base2loop.find(node->bb) != base2loop.end()) {
            // 是循环入口（涂红）
            for (auto succ : node->succs) {
                if (nodes.find(succ) != nodes.end()) {
                    edge_set.push_back("\t" + name + " -> " + bb_names[succ->bb] + ";\n");
                }
            }
            edge_set.push_back("\t" + name + " [color=red];\n");
        } else {
            // 普通节点
            for (auto succ : node->succs) {
                if (nodes.find(succ) != nodes.end()) {
                    edge_set.push_back("\t" + name + " -> " + bb_names[succ->bb] + ";\n");
                }
            }
        }
    }

    std::string digraph = "digraph G {\n";
    for (auto &e : edge_set) {
        digraph += e;
    }
    digraph += "}\n";

    std::ofstream file_output(title + ".dot");
    file_output << digraph;
    file_output.close();

    std::string dot_cmd = "dot -Tpng " + title + ".dot -o " + title + ".png";
    std::system(dot_cmd.c_str());
}


BBset_t *LoopSearch::get_parent_loop(BBset_t *loop) {
    auto base = loop2base[loop];
    for (auto prev : base->get_pre_basic_blocks()) {
        if (loop->find(prev) != loop->end()) {
            continue;
        }
        auto loop = get_inner_loop(prev);
        if (loop == nullptr || loop->find(base) == loop->end()) {
            return nullptr;
        } else {
            return loop;
        }
    }
    return nullptr;
}

std::unordered_set<BBset_t *> LoopSearch::get_loops_in_func(Function *f) {
    return func2loop.count(f) ? func2loop[f] : std::unordered_set<BBset_t *>();
}
