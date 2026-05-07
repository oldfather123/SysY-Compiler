
// Generated from SysY2022.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "SysY2022Parser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by SysY2022Parser.
 */
class  SysY2022Listener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterCompUnit(SysY2022Parser::CompUnitContext *ctx) = 0;
  virtual void exitCompUnit(SysY2022Parser::CompUnitContext *ctx) = 0;

  virtual void enterDecl(SysY2022Parser::DeclContext *ctx) = 0;
  virtual void exitDecl(SysY2022Parser::DeclContext *ctx) = 0;

  virtual void enterConstDecl(SysY2022Parser::ConstDeclContext *ctx) = 0;
  virtual void exitConstDecl(SysY2022Parser::ConstDeclContext *ctx) = 0;

  virtual void enterBType(SysY2022Parser::BTypeContext *ctx) = 0;
  virtual void exitBType(SysY2022Parser::BTypeContext *ctx) = 0;

  virtual void enterConstDef(SysY2022Parser::ConstDefContext *ctx) = 0;
  virtual void exitConstDef(SysY2022Parser::ConstDefContext *ctx) = 0;

  virtual void enterItemConstInitVal(SysY2022Parser::ItemConstInitValContext *ctx) = 0;
  virtual void exitItemConstInitVal(SysY2022Parser::ItemConstInitValContext *ctx) = 0;

  virtual void enterListConstInitVal(SysY2022Parser::ListConstInitValContext *ctx) = 0;
  virtual void exitListConstInitVal(SysY2022Parser::ListConstInitValContext *ctx) = 0;

  virtual void enterVarDecl(SysY2022Parser::VarDeclContext *ctx) = 0;
  virtual void exitVarDecl(SysY2022Parser::VarDeclContext *ctx) = 0;

  virtual void enterVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext *ctx) = 0;
  virtual void exitVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext *ctx) = 0;

  virtual void enterVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext *ctx) = 0;
  virtual void exitVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext *ctx) = 0;

  virtual void enterItemInitVal(SysY2022Parser::ItemInitValContext *ctx) = 0;
  virtual void exitItemInitVal(SysY2022Parser::ItemInitValContext *ctx) = 0;

  virtual void enterListInitVal(SysY2022Parser::ListInitValContext *ctx) = 0;
  virtual void exitListInitVal(SysY2022Parser::ListInitValContext *ctx) = 0;

  virtual void enterFuncDef(SysY2022Parser::FuncDefContext *ctx) = 0;
  virtual void exitFuncDef(SysY2022Parser::FuncDefContext *ctx) = 0;

  virtual void enterFuncType(SysY2022Parser::FuncTypeContext *ctx) = 0;
  virtual void exitFuncType(SysY2022Parser::FuncTypeContext *ctx) = 0;

  virtual void enterFuncFParams(SysY2022Parser::FuncFParamsContext *ctx) = 0;
  virtual void exitFuncFParams(SysY2022Parser::FuncFParamsContext *ctx) = 0;

  virtual void enterFuncFParam(SysY2022Parser::FuncFParamContext *ctx) = 0;
  virtual void exitFuncFParam(SysY2022Parser::FuncFParamContext *ctx) = 0;

  virtual void enterBlock(SysY2022Parser::BlockContext *ctx) = 0;
  virtual void exitBlock(SysY2022Parser::BlockContext *ctx) = 0;

  virtual void enterBlockItem(SysY2022Parser::BlockItemContext *ctx) = 0;
  virtual void exitBlockItem(SysY2022Parser::BlockItemContext *ctx) = 0;

  virtual void enterAssignStmt(SysY2022Parser::AssignStmtContext *ctx) = 0;
  virtual void exitAssignStmt(SysY2022Parser::AssignStmtContext *ctx) = 0;

  virtual void enterExpStmt(SysY2022Parser::ExpStmtContext *ctx) = 0;
  virtual void exitExpStmt(SysY2022Parser::ExpStmtContext *ctx) = 0;

  virtual void enterBlockStmt(SysY2022Parser::BlockStmtContext *ctx) = 0;
  virtual void exitBlockStmt(SysY2022Parser::BlockStmtContext *ctx) = 0;

  virtual void enterIfStmt(SysY2022Parser::IfStmtContext *ctx) = 0;
  virtual void exitIfStmt(SysY2022Parser::IfStmtContext *ctx) = 0;

  virtual void enterIfElseStmt(SysY2022Parser::IfElseStmtContext *ctx) = 0;
  virtual void exitIfElseStmt(SysY2022Parser::IfElseStmtContext *ctx) = 0;

  virtual void enterWhileStmt(SysY2022Parser::WhileStmtContext *ctx) = 0;
  virtual void exitWhileStmt(SysY2022Parser::WhileStmtContext *ctx) = 0;

  virtual void enterBreakStmt(SysY2022Parser::BreakStmtContext *ctx) = 0;
  virtual void exitBreakStmt(SysY2022Parser::BreakStmtContext *ctx) = 0;

  virtual void enterContinueStmt(SysY2022Parser::ContinueStmtContext *ctx) = 0;
  virtual void exitContinueStmt(SysY2022Parser::ContinueStmtContext *ctx) = 0;

  virtual void enterReturnStmt(SysY2022Parser::ReturnStmtContext *ctx) = 0;
  virtual void exitReturnStmt(SysY2022Parser::ReturnStmtContext *ctx) = 0;

  virtual void enterExp(SysY2022Parser::ExpContext *ctx) = 0;
  virtual void exitExp(SysY2022Parser::ExpContext *ctx) = 0;

  virtual void enterCond(SysY2022Parser::CondContext *ctx) = 0;
  virtual void exitCond(SysY2022Parser::CondContext *ctx) = 0;

  virtual void enterLVal(SysY2022Parser::LValContext *ctx) = 0;
  virtual void exitLVal(SysY2022Parser::LValContext *ctx) = 0;

  virtual void enterExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext *ctx) = 0;
  virtual void exitExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext *ctx) = 0;

  virtual void enterLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext *ctx) = 0;
  virtual void exitLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext *ctx) = 0;

  virtual void enterNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext *ctx) = 0;
  virtual void exitNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext *ctx) = 0;

  virtual void enterNumber(SysY2022Parser::NumberContext *ctx) = 0;
  virtual void exitNumber(SysY2022Parser::NumberContext *ctx) = 0;

  virtual void enterPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext *ctx) = 0;
  virtual void exitPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext *ctx) = 0;

  virtual void enterFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext *ctx) = 0;
  virtual void exitFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext *ctx) = 0;

  virtual void enterUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext *ctx) = 0;
  virtual void exitUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext *ctx) = 0;

  virtual void enterUnaryOp(SysY2022Parser::UnaryOpContext *ctx) = 0;
  virtual void exitUnaryOp(SysY2022Parser::UnaryOpContext *ctx) = 0;

  virtual void enterFuncRParams(SysY2022Parser::FuncRParamsContext *ctx) = 0;
  virtual void exitFuncRParams(SysY2022Parser::FuncRParamsContext *ctx) = 0;

  virtual void enterFuncRParam(SysY2022Parser::FuncRParamContext *ctx) = 0;
  virtual void exitFuncRParam(SysY2022Parser::FuncRParamContext *ctx) = 0;

  virtual void enterMulMulExp(SysY2022Parser::MulMulExpContext *ctx) = 0;
  virtual void exitMulMulExp(SysY2022Parser::MulMulExpContext *ctx) = 0;

  virtual void enterUnaryMulExp(SysY2022Parser::UnaryMulExpContext *ctx) = 0;
  virtual void exitUnaryMulExp(SysY2022Parser::UnaryMulExpContext *ctx) = 0;

  virtual void enterAddAddExp(SysY2022Parser::AddAddExpContext *ctx) = 0;
  virtual void exitAddAddExp(SysY2022Parser::AddAddExpContext *ctx) = 0;

  virtual void enterMulAddExp(SysY2022Parser::MulAddExpContext *ctx) = 0;
  virtual void exitMulAddExp(SysY2022Parser::MulAddExpContext *ctx) = 0;

  virtual void enterRelRelExp(SysY2022Parser::RelRelExpContext *ctx) = 0;
  virtual void exitRelRelExp(SysY2022Parser::RelRelExpContext *ctx) = 0;

  virtual void enterAddRelExp(SysY2022Parser::AddRelExpContext *ctx) = 0;
  virtual void exitAddRelExp(SysY2022Parser::AddRelExpContext *ctx) = 0;

  virtual void enterEqEqExp(SysY2022Parser::EqEqExpContext *ctx) = 0;
  virtual void exitEqEqExp(SysY2022Parser::EqEqExpContext *ctx) = 0;

  virtual void enterRelEqExp(SysY2022Parser::RelEqExpContext *ctx) = 0;
  virtual void exitRelEqExp(SysY2022Parser::RelEqExpContext *ctx) = 0;

  virtual void enterEqLAndExp(SysY2022Parser::EqLAndExpContext *ctx) = 0;
  virtual void exitEqLAndExp(SysY2022Parser::EqLAndExpContext *ctx) = 0;

  virtual void enterLAndLAndExp(SysY2022Parser::LAndLAndExpContext *ctx) = 0;
  virtual void exitLAndLAndExp(SysY2022Parser::LAndLAndExpContext *ctx) = 0;

  virtual void enterLAndLOrExp(SysY2022Parser::LAndLOrExpContext *ctx) = 0;
  virtual void exitLAndLOrExp(SysY2022Parser::LAndLOrExpContext *ctx) = 0;

  virtual void enterLOrLOrExp(SysY2022Parser::LOrLOrExpContext *ctx) = 0;
  virtual void exitLOrLOrExp(SysY2022Parser::LOrLOrExpContext *ctx) = 0;

  virtual void enterConstExp(SysY2022Parser::ConstExpContext *ctx) = 0;
  virtual void exitConstExp(SysY2022Parser::ConstExpContext *ctx) = 0;


};

