
// Generated from ./parser/SysYParser.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysYParserParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by SysYParserParser.
 */
class  SysYParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(SysYParserParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(SysYParserParser::ProgramContext *ctx) = 0;

  virtual void enterCompUnit(SysYParserParser::CompUnitContext *ctx) = 0;
  virtual void exitCompUnit(SysYParserParser::CompUnitContext *ctx) = 0;

  virtual void enterDecl(SysYParserParser::DeclContext *ctx) = 0;
  virtual void exitDecl(SysYParserParser::DeclContext *ctx) = 0;

  virtual void enterConstDecl(SysYParserParser::ConstDeclContext *ctx) = 0;
  virtual void exitConstDecl(SysYParserParser::ConstDeclContext *ctx) = 0;

  virtual void enterBType(SysYParserParser::BTypeContext *ctx) = 0;
  virtual void exitBType(SysYParserParser::BTypeContext *ctx) = 0;

  virtual void enterConstDefSingle(SysYParserParser::ConstDefSingleContext *ctx) = 0;
  virtual void exitConstDefSingle(SysYParserParser::ConstDefSingleContext *ctx) = 0;

  virtual void enterConstDefArray(SysYParserParser::ConstDefArrayContext *ctx) = 0;
  virtual void exitConstDefArray(SysYParserParser::ConstDefArrayContext *ctx) = 0;

  virtual void enterConstInitValSingle(SysYParserParser::ConstInitValSingleContext *ctx) = 0;
  virtual void exitConstInitValSingle(SysYParserParser::ConstInitValSingleContext *ctx) = 0;

  virtual void enterConstInitValArray(SysYParserParser::ConstInitValArrayContext *ctx) = 0;
  virtual void exitConstInitValArray(SysYParserParser::ConstInitValArrayContext *ctx) = 0;

  virtual void enterVarDecl(SysYParserParser::VarDeclContext *ctx) = 0;
  virtual void exitVarDecl(SysYParserParser::VarDeclContext *ctx) = 0;

  virtual void enterVarDefSingle(SysYParserParser::VarDefSingleContext *ctx) = 0;
  virtual void exitVarDefSingle(SysYParserParser::VarDefSingleContext *ctx) = 0;

  virtual void enterVarDefArray(SysYParserParser::VarDefArrayContext *ctx) = 0;
  virtual void exitVarDefArray(SysYParserParser::VarDefArrayContext *ctx) = 0;

  virtual void enterVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext *ctx) = 0;
  virtual void exitVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext *ctx) = 0;

  virtual void enterVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext *ctx) = 0;
  virtual void exitVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext *ctx) = 0;

  virtual void enterInitLVal(SysYParserParser::InitLValContext *ctx) = 0;
  virtual void exitInitLVal(SysYParserParser::InitLValContext *ctx) = 0;

  virtual void enterInitValSingle(SysYParserParser::InitValSingleContext *ctx) = 0;
  virtual void exitInitValSingle(SysYParserParser::InitValSingleContext *ctx) = 0;

  virtual void enterInitValArray(SysYParserParser::InitValArrayContext *ctx) = 0;
  virtual void exitInitValArray(SysYParserParser::InitValArrayContext *ctx) = 0;

  virtual void enterFuncDef(SysYParserParser::FuncDefContext *ctx) = 0;
  virtual void exitFuncDef(SysYParserParser::FuncDefContext *ctx) = 0;

  virtual void enterFuncType(SysYParserParser::FuncTypeContext *ctx) = 0;
  virtual void exitFuncType(SysYParserParser::FuncTypeContext *ctx) = 0;

  virtual void enterFuncFParams(SysYParserParser::FuncFParamsContext *ctx) = 0;
  virtual void exitFuncFParams(SysYParserParser::FuncFParamsContext *ctx) = 0;

  virtual void enterFuncFParamSingle(SysYParserParser::FuncFParamSingleContext *ctx) = 0;
  virtual void exitFuncFParamSingle(SysYParserParser::FuncFParamSingleContext *ctx) = 0;

  virtual void enterFuncFParamArray(SysYParserParser::FuncFParamArrayContext *ctx) = 0;
  virtual void exitFuncFParamArray(SysYParserParser::FuncFParamArrayContext *ctx) = 0;

  virtual void enterBlock(SysYParserParser::BlockContext *ctx) = 0;
  virtual void exitBlock(SysYParserParser::BlockContext *ctx) = 0;

  virtual void enterBlockItem(SysYParserParser::BlockItemContext *ctx) = 0;
  virtual void exitBlockItem(SysYParserParser::BlockItemContext *ctx) = 0;

  virtual void enterStmtAssign(SysYParserParser::StmtAssignContext *ctx) = 0;
  virtual void exitStmtAssign(SysYParserParser::StmtAssignContext *ctx) = 0;

  virtual void enterStmtExp(SysYParserParser::StmtExpContext *ctx) = 0;
  virtual void exitStmtExp(SysYParserParser::StmtExpContext *ctx) = 0;

  virtual void enterStmtBlock(SysYParserParser::StmtBlockContext *ctx) = 0;
  virtual void exitStmtBlock(SysYParserParser::StmtBlockContext *ctx) = 0;

  virtual void enterStmtCond(SysYParserParser::StmtCondContext *ctx) = 0;
  virtual void exitStmtCond(SysYParserParser::StmtCondContext *ctx) = 0;

  virtual void enterStmtWhile(SysYParserParser::StmtWhileContext *ctx) = 0;
  virtual void exitStmtWhile(SysYParserParser::StmtWhileContext *ctx) = 0;

  virtual void enterStmtBreak(SysYParserParser::StmtBreakContext *ctx) = 0;
  virtual void exitStmtBreak(SysYParserParser::StmtBreakContext *ctx) = 0;

  virtual void enterStmtContinue(SysYParserParser::StmtContinueContext *ctx) = 0;
  virtual void exitStmtContinue(SysYParserParser::StmtContinueContext *ctx) = 0;

  virtual void enterStmtReturn(SysYParserParser::StmtReturnContext *ctx) = 0;
  virtual void exitStmtReturn(SysYParserParser::StmtReturnContext *ctx) = 0;

  virtual void enterExp(SysYParserParser::ExpContext *ctx) = 0;
  virtual void exitExp(SysYParserParser::ExpContext *ctx) = 0;

  virtual void enterCond(SysYParserParser::CondContext *ctx) = 0;
  virtual void exitCond(SysYParserParser::CondContext *ctx) = 0;

  virtual void enterLValSingle(SysYParserParser::LValSingleContext *ctx) = 0;
  virtual void exitLValSingle(SysYParserParser::LValSingleContext *ctx) = 0;

  virtual void enterLValArray(SysYParserParser::LValArrayContext *ctx) = 0;
  virtual void exitLValArray(SysYParserParser::LValArrayContext *ctx) = 0;

  virtual void enterPrimaryExpParen(SysYParserParser::PrimaryExpParenContext *ctx) = 0;
  virtual void exitPrimaryExpParen(SysYParserParser::PrimaryExpParenContext *ctx) = 0;

  virtual void enterPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext *ctx) = 0;
  virtual void exitPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext *ctx) = 0;

  virtual void enterPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext *ctx) = 0;
  virtual void exitPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext *ctx) = 0;

  virtual void enterNumber(SysYParserParser::NumberContext *ctx) = 0;
  virtual void exitNumber(SysYParserParser::NumberContext *ctx) = 0;

  virtual void enterUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext *ctx) = 0;
  virtual void exitUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext *ctx) = 0;

  virtual void enterUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext *ctx) = 0;
  virtual void exitUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext *ctx) = 0;

  virtual void enterUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext *ctx) = 0;
  virtual void exitUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext *ctx) = 0;

  virtual void enterUnaryOp(SysYParserParser::UnaryOpContext *ctx) = 0;
  virtual void exitUnaryOp(SysYParserParser::UnaryOpContext *ctx) = 0;

  virtual void enterFuncRParams(SysYParserParser::FuncRParamsContext *ctx) = 0;
  virtual void exitFuncRParams(SysYParserParser::FuncRParamsContext *ctx) = 0;

  virtual void enterFuncRParam(SysYParserParser::FuncRParamContext *ctx) = 0;
  virtual void exitFuncRParam(SysYParserParser::FuncRParamContext *ctx) = 0;

  virtual void enterMulExp(SysYParserParser::MulExpContext *ctx) = 0;
  virtual void exitMulExp(SysYParserParser::MulExpContext *ctx) = 0;

  virtual void enterAddExp(SysYParserParser::AddExpContext *ctx) = 0;
  virtual void exitAddExp(SysYParserParser::AddExpContext *ctx) = 0;

  virtual void enterRelExp(SysYParserParser::RelExpContext *ctx) = 0;
  virtual void exitRelExp(SysYParserParser::RelExpContext *ctx) = 0;

  virtual void enterEqExp(SysYParserParser::EqExpContext *ctx) = 0;
  virtual void exitEqExp(SysYParserParser::EqExpContext *ctx) = 0;

  virtual void enterLAndExp(SysYParserParser::LAndExpContext *ctx) = 0;
  virtual void exitLAndExp(SysYParserParser::LAndExpContext *ctx) = 0;

  virtual void enterLOrExp(SysYParserParser::LOrExpContext *ctx) = 0;
  virtual void exitLOrExp(SysYParserParser::LOrExpContext *ctx) = 0;

  virtual void enterConstExp(SysYParserParser::ConstExpContext *ctx) = 0;
  virtual void exitConstExp(SysYParserParser::ConstExpContext *ctx) = 0;

  virtual void enterIntDecConst(SysYParserParser::IntDecConstContext *ctx) = 0;
  virtual void exitIntDecConst(SysYParserParser::IntDecConstContext *ctx) = 0;

  virtual void enterIntOctConst(SysYParserParser::IntOctConstContext *ctx) = 0;
  virtual void exitIntOctConst(SysYParserParser::IntOctConstContext *ctx) = 0;

  virtual void enterIntHexConst(SysYParserParser::IntHexConstContext *ctx) = 0;
  virtual void exitIntHexConst(SysYParserParser::IntHexConstContext *ctx) = 0;

  virtual void enterFloatConst(SysYParserParser::FloatConstContext *ctx) = 0;
  virtual void exitFloatConst(SysYParserParser::FloatConstContext *ctx) = 0;


};

