
// Generated from SysY.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "../include/SysYParser.h"



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

    virtual std::any visitDecl(SysYParser::DeclContext *context) = 0;

    virtual std::any visitBtype(SysYParser::BtypeContext *context) = 0;

    virtual std::any visitVarDef(SysYParser::VarDefContext *context) = 0;

    virtual std::any visitInitValue(SysYParser::InitValueContext *context) = 0;

    virtual std::any visitFuncDecl(SysYParser::FuncDeclContext *context) = 0;

    virtual std::any visitFuncDef(SysYParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysYParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysYParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysYParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlockStmt(SysYParser::BlockStmtContext *context) = 0;

    virtual std::any visitBlockItem(SysYParser::BlockItemContext *context) = 0;

    virtual std::any visitStmt(SysYParser::StmtContext *context) = 0;

    virtual std::any visitAssignStmt(SysYParser::AssignStmtContext *context) = 0;

    virtual std::any visitExpStmt(SysYParser::ExpStmtContext *context) = 0;

    virtual std::any visitIfStmt(SysYParser::IfStmtContext *context) = 0;

    virtual std::any visitWhileStmt(SysYParser::WhileStmtContext *context) = 0;

    virtual std::any visitBreakStmt(SysYParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(SysYParser::ContinueStmtContext *context) = 0;

    virtual std::any visitReturnStmt(SysYParser::ReturnStmtContext *context) = 0;

    virtual std::any visitEmptyStmt(SysYParser::EmptyStmtContext *context) = 0;

    virtual std::any visitRelationExp(SysYParser::RelationExpContext *context) = 0;

    virtual std::any visitVarExp(SysYParser::VarExpContext *context) = 0;

    virtual std::any visitMultiplicativeExp(SysYParser::MultiplicativeExpContext *context) = 0;

    virtual std::any visitNumberExp(SysYParser::NumberExpContext *context) = 0;

    virtual std::any visitAndExp(SysYParser::AndExpContext *context) = 0;

    virtual std::any visitUnaryExp(SysYParser::UnaryExpContext *context) = 0;

    virtual std::any visitParenExp(SysYParser::ParenExpContext *context) = 0;

    virtual std::any visitStringExp(SysYParser::StringExpContext *context) = 0;

    virtual std::any visitOrExp(SysYParser::OrExpContext *context) = 0;

    virtual std::any visitCallExp(SysYParser::CallExpContext *context) = 0;

    virtual std::any visitAdditiveExp(SysYParser::AdditiveExpContext *context) = 0;

    virtual std::any visitEqualExp(SysYParser::EqualExpContext *context) = 0;

    virtual std::any visitCall(SysYParser::CallContext *context) = 0;

    virtual std::any visitVar(SysYParser::VarContext *context) = 0;

    virtual std::any visitLValue(SysYParser::LValueContext *context) = 0;

    virtual std::any visitNumber(SysYParser::NumberContext *context) = 0;

    virtual std::any visitString(SysYParser::StringContext *context) = 0;

    virtual std::any visitFuncRParams(SysYParser::FuncRParamsContext *context) = 0;


};

