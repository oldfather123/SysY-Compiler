//
// Created by 陈硕 on 2023/6/16.
//

#ifndef COMPILER_DEMO_MYVISITOR_H
#define COMPILER_DEMO_MYVISITOR_H

#include "SysYParserBaseVisitor.h"
#include "LLVM.h"
#include "Scope.h"
#include "Builder.h"
#include "Function.h"
#include "GlobalUnit.h"
#include <map>

class MyVisitor : public antlr4::SysYParserBaseVisitor {
public:
    Scope *globalScope;

    Scope *curScope;

    Function* curFunc;

    int scopeCounter;

    ValueRef* zero = new Int_Const(0);

    stack<BasicBlock*> whileCondStk;

    stack<BasicBlock*> exitWhileStk;

    BasicBlock* TBlock;//用于短路时记录要跳转的块

    BasicBlock* FBlock;

    Builder *builder;

    GlobalUnit *globalUnit;

    MyVisitor(){
        globalUnit = new GlobalUnit();
        curFunc = nullptr;
        curScope = nullptr;
        builder = new Builder(globalUnit);
    }

    std::any visitBlock(antlr4::SysYParser::BlockContext *ctx) override;

    std::any visitProgram(antlr4::SysYParser::ProgramContext *ctx) override;

    std::any visitFuncDef(antlr4::SysYParser::FuncDefContext *ctx) override;

    std::any visitVarDecl(antlr4::SysYParser::VarDeclContext *ctx) override;

    std::any visitConstDecl(antlr4::SysYParser::ConstDeclContext *ctx) override;

    std::any visitReturnStmt(antlr4::SysYParser::ReturnStmtContext *ctx) override;

    std::any visitNumberExp(antlr4::SysYParser::NumberExpContext *ctx) override;

    std::any visitExpParenthesis(antlr4::SysYParser::ExpParenthesisContext *ctx) override;

    std::any visitUnaryOpExp(antlr4::SysYParser::UnaryOpExpContext *ctx) override;

    std::any visitMulExp(antlr4::SysYParser::MulExpContext *ctx) override;

    std::any visitPlusExp(antlr4::SysYParser::PlusExpContext *ctx) override;

    std::any visitLVal(antlr4::SysYParser::LValContext *ctx) override;

    std::any visitLValExp(antlr4::SysYParser::LValExpContext *ctx) override;

    any visitAssignStmt(antlr4::SysYParser::AssignStmtContext *ctx) override;

    any visitCallFuncExp(antlr4::SysYParser::CallFuncExpContext *ctx) override;

    any visitIfStmt(antlr4::SysYParser::IfStmtContext *ctx) override;

    any visitWhileStmt(antlr4::SysYParser::WhileStmtContext *ctx) override;

    any visitBreakStmt(antlr4::SysYParser::BreakStmtContext *ctx) override;

    any visitContinueStmt(antlr4::SysYParser::ContinueStmtContext *ctx) override;

    any visitLtCond(antlr4::SysYParser::LtCondContext *ctx) override;

    any visitEqCond(antlr4::SysYParser::EqCondContext *ctx) override;

    any visitExpCond(antlr4::SysYParser::ExpCondContext *ctx) override;

    any visitAndCond(antlr4::SysYParser::AndCondContext *ctx) override;

    any visitOrCond(antlr4::SysYParser::OrCondContext *ctx) override;

    any visitIfElseStmt(antlr4::SysYParser::IfElseStmtContext *ctx) override;

    any visitBlockItem(antlr4::SysYParser::BlockItemContext *ctx) override;
    void add_sylib_func();
    // you should not use these：
    std::any getArrInitVal(Type* type, antlr4::SysYParser::ConstInitValContext* ctx);
    std::any getArrInitVal(Type* type, antlr4::SysYParser::InitValContext* ctx);
    vector<any> GiveMeAnArray(vector<antlr4::SysYParser::ConstInitValContext*>& vals,int& idx,ArrayType* arr);
    vector<any> GiveMeAnArray(vector<antlr4::SysYParser::InitValContext*>& vals,int& idx,ArrayType* arr);


};

struct sylib_func_t {
    std::string _funcName;
    Type_Enum _retType;
    vector<Type *> _paramsTypes;

    sylib_func_t(std::string funcName, Type_Enum retType, std::initializer_list<Type*> paramsTypes)
        : _funcName(std::move(funcName)), _retType(retType){
            for (auto type : paramsTypes) _paramsTypes.push_back(type);
        }
};

#endif //COMPILER_DEMO_MYVISITOR_H
