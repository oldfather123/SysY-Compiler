#include "opt/exec/constexpr_interpreter.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <optional>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"

namespace opt {
namespace {

static bool is_single_block_straightline(const std::shared_ptr<ir::Function> &func) {
    if (!func) return false;
    auto &blocks = func->get_basic_blocks_ref();
    if (blocks.size() != 1) return false;
    auto block = blocks.front();
    // Reject any non-supported op families early
    for (auto &ins : block->get_instructions_ref()) {
        auto ty = ins->get_ins_type();
        using IT = ir::Instruction::InstructionType;
        switch (ty) {
            case IT::ADD:
            case IT::SUB:
            case IT::MUL:
            case IT::SDIV:
            case IT::SREM:
            case IT::AND:
            case IT::OR:
            case IT::XOR:
            case IT::SHL:
            case IT::LSHR:
            case IT::ASHR:
            case IT::FADD:
            case IT::FSUB:
            case IT::FMUL:
            case IT::FDIV:
            case IT::FREM:
            case IT::FNEG:
            case IT::ICMP:
            case IT::FCMP:
            case IT::FPTOSI:
            case IT::SITOFP:
            case IT::TRUNC:
            case IT::ZEXT:
            case IT::BITCAST:
            case IT::SELECT:
            case IT::RET:
                break;  // allowed
            default:
                return false;
        }
    }
    return true;
}

static std::optional<std::shared_ptr<ir::Constant>> get_const(ConstEnv const &env,
                                                              const std::shared_ptr<ir::Value> &v) {
    if (!v) return std::nullopt;
    if (auto c = std::dynamic_pointer_cast<ir::Constant>(v)) return c;
    auto it = env.find(v);
    if (it != env.end()) return it->second;
    return std::nullopt;
}

static bool is_i32(const std::shared_ptr<ir::Type> &t) {
    return t && t->is_integer_ty() && std::dynamic_pointer_cast<ir::IntegerType>(t)->bits_num() == 32;
}

static bool is_i1(const std::shared_ptr<ir::Type> &t) {
    return t && t->is_integer_ty() && std::dynamic_pointer_cast<ir::IntegerType>(t)->bits_num() == 1;
}

static bool is_f32(const std::shared_ptr<ir::Type> &t) { return t && t->is_float_ty(); }

}  // namespace

std::optional<std::shared_ptr<ir::Constant>> ConstexprInterpreter::eval_function_with_env(
    const std::shared_ptr<ir::Function> &func, const ConstEnv &in_env) {
    if (!is_single_block_straightline(func)) return std::nullopt;

    ConstEnv env = in_env;
    auto block = func->get_basic_blocks_ref().front();

    using IT = ir::Instruction::InstructionType;
    for (auto &ins : block->get_instructions_ref()) {
        auto ty = ins->get_ins_type();
        if (ty == IT::RET) {
            auto &ops = ins->get_operands_ref();
            if (ops.empty()) return std::nullopt;  // void ret unsupported in this evaluator
            auto cres = get_const(env, ops[0]);
            if (!cres) return std::nullopt;
            return *cres;
        }

        auto &ops = ins->get_operands_ref();
        auto type = ins->get_type();

        // Evaluate per opcode
        switch (ty) {
            case IT::ADD:
            case IT::SUB:
            case IT::MUL:
            case IT::SDIV:
            case IT::SREM:
            case IT::AND:
            case IT::OR:
            case IT::XOR:
            case IT::SHL:
            case IT::LSHR:
            case IT::ASHR: {
                if (!(is_i32(type) || is_i1(type))) return std::nullopt;
                auto c0 = get_const(env, ops[0]);
                auto c1 = get_const(env, ops[1]);
                if (!c0 || !c1) return std::nullopt;
                auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                auto i1c = std::dynamic_pointer_cast<ir::ConstantInt>(*c1);
                if (!i0 || !i1c) return std::nullopt;
                int a = i0->get_val();
                int b = i1c->get_val();
                int res = 0;
                switch (ty) {
                    case IT::ADD:
                        res = a + b;
                        break;
                    case IT::SUB:
                        res = a - b;
                        break;
                    case IT::MUL:
                        res = a * b;
                        break;
                    case IT::SDIV:
                        if (b == 0) return std::nullopt;
                        res = a / b;
                        break;
                    case IT::SREM:
                        if (b == 0) return std::nullopt;
                        res = a % b;
                        break;
                    case IT::AND:
                        res = a & b;
                        break;
                    case IT::OR:
                        res = a | b;
                        break;
                    case IT::XOR:
                        res = a ^ b;
                        break;
                    case IT::SHL:
                        if (b < 0 || b >= 32) return std::nullopt;
                        res = static_cast<uint32_t>(a) << b;
                        break;
                    case IT::LSHR:
                        if (b < 0 || b >= 32) return std::nullopt;
                        res = static_cast<uint32_t>(a) >> b;
                        break;
                    case IT::ASHR:
                        if (b < 0 || b >= 32) return std::nullopt;
                        res = a >> b;
                        break;
                    default:
                        return std::nullopt;
                }
                {
                    auto int_ty = std::dynamic_pointer_cast<ir::IntegerType>(type);
                    if (!int_ty) return std::nullopt;
                    if (int_ty->bits_num() == 1) res = (res & 1) ? 1 : 0;
                    env[ins] = std::make_shared<ir::ConstantInt>(
                        int_ty->bits_num() == 1 ? ir::IntegerType::get(1) : ir::IntegerType::get(32), res);
                }
                break;
            }

            case IT::FADD:
            case IT::FSUB:
            case IT::FMUL:
            case IT::FDIV:
            case IT::FREM:
            case IT::FNEG: {
                if (!is_f32(type)) return std::nullopt;
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto f0 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c0);
                if (!f0) return std::nullopt;
                float a = f0->get_val();
                float res = 0.0F;
                if (ty == IT::FNEG) {
                    res = -a;
                } else {
                    auto c1 = get_const(env, ops[1]);
                    if (!c1) return std::nullopt;
                    auto f1 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c1);
                    if (!f1) return std::nullopt;
                    float b = f1->get_val();
                    switch (ty) {
                        case IT::FADD:
                            res = a + b;
                            break;
                        case IT::FSUB:
                            res = a - b;
                            break;
                        case IT::FMUL:
                            res = a * b;
                            break;
                        case IT::FDIV:
                            if (b == 0.0F) return std::nullopt;
                            res = a / b;
                            break;
                        case IT::FREM:
                            res = std::fmod(a, b);
                            break;
                        default:
                            return std::nullopt;
                    }
                }
                env[ins] = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), res);
                break;
            }

            case IT::ICMP: {
                // result is i1
                if (!is_i1(type)) return std::nullopt;
                auto icmp = std::dynamic_pointer_cast<ir::ICmp>(ins);
                if (!icmp) return std::nullopt;
                auto c0 = get_const(env, ops[0]);
                auto c1 = get_const(env, ops[1]);
                if (!c0 || !c1) return std::nullopt;
                auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                auto i1c = std::dynamic_pointer_cast<ir::ConstantInt>(*c1);
                if (!i0 || !i1c) return std::nullopt;
                int a = i0->get_val();
                int b = i1c->get_val();
                bool r = false;
                switch (icmp->get_cmp_type()) {
                    case ir::ICmp::ICmpType::EQ:
                        r = a == b;
                        break;
                    case ir::ICmp::ICmpType::NE:
                        r = a != b;
                        break;
                    case ir::ICmp::ICmpType::SGT:
                        r = a > b;
                        break;
                    case ir::ICmp::ICmpType::SGE:
                        r = a >= b;
                        break;
                    case ir::ICmp::ICmpType::SLT:
                        r = a < b;
                        break;
                    case ir::ICmp::ICmpType::SLE:
                        r = a <= b;
                        break;
                    case ir::ICmp::ICmpType::UGT:
                        r = static_cast<uint32_t>(a) > static_cast<uint32_t>(b);
                        break;
                    case ir::ICmp::ICmpType::UGE:
                        r = static_cast<uint32_t>(a) >= static_cast<uint32_t>(b);
                        break;
                    case ir::ICmp::ICmpType::ULT:
                        r = static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
                        break;
                    case ir::ICmp::ICmpType::ULE:
                        r = static_cast<uint32_t>(a) <= static_cast<uint32_t>(b);
                        break;
                    default:
                        return std::nullopt;
                }
                env[ins] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), r ? 1 : 0);
                break;
            }

            case IT::FCMP: {
                if (!is_i1(type)) return std::nullopt;
                auto fcmp = std::dynamic_pointer_cast<ir::FCmp>(ins);
                if (!fcmp) return std::nullopt;
                auto c0 = get_const(env, ops[0]);
                auto c1 = get_const(env, ops[1]);
                if (!c0 || !c1) return std::nullopt;
                auto f0 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c0);
                auto f1 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c1);
                if (!f0 || !f1) return std::nullopt;
                float a = f0->get_val();
                float b = f1->get_val();
                bool na = std::isnan(a), nb = std::isnan(b);
                bool r = false;
                using FT = ir::FCmp::FCmpType;
                switch (fcmp->get_cmp_type()) {
                    case FT::OEQ:
                        r = !na && !nb && a == b;
                        break;
                    case FT::OGT:
                        r = !na && !nb && a > b;
                        break;
                    case FT::OGE:
                        r = !na && !nb && a >= b;
                        break;
                    case FT::OLT:
                        r = !na && !nb && a < b;
                        break;
                    case FT::OLE:
                        r = !na && !nb && a <= b;
                        break;
                    case FT::ONE:
                        r = !na && !nb && a != b;
                        break;
                    case FT::ORD:
                        r = !na && !nb;
                        break;
                    case FT::UEQ:
                        r = na || nb || a == b;
                        break;
                    case FT::UGT:
                        r = na || nb || a > b;
                        break;
                    case FT::UGE:
                        r = na || nb || a >= b;
                        break;
                    case FT::ULT:
                        r = na || nb || a < b;
                        break;
                    case FT::ULE:
                        r = na || nb || a <= b;
                        break;
                    case FT::UNE:
                        r = na || nb || a != b;
                        break;
                    case FT::UNO:
                        r = na || nb;
                        break;
                    default:
                        return std::nullopt;
                }
                env[ins] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), r ? 1 : 0);
                break;
            }

            case IT::FPTOSI: {
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto f0 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c0);
                if (!f0) return std::nullopt;
                env[ins] = f0->cast_to_int();
                break;
            }
            case IT::SITOFP: {
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                if (!i0) return std::nullopt;
                env[ins] = i0->cast_to_float();
                break;
            }

            case IT::TRUNC: {
                // Support only i32 -> i1
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                if (!i0) return std::nullopt;
                auto dst = std::dynamic_pointer_cast<ir::IntegerType>(type);
                if (!dst) return std::nullopt;
                if (dst->bits_num() == 1) {
                    int v = i0->get_val();
                    env[ins] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), (v & 1) ? 1 : 0);
                    break;
                }
                return std::nullopt;
            }
            case IT::ZEXT: {
                // Support only i1 -> i32
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                if (!i0) return std::nullopt;
                auto src_bits = std::dynamic_pointer_cast<ir::IntegerType>(ops[0]->get_type());
                auto dst_bits = std::dynamic_pointer_cast<ir::IntegerType>(type);
                if (!src_bits || !dst_bits) return std::nullopt;
                if (src_bits->bits_num() == 1 && dst_bits->bits_num() == 32) {
                    int v = i0->get_val();
                    env[ins] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), v ? 1 : 0);
                    break;
                }
                return std::nullopt;
            }
            case IT::BITCAST: {
                // Support f32 <-> i32
                auto c0 = get_const(env, ops[0]);
                if (!c0) return std::nullopt;
                auto src_ty = ops[0]->get_type();
                auto dst_ty = type;
                if (is_f32(src_ty) && is_i32(dst_ty)) {
                    auto f0 = std::dynamic_pointer_cast<ir::ConstantFloat>(*c0);
                    if (!f0) return std::nullopt;
                    float fv = f0->get_val();
                    static_assert(sizeof(float) == sizeof(uint32_t));
                    uint32_t bits;
                    std::memcpy(&bits, &fv, sizeof(bits));
                    env[ins] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(bits));
                    break;
                }
                if (is_i32(src_ty) && is_f32(dst_ty)) {
                    auto i0 = std::dynamic_pointer_cast<ir::ConstantInt>(*c0);
                    if (!i0) return std::nullopt;
                    uint32_t bits = static_cast<uint32_t>(i0->get_val());
                    float fv;
                    std::memcpy(&fv, &bits, sizeof(bits));
                    env[ins] = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), fv);
                    break;
                }
                // Same-type bitcast is a no-op
                if (src_ty == dst_ty) {
                    env[ins] = std::dynamic_pointer_cast<ir::Constant>(*c0);
                    break;
                }
                return std::nullopt;
            }
            case IT::SELECT: {
                // cond must be i1 constant
                auto ccond = get_const(env, ops[0]);
                if (!ccond) return std::nullopt;
                auto cint = std::dynamic_pointer_cast<ir::ConstantInt>(*ccond);
                if (!cint) return std::nullopt;
                auto chosen = cint->get_val() ? get_const(env, ops[1]) : get_const(env, ops[2]);
                if (!chosen) return std::nullopt;
                env[ins] = *chosen;
                break;
            }

            default:
                return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<std::shared_ptr<ir::Constant>> ConstexprInterpreter::eval_callee_on_constants(
    const std::shared_ptr<ir::Function> &callee, const std::vector<std::shared_ptr<ir::Constant>> &const_args) {
    if (!callee) return std::nullopt;
    auto &params = callee->get_arguments_ref();
    if (params.size() != const_args.size()) return std::nullopt;
    ConstEnv env;
    for (size_t i = 0; i < params.size(); ++i) env[params[i]] = const_args[i];
    return eval_function_with_env(callee, env);
}

}  // namespace opt
