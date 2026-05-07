#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;

// Record the functions that have tail recursion, map it to the basic blocks that have tail recursion.
static std::unordered_map<std::shared_ptr<ir::Function>, std::vector<std::shared_ptr<ir::BasicBlock>>> worklist;

static bool is_tail_recursion_block(std::shared_ptr<ir::BasicBlock> block);
static void collect(ir::Module *module);
static void emit_tail_recursion(std::shared_ptr<ir::Function> function,
                                std::vector<std::shared_ptr<ir::BasicBlock>> &blocks);

bool TailRecursionElimination::run(ir::Module *module) {
    logger::INFO("Running TailRecursionElimination...");
    modified = false;
    collect(module);
    modified = !worklist.empty();

    for (auto &[function, blocks] : worklist) {
        emit_tail_recursion(function, blocks);
    }
    return modified;
}

static void collect(ir::Module *module) {
    module->for_each_func([&](auto &func) {
        for (const auto &block : func->get_basic_blocks_ref()) {
            if (is_tail_recursion_block(block)) worklist[func].push_back(block);
        }
    });
}

static bool is_tail_recursion_block(std::shared_ptr<ir::BasicBlock> block) {
    auto ret = std::dynamic_pointer_cast<ir::Ret>(block->tail_instruction());
    if (!ret) return false;

    auto instructions = block->get_instructions();
    if (instructions.size() <= 1) return false;

    auto tail_inst_iter = instructions.end();
    std::advance(tail_inst_iter, -2);
    auto tail_inst = *tail_inst_iter;
    auto call = std::dynamic_pointer_cast<ir::Call>(tail_inst);
    auto parent_function = block->get_parent_func().lock();
    if (!call || call->get_function() != parent_function) return false;
    if (parent_function->get_return_type()->is_void_ty())
        return ret->get_operands_ref().empty() || ret->get_operands_ref()[0] == call;
    return true;
}

static void emit_tail_recursion(std::shared_ptr<ir::Function> function,
                                std::vector<std::shared_ptr<ir::BasicBlock>> &blocks) {
    auto entry_block = function->entry_block();
    std::shared_ptr<ir::BasicBlock> phi_block;
    auto entry_block_instructions = entry_block->get_instructions();

    // TODO(Xingkun) simplify here, emit redundant code.
    if (entry_block_instructions.size() == 1 && std::dynamic_pointer_cast<ir::Br>(entry_block_instructions.back()) &&
        !std::dynamic_pointer_cast<ir::Br>(entry_block_instructions.back())->is_cond_branch()) {
        phi_block = std::dynamic_pointer_cast<ir::BasicBlock>(
            std::dynamic_pointer_cast<ir::Br>(entry_block_instructions.back())->get_operands_ref()[0]);
    } else {
        phi_block = std::make_shared<ir::BasicBlock>(function, gen_block_name());
        function->add_basic_block(std::next(entry_block->node), phi_block);

        for (auto &inst : entry_block_instructions) {
            auto cloned_inst = inst->clone();
            cloned_inst->set_parent_block(phi_block);
            phi_block->add_instructions({cloned_inst});
            util::substitute(inst, cloned_inst);
        }

        entry_block->opt_info.predecessors.clear();
        entry_block->opt_info.successors.clear();

        util::replace_all_uses_with(entry_block, phi_block);

        // br: entry_block -> phi_block
        entry_block->add_instructions({ir::Br::create(entry_block, phi_block, gen_local_var_name())});
        entry_block->opt_info.successors.push_back(phi_block);
        phi_block->opt_info.predecessors.push_back(entry_block);
    }

    std::unordered_map<std::shared_ptr<ir::Argument>, std::shared_ptr<ir::Phi>> phi_map;
    phi_block->for_each_instruction([&](auto &inst) {
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
            auto &operands = phi->get_operands_ref();
            if (contains(operands, entry_block)) {
                auto index = std::distance(operands.begin(), std::find(operands.begin(), operands.end(), entry_block));
                auto phi_value = operands[index - 1];
                if (auto arg = std::dynamic_pointer_cast<ir::Argument>(phi_value)) {
                    phi_map[arg] = phi;
                }
            }
        }
    });

    function->for_each_argument([&](auto &arg) {
        if (phi_map.find(arg) == phi_map.end()) {
            auto phi = ir::Phi::create(phi_block, {}, {}, arg->get_type(), gen_local_var_name());
            phi_block->add_instruction(phi_block->get_instructions_ref().front()->node, phi);
            util::replace_all_uses_with(arg, phi);
            for (auto &pre_block : phi_block->opt_info.predecessors) {
                if (pre_block.lock() == entry_block)
                    util::add_incoming(phi, entry_block, arg);
                else
                    util::add_incoming(phi, entry_block, phi);
            }
            phi_map[arg] = phi;
        }
    });

    // execute the tail recursion
    for (auto &block : blocks) {
        auto instructions = block->get_instructions();
        if (instructions.size() == 1) continue;
        auto tail = block->tail_instruction();
        auto it = instructions.end();
        std::advance(it, -2);
        auto call = std::dynamic_pointer_cast<ir::Call>(*it);

        auto br = ir::Br::create(block, phi_block, gen_local_var_name());
        block->add_instructions({br});
        util::substitute(tail, br);
        for (size_t i = 0; i < function->get_arguments_ref().size(); i++) {
            auto func_arg = function->get_arguments_ref()[i];
            auto call_arg = call->args()[i];
            auto phi = phi_map[func_arg];
            util::add_incoming(phi, block, call_arg);
        }
        util::remove_instruction(call);
    }
}

}  // namespace opt::pass
