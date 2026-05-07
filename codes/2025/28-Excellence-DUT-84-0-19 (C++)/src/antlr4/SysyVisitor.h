
// Generated from Sysy.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SysyParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysyParser.
 */
class  SysyVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysyParser.
   */
    virtual std::any visitCompUnit(SysyParser::CompUnitContext *context) = 0;

    virtual std::any visitCompUnitItem(SysyParser::CompUnitItemContext *context) = 0;

    virtual std::any visitDecl(SysyParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysyParser::ConstDeclContext *context) = 0;

    virtual std::any visitInt(SysyParser::IntContext *context) = 0;

    virtual std::any visitFloat(SysyParser::FloatContext *context) = 0;

    virtual std::any visitConstDef(SysyParser::ConstDefContext *context) = 0;

    virtual std::any visitVarDecl(SysyParser::VarDeclContext *context) = 0;

    virtual std::any visitVarDef(SysyParser::VarDefContext *context) = 0;

    virtual std::any visitInit(SysyParser::InitContext *context) = 0;

    virtual std::any visitInitList(SysyParser::InitListContext *context) = 0;

    virtual std::any visitFuncDef(SysyParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType_(SysyParser::FuncType_Context *context) = 0;

    virtual std::any visitVoid(SysyParser::VoidContext *context) = 0;

    virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *context) = 0;

    virtual std::any visitScalarParam(SysyParser::ScalarParamContext *context) = 0;

    virtual std::any visitArrayParam(SysyParser::ArrayParamContext *context) = 0;

    virtual std::any visitBlock(SysyParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysyParser::BlockItemContext *context) = 0;

    virtual std::any visitAssign(SysyParser::AssignContext *context) = 0;

    virtual std::any visitExprStmt(SysyParser::ExprStmtContext *context) = 0;

    virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *context) = 0;

    virtual std::any visitIfElse(SysyParser::IfElseContext *context) = 0;

    virtual std::any visitWhile(SysyParser::WhileContext *context) = 0;

    virtual std::any visitBreak(SysyParser::BreakContext *context) = 0;

    virtual std::any visitContinue(SysyParser::ContinueContext *context) = 0;

    virtual std::any visitReturn(SysyParser::ReturnContext *context) = 0;

    virtual std::any visitExp(SysyParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysyParser::CondContext *context) = 0;

    virtual std::any visitLVal(SysyParser::LValContext *context) = 0;

    virtual std::any visitPrimaryExp_(SysyParser::PrimaryExp_Context *context) = 0;

    virtual std::any visitLValExpr(SysyParser::LValExprContext *context) = 0;

    virtual std::any visitDecIntConst(SysyParser::DecIntConstContext *context) = 0;

    virtual std::any visitOctIntConst(SysyParser::OctIntConstContext *context) = 0;

    virtual std::any visitHexIntConst(SysyParser::HexIntConstContext *context) = 0;

    virtual std::any visitDecFloatConst(SysyParser::DecFloatConstContext *context) = 0;

    virtual std::any visitHexFloatConst(SysyParser::HexFloatConstContext *context) = 0;

    virtual std::any visitNumber(SysyParser::NumberContext *context) = 0;

    virtual std::any visitUnaryExp_(SysyParser::UnaryExp_Context *context) = 0;

    virtual std::any visitCall(SysyParser::CallContext *context) = 0;

    virtual std::any visitUnaryAdd(SysyParser::UnaryAddContext *context) = 0;

    virtual std::any visitUnarySub(SysyParser::UnarySubContext *context) = 0;

    virtual std::any visitNot(SysyParser::NotContext *context) = 0;

    virtual std::any visitStringConst(SysyParser::StringConstContext *context) = 0;

    virtual std::any visitFuncRParam(SysyParser::FuncRParamContext *context) = 0;

    virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *context) = 0;

    virtual std::any visitDiv(SysyParser::DivContext *context) = 0;

    virtual std::any visitMod(SysyParser::ModContext *context) = 0;

    virtual std::any visitMul(SysyParser::MulContext *context) = 0;

    virtual std::any visitMulExp_(SysyParser::MulExp_Context *context) = 0;

    virtual std::any visitAddExp_(SysyParser::AddExp_Context *context) = 0;

    virtual std::any visitAdd(SysyParser::AddContext *context) = 0;

    virtual std::any visitSub(SysyParser::SubContext *context) = 0;

    virtual std::any visitGeq(SysyParser::GeqContext *context) = 0;

    virtual std::any visitLt(SysyParser::LtContext *context) = 0;

    virtual std::any visitRelExp_(SysyParser::RelExp_Context *context) = 0;

    virtual std::any visitLeq(SysyParser::LeqContext *context) = 0;

    virtual std::any visitGt(SysyParser::GtContext *context) = 0;

    virtual std::any visitNeq(SysyParser::NeqContext *context) = 0;

    virtual std::any visitEq(SysyParser::EqContext *context) = 0;

    virtual std::any visitEqExp_(SysyParser::EqExp_Context *context) = 0;

    virtual std::any visitLAndExp_(SysyParser::LAndExp_Context *context) = 0;

    virtual std::any visitAnd(SysyParser::AndContext *context) = 0;

    virtual std::any visitOr(SysyParser::OrContext *context) = 0;

    virtual std::any visitLOrExp_(SysyParser::LOrExp_Context *context) = 0;


};

