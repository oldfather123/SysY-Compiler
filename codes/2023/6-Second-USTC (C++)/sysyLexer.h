
// Generated from /home/jpy794/actions-runner/_work/sysy-compiler/sysy-compiler/src/ast/grammer/sysy.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"




class  sysyLexer : public antlr4::Lexer {
public:
  enum {
    Comma = 1, SemiColon = 2, Assign = 3, LeftBracket = 4, RightBracket = 5, 
    LeftBrace = 6, RightBrace = 7, LeftParen = 8, RightParen = 9, If = 10, 
    Else = 11, While = 12, Break = 13, Continue = 14, Return = 15, Const = 16, 
    Equal = 17, NonEqual = 18, Less = 19, Greater = 20, LessEqual = 21, 
    GreaterEqual = 22, Not = 23, And = 24, Or = 25, Plus = 26, Minus = 27, 
    Multiply = 28, Divide = 29, Modulo = 30, Int = 31, Float = 32, Void = 33, 
    Identifier = 34, IntConst = 35, FloatConst = 36, LineComment = 37, BlockComment = 38, 
    WhiteSpace = 39
  };

  explicit sysyLexer(antlr4::CharStream *input);

  ~sysyLexer() override;


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

