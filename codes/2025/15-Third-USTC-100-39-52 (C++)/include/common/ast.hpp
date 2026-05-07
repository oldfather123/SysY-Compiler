//m

#pragma once

// extern "C"
// {
// #include "../common/syntax_tree.h"
//     extern syntax_tree *parse(const char *input);
// }
#include "../common/syntax_tree.h"
    extern syntax_tree *parse(const char *input);
#include "../lightir/User.hpp"
#include <memory>
#include <string>
#include <vector>

enum SysyType
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID,
    TYPE_INT_ARRAY,
    TYPE_FLOAT_ARRAY
};

enum UnaryOp
{
    // +
    OP_PLUS,
    // -
    OP_MINUS,
    // !
    OP_NOT
};

enum MulOp
{
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD
};

enum AddOp
{
    // +
    OP_ADD,
    // -
    OP_SUB
};

enum RelOp
{
    // <
    OP_LT,
    // >
    OP_GT,
    // <=
    OP_LE,
    // >=
    OP_GE
};

enum EqOp
{
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

enum LOGICOP
{
    // &&
    OP_AND,
    // ||
    OP_OR
};

enum DeclKind {
    Const,
    Var,
    Func
};
class AST;

struct ASTNode;
struct ASTProgram;
struct ASTUnit;   // 包含main和其他单元
struct ASTDecl;
struct ASTConstDef;
struct ASTVarDef;
struct ASTInitVal;
// struct ASTExpression;
struct ASTExp;
struct ASTFuncDef;
// struct ASTMainDef;
struct ASTParam;
struct ASTBlock;
struct ASTStmt;
struct ASTAssignStmt;
struct ASTCompoundAssignStmt;
struct ASTSelectionStmt;
struct ASTIterationStmt;
struct ASTJumpStmt; //new
struct ASTReturnStmt;
struct ASTBreak;
struct ASTContinue;
struct ASTAddExp;
struct ASTMulExp;
struct ASTRelExp;
struct ASTEqExp;
struct ASTAndExp;
struct ASTOrExp;
struct ASTNum;
struct ASTUnaryExp;
struct ASTPrimaryExp;
struct ASTFuncExp;
struct ASTVar;

class ASTVisitor;

class AST
{
public:
    AST() = delete;
    AST(syntax_tree *);
    AST(AST &&tree)
    {
        root = tree.root;
        tree.root = nullptr;
    };
    ASTProgram *get_root() { return root.get(); }
    void run_visitor(ASTVisitor &visitor);

public:
    std::shared_ptr<ASTNode> transform_node_iter(syntax_tree_node *);
    std::shared_ptr<ASTProgram> root = nullptr;
};
// AST结点基本类型
struct ASTNode: public std::enable_shared_from_this<ASTNode>
{
    virtual Value *accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
};
// 开始结点类型 对应产生式 program -> CompUnit
struct ASTProgram : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTProgram() = default;
    std::vector<std::shared_ptr<ASTUnit>> compunits;
};
// CompUnit 对应AST结点
struct ASTUnit : ASTNode
{
    virtual ~ASTUnit() = default;
    SysyType type;
};
// Decl 对应AST结点类型，由一个域decl_kind区分constDecl和VarDecl和FuncDecl
struct ASTDecl : ASTUnit
{
    virtual Value *accept(ASTVisitor &) override final;
    DeclKind decl_kind;
    std::vector<std::shared_ptr<ASTConstDef>> cdef_lists;
    std::vector<std::shared_ptr<ASTVarDef>> vdef_lists;
    std::string func_name;
    std::vector<std::shared_ptr<ASTParam>> params;
};
// FuncDef 对应的AST结点类型
// FuncDef -> (BType|void) { FuncFParams } Block
struct ASTFuncDef : ASTUnit
{
    virtual Value *accept(ASTVisitor &) override final;
    SysyType type;
    std::string id;
    std::vector<std::shared_ptr<ASTParam>> params;
    std::shared_ptr<ASTBlock> block;
};
// struct ASTMainDef : ASTUnit
// {
//     virtual Value *accept(ASTVisitor &) override final;
//     SysyType type;
//     std::string id;
//     std::shared_ptr<ASTBlock> block;
// };
// Def(包括constDef和VarDef)对应的结点类型
struct ASTConstDef : ASTUnit
{
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTConstDef() = default;
    std::string id;
    bool is_const;
    bool is_array;
    unsigned length;
    
    SysyType type;
    // 数组下标
    std::vector<std::shared_ptr<ASTExp>> exp_lists;
    // 初始化列表
    std::shared_ptr<ASTInitVal> initval_list;
};
struct ASTVarDef : ASTUnit
{
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTVarDef() = default;
    std::string id;
    bool is_const;
    bool is_array;
    unsigned length;
    SysyType type;
    // 数组下标
    std::vector<std::shared_ptr<ASTExp>> exp_lists;
    // 初始化列表
    std::shared_ptr<ASTInitVal> initval_list;
};
struct ASTInitVal : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    virtual ~ASTInitVal() = default;
    bool is_const;
    bool is_singlevalue;
    std::vector<std::shared_ptr<ASTInitVal>> initval_list;
    std::shared_ptr<ASTExp> value;
};
// 函数形参对应AST
struct ASTParam : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    SysyType type;
    std::string id;
    // true if it is at least 1-dimension array
    bool isarray;
    // 超过1维的其他维度的大小
    std::vector<std::shared_ptr<ASTExp>> array_lists;
};
// Stmt 对应的ASTNode
struct ASTStmt : ASTNode
{
    virtual ~ASTStmt() = default;
};
// 以下是各种statement对应的类型
struct ASTBlock : ASTStmt
{
    virtual Value *accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTStmt>> stmt_lists;
    std::vector<std::shared_ptr<ASTDecl>> decl_lists;
    std::vector<int> list_type;
};
struct ASTCompoundAssignStmt : ASTStmt   //new
{
    std::shared_ptr<ASTVar> var;
    std::shared_ptr<ASTExp> expression;
    std::string op;  // "+=", "-=", "*=", "/="
    virtual Value *accept(ASTVisitor &) override final;
};
struct ASTAssignStmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTVar> var;
    std::shared_ptr<ASTExp> expression;
};
// Stmt -> if ( Cond ) stmt [else stmt]
struct ASTSelectionStmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTOrExp> cond; // TODO:
    std::shared_ptr<ASTStmt> if_stmt;
    std::shared_ptr<ASTStmt> else_stmt;
};
// Stmt -> while ( Cond ) Stmt
struct ASTIterationStmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTOrExp> cond; // TODO:
    std::shared_ptr<ASTStmt> stmt;
};
struct ASTJumpStmt : ASTStmt {    //new
    virtual ~ASTJumpStmt() = default;
};

struct ASTBreak : ASTJumpStmt {
    virtual Value *accept(ASTVisitor &) override final;
    bool is_break;
};

struct ASTContinue : ASTJumpStmt {
    virtual Value *accept(ASTVisitor &) override final;
    bool is_continue;
};

struct ASTReturnStmt : ASTJumpStmt {
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> expression;
};
// Exp
struct ASTPrimaryExp : ASTNode
{
    virtual ~ASTPrimaryExp() = default;
};
union const_val
{
    unsigned int i_val;
    float f_val;
};
struct ASTExp : ASTPrimaryExp
{
    virtual Value *accept(ASTVisitor &) override final;
    bool is_const;
    const_val val;

    std::shared_ptr<ASTAddExp> add_exp;
};
// LVal
struct ASTVar : ASTPrimaryExp
{
    virtual Value *accept(ASTVisitor &) override final;
    std::string id;
    unsigned length;
    std::vector<std::shared_ptr<ASTExp>> array_lists;
};
// Number
struct ASTNum : ASTPrimaryExp
{
    virtual Value *accept(ASTVisitor &) override final;
    SysyType type;
    union
    {
        int i_val;
        float f_val;
    };
};
// UnaryExp
struct ASTUnaryExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    UnaryOp unary_op;
    std::shared_ptr<ASTUnaryExp> unary_exp;
    std::shared_ptr<ASTFuncExp> func_exp;
    std::shared_ptr<ASTPrimaryExp> primary_exp;
};
// FuncExp
struct ASTFuncExp : ASTPrimaryExp
{
    virtual Value *accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> args;
};
// AddExp
struct ASTAddExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> add_exp;
    AddOp op;
    std::shared_ptr<ASTMulExp> mul_exp;
};
// MulExp
struct ASTMulExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTMulExp> mul_exp;
    MulOp op;
    std::shared_ptr<ASTUnaryExp> unary_exp;
};
struct ASTRelExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTRelExp> rel_exp;
    RelOp op;
    std::shared_ptr<ASTAddExp> add_exp;
};
struct ASTEqExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTEqExp> eq_exp;
    EqOp op;
    std::shared_ptr<ASTRelExp> rel_exp;
};
struct ASTAndExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAndExp> land_exp;
    LOGICOP and_op;
    std::shared_ptr<ASTEqExp> eq_exp;
};
struct ASTOrExp : ASTNode
{
    virtual Value *accept(ASTVisitor &) override final;
    std::shared_ptr<ASTOrExp> lor_exp;
    LOGICOP or_op;
    std::shared_ptr<ASTAndExp> land_exp;
};

class ASTVisitor
{
public:
    virtual Value *visit(ASTProgram &) = 0;
    virtual Value *visit(ASTFuncDef &) = 0;
    // virtual Value *visit(ASTMainDef &) = 0;
    virtual Value *visit(ASTDecl &) = 0;
    virtual Value *visit(ASTConstDef &) = 0;
    virtual Value *visit(ASTVarDef &) = 0;
    virtual Value *visit(ASTParam &) = 0;
    virtual Value *visit(ASTBlock &) = 0;
    virtual Value *visit(ASTAssignStmt &) = 0;
    virtual Value *visit(ASTCompoundAssignStmt &) = 0;
    virtual Value *visit(ASTSelectionStmt &) = 0;
    virtual Value *visit(ASTIterationStmt &) = 0;
    virtual Value *visit(ASTBreak &) = 0;
    virtual Value *visit(ASTContinue &) = 0;
    virtual Value *visit(ASTReturnStmt &) = 0;
    virtual Value *visit(ASTExp &) = 0;
    virtual Value *visit(ASTVar &) = 0;
    virtual Value *visit(ASTNum &) = 0;
    virtual Value *visit(ASTUnaryExp &) = 0;
    virtual Value *visit(ASTFuncExp &) = 0;
    virtual Value *visit(ASTAddExp &) = 0;
    virtual Value *visit(ASTMulExp &) = 0;
    virtual Value *visit(ASTRelExp &) = 0;
    virtual Value *visit(ASTEqExp &) = 0;
    virtual Value *visit(ASTAndExp &) = 0;
    virtual Value *visit(ASTOrExp &) = 0;
    virtual Value *visit(ASTInitVal &) = 0;
};

class ASTPrinter : public ASTVisitor
{
public:
    virtual Value *visit(ASTProgram &) override final;
    virtual Value *visit(ASTFuncDef &) override final;
    // virtual Value *visit(ASTMainDef &) override final;
    virtual Value *visit(ASTDecl &) override final;
    virtual Value *visit(ASTConstDef &) override final;
    virtual Value *visit(ASTVarDef &) override final;
    virtual Value *visit(ASTParam &) override final;
    virtual Value *visit(ASTBlock &) override final;
    virtual Value *visit(ASTAssignStmt &) override final;
    virtual Value *visit(ASTCompoundAssignStmt &) override final;
    virtual Value *visit(ASTSelectionStmt &) override final;
    virtual Value *visit(ASTIterationStmt &) override final;
    virtual Value *visit(ASTBreak &) override final;
    virtual Value *visit(ASTContinue &) override final;
    virtual Value *visit(ASTReturnStmt &) override final;
    virtual Value *visit(ASTExp &) override final;
    virtual Value *visit(ASTVar &) override final;
    virtual Value *visit(ASTNum &) override final;
    virtual Value *visit(ASTUnaryExp &) override final;
    virtual Value *visit(ASTFuncExp &) override final;
    virtual Value *visit(ASTAddExp &) override final;
    virtual Value *visit(ASTMulExp &) override final;
    virtual Value *visit(ASTRelExp &) override final;
    virtual Value *visit(ASTEqExp &) override final;
    virtual Value *visit(ASTAndExp &) override final;
    virtual Value *visit(ASTOrExp &) override final;
    virtual Value *visit(ASTInitVal &) override final;
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }

private:
    int depth = 0;
};