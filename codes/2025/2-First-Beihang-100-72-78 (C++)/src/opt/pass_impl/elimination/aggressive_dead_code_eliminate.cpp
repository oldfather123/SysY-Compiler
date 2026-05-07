#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static bool modified = false;
static std::unordered_set<std::shared_ptr<ir::Value>> useful_values;
static std::unordered_set<std::shared_ptr<ir::Value>> saved_values;

static void init(std::shared_ptr<ir::Function> main);
static void fill_useful_values();
static void update_value(std::shared_ptr<ir::Value> value);
static void delete_all_useless_values_for_module(ir::Module *module);
static void delete_all_useless_values_for_function(const std::shared_ptr<ir::Function> &function);
static void delete_all_useless_values_for_basic_block(const std::shared_ptr<ir::BasicBlock> &basic_block);

bool AggressiveDeadCodeElimination::run(ir::Module *module) {
    logger::INFO("Running AggressiveDeadCodeElimination...");

    init(module->get_main());
    fill_useful_values();
    delete_all_useless_values_for_module(module);

    return modified;
}

static void init(std::shared_ptr<ir::Function> main) {
    modified = false;
    useful_values.clear();
    saved_values.clear();

    for (auto it = main->get_basic_blocks_ref().rbegin(); it != main->get_basic_blocks_ref().rend(); ++it) {
        auto &basic_block = *it;
        auto terminator = basic_block->get_instructions_ref().back();
        if (terminator->get_ins_type() == ir::Instruction::InstructionType::RET) {
            saved_values.insert(terminator);
        }
    }
}

static void fill_useful_values() {
    bool no_changes = true;
    while (no_changes) {
        useful_values.insert(saved_values.begin(), saved_values.end());
        auto saved_values_copy = saved_values;
        saved_values.clear();
        for (const auto &value : saved_values_copy) {
            // std::cout << "update_value: " << value->to_string() << std::endl;
            update_value(value);
        }
        saved_values = set_difference(saved_values, useful_values);
        no_changes = !saved_values.empty();
    }
}

static void update_value(std::shared_ptr<ir::Value> value) {
    if (value->get_type()->is_pointer_ty()) {
        for (auto &user : value->get_users_ref()) {
            auto inst = std::dynamic_pointer_cast<ir::Instruction>(user.lock());
            if (inst->get_ins_type() == ir::Instruction::InstructionType::STORE ||
                inst->get_ins_type() == ir::Instruction::InstructionType::MEMSET ||
                inst->get_ins_type() == ir::Instruction::InstructionType::CALL ||
                inst->get_ins_type() == ir::Instruction::InstructionType::GETELEMENTPTR) {
                saved_values.insert(inst);
                continue;
            }
            if (!inst->get_users_ref().empty()) {
                saved_values.insert(inst);
            }
        }
    }
    if (auto inst = std::dynamic_pointer_cast<ir::Instruction>(value)) {
        saved_values.insert(inst->get_parent_block().lock());
        saved_values.insert(inst->get_parent_block().lock()->get_parent_func().lock());
        for (const auto &operand : inst->get_operands()) {
            saved_values.insert(operand);
        }
    } else if (auto basic_block = std::dynamic_pointer_cast<ir::BasicBlock>(value)) {
        // save useful call instruction
        for (auto &instr : basic_block->get_instructions_ref()) {
            if (auto call = std::dynamic_pointer_cast<ir::Call>(instr)) {
                auto callee = call->get_function();
                if (ir::Function::is_lib(callee) || callee->opt_info.has_inout ||
                    useful_values.find(callee) != useful_values.end()) {
                    saved_values.insert(instr);
                }
            }
        }

        for (auto &user : basic_block->get_users_ref()) {
            saved_values.insert(user.lock());
        }
    } else if (auto func = std::dynamic_pointer_cast<ir::Function>(value)) {
        for (auto it = func->get_basic_blocks_ref().rbegin(); it != func->get_basic_blocks_ref().rend(); ++it) {
            // ret instruction is always useful
            auto terminator = (*it)->get_instructions_ref().back();
            if (terminator->get_ins_type() == ir::Instruction::InstructionType::RET) {
                saved_values.insert(terminator);
            }

            // array (pointer) arg is always useful
            for (auto &arg : func->get_arguments_ref()) {
                if (arg->get_type()->is_pointer_ty()) {
                    saved_values.insert(arg);
                }
            }

            for (const auto &caller : func->get_users_ref()) {
                auto call = std::dynamic_pointer_cast<ir::Call>(caller.lock());
                auto call_block = call->get_parent_block().lock();
                if (useful_values.find(call_block) != useful_values.end() ||
                    saved_values.find(call_block) != saved_values.end()) {
                    saved_values.insert(call);
                }
            }
        }
    }
}

static void delete_all_useless_values_for_module(ir::Module *module) {
    // remove all useless functions
    std::vector<std::shared_ptr<ir::Function>> useless_functions;
    for (auto &function : module->get_functions()) {
        if (useful_values.find(function) == useful_values.end()) {
            useless_functions.push_back(function);
        } else {
            delete_all_useless_values_for_function(function);
        }
    }
    modified |= !useless_functions.empty();
    for (auto &function : useless_functions) {
        util::remove_function(module, function);
    }

    // remove all useless global variables
    std::vector<std::shared_ptr<ir::GlobalVariable>> useless_global_variables;
    for (const auto &global_variable : module->get_global_variables()) {
        if (useful_values.find(global_variable) == useful_values.end()) {
            useless_global_variables.push_back(global_variable);
        }
    }
    modified |= !useless_global_variables.empty();
    for (const auto &global_variable : useless_global_variables) {
        util::remove_global_variable(module, global_variable);
    }
}

static void delete_all_useless_values_for_function(const std::shared_ptr<ir::Function> &function) {
    // remove all useless basic blocks
    std::vector<std::shared_ptr<ir::BasicBlock>> useless_basic_blocks;
    for (const auto &basic_block : function->get_basic_blocks_ref()) {
        if (useful_values.find(basic_block) == useful_values.end()) {
            useless_basic_blocks.push_back(basic_block);
        } else {
            delete_all_useless_values_for_basic_block(basic_block);
        }
    }
    modified |= !useless_basic_blocks.empty();
    for (const auto &basic_block : useless_basic_blocks) {
        util::remove_basic_block(basic_block);
    }
}

static void delete_all_useless_values_for_basic_block(const std::shared_ptr<ir::BasicBlock> &basic_block) {
    // remove all useless instructions
    std::vector<std::shared_ptr<ir::Instruction>> useless_instructions;
    for (const auto &instruction : basic_block->get_instructions_ref()) {
        if (useful_values.find(instruction) == useful_values.end()) {
            useless_instructions.push_back(instruction);
        }
    }
    modified |= !useless_instructions.empty();
    for (const auto &instruction : useless_instructions) {
        util::remove_instruction(instruction);
    }
}

}  // namespace opt::pass
