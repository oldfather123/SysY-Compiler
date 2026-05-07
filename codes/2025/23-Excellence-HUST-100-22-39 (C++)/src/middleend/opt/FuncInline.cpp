#include <queue>
#include <algorithm>
#include "FuncInfo.hpp"
#include "FuncInline.hpp"

void FuncInline::collect_func_info() {
    for(auto& func: m->func_list) {
        func_info_map[func] = FuncInfo(func);
        // 如果指令数量过多或基本块数量过多，不适合内联
        if(func_info_map[func].inst_count > 50 || func_info_map[func].bb_count > 10 || !func_info_map[func].is_pure || func->name == "main") {
            func_info_map[func].is_inlineable =  false;
        } else {
            func_info_map[func].is_inlineable =  true;
        }
    }
    // 根据 callee 的信息设置 caller
    for(auto& [caller, caller_info]: func_info_map) {
        for(auto& callee: caller_info.callee_funcs) {
            func_info_map[callee].caller_funcs.insert(caller);
        }
    }
}

vector<Function*> FuncInline::sort_function() {
    unordered_map<Function*, int> in_degree;
    unordered_map<Function*, vector<Function*>> adj;
    vector<Function*> result;
    queue<Function*> q;
    // 构建邻接表和入度表
    for(auto& [func, info]: func_info_map) {
        if(!in_degree.count(func)) {
            in_degree[func] = 0;
        }
        for(auto callee: info.callee_funcs) {
            adj[callee].push_back(func);  // 注意反过来：被调用者指向调用者
            in_degree[func]++;
        }
    }
    // 初始化队列：入度为 0 的函数（最底层被调用者）
    for(auto& [func, deg]: in_degree) {
        if (deg == 0) {
            q.push(func);
        }
    }
    // 拓扑排序
    while(!q.empty()) {
        Function* current = q.front();
        q.pop();
        result.push_back(current);
        for(Function* next : adj[current]) {
            in_degree[next]--;
            if(in_degree[next] == 0) {
                q.push(next);
            }
        }
    }
    return result;
}

void FuncInline::normalize_inline_func(Function* callee) {
    // cerr << "[Info] normalizing inline function [" << callee->name << "]\n";
    vector<ReturnInst*> ret_insts;
    for(auto& bb: callee->bb_list) {
        Instruction* terminator_inst = bb->get_terminator();
        if(auto ret_inst = dynamic_cast<ReturnInst*>(terminator_inst)) {
            ret_insts.push_back(ret_inst);
        }
    }
    if(ret_insts.size() <= 1) {
        // cerr << "[Info] no need to normalize, only " << ret_insts.size() << " return instructions found.\n";
        return; // 如果返回指令数量小于等于 1，则不需要规范化
    }
    // cerr << "[Info] found " << ret_insts.size() << " return instructions, normalizing...\n";
    BasicBlock* ret_bb = new BasicBlock(m, "label_merge_ret", callee);
    if(callee->get_ret_type() == m->void_type) {
        for(auto& ret_inst: ret_insts) {
            ret_inst->parent->add_inst_before_terminator(new BranchInst(ret_bb, ret_inst->parent));
            ret_inst->parent->delete_inst(ret_inst);
        }
        ret_bb->add_inst_back(new ReturnInst(ret_bb));
    } else {
        vector<Value*> ret_vals;
        vector<BasicBlock*> ret_bbs;
        for(auto& ret_inst: ret_insts) {
            // 将所有返回指令的操作数合并到一个新的返回指令中
            if(ret_inst->operands.size() > 0) {
                ret_vals.push_back(ret_inst->get_op(0));
                ret_bbs.push_back(ret_inst->parent);
            }
            // 删除原来的返回指令
            ret_inst->parent->add_inst_before_terminator(new BranchInst(ret_bb, ret_inst->parent));
            ret_inst->parent->delete_inst(ret_inst);
        }
        PhiInst* phi_inst = new PhiInst(callee->get_ret_type(), ret_vals, ret_bbs, ret_bb, InsertPos::None);
        ret_bb->add_inst_back(phi_inst);
        ret_bb->add_inst_back(new ReturnInst(phi_inst, ret_bb));
    }
}

void FuncInline::inline_func_with_one_bb(Function* callee, Function* caller) {
    // cerr << "[Info] inlining callee with one bb [" << callee->name << "] in caller [" << caller->name << "]\n";
    auto& caller_info = func_info_map[caller];
    vector<CallInst*> call_inst_to_delete; // 需要删除的 call 指令
    for(auto& call_inst: caller_info.call_insts) {
        if(call_inst->get_op(call_inst->operands.size() - 1) != callee) {
            // cerr << "[Info] NOT match call instruction: " << call_inst->print() << "\n";
            continue; // 如果调用指令不匹配，跳过
        }
        // cerr << "[Info] MATCH call instruction: " << call_inst->print() << "\n";
        op_map.clear();
        Value* callee_ret_val = nullptr; // 用于记录 callee 的返回值
        // 建立形参到实参的映射
        auto actual_args = call_inst->operands;
        actual_args.pop_back(); // 移除最后一个操作数，即函数本身
        auto formal_args = callee->args;
        for(int i = 0; i < formal_args.size(); ++i) {
            if(i < actual_args.size()) {
                Value* actual_arg = actual_args[i];
                Value* formal_arg = formal_args[i];
                // cerr << "[Info] record " << formal_arg->print() << " as " << actual_arg->print() << "\n";
                op_map[formal_arg] = actual_arg;
            }
        }
        BasicBlock* callee_bb = callee->bb_list.front();
        BasicBlock* call_bb = call_inst->parent;
        // 直接将 callee 的指令插入到 caller 的 call_bb 中
        for(auto& inst: callee_bb->inst_list) {
            if(auto ret_inst = dynamic_cast<ReturnInst*>(inst)) {
                // cerr << "[Info] found return inst in callee: " << ret_inst->print() << "\n";
                if(ret_inst->operands.size() != 0) {
                    if(op_map.count(ret_inst->get_op(0))) {
                        callee_ret_val = op_map[ret_inst->get_op(0)]; // 记录返回值
                        // cerr << "[Info] found return value in caller: " << callee_ret_val->print() << "\n";
                    } else {
                        callee_ret_val = ret_inst->get_op(0); // 直接使用原值
                        // cerr << "[Info] return value not found in op_map: " << callee_ret_val->print() << "\n";
                    }
                }
                continue;
            }
            // cerr << "[Info] inserting instruction from callee: " << inst->print() << "\n";
            Instruction* new_inst = inst->clone(call_bb); // 克隆指令到 before_bb
            new_inst->name = "v" + std::to_string(caller->ssa_id++); // 设置新指令的名称
            // new_inst->parent = call_bb; // 设置新指令的父基本块为调用指令所在的基本块
            for(int i = 0; i < new_inst->operands.size(); i++) {
                auto& op = new_inst->operands[i];
                if(op_map.count(op)) {
                    // cerr << "[Info] replace op\n";
                    new_inst->set_op(i, op_map[op]);
                    // op = op_map[op];
                    // op->add_use(new_inst, i); // 更新操作数的use关系
                }
            }
            // cerr << "[Info] adding instruction: " << new_inst->print() << "\n";
            call_bb->add_inst_before_inst(new_inst, call_inst);
            // 更新操作数映射
            op_map[inst] = new_inst;
            // cerr << "[Info] record " << inst->print() << " as " << new_inst->print() << "\n";
        }
        // 替换调用语句的返回值
        if(callee_ret_val) {
            // cerr << "[Info] replacing call instruction " << call_inst->print() << " with return value: " << callee_ret_val->print() << "\n";
            call_inst->replace_use_with(callee_ret_val); // 替换调用语句的返回值
        }
        call_bb->delete_inst(call_inst); // 删除调用指令
        call_inst_to_delete.push_back(call_inst); // 记录需要删除的调用指令
        // cerr << "----------------------------------------------------------------------------------------------\n" << caller->print() << "\n----------------------------------------------------------------------------------------------\n";
    }
}

pair<BasicBlock*, BasicBlock*> FuncInline::split_call_inst(CallInst* call_inst, Function* callee, Function* caller) {
    // cerr << "[Info] splitting call inst's parent block: " << call_inst->parent->parent->name << "::" << call_inst->parent->name << "\n";
    bool before = true;
    BasicBlock* before_bb = call_inst->parent;
    BasicBlock* after_bb = new BasicBlock(m, "label_" + to_string(caller->ssa_id++), caller);
    vector<Instruction*> inst_to_move;
    // 首先捕获需要移动的指令
    for(auto& inst: before_bb->inst_list) {
        if(before) { // 调用语句之前的指令
            // cerr << "[Info] before call inst: " << inst->print() << "\n";
            if(inst == call_inst) {
                before = false;
            }
        } else { // 调用语句之后的指令
            // cerr << "[Info] after call inst: " << inst->print() << "\n";
            inst_to_move.push_back(inst); // 收集需要移动的指令
        }
    }
    // 移动指令
    for(auto& inst: inst_to_move) {
        before_bb->remove_inst(inst); // 从 before_bb 中删除指令
        after_bb->add_inst_back(inst); // 将指令移动到 after_bb
    }
    // 清理 CFG 关系
    for(auto& succ_bb: before_bb->succ_bbs) {
        succ_bb->remove_pre_basic_block(before_bb); // 清除 before_bb 的后继基本块的前驱关系
        succ_bb->add_pre_basic_block(after_bb);     // 添加 after_bb 作为前驱基本块
        after_bb->add_succ_basic_block(succ_bb);    // 继承原有后继基本块
    }
    before_bb->succ_bbs.clear();                   // 清除 before_bb 的后继基本块
    // 处理控制流中关于 before_bb 的 Phi 指令
    for(auto& use: before_bb->use_list) {
        if(auto phi_inst = dynamic_cast<PhiInst*>(use.val)) {
            // cerr << "[Info] found phi inst using before_bb: " << phi_inst->print() << "\n";
            // 更新 Phi 指令的操作数
            for(int i = 0; i < phi_inst->operands.size(); i += 2) {
                BasicBlock* old_bb = dynamic_cast<BasicBlock*>(phi_inst->get_op(i + 1));
                if(old_bb == before_bb) {
                    // cerr << "[Info] updating phi inst operand from " << old_bb->name << " to " << after_bb->name << "\n";
                    phi_inst->set_op(i + 1, after_bb); // 更新基本块
                }
            }
        }
    }
    return make_pair(before_bb, after_bb); // 返回拆分后的基本块
}

void FuncInline::build_formal_to_actual_arg_map(CallInst* call_inst, Function* callee) {
    // cerr << "[Info] building formal to actual argument map\n";
    auto actual_args = call_inst->operands;
    actual_args.pop_back(); // 移除最后一个操作数，即函数本身
    auto formal_args = callee->args;
    for(int i = 0; i < formal_args.size(); ++i) {
        if(i < actual_args.size()) {
            Value* actual_arg = actual_args[i];
            Value* formal_arg = formal_args[i];
            // cerr << "[Info] record " << formal_arg->print() << " as " << actual_arg->print() << "\n";
            op_map[formal_arg] = actual_arg;
        }
    }
}

void FuncInline::copy_bb_from_callee(Function* callee, Function* caller, BasicBlock* before_bb) {
    // cerr << "[Info] building basic block map\n";
    for(auto& bb: callee->bb_list) {
        if(bb == callee->bb_list.front()) { // callee::label_entry 不进行创建，直接使用 before_bb 作为入口即可
            op_map[bb] = before_bb;
            before_bb->succ_bbs = bb->succ_bbs; // 继承 callee 的后继基本块
            continue;
        }
        BasicBlock* new_bb = new BasicBlock(m, "label_" + std::to_string(caller->ssa_id++), caller);
        // 继承来自 callee 的前驱后继关系，后续进行映射替换，改为 caller 中对应的前驱后继
        new_bb->pre_bbs = bb->pre_bbs;
        new_bb->succ_bbs = bb->succ_bbs;
        // cerr << "[Info] record " << bb->name << " as " << new_bb->name << "\n";
        op_map[bb] = new_bb; // 记录基本块映射关系
    }
}

void FuncInline::map_caller_bb_pre_succ(Function* caller) {
    for(auto& bb: caller->bb_list) {
        for(auto& pre_bb: bb->pre_bbs) {
            if(op_map.count(pre_bb)) {
                // cerr << "[Info] replace pre_bb: " << pre_bb->name << " of " << bb->name << "\n";
                pre_bb = static_cast<BasicBlock*>(op_map[pre_bb]); // 替换前驱基本块
            }
        }
        for(auto& succ_bb: bb->succ_bbs) {
            if(op_map.count(succ_bb)) {
                // cerr << "[Info] replace succ_bb: " << succ_bb->name << " of " << bb->name << "\n";
                succ_bb = static_cast<BasicBlock*>(op_map[succ_bb]); // 替换后继基本块
            }
        }
    }
}

void FuncInline::copy_inst(Function* callee, Function* caller, CallInst* call_inst, BasicBlock* after_bb, vector<PhiInst*>& phi_inst_to_update) {
    Value* callee_ret_val = nullptr; // 用于记录 callee 的返回值
    for(auto& bb: callee->bb_list) {
        BasicBlock* new_bb = static_cast<BasicBlock*>(op_map[bb]); // 获取对应的新的基本块
        for(auto& inst: bb->inst_list) {
            if(auto ret_inst = dynamic_cast<ReturnInst*>(inst)) {
                // cerr << "[Info] found return inst in callee: " << ret_inst->print() << "\n";
                if(ret_inst->operands.size() != 0) {
                    callee_ret_val = ret_inst->get_op(0); // 直接使用原值
                }
                // 在返回块加入跳转到 after_bb
                new_bb->add_inst_back(new BranchInst(after_bb, new_bb));
                continue;
            } else if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
                // cerr << "[Info] found phi inst in callee: " << phi_inst->print() << "\n";
                Instruction* new_phi_inst = phi_inst->clone(new_bb);
                new_phi_inst->name = "v" + std::to_string(caller->ssa_id++);
                op_map[inst] = new_phi_inst; // 记录指令映射关系
                // cerr << "[Info] record phi " << inst->print() << " as " << new_phi_inst->print() << "\n";
                new_bb->add_inst_back(new_phi_inst); // 添加到新的基本块
                phi_inst_to_update.push_back(static_cast<PhiInst*>(new_phi_inst)); // 记录需要更新的 Phi 指令
                continue; // Phi指令特殊处理后跳过后续逻辑
            }
            // cerr << "[Info] inserting instruction from callee: " << inst->print() << "\n";
            Instruction* new_inst = inst->clone(new_bb); // 复制指令
            new_inst->name = "v" + std::to_string(caller->ssa_id++);
            new_inst->parent = new_bb;
            // cerr << "[Info] copying instruction: " << new_inst->print() << "\n";
            op_map[inst] = new_inst; // 记录指令映射关系
            if(bb == callee->bb_list.front()) {
                new_bb->add_inst_before_inst(new_inst, call_inst);
            } else {
                new_bb->add_inst_back(new_inst); // 添加到新的基本块
            }
        }
        // cerr << "[Info] new basic block created ---------------------------------------------------------------\n" << new_bb->print() << "----------------------------------------------------------------------------------------------\n";
    }
    // 创建完所有指令之后统一进行操作数的替换
    for(auto& bb: callee->bb_list) {
        BasicBlock* new_bb = static_cast<BasicBlock*>(op_map[bb]); // 获取对应的新的基本块
        for(auto& inst: new_bb->inst_list) {
            for(int i = 0; i < inst->operands.size(); i++) {
                auto& op = inst->operands[i];
                if(op_map.count(op)) {
                    // cerr << "[Info] replace op\n";
                    inst->set_op(i, op_map[op]); // 替换操作数
                    // op = op_map[op]; // 替换操作数
                    // op->add_use(inst, i); // 更新操作数的use关系
                }
            }
        }
    }
    if(callee_ret_val) {
        if(op_map.count(callee_ret_val)) {
            callee_ret_val = op_map[callee_ret_val]; // 如果返回值在映射中，使用映射后的值
        }
        // cerr << "[Info] replacing call instruction " << call_inst->print() << " with return value: " << callee_ret_val->print() << "\n";
        call_inst->replace_use_with(callee_ret_val); // 替换调用语句的返回值
    }
}

void FuncInline::update_phi_operands(vector<PhiInst*>& phi_inst_to_update) {
    for(auto& phi_inst: phi_inst_to_update) {
        for(int i = 0; i < phi_inst->operands.size(); i++) {
            auto& op = phi_inst->operands[i];
            if(op_map.count(op)) {
                // cerr << "[Info] replace phi op\n";
                phi_inst->set_op(i, op_map[op]); // 替换操作数
                // op = op_map[op];
                // op->add_use(phi_inst, i); // 更新操作数的use关系
            }
        }
    }
}

void FuncInline::inline_func_with_multiple_bb(Function* callee, Function* caller) {
    // cerr << "[Info] inlining callee with multiple bb [" << callee->name << "] in caller [" << caller->name << "]\n";
    auto& caller_info = func_info_map[caller];
    vector<CallInst*> call_inst_to_delete; // 已经内联的 call 语句
    for(auto& call_inst: caller_info.call_insts) {
        if(call_inst->get_op(call_inst->operands.size() - 1) != callee) {
            // cerr << "[Info] NOT match call instruction: " << call_inst->print() << "\n";
            continue; // 如果调用指令不匹配，跳过
        }
        // cerr << "[Info] MATCH call instruction: " << call_inst->print() << "\n";
        //* ================================================================================================================
        //* Step 1 拆分 caller 调用语句前后的指令
        pair<BasicBlock*, BasicBlock*> split_bbs = split_call_inst(call_inst, callee, caller);
        BasicBlock* before_bb = split_bbs.first; // 调用语句之前的基本块
        BasicBlock* after_bb = split_bbs.second; // 调用语句之后的基本块
        //* ================================================================================================================
        //* Step 2 创建新基本块，建立形参到实参、callee 基本块到 caller 新基本块的映射
        op_map.clear(); // 每个函数的每次内联都需要清空操作数映射
        build_formal_to_actual_arg_map(call_inst, callee); // 建立形参到实参的映射
        copy_bb_from_callee(callee, caller, before_bb); // 在 caller 中创建新基本块，同时建立 callee 基本块到 caller 新基本块的映射
        //* ================================================================================================================
        //* Step 3 复制 callee 函数的基本块（指令）到 caller 中
        vector<PhiInst*> phi_inst_to_update; // 需要更新操作数的 Phi 指令
        copy_inst(callee, caller, call_inst, after_bb, phi_inst_to_update); // 移植 callee 指令到 caller 中
        map_caller_bb_pre_succ(caller); // 映射前驱后继关系
        update_phi_operands(phi_inst_to_update); // 替换 Phi 指令的操作数
        //* ================================================================================================================
        call_inst_to_delete.push_back(call_inst); // 记录需要删除的调用语句
        // cerr << "----------------------------------------------------------------------------------------------\n" << caller->print() << "\n----------------------------------------------------------------------------------------------\n";
    }
    // 删除已经内联的 call 语句，避免重复内联
    for(auto& call_inst: call_inst_to_delete) {
        // cerr << "[Info] deleting call instruction: " << call_inst->print() << "\n";
        call_inst->parent->delete_inst(call_inst); // 删除调用语句
        caller_info.call_insts.erase(std::remove(caller_info.call_insts.begin(), caller_info.call_insts.end(), call_inst), caller_info.call_insts.end());
    }
    // cerr << "----------------------------------------------------------------------------------------------\n" << caller->print() << "\n----------------------------------------------------------------------------------------------\n";
}

void FuncInline::execute() {
    // cerr << "[Info] Enter Function Inlining Optimization\n";
    collect_func_info();
    vector<Function*> sorted_funcs = sort_function();
    for(auto& callee: sorted_funcs) {
        auto& callee_info = func_info_map[callee];
        if(callee_info.is_inlineable) {
            // cerr << "[Info] inlining function: " << callee->name << "\n";
            normalize_inline_func(callee); // 规范化内联函数，确保没有多余的基本块
            // 对每一个调用者进行内联
            for(auto& caller: callee_info.caller_funcs) {
                if(callee->bb_list.size() == 1) {
                    inline_func_with_one_bb(callee, caller);
                } else {
                    inline_func_with_multiple_bb(callee, caller);
                }
            }
            // 清除已内联函数的信息
            // cerr << "[Info] removing callee function: " << callee->name << "\n";
            func_info_map.erase(callee);
            m->func_list.erase(std::remove(m->func_list.begin(), m->func_list.end(), callee), m->func_list.end());
        }
    }
}