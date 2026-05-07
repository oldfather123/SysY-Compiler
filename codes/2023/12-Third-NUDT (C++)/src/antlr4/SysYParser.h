
// Generated from ./src/antlr4/SysY.g4 by ANTLR 4.12.0

#pragma once

#include "antlr4-runtime.h"

class SysYParser : public antlr4::Parser {
public:
  enum {
    INT = 1,
    FLOAT = 2,
    VOID = 3,
    CONST = 4,
    FOR = 5,
    DO = 6,
    WHILE = 7,
    IF = 8,
    ELSE = 9,
    CONTINUE = 10,
    BREAK = 11,
    RETURN = 12,
    LPAREN = 13,
    RPAREN = 14,
    LBRACKET = 15,
    RBRACKET = 16,
    LBRACES = 17,
    RBRACES = 18,
    COMMA = 19,
    SEMICOLON = 20,
    ASSIGN = 21,
    ADD = 22,
    SUB = 23,
    MUL = 24,
    DIV = 25,
    MOD = 26,
    LT = 27,
    GT = 28,
    LE = 29,
    GE = 30,
    EQ = 31,
    NE = 32,
    AND = 33,
    OR = 34,
    NOT = 35,
    INTLITERAL = 36,
    FLOATLITERAL = 37,
    IDENTIFIER = 38,
    STRING = 39,
    SPACE = 40,
    LINECOMMENT = 41,
    BLOCKCOMMENT = 42
  };

  enum {
    RuleComp = 0,
    RuleDecl = 1,
    RuleVarDef = 2,
    RuleInit = 3,
    RuleFuncDef = 4,
    RuleFuncFParams = 5,
    RuleFuncFParam = 6,
    RuleStmt = 7,
    RuleAssignStmt = 8,
    RuleExpStmt = 9,
    RuleIfStmt = 10,
    RuleWhileStmt = 11,
    RuleBreakStmt = 12,
    RuleContinueStmt = 13,
    RuleReturnStmt = 14,
    RuleBlockStmt = 15,
    RuleBlockItem = 16,
    RuleEmptyStmt = 17,
    RuleExp = 18,
    RuleCall = 19,
    RuleFuncRParams = 20,
    RuleLValue = 21,
    RuleNumber = 22,
    RuleString = 23,
    RuleReturnType = 24,
    RuleBType = 25
  };

  explicit SysYParser(antlr4::TokenStream *input);

  SysYParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~SysYParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN &getATN() const override;

  const std::vector<std::string> &getRuleNames() const override;

  const antlr4::dfa::Vocabulary &getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  class CompContext;
  class DeclContext;
  class VarDefContext;
  class InitContext;
  class FuncDefContext;
  class FuncFParamsContext;
  class FuncFParamContext;
  class StmtContext;
  class AssignStmtContext;
  class ExpStmtContext;
  class IfStmtContext;
  class WhileStmtContext;
  class BreakStmtContext;
  class ContinueStmtContext;
  class ReturnStmtContext;
  class BlockStmtContext;
  class BlockItemContext;
  class EmptyStmtContext;
  class ExpContext;
  class CallContext;
  class FuncRParamsContext;
  class LValueContext;
  class NumberContext;
  class StringContext;
  class ReturnTypeContext;
  class BTypeContext;

  class CompContext : public antlr4::ParserRuleContext {
  public:
    CompContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<DeclContext *> decl();
    DeclContext *decl(size_t i);
    std::vector<FuncDefContext *> funcDef();
    FuncDefContext *funcDef(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  CompContext *comp();

  class DeclContext : public antlr4::ParserRuleContext {
  public:
    DeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BTypeContext *bType();
    std::vector<VarDefContext *> varDef();
    VarDefContext *varDef(size_t i);
    antlr4::tree::TerminalNode *SEMICOLON();
    antlr4::tree::TerminalNode *CONST();
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode *COMMA(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  DeclContext *decl();

  class VarDefContext : public antlr4::ParserRuleContext {
  public:
    VarDefContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LValueContext *lValue();
    antlr4::tree::TerminalNode *ASSIGN();
    InitContext *init();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  VarDefContext *varDef();

  class InitContext : public antlr4::ParserRuleContext {
  public:
    InitContext(antlr4::ParserRuleContext *parent, size_t invokingState);

    InitContext() = default;
    void copyFrom(InitContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;
  };

  class ArrayInitContext : public InitContext {
  public:
    ArrayInitContext(InitContext *ctx);

    antlr4::tree::TerminalNode *LBRACES();
    antlr4::tree::TerminalNode *RBRACES();
    std::vector<InitContext *> init();
    InitContext *init(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode *COMMA(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class ScalarInitContext : public InitContext {
  public:
    ScalarInitContext(InitContext *ctx);

    ExpContext *exp();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  InitContext *init();

  class FuncDefContext : public antlr4::ParserRuleContext {
  public:
    FuncDefContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ReturnTypeContext *returnType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *LPAREN();
    antlr4::tree::TerminalNode *RPAREN();
    BlockStmtContext *blockStmt();
    FuncFParamsContext *funcFParams();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  FuncDefContext *funcDef();

  class FuncFParamsContext : public antlr4::ParserRuleContext {
  public:
    FuncFParamsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<FuncFParamContext *> funcFParam();
    FuncFParamContext *funcFParam(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode *COMMA(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  FuncFParamsContext *funcFParams();

  class FuncFParamContext : public antlr4::ParserRuleContext {
  public:
    FuncFParamContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BTypeContext *bType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    std::vector<antlr4::tree::TerminalNode *> LBRACKET();
    antlr4::tree::TerminalNode *LBRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RBRACKET();
    antlr4::tree::TerminalNode *RBRACKET(size_t i);
    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  FuncFParamContext *funcFParam();

  class StmtContext : public antlr4::ParserRuleContext {
  public:
    StmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AssignStmtContext *assignStmt();
    ExpStmtContext *expStmt();
    IfStmtContext *ifStmt();
    WhileStmtContext *whileStmt();
    BreakStmtContext *breakStmt();
    ContinueStmtContext *continueStmt();
    ReturnStmtContext *returnStmt();
    BlockStmtContext *blockStmt();
    EmptyStmtContext *emptyStmt();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  StmtContext *stmt();

  class AssignStmtContext : public antlr4::ParserRuleContext {
  public:
    AssignStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LValueContext *lValue();
    antlr4::tree::TerminalNode *ASSIGN();
    ExpContext *exp();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  AssignStmtContext *assignStmt();

  class ExpStmtContext : public antlr4::ParserRuleContext {
  public:
    ExpStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpContext *exp();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExpStmtContext *expStmt();

  class IfStmtContext : public antlr4::ParserRuleContext {
  public:
    IfStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IF();
    antlr4::tree::TerminalNode *LPAREN();
    ExpContext *exp();
    antlr4::tree::TerminalNode *RPAREN();
    std::vector<StmtContext *> stmt();
    StmtContext *stmt(size_t i);
    antlr4::tree::TerminalNode *ELSE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  IfStmtContext *ifStmt();

  class WhileStmtContext : public antlr4::ParserRuleContext {
  public:
    WhileStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *WHILE();
    antlr4::tree::TerminalNode *LPAREN();
    ExpContext *exp();
    antlr4::tree::TerminalNode *RPAREN();
    StmtContext *stmt();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  WhileStmtContext *whileStmt();

  class BreakStmtContext : public antlr4::ParserRuleContext {
  public:
    BreakStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BREAK();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  BreakStmtContext *breakStmt();

  class ContinueStmtContext : public antlr4::ParserRuleContext {
  public:
    ContinueStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CONTINUE();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ContinueStmtContext *continueStmt();

  class ReturnStmtContext : public antlr4::ParserRuleContext {
  public:
    ReturnStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *RETURN();
    antlr4::tree::TerminalNode *SEMICOLON();
    ExpContext *exp();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ReturnStmtContext *returnStmt();

  class BlockStmtContext : public antlr4::ParserRuleContext {
  public:
    BlockStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LBRACES();
    antlr4::tree::TerminalNode *RBRACES();
    std::vector<BlockItemContext *> blockItem();
    BlockItemContext *blockItem(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  BlockStmtContext *blockStmt();

  class BlockItemContext : public antlr4::ParserRuleContext {
  public:
    BlockItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DeclContext *decl();
    StmtContext *stmt();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  BlockItemContext *blockItem();

  class EmptyStmtContext : public antlr4::ParserRuleContext {
  public:
    EmptyStmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  EmptyStmtContext *emptyStmt();

  class ExpContext : public antlr4::ParserRuleContext {
  public:
    ExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

    ExpContext() = default;
    void copyFrom(ExpContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;
  };

  class EqExpContext : public ExpContext {
  public:
    EqExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *EQ();
    antlr4::tree::TerminalNode *NE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class LValueExpContext : public ExpContext {
  public:
    LValueExpContext(ExpContext *ctx);

    LValueContext *lValue();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class NumberExpContext : public ExpContext {
  public:
    NumberExpContext(ExpContext *ctx);

    NumberContext *number();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class AndExpContext : public ExpContext {
  public:
    AndExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *AND();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class UnaryExpContext : public ExpContext {
  public:
    UnaryExpContext(ExpContext *ctx);

    ExpContext *exp();
    antlr4::tree::TerminalNode *ADD();
    antlr4::tree::TerminalNode *SUB();
    antlr4::tree::TerminalNode *NOT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class ParenExpContext : public ExpContext {
  public:
    ParenExpContext(ExpContext *ctx);

    antlr4::tree::TerminalNode *LPAREN();
    ExpContext *exp();
    antlr4::tree::TerminalNode *RPAREN();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class AddExpContext : public ExpContext {
  public:
    AddExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *ADD();
    antlr4::tree::TerminalNode *SUB();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class MulExpContext : public ExpContext {
  public:
    MulExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *MUL();
    antlr4::tree::TerminalNode *DIV();
    antlr4::tree::TerminalNode *MOD();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class StringExpContext : public ExpContext {
  public:
    StringExpContext(ExpContext *ctx);

    StringContext *string();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class OrExpContext : public ExpContext {
  public:
    OrExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *OR();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class CallExpContext : public ExpContext {
  public:
    CallExpContext(ExpContext *ctx);

    CallContext *call();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class RelExpContext : public ExpContext {
  public:
    RelExpContext(ExpContext *ctx);

    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    antlr4::tree::TerminalNode *LT();
    antlr4::tree::TerminalNode *GT();
    antlr4::tree::TerminalNode *LE();
    antlr4::tree::TerminalNode *GE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExpContext *exp();
  ExpContext *exp(int precedence);
  class CallContext : public antlr4::ParserRuleContext {
  public:
    CallContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *LPAREN();
    antlr4::tree::TerminalNode *RPAREN();
    FuncRParamsContext *funcRParams();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  CallContext *call();

  class FuncRParamsContext : public antlr4::ParserRuleContext {
  public:
    FuncRParamsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode *COMMA(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  FuncRParamsContext *funcRParams();

  class LValueContext : public antlr4::ParserRuleContext {
  public:
    LValueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();
    std::vector<antlr4::tree::TerminalNode *> LBRACKET();
    antlr4::tree::TerminalNode *LBRACKET(size_t i);
    std::vector<ExpContext *> exp();
    ExpContext *exp(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RBRACKET();
    antlr4::tree::TerminalNode *RBRACKET(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  LValueContext *lValue();

  class NumberContext : public antlr4::ParserRuleContext {
  public:
    NumberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INTLITERAL();
    antlr4::tree::TerminalNode *FLOATLITERAL();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  NumberContext *number();

  class StringContext : public antlr4::ParserRuleContext {
  public:
    StringContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *STRING();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  StringContext *string();

  class ReturnTypeContext : public antlr4::ParserRuleContext {
  public:
    ReturnTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *VOID();
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *FLOAT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ReturnTypeContext *returnType();

  class BTypeContext : public antlr4::ParserRuleContext {
  public:
    BTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *FLOAT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  BTypeContext *bType();

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool expSempred(ExpContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};
