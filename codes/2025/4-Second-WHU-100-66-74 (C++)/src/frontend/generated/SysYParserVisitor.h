
// Generated from D:/CompilerRiscv/src/frontend/SysYParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SysYParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysYParser.
 */
class  SysYParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysYParser.
   */
    virtual std::any visitCompUnit(SysYParser::CompUnitContext *context) = 0;

    virtual std::any visitDecl(SysYParser::DeclContext *context) = 0;

    virtual std::any visitFuncName(SysYParser::FuncNameContext *context) = 0;

    virtual std::any visitConstDecl(SysYParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysYParser::BTypeContext *context) = 0;

    virtual std::any visitConstDef(SysYParser::ConstDefContext *context) = 0;

    virtual std::any visitConstInitVal(SysYParser::ConstInitValContext *context) = 0;

    virtual std::any visitVarDecl(SysYParser::VarDeclContext *context) = 0;

    virtual std::any visitVarDef(SysYParser::VarDefContext *context) = 0;

    virtual std::any visitInitVal(SysYParser::InitValContext *context) = 0;

    virtual std::any visitFuncDef(SysYParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysYParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysYParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysYParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlock(SysYParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysYParser::BlockItemContext *context) = 0;

    virtual std::any visitStmt(SysYParser::StmtContext *context) = 0;

    virtual std::any visitIfStruct(SysYParser::IfStructContext *context) = 0;

    virtual std::any visitWhileStruct(SysYParser::WhileStructContext *context) = 0;

    virtual std::any visitExp(SysYParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysYParser::CondContext *context) = 0;

    virtual std::any visitLVal(SysYParser::LValContext *context) = 0;

    virtual std::any visitNumber(SysYParser::NumberContext *context) = 0;

    virtual std::any visitUnaryOp(SysYParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysYParser::FuncRParamsContext *context) = 0;

    virtual std::any visitParam(SysYParser::ParamContext *context) = 0;

    virtual std::any visitConstExp(SysYParser::ConstExpContext *context) = 0;


};

