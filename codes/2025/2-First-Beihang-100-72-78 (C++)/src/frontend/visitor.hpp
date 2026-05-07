#ifndef GEECEECEE_FRONTEND_VISITOR_HPP
#define GEECEECEE_FRONTEND_VISITOR_HPP

#include <list>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "ir/mod.hpp"

namespace frontend::symbol_table {
enum class SymbolType { CON, VAR, FUNC };
class SymbolTable {
    using SymbolContent = std::pair<std::shared_ptr<ir::Value>, SymbolType>;

public:
    void enter_scope();
    void exit_scope();
    void insert(std::string name, SymbolType type, const std::shared_ptr<ir::Value> &val) const;
    [[nodiscard]] std::optional<SymbolContent> lookup(std::string name);
    void remove(const std::string &name) const;

    SymbolTable() { enter_scope(); }

private:
    /*
     * vector: layer. specifically, the 0th layer is the global scope.
     * map: symbol name -> symbol result
     */
    std::vector<std::unique_ptr<std::unordered_map<std::string, SymbolContent>>> scopes;
};
}  // namespace frontend::symbol_table

namespace frontend::visitor {
using namespace ast;
using namespace ir;
using namespace symbol_table;

using VisitExpsResult = std::pair<std::shared_ptr<Value>, std::list<std::shared_ptr<Instruction>>>;

class Visitor {
public:
    explicit Visitor(Module *module) : module(module) {}
    void visit(const ASTNodePtr &comp_unit);
    enum class VisitPattern { SINGLE, LIST };

private:
    // Visit a specific ASTNode type and return a value.
    // `node_type`: The type of the ASTNode to visit.
    // `value`: The type of the value returned by the visitor. It must be a type that inherits from `Value`.
    // `is_list`: A flag indicating whether the returned value is a list.
    template <NodeType node_type, typename value, VisitPattern pattern>
    [[nodiscard]] auto visit(ASTNodePtr &) -> std::enable_if_t<
        std::is_base_of_v<Value, value>,
        std::conditional_t<pattern == VisitPattern::LIST, std::list<std::shared_ptr<value>>, std::shared_ptr<value>>> {
        logger::ERROR("[Visitor::visit<", node_type, ">] method not implemented");
        __builtin_unreachable();
    }

    template <NodeType node_type>
    [[nodiscard]] VisitExpsResult visit_exps(ASTNodePtr &) {
        logger::ERROR("[Visitor::visit_exps<", node_type, ">] method not implemented");
        __builtin_unreachable();
    }

    static std::shared_ptr<Type> parse_type(const ASTNodePtr &type_node);

    // An assistant method to convert the given `left` and `right` Value to the same type so that executing binary
    // operation. Return the converted Value and a list of Instructions
    [[nodiscard]] std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>, std::list<std::shared_ptr<Instruction>>>
    make_binary_type_conversion(const std::shared_ptr<Value> &left, const std::shared_ptr<Value> &right);
    std::shared_ptr<Instruction> get_icmp_create_func(token_type::TokenType op,
                                                      const std::shared_ptr<Value> &left,
                                                      const std::shared_ptr<Value> &right);

    // An assistant method to convert the given `val` to the same type as `target_type` so that executing
    // operation on it. Return the converted Value and a list (actually one or none) of Instructions
    [[nodiscard]] std::pair<std::shared_ptr<Value>, std::list<std::shared_ptr<Instruction>>>
    make_collect_type_conversion(const std::shared_ptr<Value> &val, const std::shared_ptr<Type> &target_type);

    // calculate the type of a variable, based on `cur_b_type`
    // pass an iterator range of `GrammarNode`
    std::shared_ptr<Type> get_var_type(GrammarNodeIterator begin, GrammarNodeIterator end);

    // visit ConstInitVal or InitVal, return the ArrayWrapper and a list of Instructions
    std::pair<std::shared_ptr<ArrayValueWrapper<Value>>, std::list<std::shared_ptr<Instruction>>> visit_array_init_val(
        ASTNodePtr &array_init_val);

    // Store the InitVal or ConstInitVal to array
    std::list<std::shared_ptr<Instruction>> store_init_val_to_array(
        const std::shared_ptr<ArrayValueWrapper<Value>> &wrapper,
        const std::shared_ptr<Value> &arr_ptr,
        std::vector<std::shared_ptr<Value>> &indexes);

    // convert an ArrayValueWrapper<Constant> to standard Constant format
    inline static std::shared_ptr<Constant> wrap_cast_to_const(const std::shared_ptr<ArrayValueWrapper<Constant>> &wrap,
                                                               const std::shared_ptr<Type> &arr_type);

private:
    Module *module;
    SymbolTable symbol_table;
    std::shared_ptr<BasicBlock> cur_basic_block;
    std::shared_ptr<Function> cur_function;
    std::list<std::shared_ptr<BasicBlock>> true_basic_block_list;
    std::list<std::shared_ptr<BasicBlock>> false_basic_block_list;
    std::list<std::shared_ptr<BasicBlock>> loop_entry_blocks;
    std::list<std::shared_ptr<BasicBlock>> loop_exit_blocks;
    // TODO: change to stack
    std::shared_ptr<Type> cur_b_type;
    bool func_arg_init = false;
    bool in_array_arg = false;
    std::stack<bool> is_left;  // determine whether to load out value when visti LVal
};
}  // namespace frontend::visitor

#endif
