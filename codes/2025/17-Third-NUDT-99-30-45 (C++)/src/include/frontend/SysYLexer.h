
// Generated from SysY.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  SysYLexer : public antlr4::Lexer {
public:
  enum {
    CONST = 1, INT = 2, FLOAT = 3, VOID = 4, IF = 5, ELSE = 6, WHILE = 7, 
    BREAK = 8, CONTINUE = 9, RETURN = 10, ADD = 11, SUB = 12, MUL = 13, 
    DIV = 14, MOD = 15, EQ = 16, NE = 17, LT = 18, LE = 19, GT = 20, GE = 21, 
    AND = 22, OR = 23, NOT = 24, ASSIGN = 25, COMMA = 26, SEMICOLON = 27, 
    LPAREN = 28, RPAREN = 29, LBRACE = 30, RBRACE = 31, LBRACK = 32, RBRACK = 33, 
    Ident = 34, ILITERAL = 35, FLITERAL = 36, STRING = 37, WS = 38, LINECOMMENT = 39, 
    BLOCKCOMMENT = 40
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

