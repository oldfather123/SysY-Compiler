#include "InstCombine.hpp"

#include <PatternMatch.hpp>

void InstCombine::run(Function *func, AnalysisPassManager &APM) {
    (void)APM;

    bool changed;
    do {
        for (auto &BB : func->get_basic_blocks()) {
            for (auto &I : BB.get_instructions()) {
                try_optimize(&I);
            }
        }
        changed = !instr_to_delete_.empty();

        for (auto instr : instr_to_delete_)
            instr->get_parent()->remove_instr(instr);
        instr_to_delete_.clear();
    } while (changed);
}

bool InstCombine::try_optimize(Instruction *instr) {
    using namespace PatternMatch;

    Value *v;
    Constant *c1, *c2;

    // match constantint?
    // communicative?

    if (match(instr, m_add(m_value(v), m_constant(c1)))) {
        auto ci = dynamic_cast<ConstantInt *>(c1);
        assert(ci);
        if (ci->get_value() == 0) {
            instr->replace_all_use_with(v);
            instr_to_delete_.push_back(instr);
            return true;
        }
    }
    
    if (match(instr,
              m_add(m_add(m_value(v), m_constant(c1)), m_constant(c2)))) {
        auto ci1 = dynamic_cast<ConstantInt *>(c1);
        auto ci2 = dynamic_cast<ConstantInt *>(c2);
        assert(ci1 && ci2);
        int s = ci1->get_value() + ci2->get_value();
        auto new_add = IBinaryInst::create_add(
            v, ConstantInt::get(s, instr->get_module()), nullptr);
        new_add->insert_before(instr);
        instr->replace_all_use_with(new_add);
        instr_to_delete_.push_back(instr);
        return true;
    }

    return false;
}
