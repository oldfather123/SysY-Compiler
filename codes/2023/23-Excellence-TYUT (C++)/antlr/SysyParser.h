
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"


class SysyParser : public antlr4::Parser {
public:
    enum {
        T__0 = 1, COMMENT = 2, COMMENT_TOKEN = 3, SPACE = 4, KW_Int = 5, KW_Float = 6,
        KW_Void = 7, KW_Const = 8, KW_Return = 9, KW_If = 10, KW_Else = 11,
        KW_For = 12, KW_While = 13, KW_Do = 14, KW_Break = 15, KW_Continue = 16,
        Lparen = 17, Rparen = 18, Lbarck = 19, Rbarck = 20, Lbrace = 21, Rbrace = 22,
        Comma = 23, Semi = 24, Ques = 25, Colon = 26, Minus = 27, Point = 28,
        Tilde = 29, Add = 30, Mul = 31, Div = 32, Mode = 33, LAND = 34, LOR = 35,
        EQ = 36, NEQ = 37, LT = 38, LE = 39, GT = 40, GE = 41, Int = 42, Ident = 43,
        STRING = 44, Float = 45
    };

    enum {
        RuleCompUnit = 0, RuleDecl = 1, RuleConstDecl = 2, RuleBType = 3, RuleConstDef = 4,
        RuleConstInitVal = 5, RuleVarDecl = 6, RuleVarDef = 7, RuleInitVal = 8,
        RuleFuncDef = 9, RuleFuncType = 10, RuleFuncFParams = 11, RuleFuncFParam = 12,
        RuleBlock = 13, RuleBlockItem = 14, RuleStmt = 15, RuleExp = 16, RuleCond = 17,
        RuleLVal = 18, RulePrimaryExp = 19, RuleNumber = 20, RuleUnaryExp = 21,
        RuleUnaryOp = 22, RuleFuncRParams = 23, RuleFuncRParam = 24, RuleMulExp = 25,
        RuleAddExp = 26, RuleRelExp = 27, RuleEqExp = 28, RuleLAndExp = 29,
        RuleLOrExp = 30, RuleConstExp = 31
    };

    explicit SysyParser(antlr4::TokenStream *input);

    SysyParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

    ~SysyParser() override;

    std::string getGrammarFileName() const override;

    const antlr4::atn::ATN &getATN() const override;

    const std::vector<std::string> &getRuleNames() const override;

    const antlr4::dfa::Vocabulary &getVocabulary() const override;

    antlr4::atn::SerializedATNView getSerializedATN() const override;


    class CompUnitContext;

    class DeclContext;

    class ConstDeclContext;

    class BTypeContext;

    class ConstDefContext;

    class ConstInitValContext;

    class VarDeclContext;

    class VarDefContext;

    class InitValContext;

    class FuncDefContext;

    class FuncTypeContext;

    class FuncFParamsContext;

    class FuncFParamContext;

    class BlockContext;

    class BlockItemContext;

    class StmtContext;

    class ExpContext;

    class CondContext;

    class LValContext;

    class PrimaryExpContext;

    class NumberContext;

    class UnaryExpContext;

    class UnaryOpContext;

    class FuncRParamsContext;

    class FuncRParamContext;

    class MulExpContext;

    class AddExpContext;

    class RelExpContext;

    class EqExpContext;

    class LAndExpContext;

    class LOrExpContext;

    class ConstExpContext;

    class CompUnitContext : public antlr4::ParserRuleContext {
    public:
        CompUnitContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *EOF();

        std::vector<DeclContext *> decl();

        DeclContext *decl(size_t i);

        std::vector<FuncDefContext *> funcDef();

        FuncDefContext *funcDef(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    CompUnitContext *compUnit();

    class DeclContext : public antlr4::ParserRuleContext {
    public:
        DeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        ConstDeclContext *constDecl();

        VarDeclContext *varDecl();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    DeclContext *decl();

    class ConstDeclContext : public antlr4::ParserRuleContext {
    public:
        ConstDeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *KW_Const();

        BTypeContext *bType();

        std::vector<ConstDefContext *> constDef();

        ConstDefContext *constDef(size_t i);

        antlr4::tree::TerminalNode *Semi();

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    ConstDeclContext *constDecl();

    class BTypeContext : public antlr4::ParserRuleContext {
    public:
        BTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *KW_Int();

        antlr4::tree::TerminalNode *KW_Float();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    BTypeContext *bType();

    class ConstDefContext : public antlr4::ParserRuleContext {
    public:
        ConstDefContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *Ident();

        ConstInitValContext *constInitVal();

        std::vector<antlr4::tree::TerminalNode *> Lbarck();

        antlr4::tree::TerminalNode *Lbarck(size_t i);

        std::vector<ConstExpContext *> constExp();

        ConstExpContext *constExp(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Rbarck();

        antlr4::tree::TerminalNode *Rbarck(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    ConstDefContext *constDef();

    class ConstInitValContext : public antlr4::ParserRuleContext {
    public:
        ConstInitValContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        ConstInitValContext() = default;

        void copyFrom(ConstInitValContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class ListConstInitValContext : public ConstInitValContext {
    public:
        ListConstInitValContext(ConstInitValContext *ctx);

        antlr4::tree::TerminalNode *Lbrace();

        antlr4::tree::TerminalNode *Rbrace();

        std::vector<ConstInitValContext *> constInitVal();

        ConstInitValContext *constInitVal(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ScalarConstInitValContext : public ConstInitValContext {
    public:
        ScalarConstInitValContext(ConstInitValContext *ctx);

        ConstExpContext *constExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    ConstInitValContext *constInitVal();

    class VarDeclContext : public antlr4::ParserRuleContext {
    public:
        VarDeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        BTypeContext *bType();

        std::vector<VarDefContext *> varDef();

        VarDefContext *varDef(size_t i);

        antlr4::tree::TerminalNode *Semi();

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    VarDeclContext *varDecl();

    class VarDefContext : public antlr4::ParserRuleContext {
    public:
        VarDefContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        VarDefContext() = default;

        void copyFrom(VarDefContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class UninitVarDefContext : public VarDefContext {
    public:
        UninitVarDefContext(VarDefContext *ctx);

        antlr4::tree::TerminalNode *Ident();

        std::vector<antlr4::tree::TerminalNode *> Lbarck();

        antlr4::tree::TerminalNode *Lbarck(size_t i);

        std::vector<ConstExpContext *> constExp();

        ConstExpContext *constExp(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Rbarck();

        antlr4::tree::TerminalNode *Rbarck(size_t i);

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class InitVarDefContext : public VarDefContext {
    public:
        InitVarDefContext(VarDefContext *ctx);

        antlr4::tree::TerminalNode *Ident();

        InitValContext *initVal();

        std::vector<antlr4::tree::TerminalNode *> Lbarck();

        antlr4::tree::TerminalNode *Lbarck(size_t i);

        std::vector<ConstExpContext *> constExp();

        ConstExpContext *constExp(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Rbarck();

        antlr4::tree::TerminalNode *Rbarck(size_t i);

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    VarDefContext *varDef();

    class InitValContext : public antlr4::ParserRuleContext {
    public:
        InitValContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        InitValContext() = default;

        void copyFrom(InitValContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class ScalarInitValContext : public InitValContext {
    public:
        ScalarInitValContext(InitValContext *ctx);

        ExpContext *exp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ListInitvalContext : public InitValContext {
    public:
        ListInitvalContext(InitValContext *ctx);

        antlr4::tree::TerminalNode *Lbrace();

        antlr4::tree::TerminalNode *Rbrace();

        std::vector<InitValContext *> initVal();

        InitValContext *initVal(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    InitValContext *initVal();

    class FuncDefContext : public antlr4::ParserRuleContext {
    public:
        FuncDefContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        FuncTypeContext *funcType();

        antlr4::tree::TerminalNode *Ident();

        antlr4::tree::TerminalNode *Lparen();

        antlr4::tree::TerminalNode *Rparen();

        BlockContext *block();

        FuncFParamsContext *funcFParams();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    FuncDefContext *funcDef();

    class FuncTypeContext : public antlr4::ParserRuleContext {
    public:
        FuncTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *KW_Void();

        antlr4::tree::TerminalNode *KW_Int();

        antlr4::tree::TerminalNode *KW_Float();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    FuncTypeContext *funcType();

    class FuncFParamsContext : public antlr4::ParserRuleContext {
    public:
        FuncFParamsContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        std::vector<FuncFParamContext *> funcFParam();

        FuncFParamContext *funcFParam(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    FuncFParamsContext *funcFParams();

    class FuncFParamContext : public antlr4::ParserRuleContext {
    public:
        FuncFParamContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        BTypeContext *bType();

        antlr4::tree::TerminalNode *Ident();

        std::vector<antlr4::tree::TerminalNode *> Lbarck();

        antlr4::tree::TerminalNode *Lbarck(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Rbarck();

        antlr4::tree::TerminalNode *Rbarck(size_t i);

        std::vector<ConstExpContext *> constExp();

        ConstExpContext *constExp(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    FuncFParamContext *funcFParam();

    class BlockContext : public antlr4::ParserRuleContext {
    public:
        BlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *Lbrace();

        antlr4::tree::TerminalNode *Rbrace();

        std::vector<BlockItemContext *> blockItem();

        BlockItemContext *blockItem(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    BlockContext *block();

    class BlockItemContext : public antlr4::ParserRuleContext {
    public:
        BlockItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        DeclContext *decl();

        StmtContext *stmt();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    BlockItemContext *blockItem();

    class StmtContext : public antlr4::ParserRuleContext {
    public:
        StmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        StmtContext() = default;

        void copyFrom(StmtContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class WhileStmtContext : public StmtContext {
    public:
        WhileStmtContext(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_While();

        antlr4::tree::TerminalNode *Lparen();

        CondContext *cond();

        antlr4::tree::TerminalNode *Rparen();

        StmtContext *stmt();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class BlockStmtContext : public StmtContext {
    public:
        BlockStmtContext(StmtContext *ctx);

        BlockContext *block();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class AssignmentContext : public StmtContext {
    public:
        AssignmentContext(StmtContext *ctx);

        LValContext *lVal();

        ExpContext *exp();

        antlr4::tree::TerminalNode *Semi();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class IfStmt1Context : public StmtContext {
    public:
        IfStmt1Context(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_If();

        antlr4::tree::TerminalNode *Lparen();

        CondContext *cond();

        antlr4::tree::TerminalNode *Rparen();

        StmtContext *stmt();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class BreakStmtContext : public StmtContext {
    public:
        BreakStmtContext(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_Break();

        antlr4::tree::TerminalNode *Semi();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ExpStmtContext : public StmtContext {
    public:
        ExpStmtContext(StmtContext *ctx);

        antlr4::tree::TerminalNode *Semi();

        ExpContext *exp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class IfStmt2Context : public StmtContext {
    public:
        IfStmt2Context(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_If();

        antlr4::tree::TerminalNode *Lparen();

        CondContext *cond();

        antlr4::tree::TerminalNode *Rparen();

        std::vector<StmtContext *> stmt();

        StmtContext *stmt(size_t i);

        antlr4::tree::TerminalNode *KW_Else();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ReturnStmtContext : public StmtContext {
    public:
        ReturnStmtContext(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_Return();

        antlr4::tree::TerminalNode *Semi();

        ExpContext *exp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ContinueStmtContext : public StmtContext {
    public:
        ContinueStmtContext(StmtContext *ctx);

        antlr4::tree::TerminalNode *KW_Continue();

        antlr4::tree::TerminalNode *Semi();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    StmtContext *stmt();

    class ExpContext : public antlr4::ParserRuleContext {
    public:
        ExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        AddExpContext *addExp();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    ExpContext *exp();

    class CondContext : public antlr4::ParserRuleContext {
    public:
        CondContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        LOrExpContext *lOrExp();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    CondContext *cond();

    class LValContext : public antlr4::ParserRuleContext {
    public:
        LValContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *Ident();

        std::vector<antlr4::tree::TerminalNode *> Lbarck();

        antlr4::tree::TerminalNode *Lbarck(size_t i);

        std::vector<ExpContext *> exp();

        ExpContext *exp(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Rbarck();

        antlr4::tree::TerminalNode *Rbarck(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    LValContext *lVal();

    class PrimaryExpContext : public antlr4::ParserRuleContext {
    public:
        PrimaryExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        PrimaryExpContext() = default;

        void copyFrom(PrimaryExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class PrimaryExp2Context : public PrimaryExpContext {
    public:
        PrimaryExp2Context(PrimaryExpContext *ctx);

        LValContext *lVal();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class PrimaryExp1Context : public PrimaryExpContext {
    public:
        PrimaryExp1Context(PrimaryExpContext *ctx);

        antlr4::tree::TerminalNode *Lparen();

        ExpContext *exp();

        antlr4::tree::TerminalNode *Rparen();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class PrimaryExp3Context : public PrimaryExpContext {
    public:
        PrimaryExp3Context(PrimaryExpContext *ctx);

        NumberContext *number();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    PrimaryExpContext *primaryExp();

    class NumberContext : public antlr4::ParserRuleContext {
    public:
        NumberContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        NumberContext() = default;

        void copyFrom(NumberContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class NumberIntContext : public NumberContext {
    public:
        NumberIntContext(NumberContext *ctx);

        antlr4::tree::TerminalNode *Int();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class NunmberFloatContext : public NumberContext {
    public:
        NunmberFloatContext(NumberContext *ctx);

        antlr4::tree::TerminalNode *Float();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    NumberContext *number();

    class UnaryExpContext : public antlr4::ParserRuleContext {
    public:
        UnaryExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        UnaryExpContext() = default;

        void copyFrom(UnaryExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class UnaryExp3Context : public UnaryExpContext {
    public:
        UnaryExp3Context(UnaryExpContext *ctx);

        UnaryOpContext *unaryOp();

        UnaryExpContext *unaryExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class UnaryExp2Context : public UnaryExpContext {
    public:
        UnaryExp2Context(UnaryExpContext *ctx);

        antlr4::tree::TerminalNode *Ident();

        antlr4::tree::TerminalNode *Lparen();

        antlr4::tree::TerminalNode *Rparen();

        FuncRParamsContext *funcRParams();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class UnaryExp1Context : public UnaryExpContext {
    public:
        UnaryExp1Context(UnaryExpContext *ctx);

        PrimaryExpContext *primaryExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    UnaryExpContext *unaryExp();

    class UnaryOpContext : public antlr4::ParserRuleContext {
    public:
        UnaryOpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        antlr4::tree::TerminalNode *Add();

        antlr4::tree::TerminalNode *Minus();

        antlr4::tree::TerminalNode *Point();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    UnaryOpContext *unaryOp();

    class FuncRParamsContext : public antlr4::ParserRuleContext {
    public:
        FuncRParamsContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        std::vector<FuncRParamContext *> funcRParam();

        FuncRParamContext *funcRParam(size_t i);

        std::vector<antlr4::tree::TerminalNode *> Comma();

        antlr4::tree::TerminalNode *Comma(size_t i);


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    FuncRParamsContext *funcRParams();

    class FuncRParamContext : public antlr4::ParserRuleContext {
    public:
        FuncRParamContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        FuncRParamContext() = default;

        void copyFrom(FuncRParamContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class StringParamContext : public FuncRParamContext {
    public:
        StringParamContext(FuncRParamContext *ctx);

        antlr4::tree::TerminalNode *STRING();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class ExpParamContext : public FuncRParamContext {
    public:
        ExpParamContext(FuncRParamContext *ctx);

        ExpContext *exp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    FuncRParamContext *funcRParam();

    class MulExpContext : public antlr4::ParserRuleContext {
    public:
        MulExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        MulExpContext() = default;

        void copyFrom(MulExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class MulExp2Context : public MulExpContext {
    public:
        MulExp2Context(MulExpContext *ctx);

        MulExpContext *mulExp();

        UnaryExpContext *unaryExp();

        antlr4::tree::TerminalNode *Mul();

        antlr4::tree::TerminalNode *Div();

        antlr4::tree::TerminalNode *Mode();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class MulExp1Context : public MulExpContext {
    public:
        MulExp1Context(MulExpContext *ctx);

        UnaryExpContext *unaryExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    MulExpContext *mulExp();

    MulExpContext *mulExp(int precedence);

    class AddExpContext : public antlr4::ParserRuleContext {
    public:
        AddExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        AddExpContext() = default;

        void copyFrom(AddExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class AddExp2Context : public AddExpContext {
    public:
        AddExp2Context(AddExpContext *ctx);

        AddExpContext *addExp();

        MulExpContext *mulExp();

        antlr4::tree::TerminalNode *Add();

        antlr4::tree::TerminalNode *Minus();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class AddExp1Context : public AddExpContext {
    public:
        AddExp1Context(AddExpContext *ctx);

        MulExpContext *mulExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    AddExpContext *addExp();

    AddExpContext *addExp(int precedence);

    class RelExpContext : public antlr4::ParserRuleContext {
    public:
        RelExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        RelExpContext() = default;

        void copyFrom(RelExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class RelExp2Context : public RelExpContext {
    public:
        RelExp2Context(RelExpContext *ctx);

        RelExpContext *relExp();

        AddExpContext *addExp();

        antlr4::tree::TerminalNode *LT();

        antlr4::tree::TerminalNode *GT();

        antlr4::tree::TerminalNode *LE();

        antlr4::tree::TerminalNode *GE();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class RelExp1Context : public RelExpContext {
    public:
        RelExp1Context(RelExpContext *ctx);

        AddExpContext *addExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    RelExpContext *relExp();

    RelExpContext *relExp(int precedence);

    class EqExpContext : public antlr4::ParserRuleContext {
    public:
        EqExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        EqExpContext() = default;

        void copyFrom(EqExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class EqExp2Context : public EqExpContext {
    public:
        EqExp2Context(EqExpContext *ctx);

        EqExpContext *eqExp();

        RelExpContext *relExp();

        antlr4::tree::TerminalNode *EQ();

        antlr4::tree::TerminalNode *NEQ();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class EqExp1Context : public EqExpContext {
    public:
        EqExp1Context(EqExpContext *ctx);

        RelExpContext *relExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    EqExpContext *eqExp();

    EqExpContext *eqExp(int precedence);

    class LAndExpContext : public antlr4::ParserRuleContext {
    public:
        LAndExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        LAndExpContext() = default;

        void copyFrom(LAndExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class LAndExp1Context : public LAndExpContext {
    public:
        LAndExp1Context(LAndExpContext *ctx);

        EqExpContext *eqExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class LAndExp2Context : public LAndExpContext {
    public:
        LAndExp2Context(LAndExpContext *ctx);

        LAndExpContext *lAndExp();

        antlr4::tree::TerminalNode *LAND();

        EqExpContext *eqExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    LAndExpContext *lAndExp();

    LAndExpContext *lAndExp(int precedence);

    class LOrExpContext : public antlr4::ParserRuleContext {
    public:
        LOrExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        LOrExpContext() = default;

        void copyFrom(LOrExpContext *context);

        using antlr4::ParserRuleContext::copyFrom;

        virtual size_t getRuleIndex() const override;


    };

    class LOrExp2Context : public LOrExpContext {
    public:
        LOrExp2Context(LOrExpContext *ctx);

        LOrExpContext *lOrExp();

        antlr4::tree::TerminalNode *LOR();

        LAndExpContext *lAndExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    class LOrExp1Context : public LOrExpContext {
    public:
        LOrExp1Context(LOrExpContext *ctx);

        LAndExpContext *lAndExp();

        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
    };

    LOrExpContext *lOrExp();

    LOrExpContext *lOrExp(int precedence);

    class ConstExpContext : public antlr4::ParserRuleContext {
    public:
        ConstExpContext(antlr4::ParserRuleContext *parent, size_t invokingState);

        virtual size_t getRuleIndex() const override;

        AddExpContext *addExp();


        virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;

    };

    ConstExpContext *constExp();


    bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

    bool mulExpSempred(MulExpContext *_localctx, size_t predicateIndex);

    bool addExpSempred(AddExpContext *_localctx, size_t predicateIndex);

    bool relExpSempred(RelExpContext *_localctx, size_t predicateIndex);

    bool eqExpSempred(EqExpContext *_localctx, size_t predicateIndex);

    bool lAndExpSempred(LAndExpContext *_localctx, size_t predicateIndex);

    bool lOrExpSempred(LOrExpContext *_localctx, size_t predicateIndex);

    // By default the static state used to implement the parser is lazily initialized during the first
    // call to the constructor. You can call this function if you wish to initialize the static state
    // ahead of time.
    static void initialize();

private:
};

