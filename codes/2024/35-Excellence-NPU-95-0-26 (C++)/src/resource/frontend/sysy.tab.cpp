/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 8 "src/resource/frontend/sysy.y"


#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <common.h>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;


#line 86 "build/resource/frontend/sysy.tab.cpp"

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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_BUILD_RESOURCE_FRONTEND_SYSY_TAB_HPP_INCLUDED
# define YY_YY_BUILD_RESOURCE_FRONTEND_SYSY_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "src/resource/frontend/sysy.y"

  #include <memory>
  #include <string>
  #include <common.h>
  #include <string.h>

#line 136 "build/resource/frontend/sysy.tab.cpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    RETURN = 259,
    EQ = 260,
    LE = 261,
    GE = 262,
    NE = 263,
    AND = 264,
    OR = 265,
    CONST = 266,
    IF = 267,
    ELSE = 268,
    WHILE = 269,
    BREAK = 270,
    CONTINUE = 271,
    VOID = 272,
    FLOAT = 273,
    IDENT = 274,
    INT_CONST = 275,
    FLOAT_CONST = 276
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 35 "src/resource/frontend/sysy.y"

  std::string *str_val;
  float float_val;
  int int_val;
  BaseAST *ast_val;

#line 176 "build/resource/frontend/sysy.tab.cpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (std::unique_ptr<BaseAST> &ast);

#endif /* !YY_YY_BUILD_RESOURCE_FRONTEND_SYSY_TAB_HPP_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
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


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  13
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   212

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  39
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  55
/* YYNRULES -- Number of rules.  */
#define YYNRULES  115
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  175

#define YYUNDEFTOK  2
#define YYMAXUTOK   276


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    35,     2,     2,     2,    38,     2,     2,
      22,    23,    36,    33,    24,    34,     2,    37,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    27,
      31,    30,    32,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    25,     2,    26,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    28,     2,    29,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    72,    72,    80,    84,    93,    98,   104,   114,   124,
     128,   132,   139,   145,   151,   164,   168,   172,   180,   184,
     192,   197,   207,   219,   223,   232,   236,   244,   253,   258,
     265,   269,   277,   282,   291,   299,   303,   311,   316,   322,
     328,   340,   346,   357,   366,   374,   378,   399,   407,   411,
     415,   422,   427,   436,   441,   447,   452,   457,   462,   467,
     472,   484,   489,   498,   507,   517,   525,   534,   538,   545,
     550,   557,   565,   570,   580,   585,   595,   600,   611,   615,
     623,   628,   639,   643,   647,   651,   659,   664,   675,   680,
     691,   696,   706,   712,   717,   723,   731,   737,   744,   748,
     753,   762,   772,   776,   780,   787,   792,   797,   807,   811,
     815,   823,   827,   835,   839,   843
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "RETURN", "EQ", "LE", "GE", "NE",
  "AND", "OR", "CONST", "IF", "ELSE", "WHILE", "BREAK", "CONTINUE", "VOID",
  "FLOAT", "IDENT", "INT_CONST", "FLOAT_CONST", "'('", "')'", "','", "'['",
  "']'", "';'", "'{'", "'}'", "'='", "'<'", "'>'", "'+'", "'-'", "'!'",
  "'*'", "'/'", "'%'", "$accept", "CompUnit", "MultCompUnit",
  "SinCompUnit", "FuncDef", "FuncFParams", "SinFuncFParam", "FuncType",
  "ParaType", "Decl", "ConstDecl", "MulConstDef", "ArrayDimen",
  "SinArrayDimen", "ConstArrayInit", "MultiArrayElement",
  "SinArrayElement", "VarDecl", "MulVarDef", "SinVarDef", "SinConstDef",
  "InitVal", "ConstExp", "Btype", "Block", "MulBlockItem", "SinBlockItem",
  "Stmt", "IfStmt", "SinIfStmt", "MultElseStmt", "WhileStmtHead",
  "WhileStmt", "InWhile", "SinExp", "Exp", "LOrExp", "LAndExp", "EqExp",
  "EqOp", "RelExp", "RelOp", "AddExp", "MulExp", "LValR", "PrimaryExp",
  "Number", "FloatNumber", "UnaryExp", "FuncExp", "Params", "SinParams",
  "UnaryOp", "AddOp", "MulOp", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,    40,    41,    44,    91,    93,    59,   123,   125,
      61,    60,    62,    43,    45,    33,    42,    47,    37
};
# endif

#define YYPACT_NINF (-90)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      14,   -90,    32,   -90,   -90,    44,    14,   -90,    33,   -90,
     -90,   -90,    40,   -90,   -90,   -15,   -90,   -90,   -11,   -90,
      57,   149,   -90,    37,   132,   132,    76,   -90,    64,   -90,
     132,    85,    40,   -90,   -90,   -90,    10,   -90,    66,   138,
     -90,   -90,   132,   -90,   -90,   -90,   -12,   -90,   104,    94,
     170,    41,    58,    93,   -90,   -90,   -90,   -90,   -90,   -90,
     132,   -90,   -90,   110,   -90,    95,   -90,   -90,   110,   -90,
     119,    37,   120,   136,   155,   151,   -90,   132,   132,   -90,
     -90,   132,   -90,   -90,   -90,   -90,   132,   -90,   -90,   132,
     -90,   -90,   -90,   132,   -90,   115,   -90,   -90,    42,   -90,
     -90,   159,   157,   -90,   160,   -90,   -90,    94,   170,    41,
      58,    93,   -90,   -90,   -90,   117,   -90,   -90,   132,   164,
     165,   154,   161,   102,   -90,   -90,    64,   -90,     8,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   162,   -90,   155,
      78,   -90,   136,    46,   -90,   163,   132,   132,   -90,   -90,
     132,   134,   -90,   -90,   -90,   -90,   155,   155,   -90,   -90,
     -90,   168,   169,   166,   132,   155,    74,    74,   -90,   167,
     182,   -90,   -90,    74,   -90
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    15,     0,    16,    17,     0,     2,     3,     0,     5,
      45,    46,     0,     1,     4,    37,     6,     7,     0,    35,
       0,     0,    23,    11,     0,     0,    39,    25,     0,    34,
       0,     0,     0,    22,    18,    19,     0,     9,     0,    90,
      96,    97,     0,   108,   109,   110,     0,    44,    71,    72,
      74,    76,    80,    86,    93,    98,    94,    95,    88,   100,
       0,    38,    43,     0,    26,    37,    36,    41,     0,    24,
       0,     0,    12,   104,    91,     0,    27,     0,     0,    78,
      79,     0,    84,    85,    82,    83,     0,   111,   112,     0,
     113,   114,   115,     0,    99,     0,    40,    42,    50,     8,
      10,     0,    90,   105,     0,   102,    92,    73,    75,    77,
      81,    87,    89,    29,    33,     0,    30,    32,    70,     0,
       0,     0,     0,    90,    51,    20,     0,    56,    70,    48,
      52,    57,    61,    62,    58,    65,    59,     0,    69,    13,
       0,   101,     0,     0,    28,     0,     0,     0,    68,    67,
       0,    91,    21,    47,    49,    55,    14,   106,   103,    31,
      53,     0,     0,     0,     0,   107,    70,    70,    54,     0,
      63,    66,    60,    70,    64
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -90,   -90,   -90,   190,   -90,   -90,   126,   -90,   -90,   -90,
     -89,   -90,   -18,   -23,   109,   -90,    55,    73,   -90,   172,
     171,   -90,   -26,   199,   135,   -90,    79,    -5,   -90,   -90,
     -90,   -90,   -90,   -90,    84,   -24,   -90,   127,   128,   -90,
     129,   -90,   122,   123,   -90,   -90,   -90,   -90,   -55,   -90,
     -90,    67,   -90,   -90,   -90
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     5,     6,     7,    16,    36,    37,     8,    38,   124,
       9,    21,    26,    27,   114,   115,   116,    17,    18,    19,
      22,    61,    46,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,    48,    49,    50,    81,
      51,    86,    52,    53,    54,    55,    56,    57,    58,    59,
     104,   105,    60,    89,    93
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      47,    62,    31,    64,    67,    94,    47,    23,    64,   125,
      24,    10,   118,    28,    76,    25,    29,     1,    75,     2,
     119,    74,   120,   121,   122,     2,    11,   123,    40,    41,
      42,     3,     4,    70,    71,    10,    98,   153,   112,   125,
      34,    43,    44,    45,    13,    10,   118,    82,    83,   103,
      11,    64,    15,     2,   119,    35,   120,   121,   122,    20,
      11,   123,    40,    41,    42,    39,    40,    41,    42,   117,
      98,    47,    84,    85,    95,    43,    44,    45,   118,    43,
      44,    45,    24,    65,    74,    72,   119,    30,   120,   121,
     122,    87,    88,   123,    40,    41,    42,    39,    40,    41,
      42,    24,    98,    78,   157,   151,    63,    43,    44,    45,
      24,    43,    44,    45,    77,    68,    47,   117,   103,    47,
      24,   156,   161,   162,    73,    25,   163,    24,    64,    90,
      91,    92,   150,    64,    39,    40,    41,    42,    95,   165,
     169,   143,    64,    95,   113,   101,   144,    98,    43,    44,
      45,    39,    40,    41,    42,   102,    40,    41,    42,    24,
      73,   170,   171,    24,   164,    43,    44,    45,   174,    43,
      44,    45,    96,    32,   106,    79,    33,    97,    80,    73,
      24,   148,   140,   141,   142,   139,   146,   147,   149,   155,
     160,   166,   167,   168,   172,   173,    14,   100,   159,   152,
      66,    12,   145,    69,   107,    99,   108,   154,   110,   158,
     109,     0,   111
};

static const yytype_int16 yycheck[] =
{
      24,    25,    20,    26,    30,    60,    30,    22,    31,    98,
      25,     3,     4,    24,    26,    30,    27,     3,    42,    11,
      12,    39,    14,    15,    16,    11,    18,    19,    20,    21,
      22,    17,    18,    23,    24,     3,    28,    29,    93,   128,
       3,    33,    34,    35,     0,     3,     4,     6,     7,    73,
      18,    74,    19,    11,    12,    18,    14,    15,    16,    19,
      18,    19,    20,    21,    22,    19,    20,    21,    22,    95,
      28,    95,    31,    32,    28,    33,    34,    35,     4,    33,
      34,    35,    25,    19,   102,    19,    12,    30,    14,    15,
      16,    33,    34,    19,    20,    21,    22,    19,    20,    21,
      22,    25,    28,     9,    26,   123,    30,    33,    34,    35,
      25,    33,    34,    35,    10,    30,   140,   143,   142,   143,
      25,   139,   146,   147,    22,    30,   150,    25,   151,    36,
      37,    38,    30,   156,    19,    20,    21,    22,    28,   157,
     164,    24,   165,    28,    29,    25,    29,    28,    33,    34,
      35,    19,    20,    21,    22,    19,    20,    21,    22,    25,
      22,   166,   167,    25,    30,    33,    34,    35,   173,    33,
      34,    35,    63,    24,    23,     5,    27,    68,     8,    22,
      25,    27,    25,    23,    24,    26,    22,    22,    27,    27,
      27,    23,    23,    27,    27,    13,     6,    71,   143,   126,
      28,     2,   118,    32,    77,    70,    78,   128,    86,   142,
      81,    -1,    89
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,    11,    17,    18,    40,    41,    42,    46,    49,
       3,    18,    62,     0,    42,    19,    43,    56,    57,    58,
      19,    50,    59,    22,    25,    30,    51,    52,    24,    27,
      30,    51,    24,    27,     3,    18,    44,    45,    47,    19,
      20,    21,    22,    33,    34,    35,    61,    74,    75,    76,
      77,    79,    81,    82,    83,    84,    85,    86,    87,    88,
      91,    60,    74,    30,    52,    19,    58,    61,    30,    59,
      23,    24,    19,    22,    51,    74,    26,    10,     9,     5,
       8,    78,     6,     7,    31,    32,    80,    33,    34,    92,
      36,    37,    38,    93,    87,    28,    53,    53,    28,    63,
      45,    25,    19,    74,    89,    90,    23,    76,    77,    79,
      81,    82,    87,    29,    53,    54,    55,    61,     4,    12,
      14,    15,    16,    19,    48,    49,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    26,
      25,    23,    24,    24,    29,    73,    22,    22,    27,    27,
      30,    51,    56,    29,    65,    27,    51,    26,    90,    55,
      27,    74,    74,    74,    30,    51,    23,    23,    27,    74,
      66,    66,    27,    13,    66
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    39,    40,    41,    41,    42,    42,    42,    43,    44,
      44,    44,    45,    45,    45,    46,    46,    46,    47,    47,
      48,    48,    49,    50,    50,    51,    51,    52,    53,    53,
      54,    54,    55,    55,    56,    57,    57,    58,    58,    58,
      58,    59,    59,    60,    61,    62,    62,    63,    64,    64,
      64,    65,    65,    66,    66,    66,    66,    66,    66,    66,
      66,    67,    67,    68,    69,    70,    71,    72,    72,    73,
      73,    74,    75,    75,    76,    76,    77,    77,    78,    78,
      79,    79,    80,    80,    80,    80,    81,    81,    82,    82,
      83,    83,    84,    84,    84,    84,    85,    86,    87,    87,
      87,    88,    89,    89,    89,    90,    90,    90,    91,    91,
      91,    92,    92,    93,    93,    93
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     2,     2,     5,     1,
       3,     0,     2,     4,     5,     1,     1,     1,     1,     1,
       1,     2,     4,     1,     3,     1,     2,     3,     3,     2,
       1,     3,     1,     1,     2,     1,     3,     1,     3,     2,
       4,     3,     4,     1,     1,     1,     1,     3,     1,     2,
       0,     1,     1,     3,     4,     2,     1,     1,     1,     1,
       5,     1,     1,     5,     7,     1,     5,     2,     2,     1,
       0,     1,     1,     3,     1,     3,     1,     3,     1,     1,
       1,     3,     1,     1,     1,     1,     1,     3,     1,     3,
       1,     2,     3,     1,     1,     1,     1,     1,     1,     2,
       1,     4,     1,     3,     0,     1,     3,     4,     1,     1,
       1,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (ast, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, ast); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (ast);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, ast);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule, std::unique_ptr<BaseAST> &ast)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              , ast);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, ast); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  YYUSE (yyvaluep);
  YYUSE (ast);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (std::unique_ptr<BaseAST> &ast)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 72 "src/resource/frontend/sysy.y"
                 {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->multCompUnit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast = move(comp_unit);
  }
#line 1498 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 3:
#line 80 "src/resource/frontend/sysy.y"
                {
    auto ast = new MultCompUnitAST();
    ast->sinCompUnit.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1508 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 4:
#line 84 "src/resource/frontend/sysy.y"
                                {
    auto ast = (MultCompUnitAST*)((yyvsp[-1].ast_val));
    ast->sinCompUnit.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1518 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 5:
#line 93 "src/resource/frontend/sysy.y"
              {
    auto ast = new SinCompUnitAST();
    ast->constGlobal = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = COMP_CON;
    (yyval.ast_val) = ast;
  }
#line 1529 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 6:
#line 98 "src/resource/frontend/sysy.y"
                       {
    auto ast = new SinCompUnitAST();
    ast->funcType = unique_ptr<BaseAST>((yyvsp[-1].ast_val)); 
    ast->funcDef = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = COMP_FUNC;
    (yyval.ast_val) = ast;
  }
#line 1541 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 7:
#line 104 "src/resource/frontend/sysy.y"
                      {
    auto ast = new SinCompUnitAST();
    ast->funcType = unique_ptr<BaseAST>((yyvsp[-1].ast_val)); 
    ast->varGlobal = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = COMP_VAR;
    (yyval.ast_val) = ast;
  }
#line 1553 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 8:
#line 114 "src/resource/frontend/sysy.y"
                                   {
    auto ast = new FuncDefAST();
    ast->ident = *unique_ptr<string>((yyvsp[-4].str_val));
    ast->FuncFParams = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->block = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 1565 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 9:
#line 124 "src/resource/frontend/sysy.y"
                  {
    auto ast = new FuncFParamsAST();
    ast->para.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1575 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 10:
#line 128 "src/resource/frontend/sysy.y"
                                    {
    auto ast = (FuncFParamsAST *)((yyvsp[-2].ast_val));
    ast->para.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1585 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 11:
#line 132 "src/resource/frontend/sysy.y"
      {
    auto ast = new FuncFParamsAST();
    (yyval.ast_val) = ast;
  }
#line 1594 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 12:
#line 139 "src/resource/frontend/sysy.y"
                   {
    auto ast = new SinFuncFParamAST();
    ast->ident = *unique_ptr<string>((yyvsp[0].str_val));
    ast->paraType = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->type = PARA_VAR;
    (yyval.ast_val) = ast;
  }
#line 1606 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 13:
#line 145 "src/resource/frontend/sysy.y"
                              {
    auto ast = new SinFuncFParamAST();
    ast->ident = *unique_ptr<string>((yyvsp[-2].str_val));
    ast->paraType = unique_ptr<BaseAST>((yyvsp[-3].ast_val));
    ast->type = PARA_ARR_SIN;
    (yyval.ast_val) = ast;
  }
#line 1618 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 14:
#line 151 "src/resource/frontend/sysy.y"
                                        {
    auto ast = new SinFuncFParamAST();
    ast->ident = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->paraType = unique_ptr<BaseAST>((yyvsp[-4].ast_val));
    ast->arrayDimen = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = PARA_ARR_MUL;
    (yyval.ast_val) = ast;
  }
#line 1631 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 15:
#line 164 "src/resource/frontend/sysy.y"
        {
    auto ast = new FuncTypeAST();
    ast->type = FUNCTYPE_INT;
    (yyval.ast_val) = ast;
  }
#line 1641 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 16:
#line 168 "src/resource/frontend/sysy.y"
           {
    auto ast = new FuncTypeAST();
    ast->type = FUNCTYPE_VOID;
    (yyval.ast_val) = ast;
  }
#line 1651 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 17:
#line 172 "src/resource/frontend/sysy.y"
            {
    auto ast = new FuncTypeAST();
    ast->type = FUNCTYPE_FLOAT;
    (yyval.ast_val) = ast;
  }
#line 1661 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 18:
#line 180 "src/resource/frontend/sysy.y"
        {
    auto ast = new ParaTypeAST();
    ast->type = TYPE_INT;
    (yyval.ast_val) = ast;
  }
#line 1671 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 19:
#line 184 "src/resource/frontend/sysy.y"
            {
    auto ast = new ParaTypeAST();
    ast->type = TYPE_FLOAT;
    (yyval.ast_val) = ast;
  }
#line 1681 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 20:
#line 192 "src/resource/frontend/sysy.y"
              {
    auto ast       = new DeclAST();
    ast->ConstDecl = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type      = DECLAST_CON;
    (yyval.ast_val)             = ast;
  }
#line 1692 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 21:
#line 197 "src/resource/frontend/sysy.y"
                    {
    auto ast       = new DeclAST();
    ast->btype     = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->VarDecl   = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type      = DECLAST_VAR;
    (yyval.ast_val)             = ast;
  }
#line 1704 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 22:
#line 207 "src/resource/frontend/sysy.y"
                               {
    auto ast        = new ConstDeclAST();
    ast->Btype      = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->MulConstDef= unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val)              = ast;
  }
#line 1715 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 23:
#line 219 "src/resource/frontend/sysy.y"
                {
      auto ast = new MulConstDefAST();
      ast->SinConstDef.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
      (yyval.ast_val)       = ast;
  }
#line 1725 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 24:
#line 223 "src/resource/frontend/sysy.y"
                                 {
      auto ast = (MulConstDefAST*)((yyvsp[-2].ast_val));
      ast->SinConstDef.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
      (yyval.ast_val)       = ast;
  }
#line 1735 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 25:
#line 232 "src/resource/frontend/sysy.y"
                  {
    auto ast = new ArrayDimenAST();
    ast->sinArrayDimen.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1745 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 26:
#line 236 "src/resource/frontend/sysy.y"
                               {
    auto ast = (ArrayDimenAST *) ((yyvsp[-1].ast_val));
    ast->sinArrayDimen.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1755 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 27:
#line 244 "src/resource/frontend/sysy.y"
                    {
    auto ast = new SinArrayDimenAST();
    ast->exp = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val) = ast;
  }
#line 1765 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 28:
#line 253 "src/resource/frontend/sysy.y"
                              {
    auto ast = new ConstArrayInitAST();
    ast->multiArrayElement = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->type = ConstArrayInitAST::INIT_MUL;
    (yyval.ast_val) = ast;
  }
#line 1776 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 29:
#line 258 "src/resource/frontend/sysy.y"
              {
    auto ast = new ConstArrayInitAST();
    ast->type = ConstArrayInitAST::INIT_NULL;
    (yyval.ast_val) = ast;
  }
#line 1786 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 30:
#line 265 "src/resource/frontend/sysy.y"
                    {
    auto ast = new MultiArrayElementAST();
    ast->sinArrayElement.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1796 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 31:
#line 269 "src/resource/frontend/sysy.y"
                                            {
    auto ast = (MultiArrayElementAST *) ((yyvsp[-2].ast_val));
    ast->sinArrayElement.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 1806 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 32:
#line 277 "src/resource/frontend/sysy.y"
             {
    auto ast = new SinArrayElementAST();
    ast->constExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type =SinArrayElementAST::ARELEM_EX;
    (yyval.ast_val) = ast;
  }
#line 1817 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 33:
#line 282 "src/resource/frontend/sysy.y"
                     {
    auto ast = new SinArrayElementAST();
    ast->constArrayInit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type =SinArrayElementAST::ARELEM_AI;
    (yyval.ast_val) = ast;
  }
#line 1828 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 34:
#line 291 "src/resource/frontend/sysy.y"
                 {
       auto ast     = new VarDeclAST();
       ast->MulVarDef = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
       (yyval.ast_val)           = ast;
  }
#line 1838 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 35:
#line 299 "src/resource/frontend/sysy.y"
              {
       auto ast     = new MulVarDefAST();
       ast->SinValDef.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
       (yyval.ast_val)           = ast;
  }
#line 1848 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 36:
#line 303 "src/resource/frontend/sysy.y"
                             {
       auto ast     = (MulVarDefAST*)((yyvsp[-2].ast_val));
       ast->SinValDef.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
       (yyval.ast_val)           = ast;
  }
#line 1858 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 37:
#line 311 "src/resource/frontend/sysy.y"
          {
     auto ast   = new SinVarDefAST();
     ast->type  =  SINVARDEFAST_UIN;
     ast->ident = *unique_ptr<string>((yyvsp[0].str_val));
     (yyval.ast_val)         = ast;
  }
#line 1869 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 38:
#line 316 "src/resource/frontend/sysy.y"
                        {
    auto ast   = new SinVarDefAST();
    ast->type  =  SINVARDEFAST_INI;
    ast->ident = *unique_ptr<string>((yyvsp[-2].str_val));
    ast->InitVal= unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val)         = ast;
  }
#line 1881 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 39:
#line 322 "src/resource/frontend/sysy.y"
                       {
    auto ast = new SinVarDefAST();
    ast->type = SINVARDEFAST_UNI_ARR;
    ast->ident = *unique_ptr<string>((yyvsp[-1].str_val));
    ast->dimen = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 1893 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 40:
#line 328 "src/resource/frontend/sysy.y"
                                          {
    auto ast = new SinVarDefAST();
    ast->type = SINVARDEFAST_INI_ARR;
    ast->ident = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->dimen = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->constInit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 1906 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 41:
#line 340 "src/resource/frontend/sysy.y"
                       {
      auto ast      = new SinConstDefAST();
      ast->ident    = *unique_ptr<string>((yyvsp[-2].str_val));
      ast->constExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type     = SinConstDefAST::SINCONST_VAR;
      (yyval.ast_val)            = ast;
  }
#line 1918 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 42:
#line 346 "src/resource/frontend/sysy.y"
                                          {
    auto ast = new SinConstDefAST();
    ast->ident    = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->arrayDimen = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->constArrayInit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = SinConstDefAST::SINCONST_ARRAY;
    (yyval.ast_val) = ast;
  }
#line 1931 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 43:
#line 357 "src/resource/frontend/sysy.y"
        {
    auto ast = new InitValAST();
    ast->Exp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val)       = ast;
  }
#line 1941 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 44:
#line 366 "src/resource/frontend/sysy.y"
        {
     auto ast = new ConstExpAST();
     ast->Exp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
     (yyval.ast_val)       = ast;
  }
#line 1951 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 45:
#line 374 "src/resource/frontend/sysy.y"
        {
    auto ast = new BtypeAST();
    ast->type = BTYPE_INT;
    (yyval.ast_val)        = ast;
  }
#line 1961 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 46:
#line 378 "src/resource/frontend/sysy.y"
            {
    auto ast = new BtypeAST();
    ast->type = BTYPE_FLOAT;
    (yyval.ast_val) = ast;
  }
#line 1971 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 47:
#line 399 "src/resource/frontend/sysy.y"
                         {
    auto ast = new BlockAST();
    ast->MulBlockItem = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val) = ast;
  }
#line 1981 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 48:
#line 407 "src/resource/frontend/sysy.y"
                 {
    auto ast = new MulBlockItemAST();
    ast->SinBlockItem.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val)       = ast;
  }
#line 1991 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 49:
#line 411 "src/resource/frontend/sysy.y"
                               {
    auto ast = (MulBlockItemAST*)((yyvsp[-1].ast_val));
    ast->SinBlockItem.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val)       = ast;
  }
#line 2001 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 50:
#line 415 "src/resource/frontend/sysy.y"
      {
    auto ast = new MulBlockItemAST();
    (yyval.ast_val)       = ast;
  }
#line 2010 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 51:
#line 422 "src/resource/frontend/sysy.y"
         {
    auto ast = new SinBlockItemAST();
    ast->decl= unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type= SINBLOCKITEM_DEC;
    (yyval.ast_val) = ast;
  }
#line 2021 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 52:
#line 427 "src/resource/frontend/sysy.y"
           {
    auto ast = new SinBlockItemAST();
    ast->stmt= unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type= SINBLOCKITEM_STM;
    (yyval.ast_val) = ast;
  }
#line 2032 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 53:
#line 436 "src/resource/frontend/sysy.y"
                      {
    auto ast = new StmtAST();
    ast->SinExp = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->type= STMTAST_RET;
    (yyval.ast_val)       = ast;
  }
#line 2043 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 54:
#line 441 "src/resource/frontend/sysy.y"
                        {
    auto ast = new StmtAST();
    ast->Exp = unique_ptr<BaseAST> ((yyvsp[-1].ast_val));
    ast->ident = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->type= STMTAST_LVA;
    (yyval.ast_val)       = ast;
  }
#line 2055 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 55:
#line 447 "src/resource/frontend/sysy.y"
                 {
    auto ast = new StmtAST();
    ast->SinExp = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->type = STMTAST_SINE;
    (yyval.ast_val)        = ast;
  }
#line 2066 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 56:
#line 452 "src/resource/frontend/sysy.y"
            {
    auto ast = new StmtAST();
    ast->Block = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = STMTAST_BLO;
    (yyval.ast_val)        = ast;
  }
#line 2077 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 57:
#line 457 "src/resource/frontend/sysy.y"
             {
    auto ast = new StmtAST();
    ast->ifStmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = STMTAST_IF;
    (yyval.ast_val)        = ast;
  }
#line 2088 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 58:
#line 462 "src/resource/frontend/sysy.y"
                    {
    auto ast = new StmtAST();
    ast->WhileHead = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = STMTAST_WHILE;
    (yyval.ast_val)        = ast;
  }
#line 2099 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 59:
#line 467 "src/resource/frontend/sysy.y"
              {
    auto ast = new StmtAST();
    ast->InWhileStmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = STMTAST_INWHILE;
    (yyval.ast_val)        = ast;
  }
#line 2110 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 60:
#line 472 "src/resource/frontend/sysy.y"
                                    {
    auto ast = new StmtAST();
    ast->Exp = unique_ptr<BaseAST> ((yyvsp[-1].ast_val));
    ast->ident = *unique_ptr<string>((yyvsp[-4].str_val));
    ast->arrPara = unique_ptr<BaseAST>((yyvsp[-3].ast_val));
    ast->type= STMTAST_ARR;
    (yyval.ast_val) = ast;
  }
#line 2123 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 61:
#line 484 "src/resource/frontend/sysy.y"
              {
    auto ast =  new IfStmtAST();
    ast->sinIfStmt = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    ast->type = IFSTMT_SIN;
    (yyval.ast_val)           = ast;
  }
#line 2134 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 62:
#line 489 "src/resource/frontend/sysy.y"
                   {
    auto ast = new IfStmtAST();
    ast->multElseStmt = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    ast->type = IFSTMT_MUL;
    (yyval.ast_val)          = ast;
  }
#line 2145 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 63:
#line 498 "src/resource/frontend/sysy.y"
                        { 
    auto ast = new SinIfStmtAST();
    ast->exp = unique_ptr<BaseAST> ((yyvsp[-2].ast_val));
    ast->stmt = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 2156 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 64:
#line 507 "src/resource/frontend/sysy.y"
                                  {
      auto ast = new MultElseStmtAST();
      ast->exp = unique_ptr<BaseAST> ((yyvsp[-4].ast_val));
      ast->if_stmt = unique_ptr<BaseAST> ((yyvsp[-2].ast_val));
      ast->else_stmt = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
      (yyval.ast_val) = ast;
  }
#line 2168 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 65:
#line 517 "src/resource/frontend/sysy.y"
              {
    auto ast = new WhileStmtHeadAST();
    ast->WhileHead = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 2178 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 66:
#line 525 "src/resource/frontend/sysy.y"
                          {
    auto ast = new WhileStmtAST();
    ast->exp = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 2189 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 67:
#line 534 "src/resource/frontend/sysy.y"
                {
    auto ast = new InWhileAST();
    ast->type = STMTAST_CONTINUE;
    (yyval.ast_val) = ast;
  }
#line 2199 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 68:
#line 538 "src/resource/frontend/sysy.y"
                 {
    auto ast = new InWhileAST();
    ast->type = STMTAST_BREAK;
    (yyval.ast_val) = ast;
  }
#line 2209 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 69:
#line 545 "src/resource/frontend/sysy.y"
        {
    auto ast = new SinExpAST();
    ast->Exp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = SINEXPAST_EXP;
    (yyval.ast_val) = ast;
  }
#line 2220 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 70:
#line 550 "src/resource/frontend/sysy.y"
      {
    auto ast = new SinExpAST();
    ast->type = SINEXPAST_NULL;
    (yyval.ast_val) = ast;
  }
#line 2230 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 71:
#line 557 "src/resource/frontend/sysy.y"
           {
      auto ast    = new ExpAST();
      ast->LOrExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      (yyval.ast_val)          = ast;
  }
#line 2240 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 72:
#line 565 "src/resource/frontend/sysy.y"
            {
      auto ast    = new LOrExpAST();
      ast->LAndExp= unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type   = LOREXPAST_LAN;
      (yyval.ast_val)          = ast;
  }
#line 2251 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 73:
#line 570 "src/resource/frontend/sysy.y"
                        {
      auto ast    = new LOrExpAST();
      ast->LOrExp = unique_ptr<BaseAST> ((yyvsp[-2].ast_val));
      ast->LAndExp= unique_ptr<BaseAST> ((yyvsp[0].ast_val));
      ast->type   = LOREXPAST_LOR;
      (yyval.ast_val)          = ast;
  }
#line 2263 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 74:
#line 580 "src/resource/frontend/sysy.y"
          {
      auto ast    = new LAndExpAST();
      ast->EqExp  = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type   = LANDEXPAST_EQE;
      (yyval.ast_val)          = ast;
  }
#line 2274 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 75:
#line 585 "src/resource/frontend/sysy.y"
                       {
      auto ast    = new LAndExpAST();
      ast->EqExp  = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->LAndExp= unique_ptr<BaseAST>((yyvsp[-2].ast_val));
      ast->type   = LANDEXPAST_LAN;
      (yyval.ast_val)          = ast;
  }
#line 2286 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 76:
#line 595 "src/resource/frontend/sysy.y"
           {
      auto ast    = new EqExpAST();
      ast->RelExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type   = EQEXPAST_REL;
      (yyval.ast_val)          = ast;
  }
#line 2297 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 77:
#line 600 "src/resource/frontend/sysy.y"
                       {
      auto ast    = new EqExpAST();
      ast->EqExp  = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
      ast->EqOp   = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
      ast->RelExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type   = EQEXPAST_EQE;
      (yyval.ast_val)          = ast;
  }
#line 2310 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 78:
#line 611 "src/resource/frontend/sysy.y"
       {
      auto ast  = new EqOpAST();
      ast->type = EQOPAST_EQ;
      (yyval.ast_val)        = ast;
  }
#line 2320 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 79:
#line 615 "src/resource/frontend/sysy.y"
         {
      auto ast  = new EqOpAST();
      ast->type = EQOPAST_NE;
      (yyval.ast_val)        = ast;
  }
#line 2330 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 80:
#line 623 "src/resource/frontend/sysy.y"
          {
      auto ast    = new RelExpAST();
      ast->AddExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->type   = RELEXPAST_ADD;
      (yyval.ast_val)          = ast;    
  }
#line 2341 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 81:
#line 628 "src/resource/frontend/sysy.y"
                         {
      auto ast    = new RelExpAST();
      ast->AddExp  = unique_ptr<BaseAST>((yyvsp[0].ast_val));
      ast->RelOp   = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
      ast->RelExp = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
      ast->type   = RELEXPAST_REL;
      (yyval.ast_val)          = ast;
  }
#line 2354 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 82:
#line 639 "src/resource/frontend/sysy.y"
        {
    auto ast  = new RelOpAST();
    ast->type = RELOPAST_L;
    (yyval.ast_val)        = ast;
      }
#line 2364 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 83:
#line 643 "src/resource/frontend/sysy.y"
              {
    auto ast  = new RelOpAST();
    ast->type = RELOPAST_G;
    (yyval.ast_val)        = ast;
      }
#line 2374 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 84:
#line 647 "src/resource/frontend/sysy.y"
              {
    auto ast  = new RelOpAST();
    ast->type = RELOPAST_LE;
    (yyval.ast_val)        = ast;
      }
#line 2384 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 85:
#line 651 "src/resource/frontend/sysy.y"
              {
    auto ast  = new RelOpAST();
    ast->type = RELOPAST_GE;
    (yyval.ast_val)        = ast;
  }
#line 2394 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 86:
#line 659 "src/resource/frontend/sysy.y"
           {
    auto ast    = new AddExpAST();
    ast->MulExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type   = MULEXP;
    (yyval.ast_val)          = ast;
  }
#line 2405 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 87:
#line 664 "src/resource/frontend/sysy.y"
                          {
    auto ast    = new AddExpAST();
    ast->AddExp = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->AddOp  = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->MulExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type   = ADDMUL;
    (yyval.ast_val)          = ast;
  }
#line 2418 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 88:
#line 675 "src/resource/frontend/sysy.y"
             {
    auto ast      = new MulExpAST();
    ast->UnaryExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type     = MULEXPAST_UNA;
    (yyval.ast_val)            = ast;
  }
#line 2429 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 89:
#line 680 "src/resource/frontend/sysy.y"
                           {
    auto ast      = new MulExpAST();
    ast->MulExp   = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
    ast->MulOp    = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->UnaryExp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type     = MULEXPAST_MUL;
    (yyval.ast_val)            = ast;
  }
#line 2442 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 90:
#line 691 "src/resource/frontend/sysy.y"
          {
    auto ast   = new LValRAST();
    ast->ident = *unique_ptr<string>((yyvsp[0].str_val));
    ast->type = LValRAST::IDENT;
    (yyval.ast_val) = ast;
  }
#line 2453 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 91:
#line 696 "src/resource/frontend/sysy.y"
                       {
    auto ast = new LValRAST();
    ast->ident = *unique_ptr<string>((yyvsp[-1].str_val));
    ast->array = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = LValRAST::ARRAY;
    (yyval.ast_val) = ast;
  }
#line 2465 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 92:
#line 706 "src/resource/frontend/sysy.y"
                {
    auto ast  = new PrimaryExpAST();
    ast->kind = UNARYEXP;
    ast->Exp  = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->number = 0;
    (yyval.ast_val) = ast;
  }
#line 2477 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 93:
#line 712 "src/resource/frontend/sysy.y"
            {
    auto ast = new PrimaryExpAST();
    ast->kind = LVAL;
    ast->Lval = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 2488 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 94:
#line 717 "src/resource/frontend/sysy.y"
             {
    auto ast  = new PrimaryExpAST();
    ast->kind = NUMBER;
    ast->number = (yyvsp[0].int_val);
    ast->Exp = nullptr;
    (yyval.ast_val) = ast;
  }
#line 2500 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 95:
#line 723 "src/resource/frontend/sysy.y"
                  {
    auto ast = new PrimaryExpAST();
    ast->kind = FLOAT_NUMBER;
    ast->floatNumber = (yyvsp[0].float_val);
    (yyval.ast_val) = ast;
  }
#line 2511 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 96:
#line 731 "src/resource/frontend/sysy.y"
              {
    (yyval.int_val) = (yyvsp[0].int_val);
  }
#line 2519 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 97:
#line 737 "src/resource/frontend/sysy.y"
                {
    (yyval.float_val) = (yyvsp[0].float_val);
  }
#line 2527 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 98:
#line 744 "src/resource/frontend/sysy.y"
               {
    auto ast        = new UnaryExpAST_P();
    ast->PrimaryExp = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    (yyval.ast_val) = ast; 
  }
#line 2537 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 99:
#line 748 "src/resource/frontend/sysy.y"
                       {
    auto ast        = new UnaryExpAST_U();
    ast->UnaryOp    = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    ast->UnaryExp   = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = ast;
  }
#line 2548 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 100:
#line 753 "src/resource/frontend/sysy.y"
              {
    auto ast        = new UnaryExpAST_F();
    ast->FuncExp = unique_ptr<BaseAST> ((yyvsp[0].ast_val));
    (yyval.ast_val) = ast; 
  }
#line 2558 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 101:
#line 762 "src/resource/frontend/sysy.y"
                         {
    auto ast = new FuncExpAST();
    ast->ident = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->para  = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val)       = ast;
  }
#line 2569 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 102:
#line 772 "src/resource/frontend/sysy.y"
              {
    auto ast= new ParamsAST();
    ast->sinParams.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 2579 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 103:
#line 776 "src/resource/frontend/sysy.y"
                           {
    auto ast = (ParamsAST *)((yyvsp[-2].ast_val));
    ast->sinParams.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = ast;
  }
#line 2589 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 104:
#line 780 "src/resource/frontend/sysy.y"
      {
    auto ast = new ParamsAST();
    (yyval.ast_val)       = ast;
  }
#line 2598 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 105:
#line 787 "src/resource/frontend/sysy.y"
        {
    auto ast = new SinParamsAST();
    ast->exp = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = 1;
    (yyval.ast_val)  = ast;
  }
#line 2609 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 106:
#line 792 "src/resource/frontend/sysy.y"
                    {
    auto ast = new SinParamsAST();
    ast->ident = *unique_ptr<string>((yyvsp[-2].str_val));
    ast->type  = 2;
    (yyval.ast_val) = ast;
  }
#line 2620 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 107:
#line 797 "src/resource/frontend/sysy.y"
                               {
    auto ast = new SinParamsAST();
    ast->ident = *unique_ptr<string>((yyvsp[-3].str_val));
    ast->dimension = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast->type = 3;
    (yyval.ast_val) = ast;
  }
#line 2632 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 108:
#line 807 "src/resource/frontend/sysy.y"
        {
    auto ast = new UnaryOpAST();
    ast->op = '+';
    (yyval.ast_val) = ast;
  }
#line 2642 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 109:
#line 811 "src/resource/frontend/sysy.y"
          {
    auto ast = new UnaryOpAST();
    ast->op = '-';
    (yyval.ast_val) = ast;
  }
#line 2652 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 110:
#line 815 "src/resource/frontend/sysy.y"
          {
    auto ast = new UnaryOpAST();
    ast->op = '!';
    (yyval.ast_val) = ast;
  }
#line 2662 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 111:
#line 823 "src/resource/frontend/sysy.y"
        {
    auto ast = new AddOpAST();
    ast->op  = '+';
    (yyval.ast_val)       = ast;
  }
#line 2672 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 112:
#line 827 "src/resource/frontend/sysy.y"
          {
    auto ast = new AddOpAST();
    ast->op   = '-';
    (yyval.ast_val)       = ast;
  }
#line 2682 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 113:
#line 835 "src/resource/frontend/sysy.y"
        {
    auto ast = new MulOpAST();
    ast->op = '*';
    (yyval.ast_val)      = ast;
  }
#line 2692 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 114:
#line 839 "src/resource/frontend/sysy.y"
          {
    auto ast  = new MulOpAST();
    ast->op   = '/';
    (yyval.ast_val)        = ast;
  }
#line 2702 "build/resource/frontend/sysy.tab.cpp"
    break;

  case 115:
#line 843 "src/resource/frontend/sysy.y"
          {
    auto ast = new MulOpAST();
    ast->op  = '%';
    (yyval.ast_val)       = ast;
  }
#line 2712 "build/resource/frontend/sysy.tab.cpp"
    break;


#line 2716 "build/resource/frontend/sysy.tab.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (ast, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (ast, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, ast);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, ast);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
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


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (ast, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, ast);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp, ast);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 849 "src/resource/frontend/sysy.y"


// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
//void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
//  cerr << "error: " << s << endl;
//}
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  
        extern int yylineno;    // defined and maintained in lex
        extern char *yytext;    // defined and maintained in lex
        int len=strlen(yytext);
        int i;
        char buf[512]={0};
        for (i=0;i<len;++i)
        {
            sprintf(buf,"%s%d ",buf,yytext[i]);
        }
        fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}



