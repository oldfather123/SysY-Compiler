#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static ir::opt_support::PointerBaseInfo *ptr_base_info = nullptr;
static bool dfs_lvn(std::shared_ptr<ir::BasicBlock> root,
                    std::unordered_map<std::string, std::shared_ptr<ir::Instruction>> record2inst);
extern std::shared_ptr<ir::Value> simplify_inst(std::shared_ptr<ir::Instruction> inst);
std::string get_lvn_key(std::shared_ptr<ir::Instruction> inst);
bool can_lvn(std::shared_ptr<ir::Instruction> inst);
static bool no_changes_occurred(std::shared_ptr<ir::BasicBlock> block_1,
                                std::shared_ptr<ir::BasicBlock> block_2,
                                std::shared_ptr<ir::Value> addr);
static void visit(std::shared_ptr<ir::BasicBlock> root,
                  std::unordered_set<std::shared_ptr<ir::BasicBlock>> &visited,
                  bool reversed);
static std::unordered_set<std::shared_ptr<ir::BasicBlock>> calc_intersection(
    const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &set1,
    const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &set2);
static bool no_changes_occurred_in_same_block(std::shared_ptr<ir::Load> load_1, std::shared_ptr<ir::Load> load_2);

bool LocalValueNumbering::run(ir::Module *module) {
    logger::INFO("Running LocalValueNumbering");
    bool modified = false;

    // from pointer base analyzation
    ptr_base_info = &module->opt_info.pointer_base_info;

    module->for_each_func([&modified](auto &func) { modified |= dfs_lvn(func->get_basic_blocks_ref().front(), {}); });
    return false;
}

bool dfs_lvn(std::shared_ptr<ir::BasicBlock> root,
             std::unordered_map<std::string, std::shared_ptr<ir::Instruction>> record2inst) {
    bool modified = false;
    std::vector<std::shared_ptr<ir::Instruction>> to_delete;
    auto &instructions = root->get_instructions_ref();
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto inst = *it;
        auto try_fold = simplify_inst(inst);
        if (try_fold != inst) {
            it = util::substitute(inst, try_fold);
            continue;
        }
        if (can_lvn(inst)) {
            auto key = get_lvn_key(inst);
            if (record2inst.count(key) != 0 && record2inst[key] != inst) {
                // new: handle load instruction
                if (auto load_2 = std::dynamic_pointer_cast<ir::Load>(inst)) {
                    auto load_1 = std::dynamic_pointer_cast<ir::Load>(record2inst[key]);
                    if (no_changes_occurred_in_same_block(load_1, load_2)) {
                        it = util::substitute(inst, load_1);
                        modified = true;
                    } else if (no_changes_occurred(record2inst[key]->get_parent_block().lock(), root, load_2->addr())) {
                        it = util::substitute(inst, record2inst[key]);
                        modified = true;
                    } else {
                        record2inst[key] = inst;
                    }
                } else {
                    it = util::substitute(inst, record2inst[key]);
                    modified = true;
                }
                continue;
            }
            record2inst[key] = inst;
        }
        ++it;
    }
    for (auto &basic_block_weak : root->opt_info.immediate_dominated) {
        auto basic_block = basic_block_weak.lock();
        if (basic_block == root) continue;
        modified |= dfs_lvn(basic_block, record2inst);
    }
    return modified;
}

std::string get_lvn_key(std::shared_ptr<ir::Instruction> inst) {
    auto ins_type = inst->get_ins_type();
    switch (ins_type) {
        // TODO: More can be done
        case ir::Instruction::InstructionType::ADD:
        case ir::Instruction::InstructionType::MUL:
        case ir::Instruction::InstructionType::FADD:
        case ir::Instruction::InstructionType::FMUL: {
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            auto operand1 = inst->get_operands_ref()[1]->get_name();
            std::ostringstream oss;
            if (operand0.compare(operand1) > 0) {
                oss << ins_type << ": " << operand0 << ", " << operand1;
            } else {
                oss << ins_type << ": " << operand1 << ", " << operand0;
            }
            return oss.str();
        }
        case ir::Instruction::InstructionType::FREM:
        case ir::Instruction::InstructionType::FSUB:
        case ir::Instruction::InstructionType::FDIV:
        case ir::Instruction::InstructionType::SREM:
        case ir::Instruction::InstructionType::SUB:
        case ir::Instruction::InstructionType::SDIV: {
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            auto operand1 = inst->get_operands_ref()[1]->get_name();
            std::ostringstream oss;
            oss << ins_type << ": " << operand0 << ", " << operand1;
            return oss.str();
        }
        case ir::Instruction::InstructionType::FNEG:
        case ir::Instruction::InstructionType::LOAD: {
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            std::ostringstream oss;
            oss << ins_type << ": " << operand0;
            return oss.str();
        }
        case ir::Instruction::InstructionType::ICMP: {
            auto icmp_type = std::dynamic_pointer_cast<ir::ICmp>(inst)->get_cmp_type();
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            auto operand1 = inst->get_operands_ref()[1]->get_name();
            std::ostringstream oss;
            oss << icmp_type << ": " << operand0 << ", " << operand1;
            return oss.str();
        }
        case ir::Instruction::InstructionType::FCMP: {
            auto fcmp_type = std::dynamic_pointer_cast<ir::FCmp>(inst)->get_cmp_type();
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            auto operand1 = inst->get_operands_ref()[1]->get_name();
            std::ostringstream oss;
            oss << fcmp_type << ": " << operand0 << ", " << operand1;
            return oss.str();
        }
        case ir::Instruction::InstructionType::CALL:
        case ir::Instruction::InstructionType::GETELEMENTPTR: {
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            std::ostringstream oss;
            oss << ins_type;
            oss << ": " << operand0;
            for (size_t i = 1; i < inst->get_operands_ref().size(); ++i) {
                oss << ", " << inst->get_operands_ref()[i]->get_name();
            }
            return oss.str();
        }
        case ir::Instruction::InstructionType::ZEXT: {
            auto operand0 = inst->get_operands_ref()[0]->get_name();
            auto des_type = inst->get_type();
            std::ostringstream oss;
            oss << ins_type << ": " << operand0 << ", " << des_type->to_string();
            return oss.str();
        }
        default:
            logger::WARN("Unhandled lvn instruction type: ", inst->get_ins_type());
            return inst->to_string();
    }
}

bool can_lvn(std::shared_ptr<ir::Instruction> inst) {
    auto inst_type = inst->get_ins_type();
    switch (inst_type) {
        case ir::Instruction::InstructionType::ALLOCA:
        case ir::Instruction::InstructionType::STORE:
        case ir::Instruction::InstructionType::PHI:
        case ir::Instruction::InstructionType::RET:
        case ir::Instruction::InstructionType::BITCAST:
        case ir::Instruction::InstructionType::SITOFP:
        case ir::Instruction::InstructionType::FPTOSI:
        case ir::Instruction::InstructionType::BR:
        case ir::Instruction::InstructionType::SHL:
        case ir::Instruction::InstructionType::AND:
        case ir::Instruction::InstructionType::OR:
        case ir::Instruction::InstructionType::XOR:
        case ir::Instruction::InstructionType::ASHR:
        case ir::Instruction::InstructionType::LSHR:
        case ir::Instruction::InstructionType::TRUNC:
        case ir::Instruction::InstructionType::MEMSET:
            return false;
        case ir::Instruction::InstructionType::CALL: {
            auto func = std::dynamic_pointer_cast<ir::Call>(inst)->get_function();
            auto func_info = func->opt_info;
            return func->get_return_type() != ir::VoidType::get() && !ir::Function::is_lib(func) && func_info.is_pure &&
                   !func_info.has_inout;
        }
        default:
            return true;
    }
}

static bool no_changes_occurred(std::shared_ptr<ir::BasicBlock> block_1,
                                std::shared_ptr<ir::BasicBlock> block_2,
                                std::shared_ptr<ir::Value> addr) {
    auto base = ptr_base_info->query(addr);
    if (base == std::nullopt) {
        return false;
    }

    std::unordered_set<std::shared_ptr<ir::BasicBlock>> visited, visited_reversed;

    visit(block_1, visited, false);
    visit(block_2, visited_reversed, true);

    auto visited_common = calc_intersection(visited, visited_reversed);

    if (visited_common.empty()) {
        return false;
    }

    for (const auto &block : visited_common) {
        for (auto &inst : block->get_instructions_ref()) {
            if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
                auto store_base = ptr_base_info->query(store->addr());
                if (store_base == std::nullopt || store_base == base) {
                    return false;
                }
            } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
                auto func = call->get_function();
                if (func->opt_info.has_local_memory_write || func->opt_info.has_global_memory_write) {
                    return false;
                }
            }
        }
    }

    return true;
}

static bool no_changes_occurred_in_same_block(std::shared_ptr<ir::Load> load_1, std::shared_ptr<ir::Load> load_2) {
    if (load_1->get_parent_block().lock() != load_2->get_parent_block().lock()) {
        return false;
    }
    auto base = ptr_base_info->query(load_1->addr());
    if (base == std::nullopt) {
        return false;
    }
    auto it = load_1->node;
    while (it != load_2->node) {
        if (auto store = std::dynamic_pointer_cast<ir::Store>(*it)) {
            auto store_base = ptr_base_info->query(store->addr());
            if (store_base == std::nullopt || store_base == base) {
                return false;
            }
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(*it)) {
            auto func = call->get_function();
            if (func->opt_info.has_local_memory_write || func->opt_info.has_global_memory_write) {
                return false;
            }
        }
        ++it;
    }
    return true;
}

static void visit(std::shared_ptr<ir::BasicBlock> root,
                  std::unordered_set<std::shared_ptr<ir::BasicBlock>> &visited,
                  bool reversed) {
    if (visited.find(root) != visited.end()) {
        return;
    }
    visited.insert(root);
    std::vector<std::weak_ptr<ir::BasicBlock>> next =
        reversed ? root->opt_info.predecessors : root->opt_info.successors;
    for (const auto &next_weak : next) {
        visit(next_weak.lock(), visited, reversed);
    }
}

static std::unordered_set<std::shared_ptr<ir::BasicBlock>> calc_intersection(
    const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &set1,
    const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &set2) {
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> intersection;
    for (const auto &block : set1) {
        if (set2.find(block) != set2.end()) {
            intersection.insert(block);
        }
    }
    return intersection;
}

}  // namespace opt::pass
