
// Generated from ./src/antlr4/SysY.g4 by ANTLR 4.12.0

#pragma once

#include "antlr4-runtime.h"

class SysYLexer : public antlr4::Lexer {
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

  explicit SysYLexer(antlr4::CharStream* input);

  ~SysYLexer() override;

  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.
};
