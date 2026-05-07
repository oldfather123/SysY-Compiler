#include <deque>

#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
std::vector<std::shared_ptr<ir::Alloca>> allocas;
std::unordered_map<std::shared_ptr<ir::Alloca>, std::vector<std::shared_ptr<ir::BasicBlock>>> def_basic_blocks;
std::unordered_map<std::shared_ptr<ir::Phi>, std::shared_ptr<ir::Alloca>> phi_to_alloca;
void init(std::shared_ptr<ir::Alloca> alloca);
void insert_phi(std::shared_ptr<ir::Alloca> alloca);
void rename(const std::shared_ptr<ir::BasicBlock> &root,
            std::unordered_map<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::Value>> saved_values,
            std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> visited);
void substitute_for_phi(std::shared_ptr<ir::Phi> phi,
                        std::shared_ptr<ir::BasicBlock> basic_block,
                        std::shared_ptr<ir::Value> new_value);
void cleanup();
std::string gen_phi_name();

bool Mem2Reg::run(ir::Module *module) {
    logger::INFO("Running Mem2Reg...");
    module->for_each_func([&](const auto &func) {
        func->for_each_block([&](const auto &basic_block) {
            basic_block->for_each_instruction([&](const auto &ins) {
                if (ins->get_ins_type() == ir::Instruction::InstructionType::ALLOCA) {
                    auto alloca = std::dynamic_pointer_cast<ir::Alloca>(ins);
                    if (alloca->get_content_type() == ir::IntegerType::get(32) ||
                        alloca->get_content_type() == ir::FloatType::get()) {
                        init(alloca);
                        insert_phi(alloca);
                    }
                }
            });
        });
        // rename only once for each function
        std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> visited;
        for (const auto &basic_block : func->get_basic_blocks_ref()) {
            visited[basic_block] = false;
        }

        // init for each alloca
        std::unordered_map<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::Value>> saved_values;
        for (const auto &alloca : allocas) {
            saved_values[alloca] = ir::get_zero(alloca->get_content_type());
        }

        rename(func->get_basic_blocks().front(), saved_values, visited);

        // release all shared_ptr
        cleanup();
    });

    return false;
}

void init(std::shared_ptr<ir::Alloca> alloca) {
    allocas.push_back(alloca);
    for (const auto &user : alloca->get_users_ref()) {
        if (auto store = std::dynamic_pointer_cast<ir::Store>(user.lock())) {
            if (!contains(def_basic_blocks[alloca], store->get_parent_block().lock())) {
                def_basic_blocks[alloca].push_back(store->get_parent_block().lock());
            }
        }
    }
}

void insert_phi(std::shared_ptr<ir::Alloca> alloca) {
    // basic block with phi inserted
    std::vector<std::shared_ptr<ir::BasicBlock>> inserted_basic_blocks;

    // unhandled basic blocks
    auto unhandled_basic_blocks = std::deque(def_basic_blocks[alloca].begin(), def_basic_blocks[alloca].end());

    while (!unhandled_basic_blocks.empty()) {
        auto basic_block = unhandled_basic_blocks.front();
        unhandled_basic_blocks.pop_front();
        for (const auto &weak_dominance_frontier_basic_block : basic_block->opt_info.dominance_frontier) {
            if (auto dominance_frontier_basic_block = weak_dominance_frontier_basic_block.lock()) {
                // if not handled yet
                if (!contains(inserted_basic_blocks, dominance_frontier_basic_block)) {
                    // init values
                    const auto n = dominance_frontier_basic_block->opt_info.predecessors.size();
                    auto init_values =
                        std::vector<std::shared_ptr<ir::Value>>(n, ir::get_zero(alloca->get_content_type()));

                    // phi
                    auto phi = ir::Phi::create(
                        dominance_frontier_basic_block, init_values, alloca->get_content_type(), gen_phi_name());

                    // add phi to the head of the dominance frontier basic block
                    dominance_frontier_basic_block->add_instruction(
                        dominance_frontier_basic_block->get_instructions_ref().begin(), phi);

                    // map phi to alloca
                    phi_to_alloca[phi] = alloca;

                    // finish inserting phi
                    inserted_basic_blocks.push_back(dominance_frontier_basic_block);

                    // this dominance frontier basic block may still need to be handled
                    if (!contains(def_basic_blocks[alloca], dominance_frontier_basic_block)) {
                        unhandled_basic_blocks.push_back(dominance_frontier_basic_block);
                    }
                }
            }
        }
    }
}

void substitute_for_phi(std::shared_ptr<ir::Phi> phi,
                        std::shared_ptr<ir::BasicBlock> basic_block,
                        std::shared_ptr<ir::Value> new_value) {
    auto &operands = phi->get_operands_ref();
    auto it = std::find(operands.begin(), operands.end(), basic_block);
    if (it != operands.end()) {
        // [value, basic_block] pair is found
        auto index = std::distance(operands.begin(), it);
        util::substitute_operand(phi, index - 1, new_value);
    } else {
        __builtin_unreachable();
    }
}

void rename(const std::shared_ptr<ir::BasicBlock> &root,
            std::unordered_map<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::Value>> saved_values,
            std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> visited) {
    visited[root] = true;
    std::vector<std::shared_ptr<ir::Instruction>> instruction_to_remove;
    for (const auto &instruction : root->get_instructions_ref()) {
        // load: update use and remove
        if (instruction->get_ins_type() == ir::Instruction::InstructionType::LOAD) {
            // continue if load doesn't use an alloca
            auto operand = std::dynamic_pointer_cast<ir::Instruction>(instruction->get_operands_ref()[0]);
            if (!operand || operand->get_ins_type() != ir::Instruction::InstructionType::ALLOCA) {
                continue;
            }

            // continue if alloca is not allocating i32 or float
            auto alloca = std::dynamic_pointer_cast<ir::Alloca>(operand);
            if (alloca->get_content_type() != ir::IntegerType::get(32) &&
                alloca->get_content_type() != ir::FloatType::get()) {
                continue;
            }

            // replace alloca operand with new value
            util::replace_all_uses_with(instruction, saved_values[alloca]);
            instruction_to_remove.push_back(instruction);
        }
        // store: save and remove
        else if (instruction->get_ins_type() == ir::Instruction::InstructionType::STORE) {
            // continue if store doesn't use an alloca
            auto operand = std::dynamic_pointer_cast<ir::Instruction>(instruction->get_operands_ref()[1]);
            if (!operand || operand->get_ins_type() != ir::Instruction::InstructionType::ALLOCA) {
                continue;
            }

            // continue if alloca is not allocating i32 or float
            auto alloca = std::dynamic_pointer_cast<ir::Alloca>(operand);
            if (alloca->get_content_type() != ir::IntegerType::get(32) &&
                alloca->get_content_type() != ir::FloatType::get()) {
                continue;
            }

            // save value
            saved_values[alloca] = instruction->get_operands_ref()[0];

            // remove
            instruction_to_remove.push_back(instruction);
        }
        // phi: save
        else if (instruction->get_ins_type() == ir::Instruction::InstructionType::PHI) {
            auto &alloca = phi_to_alloca[std::dynamic_pointer_cast<ir::Phi>(instruction)];
            if (alloca) {
                saved_values[alloca] = instruction;
            }
        }
        // alloca: remove
        else if (instruction->get_ins_type() == ir::Instruction::InstructionType::ALLOCA) {
            if (def_basic_blocks.find(std::dynamic_pointer_cast<ir::Alloca>(instruction)) != def_basic_blocks.end()) {
                instruction_to_remove.push_back(instruction);
            }
        }
    }

    // update phi
    for (const auto &successor : root->opt_info.successors) {
        for (const auto &instruction : successor.lock()->get_instructions_ref()) {
            if (!(instruction->get_ins_type() == ir::Instruction::InstructionType::PHI)) {
                continue;
            }
            auto phi = std::dynamic_pointer_cast<ir::Phi>(instruction);
            auto &alloca = phi_to_alloca[std::dynamic_pointer_cast<ir::Phi>(instruction)];
            if (alloca) {
                substitute_for_phi(phi, root, saved_values[alloca]);
            }
        }
    }

    // remove
    for (const auto &instruction : instruction_to_remove) {
        util::remove_instruction(instruction);
    }

    // dfs
    for (const auto &weak_successor : root->opt_info.immediate_dominated) {
        if (auto successor = weak_successor.lock()) {
            if (!visited[successor]) {
                rename(successor, saved_values, visited);
            }
        }
    }
}

void cleanup() {
    allocas.clear();
    def_basic_blocks.clear();
    phi_to_alloca.clear();
}

static int phi_cnt = 0;

std::string gen_phi_name() { return "%phi_" + std::to_string(phi_cnt++); }

}  // namespace opt::pass
