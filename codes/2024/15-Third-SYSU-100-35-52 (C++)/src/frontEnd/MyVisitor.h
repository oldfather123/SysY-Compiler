#pragma once
#include "IRbuilder.h"
#include "Module.h"
#include "SysY2022BaseVisitor.h"
#include "SysY2022Lexer.h"
#include "antlr4-runtime.h"
#include "domTreeAnalysis.h"
#include "simplifyCFG.h"
#include "constantCombine.h"
#include "constStayRight.h"
#include "loopAnalysis.h"
#include "mem2reg.h"
#include "loopCanonicalize.h"
#include "loopEliminate.h"
#include "loopRotate.h"
#include "loopUnroll.h"
#include "LICM.h"
#include "Loop.h"
#include "sub2add.h"
#include "DCE.h"
#include "cse.h"
#include "callGraphAnalysis.h"
#include "inline.h"
#include "global2Local.h"
#include "peepHole.h"
#include "instCombine.h"
#include "DAE.h"
#include "splitGEP.h"
#include "constantArrPropagation.h"
#include "simplifyInstruction.h"
#include "GVN.h"
#include "stateLessCache.h"
#include "unifyReturn.h"
#include "funcAttrAnalysis.h"
#include "loopGepCombine.h"
#include "phiEliminate.h"
#include <iostream>
#include <memory>
#include <stack>

class MyVisitor : public SysY2022BaseVisitor {
public:
    Module irModule;
    IRbuilder myBuilder;
    void print(std::ostream& out = std::cout);
    void opt();

    antlrcpp::Any visitCompUnit(SysY2022Parser::CompUnitContext* ctx) override;

    antlrcpp::Any visitDecl(SysY2022Parser::DeclContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitConstDecl(SysY2022Parser::ConstDeclContext* ctx) override;

    antlrcpp::Any visitBType(SysY2022Parser::BTypeContext* ctx) override;

    antlrcpp::Any visitConstDef(SysY2022Parser::ConstDefContext* ctx) override;

    antlrcpp::Any visitItemConstInitVal(SysY2022Parser::ItemConstInitValContext* ctx) override;

    // const数组会单独处理
    // virtual antlrcpp::Any visitListConstInitVal(SysY2022Parser::ListConstInitValContext *ctx) override {
    //   return visitChildren(ctx);
    // }

    antlrcpp::Any visitVarDecl(SysY2022Parser::VarDeclContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext* ctx) override;

    int constArr = 0;
    VariablePtr arrInitItem(SysY2022Parser::ItemConstInitValContext* ctx, TypePtr type);
    Arr* arrInitList(SysY2022Parser::ListConstInitValContext* ctx, TypePtr type);
    VariablePtr arrInitItem(SysY2022Parser::ItemInitValContext* ctx, TypePtr type);
    Arr* arrInitList(SysY2022Parser::ListInitValContext* ctx, TypePtr type);
    void arrSetList(SysY2022Parser::ListInitValContext* ctx, ValuePtr value);

    antlrcpp::Any visitVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext* ctx) override;

    bool dfsInitVal(SysY2022Parser::InitValContext* ctx);

    antlrcpp::Any visitItemInitVal(SysY2022Parser::ItemInitValContext* ctx) override;

    antlrcpp::Any visitListInitVal(SysY2022Parser::ListInitValContext* ctx) override;

    antlrcpp::Any visitFuncDef(SysY2022Parser::FuncDefContext* ctx) override;

    antlrcpp::Any visitFuncType(SysY2022Parser::FuncTypeContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitFuncFParams(SysY2022Parser::FuncFParamsContext* ctx) override;

    antlrcpp::Any visitFuncFParam(SysY2022Parser::FuncFParamContext* ctx) override;

    antlrcpp::Any visitBlock(SysY2022Parser::BlockContext* ctx) override;

    antlrcpp::Any visitBlockItem(SysY2022Parser::BlockItemContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitAssignStmt(SysY2022Parser::AssignStmtContext* ctx) override;

    antlrcpp::Any visitExpStmt(SysY2022Parser::ExpStmtContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitBlockStmt(SysY2022Parser::BlockStmtContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitIfStmt(SysY2022Parser::IfStmtContext* ctx) override;

    antlrcpp::Any visitIfElseStmt(SysY2022Parser::IfElseStmtContext* ctx) override;

    antlrcpp::Any visitWhileStmt(SysY2022Parser::WhileStmtContext* ctx) override;

    antlrcpp::Any visitBreakStmt(SysY2022Parser::BreakStmtContext* ctx) override;

    antlrcpp::Any visitContinueStmt(SysY2022Parser::ContinueStmtContext* ctx) override;

    antlrcpp::Any visitReturnStmt(SysY2022Parser::ReturnStmtContext* ctx) override;

    antlrcpp::Any visitExp(SysY2022Parser::ExpContext* ctx) override;

    antlrcpp::Any visitCond(SysY2022Parser::CondContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitLVal(SysY2022Parser::LValContext* ctx) override;

    antlrcpp::Any visitExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext* ctx) override;

    antlrcpp::Any visitLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext* ctx) override;

    antlrcpp::Any visitNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext* ctx) override;

    antlrcpp::Any visitNumber(SysY2022Parser::NumberContext* ctx) override;

    antlrcpp::Any visitPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext* ctx) override;

    antlrcpp::Any visitFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext* ctx) override;

    antlrcpp::Any visitUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext* ctx) override;

    antlrcpp::Any visitUnaryOp(SysY2022Parser::UnaryOpContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitFuncRParams(SysY2022Parser::FuncRParamsContext* ctx) override;

    antlrcpp::Any visitFuncRParam(SysY2022Parser::FuncRParamContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitMulMulExp(SysY2022Parser::MulMulExpContext* ctx) override;

    antlrcpp::Any visitUnaryMulExp(SysY2022Parser::UnaryMulExpContext* ctx) override;

    antlrcpp::Any visitAddAddExp(SysY2022Parser::AddAddExpContext* ctx) override;

    antlrcpp::Any visitMulAddExp(SysY2022Parser::MulAddExpContext* ctx) override;

    antlrcpp::Any visitRelRelExp(SysY2022Parser::RelRelExpContext* ctx) override;

    antlrcpp::Any visitAddRelExp(SysY2022Parser::AddRelExpContext* ctx) override;

    antlrcpp::Any visitEqEqExp(SysY2022Parser::EqEqExpContext* ctx) override;

    antlrcpp::Any visitRelEqExp(SysY2022Parser::RelEqExpContext* ctx) override;

    antlrcpp::Any visitEqLAndExp(SysY2022Parser::EqLAndExpContext* ctx) override;

    antlrcpp::Any visitLAndLAndExp(SysY2022Parser::LAndLAndExpContext* ctx) override;

    antlrcpp::Any visitLAndLOrExp(SysY2022Parser::LAndLOrExpContext* ctx) override;

    antlrcpp::Any visitLOrLOrExp(SysY2022Parser::LOrLOrExpContext* ctx) override;

    antlrcpp::Any visitConstExp(SysY2022Parser::ConstExpContext* ctx) override;
};
