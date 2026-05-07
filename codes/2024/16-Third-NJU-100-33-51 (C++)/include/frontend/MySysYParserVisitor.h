//
// Created by yanayn on 7/10/24.
//

#ifndef ANTPIE_MYSYSYPARSERVISITOR_H
#define ANTPIE_MYSYSYPARSERVISITOR_H

#include <cmath>
#include <functional>

#include "AggregateValue.hh"
#include "Constant.hh"
#include "Module.hh"
#include "SysYParserBaseVisitor.h"
#include "Type.hh"
#include "VariableTable.h"
#include "antlr4-runtime.h"

#define any antlrcpp::Any

typedef struct whileBlockInfo {
 public:
  BasicBlock* condBlock;
  BasicBlock* exitBlock;
  whileBlockInfo(BasicBlock* cond, BasicBlock* exit) {
    this->condBlock = cond;
    this->exitBlock = exit;
  };
} WhileBlockInfo;

using ANTPIE::Module;
using std::stack;
using std::stof;
using std::stoi;
using std::string;
using std::vector;

class MySysYParserVisitor : SysYParserBaseVisitor {
 public:
  Module module;
  VariableTable* current;
  Type* currDefType;  // 当前的定义的类型，在多重定义的时候使用
  Value* currentRet;
  Type* currentRetType;
  Type* functionRetType;
  BasicBlock* trueBasicBlock;
  BasicBlock* falseBasicBlock;
  stack<WhileBlockInfo*> loopLayer;
  Function* currentIn;
  AggregateValue* currAggregate;
  map<string, OpTag> Integer_Operator_OpTag_Table;
  map<string, OpTag> F_Operator_OpTag_Table;
  map<string, OpTag> Logic_Operator_OpTag_Table;
  map<string, Function*> extern_function_map;

  MySysYParserVisitor(VariableTable* current);
  void setArrayInitVal(Value* arr, ArrayConstant* arrayValue);
  void setModule(Module m);
  void init_extern_function();

  IntegerConstant* zero = IntegerConstant::getConstInt(0);

  antlrcpp::Any visitProgram(SysYParserParser::ProgramContext* ctx) override;
  void buildCondition();
  antlrcpp::Any visitInitLVal(SysYParserParser::InitLValContext* ctx) override;
  antlrcpp::Any visitConstDefSingle(
      SysYParserParser::ConstDefSingleContext* ctx) override;
  antlrcpp::Any visitConstDefArray(
      SysYParserParser::ConstDefArrayContext* ctx) override;
  antlrcpp::Any visitInitValArray(
      SysYParserParser::InitValArrayContext* ctx) override;
  antlrcpp::Any visitConstInitValArray(
      SysYParserParser::ConstInitValArrayContext* ctx) override;
  antlrcpp::Any visitVarDefSingle(
      SysYParserParser::VarDefSingleContext* ctx) override;
  antlrcpp::Any visitVarDefArray(
      SysYParserParser::VarDefArrayContext* ctx) override;
  antlrcpp::Any visitVarDefSingleInitVal(
      SysYParserParser::VarDefSingleInitValContext* ctx) override;
  antlrcpp::Any visitVarDefArrayInitVal(
      SysYParserParser::VarDefArrayInitValContext* ctx) override;
  antlrcpp::Any visitConstDecl(
      SysYParserParser::ConstDeclContext* ctx) override;
  antlrcpp::Any visitVarDecl(SysYParserParser::VarDeclContext* ctx) override;
  antlrcpp::Any visitFuncDef(SysYParserParser::FuncDefContext* ctx) override;
  antlrcpp::Any visitBlock(SysYParserParser::BlockContext* ctx) override;
  antlrcpp::Any visitStmtCond(SysYParserParser::StmtCondContext* ctx) override;
  antlrcpp::Any visitStmtWhile(
      SysYParserParser::StmtWhileContext* ctx) override;
  antlrcpp::Any visitStmtBreak(
      SysYParserParser::StmtBreakContext* ctx) override;
  antlrcpp::Any visitStmtContinue(
      SysYParserParser::StmtContinueContext* ctx) override;
  antlrcpp::Any visitStmtReturn(
      SysYParserParser::StmtReturnContext* ctx) override;
  antlrcpp::Any visitLOrExp(SysYParserParser::LOrExpContext* ctx) override;
  antlrcpp::Any visitLAndExp(SysYParserParser::LAndExpContext* ctx) override;
  antlrcpp::Any visitUnaryExpFuncR(
      SysYParserParser::UnaryExpFuncRContext* ctx) override;
  antlrcpp::Any visitAddExp(SysYParserParser::AddExpContext* ctx) override;
  antlrcpp::Any visitMulExp(SysYParserParser::MulExpContext* ctx) override;
  antlrcpp::Any visitRelExp(SysYParserParser::RelExpContext* ctx) override;
  antlrcpp::Any visitEqExp(SysYParserParser::EqExpContext* ctx) override;
  antlrcpp::Any visitLValSingle(
      SysYParserParser::LValSingleContext* ctx) override;
  antlrcpp::Any visitLValArray(
      SysYParserParser::LValArrayContext* ctx) override;
  antlrcpp::Any visitStmtAssign(
      SysYParserParser::StmtAssignContext* ctx) override;
  antlrcpp::Any visitUnaryExpUnary(
      SysYParserParser::UnaryExpUnaryContext* ctx) override;
  antlrcpp::Any visitIntOctConst(
      SysYParserParser::IntOctConstContext* ctx) override;
  antlrcpp::Any visitIntDecConst(
      SysYParserParser::IntDecConstContext* ctx) override;
  antlrcpp::Any visitIntHexConst(
      SysYParserParser::IntHexConstContext* ctx) override;
  antlrcpp::Any visitFloatConst(SysYParserParser::FloatConstContext* ctx) override;

 private:
  void buildVariable(string name, Value* val);
  void buildVariable(string name, Type* type);
  Value* tranformType(Value* src, Type* dest);
  Function* memsetFunc;
};

#endif  // ANTPIE_MYSYSYPARSERVISITOR_H
