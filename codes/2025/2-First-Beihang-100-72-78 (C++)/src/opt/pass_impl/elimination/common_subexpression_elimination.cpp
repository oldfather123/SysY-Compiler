#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static bool is_same_type(std::shared_ptr<ir::Instruction> inst1, std::shared_ptr<ir::Instruction> inst2) {
    using InstructionType = ir::Instruction::InstructionType;
    if (inst1->get_ins_type() != inst2->get_ins_type()) return false;
    const std::unordered_set<InstructionType> same_type_insts = {
        InstructionType::ADD,     InstructionType::SUB,           InstructionType::MUL,    InstructionType::SDIV,
        InstructionType::SREM,    InstructionType::FADD,          InstructionType::FSUB,   InstructionType::FMUL,
        InstructionType::FDIV,    InstructionType::FREM,          InstructionType::ICMP,   InstructionType::FCMP,
        InstructionType::ALLOCA,  InstructionType::GETELEMENTPTR, InstructionType::LOAD,   InstructionType::STORE,
        InstructionType::BITCAST, InstructionType::CALL,          InstructionType::FPTOSI, InstructionType::SITOFP,
        InstructionType::PHI,     InstructionType::ZEXT,          InstructionType::BR,     InstructionType::RET};
    return same_type_insts.count(inst1->get_ins_type());
}

struct Expression {
    std::unordered_set<std::shared_ptr<ir::Instruction>> source;
    std::shared_ptr<ir::Instruction> instrcution;
    std::vector<std::shared_ptr<ir::Value>> operands;

    explicit Expression(std::shared_ptr<ir::Instruction> instruction) : instrcution(instruction) {
        this->operands = instruction->get_operands_ref();
        source.insert(instruction);
    }

    bool operator==(const Expression &other) const {
        if (!is_same_type(instrcution, other.instrcution)) return false;
        if (operands.size() != other.operands.size()) return false;
        if (std::dynamic_pointer_cast<ir::ICmp>(instrcution) &&
            std::dynamic_pointer_cast<ir::ICmp>(other.instrcution)) {
            auto icmp1 = std::dynamic_pointer_cast<ir::ICmp>(instrcution),
                 icmp2 = std::dynamic_pointer_cast<ir::ICmp>(other.instrcution);
            if (icmp1->get_cmp_type() != icmp2->get_cmp_type()) return false;
        }
        if (std::dynamic_pointer_cast<ir::FCmp>(instrcution) &&
            std::dynamic_pointer_cast<ir::FCmp>(other.instrcution)) {
            auto fcmp1 = std::dynamic_pointer_cast<ir::FCmp>(instrcution),
                 fcmp2 = std::dynamic_pointer_cast<ir::FCmp>(other.instrcution);
            if (fcmp1->get_cmp_type() != fcmp2->get_cmp_type()) return false;
        }
        if (std::dynamic_pointer_cast<ir::Call>(instrcution) &&
            std::dynamic_pointer_cast<ir::Call>(other.instrcution)) {
            auto call1 = std::dynamic_pointer_cast<ir::Call>(instrcution),
                 call2 = std::dynamic_pointer_cast<ir::Call>(other.instrcution);
            if (ir::Function::is_lib(call1->get_function()) || ir::Function::is_lib(call2->get_function()))
                return false;
        }

        for (size_t i = 0; i < operands.size(); i++) {
            auto op1 = operands[i], op2 = other.operands[i];
            if (std::dynamic_pointer_cast<ir::ConstantInt>(op1) && std::dynamic_pointer_cast<ir::ConstantInt>(op2)) {
                auto const_int1 = std::dynamic_pointer_cast<ir::ConstantInt>(op1),
                     const_int2 = std::dynamic_pointer_cast<ir::ConstantInt>(op2);
                if (const_int1->get_val() != const_int2->get_val()) return false;
                continue;
            }
            if (std::dynamic_pointer_cast<ir::ConstantFloat>(op1) &&
                std::dynamic_pointer_cast<ir::ConstantFloat>(op2)) {
                auto const_float1 = std::dynamic_pointer_cast<ir::ConstantFloat>(op1),
                     const_float2 = std::dynamic_pointer_cast<ir::ConstantFloat>(op2);
                if (const_float1->get_val() != const_float2->get_val()) return false;
                continue;
            }
            if (op1 != op2) return false;
        }
        return true;
    }
};

static bool modified = false;

static void local_cse(std::shared_ptr<ir::Function> func);

bool CommonSubexpressionElimination::run(ir::Module *module) {
    logger::INFO("Running CommonSubexpressionElimination...");
    modified = false;
    module->for_each_func([](auto &func) { local_cse(func); });
    return modified;
}

static bool optimizable(std::shared_ptr<ir::Instruction> inst) {
    using InstructionType = ir::Instruction::InstructionType;
    if (inst->is_type(InstructionType::RET) || inst->is_type(InstructionType::BR) ||
        inst->is_type(InstructionType::STORE) || inst->is_type(InstructionType::ALLOCA))
        return false;

    if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
        auto func = std::dynamic_pointer_cast<ir::Function>(call->get_function());
        return !func->get_return_type()->is_void_ty();
    }
    return true;
}

static bool is_store_with_different_index(std::shared_ptr<ir::Instruction> inst1,
                                          std::shared_ptr<ir::Instruction> inst2) {
    auto store = std::dynamic_pointer_cast<ir::Store>(inst2);
    auto load = std::dynamic_pointer_cast<ir::Load>(inst1);
    if (!load || !store) return true;

    auto l_val1 = load->addr(), l_val2 = store->addr();

    if (std::dynamic_pointer_cast<ir::Getelementptr>(l_val1) && std::dynamic_pointer_cast<ir::Getelementptr>(l_val2)) {
        auto gep1 = std::dynamic_pointer_cast<ir::Getelementptr>(l_val1),
             gep2 = std::dynamic_pointer_cast<ir::Getelementptr>(l_val2);
        if (gep1->base_ptr() != gep2->base_ptr()) return true;
        if (gep1->get_indexes().size() != gep2->get_indexes().size()) return true;
        for (size_t i = 0; i < gep1->get_indexes().size(); i++) {
            auto index1 = gep1->get_indexes()[i], index2 = gep2->get_indexes()[i];
            auto const_int1 = std::dynamic_pointer_cast<ir::ConstantInt>(index1),
                 const_int2 = std::dynamic_pointer_cast<ir::ConstantInt>(index2);
            if (const_int1 && const_int2 && const_int1->get_val() != const_int2->get_val()) return true;
        }
        return false;
    }
    return false;
}

std::shared_ptr<ir::Value> find_origin(std::shared_ptr<ir::Value> val) {
    auto res = val;
    while (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(res)) {
        res = gep->base_ptr();
    }
    return res;
}

static bool is_arg_or_global_arr_op(std::shared_ptr<ir::Value> l_val, std::shared_ptr<ir::Value> target) {
    return std::dynamic_pointer_cast<ir::Argument>(target) || (std::dynamic_pointer_cast<ir::GlobalVariable>(target) &&
                                                               !std::dynamic_pointer_cast<ir::GlobalVariable>(l_val));
}

static bool is_kill(std::shared_ptr<ir::Instruction> inst1, std::shared_ptr<ir::Instruction> inst2) {
    auto call1 = std::dynamic_pointer_cast<ir::Call>(inst1), call2 = std::dynamic_pointer_cast<ir::Call>(inst2);
    if (call1 && call2) {
        if (call1->get_function() != call2->get_function()) return false;
        if (ir::Function::is_lib(call1->get_function())) return true;
        return !call1->get_function()->opt_info.is_pure;
    }
    auto load = std::dynamic_pointer_cast<ir::Load>(inst1);
    if (!load) return false;
    if (std::dynamic_pointer_cast<ir::Store>(inst2) && is_store_with_different_index(inst1, inst2)) return false;

    auto l_val_load = load->addr();
    auto target_load = find_origin(l_val_load);

    if (is_arg_or_global_arr_op(l_val_load, target_load)) {
        if (auto store = std::dynamic_pointer_cast<ir::Store>(inst2)) {
            auto l_val_store = store->addr();
            auto target_store = find_origin(l_val_store);

            if (std::dynamic_pointer_cast<ir::GlobalVariable>(target_load) &&
                std::dynamic_pointer_cast<ir::GlobalVariable>(target_store)) {
                return target_load == target_store;
            }
            return is_arg_or_global_arr_op(l_val_store, target_store);
        }

        if (auto call = std::dynamic_pointer_cast<ir::Call>(inst2)) {
            auto func = call->get_function();
            if (ir::Function::is_lib(func)) return true;
            return !func->opt_info.is_pure;
        }
        return false;
    }

    if (auto store = std::dynamic_pointer_cast<ir::Store>(inst2)) return target_load == find_origin(store->addr());

    if (auto call = std::dynamic_pointer_cast<ir::Call>(inst2)) {
        auto func = call->get_function();
        if (ir::Function::is_lib(func)) return true;
        if (func->opt_info.is_pure) return false;
        if (func->opt_info.effected_global_vars.count(l_val_load)) return true;
        for (const auto &v : inst2->get_operands_ref()) {
            if (find_origin(v) == target_load) return true;
        }
    }

    return false;
}

std::shared_ptr<ir::Instruction> is_appear(std::shared_ptr<ir::Instruction> inst,
                                           std::vector<std::shared_ptr<ir::Instruction>> &instructions) {
    int index = static_cast<int>(instructions.size());
    for (int i = index - 1; i >= 0; i--) {
        auto inst2 = instructions[i];
        if (is_kill(inst, inst2)) return nullptr;
        auto expr1 = Expression(inst);
        auto expr2 = Expression(inst2);
        if (expr1 == expr2) return inst2;
    }
    return nullptr;
}

static void local_cse(std::shared_ptr<ir::Function> func) {
    auto blocks = util::get_topo_sort_blocks(func);
    for (const auto &block : blocks) {
        std::unordered_map<std::shared_ptr<ir::Instruction>, std::shared_ptr<ir::Instruction>> delete_worklist;
        do {
            delete_worklist.clear();
            auto instructions = block->get_instructions();
            std::vector<std::shared_ptr<ir::Instruction>> pre_instructions;
            for (const auto &inst : instructions) {
                if (!optimizable(inst)) {
                    pre_instructions.push_back(inst);
                    continue;
                }
                auto pre_inst = is_appear(inst, pre_instructions);
                if (pre_inst)
                    delete_worklist[inst] = pre_inst;
                else
                    pre_instructions.push_back(inst);
            }
            modified |= !delete_worklist.empty();
            for (auto &[ori, cur] : delete_worklist) {
                util::substitute(ori, cur);
            }
        } while (!delete_worklist.empty());
    }
}
}  // namespace opt::pass
