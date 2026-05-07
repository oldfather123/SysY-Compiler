#pragma once

#include "Value.hpp"
#include "syntax_tree.h"
extern syntax_tree *parse(const char *input);
#include "User.hpp"
#include <memory>
#include <string>
#include <vector>
#include <variant>


struct ConstStruct{
    int type; // 1 int 2 float 3 arrayint 4 arrayflaot
    std::vector <int> dim; // if array, dimesion is in here
    std::vector <int> DataI; // store confirmed data in array
    std::vector <float> DataF;
    union{
        int ConstInt;
        float ConstFp;
    }SingleData;
};

enum SysyType { TYPE_INT, TYPE_FLOAT, TYPE_VOID };

enum UnaryOp {
    // +
    OP_POS,
    // -
    OP_NEG,
    // !
    OP_NOT
};

enum BinaryOP {
    // +
    OP_PLUS,
    // -
    OP_MINUS,
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD,
    // <
    OP_LT,
    // <=
    OP_LE,
    // >
    OP_GT,
    // >=
    OP_GE,
    // ==
    OP_EQ,
    // !=
    OP_NEQ,
    // &&
    OP_AND,
    // ||
    OP_OR
};

class AST;

struct ASTNode;
struct ASTProgram;
struct ASTConstDecl;
struct ASTConstDef;
struct ASTConstExp;
struct ASTVarDecl;
struct ASTVarDef;
struct ASTConstInitVal;
struct ASTInitVal;
struct ASTExp;
struct ASTBlock;
struct ASTFuncDef;
struct ASTFParam;
struct ASTStmt;
struct ASTLVal;
struct ASTCond;
struct ASTConstExp;
struct ASTExpStmt;
struct ASTAssignStmt;
struct ASTBlockStmt;
struct ASTSelectionStmt;
struct ASTIterationStmt;
struct ASTReturnStmt;
struct ASTBreakStmt;
struct ASTContinueStmt;
struct ASTNumber;
struct ASTUnaryExp;
struct ASTBinaryExp;

class ASTVisitor;

class AST {
  public:
    AST() = delete;
    AST(syntax_tree *);
    AST(AST &&tree) {
        root = tree.root;
        tree.root = nullptr;
    };
    ASTProgram *get_root() { return root.get(); }
    void run_visitor(ASTVisitor &visitor);

  private:
    std::shared_ptr<ASTNode> transform_node_iter(syntax_tree_node *);
    std::shared_ptr<ASTProgram> root = nullptr;
};

struct ASTNode : public std::enable_shared_from_this<ASTNode>{
    virtual Value *accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
    bool is_func_def() {
        return std::dynamic_pointer_cast<ASTFuncDef>(shared_from_this()) != nullptr;
    }
    bool is_const_decl() {
        return std::dynamic_pointer_cast<ASTConstDecl>(shared_from_this()) != nullptr;
    }
    bool is_var_decl() {
        return std::dynamic_pointer_cast<ASTVarDecl>(shared_from_this()) != nullptr;
    }
    bool is_stmt() {
        return std::dynamic_pointer_cast<ASTStmt>(shared_from_this()) != nullptr;
    }
    bool is_decl() {
        return is_const_decl() || is_var_decl();
    }
};

struct ASTProgram : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTProgram() = default;
    std::vector<std::variant<
            std::shared_ptr<ASTFuncDef>,
            std::shared_ptr<ASTConstDecl>,
            std::shared_ptr<ASTVarDecl>>> defs_and_decls;
};

struct ASTConstDecl : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDecl() = default;
    std::vector<std::shared_ptr<ASTConstDef>> const_defs;
};

struct ASTConstDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDef() = default;
    bool is_array;
    SysyType type;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> array_size;
    // actually if it is an array, the vector should have size larger than 0
    std::shared_ptr<ASTConstInitVal> init_val;
};

struct ASTConstInitVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstInitVal() = default;
    bool is_single_exp;
    std::vector<std::shared_ptr<ASTConstInitVal>> init_vals;
    std::shared_ptr<ASTConstExp> const_exp;
};

struct ASTVarDecl : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDecl() = default;
    std::vector<std::shared_ptr<ASTVarDef>> var_defs;
};

struct ASTVarDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDef() = default;
    bool is_array;
    SysyType type;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> array_size;
    std::shared_ptr<ASTInitVal> init_val;
};

struct ASTInitVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTInitVal() = default;
    bool is_single_exp;
    std::vector<std::shared_ptr<ASTInitVal>> init_vals;
    std::shared_ptr<ASTExp> exp;
};

struct ASTFuncDef : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFuncDef() = default;
    SysyType type;
    std::string id;
    std::vector<std::shared_ptr<ASTFParam>> params;
    std::shared_ptr<ASTBlock> block;
};

struct ASTFParam : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTFParam() = default;
    SysyType type;
    std::string id;
    bool is_array;
    std::vector<std::shared_ptr<ASTExp>> array_size;
    // attention that the first dimension of array is not included in the array size
};

struct ASTBlock : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBlock() = default;
    std::vector<std::variant<
            std::shared_ptr<ASTVarDecl>,
            std::shared_ptr<ASTConstDecl>,
            std::shared_ptr<ASTStmt>>> decls_and_stmts;
};

struct ASTStmt : ASTNode {
    virtual Value *accept(ASTVisitor &) override = 0;
    virtual ~ASTStmt() = default;
    virtual bool is_assign_stmt() { return false; }
    virtual bool is_exp_stmt() { return false; }
    virtual bool is_block() { return false; }
    virtual bool is_selection_stmt() { return false; }
    virtual bool is_iteration_stmt() { return false; }
    virtual bool is_return_stmt() { return false; }
    virtual bool is_break_stmt() { return false; }
    virtual bool is_continue_stmt() { return false; }
};

struct ASTExpStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTExpStmt() = default;
    bool is_empty;
    std::shared_ptr<ASTExp> exp;
};

struct ASTAssignStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTAssignStmt() = default;
    virtual bool is_assign_stmt() override final { return true; }
    std::shared_ptr<ASTLVal> l_val;
    std::shared_ptr<ASTExp> exp;
};

struct ASTBlockStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBlockStmt() = default;
    virtual bool is_block() override final { return true; }
    std::shared_ptr<ASTBlock> block;
};

struct ASTSelectionStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTSelectionStmt() = default;
    virtual bool is_selection_stmt() override final { return true; }
    bool has_else;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> if_stmt;
    std::shared_ptr<ASTStmt> else_stmt;
};

struct ASTIterationStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTIterationStmt() = default;
    virtual bool is_iteration_stmt() override final { return true; }
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> stmt;
};

struct ASTReturnStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTReturnStmt() = default;
    virtual bool is_return_stmt() override final { return true; }
    bool is_empty;
    std::shared_ptr<ASTExp> exp;
};

struct ASTBreakStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBreakStmt() = default;
    virtual bool is_break_stmt() override final { return true; }
};

struct ASTContinueStmt : ASTStmt {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTContinueStmt() = default;
    virtual bool is_continue_stmt() override final { return true; }
};

struct ASTLVal : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTLVal() = default;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> array_exp;
};

struct ASTNumber : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTNumber() = default;
    SysyType type;
    std::variant<int, float> value;
};

struct ASTExp : ASTNode {
    virtual Value *accept(ASTVisitor &) override = 0;
    virtual ~ASTExp() = default;
    virtual bool is_unary_exp() { return false; }
    virtual bool is_binary_exp() { return false; }
};

struct ASTUnaryExp : ASTExp {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTUnaryExp() = default;
    ASTUnaryExp()
        : exp(nullptr), l_val(nullptr), number(nullptr), func_call_id(""), has_unary_op(false) {}
    virtual bool is_unary_exp() override final { return true; }
    UnaryOp op;
    std::shared_ptr<ASTExp> exp;
    std::shared_ptr<ASTLVal> l_val;
    std::shared_ptr<ASTNumber> number;
    std::string func_call_id;
    std::vector<std::shared_ptr<ASTExp>> func_call_args;
    bool has_unary_op;
    bool is_l_val(){return l_val != nullptr;}
    bool is_number(){return number != nullptr;}
    bool is_func_call(){return func_call_id != "";}
};

struct ASTBinaryExp : ASTExp {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTBinaryExp() = default;
    virtual bool is_binary_exp() override final { return true; }
    std::shared_ptr<ASTExp> lhs;
    std::shared_ptr<ASTExp> rhs;
    BinaryOP op;
    bool is_add_exp() { return op == OP_PLUS || op == OP_MINUS; }
    bool is_mul_exp() { return op == OP_MUL || op == OP_DIV || op == OP_MOD; }
    bool is_rel_exp() { return op == OP_LT || op == OP_LE || op == OP_GT || op == OP_GE; }
    bool is_eq_exp() { return op == OP_EQ || op == OP_NEQ; }
    bool is_logic_and_exp() { return op == OP_AND; }
    bool is_logic_or_exp() { return op == OP_OR; }
    bool is_logic_exp() { return is_logic_and_exp() || is_logic_or_exp(); }
};

struct ASTCond : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTCond() = default;
    std::shared_ptr<ASTExp> exp;
};

struct ASTConstExp : ASTNode {
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstExp() = default;
    SysyType type;
    std::shared_ptr<ASTExp> exp;
};

class ASTVisitor {
  public:
    virtual Value *visit(ASTProgram &) = 0;
    virtual Value *visit(ASTConstDecl &) = 0;
    virtual Value *visit(ASTConstDef &) = 0;
    virtual Value *visit(ASTConstInitVal &) = 0;
    virtual Value *visit(ASTVarDecl &) = 0;
    virtual Value *visit(ASTVarDef &) = 0;
    virtual Value *visit(ASTInitVal &) = 0;
    virtual Value *visit(ASTFuncDef &) = 0;
    virtual Value *visit(ASTFParam &) = 0;
    virtual Value *visit(ASTBlock &) = 0;
    virtual Value *visit(ASTExpStmt &) = 0;
    virtual Value *visit(ASTAssignStmt &) = 0;
    virtual Value *visit(ASTBlockStmt &) = 0;
    virtual Value *visit(ASTSelectionStmt &) = 0;
    virtual Value *visit(ASTIterationStmt &) = 0;
    virtual Value *visit(ASTReturnStmt &) = 0;
    virtual Value *visit(ASTBreakStmt &) = 0;
    virtual Value *visit(ASTContinueStmt &) = 0;
    virtual Value *visit(ASTLVal &) = 0;
    virtual Value *visit(ASTNumber &) = 0;
    virtual Value *visit(ASTUnaryExp &) = 0;
    virtual Value *visit(ASTBinaryExp &) = 0;
    virtual Value *visit(ASTConstExp &) = 0;
    virtual Value *visit(ASTCond &) = 0;
};

class ASTPrinter : public ASTVisitor {
  public:
    virtual Value *visit(ASTProgram &) override final;
    virtual Value *visit(ASTConstDecl &) override final;
    virtual Value *visit(ASTConstDef &) override final;
    virtual Value *visit(ASTConstInitVal &) override final;
    virtual Value *visit(ASTVarDecl &) override final;
    virtual Value *visit(ASTVarDef &) override final;
    virtual Value *visit(ASTInitVal &) override final;
    virtual Value *visit(ASTFuncDef &) override final;
    virtual Value *visit(ASTFParam &) override final;
    virtual Value *visit(ASTBlock &) override final;
    virtual Value *visit(ASTExpStmt &) override final;
    virtual Value *visit(ASTAssignStmt &) override final;
    virtual Value *visit(ASTBlockStmt &) override final;
    virtual Value *visit(ASTSelectionStmt &) override final;
    virtual Value *visit(ASTIterationStmt &) override final;
    virtual Value *visit(ASTReturnStmt &) override final;
    virtual Value *visit(ASTBreakStmt &) override final;
    virtual Value *visit(ASTContinueStmt &) override final;
    virtual Value *visit(ASTLVal &) override final;
    virtual Value *visit(ASTNumber &) override final;
    virtual Value *visit(ASTUnaryExp &) override final;
    virtual Value *visit(ASTBinaryExp &) override final;
    virtual Value *visit(ASTConstExp &) override final;
    virtual Value *visit(ASTCond &) override final;
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }

  private:
    int depth = 0;
};
