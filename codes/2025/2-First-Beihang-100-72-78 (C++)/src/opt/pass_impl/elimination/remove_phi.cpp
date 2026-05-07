#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

void remove_phi(std::shared_ptr<ir::Function> function);
void insert_parallel_copy(std::shared_ptr<ir::BasicBlock> &predecessor, std::shared_ptr<ir::BasicBlock> successor);
void remove_parallel_copy(std::shared_ptr<ir::Function> function);

bool RemovePhi::run(ir::Module *module) {
    module->for_each_func([](const auto &func) {
        remove_phi(func);
        remove_parallel_copy(func);
    });
    return false;
}

void remove_phi(std::shared_ptr<ir::Function> function) {
    for (auto &basic_block : function->get_basic_blocks()) {
        if (basic_block->get_instructions_ref().front()->get_ins_type() != ir::Instruction::InstructionType::PHI) {
            continue;
        }
        // insert parallel copies for all predecessors
        auto predecessors_copy = basic_block->opt_info.predecessors;
        for (auto &predecessor_weak : predecessors_copy) {
            auto predecessor = predecessor_weak.lock();
            insert_parallel_copy(predecessor, basic_block);
        }

        // formally eliminate all phi instructions, they still may appear in operands of other instructions
        for (auto &instruction : basic_block->get_instructions()) {
            if (auto phi = std::dynamic_pointer_cast<ir::Phi>(instruction)) {
                for (auto &predecessor_weak : basic_block->opt_info.predecessors) {
                    auto predecessor = predecessor_weak.lock();
                    auto value = util::get_phi_value_safe(phi, predecessor);
                    // phi may still possess the old basic block instead of the new one added in `insert_parallel_copy`
                    value = value ? value : util::get_phi_value_safe(phi, predecessor->opt_info.predecessors[0].lock());
                    auto phi_copy =
                        std::dynamic_pointer_cast<ir::PhiCopy>(*std::prev(predecessor->tail_instruction()->node));
                    phi_copy->add(phi, value);
                }
                util::remove_all_operands(phi);
                util::remove_instruction_from_parent_basic_block(phi);
            } else {
                break;
            }
        }
    }
}

void insert_parallel_copy(std::shared_ptr<ir::BasicBlock> &predecessor, std::shared_ptr<ir::BasicBlock> successor) {
    auto phi_copy = ir::PhiCopy::create(nullptr, gen_local_var_name());

    // no critical edges
    if (predecessor->opt_info.successors.size() == 1) {
        util::add_instruction_before(phi_copy, predecessor->tail_instruction());
        return;
    }

    // new basic block
    auto new_basic_block = std::make_shared<ir::BasicBlock>(successor->get_parent_func().lock(), gen_block_name());
    phi_copy->set_parent_block(new_basic_block);
    new_basic_block->add_instructions({phi_copy, ir::Br::create(new_basic_block, successor, gen_local_var_name())});
    new_basic_block->get_parent_func().lock()->add_basic_blocks({new_basic_block});

    // update br
    auto br = std::dynamic_pointer_cast<ir::Br>(predecessor->tail_instruction());
    util::substitute_operand(br, successor, new_basic_block);

    // update cfg
    predecessor->opt_info.successors.push_back(new_basic_block);
    predecessor->opt_info.remove_successor(successor);
    new_basic_block->opt_info.predecessors.push_back(predecessor);
    new_basic_block->opt_info.successors.push_back(successor);
    successor->opt_info.predecessors.push_back(new_basic_block);
    successor->opt_info.remove_predecessor(predecessor);
}

void remove_parallel_copy(std::shared_ptr<ir::Function> function) {
    for (auto &basic_block : function->get_basic_blocks_ref()) {
        auto last = basic_block->tail_instruction();
        std::vector<std::shared_ptr<ir::Move>> moves;
        std::vector<std::shared_ptr<ir::PhiCopy>> phi_copies;
        std::shared_ptr<ir::PhiCopy> phi_copy;
        while (last->node != basic_block->get_instructions_ref().begin() &&
               ((phi_copy = std::dynamic_pointer_cast<ir::PhiCopy>(*std::prev(last->node))))) {
            phi_copies.push_back(phi_copy);
            while (!phi_copy->get_phis().empty()) {
                auto phis = phi_copy->get_phis();
                auto values = phi_copy->get_values();
                for (int i = 0; i < values.size(); i++) {
                    const auto &phi = phis[i];
                    if (contains(values, phi)) {
                        if (phi == values[i]) {
                            phi_copy->remove(phi, values[i]);
                        }
                        continue;
                    }
                    auto move = ir::Move::create(basic_block, values[i], phi, "");
                    moves.push_back(move);
                    phi_copy->remove(phi, values[i]);
                }
                if (!phi_copy->get_phis().empty()) {
                    auto value = phi_copy->get_values().front();
                    auto place_holder =
                        std::make_shared<ir::Placeholder>(value->get_type(), "place_holder_" + gen_local_var_name());
                    auto move = ir::Move::create(basic_block, value, place_holder, "");
                    moves.push_back(move);
                    phi_copy->change_value(0, place_holder);
                }
            }
            last = std::dynamic_pointer_cast<ir::Instruction>(*std::prev(last->node));
        }
        for (const auto &p_c : phi_copies) {
            util::remove_instruction(p_c);
        }
        last = basic_block->tail_instruction();
        for (const auto &move : moves) {
            util::add_instruction_before(move, last);
        }
    }
}

}  // namespace opt::pass
