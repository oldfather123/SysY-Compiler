
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyListener.h"


/**
 * This class provides an empty implementation of SysyListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  SysyBaseListener : public SysyListener {
public:

  virtual void enterCompUnit(SysyParser::CompUnitContext * /*ctx*/) override { }
  virtual void exitCompUnit(SysyParser::CompUnitContext * /*ctx*/) override { }

  virtual void enterDecl(SysyParser::DeclContext * /*ctx*/) override { }
  virtual void exitDecl(SysyParser::DeclContext * /*ctx*/) override { }

  virtual void enterConstDecl(SysyParser::ConstDeclContext * /*ctx*/) override { }
  virtual void exitConstDecl(SysyParser::ConstDeclContext * /*ctx*/) override { }

  virtual void enterBType(SysyParser::BTypeContext * /*ctx*/) override { }
  virtual void exitBType(SysyParser::BTypeContext * /*ctx*/) override { }

  virtual void enterConstDef(SysyParser::ConstDefContext * /*ctx*/) override { }
  virtual void exitConstDef(SysyParser::ConstDefContext * /*ctx*/) override { }

  virtual void enterSingleConstInitVal(SysyParser::SingleConstInitValContext * /*ctx*/) override { }
  virtual void exitSingleConstInitVal(SysyParser::SingleConstInitValContext * /*ctx*/) override { }

  virtual void enterMultiConstInitVal(SysyParser::MultiConstInitValContext * /*ctx*/) override { }
  virtual void exitMultiConstInitVal(SysyParser::MultiConstInitValContext * /*ctx*/) override { }

  virtual void enterVarDecl(SysyParser::VarDeclContext * /*ctx*/) override { }
  virtual void exitVarDecl(SysyParser::VarDeclContext * /*ctx*/) override { }

  virtual void enterNoinitVarDef(SysyParser::NoinitVarDefContext * /*ctx*/) override { }
  virtual void exitNoinitVarDef(SysyParser::NoinitVarDefContext * /*ctx*/) override { }

  virtual void enterInitVarDef(SysyParser::InitVarDefContext * /*ctx*/) override { }
  virtual void exitInitVarDef(SysyParser::InitVarDefContext * /*ctx*/) override { }

  virtual void enterSingleInitVal(SysyParser::SingleInitValContext * /*ctx*/) override { }
  virtual void exitSingleInitVal(SysyParser::SingleInitValContext * /*ctx*/) override { }

  virtual void enterMultiInitVal(SysyParser::MultiInitValContext * /*ctx*/) override { }
  virtual void exitMultiInitVal(SysyParser::MultiInitValContext * /*ctx*/) override { }

  virtual void enterFuncDef(SysyParser::FuncDefContext * /*ctx*/) override { }
  virtual void exitFuncDef(SysyParser::FuncDefContext * /*ctx*/) override { }

  virtual void enterFuncType(SysyParser::FuncTypeContext * /*ctx*/) override { }
  virtual void exitFuncType(SysyParser::FuncTypeContext * /*ctx*/) override { }

  virtual void enterFuncFParams(SysyParser::FuncFParamsContext * /*ctx*/) override { }
  virtual void exitFuncFParams(SysyParser::FuncFParamsContext * /*ctx*/) override { }

  virtual void enterFuncFParam(SysyParser::FuncFParamContext * /*ctx*/) override { }
  virtual void exitFuncFParam(SysyParser::FuncFParamContext * /*ctx*/) override { }

  virtual void enterBlock(SysyParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(SysyParser::BlockContext * /*ctx*/) override { }

  virtual void enterBlockItem(SysyParser::BlockItemContext * /*ctx*/) override { }
  virtual void exitBlockItem(SysyParser::BlockItemContext * /*ctx*/) override { }

  virtual void enterAssignStmt(SysyParser::AssignStmtContext * /*ctx*/) override { }
  virtual void exitAssignStmt(SysyParser::AssignStmtContext * /*ctx*/) override { }

  virtual void enterExpStmt(SysyParser::ExpStmtContext * /*ctx*/) override { }
  virtual void exitExpStmt(SysyParser::ExpStmtContext * /*ctx*/) override { }

  virtual void enterBlockStmt(SysyParser::BlockStmtContext * /*ctx*/) override { }
  virtual void exitBlockStmt(SysyParser::BlockStmtContext * /*ctx*/) override { }

  virtual void enterIfStmt(SysyParser::IfStmtContext * /*ctx*/) override { }
  virtual void exitIfStmt(SysyParser::IfStmtContext * /*ctx*/) override { }

  virtual void enterIfElseStmt(SysyParser::IfElseStmtContext * /*ctx*/) override { }
  virtual void exitIfElseStmt(SysyParser::IfElseStmtContext * /*ctx*/) override { }

  virtual void enterWhileStmt(SysyParser::WhileStmtContext * /*ctx*/) override { }
  virtual void exitWhileStmt(SysyParser::WhileStmtContext * /*ctx*/) override { }

  virtual void enterBreakStmt(SysyParser::BreakStmtContext * /*ctx*/) override { }
  virtual void exitBreakStmt(SysyParser::BreakStmtContext * /*ctx*/) override { }

  virtual void enterContinueStmt(SysyParser::ContinueStmtContext * /*ctx*/) override { }
  virtual void exitContinueStmt(SysyParser::ContinueStmtContext * /*ctx*/) override { }

  virtual void enterReturnStmt(SysyParser::ReturnStmtContext * /*ctx*/) override { }
  virtual void exitReturnStmt(SysyParser::ReturnStmtContext * /*ctx*/) override { }

  virtual void enterExp(SysyParser::ExpContext * /*ctx*/) override { }
  virtual void exitExp(SysyParser::ExpContext * /*ctx*/) override { }

  virtual void enterCond(SysyParser::CondContext * /*ctx*/) override { }
  virtual void exitCond(SysyParser::CondContext * /*ctx*/) override { }

  virtual void enterLVal(SysyParser::LValContext * /*ctx*/) override { }
  virtual void exitLVal(SysyParser::LValContext * /*ctx*/) override { }

  virtual void enterParensPrimaryExp(SysyParser::ParensPrimaryExpContext * /*ctx*/) override { }
  virtual void exitParensPrimaryExp(SysyParser::ParensPrimaryExpContext * /*ctx*/) override { }

  virtual void enterLValPrimaryExp(SysyParser::LValPrimaryExpContext * /*ctx*/) override { }
  virtual void exitLValPrimaryExp(SysyParser::LValPrimaryExpContext * /*ctx*/) override { }

  virtual void enterNumberPrimaryExp(SysyParser::NumberPrimaryExpContext * /*ctx*/) override { }
  virtual void exitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext * /*ctx*/) override { }

  virtual void enterNumber(SysyParser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(SysyParser::NumberContext * /*ctx*/) override { }

  virtual void enterPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext * /*ctx*/) override { }
  virtual void exitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext * /*ctx*/) override { }

  virtual void enterFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext * /*ctx*/) override { }
  virtual void exitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext * /*ctx*/) override { }

  virtual void enterUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext * /*ctx*/) override { }
  virtual void exitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext * /*ctx*/) override { }

  virtual void enterUnaryOp(SysyParser::UnaryOpContext * /*ctx*/) override { }
  virtual void exitUnaryOp(SysyParser::UnaryOpContext * /*ctx*/) override { }

  virtual void enterFuncRParams(SysyParser::FuncRParamsContext * /*ctx*/) override { }
  virtual void exitFuncRParams(SysyParser::FuncRParamsContext * /*ctx*/) override { }

  virtual void enterMulMulExp(SysyParser::MulMulExpContext * /*ctx*/) override { }
  virtual void exitMulMulExp(SysyParser::MulMulExpContext * /*ctx*/) override { }

  virtual void enterUnaryMulExp(SysyParser::UnaryMulExpContext * /*ctx*/) override { }
  virtual void exitUnaryMulExp(SysyParser::UnaryMulExpContext * /*ctx*/) override { }

  virtual void enterAddAddExp(SysyParser::AddAddExpContext * /*ctx*/) override { }
  virtual void exitAddAddExp(SysyParser::AddAddExpContext * /*ctx*/) override { }

  virtual void enterMulAddExp(SysyParser::MulAddExpContext * /*ctx*/) override { }
  virtual void exitMulAddExp(SysyParser::MulAddExpContext * /*ctx*/) override { }

  virtual void enterRelRelExp(SysyParser::RelRelExpContext * /*ctx*/) override { }
  virtual void exitRelRelExp(SysyParser::RelRelExpContext * /*ctx*/) override { }

  virtual void enterAddRelExp(SysyParser::AddRelExpContext * /*ctx*/) override { }
  virtual void exitAddRelExp(SysyParser::AddRelExpContext * /*ctx*/) override { }

  virtual void enterEqEqExp(SysyParser::EqEqExpContext * /*ctx*/) override { }
  virtual void exitEqEqExp(SysyParser::EqEqExpContext * /*ctx*/) override { }

  virtual void enterRelEqExp(SysyParser::RelEqExpContext * /*ctx*/) override { }
  virtual void exitRelEqExp(SysyParser::RelEqExpContext * /*ctx*/) override { }

  virtual void enterEqLAndExp(SysyParser::EqLAndExpContext * /*ctx*/) override { }
  virtual void exitEqLAndExp(SysyParser::EqLAndExpContext * /*ctx*/) override { }

  virtual void enterLAndLAndExp(SysyParser::LAndLAndExpContext * /*ctx*/) override { }
  virtual void exitLAndLAndExp(SysyParser::LAndLAndExpContext * /*ctx*/) override { }

  virtual void enterLAndLOrExp(SysyParser::LAndLOrExpContext * /*ctx*/) override { }
  virtual void exitLAndLOrExp(SysyParser::LAndLOrExpContext * /*ctx*/) override { }

  virtual void enterLOrLOrExp(SysyParser::LOrLOrExpContext * /*ctx*/) override { }
  virtual void exitLOrLOrExp(SysyParser::LOrLOrExpContext * /*ctx*/) override { }

  virtual void enterConstExp(SysyParser::ConstExpContext * /*ctx*/) override { }
  virtual void exitConstExp(SysyParser::ConstExpContext * /*ctx*/) override { }

  virtual void enterEpsilon(SysyParser::EpsilonContext * /*ctx*/) override { }
  virtual void exitEpsilon(SysyParser::EpsilonContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

