
// Generated from Sysy.g4 by ANTLR 4.13.2

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

  virtual void enterCompUnitItem(SysyParser::CompUnitItemContext * /*ctx*/) override { }
  virtual void exitCompUnitItem(SysyParser::CompUnitItemContext * /*ctx*/) override { }

  virtual void enterDecl(SysyParser::DeclContext * /*ctx*/) override { }
  virtual void exitDecl(SysyParser::DeclContext * /*ctx*/) override { }

  virtual void enterConstDecl(SysyParser::ConstDeclContext * /*ctx*/) override { }
  virtual void exitConstDecl(SysyParser::ConstDeclContext * /*ctx*/) override { }

  virtual void enterInt(SysyParser::IntContext * /*ctx*/) override { }
  virtual void exitInt(SysyParser::IntContext * /*ctx*/) override { }

  virtual void enterFloat(SysyParser::FloatContext * /*ctx*/) override { }
  virtual void exitFloat(SysyParser::FloatContext * /*ctx*/) override { }

  virtual void enterConstDef(SysyParser::ConstDefContext * /*ctx*/) override { }
  virtual void exitConstDef(SysyParser::ConstDefContext * /*ctx*/) override { }

  virtual void enterVarDecl(SysyParser::VarDeclContext * /*ctx*/) override { }
  virtual void exitVarDecl(SysyParser::VarDeclContext * /*ctx*/) override { }

  virtual void enterVarDef(SysyParser::VarDefContext * /*ctx*/) override { }
  virtual void exitVarDef(SysyParser::VarDefContext * /*ctx*/) override { }

  virtual void enterInit(SysyParser::InitContext * /*ctx*/) override { }
  virtual void exitInit(SysyParser::InitContext * /*ctx*/) override { }

  virtual void enterInitList(SysyParser::InitListContext * /*ctx*/) override { }
  virtual void exitInitList(SysyParser::InitListContext * /*ctx*/) override { }

  virtual void enterFuncDef(SysyParser::FuncDefContext * /*ctx*/) override { }
  virtual void exitFuncDef(SysyParser::FuncDefContext * /*ctx*/) override { }

  virtual void enterFuncType_(SysyParser::FuncType_Context * /*ctx*/) override { }
  virtual void exitFuncType_(SysyParser::FuncType_Context * /*ctx*/) override { }

  virtual void enterVoid(SysyParser::VoidContext * /*ctx*/) override { }
  virtual void exitVoid(SysyParser::VoidContext * /*ctx*/) override { }

  virtual void enterFuncFParams(SysyParser::FuncFParamsContext * /*ctx*/) override { }
  virtual void exitFuncFParams(SysyParser::FuncFParamsContext * /*ctx*/) override { }

  virtual void enterScalarParam(SysyParser::ScalarParamContext * /*ctx*/) override { }
  virtual void exitScalarParam(SysyParser::ScalarParamContext * /*ctx*/) override { }

  virtual void enterArrayParam(SysyParser::ArrayParamContext * /*ctx*/) override { }
  virtual void exitArrayParam(SysyParser::ArrayParamContext * /*ctx*/) override { }

  virtual void enterBlock(SysyParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(SysyParser::BlockContext * /*ctx*/) override { }

  virtual void enterBlockItem(SysyParser::BlockItemContext * /*ctx*/) override { }
  virtual void exitBlockItem(SysyParser::BlockItemContext * /*ctx*/) override { }

  virtual void enterAssign(SysyParser::AssignContext * /*ctx*/) override { }
  virtual void exitAssign(SysyParser::AssignContext * /*ctx*/) override { }

  virtual void enterExprStmt(SysyParser::ExprStmtContext * /*ctx*/) override { }
  virtual void exitExprStmt(SysyParser::ExprStmtContext * /*ctx*/) override { }

  virtual void enterBlockStmt(SysyParser::BlockStmtContext * /*ctx*/) override { }
  virtual void exitBlockStmt(SysyParser::BlockStmtContext * /*ctx*/) override { }

  virtual void enterIfElse(SysyParser::IfElseContext * /*ctx*/) override { }
  virtual void exitIfElse(SysyParser::IfElseContext * /*ctx*/) override { }

  virtual void enterWhile(SysyParser::WhileContext * /*ctx*/) override { }
  virtual void exitWhile(SysyParser::WhileContext * /*ctx*/) override { }

  virtual void enterBreak(SysyParser::BreakContext * /*ctx*/) override { }
  virtual void exitBreak(SysyParser::BreakContext * /*ctx*/) override { }

  virtual void enterContinue(SysyParser::ContinueContext * /*ctx*/) override { }
  virtual void exitContinue(SysyParser::ContinueContext * /*ctx*/) override { }

  virtual void enterReturn(SysyParser::ReturnContext * /*ctx*/) override { }
  virtual void exitReturn(SysyParser::ReturnContext * /*ctx*/) override { }

  virtual void enterExp(SysyParser::ExpContext * /*ctx*/) override { }
  virtual void exitExp(SysyParser::ExpContext * /*ctx*/) override { }

  virtual void enterCond(SysyParser::CondContext * /*ctx*/) override { }
  virtual void exitCond(SysyParser::CondContext * /*ctx*/) override { }

  virtual void enterLVal(SysyParser::LValContext * /*ctx*/) override { }
  virtual void exitLVal(SysyParser::LValContext * /*ctx*/) override { }

  virtual void enterPrimaryExp_(SysyParser::PrimaryExp_Context * /*ctx*/) override { }
  virtual void exitPrimaryExp_(SysyParser::PrimaryExp_Context * /*ctx*/) override { }

  virtual void enterLValExpr(SysyParser::LValExprContext * /*ctx*/) override { }
  virtual void exitLValExpr(SysyParser::LValExprContext * /*ctx*/) override { }

  virtual void enterDecIntConst(SysyParser::DecIntConstContext * /*ctx*/) override { }
  virtual void exitDecIntConst(SysyParser::DecIntConstContext * /*ctx*/) override { }

  virtual void enterOctIntConst(SysyParser::OctIntConstContext * /*ctx*/) override { }
  virtual void exitOctIntConst(SysyParser::OctIntConstContext * /*ctx*/) override { }

  virtual void enterHexIntConst(SysyParser::HexIntConstContext * /*ctx*/) override { }
  virtual void exitHexIntConst(SysyParser::HexIntConstContext * /*ctx*/) override { }

  virtual void enterDecFloatConst(SysyParser::DecFloatConstContext * /*ctx*/) override { }
  virtual void exitDecFloatConst(SysyParser::DecFloatConstContext * /*ctx*/) override { }

  virtual void enterHexFloatConst(SysyParser::HexFloatConstContext * /*ctx*/) override { }
  virtual void exitHexFloatConst(SysyParser::HexFloatConstContext * /*ctx*/) override { }

  virtual void enterNumber(SysyParser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(SysyParser::NumberContext * /*ctx*/) override { }

  virtual void enterUnaryExp_(SysyParser::UnaryExp_Context * /*ctx*/) override { }
  virtual void exitUnaryExp_(SysyParser::UnaryExp_Context * /*ctx*/) override { }

  virtual void enterCall(SysyParser::CallContext * /*ctx*/) override { }
  virtual void exitCall(SysyParser::CallContext * /*ctx*/) override { }

  virtual void enterUnaryAdd(SysyParser::UnaryAddContext * /*ctx*/) override { }
  virtual void exitUnaryAdd(SysyParser::UnaryAddContext * /*ctx*/) override { }

  virtual void enterUnarySub(SysyParser::UnarySubContext * /*ctx*/) override { }
  virtual void exitUnarySub(SysyParser::UnarySubContext * /*ctx*/) override { }

  virtual void enterNot(SysyParser::NotContext * /*ctx*/) override { }
  virtual void exitNot(SysyParser::NotContext * /*ctx*/) override { }

  virtual void enterStringConst(SysyParser::StringConstContext * /*ctx*/) override { }
  virtual void exitStringConst(SysyParser::StringConstContext * /*ctx*/) override { }

  virtual void enterFuncRParam(SysyParser::FuncRParamContext * /*ctx*/) override { }
  virtual void exitFuncRParam(SysyParser::FuncRParamContext * /*ctx*/) override { }

  virtual void enterFuncRParams(SysyParser::FuncRParamsContext * /*ctx*/) override { }
  virtual void exitFuncRParams(SysyParser::FuncRParamsContext * /*ctx*/) override { }

  virtual void enterDiv(SysyParser::DivContext * /*ctx*/) override { }
  virtual void exitDiv(SysyParser::DivContext * /*ctx*/) override { }

  virtual void enterMod(SysyParser::ModContext * /*ctx*/) override { }
  virtual void exitMod(SysyParser::ModContext * /*ctx*/) override { }

  virtual void enterMul(SysyParser::MulContext * /*ctx*/) override { }
  virtual void exitMul(SysyParser::MulContext * /*ctx*/) override { }

  virtual void enterMulExp_(SysyParser::MulExp_Context * /*ctx*/) override { }
  virtual void exitMulExp_(SysyParser::MulExp_Context * /*ctx*/) override { }

  virtual void enterAddExp_(SysyParser::AddExp_Context * /*ctx*/) override { }
  virtual void exitAddExp_(SysyParser::AddExp_Context * /*ctx*/) override { }

  virtual void enterAdd(SysyParser::AddContext * /*ctx*/) override { }
  virtual void exitAdd(SysyParser::AddContext * /*ctx*/) override { }

  virtual void enterSub(SysyParser::SubContext * /*ctx*/) override { }
  virtual void exitSub(SysyParser::SubContext * /*ctx*/) override { }

  virtual void enterGeq(SysyParser::GeqContext * /*ctx*/) override { }
  virtual void exitGeq(SysyParser::GeqContext * /*ctx*/) override { }

  virtual void enterLt(SysyParser::LtContext * /*ctx*/) override { }
  virtual void exitLt(SysyParser::LtContext * /*ctx*/) override { }

  virtual void enterRelExp_(SysyParser::RelExp_Context * /*ctx*/) override { }
  virtual void exitRelExp_(SysyParser::RelExp_Context * /*ctx*/) override { }

  virtual void enterLeq(SysyParser::LeqContext * /*ctx*/) override { }
  virtual void exitLeq(SysyParser::LeqContext * /*ctx*/) override { }

  virtual void enterGt(SysyParser::GtContext * /*ctx*/) override { }
  virtual void exitGt(SysyParser::GtContext * /*ctx*/) override { }

  virtual void enterNeq(SysyParser::NeqContext * /*ctx*/) override { }
  virtual void exitNeq(SysyParser::NeqContext * /*ctx*/) override { }

  virtual void enterEq(SysyParser::EqContext * /*ctx*/) override { }
  virtual void exitEq(SysyParser::EqContext * /*ctx*/) override { }

  virtual void enterEqExp_(SysyParser::EqExp_Context * /*ctx*/) override { }
  virtual void exitEqExp_(SysyParser::EqExp_Context * /*ctx*/) override { }

  virtual void enterLAndExp_(SysyParser::LAndExp_Context * /*ctx*/) override { }
  virtual void exitLAndExp_(SysyParser::LAndExp_Context * /*ctx*/) override { }

  virtual void enterAnd(SysyParser::AndContext * /*ctx*/) override { }
  virtual void exitAnd(SysyParser::AndContext * /*ctx*/) override { }

  virtual void enterOr(SysyParser::OrContext * /*ctx*/) override { }
  virtual void exitOr(SysyParser::OrContext * /*ctx*/) override { }

  virtual void enterLOrExp_(SysyParser::LOrExp_Context * /*ctx*/) override { }
  virtual void exitLOrExp_(SysyParser::LOrExp_Context * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

