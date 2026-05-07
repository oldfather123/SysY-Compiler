#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

bool dfs_gvn(std::shared_ptr<ir::BasicBlock> root,
             std::unordered_map<std::string, std::shared_ptr<ir::Instruction>> record2inst);
extern std::shared_ptr<ir::Value> simplify_inst(std::shared_ptr<ir::Instruction> inst);
std::string get_gvn_key(std::shared_ptr<ir::Instruction> inst);
bool can_gvn(std::shared_ptr<ir::Instruction> inst);

bool GlobalValueNumbering::run(ir::Module *module) {
    logger::INFO("Running GlobalValueNumbering...");
    bool modified = false;
    module->for_each_func([&modified](auto &func) { modified |= dfs_gvn(func->get_basic_blocks_ref().front(), {}); });
    return false;
}

bool dfs_gvn(std::shared_ptr<ir::BasicBlock> root,
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
        if (can_gvn(inst)) {
            auto key = get_gvn_key(inst);
            if (record2inst.count(key) != 0 && record2inst[key] != inst) {
                it = util::substitute(inst, record2inst[key]);
                modified = true;
                continue;
            }
            record2inst[key] = inst;
        }
        ++it;
    }
    for (auto &basic_block_weak : root->opt_info.immediate_dominated) {
        auto basic_block = basic_block_weak.lock();
        if (basic_block == root) continue;
        modified |= dfs_gvn(basic_block, record2inst);
    }
    return modified;
}

std::string get_gvn_key(std::shared_ptr<ir::Instruction> inst) {
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
            logger::WARN("Unhandled gvn instruction type: ", inst->get_ins_type());
            return inst->to_string();
    }
}

bool can_gvn(std::shared_ptr<ir::Instruction> inst) {
    auto inst_type = inst->get_ins_type();
    switch (inst_type) {
        case ir::Instruction::InstructionType::ALLOCA:
        case ir::Instruction::InstructionType::LOAD:
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

}  // namespace opt::pass
