
// Generated from ./parser/SysYParser.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysYParserParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysYParserParser.
 */
class  SysYParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysYParserParser.
   */
    virtual std::any visitProgram(SysYParserParser::ProgramContext *context) = 0;

    virtual std::any visitCompUnit(SysYParserParser::CompUnitContext *context) = 0;

    virtual std::any visitDecl(SysYParserParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysYParserParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysYParserParser::BTypeContext *context) = 0;

    virtual std::any visitConstDefSingle(SysYParserParser::ConstDefSingleContext *context) = 0;

    virtual std::any visitConstDefArray(SysYParserParser::ConstDefArrayContext *context) = 0;

    virtual std::any visitConstInitValSingle(SysYParserParser::ConstInitValSingleContext *context) = 0;

    virtual std::any visitConstInitValArray(SysYParserParser::ConstInitValArrayContext *context) = 0;

    virtual std::any visitVarDecl(SysYParserParser::VarDeclContext *context) = 0;

    virtual std::any visitVarDefSingle(SysYParserParser::VarDefSingleContext *context) = 0;

    virtual std::any visitVarDefArray(SysYParserParser::VarDefArrayContext *context) = 0;

    virtual std::any visitVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext *context) = 0;

    virtual std::any visitVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext *context) = 0;

    virtual std::any visitInitLVal(SysYParserParser::InitLValContext *context) = 0;

    virtual std::any visitInitValSingle(SysYParserParser::InitValSingleContext *context) = 0;

    virtual std::any visitInitValArray(SysYParserParser::InitValArrayContext *context) = 0;

    virtual std::any visitFuncDef(SysYParserParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysYParserParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysYParserParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParamSingle(SysYParserParser::FuncFParamSingleContext *context) = 0;

    virtual std::any visitFuncFParamArray(SysYParserParser::FuncFParamArrayContext *context) = 0;

    virtual std::any visitBlock(SysYParserParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysYParserParser::BlockItemContext *context) = 0;

    virtual std::any visitStmtAssign(SysYParserParser::StmtAssignContext *context) = 0;

    virtual std::any visitStmtExp(SysYParserParser::StmtExpContext *context) = 0;

    virtual std::any visitStmtBlock(SysYParserParser::StmtBlockContext *context) = 0;

    virtual std::any visitStmtCond(SysYParserParser::StmtCondContext *context) = 0;

    virtual std::any visitStmtWhile(SysYParserParser::StmtWhileContext *context) = 0;

    virtual std::any visitStmtBreak(SysYParserParser::StmtBreakContext *context) = 0;

    virtual std::any visitStmtContinue(SysYParserParser::StmtContinueContext *context) = 0;

    virtual std::any visitStmtReturn(SysYParserParser::StmtReturnContext *context) = 0;

    virtual std::any visitExp(SysYParserParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysYParserParser::CondContext *context) = 0;

    virtual std::any visitLValSingle(SysYParserParser::LValSingleContext *context) = 0;

    virtual std::any visitLValArray(SysYParserParser::LValArrayContext *context) = 0;

    virtual std::any visitPrimaryExpParen(SysYParserParser::PrimaryExpParenContext *context) = 0;

    virtual std::any visitPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext *context) = 0;

    virtual std::any visitPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext *context) = 0;

    virtual std::any visitNumber(SysYParserParser::NumberContext *context) = 0;

    virtual std::any visitUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext *context) = 0;

    virtual std::any visitUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext *context) = 0;

    virtual std::any visitUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext *context) = 0;

    virtual std::any visitUnaryOp(SysYParserParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysYParserParser::FuncRParamsContext *context) = 0;

    virtual std::any visitFuncRParam(SysYParserParser::FuncRParamContext *context) = 0;

    virtual std::any visitMulExp(SysYParserParser::MulExpContext *context) = 0;

    virtual std::any visitAddExp(SysYParserParser::AddExpContext *context) = 0;

    virtual std::any visitRelExp(SysYParserParser::RelExpContext *context) = 0;

    virtual std::any visitEqExp(SysYParserParser::EqExpContext *context) = 0;

    virtual std::any visitLAndExp(SysYParserParser::LAndExpContext *context) = 0;

    virtual std::any visitLOrExp(SysYParserParser::LOrExpContext *context) = 0;

    virtual std::any visitConstExp(SysYParserParser::ConstExpContext *context) = 0;

    virtual std::any visitIntDecConst(SysYParserParser::IntDecConstContext *context) = 0;

    virtual std::any visitIntOctConst(SysYParserParser::IntOctConstContext *context) = 0;

    virtual std::any visitIntHexConst(SysYParserParser::IntHexConstContext *context) = 0;

    virtual std::any visitFloatConst(SysYParserParser::FloatConstContext *context) = 0;


};

