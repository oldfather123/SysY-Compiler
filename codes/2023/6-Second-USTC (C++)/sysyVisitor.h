
// Generated from /home/jpy794/actions-runner/_work/sysy-compiler/sysy-compiler/src/ast/grammer/sysy.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"
#include "sysyParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by sysyParser.
 */
class  sysyVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by sysyParser.
   */
    virtual std::any visitCompUnit(sysyParser::CompUnitContext *context) = 0;

    virtual std::any visitGlobalDef(sysyParser::GlobalDefContext *context) = 0;

    virtual std::any visitVardecl(sysyParser::VardeclContext *context) = 0;

    virtual std::any visitVardef(sysyParser::VardefContext *context) = 0;

    virtual std::any visitVarInit(sysyParser::VarInitContext *context) = 0;

    virtual std::any visitFuncdef(sysyParser::FuncdefContext *context) = 0;

    virtual std::any visitFuncparam(sysyParser::FuncparamContext *context) = 0;

    virtual std::any visitBlock(sysyParser::BlockContext *context) = 0;

    virtual std::any visitExpStmt(sysyParser::ExpStmtContext *context) = 0;

    virtual std::any visitStmt(sysyParser::StmtContext *context) = 0;

    virtual std::any visitLval(sysyParser::LvalContext *context) = 0;

    virtual std::any visitFuncCall(sysyParser::FuncCallContext *context) = 0;

    virtual std::any visitParenExp(sysyParser::ParenExpContext *context) = 0;

    virtual std::any visitExp(sysyParser::ExpContext *context) = 0;

    virtual std::any visitNumber(sysyParser::NumberContext *context) = 0;


};

