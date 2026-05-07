
// Generated from SysY.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SysYParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysYParser.
 */
class  SysYVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysYParser.
   */
    virtual std::any visitCompUnit(SysYParser::CompUnitContext *context) = 0;

    virtual std::any visitGlobalConstDecl(SysYParser::GlobalConstDeclContext *context) = 0;

    virtual std::any visitGlobalVarDecl(SysYParser::GlobalVarDeclContext *context) = 0;

    virtual std::any visitDecl(SysYParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysYParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysYParser::BTypeContext *context) = 0;

    virtual std::any visitConstDef(SysYParser::ConstDefContext *context) = 0;

    virtual std::any visitConstScalarInitValue(SysYParser::ConstScalarInitValueContext *context) = 0;

    virtual std::any visitConstArrayInitValue(SysYParser::ConstArrayInitValueContext *context) = 0;

    virtual std::any visitVarDecl(SysYParser::VarDeclContext *context) = 0;

    virtual std::any visitVarDef(SysYParser::VarDefContext *context) = 0;

    virtual std::any visitScalarInitValue(SysYParser::ScalarInitValueContext *context) = 0;

    virtual std::any visitArrayInitValue(SysYParser::ArrayInitValueContext *context) = 0;

    virtual std::any visitFuncType(SysYParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncDef(SysYParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncFParams(SysYParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysYParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlockStmt(SysYParser::BlockStmtContext *context) = 0;

    virtual std::any visitBlockItem(SysYParser::BlockItemContext *context) = 0;

    virtual std::any visitAssignStmt(SysYParser::AssignStmtContext *context) = 0;

    virtual std::any visitExpStmt(SysYParser::ExpStmtContext *context) = 0;

    virtual std::any visitBlkStmt(SysYParser::BlkStmtContext *context) = 0;

    virtual std::any visitIfStmt(SysYParser::IfStmtContext *context) = 0;

    virtual std::any visitWhileStmt(SysYParser::WhileStmtContext *context) = 0;

    virtual std::any visitBreakStmt(SysYParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(SysYParser::ContinueStmtContext *context) = 0;

    virtual std::any visitReturnStmt(SysYParser::ReturnStmtContext *context) = 0;

    virtual std::any visitExp(SysYParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysYParser::CondContext *context) = 0;

    virtual std::any visitLValue(SysYParser::LValueContext *context) = 0;

    virtual std::any visitPrimaryExp(SysYParser::PrimaryExpContext *context) = 0;

    virtual std::any visitNumber(SysYParser::NumberContext *context) = 0;

    virtual std::any visitCall(SysYParser::CallContext *context) = 0;

    virtual std::any visitUnaryExp(SysYParser::UnaryExpContext *context) = 0;

    virtual std::any visitUnaryOp(SysYParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysYParser::FuncRParamsContext *context) = 0;

    virtual std::any visitString(SysYParser::StringContext *context) = 0;

    virtual std::any visitMulExp(SysYParser::MulExpContext *context) = 0;

    virtual std::any visitAddExp(SysYParser::AddExpContext *context) = 0;

    virtual std::any visitRelExp(SysYParser::RelExpContext *context) = 0;

    virtual std::any visitEqExp(SysYParser::EqExpContext *context) = 0;

    virtual std::any visitLAndExp(SysYParser::LAndExpContext *context) = 0;

    virtual std::any visitLOrExp(SysYParser::LOrExpContext *context) = 0;

    virtual std::any visitConstExp(SysYParser::ConstExpContext *context) = 0;


};

