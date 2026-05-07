#include "middleend/remove_recursive_tail_call.hpp"

namespace middleend {

void RemoveRecursiveTailCall::run() {
    std::vector<ir::BasicBlock*> ret_bbs;
    for (auto bb_iter = func_->get_basic_blocks()->begin(); bb_iter!= func_->get_basic_blocks()->end(); bb_iter++) {
        auto &bb = *bb_iter;
        for (auto iter = bb->get_instructions()->begin(); iter!= bb->get_instructions()->end(); iter++) {
            TypeCase(call, ir::instruction::Call*, *iter) {
                if (call->getfunc()->get_name() == func_->get_name()) {
                    // 记录尾递归的 bb
                    auto next_iter = std::next(iter);
                    TypeCase(ret, ir::instruction::Return*, *next_iter) {
                        auto dst = call->getdst();
                        if (dst != ret->getvalue()) return;
                        for (int i = 0; i < func_->get_arg_num(); i++) {
                            auto arg = (*func_->get_arg_temp())[i];
                            if (arg->get_type().is_array()) {
                                if (arg != call->get_srcs()[i]) return;
                            }
                        }
                        ret_bbs.push_back(bb);
                    }
                    else return;
                }
            }
        }
    }
    if (ret_bbs.empty()) return;
    // 在最开头插入新bb
    auto old_bb = func_->get_basic_blocks()->front();
    auto new_bb = new ir::BasicBlock(func_, {new ir::instruction::Branch(old_bb)}, func_->get_bb_used());
    func_->get_basic_blocks()->insert(func_->get_basic_blocks()->begin(), new_bb);
    func_->set_bb_used(func_->get_bb_used() + 1);

    std::vector<ir::instruction::Phi*> args_phi;
    for (int i = 0; i < func_->get_arg_num(); i++) {
        auto arg = (*func_->get_arg_temp())[i];
        if (arg->get_type().is_array()) continue;

        auto new_temp = new Temp(func_->get_temp_used(), arg->get_type());
        func_->set_temp_used(func_->get_temp_used() + 1);
        auto phi = new ir::instruction::Phi(new_temp, {arg}, {new_bb});
        // 改写寄存器
        for (auto bb: *func_->get_basic_blocks()) {
            for (auto inst : *bb->get_instructions()) {
                for (auto &src : *inst->get_src()) {
                    if (src == arg) {
                        src = new_temp;
                    }
                }
            }
        }
        args_phi.push_back(phi);
        old_bb->add_instruction_front(phi);
    }
    for (auto bb : ret_bbs) {
        bb->get_instructions()->pop_back();
        TypeCase(call, ir::instruction::Call*, bb->get_instructions()->back()) {
            int phi_idx = 0;
            for (int i = 0; i < func_->get_arg_num(); i++) {
                auto arg = (*func_->get_arg_temp())[i];
                if (arg->get_type().is_array()) continue;
                auto param = call->get_srcs()[i];
                args_phi[phi_idx++]->add_src_and_bb(bb, param);
            }
            bb->get_instructions()->pop_back();
            bb->add_instruction(new ir::instruction::Branch(old_bb));
        }
    }
}

}