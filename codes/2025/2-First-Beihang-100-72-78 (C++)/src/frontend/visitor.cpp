#include "visitor.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ast.hpp"
#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "token.hpp"
#include "util.hpp"

// symbol tabel related function
namespace frontend::symbol_table {
void SymbolTable::enter_scope() {
    scopes.emplace_back(std::make_unique<std::unordered_map<std::string, SymbolContent>>());
}

void SymbolTable::exit_scope() {
    if (scopes.size() <= 1) {
        logger::ERROR("[SymbolTable::exit_scope] can not exit global scope");
        throw std::runtime_error("[SymbolTable::exit_scope] can not exit global scope");
    }
    scopes.pop_back();
}

std::optional<SymbolTable::SymbolContent> SymbolTable::lookup(std::string name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (auto found = (*it)->find(name); found != (*it)->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

void SymbolTable::insert(std::string name, SymbolType type, const std::shared_ptr<ir::Value> &val) const {
    auto p = std::make_pair(val, type);
    scopes.back()->emplace(name, p);
}

void SymbolTable::remove(const std::string &name) const { scopes.back()->erase(name); }
}  // namespace frontend::symbol_table

namespace frontend::visitor {
const std::string LOCAL_VAR_PREFIX = "%var_";
const std::string BASIC_BLOCK_PRFIX = "block_";
const std::string FUNCTION_PREFIX = "@";
const std::string GLOBAL_VAR_PREFIX = "@global_var_";
}  // namespace frontend::visitor

// Visitor::visit_exps specializations
namespace frontend::visitor {
std::function<std::shared_ptr<Instruction>(
    const std::shared_ptr<BasicBlock> &, const std::shared_ptr<Value> &, const std::shared_ptr<Value> &, std::string)>
get_binary_create_func(token_type::TokenType op, const std::shared_ptr<Type> &type) {
    if (type == IntegerType::get(32)) {
        switch (op) {
            case token_type::TokenType::PLUS:
                return Add::create;
            case token_type::TokenType::MINU:
                return Sub::create;
            case token_type::TokenType::MULT:
                return Mul::create;
            case token_type::TokenType::DIV:
                return SDiv::create;
            case token_type::TokenType::MOD:
                return SRem::create;
            default:
                __builtin_unreachable();
        }
    }
    if (type == FloatType::get()) {
        switch (op) {
            case token_type::TokenType::PLUS:
                return FAdd::create;
            case token_type::TokenType::MINU:
                return FSub::create;
            case token_type::TokenType::MULT:
                return FMul::create;
            case token_type::TokenType::DIV:
                return FDiv::create;
            default:
                __builtin_unreachable();
        }
    }
    __builtin_unreachable();
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::LVAL>(ASTNodePtr &lval) {
    // LVal -> Ident {'[' Exp ']'}
    auto [symbol, symbol_type] =
        symbol_table.lookup(std::get<TokenPtr>(lval->get_children().front())->get_content()).value();
    auto type = std::dynamic_pointer_cast<PointerType>(symbol->get_type());
    // determine whether lval is an array
    // the lval is an array only if it's children size is greater than 1 or it wraps an array
    if ((type != nullptr && type->get_reference_type()->get_type_id() == Type::TypeID::ARRAY_ID) ||
        lval->get_children().size() > 1) {
        std::list<std::shared_ptr<Instruction>> ins_list;
        std::vector<std::shared_ptr<Value>> indexes;
        if (std::dynamic_pointer_cast<PointerType>(symbol->get_type())->get_reference_type()->get_type_id() ==
            Type::TypeID::POINTER_ID) {
            // if inner type is pointer, load first
            auto load_ptr = Load::create(cur_basic_block, symbol, gen_local_var_name());
            ins_list.push_back(load_ptr);
            symbol = load_ptr;
        } else {
            // otherwise, set the first index as 0
            indexes.push_back(std::make_shared<ConstantInt>(IntegerType::get(32), 0));
        }

        lval->for_each_child([this, &ins_list, &indexes](auto &&child) {
            if (std::holds_alternative<ASTNodePtr>(child)) {
                auto [index, new_ins] = visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(child));
                ins_list.splice(ins_list.end(), new_ins);
                indexes.push_back(index);
            }
        });

        if (in_array_arg) {
            indexes.push_back(std::make_shared<ConstantInt>(IntegerType::get(32), 0));
        }
        auto gep = Getelementptr::create(cur_basic_block, symbol, indexes, gen_local_var_name());
        ins_list.push_back(gep);
        if ((!is_left.empty() && is_left.top()) || in_array_arg) {
            return {gep, ins_list};
        }

        auto load = Load::create(cur_basic_block, gep, gen_local_var_name());
        ins_list.push_back(load);
        return {load, ins_list};
    }

    // visit non array LVal
    if ((!is_left.empty() && is_left.top()) || symbol_type == SymbolType::CON) {
        return {symbol, {}};
    }
    // TODO if is neccessary to judge `Argument` specifically

    auto load = Load::create(cur_basic_block, symbol, gen_local_var_name());
    return {load, {load}};
}

// forward declaration
template <>
VisitExpsResult Visitor::visit_exps<NodeType::EXP>(ASTNodePtr &exp);

template <>
VisitExpsResult Visitor::visit_exps<NodeType::ADD_EXP>(ASTNodePtr &exp);

template <>
VisitExpsResult Visitor::visit_exps<NodeType::REL_EXP>(ASTNodePtr &rel_exp) {
    // RelExp -> AddExp {('<' | '>' | '<=' | '>=') AddExp}
    auto result = visit_exps<NodeType::ADD_EXP>(std::get<ASTNodePtr>(rel_exp->get_children().front()));
    auto ret_val = result.first;
    auto ret_ins_list = result.second;
    if (rel_exp->get_children().size() == 1) return {ret_val, ret_ins_list};
    auto op = std::get<TokenPtr>(rel_exp->get_children()[1])->get_type();
    // clang-format off
        rel_exp->for_each_child([this, &ret_val, &ret_ins_list, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &ret_val, &ret_ins_list, &op](ASTNodePtr &node) {
                        auto [rhs_val, rhs_ins_list] = visit_exps<NodeType::ADD_EXP>(node);
                        ret_ins_list.splice(ret_ins_list.end(), rhs_ins_list);
                        auto [left, right, ins] = make_binary_type_conversion(ret_val, rhs_val);
                        ret_ins_list.splice(ret_ins_list.end(), ins);
                        ret_val = get_icmp_create_func(op, left, right);
                        ret_ins_list.push_back(std::dynamic_pointer_cast<Instruction>(ret_val));
                    }
            }, child);
        }, 2);
    // clang-format on
    return {ret_val, ret_ins_list};
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::EQ_EXP>(ASTNodePtr &eq_exp) {
    // EqExp -> RelExp {('==' | '!=') RelExp}
    auto result = visit_exps<NodeType::REL_EXP>(std::get<ASTNodePtr>(eq_exp->get_children().front()));
    auto ret_val = result.first;
    auto ret_ins_list = result.second;
    if (eq_exp->get_children().size() == 1) return {ret_val, ret_ins_list};
    auto op = std::get<TokenPtr>(eq_exp->get_children()[1])->get_type();
    // clang-format off
        eq_exp->for_each_child([this, &ret_val, &ret_ins_list, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &ret_val, &ret_ins_list, &op](ASTNodePtr &node) {
                        auto [rhs_val, rhs_ins_list] = visit_exps<NodeType::REL_EXP>(node);
                        ret_ins_list.splice(ret_ins_list.end(), rhs_ins_list);
                        auto [left, right, ins] = make_binary_type_conversion(ret_val, rhs_val);
                        ret_ins_list.splice(ret_ins_list.end(), ins);
                        ret_val = get_icmp_create_func(op, left, right);
                        ret_ins_list.push_back(std::dynamic_pointer_cast<Instruction>(ret_val));
                    }
            }, child);
        }, 2);
    // clang-format on
    return {ret_val, ret_ins_list};
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::PRIMARY_EXP>(ASTNodePtr &primary_exp) {
    if (std::holds_alternative<TokenPtr>(primary_exp->get_children().front())) {
        // '(' Exp ')'
        return visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(primary_exp->get_children()[1]));
    }
    auto &child = std::get<ASTNodePtr>(primary_exp->get_children().front());
    if (child->get_type() == NodeType::NUMBER) {
        // Number
        auto &token = std::get<TokenPtr>(child->get_children().front());
        if (token->is_type(token_type::TokenType::FLOATCON)) {
            return {std::make_shared<ConstantFloat>(FloatType::get(), std::stof(token->get_content())), {}};
        } else if (token->is_type(token_type::TokenType::INTCON)) {
            return {std::make_shared<ConstantInt>(IntegerType::get(32), std::stoi(token->get_content(), nullptr, 0)),
                    {}};
        } else {
            __builtin_unreachable();
        }
    }
    // LVal
    is_left.push(false);
    auto res = visit_exps<NodeType::LVAL>(child);
    is_left.pop();
    return res;
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::UNARY_EXP>(ASTNodePtr &unary_exp) {
    // UnaryExp -> Ident '(' [FuncRParams] ')' | PrimaryExp | UnaryOp UnaryExp
    auto *first_node = std::get_if<ASTNodePtr>(&unary_exp->get_children().front());
    if (first_node != nullptr && (*first_node)->get_type() == NodeType::UNARY_OP) {
        auto op = std::get<TokenPtr>((*first_node)->get_children().front())->get_type();
        auto [val, ins_list] = visit_exps<NodeType::UNARY_EXP>(std::get<ASTNodePtr>(unary_exp->get_children()[1]));
        if (op == token_type::TokenType::PLUS) {
            return {val, ins_list};
        } else if (op == token_type::TokenType::MINU) {
            // minu by zero
            if (val->get_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
                // i1 --> i32
                auto [converted_val, convert_inst] = make_collect_type_conversion(val, IntegerType::get(32));
                ins_list.splice(ins_list.end(), convert_inst);

                // 0 - converted_val
                auto left_zero = std::make_shared<ConstantInt>(IntegerType::get(32), 0);
                auto new_val = Sub::create(cur_basic_block, left_zero, converted_val, gen_local_var_name());
                ins_list.push_back(new_val);
                return {new_val, ins_list};
            } else if (val->get_type() == FloatType::get()) {
                auto left_zero = std::make_shared<ConstantFloat>(FloatType::get(), 0.0);
                auto new_val = FSub::create(cur_basic_block, left_zero, val, gen_local_var_name());
                ins_list.push_back(new_val);
                return {new_val, ins_list};
            } else {
                logger::ERROR("[Visitor::visit_exps<NodeType::UNARY_EXP>] Unexpected type for MINU operation, ",
                              val->get_type()->to_string());
                __builtin_unreachable();
            }
        } else if (op == token_type::TokenType::NOT) {
            // compare with zero
            if (val->get_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
                // i1 --> i32
                auto [converted_val, convert_inst] = make_collect_type_conversion(val, IntegerType::get(32));
                ins_list.splice(ins_list.end(), convert_inst);

                // 0 == converted_val
                auto left_zero = std::make_shared<ConstantInt>(IntegerType::get(32), 0);
                auto new_val =
                    ICmp::create(cur_basic_block, ICmp::ICmpType::EQ, left_zero, converted_val, gen_local_var_name());
                ins_list.push_back(new_val);
                return {new_val, ins_list};
            } else if (val->get_type() == FloatType::get()) {
                auto left_zero = std::make_shared<ConstantFloat>(FloatType::get(), 0.0);
                auto new_val = FCmp::create(cur_basic_block, FCmp::FCmpType::OEQ, left_zero, val, gen_local_var_name());
                ins_list.push_back(new_val);
                return {new_val, ins_list};
            } else {
                logger::ERROR("[Visitor::visit_exps<NodeType::UNARY_EXP>] Unexpected type for NOT operation, ",
                              val->get_type()->to_string());
                __builtin_unreachable();
            }
        } else {
            logger::ERROR("[Visitor::visit_exps<NodeType::UNARY_EXP>] Unexpected operator type, ", op);
            __builtin_unreachable();
        }
    }
    if (std::holds_alternative<ASTNodePtr>(unary_exp->get_children().front())) {
        // PrimaryExp
        return visit_exps<NodeType::PRIMARY_EXP>(std::get<ASTNodePtr>(unary_exp->get_children().front()));
    }
    // call function
    auto &ident = std::get<TokenPtr>(unary_exp->get_children().front());
    auto ident_name = ident->get_content();
    // NOTE lib function `starttime` and `stoptime` are not in symbol table, need to judge here
    if (ident_name == "starttime" || ident_name == "stoptime") {
        auto func = std::dynamic_pointer_cast<Function>(symbol_table.lookup("_sysy_" + ident_name)->first);
        auto call = Call::create(cur_basic_block,
                                 func,
                                 {std::make_shared<ConstantInt>(IntegerType::get(32), ident->get_line())},
                                 gen_local_var_name());
        return {call, {call}};
    }
    auto call_func = std::dynamic_pointer_cast<Function>(symbol_table.lookup(ident_name)->first);
    std::vector<std::shared_ptr<Value>> args;
    std::list<std::shared_ptr<Instruction>> ins_list;
    if (unary_exp->get_children().size() > 3) {
        // with param to pass
        auto param_types = std::dynamic_pointer_cast<FunctionType>(call_func->get_type())->get_param_types();
        size_t cur_arg_num = 0;
        auto &func_r_param = std::get<ASTNodePtr>(unary_exp->get_children()[2]);
        // clang-format off
            func_r_param->for_each_child([this, &cur_arg_num, &param_types, &args, &ins_list](auto &&child) {
                if (std::holds_alternative<ASTNodePtr>(child)) {
                    in_array_arg = param_types[cur_arg_num++]->get_type_id() == Type::TypeID::POINTER_ID;
                    auto [arg, new_ins] = visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(child));

                    if (!in_array_arg) {
                        ins_list.splice(ins_list.end(), new_ins);
                        auto [converted_arg, convert_ins] =
                            make_collect_type_conversion(arg, param_types[cur_arg_num - 1]);
                        ins_list.splice(ins_list.end(), convert_ins);
                        args.push_back(converted_arg);
                    }else {
                        ins_list.splice(ins_list.end(), new_ins);
                        args.push_back(arg);
                    }
                    in_array_arg = false;
                }
            });
        // clang-format on
    }
    auto call = Call::create(cur_basic_block, call_func, args, gen_local_var_name());
    ins_list.push_back(call);
    return {call, ins_list};
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::MUL_EXP>(ASTNodePtr &mul_exp) {
    auto result = visit_exps<NodeType::UNARY_EXP>(std::get<ASTNodePtr>(mul_exp->get_children().front()));
    auto ret_val = result.first;
    auto ret_ins_list = result.second;
    if (mul_exp->get_children().size() == 1) return {ret_val, ret_ins_list};
    auto op = std::get<TokenPtr>(mul_exp->get_children()[1])->get_type();
    // clang-format off
        mul_exp->for_each_child([this, &ret_val, &ret_ins_list, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &ret_val, &ret_ins_list, &op](ASTNodePtr &node) {
                        auto [rhs_val, rhs_ins_list] = visit_exps<NodeType::UNARY_EXP>(node);
                        ret_ins_list.splice(ret_ins_list.end(), rhs_ins_list);
                        auto [left, right, ins] = make_binary_type_conversion(ret_val, rhs_val);
                        ret_ins_list.splice(ret_ins_list.end(), ins);
                        ret_val = get_binary_create_func(op, left->get_type())(cur_basic_block, left, right,
                                                                               gen_local_var_name());
                        ret_ins_list.push_back(std::dynamic_pointer_cast<Instruction>(ret_val));
                    }
            }, child);
        }, 2);
    // clang-format on
    return {ret_val, ret_ins_list};
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::ADD_EXP>(ASTNodePtr &add_exp) {
    auto result = visit_exps<NodeType::MUL_EXP>(std::get<ASTNodePtr>(add_exp->get_children().front()));
    auto ret_val = result.first;
    auto ret_ins_list = result.second;
    if (add_exp->get_children().size() == 1) return {ret_val, ret_ins_list};
    auto op = std::get<TokenPtr>(add_exp->get_children()[1])->get_type();
    // clang-format off
        add_exp->for_each_child([this, &ret_val, &ret_ins_list, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &ret_val, &ret_ins_list, &op](ASTNodePtr &node) {
                        auto [rhs_val, rhs_ins_list] = visit_exps<NodeType::MUL_EXP>(node);
                        ret_ins_list.splice(ret_ins_list.end(), rhs_ins_list);
                        auto [left, right, ins] = make_binary_type_conversion(ret_val, rhs_val);
                        ret_ins_list.splice(ret_ins_list.end(), ins);
                        ret_val = get_binary_create_func(op, left->get_type())(cur_basic_block, left, right,
                                                                               gen_local_var_name());
                        ret_ins_list.push_back(std::dynamic_pointer_cast<Instruction>(ret_val));
                    }
            }, child);
        }, 2);
    // clang-format on
    return {ret_val, ret_ins_list};
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::CONST_EXP>(ASTNodePtr &const_exp) {
    return visit_exps<NodeType::ADD_EXP>(std::get<ASTNodePtr>(const_exp->get_children().front()));
}

template <>
VisitExpsResult Visitor::visit_exps<NodeType::EXP>(ASTNodePtr &exp) {
    return visit_exps<NodeType::ADD_EXP>(std::get<ASTNodePtr>(exp->get_children().front()));
}
}  // namespace frontend::visitor

// Visitor::visit specializations
namespace frontend::visitor {
constexpr const Visitor::VisitPattern LIST = Visitor::VisitPattern::LIST;
constexpr const Visitor::VisitPattern SINGLE = Visitor::VisitPattern::SINGLE;

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::STMT, Instruction, LIST>(ASTNodePtr &stmt);

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::STMT, BasicBlock, LIST>(ASTNodePtr &stmt);

std::shared_ptr<Constant> cal_binary_exp(const std::shared_ptr<Constant> &lhs,
                                         const std::shared_ptr<Constant> &rhs,
                                         token::TokenType op) {
    switch (op) {
        case token_type::TokenType::PLUS:
            return *lhs + *rhs;
        case token_type::TokenType::MINU:
            return *lhs - *rhs;
        case token_type::TokenType::MULT:
            return *lhs * *rhs;
        case token_type::TokenType::DIV:
            return *lhs / *rhs;
        case token_type::TokenType::MOD:
            return *lhs % *rhs;
        default:
            logger::ERROR("[cal_binary_exp] unknown operator type, ", op);
            __builtin_unreachable();
    }
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::LVAL, Constant, SINGLE>(ASTNodePtr &lval) {
    // LVal -> Ident {'[' Exp ']'}
    auto ident = symbol_table.lookup(std::get<TokenPtr>(lval->get_children().front())->get_content())->first;
    if (auto glv = std::dynamic_pointer_cast<GlobalVariable>(ident)) {
        return glv->get_init_value();
    } else if (auto con = std::dynamic_pointer_cast<Constant>(ident)) {
        return con;
    } else {
        logger::ERROR("[Visitor::visit<NodeType::LVAL, Constant, SINGLE>] unexpected identifier type",
                      ident->get_type());
        __builtin_unreachable();
    }
}

// forward declaration
template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::EXP, Constant, SINGLE>(ASTNodePtr &exp);

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::PRIMARY_EXP, Constant, SINGLE>(ASTNodePtr &primary_exp) {
    if (std::holds_alternative<TokenPtr>(primary_exp->get_children().front())) {
        // '(' Exp ')'
        return visit<NodeType::EXP, Constant, SINGLE>(std::get<ASTNodePtr>(primary_exp->get_children()[1]));
    }
    auto &child = std::get<ASTNodePtr>(primary_exp->get_children().front());
    if (child->get_type() == NodeType::NUMBER) {
        // Number
        auto &token = std::get<TokenPtr>(child->get_children().front());
        if (token->is_type(token_type::TokenType::FLOATCON)) {
            return std::make_shared<ConstantFloat>(FloatType::get(), std::stof(token->get_content()));
        } else if (token->is_type(token_type::TokenType::INTCON)) {
            return std::make_shared<ConstantInt>(IntegerType::get(32), std::stoi(token->get_content(), nullptr, 0));
        } else {
            __builtin_unreachable();
        }
    }
    // LVal
    return visit<NodeType::LVAL, Constant, SINGLE>(child);
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::UNARY_EXP, Constant, SINGLE>(ASTNodePtr &unary_exp) {
    // UnaryExp -> Ident '(' [FuncRParams] ')' | PrimaryExp | UnaryOp UnaryExp
    // here, function call can not be evaluated
    // so, only consider PrimaryExp | UnaryOp UnaryExp
    auto *first_node = std::get_if<ASTNodePtr>(&unary_exp->get_children().front());
    if (first_node != nullptr && (*first_node)->get_type() == NodeType::UNARY_OP) {
        auto op = std::get<TokenPtr>((*first_node)->get_children().front())->get_type();
        auto rhs = visit<NodeType::UNARY_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(unary_exp->get_children()[1]));
        if (op == token::TokenType::PLUS) {
            return rhs;
        } else if (op == token::TokenType::MINU) {
            return -*rhs;
        } else {
            logger::ERROR("[Visitor::cal_exp_val<NodeType::UNARY_EXP, int>] unknown operator type, ", op);
            __builtin_unreachable();
        }
    }
    return visit<NodeType::PRIMARY_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(unary_exp->get_children().front()));
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::MUL_EXP, Constant, SINGLE>(ASTNodePtr &mul_exp) {
    auto res = visit<NodeType::UNARY_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(mul_exp->get_children().front()));
    if (mul_exp->get_children().size() == 1) return res;
    auto op = std::get<TokenPtr>(mul_exp->get_children()[1])->get_type();
    // clang-format off
        mul_exp->for_each_child([this, &res, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &res, &op](ASTNodePtr &node) {
                        auto rhs = visit<NodeType::UNARY_EXP, Constant, SINGLE>(node);
                        res = cal_binary_exp(res, rhs, op);
                    }
            }, child);
        }, 2);
    // clang-format on
    return res;
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::ADD_EXP, Constant, SINGLE>(ASTNodePtr &add_exp) {
    auto res = visit<NodeType::MUL_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(add_exp->get_children().front()));
    if (add_exp->get_children().size() == 1) return res;
    auto op = std::get<TokenPtr>(add_exp->get_children()[1])->get_type();
    // clang-format off
        add_exp->for_each_child([this, &res, &op](auto &&child) {
            std::visit(overloaded{
                    [&op](const TokenPtr &token) {
                        op = token->get_type();
                    },
                    [this, &res, &op](ASTNodePtr &node) {
                        auto rhs = visit<NodeType::MUL_EXP, Constant, SINGLE>(node);
                        res = cal_binary_exp(res, rhs, op);
                    }
            }, child);
        }, 2);
    // clang-format on
    return res;
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::EXP, Constant, SINGLE>(ASTNodePtr &exp) {
    if (cur_b_type == IntegerType::get(32)) {
        return visit<NodeType::ADD_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(exp->get_children().front()))
            ->cast_to_int();
    } else {
        return visit<NodeType::ADD_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(exp->get_children().front()))
            ->cast_to_float();
    }
}

template <>
std::shared_ptr<Constant> Visitor::visit<NodeType::CONST_EXP, Constant, SINGLE>(ASTNodePtr &const_exp) {
    if (cur_b_type == IntegerType::get(32)) {
        return visit<NodeType::ADD_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(const_exp->get_children().front()))
            ->cast_to_int();
    } else {
        return visit<NodeType::ADD_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(const_exp->get_children().front()))
            ->cast_to_float();
    }
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::BLOCK, BasicBlock, LIST>(ASTNodePtr &block) {
    // Block -> '{' {BlockItem} '}'
    symbol_table.enter_scope();
    // alloca for function arguments
    if (!func_arg_init) {
        for (auto &arg : cur_function->get_arguments_ref()) {
            auto alloca = Alloca::create(cur_basic_block, arg->get_type(), gen_local_var_name());
            auto store = Store::create(cur_basic_block, arg, alloca);
            cur_basic_block->add_instructions({alloca, store});
            symbol_table.insert(arg->get_name(), SymbolType::VAR, alloca);
            // NOTE orginally, argument's name was set as token name, here allocating a new name for it
            arg->set_name(gen_local_var_name());
        }
        func_arg_init = true;
    }

    std::list<std::shared_ptr<BasicBlock>> basic_block_list;

    block->for_each_child([this, &basic_block_list](auto &&child) {
        std::visit(
            overloaded{
                [this, &basic_block_list](ASTNodePtr &node) {
                    // BlockItem
                    auto &item = std::get<ASTNodePtr>(node->get_children().front());

                    // Stmt could generate basic blocks or instructions.
                    // need to judge inside
                    if (item->get_type() == NodeType::STMT) {
                        auto &stmt = std::get<ASTNodePtr>(item->get_children().front());
                        if (stmt->get_type() == NodeType::BLOCK || stmt->get_type() == NodeType::IF_STMT ||
                            stmt->get_type() == NodeType::WHILE_STMT || stmt->get_type() == NodeType::BREAK_STMT ||
                            stmt->get_type() == NodeType::CONTINUE_STMT) {
                            // Block, IfStmt, WhileStmt generate blocks
                            basic_block_list.splice(basic_block_list.end(),
                                                    visit<NodeType::STMT, BasicBlock, LIST>(item));
                        } else {
                            // otherwise, generate instructions in current scope
                            cur_basic_block->add_instructions(visit<NodeType::STMT, Instruction, LIST>(item));
                        }
                    } else {
                        // Decl generates instructions
                        cur_basic_block->add_instructions(visit<NodeType::DECL, Instruction, LIST>(item));
                    }
                },
                [](TokenPtr &token) {
                    if (token->get_type() != token_type::TokenType::LBRACE &&
                        token->get_type() != token_type::TokenType::RBRACE) {
                        logger::ERROR(
                            "[Visitor::visit] Unexpected token type: ", token->get_type(), " when visiting Block");
                        __builtin_unreachable();
                    }
                }},
            child);
    });
    symbol_table.exit_scope();
    return basic_block_list;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::RETURN_STMT, Instruction, LIST>(
    ASTNodePtr &return_stmt) {
    // ReturnStmt -> 'return' Exp ';'
    if (return_stmt->get_children().size() == 3) {
        auto [val, ins_list] = visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(return_stmt->get_children()[1]));
        auto [ret_val, conv_ins] = make_collect_type_conversion(val, cur_function->get_return_type());
        ins_list.splice(ins_list.end(), conv_ins);
        auto ret = Ret::create(cur_basic_block, ret_val);
        ins_list.push_back(ret);
        return ins_list;
    }
    // 'return' ';'
    return {Ret::create(cur_basic_block)};
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::EXP_STMT, Instruction, LIST>(ASTNodePtr &exp_stmt) {
    // ExpStmt -> [Exp] ';'
    if (exp_stmt->get_children().size() > 1) {
        auto [val, ins_list] = visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(exp_stmt->get_children().front()));
        return ins_list;
    }
    return {};
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::LAND_EXP, BasicBlock, LIST>(ASTNodePtr &land_exp) {
    // LAndExp -> EqExp { '&&' EqExp }
    std::list<std::shared_ptr<BasicBlock>> basic_block_list;
    auto &children = land_exp->get_children();

    for (size_t i = 0; i < children.size(); i += 2) {
        std::shared_ptr<BasicBlock> next_basic_block;
        if (i == children.size() - 1) {
            // EqExp
            next_basic_block = true_basic_block_list.back();
        } else {
            // EqExp '&&' EqExp
            next_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
            basic_block_list.push_back(next_basic_block);
        }

        // EqExp
        auto [value, ins_list] = visit_exps<NodeType::EQ_EXP>(std::get<ASTNodePtr>(children[i]));
        cur_basic_block->add_instructions(std::move(ins_list));
        auto [converted_value, convert_ins_list] = make_collect_type_conversion(value, IntegerType::get(1));
        cur_basic_block->add_instructions(std::move(convert_ins_list));

        // new Br
        cur_basic_block->add_instructions({Br::create(
            cur_basic_block, converted_value, next_basic_block, false_basic_block_list.back(), gen_local_var_name())});

        cur_basic_block = next_basic_block;
    }

    return basic_block_list;
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::LOR_EXP, BasicBlock, LIST>(ASTNodePtr &lor_exp) {
    // LOrExp -> LAndExp { '||' LAndExp }
    std::list<std::shared_ptr<BasicBlock>> basic_block_list;
    auto &children = lor_exp->get_children();

    for (size_t i = 0; i < children.size(); i += 2) {
        std::shared_ptr<BasicBlock> next_basic_block;
        if (i == children.size() - 1) {
            // ... LAndExp
            next_basic_block = false_basic_block_list.back();
        } else {
            // ... LAndExp ...
            next_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
            basic_block_list.push_back(next_basic_block);
        }
        // LAndExp
        true_basic_block_list.push_back(true_basic_block_list.back());
        false_basic_block_list.push_back(next_basic_block);
        basic_block_list.splice(basic_block_list.end(),
                                visit<NodeType::LAND_EXP, BasicBlock, LIST>(std::get<ASTNodePtr>(children[i])));
        true_basic_block_list.pop_back();
        false_basic_block_list.pop_back();
        cur_basic_block = next_basic_block;
    }

    return basic_block_list;
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::COND, BasicBlock, LIST>(ASTNodePtr &cond) {
    // Cond -> LOrExp
    return visit<NodeType::LOR_EXP, BasicBlock, LIST>(std::get<ASTNodePtr>(cond->get_children().front()));
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::IF_STMT, BasicBlock, LIST>(ASTNodePtr &if_stmt) {
    // IfStmt -> 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
    std::list<std::shared_ptr<BasicBlock>> basic_block_list;

    // enter scope
    symbol_table.enter_scope();

    // if basic block
    std::shared_ptr<BasicBlock> if_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    basic_block_list.push_back(if_basic_block);

    // else basic block
    bool has_else = if_stmt->get_children().size() > 5;
    std::shared_ptr<BasicBlock> else_basic_block = nullptr;
    if (has_else) {
        else_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
        basic_block_list.push_back(else_basic_block);
    }

    // finish basic block
    std::shared_ptr<BasicBlock> finish_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    basic_block_list.push_back(finish_basic_block);

    // Cond
    true_basic_block_list.push_back(if_basic_block);
    false_basic_block_list.push_back(has_else ? else_basic_block : finish_basic_block);
    basic_block_list.splice(basic_block_list.end(),
                            visit<NodeType::COND, BasicBlock, LIST>(std::get<ASTNodePtr>(if_stmt->get_children()[2])));
    true_basic_block_list.pop_back();
    false_basic_block_list.pop_back();

    // Stmt1
    cur_basic_block = if_basic_block;
    auto &stmt1 = std::get<ASTNodePtr>(if_stmt->get_children()[4]);
    auto &specific_stmt1 = std::get<ASTNodePtr>(stmt1->get_children().front());
    if (specific_stmt1->get_type() == NodeType::BLOCK || specific_stmt1->get_type() == NodeType::IF_STMT ||
        specific_stmt1->get_type() == NodeType::WHILE_STMT || specific_stmt1->get_type() == NodeType::BREAK_STMT ||
        specific_stmt1->get_type() == NodeType::CONTINUE_STMT) {
        // Block, IfStmt, WhileStmt generate blocks
        basic_block_list.splice(basic_block_list.end(), visit<NodeType::STMT, BasicBlock, LIST>(stmt1));
    } else {
        // otherwise, generate instructions in current scope
        cur_basic_block->add_instructions(visit<NodeType::STMT, Instruction, LIST>(stmt1));
    }

    // add Br
    cur_basic_block->add_instructions({Br::create(cur_basic_block, finish_basic_block, gen_local_var_name())});

    if (has_else) {
        // Stmt2
        cur_basic_block = else_basic_block;
        auto &stmt2 = std::get<ASTNodePtr>(if_stmt->get_children()[6]);
        auto &specific_stmt2 = std::get<ASTNodePtr>(stmt2->get_children().front());
        if (specific_stmt2->get_type() == NodeType::BLOCK || specific_stmt2->get_type() == NodeType::IF_STMT ||
            specific_stmt2->get_type() == NodeType::WHILE_STMT || specific_stmt2->get_type() == NodeType::BREAK_STMT ||
            specific_stmt2->get_type() == NodeType::CONTINUE_STMT) {
            basic_block_list.splice(basic_block_list.end(), visit<NodeType::STMT, BasicBlock, LIST>(stmt2));
        } else {
            cur_basic_block->add_instructions(visit<NodeType::STMT, Instruction, LIST>(stmt2));
        }

        // add Br
        cur_basic_block->add_instructions({Br::create(cur_basic_block, finish_basic_block, gen_local_var_name())});
    }

    // exit scope
    symbol_table.exit_scope();

    // set current basic block
    cur_basic_block = finish_basic_block;

    return basic_block_list;
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::WHILE_STMT, BasicBlock, LIST>(ASTNodePtr &while_stmt) {
    // WhileStmt -> 'while' '(' Cond ')' Stmt
    std::list<std::shared_ptr<BasicBlock>> basic_block_list;

    // enter scope
    symbol_table.enter_scope();

    // conde basic block
    auto cond_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    basic_block_list.push_back(cond_basic_block);

    // while basic block
    auto while_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    basic_block_list.push_back(while_basic_block);

    // finish basic block
    auto finish_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    basic_block_list.push_back(finish_basic_block);

    // add Br
    cur_basic_block->add_instructions({Br::create(cur_basic_block, cond_basic_block, gen_local_var_name())});

    // add loop info
    loop_entry_blocks.push_back(cond_basic_block);
    loop_exit_blocks.push_back(finish_basic_block);

    // Cond
    cur_basic_block = cond_basic_block;
    true_basic_block_list.push_back(while_basic_block);
    false_basic_block_list.push_back(finish_basic_block);
    basic_block_list.splice(
        basic_block_list.end(),
        visit<NodeType::COND, BasicBlock, LIST>(std::get<ASTNodePtr>(while_stmt->get_children()[2])));

    // Stmt
    cur_basic_block = while_basic_block;
    auto &stmt = std::get<ASTNodePtr>(while_stmt->get_children()[4]);
    auto &specific_stmt = std::get<ASTNodePtr>(stmt->get_children().front());
    if (specific_stmt->get_type() == NodeType::BLOCK || specific_stmt->get_type() == NodeType::IF_STMT ||
        specific_stmt->get_type() == NodeType::WHILE_STMT || specific_stmt->get_type() == NodeType::BREAK_STMT ||
        specific_stmt->get_type() == NodeType::CONTINUE_STMT) {
        // Block, IfStmt, WhileStmt generate blocks
        basic_block_list.splice(basic_block_list.end(), visit<NodeType::STMT, BasicBlock, LIST>(stmt));
    } else {
        // otherwise, generate instructions in current scope
        cur_basic_block->add_instructions(visit<NodeType::STMT, Instruction, LIST>(stmt));
    }

    // add Br
    cur_basic_block->add_instructions({Br::create(cur_basic_block, cond_basic_block, gen_local_var_name())});

    // remove loop info
    loop_entry_blocks.pop_back();
    loop_exit_blocks.pop_back();

    // exit scope
    symbol_table.exit_scope();

    // set current basic block
    cur_basic_block = finish_basic_block;

    return basic_block_list;
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::BREAK_STMT, BasicBlock, LIST>(
    ASTNodePtr & /*break_stmt*/) {
    auto finish_basic_block = loop_exit_blocks.back();
    cur_basic_block->add_instructions({Br::create(cur_basic_block, finish_basic_block, gen_local_var_name())});
    cur_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    return {cur_basic_block};
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::CONTINUE_STMT, BasicBlock, LIST>(
    ASTNodePtr & /*continue_stmt*/) {
    auto while_basic_block = loop_entry_blocks.back();
    cur_basic_block->add_instructions({Br::create(cur_basic_block, while_basic_block, gen_local_var_name())});
    cur_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    return {cur_basic_block};
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::ASSIGN_STMT, Instruction, LIST>(
    ASTNodePtr &assign_stmt) {
    // AssignStmt -> LVal '=' Exp ';'
    std::list<std::shared_ptr<Instruction>> ins_list;
    is_left.push(true);
    auto [left, left_ins] = visit_exps<NodeType::LVAL>(std::get<ASTNodePtr>(assign_stmt->get_children().front()));
    is_left.pop();
    auto [right, right_ins] = visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(assign_stmt->get_children()[2]));
    auto [actual_right, new_ins] = make_collect_type_conversion(
        right, std::dynamic_pointer_cast<PointerType>(left->get_type())->get_reference_type());
    auto store = Store::create(cur_basic_block, actual_right, left, gen_local_var_name());
    ins_list.splice(ins_list.end(), left_ins);
    ins_list.splice(ins_list.end(), right_ins);
    ins_list.splice(ins_list.end(), new_ins);
    ins_list.push_back(store);
    return ins_list;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::STMT, Instruction, LIST>(ASTNodePtr &stmt) {
    auto &child = std::get<ASTNodePtr>(stmt->get_children().front());
    switch (child->get_type()) {
        case NodeType::ASSIGN_STMT:
            return visit<NodeType::ASSIGN_STMT, Instruction, LIST>(child);
        case NodeType::RETURN_STMT:
            return visit<NodeType::RETURN_STMT, Instruction, LIST>(child);
        case NodeType::EXP_STMT:
            return visit<NodeType::EXP_STMT, Instruction, LIST>(child);
        default:
            logger::ERROR("[Visitor::visit<STMT, Instruction, LIST>] Unexpected stmt type: ", stmt->get_type());
            __builtin_unreachable();
    }
}

template <>
std::list<std::shared_ptr<BasicBlock>> Visitor::visit<NodeType::STMT, BasicBlock, LIST>(ASTNodePtr &stmt) {
    auto &child = std::get<ASTNodePtr>(stmt->get_children().front());
    switch (child->get_type()) {
        case NodeType::IF_STMT:
            return visit<NodeType::IF_STMT, BasicBlock, LIST>(child);
        case NodeType::WHILE_STMT:
            return visit<NodeType::WHILE_STMT, BasicBlock, LIST>(child);
        case NodeType::BLOCK:
            return visit<NodeType::BLOCK, BasicBlock, LIST>(child);
        case NodeType::BREAK_STMT:
            return visit<NodeType::BREAK_STMT, BasicBlock, LIST>(child);
        case NodeType::CONTINUE_STMT:
            return visit<NodeType::CONTINUE_STMT, BasicBlock, LIST>(child);
        default:
            logger::ERROR("[Visitor::visit<STMT, BasicBlock, LIST>] Unexpected stmt type: ", stmt->get_type());
            __builtin_unreachable();
    }
}

template <>
std::shared_ptr<Argument> Visitor::visit<NodeType::FUNC_F_PARAM, Argument, SINGLE>(ASTNodePtr &func_f_param) {
    // FuncFParam -> BType Ident ['[' ']' {'[' Exp ']'}]
    cur_b_type = parse_type(std::get<ASTNodePtr>(func_f_param->get_children().front()));
    std::string arg_name = std::get<TokenPtr>(func_f_param->get_children()[1])->get_content();
    auto arg_type = get_var_type(func_f_param->get_children().begin() + 2, func_f_param->get_children().end());

    // NOTE here, tne name of the argument is set to be orginal token name,
    // it would be rewritten when first entrying visit<BLOCK, BasicBlock, LIST>
    return std::make_shared<Argument>(arg_type, cur_function, arg_name);
}

template <>
std::list<std::shared_ptr<Argument>> Visitor::visit<NodeType::FUNC_F_PARAMS, Argument, LIST>(
    ASTNodePtr &func_f_params) {
    // FuncFParams -> FuncFParam {',' FuncFParam}
    std::list<std::shared_ptr<Argument>> result_list;

    // clang-format off
        func_f_params->for_each_child([this, &result_list](auto &&child) {
            std::visit(overloaded{
                    [this, &result_list](ASTNodePtr &node) {
                        result_list.push_back(visit<NodeType::FUNC_F_PARAM, Argument, SINGLE>(node));
                    },
                    [](TokenPtr &token) {
                        if (token->get_type() != token_type::TokenType::COMMA) {
                            logger::ERROR("[Visitor::visit] Unexpected token type: ", token->get_type(),
                                          " when visiting FUNC_F_PARAMS");
                            __builtin_unreachable();
                        }
                    }
            }, child);
        });
    // clang-format on

    return result_list;
}

template <>
std::shared_ptr<Function> Visitor::visit<NodeType::FUNC_DEF, Function, SINGLE>(ASTNodePtr &func_def) {
    // FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
    auto &children = func_def->get_children();

    // make a fake function type to initialize the function and insert to symbol table
    // fill the func type after construct the arguments
    const auto func_name = std::get<TokenPtr>(children[1])->get_content();
    cur_function = std::make_shared<Function>(PlaceholderType::get(), FUNCTION_PREFIX + func_name);
    symbol_table.insert(func_name, SymbolType::FUNC, cur_function);

    // visit arguments
    auto arguments = children.size() == 6
                         ? visit<NodeType::FUNC_F_PARAMS, Argument, LIST>(std::get<ASTNodePtr>(children[3]))
                         : std::list<std::shared_ptr<Argument>>();
    // get param types
    std::vector<std::shared_ptr<Type>> param_types;
    param_types.reserve(arguments.size());
    std::transform(
        arguments.begin(), arguments.end(), std::back_inserter(param_types), [](auto &arg) { return arg->get_type(); });
    const auto return_type = parse_type(std::get<ASTNodePtr>(children[0]));
    const auto func_type = FunctionType::get(return_type, param_types);
    cur_function->set_type(func_type);
    cur_function->set_arguments(std::vector<std::shared_ptr<Argument>>(arguments.begin(), arguments.end()));

    // enter a new scope
    symbol_table.enter_scope();
    cur_basic_block = std::make_shared<BasicBlock>(cur_function, gen_block_name());
    cur_function->add_basic_block(cur_function->get_basic_blocks_ref().end(), cur_basic_block);
    func_arg_init = false;

    // Block
    auto basic_block_list = visit<NodeType::BLOCK, BasicBlock, LIST>(std::get<ASTNodePtr>(children.back()));
    cur_function->add_basic_blocks(std::move(basic_block_list));

    // if the function has no return statement, add a return instruction
    if (cur_basic_block->get_instructions_ref().empty() ||
        !std::dynamic_pointer_cast<Ret>(cur_basic_block->get_instructions_ref().back())) {
        if (cur_function->get_return_type()->get_type_id() == Type::TypeID::VOID_ID) {
            cur_basic_block->add_instructions({Ret::create(cur_basic_block)});
        } else {
            if (cur_function->get_return_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
                cur_basic_block->add_instructions(
                    {Ret::create(cur_basic_block, std::make_shared<ConstantInt>(IntegerType::get(32), 0))});
            } else if (cur_function->get_return_type()->get_type_id() == Type::TypeID::FLOAT_ID) {
                cur_basic_block->add_instructions(
                    {Ret::create(cur_basic_block, std::make_shared<ConstantFloat>(FloatType::get(), 0.0f))});
            } else {
                logger::ERROR("[Visitor::visit<NodeType::FUNC_DEF, Function, SINGLE>] Unknown return type: ",
                              cur_function->get_return_type());
                __builtin_unreachable();
            }
        }
    }

    // exit scope
    symbol_table.exit_scope();

    return cur_function;
}

template <>
std::shared_ptr<ArrayValueWrapper<Constant>> Visitor::visit<NodeType::INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(
    ASTNodePtr &init_val) {
    // InitVal -> Exp | '{' [InitVal {',' InitVal}] '}'

    if (init_val->get_children().size() == 2) {
        return std::make_shared<ArrayValueWrapper<Constant>>(ArrayValueWrapper<Constant>::WrapperVec{});
    }

    if (init_val->get_children().size() > 2) {
        ArrayValueWrapper<Constant>::WrapperVec initializers;
        init_val->for_each_child([this, &initializers](auto &&child) {
            if (std::holds_alternative<ASTNodePtr>(child)) {
                initializers.push_back(
                    visit<NodeType::INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(std::get<ASTNodePtr>(child)));
            }
        });
        return std::make_shared<ArrayValueWrapper<Constant>>(std::move(initializers));
    }
    return std::make_shared<ArrayValueWrapper<Constant>>(
        visit<NodeType::EXP, Constant, SINGLE>(std::get<ASTNodePtr>(init_val->get_children()[0])));
}

template <>
std::shared_ptr<ArrayValueWrapper<Constant>>
Visitor::visit<NodeType::CONST_INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(ASTNodePtr &const_init_val) {
    // ConstInitVal -> ConstExp | '{' [ConstInitVal {',' ConstInitVal}] '}'

    if (const_init_val->get_children().size() == 2) {
        return std::make_shared<ArrayValueWrapper<Constant>>(ArrayValueWrapper<Constant>::WrapperVec{});
    }

    if (const_init_val->get_children().size() > 2) {
        ArrayValueWrapper<Constant>::WrapperVec initializers;
        const_init_val->for_each_child([this, &initializers](auto &&child) {
            if (std::holds_alternative<ASTNodePtr>(child)) {
                initializers.push_back(
                    visit<NodeType::CONST_INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(std::get<ASTNodePtr>(child)));
            }
        });
        return std::make_shared<ArrayValueWrapper<Constant>>(std::move(initializers));
    }
    return std::make_shared<ArrayValueWrapper<Constant>>(
        visit<NodeType::CONST_EXP, Constant, SINGLE>(std::get<ASTNodePtr>(const_init_val->get_children()[0])));
}

template <>
std::shared_ptr<GlobalVariable> Visitor::visit<NodeType::VAR_DEF, GlobalVariable, SINGLE>(ASTNodePtr &var_def) {
    // VarDef -> Ident {'[' ConstExp ']'} ['=' InitVal]

    // Ident
    auto ident_name = std::get<TokenPtr>(var_def->get_children()[0])->get_content();

    // type, wrapped by pointer
    int offset = std::holds_alternative<TokenPtr>(var_def->get_children().back()) ? 0 : -2;
    auto base_type = get_var_type(var_def->get_children().begin() + 1, var_def->get_children().end() + offset);
    auto type = PointerType::get(base_type);

    // InitVal
    bool has_init = var_def->get_children().size() > 1 &&
                    std::holds_alternative<TokenPtr>(var_def->get_children()[var_def->get_children().size() - 2]) &&
                    std::get<TokenPtr>(var_def->get_children()[var_def->get_children().size() - 2])->get_type() ==
                        token_type::TokenType::ASSIGN;
    auto init_val = has_init ? wrap_cast_to_const(visit<NodeType::INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(
                                                      std::get<ASTNodePtr>(var_def->get_children().back()))
                                                      ->format_as(base_type),
                                                  base_type)
                             : get_zero(base_type);

    // add to symbol table
    auto var_value = std::make_shared<GlobalVariable>(type, init_val, false, gen_global_var_name());
    symbol_table.insert(ident_name, SymbolType::VAR, var_value);

    return var_value;
}

template <>
std::shared_ptr<GlobalVariable> Visitor::visit<NodeType::CONST_DEF, GlobalVariable, SINGLE>(ASTNodePtr &const_def) {
    // ConstDef -> Ident {'[' ConstExp ']'} '=' ConstInitVal

    // Ident
    auto ident_name = std::get<TokenPtr>(const_def->get_children()[0])->get_content();

    // type
    auto base_type = get_var_type(const_def->get_children().begin() + 1, const_def->get_children().end() - 2);
    auto type = PointerType::get(base_type);

    // ConstInitVal
    auto const_init_val = wrap_cast_to_const(visit<NodeType::CONST_INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(
                                                 std::get<ASTNodePtr>(const_def->get_children().back()))
                                                 ->format_as(base_type),
                                             base_type);
    auto global_variable = std::make_shared<GlobalVariable>(type, const_init_val, true, gen_global_var_name());

    if (base_type->get_type_id() != Type::TypeID::ARRAY_ID) {
        // if it's not array, only insert init val to symbol tabel
        // TODO(Xingkun) is it necessary to collect const normal global var to module?
        symbol_table.insert(ident_name, SymbolType::CON, const_init_val);
    } else {
        symbol_table.insert(ident_name, SymbolType::CON, global_variable);
    }

    return global_variable;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::CONST_DEF, Instruction, LIST>(ASTNodePtr &const_def) {
    // ConstDef -> Ident {'[' ConstExp ']'} '=' ConstInitVal
    std::list<std::shared_ptr<Instruction>> ins_list;

    // Ident
    auto ident_name = std::get<TokenPtr>(const_def->get_children()[0])->get_content();

    // type
    int offset = std::holds_alternative<TokenPtr>(const_def->get_children().back()) ? 0 : -2;
    auto type = get_var_type(const_def->get_children().begin() + 1, const_def->get_children().end() + offset);

    // If is array, consider it as a normal variable, alloca mem for it.
    // Because when retrieving values from an array, index may not be constant.
    if (type->get_type_id() == Type::TypeID::ARRAY_ID) {
        // alloca memory
        auto alloca = Alloca::create(cur_basic_block, type, gen_local_var_name());
        ins_list.push_back(alloca);
        // insert to symbol table
        // TODO(Xingkun): whether to insert as var or con
        symbol_table.insert(ident_name, SymbolType::CON, alloca);

        // collect init array val
        auto [ori_arr_wrap, new_ins] = visit_array_init_val(std::get<ASTNodePtr>(const_def->get_children().back()));
        ins_list.splice(ins_list.end(), new_ins);
        auto init_val_wrap = ori_arr_wrap->format_as(type);
        // store for alloca
        std::vector<std::shared_ptr<Value>> indexes = {std::make_shared<ConstantInt>(IntegerType::get(32), 0)};
        ins_list.splice(ins_list.end(), store_init_val_to_array(init_val_wrap, alloca, indexes));
        return ins_list;
    }

    // Otherwise, only set init value to symbol table
    auto const_init_val =
        std::get<std::shared_ptr<Constant>>(visit<NodeType::CONST_INIT_VAL, ArrayValueWrapper<Constant>, SINGLE>(
                                                std::get<ASTNodePtr>(const_def->get_children().back()))
                                                ->val);
    symbol_table.insert(ident_name, SymbolType::CON, const_init_val);
    return {};
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::VAR_DEF, Instruction, LIST>(ASTNodePtr &var_def) {
    // VarDef -> Ident {'[' ConstExp ']'} ['=' InitVal]
    std::list<std::shared_ptr<Instruction>> ins_list;

    // Ident
    auto ident_name = std::get<TokenPtr>(var_def->get_children()[0])->get_content();

    // type
    int offset = std::holds_alternative<TokenPtr>(var_def->get_children().back()) ? 0 : -2;
    auto type = get_var_type(var_def->get_children().begin() + 1, var_def->get_children().end() + offset);

    // alloca for var
    auto alloca = Alloca::create(cur_basic_block, type, gen_local_var_name());
    ins_list.push_back(alloca);
    // insert to symbol table
    symbol_table.insert(ident_name, SymbolType::VAR, alloca);

    // if there's no init val, return here
    if (var_def->get_children().size() <= 1 ||
        !std::holds_alternative<TokenPtr>(var_def->get_children()[var_def->get_children().size() - 2]) ||
        std::get<TokenPtr>(var_def->get_children()[var_def->get_children().size() - 2])->get_type() !=
            token_type::TokenType::ASSIGN) {
        return ins_list;
    }

    // InitVal
    if (type->get_type_id() == Type::TypeID::ARRAY_ID) {
        // Array Init
        // collect init array val
        auto [ori_arr_wrap, new_ins] = visit_array_init_val(std::get<ASTNodePtr>(var_def->get_children().back()));
        ins_list.splice(ins_list.end(), new_ins);
        auto init_val_wrap = ori_arr_wrap->format_as(type);
        // store for alloca
        std::vector<std::shared_ptr<Value>> indexes = {std::make_shared<ConstantInt>(IntegerType::get(32), 0)};
        ins_list.splice(ins_list.end(), store_init_val_to_array(init_val_wrap, alloca, indexes));
        return ins_list;
    }

    // Normal init
    if (var_def->get_children().size() != 1) {
        auto [init_val, new_ins] = visit_exps<NodeType::EXP>(
            std::get<ASTNodePtr>(std::get<ASTNodePtr>(var_def->get_children().back())->get_children().front()));
        ins_list.splice(ins_list.end(), new_ins);
        // type conversion
        auto [actual_val, conv_ins] = make_collect_type_conversion(
            init_val, std::dynamic_pointer_cast<PointerType>(alloca->get_type())->get_reference_type());
        ins_list.splice(ins_list.end(), conv_ins);
        auto store = Store::create(cur_basic_block, actual_val, alloca, gen_local_var_name());
        ins_list.push_back(store);
    }

    return ins_list;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::VAR_DECL, Instruction, LIST>(ASTNodePtr &var_decl) {
    // VarDecl -> BType VarDef {',' VarDef} ';'
    std::list<std::shared_ptr<Instruction>> result_list;
    auto &children = var_decl->get_children();

    // BType
    const ASTNodePtr &b_type_node = std::get<ASTNodePtr>(children[0]);
    cur_b_type = parse_type(b_type_node);

    // VarDef
    for (size_t i = 1; i < children.size(); i += 2) {
        result_list.splice(result_list.end(),
                           visit<NodeType::VAR_DEF, Instruction, LIST>(std::get<ASTNodePtr>(children[i])));
    }
    return result_list;
}

template <>
std::list<std::shared_ptr<GlobalVariable>> Visitor::visit<NodeType::VAR_DECL, GlobalVariable, LIST>(
    ASTNodePtr &var_decl) {
    // VarDecl -> BType VarDef {',' VarDef} ';'
    std::list<std::shared_ptr<GlobalVariable>> result_list;
    auto &children = var_decl->get_children();

    // BType
    const ASTNodePtr &b_type_node = std::get<ASTNodePtr>(children[0]);
    cur_b_type = parse_type(b_type_node);

    // VarDef
    for (size_t i = 1; i < children.size(); i += 2) {
        result_list.push_back(visit<NodeType::VAR_DEF, GlobalVariable, SINGLE>(std::get<ASTNodePtr>(children[i])));
    }
    return result_list;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::CONST_DECL, Instruction, LIST>(
    ASTNodePtr &const_decl) {
    // ConstDecl -> 'const' BType ConstDef {',' ConstDef} ';'
    std::list<std::shared_ptr<Instruction>> result_list;
    auto &children = const_decl->get_children();

    // BType
    cur_b_type = parse_type(std::get<ASTNodePtr>(children[1]));

    // ConstDef
    for (size_t i = 2; i < children.size(); i += 2) {
        result_list.splice(result_list.end(),
                           visit<NodeType::CONST_DEF, Instruction, LIST>(std::get<ASTNodePtr>(children[i])));
    }

    return result_list;
}

template <>
std::list<std::shared_ptr<GlobalVariable>> Visitor::visit<NodeType::CONST_DECL, GlobalVariable, LIST>(
    ASTNodePtr &const_decl) {
    // ConstDecl -> 'const' BType ConstDef {',' ConstDef} ';'
    std::list<std::shared_ptr<GlobalVariable>> result_list;
    auto &children = const_decl->get_children();

    // BType
    cur_b_type = parse_type(std::get<ASTNodePtr>(children[1]));

    // ConstDef
    for (size_t i = 2; i < children.size(); i += 2) {
        result_list.push_back(visit<NodeType::CONST_DEF, GlobalVariable, SINGLE>(std::get<ASTNodePtr>(children[i])));
    }

    return result_list;
}

template <>
std::list<std::shared_ptr<Instruction>> Visitor::visit<NodeType::DECL, Instruction, LIST>(ASTNodePtr &decl) {
    // Decl -> ConstDecl | VarDecltype = {frontend::ast::NodeType} frontend::ast_type::NodeType::DECL
    std::list<std::shared_ptr<Instruction>> result_list;

    decl->for_each_child([this, &result_list](auto &&child) {
        std::visit(
            overloaded{
                [this, &result_list](ASTNodePtr &node) {
                    if (node->get_type() == NodeType::CONST_DECL) {
                        result_list.splice(result_list.end(), visit<NodeType::CONST_DECL, Instruction, LIST>(node));
                    } else if (node->get_type() == NodeType::VAR_DECL) {
                        result_list.splice(result_list.end(), visit<NodeType::VAR_DECL, Instruction, LIST>(node));
                    } else {
                        logger::ERROR("[Visitor::visit] unexpected node type ", node->get_type(), " in Decl");
                        __builtin_unreachable();
                    }
                },
                [](const TokenPtr &token) {
                    logger::ERROR("[Visitor::visit] unexpected token type ", token->get_type(), " in Decl");
                    __builtin_unreachable();
                }},
            child);
    });

    return result_list;
}

template <>
std::list<std::shared_ptr<GlobalVariable>> Visitor::visit<NodeType::DECL, GlobalVariable, LIST>(ASTNodePtr &decl) {
    // Decl -> ConstDecl | VarDecl

    // clang-format off
        std::list<std::shared_ptr<GlobalVariable>> result_list;
        decl->for_each_child([this, &result_list](auto &&child) {
            std::visit(overloaded{
                    [this, &result_list](ASTNodePtr &node) {
                        if (node->get_type() == NodeType::CONST_DECL) {
                            auto const_vars = visit<NodeType::CONST_DECL, GlobalVariable, LIST>(node);
                            result_list.splice(result_list.end(), const_vars);
                        } else if (node->get_type() == NodeType::VAR_DECL) {
                            auto var_vars = visit<NodeType::VAR_DECL, GlobalVariable, LIST>(node);
                            result_list.splice(result_list.end(), var_vars);
                        } else {
                            logger::ERROR("[Visitor::visit] unexpected node type ", node->get_type(), " in Decl");
                            __builtin_unreachable();
                        }
                    },
                    [](const TokenPtr &token) {
                        logger::ERROR("[Visitor::visit] unexpected token type ", token->get_type(), " in Decl");
                        __builtin_unreachable();
                    }
            }, child);
        });
        //clang-format on

        return result_list;
    }
}  // namespace frontend::visitor

// other helper function
namespace frontend::visitor {
    void Visitor::visit(const ASTNodePtr &comp_unit) {
        // Add lib funcs
        for (const auto &lib_func: Function::get_lib_funcs()) {
            symbol_table.insert(lib_func->get_name().substr(1), SymbolType::FUNC, lib_func);
        }

        // visit CompUnit
        comp_unit->for_each_child([this](auto &&child) {
            std::visit(overloaded{
                [this](ASTNodePtr &node) {
                    if (node->get_type() == NodeType::FUNC_DEF) {
                        module->add_function(visit<NodeType::FUNC_DEF, Function, SINGLE>(node));
                    } else if (node->get_type() == NodeType::DECL) {
                        module->add_global_variables(visit<NodeType::DECL, GlobalVariable, LIST>(node));
                    } else {
                        logger::ERROR("[Visitor::visit] node type", node->get_type(), " is invalid for CompUnit");
                        __builtin_unreachable();
                    }
                },
                [](TokenPtr &) {
                    logger::ERROR("[Visitor::visit] token is invalid for CompUnit");
                    __builtin_unreachable();
                }},
        child);
        });

#ifdef ENABLE_LOG
    auto content = module->to_string();
    auto path = logger::log_to_file("llvm.txt", content);
    logger::INFO("llvm is saved to ", path);
#endif
}

std::shared_ptr<Type> Visitor::parse_type(const ASTNodePtr &type_node) {
    if (type_node->get_type() != NodeType::B_TYPE && type_node->get_type() != NodeType::FUNC_TYPE) {
        logger::ERROR("[Visitor::parseType] Expected B_TYPE or FUNC_TYPE node, but got ", type_node->get_type());
    }

    switch (std::get<TokenPtr>(type_node->get_children().front())->get_type()) {
        case token_type::TokenType::INTTK:
            return IntegerType::get(32);
        case token_type::TokenType::FLOATTK:
            return FloatType::get();
        case token_type::TokenType::VOIDTK:
            return VoidType::get();
        default:
            logger::ERROR("unexpected token type ", std::get<TokenPtr>(type_node->get_children().front())->get_type());
            __builtin_unreachable();
    }
}

std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>, std::list<std::shared_ptr<Instruction>>>
Visitor::make_binary_type_conversion(const std::shared_ptr<Value> &left, const std::shared_ptr<Value> &right) {
    if (left->get_type() == right->get_type()) {
        return {left, right, {}};
    }
    if (left->get_type()->get_type_id() == Type::TypeID::INTEGER_ID &&
        right->get_type()->get_type_id() == Type::TypeID::FLOAT_ID) {
        auto sitofp = SIToFP::create(cur_basic_block, left, FloatType::get(), gen_local_var_name());
        return {sitofp, right, {sitofp}};
    }
    if (left->get_type()->get_type_id() == Type::TypeID::FLOAT_ID &&
        right->get_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
        auto sitofp = SIToFP::create(cur_basic_block, right, FloatType::get(), gen_local_var_name());
        return {left, sitofp, {sitofp}};
    }
    if (left->get_type() == IntegerType::get(32) && right->get_type() == IntegerType::get(1)) {
        auto zext = ZExt::create(cur_basic_block, right, IntegerType::get(32), gen_local_var_name());
        return {left, zext, {zext}};
    }

    if (left->get_type() == IntegerType::get(1) && right->get_type() == IntegerType::get(32)) {
        auto zext = ZExt::create(cur_basic_block, left, IntegerType::get(32), gen_local_var_name());
        return {zext, right, {zext}};
    }
    logger::ERROR("[Visitor::make_binary_type_conversion] Invalid type input. left: ",
                  *left->get_type(),
                  " right: ",
                  *right->get_type());
    __builtin_unreachable();
}

std::pair<std::shared_ptr<Value>, std::list<std::shared_ptr<Instruction>>> Visitor::make_collect_type_conversion(
    const std::shared_ptr<Value> &val, const std::shared_ptr<Type> &target_type) {
    std::list<std::shared_ptr<Instruction>> instructions;
    if (val->get_type() == target_type) {
        return {val, instructions};
    }
    if (target_type == IntegerType::get(1)) {
        std::shared_ptr<Instruction> value;
        if (val->get_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
            value = ICmp::create(cur_basic_block,
                                 ICmp::ICmpType::NE,
                                 val,
                                 std::make_shared<ConstantInt>(IntegerType::get(32), 0),
                                 gen_local_var_name());
        } else {
            value = FCmp::create(cur_basic_block,
                                 FCmp::FCmpType::ONE,
                                 val,
                                 std::make_shared<ConstantFloat>(FloatType::get(), 0.0F),
                                 gen_local_var_name());
        }
        instructions.push_back(value);
        return {value, instructions};
    }
    if (auto con = std::dynamic_pointer_cast<Constant>(val); con != nullptr) {
        if (target_type->get_type_id() == Type::TypeID::INTEGER_ID) {
            return {con->cast_to_int(), instructions};
        } else if (target_type->get_type_id() == Type::TypeID::FLOAT_ID) {
            return {con->cast_to_float(), instructions};
        } else {
            __builtin_unreachable();
        }
    }
    if (val->get_type()->get_type_id() == Type::TypeID::INTEGER_ID &&
        target_type->get_type_id() == Type::TypeID::FLOAT_ID) {
        auto sitofp = SIToFP::create(cur_basic_block, val, FloatType::get(), gen_local_var_name());
        instructions.push_back(sitofp);
        return {sitofp, instructions};
    }
    if (val->get_type()->get_type_id() == Type::TypeID::FLOAT_ID &&
        target_type->get_type_id() == Type::TypeID::INTEGER_ID) {
        auto fptosi =
            FPToSI::create(cur_basic_block, val, IntegerType::get(32), gen_local_var_name());
        instructions.push_back(fptosi);
        return {fptosi, instructions};
    }
    if (val->get_type()->get_type_id() == Type::TypeID::INTEGER_ID &&
        target_type->get_type_id() == Type::TypeID::INTEGER_ID) {
        auto zext = ZExt::create(cur_basic_block, val, IntegerType::get(32), gen_local_var_name());
        instructions.push_back(zext);
        return {zext, instructions};
    }
    __builtin_unreachable();
}

std::shared_ptr<Instruction> Visitor::get_icmp_create_func(token_type::TokenType op,
                                                           const std::shared_ptr<Value> &left,
                                                           const std::shared_ptr<Value> &right) {
    if (left->get_type()->get_type_id() == Type::TypeID::INTEGER_ID) {
        switch (op) {
            case token_type::TokenType::LSS:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::SLT, left, right, gen_local_var_name());
            case token_type::TokenType::LEQ:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::SLE, left, right, gen_local_var_name());
            case token_type::TokenType::GRE:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::SGT, left, right, gen_local_var_name());
            case token_type::TokenType::GEQ:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::SGE, left, right, gen_local_var_name());
            case token_type::TokenType::EQL:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::EQ, left, right, gen_local_var_name());
            case token_type::TokenType::NEQ:
                return ICmp::create(
                    cur_basic_block, ICmp::ICmpType::NE, left, right, gen_local_var_name());
            default:
                __builtin_unreachable();
        }
    }
    switch (op) {
        case token_type::TokenType::LSS:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::OLT, left, right, gen_local_var_name());
        case token_type::TokenType::LEQ:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::OLE, left, right, gen_local_var_name());
        case token_type::TokenType::GRE:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::OGT, left, right, gen_local_var_name());
        case token_type::TokenType::GEQ:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::OGE, left, right, gen_local_var_name());
        case token_type::TokenType::EQL:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::OEQ, left, right, gen_local_var_name());
        case token_type::TokenType::NEQ:
            return FCmp::create(
                cur_basic_block, FCmp::FCmpType::ONE, left, right, gen_local_var_name());
        default:
            __builtin_unreachable();
    }
}

std::pair<std::shared_ptr<ArrayValueWrapper<Value>>, std::list<std::shared_ptr<Instruction>>>
Visitor::visit_array_init_val(frontend::ast::ASTNodePtr &array_init_val) {
    // ConstInitVal -> ConstExp | '{' [ConstInitVal {',' ConstInitVal}] '}'
    // InitVal -> Exp | '{' [InitVal {',' InitVal}] '}'

    if (array_init_val->get_children().size() == 2) {
        return {std::make_shared<ArrayValueWrapper<Value>>(ArrayValueWrapper<Value>::WrapperVec{}), {}};
    }
    if (array_init_val->get_children().size() > 2) {
        std::list<std::shared_ptr<Instruction>> ins_list;
        ArrayValueWrapper<Value>::WrapperVec initializers;
        array_init_val->for_each_child([this, &ins_list, &initializers](auto &&child) {
            if (std::holds_alternative<ASTNodePtr>(child)) {
                auto [wrap, new_ins] = visit_array_init_val(std::get<ASTNodePtr>(child));
                initializers.push_back(wrap);
                ins_list.splice(ins_list.end(), new_ins);
            }
        });
        return {std::make_shared<ArrayValueWrapper<Value>>(initializers), ins_list};
    }
    if (array_init_val->get_type() == NodeType::CONST_INIT_VAL) {
        auto [tmp_val, new_ins] =
            visit_exps<NodeType::CONST_EXP>(std::get<ASTNodePtr>(array_init_val->get_children().front()));
        return {std::make_shared<ArrayValueWrapper<Value>>(tmp_val), new_ins};
    } else if (array_init_val->get_type() == NodeType::INIT_VAL) {
        auto [tmp_val, new_ins] =
            visit_exps<NodeType::EXP>(std::get<ASTNodePtr>(array_init_val->get_children().front()));
        return {std::make_shared<ArrayValueWrapper<Value>>(tmp_val), new_ins};
    } else {
        __builtin_unreachable();
    }
}

std::shared_ptr<Constant> Visitor::wrap_cast_to_const(const std::shared_ptr<ArrayValueWrapper<Constant>> &wrap,
                                                      const std::shared_ptr<Type> &arr_type) {
    if (wrap->is_single()) {
        return std::get<ArrayValueWrapper<Constant>::WrapperScalar>(wrap->val);
    }
    auto arr_vec = std::get<ArrayValueWrapper<Constant>::WrapperVec>(wrap->val);
    if (arr_vec.empty()) {
        return get_zero(arr_type);
    }
    std::vector<std::shared_ptr<Constant>> inits;
    inits.reserve(arr_vec.size());
    for (const auto &init : arr_vec) {
        inits.push_back(wrap_cast_to_const(init, std::dynamic_pointer_cast<ArrayType>(arr_type)->get_element_type()));
    }
    return std::make_shared<ConstantArray>(arr_type, std::move(inits));
}

std::list<std::shared_ptr<Instruction>> Visitor::store_init_val_to_array(
    const std::shared_ptr<ArrayValueWrapper<Value>> &wrapper,
    const std::shared_ptr<Value> &arr_ptr,
    std::vector<std::shared_ptr<Value>> &indexes) {
    std::list<std::shared_ptr<Instruction>> ins_list;
    if (wrapper->is_single()) {
        auto value = std::get<ArrayValueWrapper<Value>::WrapperScalar>(wrapper->val);
        if (std::dynamic_pointer_cast<ZeroInitializer>(value)) {
            // zero initializer, make all elements zero
            auto gep = Getelementptr::create(cur_basic_block, arr_ptr, indexes, gen_local_var_name());
            auto bitcast = Bitcast::create(
                cur_basic_block, gep, PointerType::get(IntegerType::get(8)), gen_local_var_name());
            auto memset = Memset::create(
                cur_basic_block, bitcast, 0, value->get_type()->bits_num() / 8, gen_local_var_name());
            ins_list.splice(ins_list.end(), {gep, bitcast, memset});
            return ins_list;
        }

        // generate gep to get addr
        auto gep = Getelementptr::create(cur_basic_block, arr_ptr, indexes, gen_local_var_name());
        ins_list.push_back(gep);

        // type conversion
        auto [collected_val, new_ins] = make_collect_type_conversion(
            value, std::dynamic_pointer_cast<PointerType>(gep->get_type())->get_reference_type());
        ins_list.splice(ins_list.end(), new_ins);

        // generate store instruction
        auto store = Store::create(cur_basic_block, collected_val, gep, gen_local_var_name());
        ins_list.push_back(store);
        return ins_list;
    }
    auto arr_wrap = std::get<ArrayValueWrapper<Value>::WrapperVec>(wrapper->val);
    for (size_t i = 0; i < arr_wrap.size(); ++i) {
        if (arr_wrap[i]->is_zero()) {
            // combine zero into memset
            size_t j = i;
            while (j < arr_wrap.size() && arr_wrap[j]->is_zero()) j++;
            if (j - i > 1) {
                indexes.push_back(std::make_shared<ConstantInt>(IntegerType::get(32), i));
                auto gep = Getelementptr::create(cur_basic_block, arr_ptr, indexes, gen_local_var_name());
                auto bitcast = Bitcast::create(cur_basic_block, gep, PointerType::get(IntegerType::get(8)), gen_local_var_name());
                auto memset = Memset::create(cur_basic_block, bitcast, 0, static_cast<int>(j - i) * arr_wrap[i]->bits_num() / 8, gen_local_var_name());
                ins_list.splice(ins_list.end(), {gep, bitcast, memset});
                indexes.pop_back();
                i = j - 1;
                continue;
            }
        }
        indexes.push_back(std::make_shared<ConstantInt>(IntegerType::get(32), i));
        ins_list.splice(ins_list.end(), store_init_val_to_array(arr_wrap[i], arr_ptr, indexes));
        indexes.pop_back();
    }
    return ins_list;
}

std::shared_ptr<Type> Visitor::get_var_type(GrammarNodeIterator begin, GrammarNodeIterator end) {
    auto res = cur_b_type;
    for (auto it = std::reverse_iterator(end); it != std::reverse_iterator(begin); ++it) {
        const auto &grammar_node = *it;
        if (!std::holds_alternative<TokenPtr>(grammar_node)) continue;
        if (std::get<TokenPtr>(grammar_node)->get_type() == token_type::TokenType::RBRACK) continue;
        if (const auto &exp = *(it - 1); std::holds_alternative<TokenPtr>(exp)) {
            res = PointerType::get(res);
        } else {
            res = ArrayType::get(
                res,
                visit<NodeType::CONST_EXP, Constant, SINGLE>(const_cast<ASTNodePtr &>(std::get<ASTNodePtr>(exp)))
                    ->cast_to_int()
                    ->get_val());
        }
    }
    return res;
}

}  // namespace frontend::visitor
