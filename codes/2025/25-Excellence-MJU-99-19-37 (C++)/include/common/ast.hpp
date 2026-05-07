#pragma once

#include <cstdarg>
extern "C" {
#include "syntax_tree.h"
extern syntax_tree *parse(const char *input);
}
#include "User.hpp"
#include <memory>
#include <string>
#include <vector>

// 基本类型
enum SysYType { TYPE_INT, TYPE_FLOAT, TYPE_VOID };

// 关系运算符
enum RelOp {
    OP_LT,   // <
    OP_LE,   // <=
    OP_GT,   // >
    OP_GE,   // >=
    OP_EQ,   // ==
    OP_NE    // !=
};

// 加减运算符
enum AddOp {
    OP_PLUS,  // +
    OP_MINUS  // -
};

// 乘除模运算符
enum MulOp {
    OP_MUL,   // *
    OP_DIV,   // /
    OP_MOD    // %
};

// 一元运算符
enum UnaryOp {
    UOP_PLUS,  // +
    UOP_MINUS, // -
    UOP_NOT    // !
};

class AST;

// 前向声明
struct ASTNode;
struct ASTProgram;
struct ASTCompUnit;
struct ASTDecl;
struct ASTConstDecl;
struct ASTConstDef;
struct ASTConstInitVal;
struct ASTVarDecl;
struct ASTVarDef;
struct ASTInitVal;
struct ASTFuncDef;
struct ASTFuncFParam;
struct ASTBlock;
struct ASTBlockItem;
struct ASTStmt;
struct ASTExp;
struct ASTCond;
struct ASTLVal;
struct ASTNumber;
struct ASTPrimaryExp;
struct ASTUnaryExp;
struct ASTFuncRParams;
struct ASTMulExp;
struct ASTAddExp;
struct ASTRelExp;
struct ASTEqExp;
struct ASTLAndExp;
struct ASTLOrExp;
struct ASTConstExp;

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
    ASTNode *transform_node_iter(syntax_tree_node *);
    std::shared_ptr<ASTProgram> root = nullptr;
};

// 基类
struct ASTNode {
    virtual Value* accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
};

// Program节点
struct ASTProgram : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTCompUnit>> compUnits;
};

// 编译单元基类
// 【修改】：使用虚继承解决钻石继承问题
struct ASTCompUnit : virtual ASTNode {
    virtual ~ASTCompUnit() = default;
};

// 语句块项基类
// 【修改】：使用虚继承解决钻石继承问题
struct ASTBlockItem : virtual ASTNode {
    virtual ~ASTBlockItem() = default;
};

// 声明基类 - 既是CompUnit也是BlockItem
struct ASTDecl : ASTCompUnit, ASTBlockItem {
    virtual ~ASTDecl() = default;
    // 需要明确指定accept方法
    virtual Value* accept(ASTVisitor &) override = 0;
};

// 常量声明
struct ASTConstDecl : ASTDecl {
    virtual Value* accept(ASTVisitor &) override final;
    SysYType bType;
    std::vector<std::shared_ptr<ASTConstDef>> constDefs;
};

// 常量定义
struct ASTConstDef : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::string ident;
    std::vector<std::shared_ptr<ASTConstExp>> dims; // 数组维度
    std::shared_ptr<ASTConstInitVal> constInitVal;
};

// 常量初始值
struct ASTConstInitVal : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstExp> constExp; // 单个值
    std::vector<std::shared_ptr<ASTConstInitVal>> constInitVals; // 数组初始化
};

// 变量声明
struct ASTVarDecl : ASTDecl {
    virtual Value* accept(ASTVisitor &) override final;
    SysYType bType;
    std::vector<std::shared_ptr<ASTVarDef>> varDefs;
};

// 变量定义
struct ASTVarDef : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::string ident;
    std::vector<std::shared_ptr<ASTConstExp>> dims; // 数组维度
    std::shared_ptr<ASTInitVal> initVal; // 可选的初始值
};

// 变量初始值
struct ASTInitVal : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp; // 单个值
    std::vector<std::shared_ptr<ASTInitVal>> initVals; // 数组初始化
};

// 函数定义
struct ASTFuncDef : ASTCompUnit {
    virtual Value* accept(ASTVisitor &) override final;
    SysYType funcType;
    std::string ident;
    std::vector<std::shared_ptr<ASTFuncFParam>> params;
    std::shared_ptr<ASTBlock> block;
};

// 函数形参
struct ASTFuncFParam : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    SysYType bType;
    std::string ident;
    bool isArray;
    std::vector<std::shared_ptr<ASTExp>> dims; // 数组维度（第一维为空）
};

// 语句块
struct ASTBlock : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTBlockItem>> blockItems;
};

// 语句基类
struct ASTStmt : ASTBlockItem {
    virtual ~ASTStmt() = default;
};

// 赋值语句
struct ASTAssignStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> lVal;
    std::shared_ptr<ASTExp> exp;
};

// 表达式语句
struct ASTExpStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp; // 可以为空
};

// 块语句
struct ASTBlockStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTBlock> block;
};

// If语句
struct ASTIfStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> thenStmt;
    std::shared_ptr<ASTStmt> elseStmt; // 可选
};

// While语句
struct ASTWhileStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> stmt;
};

// Break语句
struct ASTBreakStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
};

// Continue语句
struct ASTContinueStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
};

// Return语句
struct ASTReturnStmt : ASTStmt {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp; // 可选
};

// 表达式
struct ASTExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> addExp;
};

// 条件表达式
struct ASTCond : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLOrExp> lOrExp;
};

// 左值表达式
struct ASTLVal : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::string ident;
    std::vector<std::shared_ptr<ASTExp>> indices; // 数组下标
};

// 基本表达式
struct ASTPrimaryExp : ASTNode {
    virtual ~ASTPrimaryExp() = default;
};

// 括号表达式
struct ASTParenExp : ASTPrimaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;
};

// 左值作为基本表达式
struct ASTLValExp : ASTPrimaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> lVal;
};

// 数字
struct ASTNumber : ASTPrimaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    bool isInt;
    union {
        int intVal;
        float floatVal;
    };
};

// 一元表达式
struct ASTUnaryExp : ASTNode {
    virtual ~ASTUnaryExp() = default;
};

// 基本表达式作为一元表达式
struct ASTPrimaryUnaryExp : ASTUnaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTPrimaryExp> primaryExp;
};

// 函数调用
struct ASTCallExp : ASTUnaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    std::string ident;
    std::shared_ptr<ASTFuncRParams> funcRParams; // 可选
};

// 一元运算表达式
struct ASTUnaryOpExp : ASTUnaryExp {
    virtual Value* accept(ASTVisitor &) override final;
    UnaryOp unaryOp;
    std::shared_ptr<ASTUnaryExp> unaryExp;
};

// 函数实参
struct ASTFuncRParams : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTExp>> exps;
};

// 乘除模表达式
struct ASTMulExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTMulExp> mulExp; // 可选，左操作数
    MulOp mulOp;
    std::shared_ptr<ASTUnaryExp> unaryExp;
};

// 加减表达式
struct ASTAddExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> addExp; // 可选，左操作数
    AddOp addOp;
    std::shared_ptr<ASTMulExp> mulExp;
};

// 关系表达式
struct ASTRelExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTRelExp> relExp; // 可选，左操作数
    RelOp relOp;
    std::shared_ptr<ASTAddExp> addExp;
};

// 相等性表达式
struct ASTEqExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTEqExp> eqExp; // 可选，左操作数
    RelOp eqOp; // EQ or NE
    std::shared_ptr<ASTRelExp> relExp;
};

// 逻辑与表达式
struct ASTLAndExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLAndExp> lAndExp; // 可选，左操作数
    std::shared_ptr<ASTEqExp> eqExp;
};

// 逻辑或表达式
struct ASTLOrExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLOrExp> lOrExp; // 可选，左操作数
    std::shared_ptr<ASTLAndExp> lAndExp;
};

// 常量表达式
struct ASTConstExp : ASTNode {
    virtual Value* accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> addExp;
};

// 访问者接口
class ASTVisitor {
  public:
    virtual Value* visit(ASTProgram &) = 0;
    virtual Value* visit(ASTConstDecl &) = 0;
    virtual Value* visit(ASTConstDef &) = 0;
    virtual Value* visit(ASTConstInitVal &) = 0;
    virtual Value* visit(ASTVarDecl &) = 0;
    virtual Value* visit(ASTVarDef &) = 0;
    virtual Value* visit(ASTInitVal &) = 0;
    virtual Value* visit(ASTFuncDef &) = 0;
    virtual Value* visit(ASTFuncFParam &) = 0;
    virtual Value* visit(ASTBlock &) = 0;
    virtual Value* visit(ASTAssignStmt &) = 0;
    virtual Value* visit(ASTExpStmt &) = 0;
    virtual Value* visit(ASTBlockStmt &) = 0;
    virtual Value* visit(ASTIfStmt &) = 0;
    virtual Value* visit(ASTWhileStmt &) = 0;
    virtual Value* visit(ASTBreakStmt &) = 0;
    virtual Value* visit(ASTContinueStmt &) = 0;
    virtual Value* visit(ASTReturnStmt &) = 0;
    virtual Value* visit(ASTExp &) = 0;
    virtual Value* visit(ASTCond &) = 0;
    virtual Value* visit(ASTLVal &) = 0;
    virtual Value* visit(ASTParenExp &) = 0;
    virtual Value* visit(ASTLValExp &) = 0;
    virtual Value* visit(ASTNumber &) = 0;
    virtual Value* visit(ASTPrimaryUnaryExp &) = 0;
    virtual Value* visit(ASTCallExp &) = 0;
    virtual Value* visit(ASTUnaryOpExp &) = 0;
    virtual Value* visit(ASTFuncRParams &) = 0;
    virtual Value* visit(ASTMulExp &) = 0;
    virtual Value* visit(ASTAddExp &) = 0;
    virtual Value* visit(ASTRelExp &) = 0;
    virtual Value* visit(ASTEqExp &) = 0;
    virtual Value* visit(ASTLAndExp &) = 0;
    virtual Value* visit(ASTLOrExp &) = 0;
    virtual Value* visit(ASTConstExp &) = 0;
};

// AST打印器
class ASTPrinter : public ASTVisitor {
  public:
    virtual Value* visit(ASTProgram &) override final;
    virtual Value* visit(ASTConstDecl &) override final;
    virtual Value* visit(ASTConstDef &) override final;
    virtual Value* visit(ASTConstInitVal &) override final;
    virtual Value* visit(ASTVarDecl &) override final;
    virtual Value* visit(ASTVarDef &) override final;
    virtual Value* visit(ASTInitVal &) override final;
    virtual Value* visit(ASTFuncDef &) override final;
    virtual Value* visit(ASTFuncFParam &) override final;
    virtual Value* visit(ASTBlock &) override final;
    virtual Value* visit(ASTAssignStmt &) override final;
    virtual Value* visit(ASTExpStmt &) override final;
    virtual Value* visit(ASTBlockStmt &) override final;
    virtual Value* visit(ASTIfStmt &) override final;
    virtual Value* visit(ASTWhileStmt &) override final;
    virtual Value* visit(ASTBreakStmt &) override final;
    virtual Value* visit(ASTContinueStmt &) override final;
    virtual Value* visit(ASTReturnStmt &) override final;
    virtual Value* visit(ASTExp &) override final;
    virtual Value* visit(ASTCond &) override final;
    virtual Value* visit(ASTLVal &) override final;
    virtual Value* visit(ASTParenExp &) override final;
    virtual Value* visit(ASTLValExp &) override final;
    virtual Value* visit(ASTNumber &) override final;
    virtual Value* visit(ASTPrimaryUnaryExp &) override final;
    virtual Value* visit(ASTCallExp &) override final;
    virtual Value* visit(ASTUnaryOpExp &) override final;
    virtual Value* visit(ASTFuncRParams &) override final;
    virtual Value* visit(ASTMulExp &) override final;
    virtual Value* visit(ASTAddExp &) override final;
    virtual Value* visit(ASTRelExp &) override final;
    virtual Value* visit(ASTEqExp &) override final;
    virtual Value* visit(ASTLAndExp &) override final;
    virtual Value* visit(ASTLOrExp &) override final;
    virtual Value* visit(ASTConstExp &) override final;
    
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }

  private:
    int depth = 0;
};