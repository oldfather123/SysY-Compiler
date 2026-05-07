#include "middleend/gep_deconstruction.hpp"

namespace middleend {

void ElementPtrDeconstruct::run() {
    auto reg_id = func->get_temp_used();
    for (auto &bb : *func->get_basic_blocks()) {
        // std::cout << "before: "; bb->print(std::cout);
        for (auto it = (*bb->get_instructions()).begin(); it != (*bb->get_instructions()).end();) {
            // std::cout << "inst "; (*it)->print(std::cout);
            TypeCase (gep, ir::instruction::ElementPtr *, *it) {
                if (gep->get_indices().size() == 1) { it++; continue; }
                // std::cout << "gep: "; (*it)->print(std::cout);
                std::vector<ir::Instruction *> insts_to_insert;
                auto index_reg = gep->get_base();
                int n_indices = gep->get_indices().size();
                auto dims = gep->get_base()->get_type().dims;
                for (int i = 0; i < n_indices - 1; i++) {
                    Temp* t = new Temp(reg_id++, Type(gep->get_dst()->get_type().base_type, {dims.begin()+i+2
                    , dims.end()}).get_pointer_type());
                    insts_to_insert.push_back(new ir::instruction::ElementPtr(t, index_reg, {gep->get_indices()[i]}));
                    // (new ir::instruction::ElementPtr(t, index_reg, {gep->get_indices()[i]}))->print(std::cout);
                    index_reg = t;
                }
                insts_to_insert.push_back(new ir::instruction::ElementPtr(gep->get_dst(), index_reg, {gep->get_indices()[n_indices-1]}));
                (*it)->set_parent(nullptr);
                auto pos = (*bb->get_instructions()).erase(it);
                for (auto &i: insts_to_insert) {
                    pos = (*bb->get_instructions()).emplace(pos, i); pos++;
                    // (*pos)->print(std::cout);
                    i->set_parent(bb);
                }
                it = pos;
            }
            else it++;       
        }
        // std::cout << "after: "; bb->print(std::cout);
    }
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto it = (*bb->get_instructions()).begin(); it != (*bb->get_instructions()).end();) {
            TypeCase (gep, ir::instruction::ElementPtr *, *it) {
                if (gep->get_indices().size() != 1) { assert(false); }
                int dim_size = 4;
                for (int i = 1; i < gep->get_base()->get_type().dims.size(); i++) { dim_size *= gep->get_base()->get_type().dims[i]; }
                auto size = new Temp(reg_id++, Type(Int));
                auto li = new ir::instruction::LoadImm4(size, dim_size);
                auto offset = new Temp(reg_id++, Type(Int));
                auto mul = new ir::instruction::Binary(BinaryOp::Mul, offset, gep->get_indices()[0], size);
                auto add = new ir::instruction::Binary(BinaryOp::Add, gep->get_dst(), offset, gep->get_base());
                auto pos_iter = (*bb->get_instructions()).erase(it);
                int pos = std::distance((*bb->get_instructions()).begin(), pos_iter);
                bb->add_instruction_at(pos++, li);
                bb->add_instruction_at(pos++, mul);
                bb->add_instruction_at(pos++, add);
                it = (*bb->get_instructions()).begin() + pos;
            }
            else it++;
        }
    }
    func->set_temp_used(reg_id);
}

}