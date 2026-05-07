// A Bison parser, made by GNU Bison 3.5.1.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.





#include "sysY_parser.hh"


// Unqualified %code blocks.
#line 31 "sysY_parser.yy"

#include "sysY_driver.hh"
#define yylex (driver.lexer->yylex)

#line 50 "sysY_parser.cc"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

namespace yy {
#line 141 "sysY_parser.cc"


  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  sysY_parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  sysY_parser::sysY_parser (sysY_driver& driver_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      driver (driver_yyarg)
  {}

  sysY_parser::~sysY_parser ()
  {}

  sysY_parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/



  // by_state.
  sysY_parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  sysY_parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  sysY_parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  sysY_parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  sysY_parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  sysY_parser::symbol_number_type
  sysY_parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[+state];
  }

  sysY_parser::stack_symbol_type::stack_symbol_type ()
  {}

  sysY_parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 90: // AddExp
        value.YY_MOVE_OR_COPY< ASTAddExp* > (YY_MOVE (that.value));
        break;

      case 55: // ArrayConstExpList
        value.YY_MOVE_OR_COPY< ASTArrayConstExpList* > (YY_MOVE (that.value));
        break;

      case 82: // ArrayExpList
        value.YY_MOVE_OR_COPY< ASTArrayExpList* > (YY_MOVE (that.value));
        break;

      case 71: // AssignStmt
        value.YY_MOVE_OR_COPY< ASTAssignStmt* > (YY_MOVE (that.value));
        break;

      case 67: // Block
        value.YY_MOVE_OR_COPY< ASTBlock* > (YY_MOVE (that.value));
        break;

      case 69: // BlockItem
        value.YY_MOVE_OR_COPY< ASTBlockItem* > (YY_MOVE (that.value));
        break;

      case 68: // BlockItemList
        value.YY_MOVE_OR_COPY< ASTBlockItemList* > (YY_MOVE (that.value));
        break;

      case 73: // BlockStmt
        value.YY_MOVE_OR_COPY< ASTBlockStmt* > (YY_MOVE (that.value));
        break;

      case 76: // BreakStmt
        value.YY_MOVE_OR_COPY< ASTBreakStmt* > (YY_MOVE (that.value));
        break;

      case 86: // Callee
        value.YY_MOVE_OR_COPY< ASTCallee* > (YY_MOVE (that.value));
        break;

      case 49: // CompUnit
        value.YY_MOVE_OR_COPY< ASTCompUnit* > (YY_MOVE (that.value));
        break;

      case 80: // Cond
        value.YY_MOVE_OR_COPY< ASTCond* > (YY_MOVE (that.value));
        break;

      case 51: // ConstDecl
        value.YY_MOVE_OR_COPY< ASTConstDeclaration* > (YY_MOVE (that.value));
        break;

      case 54: // ConstDef
        value.YY_MOVE_OR_COPY< ASTConstDef* > (YY_MOVE (that.value));
        break;

      case 53: // ConstDefList
        value.YY_MOVE_OR_COPY< ASTConstDefList* > (YY_MOVE (that.value));
        break;

      case 95: // ConstExp
        value.YY_MOVE_OR_COPY< ASTConstExp* > (YY_MOVE (that.value));
        break;

      case 56: // ConstInitVal
        value.YY_MOVE_OR_COPY< ASTConstInitVal* > (YY_MOVE (that.value));
        break;

      case 57: // ConstInitValList
        value.YY_MOVE_OR_COPY< ASTConstInitValList* > (YY_MOVE (that.value));
        break;

      case 77: // ContinueStmt
        value.YY_MOVE_OR_COPY< ASTContinueStmt* > (YY_MOVE (that.value));
        break;

      case 92: // EqExp
        value.YY_MOVE_OR_COPY< ASTEqExp* > (YY_MOVE (that.value));
        break;

      case 79: // Exp
        value.YY_MOVE_OR_COPY< ASTExp* > (YY_MOVE (that.value));
        break;

      case 72: // ExpStmt
        value.YY_MOVE_OR_COPY< ASTExpStmt* > (YY_MOVE (that.value));
        break;

      case 63: // FuncDef
        value.YY_MOVE_OR_COPY< ASTFuncDef* > (YY_MOVE (that.value));
        break;

      case 65: // FuncFParam
        value.YY_MOVE_OR_COPY< ASTFuncFParam* > (YY_MOVE (that.value));
        break;

      case 64: // FuncFParamList
        value.YY_MOVE_OR_COPY< ASTFuncFParamList* > (YY_MOVE (that.value));
        break;

      case 61: // InitVal
        value.YY_MOVE_OR_COPY< ASTInitVal* > (YY_MOVE (that.value));
        break;

      case 62: // InitValList
        value.YY_MOVE_OR_COPY< ASTInitValList* > (YY_MOVE (that.value));
        break;

      case 75: // IterationStmt
        value.YY_MOVE_OR_COPY< ASTIterationStmt* > (YY_MOVE (that.value));
        break;

      case 93: // LAndExp
        value.YY_MOVE_OR_COPY< ASTLAndExp* > (YY_MOVE (that.value));
        break;

      case 94: // LOrExp
        value.YY_MOVE_OR_COPY< ASTLOrExp* > (YY_MOVE (that.value));
        break;

      case 81: // LVal
        value.YY_MOVE_OR_COPY< ASTLVal* > (YY_MOVE (that.value));
        break;

      case 89: // MulExp
        value.YY_MOVE_OR_COPY< ASTMulExp* > (YY_MOVE (that.value));
        break;

      case 84: // Number
        value.YY_MOVE_OR_COPY< ASTNumber* > (YY_MOVE (that.value));
        break;

      case 66: // ParamArrayExpList
        value.YY_MOVE_OR_COPY< ASTParamArrayExpList* > (YY_MOVE (that.value));
        break;

      case 88: // ExpList
        value.YY_MOVE_OR_COPY< ASTParamExpList* > (YY_MOVE (that.value));
        break;

      case 83: // PrimaryExp
        value.YY_MOVE_OR_COPY< ASTPrimaryExp* > (YY_MOVE (that.value));
        break;

      case 91: // RelExp
        value.YY_MOVE_OR_COPY< ASTRelExp* > (YY_MOVE (that.value));
        break;

      case 78: // ReturnStmt
        value.YY_MOVE_OR_COPY< ASTReturnStmt* > (YY_MOVE (that.value));
        break;

      case 74: // SelectStmt
        value.YY_MOVE_OR_COPY< ASTSelectStmt* > (YY_MOVE (that.value));
        break;

      case 52: // BType
        value.YY_MOVE_OR_COPY< ASTType > (YY_MOVE (that.value));
        break;

      case 85: // UnaryExp
        value.YY_MOVE_OR_COPY< ASTUnaryExp* > (YY_MOVE (that.value));
        break;

      case 58: // VarDecl
        value.YY_MOVE_OR_COPY< ASTVarDeclaration* > (YY_MOVE (that.value));
        break;

      case 60: // VarDef
        value.YY_MOVE_OR_COPY< ASTVarDef* > (YY_MOVE (that.value));
        break;

      case 59: // VarDefList
        value.YY_MOVE_OR_COPY< ASTVarDefList* > (YY_MOVE (that.value));
        break;

      case 87: // UnaryOp
        value.YY_MOVE_OR_COPY< UnaryOp > (YY_MOVE (that.value));
        break;

      case 44: // FLOATCONST
        value.YY_MOVE_OR_COPY< float > (YY_MOVE (that.value));
        break;

      case 45: // INTCONST
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case 50: // DeclDef
        value.YY_MOVE_OR_COPY< std::shared_ptr<ASTDeclaration> > (YY_MOVE (that.value));
        break;

      case 70: // Stmt
        value.YY_MOVE_OR_COPY< std::shared_ptr<ASTStmt> > (YY_MOVE (that.value));
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  sysY_parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 90: // AddExp
        value.move< ASTAddExp* > (YY_MOVE (that.value));
        break;

      case 55: // ArrayConstExpList
        value.move< ASTArrayConstExpList* > (YY_MOVE (that.value));
        break;

      case 82: // ArrayExpList
        value.move< ASTArrayExpList* > (YY_MOVE (that.value));
        break;

      case 71: // AssignStmt
        value.move< ASTAssignStmt* > (YY_MOVE (that.value));
        break;

      case 67: // Block
        value.move< ASTBlock* > (YY_MOVE (that.value));
        break;

      case 69: // BlockItem
        value.move< ASTBlockItem* > (YY_MOVE (that.value));
        break;

      case 68: // BlockItemList
        value.move< ASTBlockItemList* > (YY_MOVE (that.value));
        break;

      case 73: // BlockStmt
        value.move< ASTBlockStmt* > (YY_MOVE (that.value));
        break;

      case 76: // BreakStmt
        value.move< ASTBreakStmt* > (YY_MOVE (that.value));
        break;

      case 86: // Callee
        value.move< ASTCallee* > (YY_MOVE (that.value));
        break;

      case 49: // CompUnit
        value.move< ASTCompUnit* > (YY_MOVE (that.value));
        break;

      case 80: // Cond
        value.move< ASTCond* > (YY_MOVE (that.value));
        break;

      case 51: // ConstDecl
        value.move< ASTConstDeclaration* > (YY_MOVE (that.value));
        break;

      case 54: // ConstDef
        value.move< ASTConstDef* > (YY_MOVE (that.value));
        break;

      case 53: // ConstDefList
        value.move< ASTConstDefList* > (YY_MOVE (that.value));
        break;

      case 95: // ConstExp
        value.move< ASTConstExp* > (YY_MOVE (that.value));
        break;

      case 56: // ConstInitVal
        value.move< ASTConstInitVal* > (YY_MOVE (that.value));
        break;

      case 57: // ConstInitValList
        value.move< ASTConstInitValList* > (YY_MOVE (that.value));
        break;

      case 77: // ContinueStmt
        value.move< ASTContinueStmt* > (YY_MOVE (that.value));
        break;

      case 92: // EqExp
        value.move< ASTEqExp* > (YY_MOVE (that.value));
        break;

      case 79: // Exp
        value.move< ASTExp* > (YY_MOVE (that.value));
        break;

      case 72: // ExpStmt
        value.move< ASTExpStmt* > (YY_MOVE (that.value));
        break;

      case 63: // FuncDef
        value.move< ASTFuncDef* > (YY_MOVE (that.value));
        break;

      case 65: // FuncFParam
        value.move< ASTFuncFParam* > (YY_MOVE (that.value));
        break;

      case 64: // FuncFParamList
        value.move< ASTFuncFParamList* > (YY_MOVE (that.value));
        break;

      case 61: // InitVal
        value.move< ASTInitVal* > (YY_MOVE (that.value));
        break;

      case 62: // InitValList
        value.move< ASTInitValList* > (YY_MOVE (that.value));
        break;

      case 75: // IterationStmt
        value.move< ASTIterationStmt* > (YY_MOVE (that.value));
        break;

      case 93: // LAndExp
        value.move< ASTLAndExp* > (YY_MOVE (that.value));
        break;

      case 94: // LOrExp
        value.move< ASTLOrExp* > (YY_MOVE (that.value));
        break;

      case 81: // LVal
        value.move< ASTLVal* > (YY_MOVE (that.value));
        break;

      case 89: // MulExp
        value.move< ASTMulExp* > (YY_MOVE (that.value));
        break;

      case 84: // Number
        value.move< ASTNumber* > (YY_MOVE (that.value));
        break;

      case 66: // ParamArrayExpList
        value.move< ASTParamArrayExpList* > (YY_MOVE (that.value));
        break;

      case 88: // ExpList
        value.move< ASTParamExpList* > (YY_MOVE (that.value));
        break;

      case 83: // PrimaryExp
        value.move< ASTPrimaryExp* > (YY_MOVE (that.value));
        break;

      case 91: // RelExp
        value.move< ASTRelExp* > (YY_MOVE (that.value));
        break;

      case 78: // ReturnStmt
        value.move< ASTReturnStmt* > (YY_MOVE (that.value));
        break;

      case 74: // SelectStmt
        value.move< ASTSelectStmt* > (YY_MOVE (that.value));
        break;

      case 52: // BType
        value.move< ASTType > (YY_MOVE (that.value));
        break;

      case 85: // UnaryExp
        value.move< ASTUnaryExp* > (YY_MOVE (that.value));
        break;

      case 58: // VarDecl
        value.move< ASTVarDeclaration* > (YY_MOVE (that.value));
        break;

      case 60: // VarDef
        value.move< ASTVarDef* > (YY_MOVE (that.value));
        break;

      case 59: // VarDefList
        value.move< ASTVarDefList* > (YY_MOVE (that.value));
        break;

      case 87: // UnaryOp
        value.move< UnaryOp > (YY_MOVE (that.value));
        break;

      case 44: // FLOATCONST
        value.move< float > (YY_MOVE (that.value));
        break;

      case 45: // INTCONST
        value.move< int > (YY_MOVE (that.value));
        break;

      case 50: // DeclDef
        value.move< std::shared_ptr<ASTDeclaration> > (YY_MOVE (that.value));
        break;

      case 70: // Stmt
        value.move< std::shared_ptr<ASTStmt> > (YY_MOVE (that.value));
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.move< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  sysY_parser::stack_symbol_type&
  sysY_parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 90: // AddExp
        value.copy< ASTAddExp* > (that.value);
        break;

      case 55: // ArrayConstExpList
        value.copy< ASTArrayConstExpList* > (that.value);
        break;

      case 82: // ArrayExpList
        value.copy< ASTArrayExpList* > (that.value);
        break;

      case 71: // AssignStmt
        value.copy< ASTAssignStmt* > (that.value);
        break;

      case 67: // Block
        value.copy< ASTBlock* > (that.value);
        break;

      case 69: // BlockItem
        value.copy< ASTBlockItem* > (that.value);
        break;

      case 68: // BlockItemList
        value.copy< ASTBlockItemList* > (that.value);
        break;

      case 73: // BlockStmt
        value.copy< ASTBlockStmt* > (that.value);
        break;

      case 76: // BreakStmt
        value.copy< ASTBreakStmt* > (that.value);
        break;

      case 86: // Callee
        value.copy< ASTCallee* > (that.value);
        break;

      case 49: // CompUnit
        value.copy< ASTCompUnit* > (that.value);
        break;

      case 80: // Cond
        value.copy< ASTCond* > (that.value);
        break;

      case 51: // ConstDecl
        value.copy< ASTConstDeclaration* > (that.value);
        break;

      case 54: // ConstDef
        value.copy< ASTConstDef* > (that.value);
        break;

      case 53: // ConstDefList
        value.copy< ASTConstDefList* > (that.value);
        break;

      case 95: // ConstExp
        value.copy< ASTConstExp* > (that.value);
        break;

      case 56: // ConstInitVal
        value.copy< ASTConstInitVal* > (that.value);
        break;

      case 57: // ConstInitValList
        value.copy< ASTConstInitValList* > (that.value);
        break;

      case 77: // ContinueStmt
        value.copy< ASTContinueStmt* > (that.value);
        break;

      case 92: // EqExp
        value.copy< ASTEqExp* > (that.value);
        break;

      case 79: // Exp
        value.copy< ASTExp* > (that.value);
        break;

      case 72: // ExpStmt
        value.copy< ASTExpStmt* > (that.value);
        break;

      case 63: // FuncDef
        value.copy< ASTFuncDef* > (that.value);
        break;

      case 65: // FuncFParam
        value.copy< ASTFuncFParam* > (that.value);
        break;

      case 64: // FuncFParamList
        value.copy< ASTFuncFParamList* > (that.value);
        break;

      case 61: // InitVal
        value.copy< ASTInitVal* > (that.value);
        break;

      case 62: // InitValList
        value.copy< ASTInitValList* > (that.value);
        break;

      case 75: // IterationStmt
        value.copy< ASTIterationStmt* > (that.value);
        break;

      case 93: // LAndExp
        value.copy< ASTLAndExp* > (that.value);
        break;

      case 94: // LOrExp
        value.copy< ASTLOrExp* > (that.value);
        break;

      case 81: // LVal
        value.copy< ASTLVal* > (that.value);
        break;

      case 89: // MulExp
        value.copy< ASTMulExp* > (that.value);
        break;

      case 84: // Number
        value.copy< ASTNumber* > (that.value);
        break;

      case 66: // ParamArrayExpList
        value.copy< ASTParamArrayExpList* > (that.value);
        break;

      case 88: // ExpList
        value.copy< ASTParamExpList* > (that.value);
        break;

      case 83: // PrimaryExp
        value.copy< ASTPrimaryExp* > (that.value);
        break;

      case 91: // RelExp
        value.copy< ASTRelExp* > (that.value);
        break;

      case 78: // ReturnStmt
        value.copy< ASTReturnStmt* > (that.value);
        break;

      case 74: // SelectStmt
        value.copy< ASTSelectStmt* > (that.value);
        break;

      case 52: // BType
        value.copy< ASTType > (that.value);
        break;

      case 85: // UnaryExp
        value.copy< ASTUnaryExp* > (that.value);
        break;

      case 58: // VarDecl
        value.copy< ASTVarDeclaration* > (that.value);
        break;

      case 60: // VarDef
        value.copy< ASTVarDef* > (that.value);
        break;

      case 59: // VarDefList
        value.copy< ASTVarDefList* > (that.value);
        break;

      case 87: // UnaryOp
        value.copy< UnaryOp > (that.value);
        break;

      case 44: // FLOATCONST
        value.copy< float > (that.value);
        break;

      case 45: // INTCONST
        value.copy< int > (that.value);
        break;

      case 50: // DeclDef
        value.copy< std::shared_ptr<ASTDeclaration> > (that.value);
        break;

      case 70: // Stmt
        value.copy< std::shared_ptr<ASTStmt> > (that.value);
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.copy< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  sysY_parser::stack_symbol_type&
  sysY_parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 90: // AddExp
        value.move< ASTAddExp* > (that.value);
        break;

      case 55: // ArrayConstExpList
        value.move< ASTArrayConstExpList* > (that.value);
        break;

      case 82: // ArrayExpList
        value.move< ASTArrayExpList* > (that.value);
        break;

      case 71: // AssignStmt
        value.move< ASTAssignStmt* > (that.value);
        break;

      case 67: // Block
        value.move< ASTBlock* > (that.value);
        break;

      case 69: // BlockItem
        value.move< ASTBlockItem* > (that.value);
        break;

      case 68: // BlockItemList
        value.move< ASTBlockItemList* > (that.value);
        break;

      case 73: // BlockStmt
        value.move< ASTBlockStmt* > (that.value);
        break;

      case 76: // BreakStmt
        value.move< ASTBreakStmt* > (that.value);
        break;

      case 86: // Callee
        value.move< ASTCallee* > (that.value);
        break;

      case 49: // CompUnit
        value.move< ASTCompUnit* > (that.value);
        break;

      case 80: // Cond
        value.move< ASTCond* > (that.value);
        break;

      case 51: // ConstDecl
        value.move< ASTConstDeclaration* > (that.value);
        break;

      case 54: // ConstDef
        value.move< ASTConstDef* > (that.value);
        break;

      case 53: // ConstDefList
        value.move< ASTConstDefList* > (that.value);
        break;

      case 95: // ConstExp
        value.move< ASTConstExp* > (that.value);
        break;

      case 56: // ConstInitVal
        value.move< ASTConstInitVal* > (that.value);
        break;

      case 57: // ConstInitValList
        value.move< ASTConstInitValList* > (that.value);
        break;

      case 77: // ContinueStmt
        value.move< ASTContinueStmt* > (that.value);
        break;

      case 92: // EqExp
        value.move< ASTEqExp* > (that.value);
        break;

      case 79: // Exp
        value.move< ASTExp* > (that.value);
        break;

      case 72: // ExpStmt
        value.move< ASTExpStmt* > (that.value);
        break;

      case 63: // FuncDef
        value.move< ASTFuncDef* > (that.value);
        break;

      case 65: // FuncFParam
        value.move< ASTFuncFParam* > (that.value);
        break;

      case 64: // FuncFParamList
        value.move< ASTFuncFParamList* > (that.value);
        break;

      case 61: // InitVal
        value.move< ASTInitVal* > (that.value);
        break;

      case 62: // InitValList
        value.move< ASTInitValList* > (that.value);
        break;

      case 75: // IterationStmt
        value.move< ASTIterationStmt* > (that.value);
        break;

      case 93: // LAndExp
        value.move< ASTLAndExp* > (that.value);
        break;

      case 94: // LOrExp
        value.move< ASTLOrExp* > (that.value);
        break;

      case 81: // LVal
        value.move< ASTLVal* > (that.value);
        break;

      case 89: // MulExp
        value.move< ASTMulExp* > (that.value);
        break;

      case 84: // Number
        value.move< ASTNumber* > (that.value);
        break;

      case 66: // ParamArrayExpList
        value.move< ASTParamArrayExpList* > (that.value);
        break;

      case 88: // ExpList
        value.move< ASTParamExpList* > (that.value);
        break;

      case 83: // PrimaryExp
        value.move< ASTPrimaryExp* > (that.value);
        break;

      case 91: // RelExp
        value.move< ASTRelExp* > (that.value);
        break;

      case 78: // ReturnStmt
        value.move< ASTReturnStmt* > (that.value);
        break;

      case 74: // SelectStmt
        value.move< ASTSelectStmt* > (that.value);
        break;

      case 52: // BType
        value.move< ASTType > (that.value);
        break;

      case 85: // UnaryExp
        value.move< ASTUnaryExp* > (that.value);
        break;

      case 58: // VarDecl
        value.move< ASTVarDeclaration* > (that.value);
        break;

      case 60: // VarDef
        value.move< ASTVarDef* > (that.value);
        break;

      case 59: // VarDefList
        value.move< ASTVarDefList* > (that.value);
        break;

      case 87: // UnaryOp
        value.move< UnaryOp > (that.value);
        break;

      case 44: // FLOATCONST
        value.move< float > (that.value);
        break;

      case 45: // INTCONST
        value.move< int > (that.value);
        break;

      case 50: // DeclDef
        value.move< std::shared_ptr<ASTDeclaration> > (that.value);
        break;

      case 70: // Stmt
        value.move< std::shared_ptr<ASTStmt> > (that.value);
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  sysY_parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  sysY_parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  sysY_parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  sysY_parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  sysY_parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  sysY_parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  sysY_parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  sysY_parser::debug_level_type
  sysY_parser::debug_level () const
  {
    return yydebug_;
  }

  void
  sysY_parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  sysY_parser::state_type
  sysY_parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  sysY_parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  sysY_parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  sysY_parser::operator() ()
  {
    return parse ();
  }

  int
  sysY_parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    // User initialization code.
#line 22 "sysY_parser.yy"
{
    (yyla.location).begin.filename = (yyla.location).end.filename = &driver.file;
}

#line 1254 "sysY_parser.cc"


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (driver));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 90: // AddExp
        yylhs.value.emplace< ASTAddExp* > ();
        break;

      case 55: // ArrayConstExpList
        yylhs.value.emplace< ASTArrayConstExpList* > ();
        break;

      case 82: // ArrayExpList
        yylhs.value.emplace< ASTArrayExpList* > ();
        break;

      case 71: // AssignStmt
        yylhs.value.emplace< ASTAssignStmt* > ();
        break;

      case 67: // Block
        yylhs.value.emplace< ASTBlock* > ();
        break;

      case 69: // BlockItem
        yylhs.value.emplace< ASTBlockItem* > ();
        break;

      case 68: // BlockItemList
        yylhs.value.emplace< ASTBlockItemList* > ();
        break;

      case 73: // BlockStmt
        yylhs.value.emplace< ASTBlockStmt* > ();
        break;

      case 76: // BreakStmt
        yylhs.value.emplace< ASTBreakStmt* > ();
        break;

      case 86: // Callee
        yylhs.value.emplace< ASTCallee* > ();
        break;

      case 49: // CompUnit
        yylhs.value.emplace< ASTCompUnit* > ();
        break;

      case 80: // Cond
        yylhs.value.emplace< ASTCond* > ();
        break;

      case 51: // ConstDecl
        yylhs.value.emplace< ASTConstDeclaration* > ();
        break;

      case 54: // ConstDef
        yylhs.value.emplace< ASTConstDef* > ();
        break;

      case 53: // ConstDefList
        yylhs.value.emplace< ASTConstDefList* > ();
        break;

      case 95: // ConstExp
        yylhs.value.emplace< ASTConstExp* > ();
        break;

      case 56: // ConstInitVal
        yylhs.value.emplace< ASTConstInitVal* > ();
        break;

      case 57: // ConstInitValList
        yylhs.value.emplace< ASTConstInitValList* > ();
        break;

      case 77: // ContinueStmt
        yylhs.value.emplace< ASTContinueStmt* > ();
        break;

      case 92: // EqExp
        yylhs.value.emplace< ASTEqExp* > ();
        break;

      case 79: // Exp
        yylhs.value.emplace< ASTExp* > ();
        break;

      case 72: // ExpStmt
        yylhs.value.emplace< ASTExpStmt* > ();
        break;

      case 63: // FuncDef
        yylhs.value.emplace< ASTFuncDef* > ();
        break;

      case 65: // FuncFParam
        yylhs.value.emplace< ASTFuncFParam* > ();
        break;

      case 64: // FuncFParamList
        yylhs.value.emplace< ASTFuncFParamList* > ();
        break;

      case 61: // InitVal
        yylhs.value.emplace< ASTInitVal* > ();
        break;

      case 62: // InitValList
        yylhs.value.emplace< ASTInitValList* > ();
        break;

      case 75: // IterationStmt
        yylhs.value.emplace< ASTIterationStmt* > ();
        break;

      case 93: // LAndExp
        yylhs.value.emplace< ASTLAndExp* > ();
        break;

      case 94: // LOrExp
        yylhs.value.emplace< ASTLOrExp* > ();
        break;

      case 81: // LVal
        yylhs.value.emplace< ASTLVal* > ();
        break;

      case 89: // MulExp
        yylhs.value.emplace< ASTMulExp* > ();
        break;

      case 84: // Number
        yylhs.value.emplace< ASTNumber* > ();
        break;

      case 66: // ParamArrayExpList
        yylhs.value.emplace< ASTParamArrayExpList* > ();
        break;

      case 88: // ExpList
        yylhs.value.emplace< ASTParamExpList* > ();
        break;

      case 83: // PrimaryExp
        yylhs.value.emplace< ASTPrimaryExp* > ();
        break;

      case 91: // RelExp
        yylhs.value.emplace< ASTRelExp* > ();
        break;

      case 78: // ReturnStmt
        yylhs.value.emplace< ASTReturnStmt* > ();
        break;

      case 74: // SelectStmt
        yylhs.value.emplace< ASTSelectStmt* > ();
        break;

      case 52: // BType
        yylhs.value.emplace< ASTType > ();
        break;

      case 85: // UnaryExp
        yylhs.value.emplace< ASTUnaryExp* > ();
        break;

      case 58: // VarDecl
        yylhs.value.emplace< ASTVarDeclaration* > ();
        break;

      case 60: // VarDef
        yylhs.value.emplace< ASTVarDef* > ();
        break;

      case 59: // VarDefList
        yylhs.value.emplace< ASTVarDefList* > ();
        break;

      case 87: // UnaryOp
        yylhs.value.emplace< UnaryOp > ();
        break;

      case 44: // FLOATCONST
        yylhs.value.emplace< float > ();
        break;

      case 45: // INTCONST
        yylhs.value.emplace< int > ();
        break;

      case 50: // DeclDef
        yylhs.value.emplace< std::shared_ptr<ASTDeclaration> > ();
        break;

      case 70: // Stmt
        yylhs.value.emplace< std::shared_ptr<ASTStmt> > ();
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        yylhs.value.emplace< std::string > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2:
#line 107 "sysY_parser.yy"
                   {
    auto printer = new ASTPrinter;
    yystack_[1].value.as < ASTCompUnit* > ()->lineno = (yylhs.location).begin.line;
    driver.root = yystack_[1].value.as < ASTCompUnit* > ();
    return 0;
}
#line 1587 "sysY_parser.cc"
    break;

  case 3:
#line 114 "sysY_parser.yy"
                         {
    yystack_[1].value.as < ASTCompUnit* > ()->Declaration_list.push_back(yystack_[0].value.as < std::shared_ptr<ASTDeclaration> > ());
    yylhs.value.as < ASTCompUnit* > ()=yystack_[1].value.as < ASTCompUnit* > ();
}
#line 1596 "sysY_parser.cc"
    break;

  case 4:
#line 118 "sysY_parser.yy"
         {
    yylhs.value.as < ASTCompUnit* > () = new ASTCompUnit();
    yylhs.value.as < ASTCompUnit* > ()->Declaration_list.push_back(yystack_[0].value.as < std::shared_ptr<ASTDeclaration> > ());
    yylhs.value.as < ASTCompUnit* > ()->lineno = (yylhs.location).begin.line;
}
#line 1606 "sysY_parser.cc"
    break;

  case 5:
#line 124 "sysY_parser.yy"
                 {
    yylhs.value.as < std::shared_ptr<ASTDeclaration> > () = std::shared_ptr<ASTDeclaration>(yystack_[0].value.as < ASTConstDeclaration* > ());
}
#line 1614 "sysY_parser.cc"
    break;

  case 6:
#line 127 "sysY_parser.yy"
         {
    yylhs.value.as < std::shared_ptr<ASTDeclaration> > () = std::shared_ptr<ASTDeclaration>(yystack_[0].value.as < ASTVarDeclaration* > ());
}
#line 1622 "sysY_parser.cc"
    break;

  case 7:
#line 130 "sysY_parser.yy"
         {
    yylhs.value.as < std::shared_ptr<ASTDeclaration> > () = std::shared_ptr<ASTDeclaration>(yystack_[0].value.as < ASTFuncDef* > ());
}
#line 1630 "sysY_parser.cc"
    break;

  case 8:
#line 134 "sysY_parser.yy"
                                            {
    yylhs.value.as < ASTConstDeclaration* > () = new ASTConstDeclaration();
    yylhs.value.as < ASTConstDeclaration* > ()->type = yystack_[2].value.as < ASTType > ();
    yylhs.value.as < ASTConstDeclaration* > ()->ConstDef_list.swap(yystack_[1].value.as < ASTConstDefList* > ()->ConstDef_list);
    yylhs.value.as < ASTConstDeclaration* > ()->lineno = (yylhs.location).begin.line;
}
#line 1641 "sysY_parser.cc"
    break;

  case 9:
#line 141 "sysY_parser.yy"
          {yylhs.value.as < ASTType > ()=ASTType::TYPE_INT; }
#line 1647 "sysY_parser.cc"
    break;

  case 10:
#line 142 "sysY_parser.yy"
       {yylhs.value.as < ASTType > ()=ASTType::TYPE_FLOAT; }
#line 1653 "sysY_parser.cc"
    break;

  case 11:
#line 143 "sysY_parser.yy"
       {yylhs.value.as < ASTType > ()=ASTType::TYPE_VOID; }
#line 1659 "sysY_parser.cc"
    break;

  case 12:
#line 145 "sysY_parser.yy"
                                        {
    yystack_[2].value.as < ASTConstDefList* > ()->ConstDef_list.push_back(std::shared_ptr<ASTConstDef>(yystack_[0].value.as < ASTConstDef* > ()));
    yylhs.value.as < ASTConstDefList* > ()=yystack_[2].value.as < ASTConstDefList* > ();
}
#line 1668 "sysY_parser.cc"
    break;

  case 13:
#line 149 "sysY_parser.yy"
          {
    yylhs.value.as < ASTConstDefList* > () = new ASTConstDefList();
    yylhs.value.as < ASTConstDefList* > ()->ConstDef_list.push_back(std::shared_ptr<ASTConstDef>(yystack_[0].value.as < ASTConstDef* > ()));
}
#line 1677 "sysY_parser.cc"
    break;

  case 14:
#line 154 "sysY_parser.yy"
                                                        {
    yylhs.value.as < ASTConstDef* > () = new ASTConstDef();
    yylhs.value.as < ASTConstDef* > ()->lineno = (yylhs.location).begin.line;
    yylhs.value.as < ASTConstDef* > ()->id = yystack_[3].value.as < std::string > ();
    yylhs.value.as < ASTConstDef* > ()->ConstInitVal = std::shared_ptr<ASTConstInitVal>(yystack_[0].value.as < ASTConstInitVal* > ());
    yylhs.value.as < ASTConstDef* > ()->ConstExp_list.swap(yystack_[2].value.as < ASTArrayConstExpList* > ()->ConstExp_list);
}
#line 1689 "sysY_parser.cc"
    break;

  case 15:
#line 162 "sysY_parser.yy"
                                                              {
    yystack_[3].value.as < ASTArrayConstExpList* > ()->ConstExp_list.push_back(std::shared_ptr<ASTConstExp>(yystack_[1].value.as < ASTConstExp* > ()));
    yylhs.value.as < ASTArrayConstExpList* > ()=yystack_[3].value.as < ASTArrayConstExpList* > ();
}
#line 1698 "sysY_parser.cc"
    break;

  case 16:
#line 166 "sysY_parser.yy"
          {
    yylhs.value.as < ASTArrayConstExpList* > () = new ASTArrayConstExpList();
}
#line 1706 "sysY_parser.cc"
    break;

  case 17:
#line 170 "sysY_parser.yy"
                     {
    yylhs.value.as < ASTConstInitVal* > () = new ASTConstInitVal();
    yylhs.value.as < ASTConstInitVal* > ()->ConstExp = std::shared_ptr<ASTConstExp>(yystack_[0].value.as < ASTConstExp* > ());
    yylhs.value.as < ASTConstInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1716 "sysY_parser.cc"
    break;

  case 18:
#line 175 "sysY_parser.yy"
                                {
    yylhs.value.as < ASTConstInitVal* > () = new ASTConstInitVal();
    yylhs.value.as < ASTConstInitVal* > ()->ConstExp=nullptr;
    yylhs.value.as < ASTConstInitVal* > ()->ConstInitVal_list.swap(yystack_[1].value.as < ASTConstInitValList* > ()->ConstInitVal_list);
    yylhs.value.as < ASTConstInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1727 "sysY_parser.cc"
    break;

  case 19:
#line 181 "sysY_parser.yy"
               {
    yylhs.value.as < ASTConstInitVal* > () = new ASTConstInitVal();
    yylhs.value.as < ASTConstInitVal* > ()->ConstExp=nullptr;
    yylhs.value.as < ASTConstInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1737 "sysY_parser.cc"
    break;

  case 20:
#line 187 "sysY_parser.yy"
                                                    {
    yystack_[2].value.as < ASTConstInitValList* > ()->ConstInitVal_list.push_back(std::shared_ptr<ASTConstInitVal>(yystack_[0].value.as < ASTConstInitVal* > ()));
    yylhs.value.as < ASTConstInitValList* > ()=yystack_[2].value.as < ASTConstInitValList* > ();
}
#line 1746 "sysY_parser.cc"
    break;

  case 21:
#line 191 "sysY_parser.yy"
              {
    yylhs.value.as < ASTConstInitValList* > () = new ASTConstInitValList();
    yylhs.value.as < ASTConstInitValList* > ()->ConstInitVal_list.push_back(std::shared_ptr<ASTConstInitVal>(yystack_[0].value.as < ASTConstInitVal* > ()));
}
#line 1755 "sysY_parser.cc"
    break;

  case 22:
#line 196 "sysY_parser.yy"
                                  {
    yylhs.value.as < ASTVarDeclaration* > () = new ASTVarDeclaration();
    yylhs.value.as < ASTVarDeclaration* > ()->type = yystack_[2].value.as < ASTType > ();
    yylhs.value.as < ASTVarDeclaration* > ()->VarDef_list.swap(yystack_[1].value.as < ASTVarDefList* > ()->VarDef_list);
    yylhs.value.as < ASTVarDeclaration* > ()->lineno = (yylhs.location).begin.line;
}
#line 1766 "sysY_parser.cc"
    break;

  case 23:
#line 203 "sysY_parser.yy"
                                  {
    yystack_[2].value.as < ASTVarDefList* > ()->VarDef_list.push_back(std::shared_ptr<ASTVarDef>(yystack_[0].value.as < ASTVarDef* > ()));
    yylhs.value.as < ASTVarDefList* > () = yystack_[2].value.as < ASTVarDefList* > ();
}
#line 1775 "sysY_parser.cc"
    break;

  case 24:
#line 207 "sysY_parser.yy"
        {
    yylhs.value.as < ASTVarDefList* > () = new ASTVarDefList();
    yylhs.value.as < ASTVarDefList* > ()->VarDef_list.push_back(std::shared_ptr<ASTVarDef>(yystack_[0].value.as < ASTVarDef* > ()));
}
#line 1784 "sysY_parser.cc"
    break;

  case 25:
#line 212 "sysY_parser.yy"
                                   {
    yylhs.value.as < ASTVarDef* > () = new ASTVarDef();
    yylhs.value.as < ASTVarDef* > ()->id = yystack_[1].value.as < std::string > ();
    yylhs.value.as < ASTVarDef* > ()->InitVal = nullptr;
    yylhs.value.as < ASTVarDef* > ()->ConstExp_list.swap(yystack_[0].value.as < ASTArrayConstExpList* > ()->ConstExp_list);
    yylhs.value.as < ASTVarDef* > ()->lineno = (yylhs.location).begin.line;
}
#line 1796 "sysY_parser.cc"
    break;

  case 26:
#line 219 "sysY_parser.yy"
                                            {
    yylhs.value.as < ASTVarDef* > () = new ASTVarDef();
    yylhs.value.as < ASTVarDef* > ()->id = yystack_[3].value.as < std::string > ();
    yylhs.value.as < ASTVarDef* > ()->InitVal = std::shared_ptr<ASTInitVal>(yystack_[0].value.as < ASTInitVal* > ());
    yylhs.value.as < ASTVarDef* > ()->ConstExp_list.swap(yystack_[2].value.as < ASTArrayConstExpList* > ()->ConstExp_list);
    yylhs.value.as < ASTVarDef* > ()->lineno = (yylhs.location).begin.line;
}
#line 1808 "sysY_parser.cc"
    break;

  case 27:
#line 227 "sysY_parser.yy"
           {
    yylhs.value.as < ASTInitVal* > ()=new ASTInitVal();
    yylhs.value.as < ASTInitVal* > ()->Exp = std::shared_ptr<ASTExp>(yystack_[0].value.as < ASTExp* > ());
    yylhs.value.as < ASTInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1818 "sysY_parser.cc"
    break;

  case 28:
#line 232 "sysY_parser.yy"
               {
    yylhs.value.as < ASTInitVal* > ()=new ASTInitVal();
    yylhs.value.as < ASTInitVal* > ()->Exp = nullptr;
    yylhs.value.as < ASTInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1828 "sysY_parser.cc"
    break;

  case 29:
#line 237 "sysY_parser.yy"
                           {
    yylhs.value.as < ASTInitVal* > ()=new ASTInitVal();
    yylhs.value.as < ASTInitVal* > ()->Exp = nullptr;
    yylhs.value.as < ASTInitVal* > ()->InitVal_list.swap(yystack_[1].value.as < ASTInitValList* > ()->InitVal_list);
    yylhs.value.as < ASTInitVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 1839 "sysY_parser.cc"
    break;

  case 30:
#line 244 "sysY_parser.yy"
                                     {
    yystack_[2].value.as < ASTInitValList* > ()->InitVal_list.push_back(std::shared_ptr<ASTInitVal>(yystack_[0].value.as < ASTInitVal* > ()));
    yylhs.value.as < ASTInitValList* > () = yystack_[2].value.as < ASTInitValList* > ();
}
#line 1848 "sysY_parser.cc"
    break;

  case 31:
#line 248 "sysY_parser.yy"
         {
    yylhs.value.as < ASTInitValList* > () = new ASTInitValList();
    yylhs.value.as < ASTInitValList* > ()->InitVal_list.push_back(std::shared_ptr<ASTInitVal>(yystack_[0].value.as < ASTInitVal* > ()));
}
#line 1857 "sysY_parser.cc"
    break;

  case 32:
#line 253 "sysY_parser.yy"
                                                      {
    yylhs.value.as < ASTFuncDef* > () = new ASTFuncDef();
    yylhs.value.as < ASTFuncDef* > ()->type = yystack_[4].value.as < ASTType > ();
    yylhs.value.as < ASTFuncDef* > ()->id = yystack_[3].value.as < std::string > ();
    yylhs.value.as < ASTFuncDef* > ()->Block = std::shared_ptr<ASTBlock>(yystack_[0].value.as < ASTBlock* > ());
    yylhs.value.as < ASTFuncDef* > ()->lineno = (yylhs.location).begin.line;
}
#line 1869 "sysY_parser.cc"
    break;

  case 33:
#line 260 "sysY_parser.yy"
                                                               {
    yylhs.value.as < ASTFuncDef* > () = new ASTFuncDef();
    yylhs.value.as < ASTFuncDef* > ()->type = yystack_[5].value.as < ASTType > ();
    yylhs.value.as < ASTFuncDef* > ()->id = yystack_[4].value.as < std::string > ();
    yylhs.value.as < ASTFuncDef* > ()->FuncFParam_list.swap(yystack_[2].value.as < ASTFuncFParamList* > ()->FuncFParam_list);
    yylhs.value.as < ASTFuncDef* > ()->Block = std::shared_ptr<ASTBlock>(yystack_[0].value.as < ASTBlock* > ());
    yylhs.value.as < ASTFuncDef* > ()->lineno = (yylhs.location).begin.line;
}
#line 1882 "sysY_parser.cc"
    break;

  case 34:
#line 269 "sysY_parser.yy"
                                              {
    yystack_[2].value.as < ASTFuncFParamList* > ()->FuncFParam_list.push_back(std::shared_ptr<ASTFuncFParam>(yystack_[0].value.as < ASTFuncFParam* > ()));
    yylhs.value.as < ASTFuncFParamList* > () = yystack_[2].value.as < ASTFuncFParamList* > ();
}
#line 1891 "sysY_parser.cc"
    break;

  case 35:
#line 273 "sysY_parser.yy"
            {
    yylhs.value.as < ASTFuncFParamList* > () = new ASTFuncFParamList();
    yylhs.value.as < ASTFuncFParamList* > ()->FuncFParam_list.push_back(std::shared_ptr<ASTFuncFParam>(yystack_[0].value.as < ASTFuncFParam* > ()));
}
#line 1900 "sysY_parser.cc"
    break;

  case 36:
#line 278 "sysY_parser.yy"
                                             {
    yylhs.value.as < ASTFuncFParam* > () = new ASTFuncFParam();
    yylhs.value.as < ASTFuncFParam* > ()->type = yystack_[2].value.as < ASTType > ();
    yylhs.value.as < ASTFuncFParam* > ()->id = yystack_[1].value.as < std::string > ();
    yylhs.value.as < ASTFuncFParam* > ()->is_array = 1;
    yylhs.value.as < ASTFuncFParam* > ()->ParamArrayExp_list.swap(yystack_[0].value.as < ASTParamArrayExpList* > ()->ParamArrayExp_list);
    yylhs.value.as < ASTFuncFParam* > ()->lineno = (yylhs.location).begin.line;
}
#line 1913 "sysY_parser.cc"
    break;

  case 37:
#line 286 "sysY_parser.yy"
                  {
    yylhs.value.as < ASTFuncFParam* > () = new ASTFuncFParam();
    yylhs.value.as < ASTFuncFParam* > ()->type = yystack_[1].value.as < ASTType > ();
    yylhs.value.as < ASTFuncFParam* > ()->id = yystack_[0].value.as < std::string > ();
    yylhs.value.as < ASTFuncFParam* > ()->is_array = 0;
    yylhs.value.as < ASTFuncFParam* > ()->lineno = (yylhs.location).begin.line;
}
#line 1925 "sysY_parser.cc"
    break;

  case 38:
#line 294 "sysY_parser.yy"
                                                         {
    yystack_[3].value.as < ASTParamArrayExpList* > ()->ParamArrayExp_list.push_back(std::shared_ptr<ASTExp>(yystack_[1].value.as < ASTExp* > ()));
    yylhs.value.as < ASTParamArrayExpList* > () = yystack_[3].value.as < ASTParamArrayExpList* > ();
}
#line 1934 "sysY_parser.cc"
    break;

  case 39:
#line 298 "sysY_parser.yy"
       {
    yylhs.value.as < ASTParamArrayExpList* > () = new ASTParamArrayExpList();
    yylhs.value.as < ASTParamArrayExpList* > ()->ParamArrayExp_list.push_back(nullptr);
}
#line 1943 "sysY_parser.cc"
    break;

  case 40:
#line 303 "sysY_parser.yy"
                                 {
    yylhs.value.as < ASTBlock* > () = new ASTBlock();
    yylhs.value.as < ASTBlock* > ()->BlockItem_list.swap(yystack_[1].value.as < ASTBlockItemList* > ()->BlockItem_list);
    yylhs.value.as < ASTBlock* > ()->lineno = (yylhs.location).begin.line;
}
#line 1953 "sysY_parser.cc"
    break;

  case 41:
#line 309 "sysY_parser.yy"
                                     {
    yystack_[1].value.as < ASTBlockItemList* > ()->BlockItem_list.push_back(std::shared_ptr<ASTBlockItem>(yystack_[0].value.as < ASTBlockItem* > ()));
    yylhs.value.as < ASTBlockItemList* > () = yystack_[1].value.as < ASTBlockItemList* > ();
}
#line 1962 "sysY_parser.cc"
    break;

  case 42:
#line 313 "sysY_parser.yy"
         {
    yylhs.value.as < ASTBlockItemList* > () = new ASTBlockItemList();
}
#line 1970 "sysY_parser.cc"
    break;

  case 43:
#line 317 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTBlockItem* > () = new ASTBlockItem();
    yylhs.value.as < ASTBlockItem* > ()->ConstDecl = std::shared_ptr<ASTConstDeclaration>(yystack_[0].value.as < ASTConstDeclaration* > ());
    yylhs.value.as < ASTBlockItem* > ()->VarDecl = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->Stmt = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->lineno = (yylhs.location).begin.line;
}
#line 1982 "sysY_parser.cc"
    break;

  case 44:
#line 324 "sysY_parser.yy"
         {
    yylhs.value.as < ASTBlockItem* > () = new ASTBlockItem();
    yylhs.value.as < ASTBlockItem* > ()->ConstDecl = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->VarDecl = std::shared_ptr<ASTVarDeclaration>(yystack_[0].value.as < ASTVarDeclaration* > ());
    yylhs.value.as < ASTBlockItem* > ()->Stmt = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->lineno = (yylhs.location).begin.line;
}
#line 1994 "sysY_parser.cc"
    break;

  case 45:
#line 331 "sysY_parser.yy"
      {
    yylhs.value.as < ASTBlockItem* > () = new ASTBlockItem();
    yylhs.value.as < ASTBlockItem* > ()->ConstDecl = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->VarDecl = nullptr;
    yylhs.value.as < ASTBlockItem* > ()->Stmt = std::shared_ptr<ASTStmt>(yystack_[0].value.as < std::shared_ptr<ASTStmt> > ());
    yylhs.value.as < ASTBlockItem* > ()->lineno = (yylhs.location).begin.line;
}
#line 2006 "sysY_parser.cc"
    break;

  case 46:
#line 339 "sysY_parser.yy"
               {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTAssignStmt* > ());
}
#line 2014 "sysY_parser.cc"
    break;

  case 47:
#line 342 "sysY_parser.yy"
                   {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[1].value.as < ASTExpStmt* > ());
}
#line 2022 "sysY_parser.cc"
    break;

  case 48:
#line 345 "sysY_parser.yy"
           {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTBlockStmt* > ());
}
#line 2030 "sysY_parser.cc"
    break;

  case 49:
#line 348 "sysY_parser.yy"
            {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTSelectStmt* > ());
}
#line 2038 "sysY_parser.cc"
    break;

  case 50:
#line 351 "sysY_parser.yy"
               {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTIterationStmt* > ());
}
#line 2046 "sysY_parser.cc"
    break;

  case 51:
#line 354 "sysY_parser.yy"
           {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTBreakStmt* > ());
}
#line 2054 "sysY_parser.cc"
    break;

  case 52:
#line 357 "sysY_parser.yy"
              {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTContinueStmt* > ());
}
#line 2062 "sysY_parser.cc"
    break;

  case 53:
#line 360 "sysY_parser.yy"
            {
    yylhs.value.as < std::shared_ptr<ASTStmt> > () = std::shared_ptr<ASTStmt>(yystack_[0].value.as < ASTReturnStmt* > ());
}
#line 2070 "sysY_parser.cc"
    break;

  case 54:
#line 364 "sysY_parser.yy"
                                   {
    yylhs.value.as < ASTAssignStmt* > () = new ASTAssignStmt();
    yylhs.value.as < ASTAssignStmt* > ()->LVal = std::shared_ptr<ASTLVal>(yystack_[3].value.as < ASTLVal* > ());
    yylhs.value.as < ASTAssignStmt* > ()->Exp = std::shared_ptr<ASTExp>(yystack_[1].value.as < ASTExp* > ());
    yylhs.value.as < ASTAssignStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2081 "sysY_parser.cc"
    break;

  case 55:
#line 371 "sysY_parser.yy"
           {
    yylhs.value.as < ASTExpStmt* > () = new ASTExpStmt();
    yylhs.value.as < ASTExpStmt* > ()->Exp = std::shared_ptr<ASTExp>(yystack_[0].value.as < ASTExp* > ());
    yylhs.value.as < ASTExpStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2091 "sysY_parser.cc"
    break;

  case 56:
#line 376 "sysY_parser.yy"
         {
    yylhs.value.as < ASTExpStmt* > () = new ASTExpStmt();
    yylhs.value.as < ASTExpStmt* > ()->Exp = nullptr;
    yylhs.value.as < ASTExpStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2101 "sysY_parser.cc"
    break;

  case 57:
#line 382 "sysY_parser.yy"
               {
    yylhs.value.as < ASTBlockStmt* > () = new ASTBlockStmt();
    yylhs.value.as < ASTBlockStmt* > ()->Block = std::shared_ptr<ASTBlock>(yystack_[0].value.as < ASTBlock* > ());
    yylhs.value.as < ASTBlockStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2111 "sysY_parser.cc"
    break;

  case 58:
#line 388 "sysY_parser.yy"
                                               {
    yylhs.value.as < ASTSelectStmt* > () = new ASTSelectStmt();
    yylhs.value.as < ASTSelectStmt* > ()->Cond = std::shared_ptr<ASTCond>(yystack_[2].value.as < ASTCond* > ());
    yylhs.value.as < ASTSelectStmt* > ()->IfStmt = std::shared_ptr<ASTStmt>(yystack_[0].value.as < std::shared_ptr<ASTStmt> > ());
    yylhs.value.as < ASTSelectStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2122 "sysY_parser.cc"
    break;

  case 59:
#line 394 "sysY_parser.yy"
                                                {
    yylhs.value.as < ASTSelectStmt* > () = new ASTSelectStmt();
    yylhs.value.as < ASTSelectStmt* > ()->Cond = std::shared_ptr<ASTCond>(yystack_[4].value.as < ASTCond* > ());
    yylhs.value.as < ASTSelectStmt* > ()->IfStmt = std::shared_ptr<ASTStmt>(yystack_[2].value.as < std::shared_ptr<ASTStmt> > ());
    yylhs.value.as < ASTSelectStmt* > ()->ElseStmt = std::shared_ptr<ASTStmt>(yystack_[0].value.as < std::shared_ptr<ASTStmt> > ());
    yylhs.value.as < ASTSelectStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2134 "sysY_parser.cc"
    break;

  case 60:
#line 402 "sysY_parser.yy"
                                                     {
    yylhs.value.as < ASTIterationStmt* > () = new ASTIterationStmt();
    yylhs.value.as < ASTIterationStmt* > ()->Cond = std::shared_ptr<ASTCond>(yystack_[2].value.as < ASTCond* > ());
    yylhs.value.as < ASTIterationStmt* > ()->Stmt = std::shared_ptr<ASTStmt>(yystack_[0].value.as < std::shared_ptr<ASTStmt> > ());
    yylhs.value.as < ASTIterationStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2145 "sysY_parser.cc"
    break;

  case 61:
#line 409 "sysY_parser.yy"
                         {
    yylhs.value.as < ASTBreakStmt* > () = new ASTBreakStmt();
    yylhs.value.as < ASTBreakStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2154 "sysY_parser.cc"
    break;

  case 62:
#line 414 "sysY_parser.yy"
                               {
    yylhs.value.as < ASTContinueStmt* > () = new ASTContinueStmt();
    yylhs.value.as < ASTContinueStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2163 "sysY_parser.cc"
    break;

  case 63:
#line 419 "sysY_parser.yy"
                               {
    yylhs.value.as < ASTReturnStmt* > () = new ASTReturnStmt();
    yylhs.value.as < ASTReturnStmt* > ()->Exp = std::shared_ptr<ASTExp>(yystack_[1].value.as < ASTExp* > ());
    yylhs.value.as < ASTReturnStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2173 "sysY_parser.cc"
    break;

  case 64:
#line 424 "sysY_parser.yy"
                  {
    yylhs.value.as < ASTReturnStmt* > () = new ASTReturnStmt();
    yylhs.value.as < ASTReturnStmt* > ()->Exp = nullptr;
    yylhs.value.as < ASTReturnStmt* > ()->lineno = (yylhs.location).begin.line;
}
#line 2183 "sysY_parser.cc"
    break;

  case 65:
#line 430 "sysY_parser.yy"
          {
    yylhs.value.as < ASTExp* > () = new ASTExp();
    yylhs.value.as < ASTExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2193 "sysY_parser.cc"
    break;

  case 66:
#line 436 "sysY_parser.yy"
           {
    yylhs.value.as < ASTCond* > ()=new ASTCond();
    yylhs.value.as < ASTCond* > ()->LOrExp = std::shared_ptr<ASTLOrExp>(yystack_[0].value.as < ASTLOrExp* > ());
    yylhs.value.as < ASTCond* > ()->lineno = (yylhs.location).begin.line;
}
#line 2203 "sysY_parser.cc"
    break;

  case 67:
#line 442 "sysY_parser.yy"
                            {
    yylhs.value.as < ASTLVal* > () = new ASTLVal();
    yylhs.value.as < ASTLVal* > ()->id = yystack_[1].value.as < std::string > ();
    yylhs.value.as < ASTLVal* > ()->ArrayExp_list.swap(yystack_[0].value.as < ASTArrayExpList* > ()->ArrayExp_list);
    yylhs.value.as < ASTLVal* > ()->lineno = (yylhs.location).begin.line;
}
#line 2214 "sysY_parser.cc"
    break;

  case 68:
#line 449 "sysY_parser.yy"
                                               {
    yystack_[3].value.as < ASTArrayExpList* > ()->ArrayExp_list.push_back(std::shared_ptr<ASTExp>(yystack_[1].value.as < ASTExp* > ()));
    yylhs.value.as < ASTArrayExpList* > () = yystack_[3].value.as < ASTArrayExpList* > ();
}
#line 2223 "sysY_parser.cc"
    break;

  case 69:
#line 453 "sysY_parser.yy"
         {
    yylhs.value.as < ASTArrayExpList* > () = new ASTArrayExpList();
}
#line 2231 "sysY_parser.cc"
    break;

  case 70:
#line 457 "sysY_parser.yy"
                                      {
    yylhs.value.as < ASTPrimaryExp* > () = new ASTPrimaryExp();
    yylhs.value.as < ASTPrimaryExp* > ()->Exp = std::shared_ptr<ASTExp>(yystack_[1].value.as < ASTExp* > ());
    yylhs.value.as < ASTPrimaryExp* > ()->LVal = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->Number = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2243 "sysY_parser.cc"
    break;

  case 71:
#line 464 "sysY_parser.yy"
      {
    yylhs.value.as < ASTPrimaryExp* > () = new ASTPrimaryExp();
    yylhs.value.as < ASTPrimaryExp* > ()->Exp = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->LVal = std::shared_ptr<ASTLVal>(yystack_[0].value.as < ASTLVal* > ());
    yylhs.value.as < ASTPrimaryExp* > ()->Number = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2255 "sysY_parser.cc"
    break;

  case 72:
#line 471 "sysY_parser.yy"
        {
    yylhs.value.as < ASTPrimaryExp* > () = new ASTPrimaryExp();
    yylhs.value.as < ASTPrimaryExp* > ()->Exp = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->LVal = nullptr;
    yylhs.value.as < ASTPrimaryExp* > ()->Number = std::shared_ptr<ASTNumber>(yystack_[0].value.as < ASTNumber* > ());
    yylhs.value.as < ASTPrimaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2267 "sysY_parser.cc"
    break;

  case 73:
#line 479 "sysY_parser.yy"
               {
    yylhs.value.as < ASTNumber* > () = new ASTNumber();
    yylhs.value.as < ASTNumber* > ()->type = ASTType::TYPE_INT;
    yylhs.value.as < ASTNumber* > ()->i_val = yystack_[0].value.as < int > ();
    yylhs.value.as < ASTNumber* > ()->lineno = (yylhs.location).begin.line;
}
#line 2278 "sysY_parser.cc"
    break;

  case 74:
#line 485 "sysY_parser.yy"
            {
    yylhs.value.as < ASTNumber* > () = new ASTNumber();
    yylhs.value.as < ASTNumber* > ()->type = ASTType::TYPE_FLOAT;
    yylhs.value.as < ASTNumber* > ()->f_val = yystack_[0].value.as < float > ();
    yylhs.value.as < ASTNumber* > ()->lineno = (yylhs.location).begin.line;
}
#line 2289 "sysY_parser.cc"
    break;

  case 75:
#line 492 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTUnaryExp* > () = new ASTUnaryExp();
    yylhs.value.as < ASTUnaryExp* > ()->op = OP_POS;                            
    yylhs.value.as < ASTUnaryExp* > ()->PrimaryExp = std::shared_ptr<ASTPrimaryExp>(yystack_[0].value.as < ASTPrimaryExp* > ());
    yylhs.value.as < ASTUnaryExp* > ()->Callee = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->UnaryExp = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2302 "sysY_parser.cc"
    break;

  case 76:
#line 500 "sysY_parser.yy"
        {
    yylhs.value.as < ASTUnaryExp* > () = new ASTUnaryExp();
    yylhs.value.as < ASTUnaryExp* > ()->op = OP_POS;                            
    yylhs.value.as < ASTUnaryExp* > ()->PrimaryExp = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->Callee = std::shared_ptr<ASTCallee>(yystack_[0].value.as < ASTCallee* > ());
    yylhs.value.as < ASTUnaryExp* > ()->UnaryExp = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2315 "sysY_parser.cc"
    break;

  case 77:
#line 508 "sysY_parser.yy"
                  {
    yylhs.value.as < ASTUnaryExp* > () = new ASTUnaryExp();
    yylhs.value.as < ASTUnaryExp* > ()->op = yystack_[1].value.as < UnaryOp > ();                            
    yylhs.value.as < ASTUnaryExp* > ()->PrimaryExp = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->Callee = nullptr;
    yylhs.value.as < ASTUnaryExp* > ()->UnaryExp = std::shared_ptr<ASTUnaryExp>(yystack_[0].value.as < ASTUnaryExp* > ());
    yylhs.value.as < ASTUnaryExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2328 "sysY_parser.cc"
    break;

  case 78:
#line 517 "sysY_parser.yy"
                                                  {
    yylhs.value.as < ASTCallee* > () = new ASTCallee();
    yylhs.value.as < ASTCallee* > ()->ParamExp_list.swap(yystack_[1].value.as < ASTParamExpList* > ()->ParamExp_list);
    yylhs.value.as < ASTCallee* > ()->id = yystack_[3].value.as < std::string > ();
    yylhs.value.as < ASTCallee* > ()->lineno = (yylhs.location).begin.line;
}
#line 2339 "sysY_parser.cc"
    break;

  case 79:
#line 523 "sysY_parser.yy"
                                    {
    yylhs.value.as < ASTCallee* > () = new ASTCallee();
    yylhs.value.as < ASTCallee* > ()->id = yystack_[2].value.as < std::string > ();
    yylhs.value.as < ASTCallee* > ()->lineno = (yylhs.location).begin.line;
}
#line 2349 "sysY_parser.cc"
    break;

  case 80:
#line 529 "sysY_parser.yy"
           {yylhs.value.as < UnaryOp > () = OP_POS;}
#line 2355 "sysY_parser.cc"
    break;

  case 81:
#line 530 "sysY_parser.yy"
     {yylhs.value.as < UnaryOp > () = OP_NEG;}
#line 2361 "sysY_parser.cc"
    break;

  case 82:
#line 531 "sysY_parser.yy"
     {yylhs.value.as < UnaryOp > () = OP_NOT;}
#line 2367 "sysY_parser.cc"
    break;

  case 83:
#line 533 "sysY_parser.yy"
                         {
    yystack_[2].value.as < ASTParamExpList* > ()->ParamExp_list.push_back(std::shared_ptr<ASTExp>(yystack_[0].value.as < ASTExp* > ()));
    yylhs.value.as < ASTParamExpList* > () = yystack_[2].value.as < ASTParamExpList* > ();
}
#line 2376 "sysY_parser.cc"
    break;

  case 84:
#line 537 "sysY_parser.yy"
    {
    yylhs.value.as < ASTParamExpList* > () = new ASTParamExpList();
    yylhs.value.as < ASTParamExpList* > ()->ParamExp_list.push_back(std::shared_ptr<ASTExp>(yystack_[0].value.as < ASTExp* > ()));
}
#line 2385 "sysY_parser.cc"
    break;

  case 85:
#line 542 "sysY_parser.yy"
               {
    yylhs.value.as < ASTMulExp* > () = new ASTMulExp();
    yylhs.value.as < ASTMulExp* > ()->MulExp = nullptr;
    yylhs.value.as < ASTMulExp* > ()->UnaryExp = std::shared_ptr<ASTUnaryExp>(yystack_[0].value.as < ASTUnaryExp* > ());
    yylhs.value.as < ASTMulExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2396 "sysY_parser.cc"
    break;

  case 86:
#line 548 "sysY_parser.yy"
                     {
    yylhs.value.as < ASTMulExp* > () = new ASTMulExp();
    yylhs.value.as < ASTMulExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[2].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTMulExp* > ()->UnaryExp = std::shared_ptr<ASTUnaryExp>(yystack_[0].value.as < ASTUnaryExp* > ());
    yylhs.value.as < ASTMulExp* > ()->op = OP_MUL;
    yylhs.value.as < ASTMulExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2408 "sysY_parser.cc"
    break;

  case 87:
#line 555 "sysY_parser.yy"
                     {
    yylhs.value.as < ASTMulExp* > () = new ASTMulExp();
    yylhs.value.as < ASTMulExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[2].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTMulExp* > ()->UnaryExp = std::shared_ptr<ASTUnaryExp>(yystack_[0].value.as < ASTUnaryExp* > ());
    yylhs.value.as < ASTMulExp* > ()->op = OP_DIV;
    yylhs.value.as < ASTMulExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2420 "sysY_parser.cc"
    break;

  case 88:
#line 562 "sysY_parser.yy"
                     {
    yylhs.value.as < ASTMulExp* > () = new ASTMulExp();
    yylhs.value.as < ASTMulExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[2].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTMulExp* > ()->UnaryExp = std::shared_ptr<ASTUnaryExp>(yystack_[0].value.as < ASTUnaryExp* > ());
    yylhs.value.as < ASTMulExp* > ()->op = OP_MOD;
    yylhs.value.as < ASTMulExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2432 "sysY_parser.cc"
    break;

  case 89:
#line 570 "sysY_parser.yy"
             {
    yylhs.value.as < ASTAddExp* > () = new ASTAddExp();
    yylhs.value.as < ASTAddExp* > ()->AddExp = nullptr;
    yylhs.value.as < ASTAddExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[0].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTAddExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2443 "sysY_parser.cc"
    break;

  case 90:
#line 576 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTAddExp* > () = new ASTAddExp();
    yylhs.value.as < ASTAddExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[2].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTAddExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[0].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTAddExp* > ()->op = OP_PLUS;
    yylhs.value.as < ASTAddExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2455 "sysY_parser.cc"
    break;

  case 91:
#line 583 "sysY_parser.yy"
                   {
     yylhs.value.as < ASTAddExp* > () = new ASTAddExp();
    yylhs.value.as < ASTAddExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[2].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTAddExp* > ()->MulExp = std::shared_ptr<ASTMulExp>(yystack_[0].value.as < ASTMulExp* > ());
    yylhs.value.as < ASTAddExp* > ()->op = OP_MINUS;
    yylhs.value.as < ASTAddExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2467 "sysY_parser.cc"
    break;

  case 92:
#line 591 "sysY_parser.yy"
             {
    yylhs.value.as < ASTRelExp* > () = new ASTRelExp();
    yylhs.value.as < ASTRelExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTRelExp* > ()->RelExp = nullptr;
    yylhs.value.as < ASTRelExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2478 "sysY_parser.cc"
    break;

  case 93:
#line 597 "sysY_parser.yy"
                  {
    yylhs.value.as < ASTRelExp* > () = new ASTRelExp();
    yylhs.value.as < ASTRelExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTRelExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[2].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTRelExp* > ()->op = OP_LT;
    yylhs.value.as < ASTRelExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2490 "sysY_parser.cc"
    break;

  case 94:
#line 604 "sysY_parser.yy"
                  {
    yylhs.value.as < ASTRelExp* > () = new ASTRelExp();
    yylhs.value.as < ASTRelExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTRelExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[2].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTRelExp* > ()->op = OP_GT;
    yylhs.value.as < ASTRelExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2502 "sysY_parser.cc"
    break;

  case 95:
#line 611 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTRelExp* > () = new ASTRelExp();
    yylhs.value.as < ASTRelExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTRelExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[2].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTRelExp* > ()->op = OP_LTE;
    yylhs.value.as < ASTRelExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2514 "sysY_parser.cc"
    break;

  case 96:
#line 618 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTRelExp* > () = new ASTRelExp();
    yylhs.value.as < ASTRelExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTRelExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[2].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTRelExp* > ()->op = OP_GTE;
    yylhs.value.as < ASTRelExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2526 "sysY_parser.cc"
    break;

  case 97:
#line 626 "sysY_parser.yy"
            {
    yylhs.value.as < ASTEqExp* > () = new ASTEqExp();
    yylhs.value.as < ASTEqExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[0].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTEqExp* > ()->EqExp = nullptr;
    yylhs.value.as < ASTEqExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2537 "sysY_parser.cc"
    break;

  case 98:
#line 632 "sysY_parser.yy"
                 {
    yylhs.value.as < ASTEqExp* > () = new ASTEqExp();
    yylhs.value.as < ASTEqExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[0].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTEqExp* > ()->EqExp = std::shared_ptr<ASTEqExp>(yystack_[2].value.as < ASTEqExp* > ());
    yylhs.value.as < ASTEqExp* > ()->op = OP_EQ;
    yylhs.value.as < ASTEqExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2549 "sysY_parser.cc"
    break;

  case 99:
#line 639 "sysY_parser.yy"
                  {
     yylhs.value.as < ASTEqExp* > () = new ASTEqExp();
    yylhs.value.as < ASTEqExp* > ()->RelExp = std::shared_ptr<ASTRelExp>(yystack_[0].value.as < ASTRelExp* > ());
    yylhs.value.as < ASTEqExp* > ()->EqExp = std::shared_ptr<ASTEqExp>(yystack_[2].value.as < ASTEqExp* > ());
    yylhs.value.as < ASTEqExp* > ()->op = OP_NEQ;
    yylhs.value.as < ASTEqExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2561 "sysY_parser.cc"
    break;

  case 100:
#line 647 "sysY_parser.yy"
             {
    yylhs.value.as < ASTLAndExp* > () = new ASTLAndExp();
    yylhs.value.as < ASTLAndExp* > ()->LAndExp = nullptr;
    yylhs.value.as < ASTLAndExp* > ()->EqExp = std::shared_ptr<ASTEqExp>(yystack_[0].value.as < ASTEqExp* > ());
    yylhs.value.as < ASTLAndExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2572 "sysY_parser.cc"
    break;

  case 101:
#line 653 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTLAndExp* > () = new ASTLAndExp();
    yylhs.value.as < ASTLAndExp* > ()->LAndExp = std::shared_ptr<ASTLAndExp>(yystack_[2].value.as < ASTLAndExp* > ());
    yylhs.value.as < ASTLAndExp* > ()->EqExp = std::shared_ptr<ASTEqExp>(yystack_[0].value.as < ASTEqExp* > ());
    yylhs.value.as < ASTLAndExp* > ()->op = OP_AND;
    yylhs.value.as < ASTLAndExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2584 "sysY_parser.cc"
    break;

  case 102:
#line 661 "sysY_parser.yy"
              {
    yylhs.value.as < ASTLOrExp* > () = new ASTLOrExp();
    yylhs.value.as < ASTLOrExp* > ()->LOrExp = nullptr;
    yylhs.value.as < ASTLOrExp* > ()->LAndExp = std::shared_ptr<ASTLAndExp>(yystack_[0].value.as < ASTLAndExp* > ());
    yylhs.value.as < ASTLOrExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2595 "sysY_parser.cc"
    break;

  case 103:
#line 667 "sysY_parser.yy"
                   {
    yylhs.value.as < ASTLOrExp* > () = new ASTLOrExp();
    yylhs.value.as < ASTLOrExp* > ()->LOrExp = std::shared_ptr<ASTLOrExp>(yystack_[2].value.as < ASTLOrExp* > ());
    yylhs.value.as < ASTLOrExp* > ()->LAndExp = std::shared_ptr<ASTLAndExp>(yystack_[0].value.as < ASTLAndExp* > ());
    yylhs.value.as < ASTLOrExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2606 "sysY_parser.cc"
    break;

  case 104:
#line 674 "sysY_parser.yy"
               {
    yylhs.value.as < ASTConstExp* > () = new ASTConstExp();
    yylhs.value.as < ASTConstExp* > ()->AddExp = std::shared_ptr<ASTAddExp>(yystack_[0].value.as < ASTAddExp* > ());
    yylhs.value.as < ASTConstExp* > ()->lineno = (yylhs.location).begin.line;
}
#line 2616 "sysY_parser.cc"
    break;


#line 2620 "sysY_parser.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[+yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yy_error_token_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yy_error_token_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  sysY_parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  sysY_parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    std::ptrdiff_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */
    if (!yyla.empty ())
      {
        symbol_number_type yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];

        int yyn = yypact_[+yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yy_error_token_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const short sysY_parser::yypact_ninf_ = -141;

  const signed char sysY_parser::yytable_ninf_ = -1;

  const short
  sysY_parser::yypact_[] =
  {
      -1,  -141,  -141,  -141,   145,     9,   103,  -141,  -141,   -17,
    -141,  -141,    23,  -141,  -141,  -141,    66,    29,  -141,  -141,
      85,  -141,    -7,    -2,  -141,    47,    59,  -141,    23,    70,
      53,    45,  -141,   122,   151,  -141,  -141,   136,  -141,  -141,
    -141,    74,   145,    70,  -141,  -141,   151,    54,  -141,    99,
    -141,  -141,  -141,  -141,  -141,  -141,  -141,  -141,  -141,   151,
       0,    76,    76,   113,    65,  -141,  -141,    11,  -141,   123,
    -141,  -141,   129,  -141,  -141,    40,   144,   135,  -141,   151,
     151,   151,   151,   151,  -141,  -141,  -141,    44,  -141,   146,
     154,   150,   160,   161,  -141,    47,  -141,  -141,  -141,  -141,
    -141,   162,  -141,  -141,  -141,  -141,  -141,  -141,  -141,   167,
     151,  -141,   122,  -141,  -141,  -141,    56,   151,  -141,  -141,
    -141,     0,     0,   136,  -141,   151,  -141,   168,   151,  -141,
    -141,  -141,   151,   164,  -141,   151,  -141,   165,  -141,   172,
      76,   191,    98,   152,   163,  -141,   186,   190,  -141,  -141,
    -141,   108,   151,   151,   151,   151,   151,   151,   151,   151,
     108,  -141,   183,    76,    76,    76,    76,   191,   191,    98,
     152,  -141,   108,  -141
  };

  const signed char
  sysY_parser::yydefact_[] =
  {
       0,     9,    11,    10,     0,     0,     0,     4,     5,     0,
       6,     7,     0,     1,     2,     3,    16,     0,    24,    16,
       0,    13,     0,    25,    22,     0,     0,     8,     0,     0,
       0,     0,    35,     0,     0,    16,    23,     0,    12,    42,
      32,    37,     0,     0,    80,    81,     0,     0,    82,    69,
      74,    73,    26,    27,    71,    75,    72,    85,    76,     0,
      89,    65,   104,     0,     0,    14,    17,    56,    39,    36,
      34,    33,     0,    28,    31,     0,     0,    67,    77,     0,
       0,     0,     0,     0,    15,    19,    21,     0,    40,     0,
       0,     0,     0,     0,    43,     0,    44,    57,    41,    45,
      46,     0,    48,    49,    50,    51,    52,    53,    55,    71,
       0,    70,     0,    29,    79,    84,     0,     0,    86,    87,
      88,    90,    91,     0,    18,     0,    64,     0,     0,    61,
      62,    47,     0,     0,    30,     0,    78,     0,    20,     0,
      92,    97,   100,   102,    66,    63,     0,     0,    38,    83,
      68,    56,     0,     0,     0,     0,     0,     0,     0,     0,
      56,    54,    58,    93,    95,    94,    96,    98,    99,   101,
     103,    60,    56,    59
  };

  const short
  sysY_parser::yypgoto_[] =
  {
    -141,  -141,  -141,   202,   142,     1,  -141,   182,   192,   -58,
    -141,   147,  -141,   187,   -43,  -141,  -141,  -141,   171,  -141,
     -19,  -141,  -141,  -140,  -141,  -141,  -141,  -141,  -141,  -141,
    -141,  -141,   -32,    87,   -65,  -141,  -141,  -141,   -28,  -141,
    -141,  -141,    33,   -34,   -20,    58,    60,  -141,   184
  };

  const short
  sysY_parser::yydefgoto_[] =
  {
      -1,     5,     6,     7,     8,     9,    20,    21,    23,    65,
      87,    10,    17,    18,    52,    75,    11,    31,    32,    69,
      97,    67,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   139,    54,    77,    55,    56,    57,    58,
      59,   116,    60,    61,   141,   142,   143,   144,    66
  };

  const unsigned char
  sysY_parser::yytable_[] =
  {
      62,    53,   109,    62,    74,    12,    86,    79,    80,    13,
      40,   162,    29,    33,    72,    53,    44,    45,    34,     1,
     171,     2,     3,    30,    71,     1,    16,     2,     3,    46,
      62,    78,   173,    39,    88,     4,    89,     1,    90,     2,
       3,    91,    81,    30,   115,    24,    25,     4,    92,    93,
      48,   118,   119,   120,    49,    50,    51,   112,   127,    44,
      45,   123,    42,   113,    43,   138,    19,   124,    95,   134,
      44,    45,    46,   135,    37,   136,    47,    73,   133,    34,
      53,    82,    83,    46,    22,   137,   109,    64,    85,    62,
      35,   140,    39,    48,   140,   109,    41,    49,    50,    51,
     147,    27,    28,   149,    48,    68,    14,   109,    49,    50,
      51,   156,   157,    44,    45,   121,   122,    76,   163,   164,
     165,   166,   140,   140,   140,   140,    46,    44,    45,     1,
      39,     2,     3,    89,    84,    90,   167,   168,    91,     4,
      46,    44,    45,   110,    47,    92,    93,    48,   111,    44,
      45,    49,    50,    51,    46,   117,    44,    45,    64,    44,
      45,    48,    46,   114,   125,    49,    50,    51,   128,    46,
     126,     1,    46,     2,     3,    48,   129,   130,   131,    49,
      50,    51,   132,    48,   145,   148,   150,    49,    50,    51,
      48,   151,   158,    48,    49,    50,    51,    49,    50,    51,
     152,   153,   154,   155,   159,   160,   161,   172,    15,    94,
      38,    26,    36,    70,    96,   146,   169,     0,    63,   170
  };

  const short
  sysY_parser::yycheck_[] =
  {
      34,    33,    67,    37,    47,     4,    64,     7,     8,     0,
      29,   151,    19,    15,    46,    47,     5,     6,    20,    26,
     160,    28,    29,    22,    43,    26,    43,    28,    29,    18,
      64,    59,   172,    22,    23,    36,    25,    26,    27,    28,
      29,    30,    42,    42,    76,    16,    17,    36,    37,    38,
      39,    79,    80,    81,    43,    44,    45,    17,    90,     5,
       6,    17,    17,    23,    19,   123,    43,    23,    67,   112,
       5,     6,    18,    17,    15,    19,    22,    23,   110,    20,
     112,     5,     6,    18,    18,   117,   151,    22,    23,   123,
      43,   125,    22,    39,   128,   160,    43,    43,    44,    45,
     132,    16,    17,   135,    39,    31,     3,   172,    43,    44,
      45,    13,    14,     5,     6,    82,    83,    18,   152,   153,
     154,   155,   156,   157,   158,   159,    18,     5,     6,    26,
      22,    28,    29,    25,    21,    27,   156,   157,    30,    36,
      18,     5,     6,    20,    22,    37,    38,    39,    19,     5,
       6,    43,    44,    45,    18,    20,     5,     6,    22,     5,
       6,    39,    18,    19,    18,    43,    44,    45,    18,    18,
      16,    26,    18,    28,    29,    39,    16,    16,    16,    43,
      44,    45,    15,    39,    16,    21,    21,    43,    44,    45,
      39,    19,    40,    39,    43,    44,    45,    43,    44,    45,
       9,    10,    11,    12,    41,    19,    16,    24,     6,    67,
      28,    19,    25,    42,    67,   128,   158,    -1,    34,   159
  };

  const signed char
  sysY_parser::yystos_[] =
  {
       0,    26,    28,    29,    36,    48,    49,    50,    51,    52,
      58,    63,    52,     0,     3,    50,    43,    59,    60,    43,
      53,    54,    18,    55,    16,    17,    55,    16,    17,    19,
      52,    64,    65,    15,    20,    43,    60,    15,    54,    22,
      67,    43,    17,    19,     5,     6,    18,    22,    39,    43,
      44,    45,    61,    79,    81,    83,    84,    85,    86,    87,
      89,    90,    90,    95,    22,    56,    95,    68,    31,    66,
      65,    67,    79,    23,    61,    62,    18,    82,    85,     7,
       8,    42,     5,     6,    21,    23,    56,    57,    23,    25,
      27,    30,    37,    38,    51,    52,    58,    67,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    81,
      20,    19,    17,    23,    19,    79,    88,    20,    85,    85,
      85,    89,    89,    17,    23,    18,    16,    79,    18,    16,
      16,    16,    15,    79,    61,    17,    19,    79,    56,    80,
      90,    91,    92,    93,    94,    16,    80,    79,    21,    79,
      21,    19,     9,    10,    11,    12,    13,    14,    40,    41,
      19,    16,    70,    90,    90,    90,    90,    91,    91,    92,
      93,    70,    24,    70
  };

  const signed char
  sysY_parser::yyr1_[] =
  {
       0,    47,    48,    49,    49,    50,    50,    50,    51,    52,
      52,    52,    53,    53,    54,    55,    55,    56,    56,    56,
      57,    57,    58,    59,    59,    60,    60,    61,    61,    61,
      62,    62,    63,    63,    64,    64,    65,    65,    66,    66,
      67,    68,    68,    69,    69,    69,    70,    70,    70,    70,
      70,    70,    70,    70,    71,    72,    72,    73,    74,    74,
      75,    76,    77,    78,    78,    79,    80,    81,    82,    82,
      83,    83,    83,    84,    84,    85,    85,    85,    86,    86,
      87,    87,    87,    88,    88,    89,    89,    89,    89,    90,
      90,    90,    91,    91,    91,    91,    91,    92,    92,    92,
      93,    93,    94,    94,    95
  };

  const signed char
  sysY_parser::yyr2_[] =
  {
       0,     2,     2,     2,     1,     1,     1,     1,     4,     1,
       1,     1,     3,     1,     4,     4,     0,     1,     3,     2,
       3,     1,     3,     3,     1,     2,     4,     1,     2,     3,
       3,     1,     5,     6,     3,     1,     3,     2,     4,     1,
       3,     2,     0,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     4,     1,     0,     1,     5,     7,
       5,     2,     2,     3,     2,     1,     1,     2,     4,     0,
       3,     1,     1,     1,     1,     1,     1,     2,     4,     3,
       1,     1,     1,     3,     1,     1,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     3,     1,     3,     1
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const sysY_parser::yytname_[] =
  {
  "$end", "error", "$undefined", "END", "ERROR", "ADD", "SUB", "MUL",
  "DIV", "LT", "LTE", "GT", "GTE", "EQ", "NEQ", "ASSIN", "SEMICOLON",
  "COMMA", "LPARENTHESE", "RPARENTHESE", "LBRACKET", "RBRACKET", "LBRACE",
  "RBRACE", "ELSE", "IF", "INT", "RETURN", "VOID", "FLOAT", "WHILE",
  "ARRAY", "LETTER", "EOL", "COMMENT", "BLANK", "CONST", "BREAK",
  "CONTINUE", "NOT", "AND", "OR", "MOD", "IDENTIFIER", "FLOATCONST",
  "INTCONST", "STRINGCONST", "$accept", "Begin", "CompUnit", "DeclDef",
  "ConstDecl", "BType", "ConstDefList", "ConstDef", "ArrayConstExpList",
  "ConstInitVal", "ConstInitValList", "VarDecl", "VarDefList", "VarDef",
  "InitVal", "InitValList", "FuncDef", "FuncFParamList", "FuncFParam",
  "ParamArrayExpList", "Block", "BlockItemList", "BlockItem", "Stmt",
  "AssignStmt", "ExpStmt", "BlockStmt", "SelectStmt", "IterationStmt",
  "BreakStmt", "ContinueStmt", "ReturnStmt", "Exp", "Cond", "LVal",
  "ArrayExpList", "PrimaryExp", "Number", "UnaryExp", "Callee", "UnaryOp",
  "ExpList", "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp",
  "ConstExp", YY_NULLPTR
  };

#if YYDEBUG
  const short
  sysY_parser::yyrline_[] =
  {
       0,   107,   107,   114,   118,   124,   127,   130,   134,   141,
     142,   143,   145,   149,   154,   162,   166,   170,   175,   181,
     187,   191,   196,   203,   207,   212,   219,   227,   232,   237,
     244,   248,   253,   260,   269,   273,   278,   286,   294,   298,
     303,   309,   313,   317,   324,   331,   339,   342,   345,   348,
     351,   354,   357,   360,   364,   371,   376,   382,   388,   394,
     402,   409,   414,   419,   424,   430,   436,   442,   449,   453,
     457,   464,   471,   479,   485,   492,   500,   508,   517,   523,
     529,   530,   531,   533,   537,   542,   548,   555,   562,   570,
     576,   583,   591,   597,   604,   611,   618,   626,   632,   639,
     647,   653,   661,   667,   674
  };

  // Print the state stack on the debug stream.
  void
  sysY_parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  sysY_parser::yy_reduce_print_ (int yyrule)
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


} // yy
#line 3143 "sysY_parser.cc"

#line 680 "sysY_parser.yy"


// Register errors to the driver
void yy::sysY_parser::error (const location_type& l,const std::string& m) {
    driver.error(l, m);
}







