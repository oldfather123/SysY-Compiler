#include "LT.hpp"

void LengauerTarjan::dfs(BasicBlock* v, int& N) {
    dfs_num[v] = N++; // 标注访问顺序
    vertex.push_back(v); // 按上述顺序存储基本块
    label[v] = v; // 初始化为自身
    semi[v] = v;
    // 遍历其后继基本块
    for(auto w: v->succ_bbs) {
        if(dfs_num.find(w) == dfs_num.end()) { // 如果暂时未访问过
            parent[w] = v; // 记录父结点
            dfs(w, N);
        }
    }
}

void LengauerTarjan::compress(BasicBlock* v) {
    if(ancestor.find(ancestor[v]) != ancestor.end()) { // 如果祖先存在
        compress(ancestor[v]); // 递归压缩
        if(dfs_num[label[ancestor[v]]] < dfs_num[label[v]]) { // 如果祖先的label更早被访问
            label[v] = label[ancestor[v]]; // 则更新label为更早访问的结点的label
        }
        ancestor[v] = ancestor[ancestor[v]]; // 更新祖先为祖先的祖先
    }
}

BasicBlock* LengauerTarjan::eval(BasicBlock* v) {
    if(ancestor.find(v) == ancestor.end()) { // 如果没有祖先
        return label[v]; // 则直接返回其label
    }
    compress(v); // 反之进行路径压缩
    // 返回更早访问的label
    return dfs_num[label[ancestor[v]]] < dfs_num[label[v]] ? label[ancestor[v]]: label[v];
}

void LengauerTarjan::link(BasicBlock* v, BasicBlock* w) {
    ancestor[w] = v; // 将w的祖先设置为v
}

void LengauerTarjan::build_dominator_tree(Function* f) {
    dfs_num.clear(); vertex.clear();
    parent.clear(); semi.clear(); idom.clear();
    bucket.clear(); ancestor.clear(); label.clear();
    BasicBlock* start = f->bb_list.front();
    int N = 0;
    dfs(start, N); // 初始化深度优先遍历的相关信息
    for(int i = vertex.size() - 1; i >= 1; --i) {
        BasicBlock* w = vertex[i];
        for(auto v: w->pre_bbs) {
            // 如果从开始基本块开始无法访问到该基本块，
            // 说明该基本块为不可达基本块，无需考虑
            if(dfs_num.find(v) == dfs_num.end()) { // ignore unreachable
                continue;
            }
            BasicBlock* u = eval(v);
            if(dfs_num[semi[w]] > dfs_num[semi[u]]) {
                semi[w] = semi[u];
            }
        }
        bucket[semi[w]].push_back(w);
        link(parent[w], w);
        for(auto v: bucket[parent[w]]) {
            BasicBlock* u = eval(v);
            idom[v] = (semi[u] == semi[v]) ? semi[v]: idom[u];
        }
        bucket[parent[w]].clear();
    }
    for(int i = 1; i < vertex.size(); ++i) {
        BasicBlock* w = vertex[i];
        if(idom[w] != semi[w]) {
            idom[w] = idom[idom[w]];
        }
    }
    // 设置最终 idom 信息
    for(auto& [b, d]: idom) {
        b->idom = d;
    }
    f->bb_list.front()->idom = nullptr;
}

void LengauerTarjan::compute_dom_frontier(Function* f) {
    for(auto bb: f->bb_list) {
        if(bb->pre_bbs.size() < 2) {
            continue;
        }
        for(auto pre_bb: bb->pre_bbs) {
            BasicBlock* runner = pre_bb;
            while(runner && runner != bb->idom) {
                runner->dom_frontier.insert(bb);
                runner = runner->idom;
            }
        }
    }
}

void LengauerTarjan::build_reverse_dom_tree(Function* func) {
    func->bb_list.front()->idom = nullptr;
    for(auto bb: func->bb_list) {
        if(bb->idom) {
            bb->idom->rdoms.insert(bb);
        }
    }
}

void LengauerTarjan::build_reverse_dom_frontier(Function* func){
    for(auto bb: func->bb_list) {
        for(auto df: bb->dom_frontier) {
            df->rdom_frontier.insert(bb);
        }
    }
}

void LengauerTarjan::execute() {
    for(auto func: m->func_list) {
        if(func->bb_list.empty()) { // 如果函数没有基本块，则跳过
            continue;
        }
        build_dominator_tree(func);
        compute_dom_frontier(func);
        build_reverse_dom_tree(func);
        build_reverse_dom_frontier(func);
    }
}