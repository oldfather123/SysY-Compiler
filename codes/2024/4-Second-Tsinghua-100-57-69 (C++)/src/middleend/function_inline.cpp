#include "middleend/function_inline.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/dominator_tree.hpp"
#include <iostream>
#include <queue>

namespace middleend {

std::unordered_map<int, Temp*> temp_map;

void update_phi_in_fi(ir::BasicBlock* bb, ir::BasicBlock* old_bb, ir::BasicBlock* new_bb){
    for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
        if (auto phi = dynamic_cast<ir::instruction::Phi *>(*iter)) {
            for (int i = 0; i < phi->getbbs()->size(); i++) {
                if (old_bb == phi->getbbs()->at(i)) {
                    phi->getbbs()->at(i) = new_bb;
                    break;
                }
            }
        }
        else break;
    }
}

Temp *FunctionInline::new_temp(Temp *temp, int now_temp_used) {
    if(temp == nullptr)return nullptr;
    // 检查temp是否在func_arg_temp里
    for (int i = 0; i < func_arg_temp.size(); i++){
        if (func_arg_temp[i] == temp->get_index()) return call_arg_temp[i];
    }
    int new_index = now_temp_used + temp->get_index();
    if (temp_map.find(new_index) != temp_map.end()) return temp_map[new_index];
    auto new_temp = new Temp(new_index, temp->get_type());
    temp_map[new_index] = new_temp;
    return new_temp;
}

void FunctionInline::new_bb(ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *next_bb, int now_temp_used) {
    // std::cout << "bb: " << bb->get_index() << " new_bb: " << new_bb->get_index() << " next_bb: " << next_bb->get_index() << std::endl;
    for (auto & inst : *bb->get_instructions()) {
        // std::cout << "inst: " << inst->to_str() << std::endl;
        ir::Instruction *new_inst = nullptr;
        if (auto assign = dynamic_cast<ir::instruction::Assign *>(inst)) {
            auto dst = new_temp(assign->getdst(), now_temp_used);
            auto src = new_temp(assign->getsrc(), now_temp_used);
            new_inst = new ir::instruction::Assign(dst, src);
        }
        if (auto li = dynamic_cast<ir::instruction::LoadImm4 *>(inst)) {
            auto dst = new_temp(li->getdst(), now_temp_used);
            new_inst = new ir::instruction::LoadImm4(dst, li->getimm());
        }
        if (auto unary = dynamic_cast<ir::instruction::Unary *>(inst)) {
            auto dst = new_temp(unary->getdst(), now_temp_used);
            auto src = new_temp(unary->getsrc(), now_temp_used);
            new_inst = new ir::instruction::Unary(unary->get_type(), dst, src);
        }
        if (auto binary = dynamic_cast<ir::instruction::Binary *>(inst)) {
            auto dst = new_temp(binary->getdst(), now_temp_used);
            auto lhs = new_temp(binary->getlhs(), now_temp_used);
            auto rhs = new_temp(binary->getrhs(), now_temp_used);
            new_inst = new ir::instruction::Binary(binary->get_type(), dst, lhs, rhs);
        }
        if (auto call = dynamic_cast<ir::instruction::Call *>(inst)) {
            auto dst = new_temp(call->getdst(), now_temp_used);
            std::vector<Temp *> args;
            for (auto src : call->get_srcs()){
                args.push_back(new_temp(src, now_temp_used));
            }
            new_inst = new ir::instruction::Call(dst, args, call->getfunc());
        }
        if (auto branch = dynamic_cast<ir::instruction::Branch *>(inst)) {
            new_inst = new ir::instruction::Branch(bb_map[branch->get_bb()->get_index()]);
        }
        if (auto cond = dynamic_cast<ir::instruction::CondBranch *>(inst)) {
            new_inst = new ir::instruction::CondBranch(cond->get_type(), bb_map[cond->get_true_bb()->get_index()], bb_map[cond->get_false_bb()->get_index()], new_temp(cond->getcond(), now_temp_used));
        }
        if (auto alloca = dynamic_cast<ir::instruction::Alloca *>(inst)) {
            new_inst = new ir::instruction::Alloca(new_temp(alloca->getaddr(), now_temp_used), alloca->gettype());
        }
        if (auto store = dynamic_cast<ir::instruction::Store *>(inst)) {
            new_inst = new ir::instruction::Store(new_temp(store->getaddr(), now_temp_used), new_temp(store->getsrc(), now_temp_used), store->not_delete(), store->getoffset());
        }
        if (auto load = dynamic_cast<ir::instruction::Load *>(inst)) {
            new_inst = new ir::instruction::Load(new_temp(load->getdst(), now_temp_used), new_temp(load->getaddr(), now_temp_used), load->not_delete(), load->getoffset());
        }
        if (auto la = dynamic_cast<ir::instruction::LoadAddr *>(inst)) {
            new_inst = new ir::instruction::LoadAddr(new_temp(la->get_addr(), now_temp_used), la->get_name(), la->type_);
        }
        if (auto elementptr = dynamic_cast<ir::instruction::ElementPtr *>(inst)) {
            std::vector<Temp*> indices;
            for(auto i = elementptr->get_src()->begin() + 1; i != elementptr->get_src()->end(); i++){
                indices.push_back(new_temp(*i, now_temp_used));
            }
            new_inst = new ir::instruction::ElementPtr(new_temp(elementptr->get_dst(), now_temp_used), new_temp(elementptr->get_base(), now_temp_used), indices);
        }
        if (auto phi = dynamic_cast<ir::instruction::Phi *>(inst)) {
            std::vector<Temp*> values;
            std::vector<ir::BasicBlock*> bbs;
            for(auto value : *phi->getvalues()){
                values.push_back(new_temp(value, now_temp_used));
            }
            for(auto bb : *phi->getbbs())
                bbs.push_back(bb_map[bb->get_index()]);
            new_inst = new ir::instruction::Phi(new_temp(phi->getdst(), now_temp_used), values, bbs);
        }
        if (auto ret = dynamic_cast<ir::instruction::Return *>(inst)) {
            // 修改 return 指令为 jump
            auto value = ret->getvalue();
            if(value != nullptr){
                next_bb_bbs.push_back(new_bb);
                next_bb_values.push_back(new_temp(value, now_temp_used));
            }
            new_inst = new ir::instruction::Branch(next_bb);
        }
        if (auto cast = dynamic_cast<ir::instruction::Cast *>(inst)) {
            auto dst = new_temp(cast->getdst(), now_temp_used);
            auto src = new_temp(cast->getsrc(), now_temp_used);
            new_inst = new ir::instruction::Cast(dst, src, cast->get_type());
        }
        if (auto binaryimm = dynamic_cast<ir::instruction::BinaryImm *>(inst)) {
            auto dst = new_temp(binaryimm->getdst(), now_temp_used);
            auto src = new_temp(binaryimm->getlhs(), now_temp_used);
            int imm = binaryimm->getimm();
            new_inst = new ir::instruction::BinaryImm(binaryimm->get_type(), dst, src, ConstValue(imm));
        }
        new_bb->add_instruction(new_inst);
        // std::cout << "new_inst: " << new_inst->to_str() << std::endl;
    }
}

void FunctionInline::inline_function(ir::Function *now_func) {
    bool changed = true;
    while(changed){
        changed = false;
        auto now_bbs = *now_func->get_basic_blocks();
        for (int i = 0; i < now_bbs.size(); i++) {
            auto bb = now_bbs[i];
            // std::cout << bb->get_index() << std::endl;
            for (auto inst = bb->get_instructions()->begin(); inst != bb->get_instructions()->end(); inst++){
                if (auto call = dynamic_cast<ir::instruction::Call *>(*inst)) {
                    if (can_inline_func.find(call->getfunc()->get_name()) != can_inline_func.end()) { // 进行内联
                        changed = true;
                        inline_func = call->getfunc();
                        // 新建bb
                        bb_map.clear();
                        temp_map.clear();
                        bool zero_fisrt = false;
                        for (auto bb_iter = inline_func->get_basic_blocks()->begin(); bb_iter != inline_func->get_basic_blocks()->end(); bb_iter++) {
                            auto inline_bb = *bb_iter;
                            if (bb_iter == inline_func->get_basic_blocks()->begin()) {
                                bb_map[inline_bb->get_index()] = bb;
                                if (inline_bb->get_index() == 0) zero_fisrt = true;
                                continue;
                            }
                            ir::BasicBlock *new_bb_;
                            if (zero_fisrt) {
                                new_bb_ = new ir::BasicBlock(now_func, {}, now_func->get_bb_used() + inline_bb->get_index() - 1);
                            }
                            else {
                                new_bb_ = new ir::BasicBlock(now_func, {}, now_func->get_bb_used() + inline_bb->get_index());
                            }
                            now_func->add_basic_block(new_bb_);
                            bb_map[inline_bb->get_index()] = new_bb_;
                        }
                        func_arg_temp.clear();
                        call_arg_temp.clear();
                        for (auto arg : call->get_srcs())
                            call_arg_temp.push_back(arg);
                        for (auto arg : *inline_func->get_arg_temp())
                            func_arg_temp.push_back(arg->get_index());
                        // 给新的bb增加指令
                        ir::BasicBlock *next_bb = new ir::BasicBlock(now_func, {}, now_func->get_bb_used() + inline_func->get_bb_used());
                        now_func->add_basic_block(next_bb);
                        bb->get_instructions()->erase(inst); // 删除原来的call指令
                        while (inst != bb->get_instructions()->end()) {
                            next_bb->add_instruction(*inst);
                            bb->get_instructions()->erase(inst);
                        }
                        // 更新phi指令
                        auto end_inst = next_bb->get_instructions()->at(next_bb->get_instructions()->size() - 1);
                        TypeCase(branch, ir::instruction::Branch*, end_inst) {
                            update_phi_in_fi(branch->get_bb(), bb, next_bb);
                        }
                        else TypeCase(cond_branch, ir::instruction::CondBranch*, end_inst) {
                            update_phi_in_fi(cond_branch->get_true_bb(), bb, next_bb);
                            update_phi_in_fi(cond_branch->get_false_bb(), bb, next_bb);
                        }

                        next_bb_values.clear();
                        next_bb_bbs.clear();
                        for (auto inline_bb : *inline_func->get_basic_blocks()){
                            new_bb(inline_bb, bb_map[inline_bb->get_index()], next_bb, now_func->get_temp_used());
                        }
                        // 给下一个bb新增assign或phi
                        if(call->getdst() != nullptr){
                            if(next_bb_values.size() == 1){
                                next_bb->add_instruction_front(new ir::instruction::Assign(call->getdst(), next_bb_values[0]));
                            }
                            else{
                                next_bb->add_instruction_front(new ir::instruction::Phi(call->getdst(), next_bb_values, next_bb_bbs));
                            }
                        }
                        // std::cout << "inline " << inline_func->get_name() << ": next bb is " << next_bb->get_index() << " = -1 + " << main_func->get_bb_used() << " + " << inline_func->get_bb_used() << std::endl;
                        now_func->set_temp_used(now_func->get_temp_used() + inline_func->get_temp_used());
                        now_func->set_bb_used(next_bb->get_index() + 1);
                        // now_bbs.push_back(next_bb);
                        break;
                    }
                }
            }
        }
        // 去除无用的call指令
        CFG *cfg = new CFG(now_func);
        UseDefAnalysis *ud = new UseDefAnalysis(cfg);
        auto use_list = ud->get_usesets();

        for (auto &bb: *now_func->get_basic_blocks()) {
            for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
                auto inst = *iter;
                TypeCase(call, ir::instruction::Call*, inst) {
                    if (call->getdst() == nullptr || use_list[call->getdst()].empty()) {
                        if (call->getfunc()->has_side_effect == 0) {
                            // std::cout << "remove useless call: " << call->to_str() << std::endl;
                            bb->get_instructions()->erase(iter);
                            iter--;
                        }
                    }
                }
            }
        }
    }
}

void FunctionInline::run() {
    // 判断一个函数能否被内联
    const int LONG_FUNC_LEN = 10000;
    const int TOO_LONG_FUNC_LEN = 1000000;
    for (auto func : *module_->get_functions()) {
        if (func->is_main()) continue;
        int length = 0;
        bool flag = true;
        for (auto bb : *func->get_basic_blocks()) {
            length += bb->get_instructions()->size();
            for (auto & inst : *bb->get_instructions()) {
                if (auto call = dynamic_cast<ir::instruction::Call *>(inst)) {
                    if (call->getfunc()->get_name() == func->get_name()) {
                        flag = false;
                        break;
                    }
                }
            }
        }
        if (flag && length < LONG_FUNC_LEN) {
            can_inline_func.insert(func->get_name());
        }
    }

    // 内联
    auto main_func = module_->func_map["main"];
    inline_function(main_func);
    for (auto &func : *module_->get_functions()) {
        if ((can_inline_func.find(func->get_name()) != can_inline_func.end()) || (func->get_name() == "main")) continue;
        inline_function(func);
    }
    // recursive_inline();
}

void FunctionInline::recursive_inline() {
    for (auto &func : *module_->get_functions()) {
        if ((can_inline_func.find(func->get_name()) != can_inline_func.end()) || (func->get_name() == "main")) continue;
        // std::cout << "recursive inline " << func->get_name() << std::endl;
        auto now_bbs = *func->get_basic_blocks();
        ir::Function *inline_func = new ir::Function(*func);
        std::unordered_set<int> not_inline_bb;
        std::unordered_set<ir::BasicBlock*> can_inline_bb;
        CFG *cfg = new CFG(func);
        DominatorTree_ *dt = new DominatorTree_(cfg);
        for (auto bb: now_bbs) {
            for (auto inst : *bb->get_instructions()) {
                TypeCase(call, ir::instruction::Call*, inst) {
                    if (call->getfunc()->get_name() == func->get_name()) {
                        not_inline_bb.insert(bb->get_index());
                    }
                }
            }
        }
        int max_bb_idx = 0;
        int max_temp_idx = 0;
        for (auto bb: now_bbs) {
            for (auto inst : *bb->get_instructions()) {
                TypeCase(ret, ir::instruction::Return*, inst) {
                    bool flag = true;
                    // TODO: 不应该是dom，而应该找路径
                    for (auto dom_bb: dt->get_dom(bb)) {
                        if (not_inline_bb.count(dom_bb->get_index())) {
                            flag = false;
                            break;
                        }
                    }
                    if (flag) {
                        for (auto dom_bb: dt->get_dom(bb)) {
                            can_inline_bb.insert(dom_bb);
                            if (dom_bb->get_index() > max_bb_idx) {
                                max_bb_idx = dom_bb->get_index();
                            }
                            for (auto inst: *dom_bb->get_instructions()) {
                                for (auto temp: *inst->get_dst()) {
                                    if (temp == nullptr) continue;
                                    if (temp->get_index() > max_temp_idx) {
                                        max_temp_idx = temp->get_index();
                                    }
                                }
                                for (auto temp: *inst->get_src()) {
                                    if (temp == nullptr) continue;
                                    if (temp->get_index() > max_temp_idx) {
                                        max_temp_idx = temp->get_index();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        inline_func->get_basic_blocks()->clear();
        for (auto bb: can_inline_bb) {
            // std::cout << "bb: " << bb->get_index() << std::endl;
            ir::BasicBlock *old_bb = new ir::BasicBlock(*bb);
            for (auto iter = old_bb->get_instructions()->begin(); iter != old_bb->get_instructions()->end(); iter++) {
                auto inst = *iter;
                //TODO: 处理phi
                TypeCase(cond, ir::instruction::CondBranch*, inst) {
                    if (!can_inline_bb.count(cond->get_true_bb())) {
                        old_bb->get_instructions()->erase(iter);
                        auto new_inst = new ir::instruction::Branch(cond->get_false_bb());
                        old_bb->get_instructions()->insert(iter, new_inst);
                    }
                    else if (!can_inline_bb.count(cond->get_false_bb())) {
                        old_bb->get_instructions()->erase(iter);
                        auto new_inst = new ir::instruction::Branch(cond->get_true_bb());
                        old_bb->get_instructions()->insert(iter, new_inst);
                    }
                }
            }
            inline_func->add_basic_block(old_bb);
        }
        inline_func->set_bb_used(max_bb_idx + 1);
        inline_func->set_temp_used(max_temp_idx + 1);
        inline_func->print(std::cout);
        for (int i = 0; i < now_bbs.size(); i++) {
            auto bb = now_bbs[i];
            for (auto inst = bb->get_instructions()->begin(); inst != bb->get_instructions()->end(); inst++){
                if (auto call = dynamic_cast<ir::instruction::Call *>(*inst)) {
                    if (call->getfunc()->get_name() == func->get_name()) { // 进行内联
                        // inline_func = call->getfunc();
                        // 新建bb
                        bb_map.clear();
                        bool zero_fisrt = false;
                        for (auto bb_iter = inline_func->get_basic_blocks()->begin(); bb_iter != inline_func->get_basic_blocks()->end(); bb_iter++) {
                            auto inline_bb = *bb_iter;
                            if (bb_iter == inline_func->get_basic_blocks()->begin()) {
                                bb_map[inline_bb->get_index()] = bb;
                                if (inline_bb->get_index() == 0) zero_fisrt = true;
                                continue;
                            }
                            ir::BasicBlock *new_bb_;
                            if (zero_fisrt) {
                                new_bb_ = new ir::BasicBlock(func, {}, func->get_bb_used() + inline_bb->get_index() - 1);
                            }
                            else {
                                new_bb_ = new ir::BasicBlock(func, {}, func->get_bb_used() + inline_bb->get_index());
                            }
                            func->add_basic_block(new_bb_);
                            bb_map[inline_bb->get_index()] = new_bb_;
                        }
                        func_arg_temp.clear();
                        call_arg_temp.clear();
                        for (auto arg : call->get_srcs())
                            call_arg_temp.push_back(arg);
                        for (auto arg : *inline_func->get_arg_temp())
                            func_arg_temp.push_back(arg->get_index());
                        // 给新的bb增加指令
                        ir::BasicBlock *next_bb = new ir::BasicBlock(func, {}, func->get_bb_used() + inline_func->get_bb_used());
                        func->add_basic_block(next_bb);
                        bb->get_instructions()->erase(inst); // 删除原来的call指令
                        while (inst != bb->get_instructions()->end()) {
                            next_bb->add_instruction(*inst);
                            bb->get_instructions()->erase(inst);
                        }
                        // 更新phi指令
                        auto end_inst = next_bb->get_instructions()->at(next_bb->get_instructions()->size() - 1);
                        TypeCase(branch, ir::instruction::Branch*, end_inst) {
                            update_phi_in_fi(branch->get_bb(), bb, next_bb);
                        }
                        else TypeCase(cond_branch, ir::instruction::CondBranch*, end_inst) {
                            update_phi_in_fi(cond_branch->get_true_bb(), bb, next_bb);
                            update_phi_in_fi(cond_branch->get_false_bb(), bb, next_bb);
                        }

                        next_bb_values.clear();
                        next_bb_bbs.clear();
                        for (auto inline_bb : *inline_func->get_basic_blocks()){
                            new_bb(inline_bb, bb_map[inline_bb->get_index()], next_bb, func->get_temp_used());
                        }
                        // 给下一个bb新增assign或phi
                        if(call->getdst() != nullptr){
                            if(next_bb_values.size() == 1){
                                next_bb->add_instruction_front(new ir::instruction::Assign(call->getdst(), next_bb_values[0]));
                            }
                            else{
                                next_bb->add_instruction_front(new ir::instruction::Phi(call->getdst(), next_bb_values, next_bb_bbs));
                            }
                        }
                        // std::cout << "inline " << inline_func->get_name() << ": next bb is " << next_bb->get_index() << " = -1 + " << main_func->get_bb_used() << " + " << inline_func->get_bb_used() << std::endl;
                        func->set_temp_used(func->get_temp_used() + inline_func->get_temp_used());
                        func->set_bb_used(next_bb->get_index() + 1);
                        now_bbs.push_back(next_bb);
                        break;
                    }
                }
            }
        }
        func->print(std::cout);
        for (auto &bb : *inline_func->get_basic_blocks()) {
            delete bb;
        }
        delete inline_func;
    }
}

}