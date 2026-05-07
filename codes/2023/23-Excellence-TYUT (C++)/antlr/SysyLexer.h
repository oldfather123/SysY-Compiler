
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"


class SysyLexer : public antlr4::Lexer {
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

    explicit SysyLexer(antlr4::CharStream *input);

    ~SysyLexer() override;


    std::string getGrammarFileName() const override;

    const std::vector<std::string> &getRuleNames() const override;

    const std::vector<std::string> &getChannelNames() const override;

    const std::vector<std::string> &getModeNames() const override;

    const antlr4::dfa::Vocabulary &getVocabulary() const override;

    antlr4::atn::SerializedATNView getSerializedATN() const override;

    const antlr4::atn::ATN &getATN() const override;

    // By default the static state used to implement the lexer is lazily initialized during the first
    // call to the constructor. You can call this function if you wish to initialize the static state
    // ahead of time.
    static void initialize();

private:

    // Individual action functions triggered by action() above.

    // Individual semantic predicate functions triggered by sempred() above.

};

