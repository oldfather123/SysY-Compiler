#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
using SCEVExpr = ir::opt_support::SCEVExpr;
using SCEVType = SCEVExpr::SCEVType;
using SCEVInfo = ir::opt_support::SCEVInfo;

static SCEVInfo *scev_info;
// static std::unordered_set<std::shared_ptr<ir::Instruction>> visited;

static std::unordered_map<std::shared_ptr<ir::Phi>, int> iter_cnt;
constexpr int MAX_ITER_CNT = 7;

static std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> find_in_loop_header(
    std::shared_ptr<ir::Function> func, std::shared_ptr<ir::BasicBlock> block);
static bool basic_induce_var_analysis(std::shared_ptr<ir::Phi> phi,
                                      std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> loop);
static void general_induce_var_analysis(std::shared_ptr<ir::Instruction> inst);

bool ScalarEvolutionAnalyzation::run(ir::Module *module) {
    logger::INFO("Running ScalarEvolutionAnalyzation...");
    scev_info = &module->opt_info.scev_info;
    // std::cout << module->to_string() << std::endl;
    // visited.clear();
    iter_cnt.clear();
    for (const auto &func : module->get_functions_ref()) {
        for (const auto &block : func->get_basic_blocks_ref()) {
            auto loop = find_in_loop_header(func, block);
            bool modified = false;
            do {
                modified = false;
                for (const auto &inst : block->get_instructions_ref()) {
                    if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst);
                        phi && phi->get_operands_ref().size() == 4) {
                        modified |= basic_induce_var_analysis(phi, loop);
                    }
                }
            } while (modified);
        }

        for (const auto &block : func->get_basic_blocks_ref()) {
            for (const auto &inst : block->get_instructions_ref()) {
                general_induce_var_analysis(inst);
            }
        }
    }
    // std::cout << *scev_info << std::endl;
    return false;
}

static std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> find_in_loop_header(
    std::shared_ptr<ir::Function> func, std::shared_ptr<ir::BasicBlock> block) {
    for (const auto &loop : func->opt_info.loop_forest) {
        if (loop->header == block) return loop;
    }
    return std::nullopt;
}

static std::shared_ptr<ir::Value> get_initial(std::shared_ptr<ir::Phi> phi,
                                              std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> loop) {
    if (!loop.has_value()) return phi->get_operands_ref()[0];
    return util::get_phi_value(phi, loop.value()->pre_header);
}

static std::shared_ptr<ir::Value> get_next(std::shared_ptr<ir::Phi> phi,
                                           std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> loop) {
    if (!loop.has_value()) return phi->get_operands_ref()[1];
    assert(loop.value()->latches.size() == 1);
    return util::get_phi_value(phi, *loop.value()->latches.begin());
}

static bool is_same_loop(std::shared_ptr<SCEVExpr> &lhs, std::shared_ptr<SCEVExpr> &rhs) {
    return lhs->loop.lock() == rhs->loop.lock() && lhs->loop.lock() != nullptr;
}

static std::optional<std::shared_ptr<SCEVExpr>> fold_add(std::shared_ptr<SCEVExpr> &lhs,
                                                         std::shared_ptr<SCEVExpr> &rhs) {
    if (lhs->type == SCEVType::CONSTANT && rhs->type == SCEVType::CONSTANT)
        return std::make_shared<SCEVExpr>(SCEVType::CONSTANT, lhs->constant().value() + rhs->constant().value());
    if (lhs->type == SCEVType::ADD_REC && rhs->type == SCEVType::CONSTANT) {
        auto base = lhs->operands().value()[0];
        auto new_base = fold_add(base, rhs);
        if (new_base.has_value()) {
            auto expr = lhs->clone();
            expr->operands_ref().value().get()[0] = new_base.value();
            return expr;
        }
    }
    if (lhs->type == SCEVType::ADD_REC && rhs->type == SCEVType::ADD_REC && is_same_loop(lhs, rhs)) {
        std::vector<std::shared_ptr<SCEVExpr>> operands;
        auto size = std::max(lhs->operands().value().size(), rhs->operands().value().size());
        operands.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            auto lsh_operand = i < lhs->operands().value().size() ? lhs->operands().value()[i] : nullptr;
            auto rsh_operand = i < rhs->operands().value().size() ? rhs->operands().value()[i] : nullptr;
            if (lsh_operand && rsh_operand) {
                auto new_operand = fold_add(lsh_operand, rsh_operand);
                if (new_operand.has_value())
                    operands.push_back(new_operand.value());
                else
                    return std::nullopt;
            } else if (lsh_operand)
                operands.push_back(lsh_operand);
            else
                operands.push_back(rsh_operand);
        }
        return std::make_shared<SCEVExpr>(SCEVType::ADD_REC, std::move(operands), lhs->loop.lock());
    }
    return std::nullopt;
}

static std::optional<std::shared_ptr<SCEVExpr>> fold_mul(std::shared_ptr<SCEVExpr> &lhs,
                                                         std::shared_ptr<SCEVExpr> &rhs) {
    if (lhs->type == SCEVType::CONSTANT && rhs->type == SCEVType::CONSTANT)
        return std::make_shared<SCEVExpr>(SCEVType::CONSTANT, lhs->constant().value() * rhs->constant().value());
    if (lhs->type == SCEVType::ADD_REC && rhs->type == SCEVType::CONSTANT) {
        std::vector<std::shared_ptr<SCEVExpr>> operands;
        auto rhs_operands = lhs->operands().value();
        for (auto operand : rhs_operands) {
            auto expr = fold_mul(operand, rhs);
            if (expr.has_value())
                operands.push_back(expr.value());
            else
                return std::nullopt;
        }
        return std::make_shared<SCEVExpr>(SCEVType::ADD_REC, std::move(operands), lhs->loop.lock());
    }
    return std::nullopt;
}

static bool basic_induce_var_analysis(std::shared_ptr<ir::Phi> phi,
                                      std::optional<std::shared_ptr<ir::opt_support::LoopInfo>> loop) {
    if (iter_cnt[phi] >= MAX_ITER_CNT) return false;
    iter_cnt[phi]++;
    auto initial = get_initial(phi, loop);
    auto next = get_next(phi, loop);

    auto next_add_inst = std::dynamic_pointer_cast<ir::Add>(next);
    if (!next_add_inst) return false;

    auto op1 = next_add_inst->get_operands_ref()[0];
    auto op2 = next_add_inst->get_operands_ref()[1];
    if (op1 == phi || op2 == phi) {
        auto c = op1 == phi ? op2 : op1;
        if (!std::dynamic_pointer_cast<ir::ConstantInt>(c) && !scev_info->contains(c)) return false;
        auto init_expr = scev_info->query(initial);
        auto inc_expr = scev_info->query(c);
        if (init_expr.has_value() && inc_expr.has_value()) {
            std::vector<std::shared_ptr<SCEVExpr>> operands;
            operands.push_back(init_expr.value());
            operands.push_back(inc_expr.value());
            auto new_expr = std::make_shared<SCEVExpr>(
                SCEVType::ADD_REC, std::move(operands), loop.has_value() ? loop.value() : nullptr);
            scev_info->add_expr(phi, std::move(new_expr));
            return true;
        }
    }
    return false;
}

static void general_induce_var_analysis(std::shared_ptr<ir::Instruction> inst) {
    if (auto add = std::dynamic_pointer_cast<ir::Add>(inst)) {
        auto lhs = scev_info->query(add->get_operands_ref()[0]);
        auto rhs = scev_info->query(add->get_operands_ref()[1]);
        if (lhs.has_value() && rhs.has_value()) {
            auto lhs_val = lhs.value(), rhs_val = rhs.value();
            if (auto l_loop = lhs_val->loop.lock(), r_loop = rhs_val->loop.lock(); l_loop && r_loop && l_loop != r_loop)
                return;
            auto expr = fold_add(lhs_val, rhs_val);
            if (expr.has_value()) scev_info->add_expr(inst, std::move(expr.value()));
        }
    } else if (auto mul = std::dynamic_pointer_cast<ir::Mul>(inst)) {
        auto lhs = scev_info->query(mul->get_operands_ref()[0]);
        auto rhs = scev_info->query(mul->get_operands_ref()[1]);
        if (lhs.has_value() && rhs.has_value()) {
            auto expr = fold_mul(lhs.value(), rhs.value());
            if (expr.has_value()) scev_info->add_expr(inst, std::move(expr.value()));
        }
    }
}
}  // namespace opt::pass
