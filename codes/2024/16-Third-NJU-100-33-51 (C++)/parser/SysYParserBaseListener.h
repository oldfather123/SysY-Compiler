
// Generated from ./parser/SysYParser.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysYParserListener.h"


/**
 * This class provides an empty implementation of SysYParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  SysYParserBaseListener : public SysYParserListener {
public:

  virtual void enterProgram(SysYParserParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(SysYParserParser::ProgramContext * /*ctx*/) override { }

  virtual void enterCompUnit(SysYParserParser::CompUnitContext * /*ctx*/) override { }
  virtual void exitCompUnit(SysYParserParser::CompUnitContext * /*ctx*/) override { }

  virtual void enterDecl(SysYParserParser::DeclContext * /*ctx*/) override { }
  virtual void exitDecl(SysYParserParser::DeclContext * /*ctx*/) override { }

  virtual void enterConstDecl(SysYParserParser::ConstDeclContext * /*ctx*/) override { }
  virtual void exitConstDecl(SysYParserParser::ConstDeclContext * /*ctx*/) override { }

  virtual void enterBType(SysYParserParser::BTypeContext * /*ctx*/) override { }
  virtual void exitBType(SysYParserParser::BTypeContext * /*ctx*/) override { }

  virtual void enterConstDefSingle(SysYParserParser::ConstDefSingleContext * /*ctx*/) override { }
  virtual void exitConstDefSingle(SysYParserParser::ConstDefSingleContext * /*ctx*/) override { }

  virtual void enterConstDefArray(SysYParserParser::ConstDefArrayContext * /*ctx*/) override { }
  virtual void exitConstDefArray(SysYParserParser::ConstDefArrayContext * /*ctx*/) override { }

  virtual void enterConstInitValSingle(SysYParserParser::ConstInitValSingleContext * /*ctx*/) override { }
  virtual void exitConstInitValSingle(SysYParserParser::ConstInitValSingleContext * /*ctx*/) override { }

  virtual void enterConstInitValArray(SysYParserParser::ConstInitValArrayContext * /*ctx*/) override { }
  virtual void exitConstInitValArray(SysYParserParser::ConstInitValArrayContext * /*ctx*/) override { }

  virtual void enterVarDecl(SysYParserParser::VarDeclContext * /*ctx*/) override { }
  virtual void exitVarDecl(SysYParserParser::VarDeclContext * /*ctx*/) override { }

  virtual void enterVarDefSingle(SysYParserParser::VarDefSingleContext * /*ctx*/) override { }
  virtual void exitVarDefSingle(SysYParserParser::VarDefSingleContext * /*ctx*/) override { }

  virtual void enterVarDefArray(SysYParserParser::VarDefArrayContext * /*ctx*/) override { }
  virtual void exitVarDefArray(SysYParserParser::VarDefArrayContext * /*ctx*/) override { }

  virtual void enterVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext * /*ctx*/) override { }
  virtual void exitVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext * /*ctx*/) override { }

  virtual void enterVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext * /*ctx*/) override { }
  virtual void exitVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext * /*ctx*/) override { }

  virtual void enterInitLVal(SysYParserParser::InitLValContext * /*ctx*/) override { }
  virtual void exitInitLVal(SysYParserParser::InitLValContext * /*ctx*/) override { }

  virtual void enterInitValSingle(SysYParserParser::InitValSingleContext * /*ctx*/) override { }
  virtual void exitInitValSingle(SysYParserParser::InitValSingleContext * /*ctx*/) override { }

  virtual void enterInitValArray(SysYParserParser::InitValArrayContext * /*ctx*/) override { }
  virtual void exitInitValArray(SysYParserParser::InitValArrayContext * /*ctx*/) override { }

  virtual void enterFuncDef(SysYParserParser::FuncDefContext * /*ctx*/) override { }
  virtual void exitFuncDef(SysYParserParser::FuncDefContext * /*ctx*/) override { }

  virtual void enterFuncType(SysYParserParser::FuncTypeContext * /*ctx*/) override { }
  virtual void exitFuncType(SysYParserParser::FuncTypeContext * /*ctx*/) override { }

  virtual void enterFuncFParams(SysYParserParser::FuncFParamsContext * /*ctx*/) override { }
  virtual void exitFuncFParams(SysYParserParser::FuncFParamsContext * /*ctx*/) override { }

  virtual void enterFuncFParamSingle(SysYParserParser::FuncFParamSingleContext * /*ctx*/) override { }
  virtual void exitFuncFParamSingle(SysYParserParser::FuncFParamSingleContext * /*ctx*/) override { }

  virtual void enterFuncFParamArray(SysYParserParser::FuncFParamArrayContext * /*ctx*/) override { }
  virtual void exitFuncFParamArray(SysYParserParser::FuncFParamArrayContext * /*ctx*/) override { }

  virtual void enterBlock(SysYParserParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(SysYParserParser::BlockContext * /*ctx*/) override { }

  virtual void enterBlockItem(SysYParserParser::BlockItemContext * /*ctx*/) override { }
  virtual void exitBlockItem(SysYParserParser::BlockItemContext * /*ctx*/) override { }

  virtual void enterStmtAssign(SysYParserParser::StmtAssignContext * /*ctx*/) override { }
  virtual void exitStmtAssign(SysYParserParser::StmtAssignContext * /*ctx*/) override { }

  virtual void enterStmtExp(SysYParserParser::StmtExpContext * /*ctx*/) override { }
  virtual void exitStmtExp(SysYParserParser::StmtExpContext * /*ctx*/) override { }

  virtual void enterStmtBlock(SysYParserParser::StmtBlockContext * /*ctx*/) override { }
  virtual void exitStmtBlock(SysYParserParser::StmtBlockContext * /*ctx*/) override { }

  virtual void enterStmtCond(SysYParserParser::StmtCondContext * /*ctx*/) override { }
  virtual void exitStmtCond(SysYParserParser::StmtCondContext * /*ctx*/) override { }

  virtual void enterStmtWhile(SysYParserParser::StmtWhileContext * /*ctx*/) override { }
  virtual void exitStmtWhile(SysYParserParser::StmtWhileContext * /*ctx*/) override { }

  virtual void enterStmtBreak(SysYParserParser::StmtBreakContext * /*ctx*/) override { }
  virtual void exitStmtBreak(SysYParserParser::StmtBreakContext * /*ctx*/) override { }

  virtual void enterStmtContinue(SysYParserParser::StmtContinueContext * /*ctx*/) override { }
  virtual void exitStmtContinue(SysYParserParser::StmtContinueContext * /*ctx*/) override { }

  virtual void enterStmtReturn(SysYParserParser::StmtReturnContext * /*ctx*/) override { }
  virtual void exitStmtReturn(SysYParserParser::StmtReturnContext * /*ctx*/) override { }

  virtual void enterExp(SysYParserParser::ExpContext * /*ctx*/) override { }
  virtual void exitExp(SysYParserParser::ExpContext * /*ctx*/) override { }

  virtual void enterCond(SysYParserParser::CondContext * /*ctx*/) override { }
  virtual void exitCond(SysYParserParser::CondContext * /*ctx*/) override { }

  virtual void enterLValSingle(SysYParserParser::LValSingleContext * /*ctx*/) override { }
  virtual void exitLValSingle(SysYParserParser::LValSingleContext * /*ctx*/) override { }

  virtual void enterLValArray(SysYParserParser::LValArrayContext * /*ctx*/) override { }
  virtual void exitLValArray(SysYParserParser::LValArrayContext * /*ctx*/) override { }

  virtual void enterPrimaryExpParen(SysYParserParser::PrimaryExpParenContext * /*ctx*/) override { }
  virtual void exitPrimaryExpParen(SysYParserParser::PrimaryExpParenContext * /*ctx*/) override { }

  virtual void enterPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext * /*ctx*/) override { }
  virtual void exitPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext * /*ctx*/) override { }

  virtual void enterPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext * /*ctx*/) override { }
  virtual void exitPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext * /*ctx*/) override { }

  virtual void enterNumber(SysYParserParser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(SysYParserParser::NumberContext * /*ctx*/) override { }

  virtual void enterUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext * /*ctx*/) override { }
  virtual void exitUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext * /*ctx*/) override { }

  virtual void enterUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext * /*ctx*/) override { }
  virtual void exitUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext * /*ctx*/) override { }

  virtual void enterUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext * /*ctx*/) override { }
  virtual void exitUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext * /*ctx*/) override { }

  virtual void enterUnaryOp(SysYParserParser::UnaryOpContext * /*ctx*/) override { }
  virtual void exitUnaryOp(SysYParserParser::UnaryOpContext * /*ctx*/) override { }

  virtual void enterFuncRParams(SysYParserParser::FuncRParamsContext * /*ctx*/) override { }
  virtual void exitFuncRParams(SysYParserParser::FuncRParamsContext * /*ctx*/) override { }

  virtual void enterFuncRParam(SysYParserParser::FuncRParamContext * /*ctx*/) override { }
  virtual void exitFuncRParam(SysYParserParser::FuncRParamContext * /*ctx*/) override { }

  virtual void enterMulExp(SysYParserParser::MulExpContext * /*ctx*/) override { }
  virtual void exitMulExp(SysYParserParser::MulExpContext * /*ctx*/) override { }

  virtual void enterAddExp(SysYParserParser::AddExpContext * /*ctx*/) override { }
  virtual void exitAddExp(SysYParserParser::AddExpContext * /*ctx*/) override { }

  virtual void enterRelExp(SysYParserParser::RelExpContext * /*ctx*/) override { }
  virtual void exitRelExp(SysYParserParser::RelExpContext * /*ctx*/) override { }

  virtual void enterEqExp(SysYParserParser::EqExpContext * /*ctx*/) override { }
  virtual void exitEqExp(SysYParserParser::EqExpContext * /*ctx*/) override { }

  virtual void enterLAndExp(SysYParserParser::LAndExpContext * /*ctx*/) override { }
  virtual void exitLAndExp(SysYParserParser::LAndExpContext * /*ctx*/) override { }

  virtual void enterLOrExp(SysYParserParser::LOrExpContext * /*ctx*/) override { }
  virtual void exitLOrExp(SysYParserParser::LOrExpContext * /*ctx*/) override { }

  virtual void enterConstExp(SysYParserParser::ConstExpContext * /*ctx*/) override { }
  virtual void exitConstExp(SysYParserParser::ConstExpContext * /*ctx*/) override { }

  virtual void enterIntDecConst(SysYParserParser::IntDecConstContext * /*ctx*/) override { }
  virtual void exitIntDecConst(SysYParserParser::IntDecConstContext * /*ctx*/) override { }

  virtual void enterIntOctConst(SysYParserParser::IntOctConstContext * /*ctx*/) override { }
  virtual void exitIntOctConst(SysYParserParser::IntOctConstContext * /*ctx*/) override { }

  virtual void enterIntHexConst(SysYParserParser::IntHexConstContext * /*ctx*/) override { }
  virtual void exitIntHexConst(SysYParserParser::IntHexConstContext * /*ctx*/) override { }

  virtual void enterFloatConst(SysYParserParser::FloatConstContext * /*ctx*/) override { }
  virtual void exitFloatConst(SysYParserParser::FloatConstContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

