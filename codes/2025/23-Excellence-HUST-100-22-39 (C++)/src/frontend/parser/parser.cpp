/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 4 "parser.y"

#include <memory>
#include <cstring>
#include <stdarg.h>
#include "ast.hpp"

using namespace std;

unique_ptr<CompUnitAST> root; /* the top level root node of our final AST */

extern int yylineno;
extern int yylex();
extern void yyerror(const char *s);
extern void initFileName(char *name);
char filename[100];

#line 88 "parser.cpp"

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

#include "parser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_INTTYPE = 5,                    /* INTTYPE  */
  YYSYMBOL_FLOATTYPE = 6,                  /* FLOATTYPE  */
  YYSYMBOL_VOID = 7,                       /* VOID  */
  YYSYMBOL_CONST = 8,                      /* CONST  */
  YYSYMBOL_RETURN = 9,                     /* RETURN  */
  YYSYMBOL_IF = 10,                        /* IF  */
  YYSYMBOL_ELSE = 11,                      /* ELSE  */
  YYSYMBOL_WHILE = 12,                     /* WHILE  */
  YYSYMBOL_BREAK = 13,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 14,                  /* CONTINUE  */
  YYSYMBOL_ID = 15,                        /* ID  */
  YYSYMBOL_GTE = 16,                       /* GTE  */
  YYSYMBOL_LTE = 17,                       /* LTE  */
  YYSYMBOL_GT = 18,                        /* GT  */
  YYSYMBOL_LT = 19,                        /* LT  */
  YYSYMBOL_EQ = 20,                        /* EQ  */
  YYSYMBOL_NEQ = 21,                       /* NEQ  */
  YYSYMBOL_LP = 22,                        /* LP  */
  YYSYMBOL_RP = 23,                        /* RP  */
  YYSYMBOL_LB = 24,                        /* LB  */
  YYSYMBOL_RB = 25,                        /* RB  */
  YYSYMBOL_LC = 26,                        /* LC  */
  YYSYMBOL_RC = 27,                        /* RC  */
  YYSYMBOL_COMMA = 28,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 29,                 /* SEMICOLON  */
  YYSYMBOL_NOT = 30,                       /* NOT  */
  YYSYMBOL_ASSIGN = 31,                    /* ASSIGN  */
  YYSYMBOL_MINUS = 32,                     /* MINUS  */
  YYSYMBOL_ADD = 33,                       /* ADD  */
  YYSYMBOL_MUL = 34,                       /* MUL  */
  YYSYMBOL_DIV = 35,                       /* DIV  */
  YYSYMBOL_MOD = 36,                       /* MOD  */
  YYSYMBOL_AND = 37,                       /* AND  */
  YYSYMBOL_OR = 38,                        /* OR  */
  YYSYMBOL_LOWER_THEN_ELSE = 39,           /* LOWER_THEN_ELSE  */
  YYSYMBOL_YYACCEPT = 40,                  /* $accept  */
  YYSYMBOL_Program = 41,                   /* Program  */
  YYSYMBOL_CompUnit = 42,                  /* CompUnit  */
  YYSYMBOL_DeclDef = 43,                   /* DeclDef  */
  YYSYMBOL_Decl = 44,                      /* Decl  */
  YYSYMBOL_BType = 45,                     /* BType  */
  YYSYMBOL_VoidType = 46,                  /* VoidType  */
  YYSYMBOL_DefList = 47,                   /* DefList  */
  YYSYMBOL_Def = 48,                       /* Def  */
  YYSYMBOL_Arrays = 49,                    /* Arrays  */
  YYSYMBOL_InitVal = 50,                   /* InitVal  */
  YYSYMBOL_InitValList = 51,               /* InitValList  */
  YYSYMBOL_FuncDef = 52,                   /* FuncDef  */
  YYSYMBOL_FuncParamList = 53,             /* FuncParamList  */
  YYSYMBOL_FuncParam = 54,                 /* FuncParam  */
  YYSYMBOL_Block = 55,                     /* Block  */
  YYSYMBOL_BlockItemList = 56,             /* BlockItemList  */
  YYSYMBOL_BlockItem = 57,                 /* BlockItem  */
  YYSYMBOL_Stmt = 58,                      /* Stmt  */
  YYSYMBOL_ConditionStmt = 59,             /* ConditionStmt  */
  YYSYMBOL_LoopStmt = 60,                  /* LoopStmt  */
  YYSYMBOL_ReturnStmt = 61,                /* ReturnStmt  */
  YYSYMBOL_Exp = 62,                       /* Exp  */
  YYSYMBOL_Cond = 63,                      /* Cond  */
  YYSYMBOL_LVal = 64,                      /* LVal  */
  YYSYMBOL_PrimaryExp = 65,                /* PrimaryExp  */
  YYSYMBOL_Number = 66,                    /* Number  */
  YYSYMBOL_UnaryExp = 67,                  /* UnaryExp  */
  YYSYMBOL_Call = 68,                      /* Call  */
  YYSYMBOL_UnaryOp = 69,                   /* UnaryOp  */
  YYSYMBOL_FuncArgList = 70,               /* FuncArgList  */
  YYSYMBOL_MulExp = 71,                    /* MulExp  */
  YYSYMBOL_AddExp = 72,                    /* AddExp  */
  YYSYMBOL_RelExp = 73,                    /* RelExp  */
  YYSYMBOL_EqExp = 74,                     /* EqExp  */
  YYSYMBOL_LAndExp = 75,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 76                     /* LOrExp  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
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
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
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

#if 1

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
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

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
#define YYLAST   244

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  91
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  161

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   294


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   121,   121,   127,   131,   138,   142,   149,   155,   164,
     167,   173,   179,   183,   190,   196,   201,   206,   213,   217,
     224,   228,   231,   238,   242,   249,   256,   262,   269,   278,
     282,   289,   295,   301,   311,   314,   321,   325,   332,   336,
     343,   347,   353,   358,   362,   366,   371,   376,   381,   389,
     394,   403,   411,   415,   421,   427,   433,   437,   445,   449,
     453,   460,   465,   473,   477,   481,   489,   493,   501,   504,
     507,   513,   517,   524,   528,   534,   540,   549,   553,   559,
     568,   572,   578,   584,   590,   599,   603,   609,   618,   622,
     630,   634
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT",
  "INTTYPE", "FLOATTYPE", "VOID", "CONST", "RETURN", "IF", "ELSE", "WHILE",
  "BREAK", "CONTINUE", "ID", "GTE", "LTE", "GT", "LT", "EQ", "NEQ", "LP",
  "RP", "LB", "RB", "LC", "RC", "COMMA", "SEMICOLON", "NOT", "ASSIGN",
  "MINUS", "ADD", "MUL", "DIV", "MOD", "AND", "OR", "LOWER_THEN_ELSE",
  "$accept", "Program", "CompUnit", "DeclDef", "Decl", "BType", "VoidType",
  "DefList", "Def", "Arrays", "InitVal", "InitValList", "FuncDef",
  "FuncParamList", "FuncParam", "Block", "BlockItemList", "BlockItem",
  "Stmt", "ConditionStmt", "LoopStmt", "ReturnStmt", "Exp", "Cond", "LVal",
  "PrimaryExp", "Number", "UnaryExp", "Call", "UnaryOp", "FuncArgList",
  "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-127)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      79,  -127,  -127,  -127,    20,    38,    79,  -127,  -127,    52,
      60,  -127,    63,  -127,  -127,   -11,     0,  -127,    42,    -9,
      61,    30,   211,   177,    -8,    63,  -127,    53,  -127,    26,
      94,    -4,  -127,  -127,  -127,    32,   211,  -127,  -127,  -127,
      88,  -127,  -127,  -127,  -127,  -127,   211,    84,    49,    47,
    -127,  -127,   211,   177,  -127,    26,    37,   102,  -127,    99,
      26,    20,   197,   103,   107,  -127,  -127,   211,   211,   211,
     211,   211,  -127,  -127,    71,   108,  -127,  -127,    26,   202,
     118,   122,   120,   121,  -127,  -127,  -127,    63,  -127,   133,
    -127,  -127,  -127,  -127,  -127,   123,   125,   126,  -127,  -127,
    -127,  -127,    40,  -127,  -127,  -127,  -127,    84,    84,  -127,
     177,  -127,  -127,  -127,   124,   211,   211,  -127,  -127,  -127,
    -127,  -127,   211,   130,  -127,   211,  -127,  -127,   134,    49,
      78,    81,   127,   131,   135,   132,   103,  -127,   164,   211,
     211,   211,   211,   211,   211,   211,   211,   164,  -127,   159,
      49,    49,    49,    49,    78,    78,    81,   127,  -127,   164,
    -127
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     9,    10,    11,     0,     0,     2,     4,     5,     0,
       0,     6,     0,     1,     3,    17,     0,    12,     0,    17,
       0,     0,     0,     0,    16,     0,     8,     0,     7,     0,
       0,     0,    29,    61,    62,    56,     0,    70,    69,    68,
       0,    59,    63,    60,    73,    64,     0,    77,    54,     0,
      15,    20,     0,     0,    13,     0,     0,     0,    26,    31,
       0,     0,     0,    57,     0,    18,    65,     0,     0,     0,
       0,     0,    21,    24,     0,     0,    14,    28,     0,     0,
       0,     0,     0,     0,    34,    40,    38,     0,    45,     0,
      36,    39,    47,    48,    46,     0,    59,     0,    25,    30,
      66,    71,     0,    58,    74,    75,    76,    79,    78,    22,
       0,    19,    27,    53,     0,     0,     0,    44,    43,    35,
      37,    42,     0,    32,    67,     0,    23,    52,     0,    80,
      85,    88,    90,    55,     0,     0,    33,    72,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    41,    49,
      81,    82,    83,    84,    86,    87,    89,    91,    51,     0,
      50
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -127,  -127,  -127,   165,   -50,     4,  -127,   160,   150,   -32,
     -44,  -127,  -127,   155,   128,   -23,  -127,    95,  -126,  -127,
    -127,  -127,   -22,    67,   -55,  -127,  -127,     3,  -127,  -127,
    -127,    51,   -98,   -18,    43,    39,  -127
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,     7,     8,    30,    10,    16,    17,    24,
      50,    74,    11,    31,    32,    88,    89,    90,    91,    92,
      93,    94,    95,   128,    41,    42,    43,    44,    45,    46,
     102,    47,    48,   130,   131,   132,   133
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      40,    51,    96,    63,     9,    73,    58,    86,    12,    76,
       9,    21,   149,    22,    64,    22,    52,   129,   129,    60,
      23,   158,    23,    53,    61,     1,     2,    51,    25,    26,
      75,    51,    77,   160,    96,     1,     2,    98,    13,    86,
     101,   150,   151,   152,   153,   129,   129,   129,   129,    66,
      33,    34,    57,    29,    62,   112,    22,   114,     1,     2,
      78,    87,    35,   124,    27,    61,   126,    15,   125,    36,
     104,   105,   106,    49,    72,    18,    55,    37,    19,    38,
      39,    70,    71,    96,     1,     2,     3,     4,    51,    25,
      28,   136,    96,    87,   139,   140,   141,   142,   109,   110,
     135,   143,   144,   137,    96,    33,    34,     1,     2,    59,
       4,    79,    80,    65,    81,    82,    83,    35,    67,    68,
      69,   107,   108,    97,    36,   154,   155,    52,    57,    84,
     103,    85,    37,   111,    38,    39,    33,    34,     1,     2,
     115,     4,    79,    80,   116,    81,    82,    83,    35,   117,
     118,   123,   121,   127,    22,    36,   122,   138,   147,    57,
     119,   148,    85,    37,   145,    38,    39,    33,    34,   146,
     159,    14,    20,    79,    80,    54,    81,    82,    83,    35,
      33,    34,    56,   134,   120,   157,    36,     0,   156,    99,
      57,     0,    35,    85,    37,     0,    38,    39,     0,    36,
      33,    34,     0,    49,     0,    33,    34,    37,     0,    38,
      39,     0,    35,     0,    33,    34,     0,    35,     0,    36,
     100,     0,     0,     0,    36,     0,    35,    37,     0,    38,
      39,   113,    37,    36,    38,    39,     0,     0,     0,     0,
       0,    37,     0,    38,    39
};

static const yytype_int16 yycheck[] =
{
      22,    23,    57,    35,     0,    49,    29,    57,     4,    53,
       6,    22,   138,    24,    36,    24,    24,   115,   116,    23,
      31,   147,    31,    31,    28,     5,     6,    49,    28,    29,
      52,    53,    55,   159,    89,     5,     6,    60,     0,    89,
      62,   139,   140,   141,   142,   143,   144,   145,   146,    46,
       3,     4,    26,    23,    22,    78,    24,    79,     5,     6,
      23,    57,    15,    23,    22,    28,   110,    15,    28,    22,
      67,    68,    69,    26,    27,    15,    23,    30,    15,    32,
      33,    32,    33,   138,     5,     6,     7,     8,   110,    28,
      29,   123,   147,    89,    16,    17,    18,    19,    27,    28,
     122,    20,    21,   125,   159,     3,     4,     5,     6,    15,
       8,     9,    10,    25,    12,    13,    14,    15,    34,    35,
      36,    70,    71,    24,    22,   143,   144,    24,    26,    27,
      23,    29,    30,    25,    32,    33,     3,     4,     5,     6,
      22,     8,     9,    10,    22,    12,    13,    14,    15,    29,
      29,    25,    29,    29,    24,    22,    31,    23,    23,    26,
      27,    29,    29,    30,    37,    32,    33,     3,     4,    38,
      11,     6,    12,     9,    10,    25,    12,    13,    14,    15,
       3,     4,    27,   116,    89,   146,    22,    -1,   145,    61,
      26,    -1,    15,    29,    30,    -1,    32,    33,    -1,    22,
       3,     4,    -1,    26,    -1,     3,     4,    30,    -1,    32,
      33,    -1,    15,    -1,     3,     4,    -1,    15,    -1,    22,
      23,    -1,    -1,    -1,    22,    -1,    15,    30,    -1,    32,
      33,    29,    30,    22,    32,    33,    -1,    -1,    -1,    -1,
      -1,    30,    -1,    32,    33
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     5,     6,     7,     8,    41,    42,    43,    44,    45,
      46,    52,    45,     0,    43,    15,    47,    48,    15,    15,
      47,    22,    24,    31,    49,    28,    29,    22,    29,    23,
      45,    53,    54,     3,     4,    15,    22,    30,    32,    33,
      62,    64,    65,    66,    67,    68,    69,    71,    72,    26,
      50,    62,    24,    31,    48,    23,    53,    26,    55,    15,
      23,    28,    22,    49,    62,    25,    67,    34,    35,    36,
      32,    33,    27,    50,    51,    62,    50,    55,    23,     9,
      10,    12,    13,    14,    27,    29,    44,    45,    55,    56,
      57,    58,    59,    60,    61,    62,    64,    24,    55,    54,
      23,    62,    70,    23,    67,    67,    67,    71,    71,    27,
      28,    25,    55,    29,    62,    22,    22,    29,    29,    27,
      57,    29,    31,    25,    23,    28,    50,    29,    63,    72,
      73,    74,    75,    76,    63,    62,    49,    62,    23,    16,
      17,    18,    19,    20,    21,    37,    38,    23,    29,    58,
      72,    72,    72,    72,    73,    73,    74,    75,    58,    11,
      58
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    40,    41,    42,    42,    43,    43,    44,    44,    45,
      45,    46,    47,    47,    48,    48,    48,    48,    49,    49,
      50,    50,    50,    51,    51,    52,    52,    52,    52,    53,
      53,    54,    54,    54,    55,    55,    56,    56,    57,    57,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    59,
      59,    60,    61,    61,    62,    63,    64,    64,    65,    65,
      65,    66,    66,    67,    67,    67,    68,    68,    69,    69,
      69,    70,    70,    71,    71,    71,    71,    72,    72,    72,
      73,    73,    73,    73,    73,    74,    74,    74,    75,    75,
      76,    76
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     1,     4,     3,     1,
       1,     1,     1,     3,     4,     3,     2,     1,     3,     4,
       1,     2,     3,     3,     1,     6,     5,     6,     5,     1,
       3,     2,     4,     5,     2,     3,     1,     2,     1,     1,
       1,     4,     2,     2,     2,     1,     1,     1,     1,     5,
       7,     5,     3,     2,     1,     1,     1,     2,     3,     1,
       1,     1,     1,     1,     1,     2,     3,     4,     1,     1,
       1,     1,     3,     1,     3,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


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
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


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


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
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
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
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
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
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
# endif
#endif

#ifndef yytnamerr
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
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
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
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
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
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
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
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
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


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
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
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
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
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
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
  *++yylsp = yylloc;

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

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* Program: CompUnit  */
#line 121 "parser.y"
             {
        root = unique_ptr<CompUnitAST>((yyvsp[0].comp_unit));
    }
#line 1668 "parser.cpp"
    break;

  case 3: /* CompUnit: CompUnit DeclDef  */
#line 127 "parser.y"
                     {
        (yyval.comp_unit) = (yyvsp[-1].comp_unit);
        (yyval.comp_unit)->decl_def_list.push_back(unique_ptr<DeclDefAST>((yyvsp[0].decl_def)));
    }
#line 1677 "parser.cpp"
    break;

  case 4: /* CompUnit: DeclDef  */
#line 131 "parser.y"
            {
        (yyval.comp_unit) = new CompUnitAST();
        (yyval.comp_unit)->decl_def_list.push_back(unique_ptr<DeclDefAST>((yyvsp[0].decl_def)));
    }
#line 1686 "parser.cpp"
    break;

  case 5: /* DeclDef: Decl  */
#line 138 "parser.y"
         {
        (yyval.decl_def) = new DeclDefAST();
        (yyval.decl_def)->decl = unique_ptr<DeclAST>((yyvsp[0].decl));
    }
#line 1695 "parser.cpp"
    break;

  case 6: /* DeclDef: FuncDef  */
#line 142 "parser.y"
            {
        (yyval.decl_def) = new DeclDefAST();
        (yyval.decl_def)->func_def = unique_ptr<FuncDefAST>((yyvsp[0].func_def));
    }
#line 1704 "parser.cpp"
    break;

  case 7: /* Decl: CONST BType DefList SEMICOLON  */
#line 149 "parser.y"
                                  {
        (yyval.decl) = new DeclAST();
        (yyval.decl)->is_const = true;
        (yyval.decl)->type = (yyvsp[-2].type);
        (yyval.decl)->def_list.swap((yyvsp[-1].def_list)->list);
    }
#line 1715 "parser.cpp"
    break;

  case 8: /* Decl: BType DefList SEMICOLON  */
#line 155 "parser.y"
                            {
        (yyval.decl) = new DeclAST();
        (yyval.decl)->is_const = false;
        (yyval.decl)->type = (yyvsp[-2].type);
        (yyval.decl)->def_list.swap((yyvsp[-1].def_list)->list);
    }
#line 1726 "parser.cpp"
    break;

  case 9: /* BType: INTTYPE  */
#line 164 "parser.y"
            {
        (yyval.type) = TYPE_INT;
    }
#line 1734 "parser.cpp"
    break;

  case 10: /* BType: FLOATTYPE  */
#line 167 "parser.y"
              {
        (yyval.type) = TYPE_FLOAT;
    }
#line 1742 "parser.cpp"
    break;

  case 11: /* VoidType: VOID  */
#line 173 "parser.y"
         {
        (yyval.type) = TYPE_VOID;
    }
#line 1750 "parser.cpp"
    break;

  case 12: /* DefList: Def  */
#line 179 "parser.y"
        {
        (yyval.def_list) = new DefListAST();
        (yyval.def_list)->list.push_back(unique_ptr<DefAST>((yyvsp[0].def)));
    }
#line 1759 "parser.cpp"
    break;

  case 13: /* DefList: DefList COMMA Def  */
#line 183 "parser.y"
                      {
        (yyval.def_list) = (yyvsp[-2].def_list);
        (yyval.def_list)->list.push_back(unique_ptr<DefAST>((yyvsp[0].def)));
    }
#line 1768 "parser.cpp"
    break;

  case 14: /* Def: ID Arrays ASSIGN InitVal  */
#line 190 "parser.y"
                             {
        (yyval.def) = new DefAST();
        (yyval.def)->id = unique_ptr<string>((yyvsp[-3].token));
        (yyval.def)->arrays.swap((yyvsp[-2].arrays)->list);
        (yyval.def)->init_val = unique_ptr<InitValAST>((yyvsp[0].init_val));
    }
#line 1779 "parser.cpp"
    break;

  case 15: /* Def: ID ASSIGN InitVal  */
#line 196 "parser.y"
                      {
        (yyval.def) = new DefAST();
        (yyval.def)->id = unique_ptr<string>((yyvsp[-2].token));
        (yyval.def)->init_val = unique_ptr<InitValAST>((yyvsp[0].init_val));
    }
#line 1789 "parser.cpp"
    break;

  case 16: /* Def: ID Arrays  */
#line 201 "parser.y"
              {
        (yyval.def) = new DefAST();
        (yyval.def)->id = unique_ptr<string>((yyvsp[-1].token));
        (yyval.def)->arrays.swap((yyvsp[0].arrays)->list);
    }
#line 1799 "parser.cpp"
    break;

  case 17: /* Def: ID  */
#line 206 "parser.y"
       {
        (yyval.def) = new DefAST();
        (yyval.def)->id = unique_ptr<string>((yyvsp[0].token));
    }
#line 1808 "parser.cpp"
    break;

  case 18: /* Arrays: LB Exp RB  */
#line 213 "parser.y"
              {
        (yyval.arrays) = new ArraysAST();
        (yyval.arrays)->list.push_back(unique_ptr<AddExpAST>((yyvsp[-1].add_exp)));
    }
#line 1817 "parser.cpp"
    break;

  case 19: /* Arrays: Arrays LB Exp RB  */
#line 217 "parser.y"
                     {
        (yyval.arrays) = (yyvsp[-3].arrays);
        (yyval.arrays)->list.push_back(unique_ptr<AddExpAST>((yyvsp[-1].add_exp)));
    }
#line 1826 "parser.cpp"
    break;

  case 20: /* InitVal: Exp  */
#line 224 "parser.y"
        {
        (yyval.init_val) = new InitValAST();
        (yyval.init_val)->exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 1835 "parser.cpp"
    break;

  case 21: /* InitVal: LC RC  */
#line 228 "parser.y"
          {
        (yyval.init_val) = new InitValAST();
    }
#line 1843 "parser.cpp"
    break;

  case 22: /* InitVal: LC InitValList RC  */
#line 231 "parser.y"
                      {
        (yyval.init_val) = new InitValAST();
        (yyval.init_val)->init_val_list.swap((yyvsp[-1].init_val_list)->list);
    }
#line 1852 "parser.cpp"
    break;

  case 23: /* InitValList: InitValList COMMA InitVal  */
#line 238 "parser.y"
                              {
        (yyval.init_val_list) = (yyvsp[-2].init_val_list);
        (yyval.init_val_list)->list.push_back(unique_ptr<InitValAST>((yyvsp[0].init_val)));
    }
#line 1861 "parser.cpp"
    break;

  case 24: /* InitValList: InitVal  */
#line 242 "parser.y"
            {
        (yyval.init_val_list) = new InitValListAST();
        (yyval.init_val_list)->list.push_back(unique_ptr<InitValAST>((yyvsp[0].init_val)));
    }
#line 1870 "parser.cpp"
    break;

  case 25: /* FuncDef: BType ID LP FuncParamList RP Block  */
#line 249 "parser.y"
                                       {
        (yyval.func_def) = new FuncDefAST();
        (yyval.func_def)->func_type = (yyvsp[-5].type);
        (yyval.func_def)->id = unique_ptr<string>((yyvsp[-4].token));
        (yyval.func_def)->func_param_list.swap((yyvsp[-2].func_param_list)->list);
        (yyval.func_def)->block = unique_ptr<BlockAST>((yyvsp[0].block));
    }
#line 1882 "parser.cpp"
    break;

  case 26: /* FuncDef: BType ID LP RP Block  */
#line 256 "parser.y"
                         {
        (yyval.func_def) = new FuncDefAST();
        (yyval.func_def)->func_type = (yyvsp[-4].type);
        (yyval.func_def)->id = unique_ptr<string>((yyvsp[-3].token));
        (yyval.func_def)->block = unique_ptr<BlockAST>((yyvsp[0].block));
    }
#line 1893 "parser.cpp"
    break;

  case 27: /* FuncDef: VoidType ID LP FuncParamList RP Block  */
#line 262 "parser.y"
                                          {
        (yyval.func_def) = new FuncDefAST();
        (yyval.func_def)->func_type = (yyvsp[-5].type);
        (yyval.func_def)->id = unique_ptr<string>((yyvsp[-4].token));
        (yyval.func_def)->func_param_list.swap((yyvsp[-2].func_param_list)->list);
        (yyval.func_def)->block = unique_ptr<BlockAST>((yyvsp[0].block));
    }
#line 1905 "parser.cpp"
    break;

  case 28: /* FuncDef: VoidType ID LP RP Block  */
#line 269 "parser.y"
                            {
        (yyval.func_def) = new FuncDefAST();
        (yyval.func_def)->func_type = (yyvsp[-4].type);
        (yyval.func_def)->id = unique_ptr<string>((yyvsp[-3].token));
        (yyval.func_def)->block = unique_ptr<BlockAST>((yyvsp[0].block));
    }
#line 1916 "parser.cpp"
    break;

  case 29: /* FuncParamList: FuncParam  */
#line 278 "parser.y"
              {
        (yyval.func_param_list) = new FuncParamListAST();
        (yyval.func_param_list)->list.push_back(unique_ptr<FuncParamAST>((yyvsp[0].func_param)));
    }
#line 1925 "parser.cpp"
    break;

  case 30: /* FuncParamList: FuncParamList COMMA FuncParam  */
#line 282 "parser.y"
                                  {
        (yyval.func_param_list) = (yyvsp[-2].func_param_list);
        (yyval.func_param_list)->list.push_back(unique_ptr<FuncParamAST>((yyvsp[0].func_param)));
    }
#line 1934 "parser.cpp"
    break;

  case 31: /* FuncParam: BType ID  */
#line 289 "parser.y"
             {
        (yyval.func_param) = new FuncParamAST();
        (yyval.func_param)->type = (yyvsp[-1].type);
        (yyval.func_param)->id = unique_ptr<string>((yyvsp[0].token));
        (yyval.func_param)->is_array = false;
    }
#line 1945 "parser.cpp"
    break;

  case 32: /* FuncParam: BType ID LB RB  */
#line 295 "parser.y"
                   {
        (yyval.func_param) = new FuncParamAST();
        (yyval.func_param)->type = (yyvsp[-3].type);
        (yyval.func_param)->id = unique_ptr<string>((yyvsp[-2].token));
        (yyval.func_param)->is_array = true;
    }
#line 1956 "parser.cpp"
    break;

  case 33: /* FuncParam: BType ID LB RB Arrays  */
#line 301 "parser.y"
                          {
        (yyval.func_param) = new FuncParamAST();
        (yyval.func_param)->type = (yyvsp[-4].type);
        (yyval.func_param)->id = unique_ptr<string>((yyvsp[-3].token));
        (yyval.func_param)->is_array = true;
        (yyval.func_param)->arrays.swap((yyvsp[0].arrays)->list);
    }
#line 1968 "parser.cpp"
    break;

  case 34: /* Block: LC RC  */
#line 311 "parser.y"
          {
        (yyval.block) = new BlockAST();
    }
#line 1976 "parser.cpp"
    break;

  case 35: /* Block: LC BlockItemList RC  */
#line 314 "parser.y"
                        {
        (yyval.block) = new BlockAST();
        (yyval.block)->block_item_list.swap((yyvsp[-1].block_item_list)->list);
    }
#line 1985 "parser.cpp"
    break;

  case 36: /* BlockItemList: BlockItem  */
#line 321 "parser.y"
              {
        (yyval.block_item_list) = new BlockItemListAST();
        (yyval.block_item_list)->list.push_back(unique_ptr<BlockItemAST>((yyvsp[0].block_it)));
    }
#line 1994 "parser.cpp"
    break;

  case 37: /* BlockItemList: BlockItemList BlockItem  */
#line 325 "parser.y"
                            {
        (yyval.block_item_list) = (yyvsp[-1].block_item_list);
        (yyval.block_item_list)->list.push_back(unique_ptr<BlockItemAST>((yyvsp[0].block_it)));
    }
#line 2003 "parser.cpp"
    break;

  case 38: /* BlockItem: Decl  */
#line 332 "parser.y"
         {
        (yyval.block_it) = new BlockItemAST();
        (yyval.block_it)->decl = unique_ptr<DeclAST>((yyvsp[0].decl));
    }
#line 2012 "parser.cpp"
    break;

  case 39: /* BlockItem: Stmt  */
#line 336 "parser.y"
         {
        (yyval.block_it) = new BlockItemAST();
        (yyval.block_it)->stmt = unique_ptr<StmtAST>((yyvsp[0].stmt));
    }
#line 2021 "parser.cpp"
    break;

  case 40: /* Stmt: SEMICOLON  */
#line 343 "parser.y"
              {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = SEMI;
    }
#line 2030 "parser.cpp"
    break;

  case 41: /* Stmt: LVal ASSIGN Exp SEMICOLON  */
#line 347 "parser.y"
                              {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = ASS;
        (yyval.stmt)->l_val = unique_ptr<LValAST>((yyvsp[-3].l_val));
        (yyval.stmt)->exp = unique_ptr<AddExpAST>((yyvsp[-1].add_exp));
    }
#line 2041 "parser.cpp"
    break;

  case 42: /* Stmt: Exp SEMICOLON  */
#line 353 "parser.y"
                  {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = EXP;
        (yyval.stmt)->exp = unique_ptr<AddExpAST>((yyvsp[-1].add_exp));
    }
#line 2051 "parser.cpp"
    break;

  case 43: /* Stmt: CONTINUE SEMICOLON  */
#line 358 "parser.y"
                       {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = CONT;
    }
#line 2060 "parser.cpp"
    break;

  case 44: /* Stmt: BREAK SEMICOLON  */
#line 362 "parser.y"
                    {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = BRK;
    }
#line 2069 "parser.cpp"
    break;

  case 45: /* Stmt: Block  */
#line 366 "parser.y"
          {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = BLK;
        (yyval.stmt)->block = unique_ptr<BlockAST>((yyvsp[0].block));
    }
#line 2079 "parser.cpp"
    break;

  case 46: /* Stmt: ReturnStmt  */
#line 371 "parser.y"
               {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = RET;
        (yyval.stmt)->ret_stmt = unique_ptr<ReturnStmtAST>((yyvsp[0].ret_stmt));
    }
#line 2089 "parser.cpp"
    break;

  case 47: /* Stmt: ConditionStmt  */
#line 376 "parser.y"
                  {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = COND;
        (yyval.stmt)->cond_stmt = unique_ptr<CondStmtAST>((yyvsp[0].cond_stmt));
    }
#line 2099 "parser.cpp"
    break;

  case 48: /* Stmt: LoopStmt  */
#line 381 "parser.y"
             {
        (yyval.stmt) = new StmtAST();
        (yyval.stmt)->type = LOOP;
        (yyval.stmt)->loop_stmt = unique_ptr<LoopStmtAST>((yyvsp[0].loop_stmt));
    }
#line 2109 "parser.cpp"
    break;

  case 49: /* ConditionStmt: IF LP Cond RP Stmt  */
#line 389 "parser.y"
                                             {
        (yyval.cond_stmt) = new CondStmtAST();
        (yyval.cond_stmt)->cond = unique_ptr<LOrExpAST>((yyvsp[-2].l_or_exp));
        (yyval.cond_stmt)->if_stmt = unique_ptr<StmtAST>((yyvsp[0].stmt));
    }
#line 2119 "parser.cpp"
    break;

  case 50: /* ConditionStmt: IF LP Cond RP Stmt ELSE Stmt  */
#line 394 "parser.y"
                                 {
        (yyval.cond_stmt) = new CondStmtAST();
        (yyval.cond_stmt)->cond = unique_ptr<LOrExpAST>((yyvsp[-4].l_or_exp));
        (yyval.cond_stmt)->if_stmt = unique_ptr<StmtAST>((yyvsp[-2].stmt));
        (yyval.cond_stmt)->else_stmt = unique_ptr<StmtAST>((yyvsp[0].stmt));
    }
#line 2130 "parser.cpp"
    break;

  case 51: /* LoopStmt: WHILE LP Cond RP Stmt  */
#line 403 "parser.y"
                          {
        (yyval.loop_stmt) = new LoopStmtAST();
        (yyval.loop_stmt)->cond = unique_ptr<LOrExpAST>((yyvsp[-2].l_or_exp));
        (yyval.loop_stmt)->stmt = unique_ptr<StmtAST>((yyvsp[0].stmt));
    }
#line 2140 "parser.cpp"
    break;

  case 52: /* ReturnStmt: RETURN Exp SEMICOLON  */
#line 411 "parser.y"
                         {
        (yyval.ret_stmt) = new ReturnStmtAST();
        (yyval.ret_stmt)->exp = unique_ptr<AddExpAST>((yyvsp[-1].add_exp));
    }
#line 2149 "parser.cpp"
    break;

  case 53: /* ReturnStmt: RETURN SEMICOLON  */
#line 415 "parser.y"
                     {
        (yyval.ret_stmt) = new ReturnStmtAST();
    }
#line 2157 "parser.cpp"
    break;

  case 54: /* Exp: AddExp  */
#line 421 "parser.y"
           {
        (yyval.add_exp) = (yyvsp[0].add_exp);
    }
#line 2165 "parser.cpp"
    break;

  case 55: /* Cond: LOrExp  */
#line 427 "parser.y"
           {
        (yyval.l_or_exp) = (yyvsp[0].l_or_exp);
    }
#line 2173 "parser.cpp"
    break;

  case 56: /* LVal: ID  */
#line 433 "parser.y"
       {
        (yyval.l_val) = new LValAST();
        (yyval.l_val)->id = unique_ptr<string>((yyvsp[0].token));
    }
#line 2182 "parser.cpp"
    break;

  case 57: /* LVal: ID Arrays  */
#line 437 "parser.y"
              {
        (yyval.l_val) = new LValAST();
        (yyval.l_val)->id = unique_ptr<string>((yyvsp[-1].token));
        (yyval.l_val)->arrays.swap((yyvsp[0].arrays)->list);
    }
#line 2192 "parser.cpp"
    break;

  case 58: /* PrimaryExp: LP Exp RP  */
#line 445 "parser.y"
              {
        (yyval.pri_exp) = new PrimaryExpAST();
        (yyval.pri_exp)->exp = unique_ptr<AddExpAST>((yyvsp[-1].add_exp));
    }
#line 2201 "parser.cpp"
    break;

  case 59: /* PrimaryExp: LVal  */
#line 449 "parser.y"
         {
        (yyval.pri_exp) = new PrimaryExpAST();
        (yyval.pri_exp)->l_val = unique_ptr<LValAST>((yyvsp[0].l_val));
    }
#line 2210 "parser.cpp"
    break;

  case 60: /* PrimaryExp: Number  */
#line 453 "parser.y"
           {
        (yyval.pri_exp) = new PrimaryExpAST();
        (yyval.pri_exp)->number = unique_ptr<NumberAST>((yyvsp[0].number));
    }
#line 2219 "parser.cpp"
    break;

  case 61: /* Number: INT  */
#line 460 "parser.y"
        {
        (yyval.number) = new NumberAST();
        (yyval.number)->is_int = true;
        (yyval.number)->int_val = (yyvsp[0].int_val);
    }
#line 2229 "parser.cpp"
    break;

  case 62: /* Number: FLOAT  */
#line 465 "parser.y"
          {
        (yyval.number) = new NumberAST();
        (yyval.number)->is_int = false;
        (yyval.number)->float_val = (yyvsp[0].float_val);
    }
#line 2239 "parser.cpp"
    break;

  case 63: /* UnaryExp: PrimaryExp  */
#line 473 "parser.y"
               {
        (yyval.unary_exp) = new UnaryExpAST();
        (yyval.unary_exp)->pri_exp = unique_ptr<PrimaryExpAST>((yyvsp[0].pri_exp));
    }
#line 2248 "parser.cpp"
    break;

  case 64: /* UnaryExp: Call  */
#line 477 "parser.y"
         {
        (yyval.unary_exp) = new UnaryExpAST();
        (yyval.unary_exp)->call = unique_ptr<CallAST>((yyvsp[0].call));
    }
#line 2257 "parser.cpp"
    break;

  case 65: /* UnaryExp: UnaryOp UnaryExp  */
#line 481 "parser.y"
                     {
        (yyval.unary_exp) = new UnaryExpAST();
        (yyval.unary_exp)->op = (yyvsp[-1].op);
        (yyval.unary_exp)->unary_exp = unique_ptr<UnaryExpAST>((yyvsp[0].unary_exp));
    }
#line 2267 "parser.cpp"
    break;

  case 66: /* Call: ID LP RP  */
#line 489 "parser.y"
             {
        (yyval.call) = new CallAST();
        (yyval.call)->id = unique_ptr<string>((yyvsp[-2].token));
    }
#line 2276 "parser.cpp"
    break;

  case 67: /* Call: ID LP FuncArgList RP  */
#line 493 "parser.y"
                         {
        (yyval.call) = new CallAST();
        (yyval.call)->id = unique_ptr<string>((yyvsp[-3].token));
        (yyval.call)->func_arg_list.swap((yyvsp[-1].func_arg_list)->list);
    }
#line 2286 "parser.cpp"
    break;

  case 68: /* UnaryOp: ADD  */
#line 501 "parser.y"
        {
        (yyval.op) = UOP_ADD;
    }
#line 2294 "parser.cpp"
    break;

  case 69: /* UnaryOp: MINUS  */
#line 504 "parser.y"
          {
        (yyval.op) = UOP_MINUS;
    }
#line 2302 "parser.cpp"
    break;

  case 70: /* UnaryOp: NOT  */
#line 507 "parser.y"
        {
        (yyval.op) = UOP_NOT;
    }
#line 2310 "parser.cpp"
    break;

  case 71: /* FuncArgList: Exp  */
#line 513 "parser.y"
        {
        (yyval.func_arg_list) = new FuncArgListAST();
        (yyval.func_arg_list)->list.push_back(unique_ptr<AddExpAST>((yyvsp[0].add_exp)));
    }
#line 2319 "parser.cpp"
    break;

  case 72: /* FuncArgList: FuncArgList COMMA Exp  */
#line 517 "parser.y"
                          {
        (yyval.func_arg_list) = (FuncArgListAST*) (yyvsp[-2].func_arg_list);
        (yyval.func_arg_list)->list.push_back(unique_ptr<AddExpAST>((yyvsp[0].add_exp)));
    }
#line 2328 "parser.cpp"
    break;

  case 73: /* MulExp: UnaryExp  */
#line 524 "parser.y"
             {
        (yyval.mul_exp) = new MulExpAST();
        (yyval.mul_exp)->unary_exp = unique_ptr<UnaryExpAST>((yyvsp[0].unary_exp));
    }
#line 2337 "parser.cpp"
    break;

  case 74: /* MulExp: MulExp MUL UnaryExp  */
#line 528 "parser.y"
                        {
        (yyval.mul_exp) = new MulExpAST();
        (yyval.mul_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[-2].mul_exp));
        (yyval.mul_exp)->op = MOP_MUL;
        (yyval.mul_exp)->unary_exp = unique_ptr<UnaryExpAST>((yyvsp[0].unary_exp));
    }
#line 2348 "parser.cpp"
    break;

  case 75: /* MulExp: MulExp DIV UnaryExp  */
#line 534 "parser.y"
                        {
        (yyval.mul_exp) = new MulExpAST();
        (yyval.mul_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[-2].mul_exp));
        (yyval.mul_exp)->op = MOP_DIV;
        (yyval.mul_exp)->unary_exp = unique_ptr<UnaryExpAST>((yyvsp[0].unary_exp));
    }
#line 2359 "parser.cpp"
    break;

  case 76: /* MulExp: MulExp MOD UnaryExp  */
#line 540 "parser.y"
                        {
        (yyval.mul_exp) = new MulExpAST();
        (yyval.mul_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[-2].mul_exp));
        (yyval.mul_exp)->op = MOP_MOD;
        (yyval.mul_exp)->unary_exp = unique_ptr<UnaryExpAST>((yyvsp[0].unary_exp));
    }
#line 2370 "parser.cpp"
    break;

  case 77: /* AddExp: MulExp  */
#line 549 "parser.y"
           {
        (yyval.add_exp) = new AddExpAST();
        (yyval.add_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[0].mul_exp));
    }
#line 2379 "parser.cpp"
    break;

  case 78: /* AddExp: AddExp ADD MulExp  */
#line 553 "parser.y"
                      {
        (yyval.add_exp) = new AddExpAST();
        (yyval.add_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[-2].add_exp));
        (yyval.add_exp)->op = AOP_ADD;
        (yyval.add_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[0].mul_exp));
    }
#line 2390 "parser.cpp"
    break;

  case 79: /* AddExp: AddExp MINUS MulExp  */
#line 559 "parser.y"
                        {
        (yyval.add_exp) = new AddExpAST();
        (yyval.add_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[-2].add_exp));
        (yyval.add_exp)->op = AOP_MINUS;
        (yyval.add_exp)->mul_exp = unique_ptr<MulExpAST>((yyvsp[0].mul_exp));
    }
#line 2401 "parser.cpp"
    break;

  case 80: /* RelExp: AddExp  */
#line 568 "parser.y"
           {
        (yyval.rel_exp) = new RelExpAST();
        (yyval.rel_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 2410 "parser.cpp"
    break;

  case 81: /* RelExp: RelExp GTE AddExp  */
#line 572 "parser.y"
                      {
        (yyval.rel_exp) = new RelExpAST();
        (yyval.rel_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[-2].rel_exp));
        (yyval.rel_exp)->op = ROP_GTE;
        (yyval.rel_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 2421 "parser.cpp"
    break;

  case 82: /* RelExp: RelExp LTE AddExp  */
#line 578 "parser.y"
                      {
        (yyval.rel_exp) = new RelExpAST();
        (yyval.rel_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[-2].rel_exp));
        (yyval.rel_exp)->op = ROP_LTE;
        (yyval.rel_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 2432 "parser.cpp"
    break;

  case 83: /* RelExp: RelExp GT AddExp  */
#line 584 "parser.y"
                     {
        (yyval.rel_exp) = new RelExpAST();
        (yyval.rel_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[-2].rel_exp));
        (yyval.rel_exp)->op = ROP_GT;
        (yyval.rel_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 2443 "parser.cpp"
    break;

  case 84: /* RelExp: RelExp LT AddExp  */
#line 590 "parser.y"
                     {
        (yyval.rel_exp) = new RelExpAST();
        (yyval.rel_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[-2].rel_exp));
        (yyval.rel_exp)->op = ROP_LT;
        (yyval.rel_exp)->add_exp = unique_ptr<AddExpAST>((yyvsp[0].add_exp));
    }
#line 2454 "parser.cpp"
    break;

  case 85: /* EqExp: RelExp  */
#line 599 "parser.y"
           {
        (yyval.eq_exp) = new EqExpAST();
        (yyval.eq_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[0].rel_exp));
    }
#line 2463 "parser.cpp"
    break;

  case 86: /* EqExp: EqExp EQ RelExp  */
#line 603 "parser.y"
                    {
        (yyval.eq_exp) = new EqExpAST();
        (yyval.eq_exp)->eq_exp = unique_ptr<EqExpAST>((yyvsp[-2].eq_exp));
        (yyval.eq_exp)->op = EOP_EQ;
        (yyval.eq_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[0].rel_exp));
    }
#line 2474 "parser.cpp"
    break;

  case 87: /* EqExp: EqExp NEQ RelExp  */
#line 609 "parser.y"
                     {
        (yyval.eq_exp) = new EqExpAST();
        (yyval.eq_exp)->eq_exp = unique_ptr<EqExpAST>((yyvsp[-2].eq_exp));
        (yyval.eq_exp)->op = EOP_NEQ;
        (yyval.eq_exp)->rel_exp = unique_ptr<RelExpAST>((yyvsp[0].rel_exp));
    }
#line 2485 "parser.cpp"
    break;

  case 88: /* LAndExp: EqExp  */
#line 618 "parser.y"
          {
        (yyval.l_and_exp) = new LAndExpAST();
        (yyval.l_and_exp)->eq_exp = unique_ptr<EqExpAST>((yyvsp[0].eq_exp));
    }
#line 2494 "parser.cpp"
    break;

  case 89: /* LAndExp: LAndExp AND EqExp  */
#line 622 "parser.y"
                      {
        (yyval.l_and_exp) = new LAndExpAST();
        (yyval.l_and_exp)->l_and_exp = unique_ptr<LAndExpAST>((yyvsp[-2].l_and_exp));
        (yyval.l_and_exp)->eq_exp = unique_ptr<EqExpAST>((yyvsp[0].eq_exp));
    }
#line 2504 "parser.cpp"
    break;

  case 90: /* LOrExp: LAndExp  */
#line 630 "parser.y"
            {
        (yyval.l_or_exp) = new LOrExpAST();
        (yyval.l_or_exp)->l_and_exp = unique_ptr<LAndExpAST>((yyvsp[0].l_and_exp));
    }
#line 2513 "parser.cpp"
    break;

  case 91: /* LOrExp: LOrExp OR LAndExp  */
#line 634 "parser.y"
                      {
        (yyval.l_or_exp) = new LOrExpAST();
        (yyval.l_or_exp)->l_or_exp = unique_ptr<LOrExpAST>((yyvsp[-2].l_or_exp));
        (yyval.l_or_exp)->l_and_exp = unique_ptr<LAndExpAST>((yyvsp[0].l_and_exp));
    }
#line 2523 "parser.cpp"
    break;


#line 2527 "parser.cpp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
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
                      yytoken, &yylval, &yylloc);
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
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 639 "parser.y"


void initFileName(char *name) {
    strcpy(filename, name);
}

void yyerror(const char* fmt) {
    printf("%s:%d ", filename, yylloc.first_line);
    printf("%s\n", fmt);
}
