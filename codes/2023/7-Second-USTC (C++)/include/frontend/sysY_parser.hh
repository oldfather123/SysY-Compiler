// A Bison parser, made by GNU Bison 3.5.1.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file sysY_parser.hh
 ** Define the yy::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.

#ifndef YY_YY_SYSY_PARSER_HH_INCLUDED
# define YY_YY_SYSY_PARSER_HH_INCLUDED
// "%code requires" blocks.
#line 12 "sysY_parser.yy"

#include <string>
#include "sysY_ast.hh"
class sysY_driver;

#line 54 "sysY_parser.hh"

# include <cassert>
# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "location.hh"
#include <typeinfo>
#ifndef YY_ASSERT
# include <cassert>
# define YY_ASSERT assert
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

namespace yy {
#line 188 "sysY_parser.hh"




  /// A Bison parser.
  class sysY_parser
  {
  public:
#ifndef YYSTYPE
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class semantic_type
  {
  public:
    /// Type of *this.
    typedef semantic_type self_type;

    /// Empty construction.
    semantic_type () YY_NOEXCEPT
      : yybuffer_ ()
      , yytypeid_ (YY_NULLPTR)
    {}

    /// Construct and fill.
    template <typename T>
    semantic_type (YY_RVREF (T) t)
      : yytypeid_ (&typeid (T))
    {
      YY_ASSERT (sizeof (T) <= size);
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

    /// Destruction, allowed only if empty.
    ~semantic_type () YY_NOEXCEPT
    {
      YY_ASSERT (!yytypeid_);
    }

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == *that.yytypeid_);
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
      yytypeid_ = YY_NULLPTR;
    }

  private:
    /// Prohibit blind copies.
    self_type& operator= (const self_type&);
    semantic_type (const self_type&);

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yybuffer_.yyraw;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yybuffer_.yyraw;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // AddExp
      char dummy1[sizeof (ASTAddExp*)];

      // ArrayConstExpList
      char dummy2[sizeof (ASTArrayConstExpList*)];

      // ArrayExpList
      char dummy3[sizeof (ASTArrayExpList*)];

      // AssignStmt
      char dummy4[sizeof (ASTAssignStmt*)];

      // Block
      char dummy5[sizeof (ASTBlock*)];

      // BlockItem
      char dummy6[sizeof (ASTBlockItem*)];

      // BlockItemList
      char dummy7[sizeof (ASTBlockItemList*)];

      // BlockStmt
      char dummy8[sizeof (ASTBlockStmt*)];

      // BreakStmt
      char dummy9[sizeof (ASTBreakStmt*)];

      // Callee
      char dummy10[sizeof (ASTCallee*)];

      // CompUnit
      char dummy11[sizeof (ASTCompUnit*)];

      // Cond
      char dummy12[sizeof (ASTCond*)];

      // ConstDecl
      char dummy13[sizeof (ASTConstDeclaration*)];

      // ConstDef
      char dummy14[sizeof (ASTConstDef*)];

      // ConstDefList
      char dummy15[sizeof (ASTConstDefList*)];

      // ConstExp
      char dummy16[sizeof (ASTConstExp*)];

      // ConstInitVal
      char dummy17[sizeof (ASTConstInitVal*)];

      // ConstInitValList
      char dummy18[sizeof (ASTConstInitValList*)];

      // ContinueStmt
      char dummy19[sizeof (ASTContinueStmt*)];

      // EqExp
      char dummy20[sizeof (ASTEqExp*)];

      // Exp
      char dummy21[sizeof (ASTExp*)];

      // ExpStmt
      char dummy22[sizeof (ASTExpStmt*)];

      // FuncDef
      char dummy23[sizeof (ASTFuncDef*)];

      // FuncFParam
      char dummy24[sizeof (ASTFuncFParam*)];

      // FuncFParamList
      char dummy25[sizeof (ASTFuncFParamList*)];

      // InitVal
      char dummy26[sizeof (ASTInitVal*)];

      // InitValList
      char dummy27[sizeof (ASTInitValList*)];

      // IterationStmt
      char dummy28[sizeof (ASTIterationStmt*)];

      // LAndExp
      char dummy29[sizeof (ASTLAndExp*)];

      // LOrExp
      char dummy30[sizeof (ASTLOrExp*)];

      // LVal
      char dummy31[sizeof (ASTLVal*)];

      // MulExp
      char dummy32[sizeof (ASTMulExp*)];

      // Number
      char dummy33[sizeof (ASTNumber*)];

      // ParamArrayExpList
      char dummy34[sizeof (ASTParamArrayExpList*)];

      // ExpList
      char dummy35[sizeof (ASTParamExpList*)];

      // PrimaryExp
      char dummy36[sizeof (ASTPrimaryExp*)];

      // RelExp
      char dummy37[sizeof (ASTRelExp*)];

      // ReturnStmt
      char dummy38[sizeof (ASTReturnStmt*)];

      // SelectStmt
      char dummy39[sizeof (ASTSelectStmt*)];

      // BType
      char dummy40[sizeof (ASTType)];

      // UnaryExp
      char dummy41[sizeof (ASTUnaryExp*)];

      // VarDecl
      char dummy42[sizeof (ASTVarDeclaration*)];

      // VarDef
      char dummy43[sizeof (ASTVarDef*)];

      // VarDefList
      char dummy44[sizeof (ASTVarDefList*)];

      // UnaryOp
      char dummy45[sizeof (UnaryOp)];

      // FLOATCONST
      char dummy46[sizeof (float)];

      // INTCONST
      char dummy47[sizeof (int)];

      // DeclDef
      char dummy48[sizeof (std::shared_ptr<ASTDeclaration>)];

      // Stmt
      char dummy49[sizeof (std::shared_ptr<ASTStmt>)];

      // IDENTIFIER
      // STRINGCONST
      char dummy50[sizeof (std::string)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me;
      /// A buffer large enough to store any of the semantic values.
      char yyraw[size];
    } yybuffer_;

    /// Whether the content is built: if defined, the name of the stored type.
    const std::type_info *yytypeid_;
  };

#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Tokens.
    struct token
    {
      enum yytokentype
      {
        TOK_END = 302,
        TOK_ERROR = 258,
        TOK_ADD = 259,
        TOK_SUB = 260,
        TOK_MUL = 261,
        TOK_DIV = 262,
        TOK_LT = 263,
        TOK_LTE = 264,
        TOK_GT = 265,
        TOK_GTE = 266,
        TOK_EQ = 267,
        TOK_NEQ = 268,
        TOK_ASSIN = 269,
        TOK_SEMICOLON = 270,
        TOK_COMMA = 271,
        TOK_LPARENTHESE = 272,
        TOK_RPARENTHESE = 273,
        TOK_LBRACKET = 274,
        TOK_RBRACKET = 275,
        TOK_LBRACE = 276,
        TOK_RBRACE = 277,
        TOK_ELSE = 278,
        TOK_IF = 279,
        TOK_INT = 280,
        TOK_RETURN = 281,
        TOK_VOID = 282,
        TOK_FLOAT = 283,
        TOK_WHILE = 284,
        TOK_ARRAY = 285,
        TOK_LETTER = 286,
        TOK_EOL = 287,
        TOK_COMMENT = 288,
        TOK_BLANK = 289,
        TOK_CONST = 290,
        TOK_BREAK = 291,
        TOK_CONTINUE = 292,
        TOK_NOT = 293,
        TOK_AND = 294,
        TOK_OR = 295,
        TOK_MOD = 296,
        TOK_IDENTIFIER = 297,
        TOK_FLOATCONST = 298,
        TOK_INTCONST = 299,
        TOK_STRINGCONST = 300
      };
    };

    /// (External) token type, as returned by yylex.
    typedef token::yytokentype token_type;

    /// Symbol type: an internal symbol number.
    typedef int symbol_number_type;

    /// The symbol type number to denote an empty symbol.
    enum { empty_symbol = -2 };

    /// Internal symbol number for tokens (subsumed by symbol_number_type).
    typedef signed char token_number_type;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol type
    /// via type_get ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ()
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that);
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTAddExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTAddExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTArrayConstExpList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTArrayConstExpList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTArrayExpList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTArrayExpList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTAssignStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTAssignStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTBlock*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTBlock*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTBlockItem*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTBlockItem*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTBlockItemList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTBlockItemList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTBlockStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTBlockStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTBreakStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTBreakStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTCallee*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTCallee*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTCompUnit*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTCompUnit*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTCond*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTCond*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstDeclaration*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstDeclaration*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstDef*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstDef*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstDefList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstDefList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstInitVal*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstInitVal*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTConstInitValList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTConstInitValList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTContinueStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTContinueStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTEqExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTEqExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTExpStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTExpStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTFuncDef*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTFuncDef*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTFuncFParam*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTFuncFParam*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTFuncFParamList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTFuncFParamList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTInitVal*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTInitVal*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTInitValList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTInitValList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTIterationStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTIterationStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTLAndExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTLAndExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTLOrExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTLOrExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTLVal*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTLVal*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTMulExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTMulExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTNumber*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTNumber*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTParamArrayExpList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTParamArrayExpList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTParamExpList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTParamExpList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTPrimaryExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTPrimaryExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTRelExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTRelExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTReturnStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTReturnStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTSelectStmt*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTSelectStmt*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTType&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTType& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTUnaryExp*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTUnaryExp*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTVarDeclaration*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTVarDeclaration*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTVarDef*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTVarDef*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ASTVarDefList*&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ASTVarDefList*& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, UnaryOp&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const UnaryOp& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, float&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const float& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<ASTDeclaration>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<ASTDeclaration>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<ASTStmt>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<ASTStmt>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }

      /// Destroy contents, and record that is empty.
      void clear ()
      {
        // User destructor.
        symbol_number_type yytype = this->type_get ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yytype)
        {
       default:
          break;
        }

        // Type destructor.
switch (yytype)
    {
      case 90: // AddExp
        value.template destroy< ASTAddExp* > ();
        break;

      case 55: // ArrayConstExpList
        value.template destroy< ASTArrayConstExpList* > ();
        break;

      case 82: // ArrayExpList
        value.template destroy< ASTArrayExpList* > ();
        break;

      case 71: // AssignStmt
        value.template destroy< ASTAssignStmt* > ();
        break;

      case 67: // Block
        value.template destroy< ASTBlock* > ();
        break;

      case 69: // BlockItem
        value.template destroy< ASTBlockItem* > ();
        break;

      case 68: // BlockItemList
        value.template destroy< ASTBlockItemList* > ();
        break;

      case 73: // BlockStmt
        value.template destroy< ASTBlockStmt* > ();
        break;

      case 76: // BreakStmt
        value.template destroy< ASTBreakStmt* > ();
        break;

      case 86: // Callee
        value.template destroy< ASTCallee* > ();
        break;

      case 49: // CompUnit
        value.template destroy< ASTCompUnit* > ();
        break;

      case 80: // Cond
        value.template destroy< ASTCond* > ();
        break;

      case 51: // ConstDecl
        value.template destroy< ASTConstDeclaration* > ();
        break;

      case 54: // ConstDef
        value.template destroy< ASTConstDef* > ();
        break;

      case 53: // ConstDefList
        value.template destroy< ASTConstDefList* > ();
        break;

      case 95: // ConstExp
        value.template destroy< ASTConstExp* > ();
        break;

      case 56: // ConstInitVal
        value.template destroy< ASTConstInitVal* > ();
        break;

      case 57: // ConstInitValList
        value.template destroy< ASTConstInitValList* > ();
        break;

      case 77: // ContinueStmt
        value.template destroy< ASTContinueStmt* > ();
        break;

      case 92: // EqExp
        value.template destroy< ASTEqExp* > ();
        break;

      case 79: // Exp
        value.template destroy< ASTExp* > ();
        break;

      case 72: // ExpStmt
        value.template destroy< ASTExpStmt* > ();
        break;

      case 63: // FuncDef
        value.template destroy< ASTFuncDef* > ();
        break;

      case 65: // FuncFParam
        value.template destroy< ASTFuncFParam* > ();
        break;

      case 64: // FuncFParamList
        value.template destroy< ASTFuncFParamList* > ();
        break;

      case 61: // InitVal
        value.template destroy< ASTInitVal* > ();
        break;

      case 62: // InitValList
        value.template destroy< ASTInitValList* > ();
        break;

      case 75: // IterationStmt
        value.template destroy< ASTIterationStmt* > ();
        break;

      case 93: // LAndExp
        value.template destroy< ASTLAndExp* > ();
        break;

      case 94: // LOrExp
        value.template destroy< ASTLOrExp* > ();
        break;

      case 81: // LVal
        value.template destroy< ASTLVal* > ();
        break;

      case 89: // MulExp
        value.template destroy< ASTMulExp* > ();
        break;

      case 84: // Number
        value.template destroy< ASTNumber* > ();
        break;

      case 66: // ParamArrayExpList
        value.template destroy< ASTParamArrayExpList* > ();
        break;

      case 88: // ExpList
        value.template destroy< ASTParamExpList* > ();
        break;

      case 83: // PrimaryExp
        value.template destroy< ASTPrimaryExp* > ();
        break;

      case 91: // RelExp
        value.template destroy< ASTRelExp* > ();
        break;

      case 78: // ReturnStmt
        value.template destroy< ASTReturnStmt* > ();
        break;

      case 74: // SelectStmt
        value.template destroy< ASTSelectStmt* > ();
        break;

      case 52: // BType
        value.template destroy< ASTType > ();
        break;

      case 85: // UnaryExp
        value.template destroy< ASTUnaryExp* > ();
        break;

      case 58: // VarDecl
        value.template destroy< ASTVarDeclaration* > ();
        break;

      case 60: // VarDef
        value.template destroy< ASTVarDef* > ();
        break;

      case 59: // VarDefList
        value.template destroy< ASTVarDefList* > ();
        break;

      case 87: // UnaryOp
        value.template destroy< UnaryOp > ();
        break;

      case 44: // FLOATCONST
        value.template destroy< float > ();
        break;

      case 45: // INTCONST
        value.template destroy< int > ();
        break;

      case 50: // DeclDef
        value.template destroy< std::shared_ptr<ASTDeclaration> > ();
        break;

      case 70: // Stmt
        value.template destroy< std::shared_ptr<ASTStmt> > ();
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.template destroy< std::string > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_type
    {
      /// Default constructor.
      by_type ();

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_type (by_type&& that);
#endif

      /// Copy constructor.
      by_type (const by_type& that);

      /// The symbol type as needed by the constructor.
      typedef token_type kind_type;

      /// Constructor from (external) token numbers.
      by_type (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol type from \a that.
      void move (by_type& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The symbol type.
      /// \a empty_symbol when empty.
      /// An int, not token_number_type, to be able to store empty_symbol.
      int type;
    };

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_type>
    {
      /// Superclass.
      typedef basic_symbol<by_type> super_type;

      /// Empty symbol.
      symbol_type () {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type(token_type (tok), std::move (l))
      {
        YY_ASSERT (tok == 0 || tok == token::TOK_END || tok == token::TOK_ERROR || tok == token::TOK_ADD || tok == token::TOK_SUB || tok == token::TOK_MUL || tok == token::TOK_DIV || tok == token::TOK_LT || tok == token::TOK_LTE || tok == token::TOK_GT || tok == token::TOK_GTE || tok == token::TOK_EQ || tok == token::TOK_NEQ || tok == token::TOK_ASSIN || tok == token::TOK_SEMICOLON || tok == token::TOK_COMMA || tok == token::TOK_LPARENTHESE || tok == token::TOK_RPARENTHESE || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_ELSE || tok == token::TOK_IF || tok == token::TOK_INT || tok == token::TOK_RETURN || tok == token::TOK_VOID || tok == token::TOK_FLOAT || tok == token::TOK_WHILE || tok == token::TOK_ARRAY || tok == token::TOK_LETTER || tok == token::TOK_EOL || tok == token::TOK_COMMENT || tok == token::TOK_BLANK || tok == token::TOK_CONST || tok == token::TOK_BREAK || tok == token::TOK_CONTINUE || tok == token::TOK_NOT || tok == token::TOK_AND || tok == token::TOK_OR || tok == token::TOK_MOD);
      }
#else
      symbol_type (int tok, const location_type& l)
        : super_type(token_type (tok), l)
      {
        YY_ASSERT (tok == 0 || tok == token::TOK_END || tok == token::TOK_ERROR || tok == token::TOK_ADD || tok == token::TOK_SUB || tok == token::TOK_MUL || tok == token::TOK_DIV || tok == token::TOK_LT || tok == token::TOK_LTE || tok == token::TOK_GT || tok == token::TOK_GTE || tok == token::TOK_EQ || tok == token::TOK_NEQ || tok == token::TOK_ASSIN || tok == token::TOK_SEMICOLON || tok == token::TOK_COMMA || tok == token::TOK_LPARENTHESE || tok == token::TOK_RPARENTHESE || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_ELSE || tok == token::TOK_IF || tok == token::TOK_INT || tok == token::TOK_RETURN || tok == token::TOK_VOID || tok == token::TOK_FLOAT || tok == token::TOK_WHILE || tok == token::TOK_ARRAY || tok == token::TOK_LETTER || tok == token::TOK_EOL || tok == token::TOK_COMMENT || tok == token::TOK_BLANK || tok == token::TOK_CONST || tok == token::TOK_BREAK || tok == token::TOK_CONTINUE || tok == token::TOK_NOT || tok == token::TOK_AND || tok == token::TOK_OR || tok == token::TOK_MOD);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, float v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_FLOATCONST);
      }
#else
      symbol_type (int tok, const float& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_FLOATCONST);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_INTCONST);
      }
#else
      symbol_type (int tok, const int& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_INTCONST);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, std::string v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_IDENTIFIER || tok == token::TOK_STRINGCONST);
      }
#else
      symbol_type (int tok, const std::string& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_IDENTIFIER || tok == token::TOK_STRINGCONST);
      }
#endif
    };

    /// Build a parser object.
    sysY_parser (sysY_driver& driver_yyarg);
    virtual ~sysY_parser ();

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    // Implementation of make_symbol for each symbol type.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::TOK_END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::TOK_END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ERROR (location_type l)
      {
        return symbol_type (token::TOK_ERROR, std::move (l));
      }
#else
      static
      symbol_type
      make_ERROR (const location_type& l)
      {
        return symbol_type (token::TOK_ERROR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ADD (location_type l)
      {
        return symbol_type (token::TOK_ADD, std::move (l));
      }
#else
      static
      symbol_type
      make_ADD (const location_type& l)
      {
        return symbol_type (token::TOK_ADD, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SUB (location_type l)
      {
        return symbol_type (token::TOK_SUB, std::move (l));
      }
#else
      static
      symbol_type
      make_SUB (const location_type& l)
      {
        return symbol_type (token::TOK_SUB, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MUL (location_type l)
      {
        return symbol_type (token::TOK_MUL, std::move (l));
      }
#else
      static
      symbol_type
      make_MUL (const location_type& l)
      {
        return symbol_type (token::TOK_MUL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DIV (location_type l)
      {
        return symbol_type (token::TOK_DIV, std::move (l));
      }
#else
      static
      symbol_type
      make_DIV (const location_type& l)
      {
        return symbol_type (token::TOK_DIV, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LT (location_type l)
      {
        return symbol_type (token::TOK_LT, std::move (l));
      }
#else
      static
      symbol_type
      make_LT (const location_type& l)
      {
        return symbol_type (token::TOK_LT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LTE (location_type l)
      {
        return symbol_type (token::TOK_LTE, std::move (l));
      }
#else
      static
      symbol_type
      make_LTE (const location_type& l)
      {
        return symbol_type (token::TOK_LTE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GT (location_type l)
      {
        return symbol_type (token::TOK_GT, std::move (l));
      }
#else
      static
      symbol_type
      make_GT (const location_type& l)
      {
        return symbol_type (token::TOK_GT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GTE (location_type l)
      {
        return symbol_type (token::TOK_GTE, std::move (l));
      }
#else
      static
      symbol_type
      make_GTE (const location_type& l)
      {
        return symbol_type (token::TOK_GTE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EQ (location_type l)
      {
        return symbol_type (token::TOK_EQ, std::move (l));
      }
#else
      static
      symbol_type
      make_EQ (const location_type& l)
      {
        return symbol_type (token::TOK_EQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NEQ (location_type l)
      {
        return symbol_type (token::TOK_NEQ, std::move (l));
      }
#else
      static
      symbol_type
      make_NEQ (const location_type& l)
      {
        return symbol_type (token::TOK_NEQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIN (location_type l)
      {
        return symbol_type (token::TOK_ASSIN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIN (const location_type& l)
      {
        return symbol_type (token::TOK_ASSIN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMICOLON (location_type l)
      {
        return symbol_type (token::TOK_SEMICOLON, std::move (l));
      }
#else
      static
      symbol_type
      make_SEMICOLON (const location_type& l)
      {
        return symbol_type (token::TOK_SEMICOLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::TOK_COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::TOK_COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPARENTHESE (location_type l)
      {
        return symbol_type (token::TOK_LPARENTHESE, std::move (l));
      }
#else
      static
      symbol_type
      make_LPARENTHESE (const location_type& l)
      {
        return symbol_type (token::TOK_LPARENTHESE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPARENTHESE (location_type l)
      {
        return symbol_type (token::TOK_RPARENTHESE, std::move (l));
      }
#else
      static
      symbol_type
      make_RPARENTHESE (const location_type& l)
      {
        return symbol_type (token::TOK_RPARENTHESE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACKET (location_type l)
      {
        return symbol_type (token::TOK_LBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACKET (location_type l)
      {
        return symbol_type (token::TOK_RBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACE (location_type l)
      {
        return symbol_type (token::TOK_LBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACE (location_type l)
      {
        return symbol_type (token::TOK_RBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ELSE (location_type l)
      {
        return symbol_type (token::TOK_ELSE, std::move (l));
      }
#else
      static
      symbol_type
      make_ELSE (const location_type& l)
      {
        return symbol_type (token::TOK_ELSE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IF (location_type l)
      {
        return symbol_type (token::TOK_IF, std::move (l));
      }
#else
      static
      symbol_type
      make_IF (const location_type& l)
      {
        return symbol_type (token::TOK_IF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT (location_type l)
      {
        return symbol_type (token::TOK_INT, std::move (l));
      }
#else
      static
      symbol_type
      make_INT (const location_type& l)
      {
        return symbol_type (token::TOK_INT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RETURN (location_type l)
      {
        return symbol_type (token::TOK_RETURN, std::move (l));
      }
#else
      static
      symbol_type
      make_RETURN (const location_type& l)
      {
        return symbol_type (token::TOK_RETURN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_VOID (location_type l)
      {
        return symbol_type (token::TOK_VOID, std::move (l));
      }
#else
      static
      symbol_type
      make_VOID (const location_type& l)
      {
        return symbol_type (token::TOK_VOID, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT (location_type l)
      {
        return symbol_type (token::TOK_FLOAT, std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT (const location_type& l)
      {
        return symbol_type (token::TOK_FLOAT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHILE (location_type l)
      {
        return symbol_type (token::TOK_WHILE, std::move (l));
      }
#else
      static
      symbol_type
      make_WHILE (const location_type& l)
      {
        return symbol_type (token::TOK_WHILE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ARRAY (location_type l)
      {
        return symbol_type (token::TOK_ARRAY, std::move (l));
      }
#else
      static
      symbol_type
      make_ARRAY (const location_type& l)
      {
        return symbol_type (token::TOK_ARRAY, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LETTER (location_type l)
      {
        return symbol_type (token::TOK_LETTER, std::move (l));
      }
#else
      static
      symbol_type
      make_LETTER (const location_type& l)
      {
        return symbol_type (token::TOK_LETTER, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EOL (location_type l)
      {
        return symbol_type (token::TOK_EOL, std::move (l));
      }
#else
      static
      symbol_type
      make_EOL (const location_type& l)
      {
        return symbol_type (token::TOK_EOL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMENT (location_type l)
      {
        return symbol_type (token::TOK_COMMENT, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMENT (const location_type& l)
      {
        return symbol_type (token::TOK_COMMENT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BLANK (location_type l)
      {
        return symbol_type (token::TOK_BLANK, std::move (l));
      }
#else
      static
      symbol_type
      make_BLANK (const location_type& l)
      {
        return symbol_type (token::TOK_BLANK, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONST (location_type l)
      {
        return symbol_type (token::TOK_CONST, std::move (l));
      }
#else
      static
      symbol_type
      make_CONST (const location_type& l)
      {
        return symbol_type (token::TOK_CONST, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BREAK (location_type l)
      {
        return symbol_type (token::TOK_BREAK, std::move (l));
      }
#else
      static
      symbol_type
      make_BREAK (const location_type& l)
      {
        return symbol_type (token::TOK_BREAK, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONTINUE (location_type l)
      {
        return symbol_type (token::TOK_CONTINUE, std::move (l));
      }
#else
      static
      symbol_type
      make_CONTINUE (const location_type& l)
      {
        return symbol_type (token::TOK_CONTINUE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (location_type l)
      {
        return symbol_type (token::TOK_NOT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const location_type& l)
      {
        return symbol_type (token::TOK_NOT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND (location_type l)
      {
        return symbol_type (token::TOK_AND, std::move (l));
      }
#else
      static
      symbol_type
      make_AND (const location_type& l)
      {
        return symbol_type (token::TOK_AND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR (location_type l)
      {
        return symbol_type (token::TOK_OR, std::move (l));
      }
#else
      static
      symbol_type
      make_OR (const location_type& l)
      {
        return symbol_type (token::TOK_OR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MOD (location_type l)
      {
        return symbol_type (token::TOK_MOD, std::move (l));
      }
#else
      static
      symbol_type
      make_MOD (const location_type& l)
      {
        return symbol_type (token::TOK_MOD, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENTIFIER (std::string v, location_type l)
      {
        return symbol_type (token::TOK_IDENTIFIER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENTIFIER (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_IDENTIFIER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOATCONST (float v, location_type l)
      {
        return symbol_type (token::TOK_FLOATCONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOATCONST (const float& v, const location_type& l)
      {
        return symbol_type (token::TOK_FLOATCONST, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTCONST (int v, location_type l)
      {
        return symbol_type (token::TOK_INTCONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INTCONST (const int& v, const location_type& l)
      {
        return symbol_type (token::TOK_INTCONST, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRINGCONST (std::string v, location_type l)
      {
        return symbol_type (token::TOK_STRINGCONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRINGCONST (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_STRINGCONST, v, l);
      }
#endif


  private:
    /// This class is not copyable.
    sysY_parser (const sysY_parser&);
    sysY_parser& operator= (const sysY_parser&);

    /// Stored state numbers (used for stacks).
    typedef unsigned char state_type;

    /// Generate an error message.
    /// \param yystate   the state where the error occurred.
    /// \param yyla      the lookahead token.
    virtual std::string yysyntax_error_ (state_type yystate,
                                         const symbol_type& yyla) const;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const short yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token number \a t to a symbol number.
    /// In theory \a t should be a token_type, but character literals
    /// are valid, yet not members of the token_type enum.
    static token_number_type yytranslate_ (int t);

    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const signed char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const unsigned char yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
    // symbol of state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
    static const signed char yyr1_[];

    // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
    static const signed char yyr2_[];


    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);


    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol type, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol type as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol type from \a that.
      void move (by_state& that);

      /// The (internal) type number (corresponding to \a state).
      /// \a empty_symbol when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::reverse_iterator iterator;
      typedef typename S::const_reverse_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200)
        : seq_ (n)
      {}

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      std::ptrdiff_t
      ssize () const YY_NOEXCEPT
      {
        return std::ptrdiff_t (size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.rbegin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.rend ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range)
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
      stack (const stack&);
      stack& operator= (const stack&);
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1);

    /// Some specific tokens.
    static const token_number_type yy_error_token_ = 1;
    static const token_number_type yy_undef_token_ = 2;

    /// Constants.
    enum
    {
      yyeof_ = 0,
      yylast_ = 219,     ///< Last index in yytable_.
      yynnts_ = 49,  ///< Number of nonterminal symbols.
      yyfinal_ = 13, ///< Termination state number.
      yyntokens_ = 47  ///< Number of tokens.
    };


    // User arguments.
    sysY_driver& driver;
  };

  inline
  sysY_parser::token_number_type
  sysY_parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const token_number_type
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,     2,     3
    };
    const int user_token_number_max_ = 302;

    if (t <= 0)
      return yyeof_;
    else if (t <= user_token_number_max_)
      return translate_table[t];
    else
      return yy_undef_token_;
  }

  // basic_symbol.
#if 201103L <= YY_CPLUSPLUS
  template <typename Base>
  sysY_parser::basic_symbol<Base>::basic_symbol (basic_symbol&& that)
    : Base (std::move (that))
    , value ()
    , location (std::move (that.location))
  {
    switch (this->type_get ())
    {
      case 90: // AddExp
        value.move< ASTAddExp* > (std::move (that.value));
        break;

      case 55: // ArrayConstExpList
        value.move< ASTArrayConstExpList* > (std::move (that.value));
        break;

      case 82: // ArrayExpList
        value.move< ASTArrayExpList* > (std::move (that.value));
        break;

      case 71: // AssignStmt
        value.move< ASTAssignStmt* > (std::move (that.value));
        break;

      case 67: // Block
        value.move< ASTBlock* > (std::move (that.value));
        break;

      case 69: // BlockItem
        value.move< ASTBlockItem* > (std::move (that.value));
        break;

      case 68: // BlockItemList
        value.move< ASTBlockItemList* > (std::move (that.value));
        break;

      case 73: // BlockStmt
        value.move< ASTBlockStmt* > (std::move (that.value));
        break;

      case 76: // BreakStmt
        value.move< ASTBreakStmt* > (std::move (that.value));
        break;

      case 86: // Callee
        value.move< ASTCallee* > (std::move (that.value));
        break;

      case 49: // CompUnit
        value.move< ASTCompUnit* > (std::move (that.value));
        break;

      case 80: // Cond
        value.move< ASTCond* > (std::move (that.value));
        break;

      case 51: // ConstDecl
        value.move< ASTConstDeclaration* > (std::move (that.value));
        break;

      case 54: // ConstDef
        value.move< ASTConstDef* > (std::move (that.value));
        break;

      case 53: // ConstDefList
        value.move< ASTConstDefList* > (std::move (that.value));
        break;

      case 95: // ConstExp
        value.move< ASTConstExp* > (std::move (that.value));
        break;

      case 56: // ConstInitVal
        value.move< ASTConstInitVal* > (std::move (that.value));
        break;

      case 57: // ConstInitValList
        value.move< ASTConstInitValList* > (std::move (that.value));
        break;

      case 77: // ContinueStmt
        value.move< ASTContinueStmt* > (std::move (that.value));
        break;

      case 92: // EqExp
        value.move< ASTEqExp* > (std::move (that.value));
        break;

      case 79: // Exp
        value.move< ASTExp* > (std::move (that.value));
        break;

      case 72: // ExpStmt
        value.move< ASTExpStmt* > (std::move (that.value));
        break;

      case 63: // FuncDef
        value.move< ASTFuncDef* > (std::move (that.value));
        break;

      case 65: // FuncFParam
        value.move< ASTFuncFParam* > (std::move (that.value));
        break;

      case 64: // FuncFParamList
        value.move< ASTFuncFParamList* > (std::move (that.value));
        break;

      case 61: // InitVal
        value.move< ASTInitVal* > (std::move (that.value));
        break;

      case 62: // InitValList
        value.move< ASTInitValList* > (std::move (that.value));
        break;

      case 75: // IterationStmt
        value.move< ASTIterationStmt* > (std::move (that.value));
        break;

      case 93: // LAndExp
        value.move< ASTLAndExp* > (std::move (that.value));
        break;

      case 94: // LOrExp
        value.move< ASTLOrExp* > (std::move (that.value));
        break;

      case 81: // LVal
        value.move< ASTLVal* > (std::move (that.value));
        break;

      case 89: // MulExp
        value.move< ASTMulExp* > (std::move (that.value));
        break;

      case 84: // Number
        value.move< ASTNumber* > (std::move (that.value));
        break;

      case 66: // ParamArrayExpList
        value.move< ASTParamArrayExpList* > (std::move (that.value));
        break;

      case 88: // ExpList
        value.move< ASTParamExpList* > (std::move (that.value));
        break;

      case 83: // PrimaryExp
        value.move< ASTPrimaryExp* > (std::move (that.value));
        break;

      case 91: // RelExp
        value.move< ASTRelExp* > (std::move (that.value));
        break;

      case 78: // ReturnStmt
        value.move< ASTReturnStmt* > (std::move (that.value));
        break;

      case 74: // SelectStmt
        value.move< ASTSelectStmt* > (std::move (that.value));
        break;

      case 52: // BType
        value.move< ASTType > (std::move (that.value));
        break;

      case 85: // UnaryExp
        value.move< ASTUnaryExp* > (std::move (that.value));
        break;

      case 58: // VarDecl
        value.move< ASTVarDeclaration* > (std::move (that.value));
        break;

      case 60: // VarDef
        value.move< ASTVarDef* > (std::move (that.value));
        break;

      case 59: // VarDefList
        value.move< ASTVarDefList* > (std::move (that.value));
        break;

      case 87: // UnaryOp
        value.move< UnaryOp > (std::move (that.value));
        break;

      case 44: // FLOATCONST
        value.move< float > (std::move (that.value));
        break;

      case 45: // INTCONST
        value.move< int > (std::move (that.value));
        break;

      case 50: // DeclDef
        value.move< std::shared_ptr<ASTDeclaration> > (std::move (that.value));
        break;

      case 70: // Stmt
        value.move< std::shared_ptr<ASTStmt> > (std::move (that.value));
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.move< std::string > (std::move (that.value));
        break;

      default:
        break;
    }

  }
#endif

  template <typename Base>
  sysY_parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->type_get ())
    {
      case 90: // AddExp
        value.copy< ASTAddExp* > (YY_MOVE (that.value));
        break;

      case 55: // ArrayConstExpList
        value.copy< ASTArrayConstExpList* > (YY_MOVE (that.value));
        break;

      case 82: // ArrayExpList
        value.copy< ASTArrayExpList* > (YY_MOVE (that.value));
        break;

      case 71: // AssignStmt
        value.copy< ASTAssignStmt* > (YY_MOVE (that.value));
        break;

      case 67: // Block
        value.copy< ASTBlock* > (YY_MOVE (that.value));
        break;

      case 69: // BlockItem
        value.copy< ASTBlockItem* > (YY_MOVE (that.value));
        break;

      case 68: // BlockItemList
        value.copy< ASTBlockItemList* > (YY_MOVE (that.value));
        break;

      case 73: // BlockStmt
        value.copy< ASTBlockStmt* > (YY_MOVE (that.value));
        break;

      case 76: // BreakStmt
        value.copy< ASTBreakStmt* > (YY_MOVE (that.value));
        break;

      case 86: // Callee
        value.copy< ASTCallee* > (YY_MOVE (that.value));
        break;

      case 49: // CompUnit
        value.copy< ASTCompUnit* > (YY_MOVE (that.value));
        break;

      case 80: // Cond
        value.copy< ASTCond* > (YY_MOVE (that.value));
        break;

      case 51: // ConstDecl
        value.copy< ASTConstDeclaration* > (YY_MOVE (that.value));
        break;

      case 54: // ConstDef
        value.copy< ASTConstDef* > (YY_MOVE (that.value));
        break;

      case 53: // ConstDefList
        value.copy< ASTConstDefList* > (YY_MOVE (that.value));
        break;

      case 95: // ConstExp
        value.copy< ASTConstExp* > (YY_MOVE (that.value));
        break;

      case 56: // ConstInitVal
        value.copy< ASTConstInitVal* > (YY_MOVE (that.value));
        break;

      case 57: // ConstInitValList
        value.copy< ASTConstInitValList* > (YY_MOVE (that.value));
        break;

      case 77: // ContinueStmt
        value.copy< ASTContinueStmt* > (YY_MOVE (that.value));
        break;

      case 92: // EqExp
        value.copy< ASTEqExp* > (YY_MOVE (that.value));
        break;

      case 79: // Exp
        value.copy< ASTExp* > (YY_MOVE (that.value));
        break;

      case 72: // ExpStmt
        value.copy< ASTExpStmt* > (YY_MOVE (that.value));
        break;

      case 63: // FuncDef
        value.copy< ASTFuncDef* > (YY_MOVE (that.value));
        break;

      case 65: // FuncFParam
        value.copy< ASTFuncFParam* > (YY_MOVE (that.value));
        break;

      case 64: // FuncFParamList
        value.copy< ASTFuncFParamList* > (YY_MOVE (that.value));
        break;

      case 61: // InitVal
        value.copy< ASTInitVal* > (YY_MOVE (that.value));
        break;

      case 62: // InitValList
        value.copy< ASTInitValList* > (YY_MOVE (that.value));
        break;

      case 75: // IterationStmt
        value.copy< ASTIterationStmt* > (YY_MOVE (that.value));
        break;

      case 93: // LAndExp
        value.copy< ASTLAndExp* > (YY_MOVE (that.value));
        break;

      case 94: // LOrExp
        value.copy< ASTLOrExp* > (YY_MOVE (that.value));
        break;

      case 81: // LVal
        value.copy< ASTLVal* > (YY_MOVE (that.value));
        break;

      case 89: // MulExp
        value.copy< ASTMulExp* > (YY_MOVE (that.value));
        break;

      case 84: // Number
        value.copy< ASTNumber* > (YY_MOVE (that.value));
        break;

      case 66: // ParamArrayExpList
        value.copy< ASTParamArrayExpList* > (YY_MOVE (that.value));
        break;

      case 88: // ExpList
        value.copy< ASTParamExpList* > (YY_MOVE (that.value));
        break;

      case 83: // PrimaryExp
        value.copy< ASTPrimaryExp* > (YY_MOVE (that.value));
        break;

      case 91: // RelExp
        value.copy< ASTRelExp* > (YY_MOVE (that.value));
        break;

      case 78: // ReturnStmt
        value.copy< ASTReturnStmt* > (YY_MOVE (that.value));
        break;

      case 74: // SelectStmt
        value.copy< ASTSelectStmt* > (YY_MOVE (that.value));
        break;

      case 52: // BType
        value.copy< ASTType > (YY_MOVE (that.value));
        break;

      case 85: // UnaryExp
        value.copy< ASTUnaryExp* > (YY_MOVE (that.value));
        break;

      case 58: // VarDecl
        value.copy< ASTVarDeclaration* > (YY_MOVE (that.value));
        break;

      case 60: // VarDef
        value.copy< ASTVarDef* > (YY_MOVE (that.value));
        break;

      case 59: // VarDefList
        value.copy< ASTVarDefList* > (YY_MOVE (that.value));
        break;

      case 87: // UnaryOp
        value.copy< UnaryOp > (YY_MOVE (that.value));
        break;

      case 44: // FLOATCONST
        value.copy< float > (YY_MOVE (that.value));
        break;

      case 45: // INTCONST
        value.copy< int > (YY_MOVE (that.value));
        break;

      case 50: // DeclDef
        value.copy< std::shared_ptr<ASTDeclaration> > (YY_MOVE (that.value));
        break;

      case 70: // Stmt
        value.copy< std::shared_ptr<ASTStmt> > (YY_MOVE (that.value));
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.copy< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }



  template <typename Base>
  bool
  sysY_parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  void
  sysY_parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->type_get ())
    {
      case 90: // AddExp
        value.move< ASTAddExp* > (YY_MOVE (s.value));
        break;

      case 55: // ArrayConstExpList
        value.move< ASTArrayConstExpList* > (YY_MOVE (s.value));
        break;

      case 82: // ArrayExpList
        value.move< ASTArrayExpList* > (YY_MOVE (s.value));
        break;

      case 71: // AssignStmt
        value.move< ASTAssignStmt* > (YY_MOVE (s.value));
        break;

      case 67: // Block
        value.move< ASTBlock* > (YY_MOVE (s.value));
        break;

      case 69: // BlockItem
        value.move< ASTBlockItem* > (YY_MOVE (s.value));
        break;

      case 68: // BlockItemList
        value.move< ASTBlockItemList* > (YY_MOVE (s.value));
        break;

      case 73: // BlockStmt
        value.move< ASTBlockStmt* > (YY_MOVE (s.value));
        break;

      case 76: // BreakStmt
        value.move< ASTBreakStmt* > (YY_MOVE (s.value));
        break;

      case 86: // Callee
        value.move< ASTCallee* > (YY_MOVE (s.value));
        break;

      case 49: // CompUnit
        value.move< ASTCompUnit* > (YY_MOVE (s.value));
        break;

      case 80: // Cond
        value.move< ASTCond* > (YY_MOVE (s.value));
        break;

      case 51: // ConstDecl
        value.move< ASTConstDeclaration* > (YY_MOVE (s.value));
        break;

      case 54: // ConstDef
        value.move< ASTConstDef* > (YY_MOVE (s.value));
        break;

      case 53: // ConstDefList
        value.move< ASTConstDefList* > (YY_MOVE (s.value));
        break;

      case 95: // ConstExp
        value.move< ASTConstExp* > (YY_MOVE (s.value));
        break;

      case 56: // ConstInitVal
        value.move< ASTConstInitVal* > (YY_MOVE (s.value));
        break;

      case 57: // ConstInitValList
        value.move< ASTConstInitValList* > (YY_MOVE (s.value));
        break;

      case 77: // ContinueStmt
        value.move< ASTContinueStmt* > (YY_MOVE (s.value));
        break;

      case 92: // EqExp
        value.move< ASTEqExp* > (YY_MOVE (s.value));
        break;

      case 79: // Exp
        value.move< ASTExp* > (YY_MOVE (s.value));
        break;

      case 72: // ExpStmt
        value.move< ASTExpStmt* > (YY_MOVE (s.value));
        break;

      case 63: // FuncDef
        value.move< ASTFuncDef* > (YY_MOVE (s.value));
        break;

      case 65: // FuncFParam
        value.move< ASTFuncFParam* > (YY_MOVE (s.value));
        break;

      case 64: // FuncFParamList
        value.move< ASTFuncFParamList* > (YY_MOVE (s.value));
        break;

      case 61: // InitVal
        value.move< ASTInitVal* > (YY_MOVE (s.value));
        break;

      case 62: // InitValList
        value.move< ASTInitValList* > (YY_MOVE (s.value));
        break;

      case 75: // IterationStmt
        value.move< ASTIterationStmt* > (YY_MOVE (s.value));
        break;

      case 93: // LAndExp
        value.move< ASTLAndExp* > (YY_MOVE (s.value));
        break;

      case 94: // LOrExp
        value.move< ASTLOrExp* > (YY_MOVE (s.value));
        break;

      case 81: // LVal
        value.move< ASTLVal* > (YY_MOVE (s.value));
        break;

      case 89: // MulExp
        value.move< ASTMulExp* > (YY_MOVE (s.value));
        break;

      case 84: // Number
        value.move< ASTNumber* > (YY_MOVE (s.value));
        break;

      case 66: // ParamArrayExpList
        value.move< ASTParamArrayExpList* > (YY_MOVE (s.value));
        break;

      case 88: // ExpList
        value.move< ASTParamExpList* > (YY_MOVE (s.value));
        break;

      case 83: // PrimaryExp
        value.move< ASTPrimaryExp* > (YY_MOVE (s.value));
        break;

      case 91: // RelExp
        value.move< ASTRelExp* > (YY_MOVE (s.value));
        break;

      case 78: // ReturnStmt
        value.move< ASTReturnStmt* > (YY_MOVE (s.value));
        break;

      case 74: // SelectStmt
        value.move< ASTSelectStmt* > (YY_MOVE (s.value));
        break;

      case 52: // BType
        value.move< ASTType > (YY_MOVE (s.value));
        break;

      case 85: // UnaryExp
        value.move< ASTUnaryExp* > (YY_MOVE (s.value));
        break;

      case 58: // VarDecl
        value.move< ASTVarDeclaration* > (YY_MOVE (s.value));
        break;

      case 60: // VarDef
        value.move< ASTVarDef* > (YY_MOVE (s.value));
        break;

      case 59: // VarDefList
        value.move< ASTVarDefList* > (YY_MOVE (s.value));
        break;

      case 87: // UnaryOp
        value.move< UnaryOp > (YY_MOVE (s.value));
        break;

      case 44: // FLOATCONST
        value.move< float > (YY_MOVE (s.value));
        break;

      case 45: // INTCONST
        value.move< int > (YY_MOVE (s.value));
        break;

      case 50: // DeclDef
        value.move< std::shared_ptr<ASTDeclaration> > (YY_MOVE (s.value));
        break;

      case 70: // Stmt
        value.move< std::shared_ptr<ASTStmt> > (YY_MOVE (s.value));
        break;

      case 43: // IDENTIFIER
      case 46: // STRINGCONST
        value.move< std::string > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_type.
  inline
  sysY_parser::by_type::by_type ()
    : type (empty_symbol)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  sysY_parser::by_type::by_type (by_type&& that)
    : type (that.type)
  {
    that.clear ();
  }
#endif

  inline
  sysY_parser::by_type::by_type (const by_type& that)
    : type (that.type)
  {}

  inline
  sysY_parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  sysY_parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  sysY_parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  sysY_parser::by_type::type_get () const YY_NOEXCEPT
  {
    return type;
  }

} // yy
#line 3455 "sysY_parser.hh"





#endif // !YY_YY_SYSY_PARSER_HH_INCLUDED
