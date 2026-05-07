#include "ConstProp.hh"
#include "DeadCodeEliWithBr.hh"
#include "TailRecursionElim.hh"

#include "utils.hh"
using std::string_literals::operator""s;
void TailRecursionElim::execute()
{
    LOG(DEBUG) << "tail recursion hello";
    for (auto f : m_->get_functions())
    {
        if (f->get_return_type()->is_void_type())
            continue;
        f_ = f;
        preheader = latch = header = return_bb = nullptr;
        phi_args.clear();
        args_latch.clear();
        ret_phi = nullptr;
        acc_phi = acc_latch = nullptr;
        if (f_->get_basic_blocks().size() == 0)
            continue;
        for (auto bb : f->get_basic_blocks())
        {
            cur_bb = bb;
            if (auto call = get_candidate(cur_bb))
            {
                auto b = eliminate_call(call);
            }
        }
    }

    auto cp = ConstProp(m_);
    cp.execute();

    auto dce = DeadCodeEliWithBr(m_);
    dce.execute();
}

bool TailRecursionElim::eliminate_call(CallInst *call)
{
    auto parent = call->get_parent();
    // const auto call_it = call->get_iterator();
    auto it = cur_bb->get_instructions().begin();
    for (; it != cur_bb->get_instructions().end(); it++)
    {
        auto call_instr = dynamic_cast<CallInst *>(*it);
        if (call_instr && call_instr == call)
            break;
    }
    if (it == cur_bb->get_instructions().end())
    {
        return false;
    }
    it++;
    // auto it = ++call->get_iterator();
    auto term_it = call->get_parent()->get_terminator_itr();
    Instruction *accumulator_or_call = dynamic_cast<Instruction *>(call);
    // 若通过中间的空跳转块，无法消除
    if (!(*term_it)->is_br())
    {
        return false;
    }
    if ((*term_it)->get_num_operands() == 3)
    {
        return false;
    }
    BasicBlock *bb = dynamic_cast<BasicBlock *>((*term_it)->get_operand(0));
    if (!bb->get_terminator()->is_ret())
    {
        return false;
    }
    else
    {
        ret_phi = dynamic_cast<Instruction *>(bb->get_terminator()->get_operand(0));
        return_bb = bb;
    }

    if (it != term_it)
        return false;
    int id = -1;
    for (auto use : accumulator_or_call->get_use_list())
        if (use.val_ == ret_phi)
        {
            id = use.arg_no_;
            break;
        }
    if (id == -1)
    {
        LOG(WARNING) << "did not find use of call";
        return false;
    }
    if (not preheader)
        create_header();

    for (size_t i = 1; i < call->get_num_operands(); ++i)
    {
        auto op = call->get_operand(i);
        args_latch[i - 1]->add_phi_pair_operand(op, call->get_parent());
    }
    ret_phi->remove_operands(id, id + 1);
    for (int i=0; i < (int)ret_phi->get_num_operands()/2; i++){
        auto value = ret_phi->get_operand(i*2);
        auto bb = ret_phi->get_operand(i*2 + 1);
        value->remove_use(ret_phi);
        value->add_use(ret_phi,i*2);
        bb->remove_use(ret_phi);
        bb->add_use(ret_phi,i*2+1);
    }
    parent->delete_instr(call);
    parent->delete_instr(*(parent->get_terminator_itr()));
    for (auto succ : parent->get_succ_basic_blocks())
        succ->remove_pre_basic_block(parent);
    parent->get_succ_basic_blocks().clear();
    BranchInst::create_br(latch, parent);

    return true;
}

CallInst *TailRecursionElim::get_candidate(BasicBlock *bb)
{
    auto &list = bb->get_instructions();
    for (auto it = list.rbegin(); it != list.rend(); ++it)
    {
        auto call = dynamic_cast<CallInst *>(*it);
        if (call && dynamic_cast<Function *>(call->get_operand(0)) == f_)
        {
            return call;
        }
    }
    return {};
}

void TailRecursionElim::create_header()
{
    auto original_entry = header = f_->get_entry_block();
    preheader = BasicBlock::create(m_, "", f_);
    f_->get_basic_blocks().pop_back();
    f_->get_basic_blocks().push_front(preheader);
    preheader->take_name(original_entry);
    BranchInst::create_br(original_entry, preheader);
    latch = BasicBlock::create(m_, "", f_);
    latch->set_name("tail_latch");
    BranchInst::create_cond_br(ConstantInt::get(true, m_), original_entry, return_bb, latch);
    original_entry->set_name("tail_header");

    std::vector<Instruction*> delete_list;
    for(auto instr: original_entry->get_instructions()){
        if(instr->is_alloca()){
            delete_list.push_back(instr);
        }
    }
    for(auto instr: delete_list){
        original_entry->delete_instr(instr);
        preheader->add_instr_begin(instr);
    }


    for (auto arg : f_->get_args())
    {
        PhiInst *phi_arg = PhiInst::create_phi(arg->get_type(), original_entry);
        original_entry->add_instr_begin(dynamic_cast<Instruction *>(phi_arg));
        PhiInst *phi_latch = PhiInst::create_phi(arg->get_type(), latch);
        latch->add_instr_begin(dynamic_cast<Instruction *>(phi_latch));
        arg->replace_all_use_with(phi_arg);
        phi_arg->add_phi_pair_operand(dynamic_cast<Value *>(arg), preheader);
        phi_arg->add_phi_pair_operand(dynamic_cast<Value *>(phi_latch), latch); // we might have multiple tail calls
        phi_args.push_back(phi_arg);
        args_latch.push_back(phi_latch);
    }
}
