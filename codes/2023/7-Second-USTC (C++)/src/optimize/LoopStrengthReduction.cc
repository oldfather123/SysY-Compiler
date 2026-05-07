#include "LoopStrengthReduction.hh"

void LoopStrengthReduction::execute()
{
    auto dce = DeadCodeEliWithBr(m_);
    m_->set_print_name();
    init_loop_info();
    strength_reduction();
    dce.execute();
    m_->set_print_name();
}

void LoopStrengthReduction::init_loop_info()
{
    loopInfo = new LoopInfo(m_);
    loopInfo->execute();
    loopSet = &(loopInfo->allLoops);
}


void LoopStrengthReduction::strength_reduction()
{
    for(auto loop : *loopSet)
    {
        loop->find_dependent_iduction_var();
        std::cout<<loop->get_ind_vars().size()<<std::endl;
        loop->debug_print_basic_iv();
        for(auto iv : loop->get_ind_vars())
        {
            if(iv->is_dependent_iv())
            {
                auto coefCons = Utils::get_const_int_val(iv->coef);
                auto consCons = Utils::get_const_int_val(iv->cons);
                auto basis = iv->basis;
                auto preHeader = loop->get_preheader();
                auto endBlock = loop->get_end_block();

                if(coefCons.has_value() && coefCons == 1 && consCons.has_value())
                    continue;
                
                if(coefCons.has_value() && coefCons == 1)
                {
                    if(consCons == 0)
                    {
                        iv->inst->replace_all_use_with(basis->inst);
                        continue;
                    }
                    else
                    {
                        //TODO
                        continue;
                    }
                }
                
                auto stepVal = Utils::insert_mul_before_br(basis->stepVal, iv->coef, m_, preHeader);
                auto initVal = Utils::insert_mul_before_br(basis->initVal, iv->coef, m_, preHeader);
                initVal = Utils::insert_add_before_br(initVal, iv->cons, m_, preHeader);
                
                if(basis->updateOp == Loop::InductionVar::UpdateOp::add)
                {
                    initVal = Utils::insert_sub_before_br(initVal, stepVal, m_, preHeader);
                }
                else if(basis->updateOp == Loop::InductionVar::UpdateOp::sub)
                {
                    initVal = Utils::insert_add_before_br(initVal, stepVal, m_, preHeader);
                }
                else
                {
                    std::cout << "Error" << std::endl;
                    continue;
                }

                
                

                auto ivPhi = PhiInst::create_phi(Type::get_int32_type(m_), loop->getBase());
                ivPhi->add_phi_pair_operand(initVal, preHeader);
                
                Value *newStepInst = (basis->updateOp == Loop::InductionVar::UpdateOp::add) ? 
                    newStepInst = Utils::insert_add(ivPhi, stepVal, m_, endBlock)
                    : newStepInst = Utils::insert_sub(ivPhi, stepVal, m_, endBlock);
                dynamic_cast<Instruction*>(newStepInst)->get_parent()->get_instructions().pop_back();
                loop->insert_at_begin(newStepInst);

                ivPhi->add_phi_pair_operand(newStepInst, endBlock);
                loop->getBase()->get_instructions().push_front(ivPhi);

                iv->inst->replace_all_use_with(newStepInst);
                iv->inst->get_parent()->delete_instr(iv->inst);
            }
        }
    }
}