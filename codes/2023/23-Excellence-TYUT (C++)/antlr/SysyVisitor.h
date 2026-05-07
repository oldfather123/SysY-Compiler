
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyParser.h"


/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysyParser.
 */
class SysyVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

    /**
     * Visit parse trees produced by SysyParser.
     */
    virtual std::any visitCompUnit(SysyParser::CompUnitContext *context) = 0;

    virtual std::any visitDecl(SysyParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysyParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysyParser::BTypeContext *context) = 0;

    virtual std::any visitConstDef(SysyParser::ConstDefContext *context) = 0;

    virtual std::any visitScalarConstInitVal(SysyParser::ScalarConstInitValContext *context) = 0;

    virtual std::any visitListConstInitVal(SysyParser::ListConstInitValContext *context) = 0;

    virtual std::any visitVarDecl(SysyParser::VarDeclContext *context) = 0;

    virtual std::any visitUninitVarDef(SysyParser::UninitVarDefContext *context) = 0;

    virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *context) = 0;

    virtual std::any visitScalarInitVal(SysyParser::ScalarInitValContext *context) = 0;

    virtual std::any visitListInitval(SysyParser::ListInitvalContext *context) = 0;

    virtual std::any visitFuncDef(SysyParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysyParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysyParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlock(SysyParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysyParser::BlockItemContext *context) = 0;

    virtual std::any visitAssignment(SysyParser::AssignmentContext *context) = 0;

    virtual std::any visitExpStmt(SysyParser::ExpStmtContext *context) = 0;

    virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *context) = 0;

    virtual std::any visitIfStmt1(SysyParser::IfStmt1Context *context) = 0;

    virtual std::any visitIfStmt2(SysyParser::IfStmt2Context *context) = 0;

    virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *context) = 0;

    virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *context) = 0;

    virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *context) = 0;

    virtual std::any visitExp(SysyParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysyParser::CondContext *context) = 0;

    virtual std::any visitLVal(SysyParser::LValContext *context) = 0;

    virtual std::any visitPrimaryExp1(SysyParser::PrimaryExp1Context *context) = 0;

    virtual std::any visitPrimaryExp2(SysyParser::PrimaryExp2Context *context) = 0;

    virtual std::any visitPrimaryExp3(SysyParser::PrimaryExp3Context *context) = 0;

    virtual std::any visitNunmberFloat(SysyParser::NunmberFloatContext *context) = 0;

    virtual std::any visitNumberInt(SysyParser::NumberIntContext *context) = 0;

    virtual std::any visitUnaryExp1(SysyParser::UnaryExp1Context *context) = 0;

    virtual std::any visitUnaryExp2(SysyParser::UnaryExp2Context *context) = 0;

    virtual std::any visitUnaryExp3(SysyParser::UnaryExp3Context *context) = 0;

    virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *context) = 0;

    virtual std::any visitExpParam(SysyParser::ExpParamContext *context) = 0;

    virtual std::any visitStringParam(SysyParser::StringParamContext *context) = 0;

    virtual std::any visitMulExp2(SysyParser::MulExp2Context *context) = 0;

    virtual std::any visitMulExp1(SysyParser::MulExp1Context *context) = 0;

    virtual std::any visitAddExp2(SysyParser::AddExp2Context *context) = 0;

    virtual std::any visitAddExp1(SysyParser::AddExp1Context *context) = 0;

    virtual std::any visitRelExp2(SysyParser::RelExp2Context *context) = 0;

    virtual std::any visitRelExp1(SysyParser::RelExp1Context *context) = 0;

    virtual std::any visitEqExp2(SysyParser::EqExp2Context *context) = 0;

    virtual std::any visitEqExp1(SysyParser::EqExp1Context *context) = 0;

    virtual std::any visitLAndExp1(SysyParser::LAndExp1Context *context) = 0;

    virtual std::any visitLAndExp2(SysyParser::LAndExp2Context *context) = 0;

    virtual std::any visitLOrExp2(SysyParser::LOrExp2Context *context) = 0;

    virtual std::any visitLOrExp1(SysyParser::LOrExp1Context *context) = 0;

    virtual std::any visitConstExp(SysyParser::ConstExpContext *context) = 0;


};

