#include <set>
#include <unordered_set>
#include <stack>
#include <queue>
#include <algorithm>
#include "LoopInit.hpp"

map<BasicBlock*, vector<Edge*>> LoopInitializer::get_back_edges(Function* func) {
    map<BasicBlock*, vector<Edge*>> back_edges_from_header;
    // 遍历所有基本块，寻找回边
    for (auto bb: func->bb_list) {
        for (auto suc: bb->succ_bbs) {
            if (bb->is_dominated_by(suc)) { // 如果当前基本块被其后继基本块支配，则为这条边为回边
                back_edges_from_header[suc].push_back(new Edge(bb, suc)); // 将回边存入对应的后继基本块
            }
        }
    }
    return back_edges_from_header;
}

vector<BasicBlock*> LoopInitializer::get_loop_body(Edge* back_edge) {
    vector<BasicBlock*> body; // 循环体
    set<BasicBlock*> visited; // 记录访问过的基本块
    stack<BasicBlock*> worklist; // 工作列表，用于遍历支配树
    // 回边：A -> B
    auto A = back_edge->first;
    auto B = back_edge->second;
    // 循环头尾加入已访问集合
    visited.insert(A);
    visited.insert(B);
    worklist.push(A); // 从A开始遍历支配树
    while(!worklist.empty()) {
        auto cur_bb = worklist.top();
        worklist.pop();
        body.push_back(cur_bb); // 将当前基本块加入循环体
        // 遍历当前基本块的前驱基本块
        for(auto pre_bb: cur_bb->pre_bbs) {
            if(visited.find(pre_bb) == visited.end()) { // 如果前驱基本块未被访问过
                visited.insert(pre_bb); // 标记为已访问
                worklist.push(pre_bb); // 将前驱基本块加入工作列表
            }
        }
    }
    body.push_back(B); // 将循环头加入循环体
    return body;
}

void LoopInitializer::detect_loops(Function* func) {
    map<BasicBlock*, vector<Edge*>> back_edges_from_header = get_back_edges(func);
    if(back_edges_from_header.empty()) {
        return; // 没有回边，说明没有循环
    }
    for(auto [header, back_edges]: back_edges_from_header) { // 每一个回边尾都对应一个循环
        set<BasicBlock*> body_set; // 循环体
        for(auto back_edge: back_edges) {
            vector<BasicBlock*> sub_body = get_loop_body(back_edge); // 获取循环体
            if(sub_body.empty()) {
                continue; // 如果循环体为空，则跳过
            }
            for(auto bb: sub_body) {
                body_set.insert(bb); // 将循环体基本块加入集合，避免重复
            }
        }
        vector<BasicBlock*> body(body_set.begin(),body_set.end()); // 循环体
        loops[func]->emplace_back(make_shared<Loop>(header, back_edges, body)); // 使用智能指针管理循环对象
    }
}

void LoopInitializer::build_loop_tree(Function* func) {
    for(auto& child: *loops[func]) {
        for(auto& parent: *loops[func]) {
            if(child == parent) { // 同一循环
                continue;
            }
            bool nested = true;
            for(auto bb: child->body) {
                if(std::find(parent->body.begin(), parent->body.end(), bb) == parent->body.end()) {
                    nested = false; // 如果子循环的某个基本块不在父循环中，则不是嵌套循环
                    break;
                }
            }
            if(nested) {
                if(child->parent && child->parent->body.size() < parent->body.size()) {
                    continue; // 如果子循环已经有父循环，并且当前父循环更大，则不更新，确保多层嵌套关系的正确
                } else {
                    child->parent = parent; // 设置父循环
                }
            }
        }
    }
    // 确定好每个循环的父循环后，统一设置子循环列表
    for(auto& child: *loops[func]) {
        if(child->parent) {
            child->parent->children->push_back(child); // 将子循环加入父循环的子循环列表
        }
    }
}

void LoopInitializer::order_loops(Function* func) {
    LoopVecPtr ordered_loops = make_shared<vector<LoopPtr>>(); // 用于存储已处理的循环
    set<LoopPtr> processed;
    while(processed.size() < loops[func]->size()) { // 处理所有循环直到全部加入队列
        for(auto& loop: *loops[func]) {
            if(processed.find(loop) != processed.end()) { // 如果循环已处理，跳过                
                continue;
            }
            // 检查此循环的所有子循环是否都已处理
            bool all_children_processed = true;
            for(auto& child: *(loop->children)) {
                if(processed.find(child) == processed.end()) { // 存在未处理的子循环
                    all_children_processed = false;
                    break;
                }
            }
            if(all_children_processed) { // 如果所有子循环已处理，添加此循环
                ordered_loops->push_back(loop);
                processed.insert(loop);
            }
        }
    }
    loops[func] = std::move(ordered_loops); // 更新原循环列表为处理后的顺序
}

void LoopInitializer::set_loop_pre_header(LoopPtr loop) {
    for(auto pre: loop->header->pre_bbs) { //* 1. 保证是循环头的前驱块
        if(std::find(loop->body.begin(), loop->body.end(), pre) == loop->body.end()) { //* 2. 保证该前驱块为非循环体块
            if(loop->pre_header) { //* 3. 保证非循环前驱块的唯一性
                loop->pre_header = nullptr; // 如果已经存在一个，说明唯一的非循环前驱块的唯一性不能被保证
                break;
            }
            if(pre->succ_bbs.size() == 1 && pre->succ_bbs[0] == loop->header) { // 4. 保证该前驱块的后继块的唯一性
                loop->pre_header = pre;
            }
        }
    }
    if(loop->pre_header) {
        return;
    }
    // 如果没有找到唯一的非循环前驱块，则需要创建一个新的 pre-header，同时调整CFG以及循环相关基本块指令
    loop->pre_header = new BasicBlock(loop->header->parent->parent, "label_" + std::to_string(loop->header->parent->ssa_id++) + "_preheader", loop->header->parent);
    // 将 pre-header 添加到父循环的循环体中，但不加入本身的循环体
    LoopPtr parent = loop->parent;
    while(parent)  { 
        parent->body.push_back(loop->pre_header); // 将 pre-header 添加到父循环的循环体中
        parent = parent->parent; // 向上遍历父循环
    }
    // 调整控制流图与相关指令(br)
    // TODO: 仍然是循环迭代器被删除的错误，不知道为何一直能够运行
    unordered_set<BasicBlock*> pre_header_set; // 用于存储非循环体前驱块
    for(auto pre: loop->header->pre_bbs) {
        if(std::find(loop->body.begin(), loop->body.end(), pre) == loop->body.end()) { // 非循环体块
            // 修改原有非循环体前驱块的控制流关系，将其从循环头移除，添加到 pre-header
            pre->remove_succ_basic_block(loop->header);
            // loop->header->remove_pre_basic_block(pre);
            pre_header_set.insert(pre); // 将非循环体前驱块加入集合
            pre->add_succ_basic_block(loop->pre_header);
            loop->pre_header->add_pre_basic_block(pre);
            // 修改循环头的非循环体前驱块的跳转指令，使之可以跳转到 pre-header
            if(auto br = dynamic_cast<BranchInst*>(pre->get_terminator())) { // 确保是分支指令
                if(br->operands.size() == 1) { // 无条件跳转
                    br->replace_op(0, loop->pre_header); // 直接替换跳转目标
                } else { // 条件跳转，替换原有循环头的位置为 pre-header
                    if(br->get_op(1) == loop->header) { // 如果 true_bb 是循环头
                        br->replace_op(1, loop->pre_header);
                    } else if(br->get_op(2) == loop->header) { // 如果 false_bb 是循环头
                        br->replace_op(2, loop->pre_header);
                    } else {
                        cerr << "[ERROR] LoopInitializer::set_loop_pre_header: pre-header not found for loop header " << loop->header->name << endl;
                        exit(1);
                    }
                }
            }
        }
    }
    for(auto pre: pre_header_set) { // 从集合中删除非循环体前驱块的前驱关系
        loop->header->remove_pre_basic_block(pre);
    }
    // 加入跳转指令，注意，BranchInst 构造时自动连接给定基本块
    loop->pre_header->add_inst_back(new BranchInst(loop->header, loop->pre_header));
    // 针对循环头中的 Phi 指令进行处理
    //! 注意：此时 pre-header 中已经存在一条跳转到循环头的无条件跳转指令（br label %loop->header）
    for(auto inst: loop->header->inst_list) {
        if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
            vector<Value*> phi_vals; // 存储循环头 Phi 指令的非循环前驱块的值
            vector<BasicBlock*> phi_bbs; // 存储循环头 Phi 指令的非循环前驱块
            // 清除非循环体前驱的值来源
            vector<int> operand_to_delete;
            for(int i = 0; i < phi_inst->operands.size(); i += 2) {
                Value* val = phi_inst->get_op(i); // 获取源值
                BasicBlock* pre_bb = dynamic_cast<BasicBlock*>(phi_inst->get_op(i + 1)); // 获取源基本块
                if(val && pre_bb) { // 确保正确获取操作数
                    if(std::find(loop->body.begin(), loop->body.end(), pre_bb) == loop->body.end()) { // 非循环体块
                        phi_vals.push_back(val); // 收集非循环前驱块的值
                        phi_bbs.push_back(pre_bb); // 收集非循环前驱块
                        phi_inst->remove_ops(i, i + 1); // 删除原有循环头中Phi指令的非循环前驱块的操作数对
                        i -= 2; // 调整i，避免跳过下一个操作
                    }
                }
            }
            for(auto index = operand_to_delete.rbegin(); index != operand_to_delete.rend(); ++index) {
                phi_inst->remove_ops(*index, *index + 1); // 从后向前删除操作数对
            }
            // 创建新的 Phi 指令用于选择非循环体前驱的值来源，并加入 pre-header
            if(phi_vals.size() == 1 && phi_bbs.size() == 1) { // 只有一个来源，那直接使用这个值即可
                phi_inst->add_ops(phi_vals[0], loop->pre_header);
                continue; // 跳过创建新 Phi 指令的步骤
            }
            if(!phi_vals.empty() && !phi_bbs.empty() && phi_vals.size() == phi_bbs.size()) {
                PhiInst* new_phi_inst = new PhiInst(phi_inst->type, phi_vals, phi_bbs, loop->pre_header, InsertPos::None);
                new_phi_inst->name = std::to_string(new_phi_inst->parent->parent->ssa_id++);
                loop->pre_header->add_inst_before_terminator(new_phi_inst);
                phi_inst->add_ops(new_phi_inst, loop->pre_header);
            }
        }
    }
}

void LoopInitializer::set_loop_latch(LoopPtr loop) {
    if(loop->latch) {
        return; // 如果已经有了循环出口，则不需要设置
    }
    if(loop->back_edges.size() == 1) {
        loop->latch = loop->back_edges[0]->first; // 直接使用回边的源作为循环出口
        return;
    }
    // 如果没有找到循环出口，则创建一个新的
    loop->latch = new BasicBlock(loop->header->parent->parent, "label_" + std::to_string(loop->header->parent->ssa_id++) + "_latch", loop->header->parent);
    // 将新创建的 latch 基本块加入循环及其父循环的 body 中
    loop->body.push_back(loop->latch);
    LoopPtr parent = loop->parent;
    while(parent)  { 
        parent->body.push_back(loop->latch); // 将 latch 添加到父循环的循环体中
        parent = parent->parent; // 向上遍历父循环
    }
    // latch 中添加跳转指令到循环头，作为新的唯一回边
    loop->latch->add_inst_back(new BranchInst(loop->header, loop->latch));
    // 原回边都需要跳转到 latch
    for(auto back_edge: loop->back_edges) {
        BasicBlock* src = back_edge->first; // 回边的源
        BasicBlock* dest = back_edge->second; // 回边的目标
        if(src == loop->header) {
            continue; // 如果回边已经是从循环头开始，则不需要修改
        }
        // 修改回边的目标为 latch
        src->remove_succ_basic_block(dest);
        dest->remove_pre_basic_block(src);
        src->add_succ_basic_block(loop->latch);
        loop->latch->add_pre_basic_block(src);
        // 修改回边的跳转指令，使之跳转到 latch
        if(auto br = dynamic_cast<BranchInst*>(src->get_terminator())) {
            if(br->operands.size() == 1) { // 无条件跳转
                br->replace_op(0, loop->latch); // 直接替换跳转目标
            } else { // 条件跳转，替换原有循环头的位置
                if(br->get_op(1) == dest) { // 如果 true_bb 是回边目标
                    br->replace_op(1, loop->latch);
                } else if(br->get_op(2) == dest) { // 如果 false_bb 是回边目标
                    br->replace_op(2, loop->latch);
                } else {
                    cerr << "[ERROR] LoopInitializer::set_loop_latch: latch not found for loop header " << loop->header->name << " in ";
                    br->print(cerr);
                    cerr << endl;
                    exit(1);
                }
            }
        }
    }
    // 调整循环头的 Phi 指令
    for(auto inst: loop->header->inst_list) {
        if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
            vector<Value*> phi_vals; // 存储循环头 Phi 指令的非循环前驱块的值
            vector<BasicBlock*> phi_bbs; // 存储循环头 Phi 指令的非循环前驱块
            // 清除循环体前驱的值来源
            vector<int> operand_to_delete;
            for(int i = 0; i < phi_inst->operands.size(); i += 2) {
                Value* val = phi_inst->get_op(i); // 获取源值
                BasicBlock* pre_bb = dynamic_cast<BasicBlock*>(phi_inst->get_op(i + 1)); // 获取源基本块
                if(val && pre_bb) { // 确保正确获取操作数
                    if(std::find(loop->body.begin(), loop->body.end(), pre_bb) != loop->body.end()) { // 循环体块
                        phi_vals.push_back(val); // 收集非循环前驱块的值
                        phi_bbs.push_back(pre_bb); // 收集非循环前驱块
                        operand_to_delete.push_back(i); // 记录需要删除的操作数对
                    }
                }
            }
            for(auto index = operand_to_delete.rbegin(); index != operand_to_delete.rend(); ++index) {
                phi_inst->remove_ops(*index, *index + 1); // 从后向前删除操作数对
            }
            // 创建新的 Phi 指令用于选择非循环体前驱的值来源，并加入 latch
            if(phi_vals.size() == 1 && phi_bbs.size() == 1) { // 只有一个来源，那直接使用这个值即可
                phi_inst->add_ops(phi_vals[0], loop->latch);
                continue; // 跳过创建新 Phi 指令的步骤
            }
            if(!phi_vals.empty() && !phi_bbs.empty() && phi_vals.size() == phi_bbs.size()) {
                PhiInst* new_phi_inst = new PhiInst(phi_inst->type, phi_vals, phi_bbs, loop->latch, InsertPos::None);
                new_phi_inst->name = std::to_string(new_phi_inst->parent->parent->ssa_id++); // 设置新的Phi指令名称
                loop->latch->add_inst_before_terminator(new_phi_inst);
                phi_inst->add_ops(new_phi_inst, loop->latch);
            }
        }
    }
}

void LoopInitializer::set_basic_indu_vars(LoopPtr loop) {
    for(auto inst: loop->header->inst_list) {
        auto phi_inst = dynamic_cast<PhiInst*>(inst); // 基本归纳变量一定是循环头的 Phi 指令
        if(!phi_inst || phi_inst->operands.size() != 4) {
            continue; // Loop Simplify 保证循环头前驱只有 pre-header 和 latch
        }
        // 循环头的前驱块
        auto bb1 = dynamic_cast<BasicBlock*>(phi_inst->get_op(1));
        auto bb2 = dynamic_cast<BasicBlock*>(phi_inst->get_op(3));
        // pre-header 的值是初始值，latch 的值是变化表达式
        Value* init_val = nullptr; // 初始值
        Instruction* update_exp = nullptr; // 变化表达式
        if(bb1 == loop->pre_header && bb2 == loop->latch) { // pre-header 在前，latch 在后
            init_val = phi_inst->get_op(0);
            update_exp = dynamic_cast<Instruction*>(phi_inst->get_op(2));
        } else if(bb1 == loop->latch && bb2 == loop->pre_header) { // latch 在前，pre-header 在后
            init_val = phi_inst->get_op(2);
            update_exp = dynamic_cast<Instruction*>(phi_inst->get_op(0));
        } else { // 前驱块存在问题，检查 Loop Simplify 是否正确
            cerr << "[ERROR] LoopInitializer::set_basic_indu_vars: unexpected PhiInst operands in loop header " << loop->header->name << endl;
            exit(1);
        }
        if(!update_exp || (!update_exp->is_add() && !update_exp->is_sub())) { // 变化表达式必须是加法或减法
            continue;
        }
        int step = 0; // 更新步长
        Value* l_op = update_exp->get_op(0); // 左操作数
        Value* r_op = update_exp->get_op(1); // 右操作数
        if(l_op == phi_inst && dynamic_cast<ConstInt*>(r_op)) { // 左操作数是 Phi 指令，右操作数是常量
            step = static_cast<ConstInt*>(r_op)->val; // 步长为右操作数的值
        } else if(r_op == phi_inst && dynamic_cast<ConstInt*>(l_op)) { // 右操作数是 Phi 指令，左操作数是常量
            step = static_cast<ConstInt*>(l_op)->val; // 步长为左操作数的值
        } else {
            continue; // 如果不是基本归纳变量的变化表达式，则跳过
        }
        if(update_exp->is_sub()) { // 如果是减法，则步长取反，即反向更新
            step = -step;
        }
        loop->biv_to_divs[phi_inst].push_back(new IVExp(phi_inst, phi_inst, step, 0, nullptr)); // 第一位的 k 代表更新步长，b 没有意义
    }
}

void LoopInitializer::set_derived_indu_vars(LoopPtr loop) {
    unordered_set<Value*> visited; // 记录已经访问过的归纳变量相关表达式，避免重复处理
    queue<IVExp*> worklist; // 工作列表，用于广度优先搜索归纳变量
    // 初始化工作列表：将所有基本归纳变量加入工作列表
    for(auto [biv, divs]: loop->biv_to_divs) {
        if(visited.find(biv) == visited.end()) { // 如果基本归纳变量未被访问过
            visited.insert(biv);
            // 标记基本归纳变量 Phi 指令本身为已访问
            visited.insert(divs[0]->inst);
            // 标记基本归纳变量的变化表达式为已访问
            auto pre_bb1 = dynamic_cast<BasicBlock*>(divs[0]->inst->get_op(1));
            auto pre_bb2 = dynamic_cast<BasicBlock*>(divs[0]->inst->get_op(3));
            if(pre_bb1 && pre_bb1 == loop->latch) {
                visited.insert(divs[0]->inst->get_op(0));
            } else if(pre_bb2 && pre_bb2 == loop->latch) {
                visited.insert(divs[0]->inst->get_op(2));
            }
            // 从基本归纳变量本身开始搜寻
            worklist.push(divs[0]);
        } else {
            cerr << "[ERROR] LoopInitializer::set_derived_indu_vars: duplicate basic induction variable detected in loop " << loop->header->name << endl;
            exit(1); // 如果基本归纳变量重复，说明 Loop Simplify 存在问题
        }
    }
    // 收集循环体内所有二元表达式，作为可能的归纳变量计算式
    vector<Instruction*> candidates;
    for(auto bb: loop->body)  {
        if(!loop->is_always_executed(bb)) {
            continue;
        }
        for(auto inst: bb->inst_list) {
            if(auto binary_inst = dynamic_cast<BinaryInst*>(inst)) {
                if(binary_inst->is_add() || binary_inst->is_sub() || binary_inst->is_mul() || binary_inst->is_shl()) {
                    candidates.push_back(binary_inst);
                }
            }
        }
    }
    while(!worklist.empty()) {
        IVExp* cur_exp = worklist.front();
        worklist.pop();
        for(auto binary_inst: candidates) {
            if(visited.count(binary_inst)) { // 如果已经访问过，跳过
                continue;
            }
            auto op0 = binary_inst->get_op(0);
            auto op1 = binary_inst->get_op(1);
            ConstInt* const_op = nullptr;
            Value* iv_op = nullptr;
            if(op0 == cur_exp->inst) { // 当前归纳变量为左操作数：i + 1, i - 1, i * 2, i << 1
                if(dynamic_cast<ConstInt*>(op1)) {
                    const_op = static_cast<ConstInt*>(op1);
                    iv_op = op0;
                } else {
                    continue; // 右操作数不是常量，跳过
                }
            } else if(op1 == cur_exp->inst && (binary_inst->is_add() || binary_inst->is_mul())) { // 当前归纳变量为右操作数，说明只能是乘法或者加法：1 + i, 2 * i
                if(dynamic_cast<ConstInt*>(op0)) {
                    const_op = static_cast<ConstInt*>(op0);
                    iv_op = op1;
                } else {
                    continue; // 左操作数不是常量，跳过
                }
            } else { // 不是当前归纳变量的变化表达式，跳过
                continue;
            }
            int c = const_op->val; // 常量操作数的值
            int new_k, new_b;
            if(!cur_exp->pre_exp) {
                new_k = 1;
                new_b = 0;
            } else {
                new_k = cur_exp->k; // 继承前一个计算式的 k 值
                new_b = cur_exp->b; // 继承前一个计算式的 b 值
            }
            switch(binary_inst->iid) {
                case IRInstID::Add: { // i + c
                    new_b += c;
                    break;
                }
                case IRInstID::Sub: { // i - c
                    new_b -= c;
                    break;
                }
                case IRInstID::Mul: { // i * c
                    new_k *= c;
                    new_b *= c;
                    break;
                }
                case IRInstID::Shl: { // i << c
                    new_k *= 1 << c; // 2 ^ const_val
                    new_b *= 1 << c;
                    break;
                }
                default: {
                    continue;
                }
            }
            Value* base_biv = cur_exp->base; // 基本归纳变量
            IVExp* new_exp = new IVExp(binary_inst, base_biv, new_k, new_b, cur_exp);
            loop->biv_to_divs[base_biv].push_back(new_exp); // 记录归纳变量
            worklist.push(new_exp);
            visited.insert(binary_inst);
        }
    }
}

void LoopInitializer::execute() {
    for(auto func: m->func_list) {
        if(func->bb_list.empty()) {
            continue; // 跳过声明函数
        }
        loops[func] = make_shared<vector<LoopPtr>>(); // 初始化循环列表
        detect_loops(func); // 检测函数中的循环
        build_loop_tree(func); // 设置嵌套关系
        order_loops(func); // 按照从内到外的顺序排列循环，确保内层循环先处理
        // show_loops(loops[func]); // 打印检测到的循环信息
        for(auto& loop: *loops[func]) {
            set_loop_pre_header(loop); // 设置循环前置块
            set_loop_latch(loop); // 设置循环出口
            set_basic_indu_vars(loop); // 查找基本归纳
            set_derived_indu_vars(loop); // 查找基本归纳
        }
    }
}