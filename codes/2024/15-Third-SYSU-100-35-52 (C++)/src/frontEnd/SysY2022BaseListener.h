
// Generated from SysY2022.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "SysY2022Listener.h"


/**
 * This class provides an empty implementation of SysY2022Listener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  SysY2022BaseListener : public SysY2022Listener {
public:

  virtual void enterCompUnit(SysY2022Parser::CompUnitContext * /*ctx*/) override { }
  virtual void exitCompUnit(SysY2022Parser::CompUnitContext * /*ctx*/) override { }

  virtual void enterDecl(SysY2022Parser::DeclContext * /*ctx*/) override { }
  virtual void exitDecl(SysY2022Parser::DeclContext * /*ctx*/) override { }

  virtual void enterConstDecl(SysY2022Parser::ConstDeclContext * /*ctx*/) override { }
  virtual void exitConstDecl(SysY2022Parser::ConstDeclContext * /*ctx*/) override { }

  virtual void enterBType(SysY2022Parser::BTypeContext * /*ctx*/) override { }
  virtual void exitBType(SysY2022Parser::BTypeContext * /*ctx*/) override { }

  virtual void enterConstDef(SysY2022Parser::ConstDefContext * /*ctx*/) override { }
  virtual void exitConstDef(SysY2022Parser::ConstDefContext * /*ctx*/) override { }

  virtual void enterItemConstInitVal(SysY2022Parser::ItemConstInitValContext * /*ctx*/) override { }
  virtual void exitItemConstInitVal(SysY2022Parser::ItemConstInitValContext * /*ctx*/) override { }

  virtual void enterListConstInitVal(SysY2022Parser::ListConstInitValContext * /*ctx*/) override { }
  virtual void exitListConstInitVal(SysY2022Parser::ListConstInitValContext * /*ctx*/) override { }

  virtual void enterVarDecl(SysY2022Parser::VarDeclContext * /*ctx*/) override { }
  virtual void exitVarDecl(SysY2022Parser::VarDeclContext * /*ctx*/) override { }

  virtual void enterVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext * /*ctx*/) override { }
  virtual void exitVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext * /*ctx*/) override { }

  virtual void enterVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext * /*ctx*/) override { }
  virtual void exitVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext * /*ctx*/) override { }

  virtual void enterItemInitVal(SysY2022Parser::ItemInitValContext * /*ctx*/) override { }
  virtual void exitItemInitVal(SysY2022Parser::ItemInitValContext * /*ctx*/) override { }

  virtual void enterListInitVal(SysY2022Parser::ListInitValContext * /*ctx*/) override { }
  virtual void exitListInitVal(SysY2022Parser::ListInitValContext * /*ctx*/) override { }

  virtual void enterFuncDef(SysY2022Parser::FuncDefContext * /*ctx*/) override { }
  virtual void exitFuncDef(SysY2022Parser::FuncDefContext * /*ctx*/) override { }

  virtual void enterFuncType(SysY2022Parser::FuncTypeContext * /*ctx*/) override { }
  virtual void exitFuncType(SysY2022Parser::FuncTypeContext * /*ctx*/) override { }

  virtual void enterFuncFParams(SysY2022Parser::FuncFParamsContext * /*ctx*/) override { }
  virtual void exitFuncFParams(SysY2022Parser::FuncFParamsContext * /*ctx*/) override { }

  virtual void enterFuncFParam(SysY2022Parser::FuncFParamContext * /*ctx*/) override { }
  virtual void exitFuncFParam(SysY2022Parser::FuncFParamContext * /*ctx*/) override { }

  virtual void enterBlock(SysY2022Parser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(SysY2022Parser::BlockContext * /*ctx*/) override { }

  virtual void enterBlockItem(SysY2022Parser::BlockItemContext * /*ctx*/) override { }
  virtual void exitBlockItem(SysY2022Parser::BlockItemContext * /*ctx*/) override { }

  virtual void enterAssignStmt(SysY2022Parser::AssignStmtContext * /*ctx*/) override { }
  virtual void exitAssignStmt(SysY2022Parser::AssignStmtContext * /*ctx*/) override { }

  virtual void enterExpStmt(SysY2022Parser::ExpStmtContext * /*ctx*/) override { }
  virtual void exitExpStmt(SysY2022Parser::ExpStmtContext * /*ctx*/) override { }

  virtual void enterBlockStmt(SysY2022Parser::BlockStmtContext * /*ctx*/) override { }
  virtual void exitBlockStmt(SysY2022Parser::BlockStmtContext * /*ctx*/) override { }

  virtual void enterIfStmt(SysY2022Parser::IfStmtContext * /*ctx*/) override { }
  virtual void exitIfStmt(SysY2022Parser::IfStmtContext * /*ctx*/) override { }

  virtual void enterIfElseStmt(SysY2022Parser::IfElseStmtContext * /*ctx*/) override { }
  virtual void exitIfElseStmt(SysY2022Parser::IfElseStmtContext * /*ctx*/) override { }

  virtual void enterWhileStmt(SysY2022Parser::WhileStmtContext * /*ctx*/) override { }
  virtual void exitWhileStmt(SysY2022Parser::WhileStmtContext * /*ctx*/) override { }

  virtual void enterBreakStmt(SysY2022Parser::BreakStmtContext * /*ctx*/) override { }
  virtual void exitBreakStmt(SysY2022Parser::BreakStmtContext * /*ctx*/) override { }

  virtual void enterContinueStmt(SysY2022Parser::ContinueStmtContext * /*ctx*/) override { }
  virtual void exitContinueStmt(SysY2022Parser::ContinueStmtContext * /*ctx*/) override { }

  virtual void enterReturnStmt(SysY2022Parser::ReturnStmtContext * /*ctx*/) override { }
  virtual void exitReturnStmt(SysY2022Parser::ReturnStmtContext * /*ctx*/) override { }

  virtual void enterExp(SysY2022Parser::ExpContext * /*ctx*/) override { }
  virtual void exitExp(SysY2022Parser::ExpContext * /*ctx*/) override { }

  virtual void enterCond(SysY2022Parser::CondContext * /*ctx*/) override { }
  virtual void exitCond(SysY2022Parser::CondContext * /*ctx*/) override { }

  virtual void enterLVal(SysY2022Parser::LValContext * /*ctx*/) override { }
  virtual void exitLVal(SysY2022Parser::LValContext * /*ctx*/) override { }

  virtual void enterExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext * /*ctx*/) override { }
  virtual void exitExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext * /*ctx*/) override { }

  virtual void enterLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext * /*ctx*/) override { }
  virtual void exitLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext * /*ctx*/) override { }

  virtual void enterNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext * /*ctx*/) override { }
  virtual void exitNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext * /*ctx*/) override { }

  virtual void enterNumber(SysY2022Parser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(SysY2022Parser::NumberContext * /*ctx*/) override { }

  virtual void enterPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext * /*ctx*/) override { }
  virtual void exitPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext * /*ctx*/) override { }

  virtual void enterFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext * /*ctx*/) override { }
  virtual void exitFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext * /*ctx*/) override { }

  virtual void enterUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext * /*ctx*/) override { }
  virtual void exitUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext * /*ctx*/) override { }

  virtual void enterUnaryOp(SysY2022Parser::UnaryOpContext * /*ctx*/) override { }
  virtual void exitUnaryOp(SysY2022Parser::UnaryOpContext * /*ctx*/) override { }

  virtual void enterFuncRParams(SysY2022Parser::FuncRParamsContext * /*ctx*/) override { }
  virtual void exitFuncRParams(SysY2022Parser::FuncRParamsContext * /*ctx*/) override { }

  virtual void enterFuncRParam(SysY2022Parser::FuncRParamContext * /*ctx*/) override { }
  virtual void exitFuncRParam(SysY2022Parser::FuncRParamContext * /*ctx*/) override { }

  virtual void enterMulMulExp(SysY2022Parser::MulMulExpContext * /*ctx*/) override { }
  virtual void exitMulMulExp(SysY2022Parser::MulMulExpContext * /*ctx*/) override { }

  virtual void enterUnaryMulExp(SysY2022Parser::UnaryMulExpContext * /*ctx*/) override { }
  virtual void exitUnaryMulExp(SysY2022Parser::UnaryMulExpContext * /*ctx*/) override { }

  virtual void enterAddAddExp(SysY2022Parser::AddAddExpContext * /*ctx*/) override { }
  virtual void exitAddAddExp(SysY2022Parser::AddAddExpContext * /*ctx*/) override { }

  virtual void enterMulAddExp(SysY2022Parser::MulAddExpContext * /*ctx*/) override { }
  virtual void exitMulAddExp(SysY2022Parser::MulAddExpContext * /*ctx*/) override { }

  virtual void enterRelRelExp(SysY2022Parser::RelRelExpContext * /*ctx*/) override { }
  virtual void exitRelRelExp(SysY2022Parser::RelRelExpContext * /*ctx*/) override { }

  virtual void enterAddRelExp(SysY2022Parser::AddRelExpContext * /*ctx*/) override { }
  virtual void exitAddRelExp(SysY2022Parser::AddRelExpContext * /*ctx*/) override { }

  virtual void enterEqEqExp(SysY2022Parser::EqEqExpContext * /*ctx*/) override { }
  virtual void exitEqEqExp(SysY2022Parser::EqEqExpContext * /*ctx*/) override { }

  virtual void enterRelEqExp(SysY2022Parser::RelEqExpContext * /*ctx*/) override { }
  virtual void exitRelEqExp(SysY2022Parser::RelEqExpContext * /*ctx*/) override { }

  virtual void enterEqLAndExp(SysY2022Parser::EqLAndExpContext * /*ctx*/) override { }
  virtual void exitEqLAndExp(SysY2022Parser::EqLAndExpContext * /*ctx*/) override { }

  virtual void enterLAndLAndExp(SysY2022Parser::LAndLAndExpContext * /*ctx*/) override { }
  virtual void exitLAndLAndExp(SysY2022Parser::LAndLAndExpContext * /*ctx*/) override { }

  virtual void enterLAndLOrExp(SysY2022Parser::LAndLOrExpContext * /*ctx*/) override { }
  virtual void exitLAndLOrExp(SysY2022Parser::LAndLOrExpContext * /*ctx*/) override { }

  virtual void enterLOrLOrExp(SysY2022Parser::LOrLOrExpContext * /*ctx*/) override { }
  virtual void exitLOrLOrExp(SysY2022Parser::LOrLOrExpContext * /*ctx*/) override { }

  virtual void enterConstExp(SysY2022Parser::ConstExpContext * /*ctx*/) override { }
  virtual void exitConstExp(SysY2022Parser::ConstExpContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

