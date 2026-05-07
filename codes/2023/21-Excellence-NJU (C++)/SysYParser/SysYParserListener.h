
// Generated from SysYParser.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysYParser.h"


namespace antlr4 {

/**
 * This interface defines an abstract listener for a parse tree produced by SysYParser.
 */
class  SysYParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(SysYParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(SysYParser::ProgramContext *ctx) = 0;

  virtual void enterCompUnit(SysYParser::CompUnitContext *ctx) = 0;
  virtual void exitCompUnit(SysYParser::CompUnitContext *ctx) = 0;

  virtual void enterDecl(SysYParser::DeclContext *ctx) = 0;
  virtual void exitDecl(SysYParser::DeclContext *ctx) = 0;

  virtual void enterConstDecl(SysYParser::ConstDeclContext *ctx) = 0;
  virtual void exitConstDecl(SysYParser::ConstDeclContext *ctx) = 0;

  virtual void enterBType(SysYParser::BTypeContext *ctx) = 0;
  virtual void exitBType(SysYParser::BTypeContext *ctx) = 0;

  virtual void enterConstDef(SysYParser::ConstDefContext *ctx) = 0;
  virtual void exitConstDef(SysYParser::ConstDefContext *ctx) = 0;

  virtual void enterConstInitVal(SysYParser::ConstInitValContext *ctx) = 0;
  virtual void exitConstInitVal(SysYParser::ConstInitValContext *ctx) = 0;

  virtual void enterVarDecl(SysYParser::VarDeclContext *ctx) = 0;
  virtual void exitVarDecl(SysYParser::VarDeclContext *ctx) = 0;

  virtual void enterVarDef(SysYParser::VarDefContext *ctx) = 0;
  virtual void exitVarDef(SysYParser::VarDefContext *ctx) = 0;

  virtual void enterInitVal(SysYParser::InitValContext *ctx) = 0;
  virtual void exitInitVal(SysYParser::InitValContext *ctx) = 0;

  virtual void enterFuncDef(SysYParser::FuncDefContext *ctx) = 0;
  virtual void exitFuncDef(SysYParser::FuncDefContext *ctx) = 0;

  virtual void enterFuncType(SysYParser::FuncTypeContext *ctx) = 0;
  virtual void exitFuncType(SysYParser::FuncTypeContext *ctx) = 0;

  virtual void enterFuncFParams(SysYParser::FuncFParamsContext *ctx) = 0;
  virtual void exitFuncFParams(SysYParser::FuncFParamsContext *ctx) = 0;

  virtual void enterFuncFParam(SysYParser::FuncFParamContext *ctx) = 0;
  virtual void exitFuncFParam(SysYParser::FuncFParamContext *ctx) = 0;

  virtual void enterBlock(SysYParser::BlockContext *ctx) = 0;
  virtual void exitBlock(SysYParser::BlockContext *ctx) = 0;

  virtual void enterBlockItem(SysYParser::BlockItemContext *ctx) = 0;
  virtual void exitBlockItem(SysYParser::BlockItemContext *ctx) = 0;

  virtual void enterAssignStmt(SysYParser::AssignStmtContext *ctx) = 0;
  virtual void exitAssignStmt(SysYParser::AssignStmtContext *ctx) = 0;

  virtual void enterExpStmt(SysYParser::ExpStmtContext *ctx) = 0;
  virtual void exitExpStmt(SysYParser::ExpStmtContext *ctx) = 0;

  virtual void enterBlockStmt(SysYParser::BlockStmtContext *ctx) = 0;
  virtual void exitBlockStmt(SysYParser::BlockStmtContext *ctx) = 0;

  virtual void enterIfStmt(SysYParser::IfStmtContext *ctx) = 0;
  virtual void exitIfStmt(SysYParser::IfStmtContext *ctx) = 0;

  virtual void enterIfElseStmt(SysYParser::IfElseStmtContext *ctx) = 0;
  virtual void exitIfElseStmt(SysYParser::IfElseStmtContext *ctx) = 0;

  virtual void enterWhileStmt(SysYParser::WhileStmtContext *ctx) = 0;
  virtual void exitWhileStmt(SysYParser::WhileStmtContext *ctx) = 0;

  virtual void enterBreakStmt(SysYParser::BreakStmtContext *ctx) = 0;
  virtual void exitBreakStmt(SysYParser::BreakStmtContext *ctx) = 0;

  virtual void enterContinueStmt(SysYParser::ContinueStmtContext *ctx) = 0;
  virtual void exitContinueStmt(SysYParser::ContinueStmtContext *ctx) = 0;

  virtual void enterReturnStmt(SysYParser::ReturnStmtContext *ctx) = 0;
  virtual void exitReturnStmt(SysYParser::ReturnStmtContext *ctx) = 0;

  virtual void enterExpParenthesis(SysYParser::ExpParenthesisContext *ctx) = 0;
  virtual void exitExpParenthesis(SysYParser::ExpParenthesisContext *ctx) = 0;

  virtual void enterCallFuncExp(SysYParser::CallFuncExpContext *ctx) = 0;
  virtual void exitCallFuncExp(SysYParser::CallFuncExpContext *ctx) = 0;

  virtual void enterLValExp(SysYParser::LValExpContext *ctx) = 0;
  virtual void exitLValExp(SysYParser::LValExpContext *ctx) = 0;

  virtual void enterNumberExp(SysYParser::NumberExpContext *ctx) = 0;
  virtual void exitNumberExp(SysYParser::NumberExpContext *ctx) = 0;

  virtual void enterUnaryOpExp(SysYParser::UnaryOpExpContext *ctx) = 0;
  virtual void exitUnaryOpExp(SysYParser::UnaryOpExpContext *ctx) = 0;

  virtual void enterPlusExp(SysYParser::PlusExpContext *ctx) = 0;
  virtual void exitPlusExp(SysYParser::PlusExpContext *ctx) = 0;

  virtual void enterMulExp(SysYParser::MulExpContext *ctx) = 0;
  virtual void exitMulExp(SysYParser::MulExpContext *ctx) = 0;

  virtual void enterLtCond(SysYParser::LtCondContext *ctx) = 0;
  virtual void exitLtCond(SysYParser::LtCondContext *ctx) = 0;

  virtual void enterOrCond(SysYParser::OrCondContext *ctx) = 0;
  virtual void exitOrCond(SysYParser::OrCondContext *ctx) = 0;

  virtual void enterExpCond(SysYParser::ExpCondContext *ctx) = 0;
  virtual void exitExpCond(SysYParser::ExpCondContext *ctx) = 0;

  virtual void enterAndCond(SysYParser::AndCondContext *ctx) = 0;
  virtual void exitAndCond(SysYParser::AndCondContext *ctx) = 0;

  virtual void enterEqCond(SysYParser::EqCondContext *ctx) = 0;
  virtual void exitEqCond(SysYParser::EqCondContext *ctx) = 0;

  virtual void enterLVal(SysYParser::LValContext *ctx) = 0;
  virtual void exitLVal(SysYParser::LValContext *ctx) = 0;

  virtual void enterNumber(SysYParser::NumberContext *ctx) = 0;
  virtual void exitNumber(SysYParser::NumberContext *ctx) = 0;

  virtual void enterUnaryOp(SysYParser::UnaryOpContext *ctx) = 0;
  virtual void exitUnaryOp(SysYParser::UnaryOpContext *ctx) = 0;

  virtual void enterFuncRParams(SysYParser::FuncRParamsContext *ctx) = 0;
  virtual void exitFuncRParams(SysYParser::FuncRParamsContext *ctx) = 0;

  virtual void enterParam(SysYParser::ParamContext *ctx) = 0;
  virtual void exitParam(SysYParser::ParamContext *ctx) = 0;

  virtual void enterConstExp(SysYParser::ConstExpContext *ctx) = 0;
  virtual void exitConstExp(SysYParser::ConstExpContext *ctx) = 0;


};

}  // namespace antlr4
