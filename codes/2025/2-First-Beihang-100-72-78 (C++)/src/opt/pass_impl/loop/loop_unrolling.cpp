#include <memory>
#include <unordered_map>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
constexpr const int MAX_UNROLL_SIZE = 1000;
constexpr const int MAX_TOTAL_SIZE = 15000;
static bool modified = false;

static void try_unroll(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static void try_unroll_countable(std::shared_ptr<ir::opt_support::LoopInfo> loop);

static int total_cnt = 0;

bool LoopUnrolling::run(ir::Module *module) {
    logger::INFO("Running LoopUnrolling...");
    modified = false;
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            try_unroll(loop);
        }
    }
    return modified;
}

static void try_unroll(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (loop->trip_count.has_value())
        try_unroll_countable(loop);
    else
        return;  // TODO(Xingkun):  handle unroll for non-countable loops
}

static void try_unroll_countable(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (total_cnt > MAX_TOTAL_SIZE) return;
    if (loop->trip_count.value() == 0) return;  // TODO(Xingkun): delete useless loop
    if (loop->exits.size() != 1) return;
    if (loop->ind_vars.size() != 1) return;
    if (loop->size().value() > MAX_UNROLL_SIZE) return;

    auto ori_exiting = *loop->exitings.begin();
    if (ori_exiting != loop->header) return;
    auto ori_header_br = std::dynamic_pointer_cast<ir::Br>(ori_exiting->tail_instruction());
    auto ori_first_entry = std::dynamic_pointer_cast<ir::BasicBlock>(
        ori_header_br->get_operands_ref()[1] == *loop->exits.begin() ? ori_header_br->get_operands_ref()[2]
                                                                     : ori_header_br->get_operands_ref()[1]);

    auto ind_var_info = loop->ind_vars.front();
    auto ind_var = ind_var_info.ind_var;
    auto init_int = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.init);
    auto bound_int = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.bound);
    auto step_int = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.step);
    auto alu = std::dynamic_pointer_cast<ir::Instruction>(ind_var_info.alu);
    if (!init_int || !bound_int || !step_int || !alu || !alu->is_binary()) return;

    modified = true;

    int init = init_int->get_val();
    int step = step_int->get_val();

    auto cur_func = loop->header->get_parent_func().lock();
    int cur_ind_var_val = init;
    std::shared_ptr<ir::BasicBlock> prev_latch = loop->pre_header, prev_header = loop->header;
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> value_clone_map;
    auto header_phi_map = [&]() {
        std::unordered_map<std::shared_ptr<ir::Phi>, std::shared_ptr<ir::Value>> res;
        for (const auto &phi : util::get_phis(loop->header)) {
            if (phi == ind_var) continue;
            res[phi] = util::get_phi_value(phi, loop->pre_header);
        }
        return res;
    }();

    total_cnt += loop->size().value();

    for (int i = 0; i < loop->trip_count.value(); i++) {
        // clone loop blocks
        std::list<std::shared_ptr<ir::BasicBlock>> new_blocks;
        value_clone_map.clear();
        for (const auto &block : loop->all_blocks()) {
            auto new_block = std::make_shared<ir::BasicBlock>(cur_func, gen_block_name());
            value_clone_map[block] = new_block;
            // clone instructions
            for (const auto &inst : block->get_instructions_ref()) {
                auto new_inst = inst->clone();
                new_block->add_instruction(new_block->get_instructions_ref().end(), new_inst);
                new_inst->set_parent_block(new_block);
                value_clone_map[inst] = new_inst;
            }
            new_blocks.push_back(new_block);
        }
        // replace inst operands
        for (auto &block : new_blocks) {
            for (auto &inst : block->get_instructions_ref()) {
                util::substitute_operand(inst, value_clone_map);
            }
        }

        // calculate alu value
        int alu_val = alu->get_ins_type() == ir::Instruction::InstructionType::ADD   ? cur_ind_var_val + step
                      : alu->get_ins_type() == ir::Instruction::InstructionType::SUB ? cur_ind_var_val - step
                                                                                     : cur_ind_var_val * step;

        // substitute ind var and alu value
        auto new_ind_var = std::dynamic_pointer_cast<ir::Instruction>(value_clone_map[ind_var]);
        auto new_alu = std::dynamic_pointer_cast<ir::Instruction>(value_clone_map[alu]);
        util::substitute(new_ind_var, std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), cur_ind_var_val));
        util::substitute(new_alu, std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), alu_val));
        value_clone_map[ind_var] = new_ind_var;
        value_clone_map[alu] = new_alu;

        // substitute header phis
        for (auto &[ori_phi, ori_val] : header_phi_map) {
            auto new_phi = std::dynamic_pointer_cast<ir::Phi>(value_clone_map[ori_phi]);
            util::substitute(new_phi, ori_val);
            header_phi_map[ori_phi] =
                util::get_phi_value(new_phi, std::dynamic_pointer_cast<ir::BasicBlock>(value_clone_map[loop->latch()]));
            value_clone_map[ori_phi] = header_phi_map[ori_phi];
        }

        // header jumps to the first loop block with no condition
        auto new_header = std::dynamic_pointer_cast<ir::BasicBlock>(value_clone_map[loop->header]);
        auto new_first_entry = std::dynamic_pointer_cast<ir::BasicBlock>(value_clone_map[ori_first_entry]);
        auto new_header_br = ir::Br::create(new_header, new_first_entry, gen_local_var_name());
        util::substitute(std::dynamic_pointer_cast<ir::Instruction>(new_header->tail_instruction()), new_header_br);
        new_header->add_instruction(new_header->get_instructions_ref().end(), new_header_br);

        // make last latch jumps to the new header
        if (prev_latch) {
            util::redirect(prev_latch, prev_header, new_header);
        }

        // insert blocks
        cur_func->add_basic_blocks(std::move(new_blocks));

        // iter corresponding vars
        cur_ind_var_val = alu_val;
        prev_latch = std::dynamic_pointer_cast<ir::BasicBlock>(value_clone_map[loop->latch()]);
        prev_header = std::dynamic_pointer_cast<ir::BasicBlock>(value_clone_map[loop->header]);
    }

    // finally, redirect the last latch to exit block
    util::redirect(prev_latch, prev_header, *loop->exits.begin());

    // substitute in exit
    for (const auto &phi : util::get_phis(*loop->exits.begin())) {
        if (phi->is_lcssa) util::substitute(phi, value_clone_map[phi->get_operands_ref()[0]]);
    }

    // delete original loop
    for (const auto &block : loop->all_blocks()) {
        util::remove_basic_block(block);
    }
}

}  // namespace opt::pass
