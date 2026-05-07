
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by SysyParser.
 */
class  SysyListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterCompUnit(SysyParser::CompUnitContext *ctx) = 0;
  virtual void exitCompUnit(SysyParser::CompUnitContext *ctx) = 0;

  virtual void enterDecl(SysyParser::DeclContext *ctx) = 0;
  virtual void exitDecl(SysyParser::DeclContext *ctx) = 0;

  virtual void enterConstDecl(SysyParser::ConstDeclContext *ctx) = 0;
  virtual void exitConstDecl(SysyParser::ConstDeclContext *ctx) = 0;

  virtual void enterBType(SysyParser::BTypeContext *ctx) = 0;
  virtual void exitBType(SysyParser::BTypeContext *ctx) = 0;

  virtual void enterConstDef(SysyParser::ConstDefContext *ctx) = 0;
  virtual void exitConstDef(SysyParser::ConstDefContext *ctx) = 0;

  virtual void enterSingleConstInitVal(SysyParser::SingleConstInitValContext *ctx) = 0;
  virtual void exitSingleConstInitVal(SysyParser::SingleConstInitValContext *ctx) = 0;

  virtual void enterMultiConstInitVal(SysyParser::MultiConstInitValContext *ctx) = 0;
  virtual void exitMultiConstInitVal(SysyParser::MultiConstInitValContext *ctx) = 0;

  virtual void enterVarDecl(SysyParser::VarDeclContext *ctx) = 0;
  virtual void exitVarDecl(SysyParser::VarDeclContext *ctx) = 0;

  virtual void enterNoinitVarDef(SysyParser::NoinitVarDefContext *ctx) = 0;
  virtual void exitNoinitVarDef(SysyParser::NoinitVarDefContext *ctx) = 0;

  virtual void enterInitVarDef(SysyParser::InitVarDefContext *ctx) = 0;
  virtual void exitInitVarDef(SysyParser::InitVarDefContext *ctx) = 0;

  virtual void enterSingleInitVal(SysyParser::SingleInitValContext *ctx) = 0;
  virtual void exitSingleInitVal(SysyParser::SingleInitValContext *ctx) = 0;

  virtual void enterMultiInitVal(SysyParser::MultiInitValContext *ctx) = 0;
  virtual void exitMultiInitVal(SysyParser::MultiInitValContext *ctx) = 0;

  virtual void enterFuncDef(SysyParser::FuncDefContext *ctx) = 0;
  virtual void exitFuncDef(SysyParser::FuncDefContext *ctx) = 0;

  virtual void enterFuncType(SysyParser::FuncTypeContext *ctx) = 0;
  virtual void exitFuncType(SysyParser::FuncTypeContext *ctx) = 0;

  virtual void enterFuncFParams(SysyParser::FuncFParamsContext *ctx) = 0;
  virtual void exitFuncFParams(SysyParser::FuncFParamsContext *ctx) = 0;

  virtual void enterFuncFParam(SysyParser::FuncFParamContext *ctx) = 0;
  virtual void exitFuncFParam(SysyParser::FuncFParamContext *ctx) = 0;

  virtual void enterBlock(SysyParser::BlockContext *ctx) = 0;
  virtual void exitBlock(SysyParser::BlockContext *ctx) = 0;

  virtual void enterBlockItem(SysyParser::BlockItemContext *ctx) = 0;
  virtual void exitBlockItem(SysyParser::BlockItemContext *ctx) = 0;

  virtual void enterAssignStmt(SysyParser::AssignStmtContext *ctx) = 0;
  virtual void exitAssignStmt(SysyParser::AssignStmtContext *ctx) = 0;

  virtual void enterExpStmt(SysyParser::ExpStmtContext *ctx) = 0;
  virtual void exitExpStmt(SysyParser::ExpStmtContext *ctx) = 0;

  virtual void enterBlockStmt(SysyParser::BlockStmtContext *ctx) = 0;
  virtual void exitBlockStmt(SysyParser::BlockStmtContext *ctx) = 0;

  virtual void enterIfStmt(SysyParser::IfStmtContext *ctx) = 0;
  virtual void exitIfStmt(SysyParser::IfStmtContext *ctx) = 0;

  virtual void enterIfElseStmt(SysyParser::IfElseStmtContext *ctx) = 0;
  virtual void exitIfElseStmt(SysyParser::IfElseStmtContext *ctx) = 0;

  virtual void enterWhileStmt(SysyParser::WhileStmtContext *ctx) = 0;
  virtual void exitWhileStmt(SysyParser::WhileStmtContext *ctx) = 0;

  virtual void enterBreakStmt(SysyParser::BreakStmtContext *ctx) = 0;
  virtual void exitBreakStmt(SysyParser::BreakStmtContext *ctx) = 0;

  virtual void enterContinueStmt(SysyParser::ContinueStmtContext *ctx) = 0;
  virtual void exitContinueStmt(SysyParser::ContinueStmtContext *ctx) = 0;

  virtual void enterReturnStmt(SysyParser::ReturnStmtContext *ctx) = 0;
  virtual void exitReturnStmt(SysyParser::ReturnStmtContext *ctx) = 0;

  virtual void enterExp(SysyParser::ExpContext *ctx) = 0;
  virtual void exitExp(SysyParser::ExpContext *ctx) = 0;

  virtual void enterCond(SysyParser::CondContext *ctx) = 0;
  virtual void exitCond(SysyParser::CondContext *ctx) = 0;

  virtual void enterLVal(SysyParser::LValContext *ctx) = 0;
  virtual void exitLVal(SysyParser::LValContext *ctx) = 0;

  virtual void enterParensPrimaryExp(SysyParser::ParensPrimaryExpContext *ctx) = 0;
  virtual void exitParensPrimaryExp(SysyParser::ParensPrimaryExpContext *ctx) = 0;

  virtual void enterLValPrimaryExp(SysyParser::LValPrimaryExpContext *ctx) = 0;
  virtual void exitLValPrimaryExp(SysyParser::LValPrimaryExpContext *ctx) = 0;

  virtual void enterNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *ctx) = 0;
  virtual void exitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *ctx) = 0;

  virtual void enterNumber(SysyParser::NumberContext *ctx) = 0;
  virtual void exitNumber(SysyParser::NumberContext *ctx) = 0;

  virtual void enterPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *ctx) = 0;
  virtual void exitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *ctx) = 0;

  virtual void enterFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *ctx) = 0;
  virtual void exitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *ctx) = 0;

  virtual void enterUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *ctx) = 0;
  virtual void exitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *ctx) = 0;

  virtual void enterUnaryOp(SysyParser::UnaryOpContext *ctx) = 0;
  virtual void exitUnaryOp(SysyParser::UnaryOpContext *ctx) = 0;

  virtual void enterFuncRParams(SysyParser::FuncRParamsContext *ctx) = 0;
  virtual void exitFuncRParams(SysyParser::FuncRParamsContext *ctx) = 0;

  virtual void enterMulMulExp(SysyParser::MulMulExpContext *ctx) = 0;
  virtual void exitMulMulExp(SysyParser::MulMulExpContext *ctx) = 0;

  virtual void enterUnaryMulExp(SysyParser::UnaryMulExpContext *ctx) = 0;
  virtual void exitUnaryMulExp(SysyParser::UnaryMulExpContext *ctx) = 0;

  virtual void enterAddAddExp(SysyParser::AddAddExpContext *ctx) = 0;
  virtual void exitAddAddExp(SysyParser::AddAddExpContext *ctx) = 0;

  virtual void enterMulAddExp(SysyParser::MulAddExpContext *ctx) = 0;
  virtual void exitMulAddExp(SysyParser::MulAddExpContext *ctx) = 0;

  virtual void enterRelRelExp(SysyParser::RelRelExpContext *ctx) = 0;
  virtual void exitRelRelExp(SysyParser::RelRelExpContext *ctx) = 0;

  virtual void enterAddRelExp(SysyParser::AddRelExpContext *ctx) = 0;
  virtual void exitAddRelExp(SysyParser::AddRelExpContext *ctx) = 0;

  virtual void enterEqEqExp(SysyParser::EqEqExpContext *ctx) = 0;
  virtual void exitEqEqExp(SysyParser::EqEqExpContext *ctx) = 0;

  virtual void enterRelEqExp(SysyParser::RelEqExpContext *ctx) = 0;
  virtual void exitRelEqExp(SysyParser::RelEqExpContext *ctx) = 0;

  virtual void enterEqLAndExp(SysyParser::EqLAndExpContext *ctx) = 0;
  virtual void exitEqLAndExp(SysyParser::EqLAndExpContext *ctx) = 0;

  virtual void enterLAndLAndExp(SysyParser::LAndLAndExpContext *ctx) = 0;
  virtual void exitLAndLAndExp(SysyParser::LAndLAndExpContext *ctx) = 0;

  virtual void enterLAndLOrExp(SysyParser::LAndLOrExpContext *ctx) = 0;
  virtual void exitLAndLOrExp(SysyParser::LAndLOrExpContext *ctx) = 0;

  virtual void enterLOrLOrExp(SysyParser::LOrLOrExpContext *ctx) = 0;
  virtual void exitLOrLOrExp(SysyParser::LOrLOrExpContext *ctx) = 0;

  virtual void enterConstExp(SysyParser::ConstExpContext *ctx) = 0;
  virtual void exitConstExp(SysyParser::ConstExpContext *ctx) = 0;

  virtual void enterEpsilon(SysyParser::EpsilonContext *ctx) = 0;
  virtual void exitEpsilon(SysyParser::EpsilonContext *ctx) = 0;


};

