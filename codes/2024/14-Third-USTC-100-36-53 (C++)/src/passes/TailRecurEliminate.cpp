#include "TailRecurEliminate.hpp"
#include "FuncInfo.hpp"

// TODO: 对 call 语句进行 sink，并识别更多尾递归的模式
// TODO: 标识 tail call 并在后端处理
// TODO: 将一般递归转化为尾递归？
void TailRecurEliminate::run(Function *func, AnalysisPassManager &APM) {
    if (func->is_declaration())
        return;

    // 判断尾递归，目前只接收 
    // bb:
    //   ...
    //   %x = call func(...)
    //   ret %x
    std::vector<CallInst *> recur_calls;
    for (auto &BB: func->get_basic_blocks()) {
        for (auto &I : BB.get_instructions()) {
            if (const auto call = dynamic_cast<CallInst *>(&I)) {
                if (call->get_operand(0) != func)
                    continue;

                const auto next = call->getNext();
                if (!next->is_ret())
                    return;
                assert(next == BB.get_terminator());
                if (next->get_operand(0) != call)
                    return;

                recur_calls.push_back(call);
            }
        }
    }

    if (recur_calls.empty())
        return;

    //
    const auto entry = func->get_entry_block();
    const auto head = BasicBlock::create(func->get_parent(), "", func);
    head->get_instructions() = std::move(entry->get_instructions());
    for (auto succ : entry->get_succ_basic_blocks()) {
        succ->remove_pre_basic_block(entry);
        succ->add_pre_basic_block(head);
        for(auto &inst : succ->get_instructions())
        {
            if(!(&inst)->is_phi()) break;
            for(unsigned i = 1; i < (&inst)->get_num_operand(); i+=2)
            {
                if((&inst)->get_operand(i) == entry)
                    (&inst)->set_operand(i, head);
            }
        }
    }
    head->get_succ_basic_blocks() = std::move(entry->get_succ_basic_blocks());
    BranchInst::create_br(head, func->get_entry_block());

    //
    std::vector<PhiInst *> arg_phi;
    for (auto &arg : func->get_args()) {
        auto phi = PhiInst::create_phi(arg.get_type(), head);
        head->add_instr_begin(phi);
        arg.replace_all_use_with(phi);
        phi->add_phi_pair_operand(&arg, entry);
        arg_phi.push_back(phi);
    }

    //
    for (auto call : recur_calls) {
        auto bb = call->get_parent();
        auto &instr_list = bb->get_instructions();

        auto opds = call->get_operands();

        for (size_t i = 1; i < opds.size(); ++i) {
            const auto phi = arg_phi[i - 1];
            phi->add_phi_pair_operand(opds[i], bb);
        }

        instr_list.pop_back(); // ret
        instr_list.pop_back(); // call
        BranchInst::create_br(head, bb);
    }

    APM.invalidateAll<Function>(func);
    APM.invalidate<FuncInfo>(func->get_parent());
}
