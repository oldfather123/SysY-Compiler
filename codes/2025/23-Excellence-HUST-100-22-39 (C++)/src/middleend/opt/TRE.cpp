#include <map>
#include <algorithm>
#include "TRE.hpp"

// TODO: 对 call 语句进行 sink，并识别更多尾递归的模式
// TODO: 标识 tail call 并在后端处理
// TODO: 将一般递归转化为尾递归？
void TailRecurEliminate::run(Function* func) {
    if (func->bb_list.empty())
        return;

    // 判断尾递归，目前只接收 
    // bb:
    //   ...
    //   %x = call func(...)
    //   ret %x
    std::vector<CallInst *> recur_calls;
    for (auto BB: func->bb_list) {
        for (auto I : BB->inst_list) {
            if (const auto call = dynamic_cast<CallInst *>(I)) {
                if (call->get_op(0) != func)
                    continue;
                
                const auto next = std::next(std::find(BB->inst_list.begin(), BB->inst_list.end(), I));
                if (next == BB->inst_list.end())
                    continue;
                auto next_inst = *next;

                if (!next_inst->is_ret())
                    continue;
                
                if (next_inst->get_op(0) != call)
                    continue;

                recur_calls.push_back(call);
            }
        }
    }

    if (recur_calls.empty())
        return;

    //找到入口块
    const auto entry = func->bb_list[0];


    const auto head = new BasicBlock(func->parent, "new_Tail", func);

    head->inst_list = std::move(entry->inst_list);

    for (auto succ : entry->succ_bbs) {
        succ->remove_pre_basic_block(entry);
        succ->add_pre_basic_block(head);
        for(auto inst : succ->inst_list)
        {
            if(!(inst->is_phi())) break;
            for(unsigned i = 1; i < inst->operands.size(); i+=2)
            {
                if(inst->operands[i] == entry)
                    inst->operands[i] = head;
            }
        }
    }

    head->succ_bbs = std::move(entry->succ_bbs);

    func->bb_list[0]->add_inst_back(new BranchInst(head,func->bb_list[0]));

    //
    std::vector<PhiInst *> arg_phi;
    for (auto arg : func->args) {
        PhiInst* phi = new PhiInst(arg->type, {}, {}, head, InsertPos::Head);
        // auto phi = PhiInst::create_phi(arg->type, head);
        // head->add_inst_front(phi);
        arg->replace_use_with(phi);
        phi->add_ops(arg, entry);
        arg_phi.push_back(phi);
    }

    //
    for (auto call : recur_calls) {
        auto bb = call->parent;
        auto instr_list = bb->inst_list;

        auto opds = call->operands;

        for (size_t i = 1; i < opds.size(); ++i) {
            const auto phi = arg_phi[i - 1];
            phi->add_ops(opds[i], bb);
        }

        instr_list.pop_back(); // ret
        instr_list.pop_back(); // call
        instr_list.push_back(new BranchInst(head, bb));
    }

}

void TailRecurEliminate::recover_ret(Function* func){
    std::map<BasicBlock*,Value*> ret_phi_map;
    for(auto bb:func->bb_list){
        //找到ret块
        if(bb->name == "label_ret"){
            //指令数大于1，说明存在phi块，我们给他返回去
            if(bb->inst_list.size() > 1){
                // std::cout << "ret phi" << bb->inst_list.size() << std::endl;
                auto instr = bb->inst_list.front();
                for (int i = 0; i < instr->operands.size(); i += 2){
                    auto val = instr->operands[i];

                    auto label = instr->operands[i+1];
                    ret_phi_map[dynamic_cast<BasicBlock*>(label)] = val;
                }
            }
            break;
        }
    }

    for(auto bb:func->bb_list){
        // std::cout << bb->name << std::endl;
        if(bb->name == "label_ret") continue;
        //如果存在这种
        if(ret_phi_map.find(bb) != ret_phi_map.end() && bb->get_terminator()->is_br()){
            if(bb->get_terminator()->operands.size() == 1){
                // 删除bb最后的br语句
                bb->inst_list.pop_back();
                bb->add_inst_back(new ReturnInst(dynamic_cast<Value*>(ret_phi_map[bb]),bb));
            }
        }
    }

}


void TailRecurEliminate::execute(){
    for(auto func:m->func_list){
        // std::cout << func->name << std::endl;
        recover_ret(func);
        run(func);
    }
}