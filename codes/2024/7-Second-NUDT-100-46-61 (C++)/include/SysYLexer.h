
// Generated from SysY.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"




class  SysYLexer : public antlr4::Lexer {
public:
  enum {
    CONST = 1, INT = 2, FLOAT = 3, VOID = 4, IF = 5, ELSE = 6, WHILE = 7, 
    BREAK = 8, CONTINUE = 9, RETURN = 10, ASSIGN = 11, ADD = 12, SUB = 13, 
    MUL = 14, DIV = 15, MODULO = 16, LT = 17, GT = 18, LE = 19, GE = 20, 
    EQ = 21, NE = 22, AND = 23, OR = 24, NOT = 25, LPAREN = 26, RPAREN = 27, 
    LBRACKET = 28, RBRACKET = 29, LBRACE = 30, RBRACE = 31, COMMA = 32, 
    SEMICOLON = 33, ID = 34, ILITERAL = 35, FLITERAL = 36, STRING = 37, 
    WS = 38, LINECOMMENT = 39, BLOCKCOMMENT = 40
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

