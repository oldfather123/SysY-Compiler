
// Generated from Lex.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"


class Lex : public antlr4::Lexer {
public:
    enum {
        COMMENT_TOKEN = 1, SPACE = 2, KW_Int = 3, KW_Float = 4, KW_Void = 5,
        KW_Const = 6, KW_Return = 7, KW_If = 8, KW_Else = 9, KW_For = 10, KW_While = 11,
        KW_Do = 12, KW_Break = 13, KW_Continue = 14, Lparen = 15, Rparen = 16,
        Lbarck = 17, Rbarck = 18, Lbrace = 19, Rbrace = 20, Comma = 21, Semi = 22,
        Ques = 23, Colon = 24, Minus = 25, Point = 26, Tilde = 27, Add = 28,
        Mul = 29, Div = 30, Mode = 31, LAND = 32, LOR = 33, EQ = 34, NEQ = 35,
        LT = 36, LE = 37, GT = 38, GE = 39, Int = 40, Ident = 41, STRING = 42,
        Float = 43
    };

    explicit Lex(antlr4::CharStream *input);

    ~Lex() override;


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

