
// Generated from SysYLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  SysYLexer : public antlr4::Lexer {
public:
  enum {
    CONST = 1, INT = 2, FLOAT = 3, VOID = 4, IF = 5, ELSE = 6, WHILE = 7, 
    BREAK = 8, CONTINUE = 9, RETURN = 10, PLUS = 11, MINUS = 12, MUL = 13, 
    DIV = 14, MOD = 15, ASSIGN = 16, EQ = 17, NEQ = 18, LT = 19, GT = 20, 
    LE = 21, GE = 22, NOT = 23, AND = 24, OR = 25, LPAREN = 26, RPAREN = 27, 
    LBRACE = 28, RBRACE = 29, LBRACK = 30, RBRACK = 31, COMMA = 32, SEMICOLON = 33, 
    FLOAT_CONST = 34, INT_CONST = 35, STRING_CONST = 36, IDENT = 37, LINE_COMMENT = 38, 
    BLOCK_COMMENT = 39, WS = 40
  };

  explicit SysYLexer(antlr4::CharStream *input);

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

