#pragma once

#include<vector>
#include<iostream>
#include <memory>

enum class ASTType
{
    TYPE_INT,
    TYPE_VOID,
    TYPE_FLOAT
};

enum RelOp {
  // <=
  OP_LTE,
  // <
  OP_LT,
  // >
  OP_GT,
  // >=
  OP_GTE,
  // ==
  OP_EQ,
  // !=
  OP_NEQ
};

enum AddOp {
  // +
  OP_PLUS,
  // -
  OP_MINUS
};

enum MulOp {
  // *
  OP_MUL,
  // /
  OP_DIV,
  // %
  OP_MOD
};

enum UnaryOp {
  // +
  OP_POS,
  // -
  OP_NEG,
  // !
  OP_NOT
};

enum LogOp {
  // &&
  OP_AND,
  // ||
  OP_OR,

};

struct ASTNode;
struct ASTCompUnit;
struct ASTDeclaration;
struct ASTConstDef;
struct ASTConstDeclaration;
struct ASTConstDefList;
struct ASTConstInitVal;
struct ASTVarDeclaration;
struct ASTVarDefList;
struct ASTVarDef;
struct ASTInitVal;
struct ASTInitValList;
struct ASTConstInitValList;
struct ASTFuncDef;
struct ASTFuncFParam;
struct ASTFuncFParamList;
struct ASTParamArrayExpList;
struct ASTBlock;
struct ASTBlockItemList;
struct ASTBlockItem;
struct ASTStmt;
struct ASTAssignStmt;
struct ASTExpStmt;
struct ASTBlockStmt;
struct ASTSelectStmt;
struct ASTIterationStmt;
struct ASTBreakStmt;
struct ASTContinueStmt;
struct ASTReturnStmt;
struct ASTExp;
struct ASTCond;
struct ASTLVal;
struct ASTArrayExpList;
struct ASTPrimaryExp;
struct ASTNumber;
struct ASTUnaryExp;
struct ASTCallee;
struct ASTParamExpList;
struct ASTMulExp;
struct ASTAddExp;
struct ASTRelExp;
struct ASTEqExp;
struct ASTLAndExp;
struct ASTLOrExp;
struct ASTConstExp;

class ASTVisitor;


struct ASTNode {
    int lineno;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor &) = 0;
};

struct ASTCompUnit : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTDeclaration>> Declaration_list;
};

struct ASTDeclaration : ASTNode{
    virtual void accept(ASTVisitor &) override;
    ASTType type;
};

struct ASTConstDeclaration : ASTDeclaration{
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTConstDef>> ConstDef_list;
};

struct ASTConstDefList{
    std::vector<std::shared_ptr<ASTConstDef>> ConstDef_list;
};

struct ASTConstDef : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> ConstExp_list;
    std::shared_ptr<ASTConstInitVal> ConstInitVal;
};

struct ASTArrayConstExpList{
    std::vector<std::shared_ptr<ASTConstExp>> ConstExp_list;
};

struct ASTConstInitVal : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstExp> ConstExp;
    std::vector<std::shared_ptr<ASTConstInitVal>> ConstInitVal_list;
};

struct ASTConstInitValList{
    std::vector<std::shared_ptr<ASTConstInitVal>> ConstInitVal_list;
};

struct ASTVarDeclaration : ASTDeclaration{
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTVarDef>> VarDef_list;
};

struct ASTVarDefList{
    std::vector<std::shared_ptr<ASTVarDef>> VarDef_list;
};

struct ASTVarDef : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> ConstExp_list;
    std::shared_ptr<ASTInitVal> InitVal;
};

struct ASTInitVal : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> Exp;
    std::vector<std::shared_ptr<ASTInitVal>>InitVal_list;
};

struct ASTInitValList{
    std::vector<std::shared_ptr<ASTInitVal>>InitVal_list;
};

struct ASTFuncDef : ASTDeclaration
{
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTFuncFParam>> FuncFParam_list;
    std::shared_ptr<ASTBlock> Block;
};

struct ASTFuncFParamList{
    std::vector<std::shared_ptr<ASTFuncFParam>> FuncFParam_list;
};

struct ASTFuncFParam : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    ASTType type;
    std::string id;
    bool is_array;
    std::vector<std::shared_ptr<ASTExp>> ParamArrayExp_list;
};

struct ASTParamArrayExpList{
    std::vector<std::shared_ptr<ASTExp>> ParamArrayExp_list;
};

struct ASTBlock : ASTNode
{
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTBlockItem>>BlockItem_list;
};

struct ASTBlockItemList {
    std::vector<std::shared_ptr<ASTBlockItem>>BlockItem_list;
};

struct ASTBlockItem : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstDeclaration> ConstDecl; 
    std::shared_ptr<ASTVarDeclaration> VarDecl;
    std::shared_ptr<ASTStmt> Stmt;
};

struct ASTStmt : ASTNode{
    virtual void accept(ASTVisitor &) override;
};

struct ASTAssignStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> LVal;
    std::shared_ptr<ASTExp> Exp;
};

struct ASTExpStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> Exp;
};

struct ASTBlockStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTBlock> Block;
};

struct ASTSelectStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> Cond;
    std::shared_ptr<ASTStmt> IfStmt;
    std::shared_ptr<ASTStmt> ElseStmt;
};

struct ASTIterationStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> Cond;
    std::shared_ptr<ASTStmt> Stmt;
};

struct ASTBreakStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
};

struct ASTContinueStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
};

struct ASTReturnStmt : ASTStmt{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> Exp;
};

struct ASTExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> AddExp;
};

struct ASTCond : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLOrExp> LOrExp;
};

struct ASTLVal : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> ArrayExp_list;
};

struct ASTArrayExpList{
    std::vector<std::shared_ptr<ASTExp>> ArrayExp_list;
};

struct ASTPrimaryExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> Exp;
    std::shared_ptr<ASTLVal> LVal;
    std::shared_ptr<ASTNumber> Number;
};

struct ASTNumber : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    ASTType type;
    union {
        int32_t i_val;
        float f_val;
    };
};

struct ASTUnaryExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTPrimaryExp> PrimaryExp;
    std::shared_ptr<ASTCallee> Callee;
    UnaryOp op;
    std::shared_ptr<ASTUnaryExp> UnaryExp;
};

struct ASTCallee : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> ParamExp_list;
};

struct ASTParamExpList{
    std::vector<std::shared_ptr<ASTExp>> ParamExp_list;
};

struct ASTMulExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    MulOp op;
    std::shared_ptr<ASTMulExp> MulExp;
    std::shared_ptr<ASTUnaryExp> UnaryExp;
};

struct ASTAddExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    AddOp op;
    std::shared_ptr<ASTMulExp> MulExp;
    std::shared_ptr<ASTAddExp> AddExp;
};

struct ASTRelExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    RelOp op;
    std::shared_ptr<ASTAddExp> AddExp;
    std::shared_ptr<ASTRelExp> RelExp;
};

struct ASTEqExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    RelOp op;
    std::shared_ptr<ASTRelExp> RelExp;
    std::shared_ptr<ASTEqExp> EqExp;
};

struct ASTLAndExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    LogOp op;
    std::shared_ptr<ASTEqExp> EqExp;
    std::shared_ptr<ASTLAndExp> LAndExp;
};

struct ASTLOrExp : ASTNode{
    virtual void accept(ASTVisitor &) override final;
    LogOp op;
    std::shared_ptr<ASTLAndExp> LAndExp;
    std::shared_ptr<ASTLOrExp> LOrExp;
};


struct ASTConstExp :ASTNode{
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> AddExp;
};




class ASTVisitor{
public:
    virtual void visit(ASTCompUnit &) = 0;
    virtual void visit(ASTConstDef &) = 0;
    virtual void visit(ASTConstDeclaration &) = 0;
    virtual void visit(ASTConstInitVal &) = 0;
    virtual void visit(ASTVarDeclaration &) = 0;
    virtual void visit(ASTVarDef &) = 0;
    virtual void visit(ASTInitVal &) = 0;
    virtual void visit(ASTFuncDef &) = 0;
    virtual void visit(ASTFuncFParam &) = 0;
    virtual void visit(ASTBlock &) = 0;
    virtual void visit(ASTAssignStmt &) = 0;
    virtual void visit(ASTExpStmt &) = 0;
    virtual void visit(ASTBlockStmt &) = 0;
    virtual void visit(ASTSelectStmt &) = 0;
    virtual void visit(ASTIterationStmt &) = 0;
    virtual void visit(ASTBreakStmt &) = 0;
    virtual void visit(ASTContinueStmt &) = 0;
    virtual void visit(ASTReturnStmt &) = 0;
    virtual void visit(ASTLVal &) = 0;
    virtual void visit(ASTNumber &) = 0;
    virtual void visit(ASTUnaryExp &) = 0;
    virtual void visit(ASTCallee &) = 0;
    virtual void visit(ASTMulExp &) = 0;
    virtual void visit(ASTAddExp &) = 0;
    virtual void visit(ASTRelExp &) = 0;
    virtual void visit(ASTEqExp &) = 0;
    virtual void visit(ASTLAndExp &) = 0;
    virtual void visit(ASTLOrExp &) = 0;
};

class ASTPrinter : public ASTVisitor{
public:
    virtual void visit(ASTCompUnit &) override final;
    virtual void visit(ASTConstDef &) override final;
    virtual void visit(ASTConstDeclaration &) override final;
    virtual void visit(ASTConstInitVal &) override final;
    virtual void visit(ASTVarDeclaration &) override final;
    virtual void visit(ASTVarDef &) override final;
    virtual void visit(ASTInitVal &) override final;
    virtual void visit(ASTFuncDef &) override final;
    virtual void visit(ASTFuncFParam &) override final;
    virtual void visit(ASTBlock &) override final;
    virtual void visit(ASTAssignStmt &) override final;
    virtual void visit(ASTExpStmt &) override final;
    virtual void visit(ASTBlockStmt &) override final;
    virtual void visit(ASTSelectStmt &) override final;
    virtual void visit(ASTIterationStmt &) override final;
    virtual void visit(ASTBreakStmt &) override final;
    virtual void visit(ASTContinueStmt &) override final;
    virtual void visit(ASTReturnStmt &) override final;
    virtual void visit(ASTLVal &) override final;
    virtual void visit(ASTNumber &) override final;
    virtual void visit(ASTUnaryExp &) override final;
    virtual void visit(ASTCallee &) override final;
    virtual void visit(ASTMulExp &) override final;
    virtual void visit(ASTAddExp &) override final;
    virtual void visit(ASTRelExp &) override final;
    virtual void visit(ASTEqExp &) override final;
    virtual void visit(ASTLAndExp &) override final;
    virtual void visit(ASTLOrExp &) override final;
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }
private:
    int depth = 0;
};

class InitZeroJudger : public ASTVisitor{
public:
    virtual void visit(ASTCompUnit &) override final;
    virtual void visit(ASTConstDef &) override final;
    virtual void visit(ASTConstDeclaration &) override final;
    virtual void visit(ASTConstInitVal &) override final;
    virtual void visit(ASTVarDeclaration &) override final;
    virtual void visit(ASTVarDef &) override final;
    virtual void visit(ASTInitVal &) override final;
    virtual void visit(ASTFuncDef &) override final;
    virtual void visit(ASTFuncFParam &) override final;
    virtual void visit(ASTBlock &) override final;
    virtual void visit(ASTAssignStmt &) override final;
    virtual void visit(ASTExpStmt &) override final;
    virtual void visit(ASTBlockStmt &) override final;
    virtual void visit(ASTSelectStmt &) override final;
    virtual void visit(ASTIterationStmt &) override final;
    virtual void visit(ASTContinueStmt &) override final;
    virtual void visit(ASTBreakStmt &) override final;
    virtual void visit(ASTReturnStmt &) override final;
    virtual void visit(ASTLVal &) override final;
    virtual void visit(ASTNumber &) override final;
    virtual void visit(ASTUnaryExp &) override final;
    virtual void visit(ASTCallee &) override final;
    virtual void visit(ASTMulExp &) override final;
    virtual void visit(ASTAddExp &) override final;
    virtual void visit(ASTRelExp &) override final;
    virtual void visit(ASTEqExp &) override final;
    virtual void visit(ASTLAndExp &) override final;
    virtual void visit(ASTLOrExp &) override final;

    void reset() { all_zero = true; }
    bool is_all_zero() { return all_zero; }

private:
    bool all_zero;
};
