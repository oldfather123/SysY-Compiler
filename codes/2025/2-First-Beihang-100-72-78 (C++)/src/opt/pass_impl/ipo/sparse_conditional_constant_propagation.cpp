#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"
namespace opt::pass {
struct ValueStatus {
    enum Status { BOT, CONS, TOP };
    Status status;
    std::shared_ptr<ir::Constant> value;

    bool operator!=(ValueStatus &other) const {
        if (status != other.status) return true;
        if (status != CONS) return false;
        if (std::dynamic_pointer_cast<ir::ConstantInt>(value))
            return std::dynamic_pointer_cast<ir::ConstantInt>(value) !=
                   std::dynamic_pointer_cast<ir::ConstantInt>(other.value);
        if (std::dynamic_pointer_cast<ir::ConstantFloat>(value))
            return std::dynamic_pointer_cast<ir::ConstantFloat>(value) !=
                   std::dynamic_pointer_cast<ir::ConstantFloat>(other.value);

        return false;
    }

    void operator^=(ValueStatus &other) {
        if ((status == TOP && other.status != TOP) || (status == CONS && other.status == BOT)) {
            status = other.status;
            value = other.value;
        } else if (status == other.status && status == CONS) {
            if (std::dynamic_pointer_cast<ir::ConstantInt>(value)) {
                if (std::dynamic_pointer_cast<ir::ConstantInt>(value) !=
                    std::dynamic_pointer_cast<ir::ConstantInt>(other.value)) {
                    status = BOT;
                    value = nullptr;
                }
            } else if (std::dynamic_pointer_cast<ir::ConstantFloat>(value)) {
                // There may be precision errors
                if (std::dynamic_pointer_cast<ir::ConstantFloat>(value) !=
                    std::dynamic_pointer_cast<ir::ConstantFloat>(other.value)) {
                    status = BOT;
                    value = nullptr;
                }
            }
        }
    }
};

static bool modified = false;

static std::unordered_map<std::shared_ptr<ir::Value>, ValueStatus> value_map;
static std::set<std::pair<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>>> marked;
static std::vector<std::pair<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>>> cfg_worklist;
static std::vector<std::shared_ptr<ir::Instruction>> ssa_worklist;

static ValueStatus pre_status;
static ValueStatus cur_status;
static std::shared_ptr<ir::BasicBlock> cur_block;
static std::shared_ptr<ir::Instruction> cur_inst;
static std::unordered_set<std::shared_ptr<ir::Instruction>> deleted_insts;

static void travel_function(std::shared_ptr<ir::Function> func);
static void process_inst(std::shared_ptr<ir::Instruction> inst);
static void substitute_constant(std::shared_ptr<ir::Function> func);

extern std::optional<std::shared_ptr<ir::Constant>> try_fold_binary_constant(std::shared_ptr<ir::Instruction> inst,
                                                                             std::shared_ptr<ir::Value> left,
                                                                             std::shared_ptr<ir::Value> right);
static std::optional<std::shared_ptr<ir::Constant>> const_fold(std::shared_ptr<ir::Instruction> inst);
static std::optional<std::shared_ptr<ir::Constant>> fold_conversion(std::shared_ptr<ir::Instruction> inst,
                                                                    std::shared_ptr<ir::Constant> v);
void cond_br2jump(std::shared_ptr<ir::Br> br,
                  std::shared_ptr<ir::BasicBlock> jump_block,
                  std::shared_ptr<ir::BasicBlock> invalid_block);

bool SparseConditionalConstantPropagation::run(ir::Module *module) {
    logger::INFO("Running SparseConditionalConstantPropagation...");
    modified = false;
    module->for_each_func([](auto &func) { travel_function(func); });

    return modified;
}

static void travel_function(std::shared_ptr<ir::Function> func) {
    value_map.clear();
    marked.clear();
    cfg_worklist.clear();
    ssa_worklist.clear();
    deleted_insts.clear();
    cfg_worklist.push_back({nullptr, func->entry_block()});

    func->for_each_block(
        [&](auto &block) { block->for_each_instruction([&](auto &inst) { value_map[inst] = {ValueStatus::TOP}; }); });

    size_t i = 0, j = 0;
    while (i < cfg_worklist.size() || j < ssa_worklist.size()) {
        while (i < cfg_worklist.size()) {
            auto [pre_block, block] = cfg_worklist[i++];

            if (marked.count({pre_block, block})) continue;
            marked.insert({pre_block, block});

            for (const auto &inst : block->get_instructions()) {
                process_inst(inst);
            }
        }
        while (j < ssa_worklist.size()) {
            auto inst = ssa_worklist[j++];
            auto block = inst->get_parent_block().lock();
            if (block->opt_info.predecessors.empty()) {
                process_inst(inst);
                continue;
            }
            for (auto &pre_block : block->opt_info.predecessors) {
                if (marked.count({pre_block.lock(), block})) {
                    process_inst(inst);
                }
            }
        }
    }
    substitute_constant(func);
}

static void process_phi(std::shared_ptr<ir::Phi> phi) {
    for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
        auto pre_block = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i + 1]);
        if (marked.count({pre_block, cur_block})) {
            auto op = phi->get_operands_ref()[i];
            auto op_status = value_map[op];
            cur_status ^= op_status;
        }
    }
}

static void process_br(std::shared_ptr<ir::Br> br) {
    if (!br->is_cond_branch()) {
        auto jump_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[0]);
        cfg_worklist.push_back({cur_block, jump_block});
        return;
    }

    auto cond = br->get_operands_ref()[0];
    auto true_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
    auto false_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);

    if (value_map.find(cond) != value_map.end()) {
        cond = value_map[cond].value;  // Retrieve cond from value_map
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(cond)) {
            cfg_worklist.push_back({cur_block, const_int->get_val() ? true_block : false_block});
        } else if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(cond)) {
            cfg_worklist.push_back({cur_block, const_float->get_val() != 0.0F ? true_block : false_block});
        } else {
            cfg_worklist.push_back({cur_block, true_block});
            cfg_worklist.push_back({cur_block, false_block});
        }
    } else {
        cfg_worklist.push_back({cur_block, true_block});
        cfg_worklist.push_back({cur_block, false_block});
    }
}

static void process_foldable(std::shared_ptr<ir::Instruction> inst) {
    auto folded = const_fold(inst);
    if (folded) {
        cur_status.status = ValueStatus::CONS;
        cur_status.value = folded.value();
    } else {
        for (auto &operand : inst->get_operands_ref()) {
            if (value_map.find(operand) != value_map.end() && value_map[operand].status == ValueStatus::BOT) {
                cur_status.status = ValueStatus::BOT;
                break;
            }
        }
    }
}

static void process_inst(std::shared_ptr<ir::Instruction> inst) {
    if (deleted_insts.count(inst)) return;
    cur_inst = inst;
    cur_block = inst->get_parent_block().lock();
    pre_status = value_map[inst];
    cur_status = pre_status;

    if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst))
        process_phi(phi);
    else if (auto br = std::dynamic_pointer_cast<ir::Br>(inst))
        process_br(br);
    else if (inst->is_binary() || inst->is_conversion())
        process_foldable(inst);
    else
        cur_status = {ValueStatus::BOT, nullptr};

    if (cur_status != pre_status) {
        value_map[inst] = cur_status;
        for (auto &user : inst->get_users_ref())
            if (auto instr = std::dynamic_pointer_cast<ir::Instruction>(user.lock())) ssa_worklist.push_back(instr);
        if (cur_status.status == ValueStatus::CONS) {
            util::substitute(inst, cur_status.value);
            modified = true;
            deleted_insts.insert(inst);
        }
    }
}

static std::optional<std::shared_ptr<ir::Constant>> const_fold(std::shared_ptr<ir::Instruction> inst) {
    if (inst->is_binary()) {
        auto value1 = inst->get_operands_ref()[0];
        auto value2 = inst->get_operands_ref()[1];
        if (value_map.find(value1) != value_map.end() && value_map.find(value2) != value_map.end()) {
            auto v1 = std::dynamic_pointer_cast<ir::Constant>(value_map[value1].value);
            auto v2 = std::dynamic_pointer_cast<ir::Constant>(value_map[value2].value);
            if (v1 && v2) {
                return try_fold_binary_constant(inst, v1, v2);
            }
        } else {
            auto v1 = std::dynamic_pointer_cast<ir::Constant>(value1);
            auto v2 = std::dynamic_pointer_cast<ir::Constant>(value2);
            if (v1 && v2) {
                return try_fold_binary_constant(inst, v1, v2);
            }
        }
    } else if (inst->is_conversion()) {
        auto value = inst->get_operands_ref()[0];
        if (value_map.find(value) != value_map.end()) {
            if (auto v = std::dynamic_pointer_cast<ir::Constant>(value_map[value].value)) {
                return fold_conversion(inst, v);
            }
        } else {
            if (auto v = std::dynamic_pointer_cast<ir::Constant>(value)) {
                return fold_conversion(inst, v);
            }
        }
    }
    return std::nullopt;
}

static std::optional<std::shared_ptr<ir::Constant>> fold_conversion(std::shared_ptr<ir::Instruction> inst,
                                                                    std::shared_ptr<ir::Constant> v) {
    if (inst->get_ins_type() == ir::Instruction::InstructionType::ZEXT) {
        return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32),
                                                 std::dynamic_pointer_cast<ir::ConstantInt>(v)->get_val());
    }
    if (inst->get_ins_type() == ir::Instruction::InstructionType::FPTOSI) {
        return std::make_shared<ir::ConstantInt>(
            ir::IntegerType::get(32), static_cast<int>(std::dynamic_pointer_cast<ir::ConstantFloat>(v)->get_val()));
    }
    if (inst->get_ins_type() == ir::Instruction::InstructionType::SITOFP) {
        return std::make_shared<ir::ConstantFloat>(
            ir::FloatType::get(), static_cast<float>(std::dynamic_pointer_cast<ir::ConstantInt>(v)->get_val()));
    }
    return std::nullopt;
}

void substitute_constant(std::shared_ptr<ir::Function> func) {
    auto blocks = func->get_basic_blocks();
    for (const auto &block : blocks) {
        auto tail_inst = block->tail_instruction();
        if (auto br = std::dynamic_pointer_cast<ir::Br>(tail_inst); br && br->is_cond_branch()) {
            auto cond = br->get_operands_ref()[0];
            if (std::dynamic_pointer_cast<ir::Constant>(cond)) {
                modified = true;
                auto true_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
                auto false_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);
                if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(cond)) {
                    if (const_int->get_val() != 0) {
                        cond_br2jump(br, true_block, false_block);
                    } else {
                        cond_br2jump(br, false_block, true_block);
                    }
                } else if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(cond)) {
                    if (const_float->get_val() != 0.0F) {
                        cond_br2jump(br, true_block, false_block);
                    } else {
                        cond_br2jump(br, false_block, true_block);
                    }
                } else {
                    __builtin_unreachable();
                }
            }
        }
    }
}

void delete_phi_value(std::shared_ptr<ir::BasicBlock> cur_block, std::shared_ptr<ir::BasicBlock> next_block) {
    auto instructions = next_block->get_instructions();
    for (const auto &inst : instructions) {
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
            if (contains(phi->get_operands_ref(), cur_block)) {
                util::remove_phi_block(phi, cur_block);
            }
        }
    }
}

void cond_br2jump(std::shared_ptr<ir::Br> br,
                  std::shared_ptr<ir::BasicBlock> jump_block,
                  std::shared_ptr<ir::BasicBlock> invalid_block) {
    auto block = br->get_parent_block().lock();
    util::remove_all_operands(br);
    br->add_operand(jump_block);
    jump_block->add_user(br);
    value_map[br] = cur_status;
    block->opt_info.successors.push_back(jump_block);
    jump_block->opt_info.predecessors.push_back(block);

    if (jump_block != invalid_block) {
        block->opt_info.remove_successor(invalid_block);
        invalid_block->opt_info.remove_predecessor(block);
        delete_phi_value(block, invalid_block);
    }
}

}  // namespace opt::pass
