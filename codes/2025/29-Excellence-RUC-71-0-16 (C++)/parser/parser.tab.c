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
#line 1 "parser.y"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include "../include/ast.h"

// 声明全局根变量
std::unique_ptr<CompUnit> root;

int yylex();
void yyerror(const char *s);
extern char* yytext;
extern int line_number;
extern int col_number;

// 类型转换辅助函数
template<typename T>
std::unique_ptr<T> make_unique_from_ptr(T* ptr) {
    return std::unique_ptr<T>(ptr);
}

#line 97 "parser.tab.c"

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

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT_CONST = 3,                  /* INT_CONST  */
  YYSYMBOL_FLOAT_CONST = 4,                /* FLOAT_CONST  */
  YYSYMBOL_IDENT = 5,                      /* IDENT  */
  YYSYMBOL_STR_CONST = 6,                  /* STR_CONST  */
  YYSYMBOL_ERROR = 7,                      /* ERROR  */
  YYSYMBOL_PLUS = 8,                       /* PLUS  */
  YYSYMBOL_MINUS = 9,                      /* MINUS  */
  YYSYMBOL_MUL = 10,                       /* MUL  */
  YYSYMBOL_DIV = 11,                       /* DIV  */
  YYSYMBOL_MOD = 12,                       /* MOD  */
  YYSYMBOL_ASSIGN = 13,                    /* ASSIGN  */
  YYSYMBOL_NOT = 14,                       /* NOT  */
  YYSYMBOL_LT = 15,                        /* LT  */
  YYSYMBOL_GT = 16,                        /* GT  */
  YYSYMBOL_LEQ = 17,                       /* LEQ  */
  YYSYMBOL_GEQ = 18,                       /* GEQ  */
  YYSYMBOL_EQ = 19,                        /* EQ  */
  YYSYMBOL_NE = 20,                        /* NE  */
  YYSYMBOL_AND = 21,                       /* AND  */
  YYSYMBOL_OR = 22,                        /* OR  */
  YYSYMBOL_LPAREN = 23,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 24,                    /* RPAREN  */
  YYSYMBOL_LBRACKET = 25,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 26,                  /* RBRACKET  */
  YYSYMBOL_LBRACE = 27,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 28,                    /* RBRACE  */
  YYSYMBOL_COMMA = 29,                     /* COMMA  */
  YYSYMBOL_SEMI = 30,                      /* SEMI  */
  YYSYMBOL_CONST = 31,                     /* CONST  */
  YYSYMBOL_IF = 32,                        /* IF  */
  YYSYMBOL_ELSE = 33,                      /* ELSE  */
  YYSYMBOL_WHILE = 34,                     /* WHILE  */
  YYSYMBOL_VOID = 35,                      /* VOID  */
  YYSYMBOL_INT = 36,                       /* INT  */
  YYSYMBOL_FLOAT = 37,                     /* FLOAT  */
  YYSYMBOL_RETURN = 38,                    /* RETURN  */
  YYSYMBOL_BREAK = 39,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 40,                  /* CONTINUE  */
  YYSYMBOL_UMINUS = 41,                    /* UMINUS  */
  YYSYMBOL_UPLUS = 42,                     /* UPLUS  */
  YYSYMBOL_UNOT = 43,                      /* UNOT  */
  YYSYMBOL_LOWER_THAN_ELSE = 44,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_YYACCEPT = 45,                  /* $accept  */
  YYSYMBOL_CompUnit = 46,                  /* CompUnit  */
  YYSYMBOL_CompItemList = 47,              /* CompItemList  */
  YYSYMBOL_Decl = 48,                      /* Decl  */
  YYSYMBOL_ConstDecl = 49,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 50,              /* ConstDefList  */
  YYSYMBOL_ConstDef = 51,                  /* ConstDef  */
  YYSYMBOL_VarDecl = 52,                   /* VarDecl  */
  YYSYMBOL_VarDefList = 53,                /* VarDefList  */
  YYSYMBOL_VarDef = 54,                    /* VarDef  */
  YYSYMBOL_ArrayDims = 55,                 /* ArrayDims  */
  YYSYMBOL_FuncDef = 56,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 57,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 58,                /* FuncFParam  */
  YYSYMBOL_Block = 59,                     /* Block  */
  YYSYMBOL_BlockItemList = 60,             /* BlockItemList  */
  YYSYMBOL_Stmt = 61,                      /* Stmt  */
  YYSYMBOL_AssignStmt = 62,                /* AssignStmt  */
  YYSYMBOL_ExpStmt = 63,                   /* ExpStmt  */
  YYSYMBOL_IfStmt = 64,                    /* IfStmt  */
  YYSYMBOL_WhileStmt = 65,                 /* WhileStmt  */
  YYSYMBOL_BreakStmt = 66,                 /* BreakStmt  */
  YYSYMBOL_ContinueStmt = 67,              /* ContinueStmt  */
  YYSYMBOL_ReturnStmt = 68,                /* ReturnStmt  */
  YYSYMBOL_Exp = 69,                       /* Exp  */
  YYSYMBOL_Cond = 70,                      /* Cond  */
  YYSYMBOL_LOrExp = 71,                    /* LOrExp  */
  YYSYMBOL_LAndExp = 72,                   /* LAndExp  */
  YYSYMBOL_EqExp = 73,                     /* EqExp  */
  YYSYMBOL_RelExp = 74,                    /* RelExp  */
  YYSYMBOL_AddExp = 75,                    /* AddExp  */
  YYSYMBOL_MulExp = 76,                    /* MulExp  */
  YYSYMBOL_UnaryExp = 77,                  /* UnaryExp  */
  YYSYMBOL_FunctionCall = 78,              /* FunctionCall  */
  YYSYMBOL_FuncRParams = 79,               /* FuncRParams  */
  YYSYMBOL_PrimaryExp = 80,                /* PrimaryExp  */
  YYSYMBOL_LVal = 81,                      /* LVal  */
  YYSYMBOL_ArrayIndices = 82,              /* ArrayIndices  */
  YYSYMBOL_Number = 83,                    /* Number  */
  YYSYMBOL_StringLiteral = 84,             /* StringLiteral  */
  YYSYMBOL_ConstExp = 85,                  /* ConstExp  */
  YYSYMBOL_InitVal = 86,                   /* InitVal  */
  YYSYMBOL_InitValList = 87,               /* InitValList  */
  YYSYMBOL_ConstInitVal = 88,              /* ConstInitVal  */
  YYSYMBOL_ConstInitValList = 89,          /* ConstInitValList  */
  YYSYMBOL_BType = 90                      /* BType  */
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

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

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
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   295

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  45
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  109
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  193

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   299


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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   137,   137,   148,   155,   162,   167,   176,   177,   182,
     194,   199,   208,   215,   227,   239,   244,   253,   259,   266,
     273,   284,   289,   298,   305,   317,   324,   339,   344,   353,
     358,   363,   373,   376,   385,   392,   399,   406,   417,   418,
     419,   420,   421,   422,   423,   424,   429,   438,   442,   451,
     458,   470,   479,   486,   493,   497,   506,   511,   516,   517,
     527,   528,   538,   539,   545,   555,   556,   562,   568,   574,
     584,   585,   591,   601,   602,   608,   614,   624,   625,   629,
     633,   641,   646,   660,   665,   674,   678,   679,   680,   681,
     686,   691,   700,   705,   714,   717,   724,   732,   737,   741,
     745,   757,   762,   771,   775,   779,   791,   796,   805,   806
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT_CONST",
  "FLOAT_CONST", "IDENT", "STR_CONST", "ERROR", "PLUS", "MINUS", "MUL",
  "DIV", "MOD", "ASSIGN", "NOT", "LT", "GT", "LEQ", "GEQ", "EQ", "NE",
  "AND", "OR", "LPAREN", "RPAREN", "LBRACKET", "RBRACKET", "LBRACE",
  "RBRACE", "COMMA", "SEMI", "CONST", "IF", "ELSE", "WHILE", "VOID", "INT",
  "FLOAT", "RETURN", "BREAK", "CONTINUE", "UMINUS", "UPLUS", "UNOT",
  "LOWER_THAN_ELSE", "$accept", "CompUnit", "CompItemList", "Decl",
  "ConstDecl", "ConstDefList", "ConstDef", "VarDecl", "VarDefList",
  "VarDef", "ArrayDims", "FuncDef", "FuncFParams", "FuncFParam", "Block",
  "BlockItemList", "Stmt", "AssignStmt", "ExpStmt", "IfStmt", "WhileStmt",
  "BreakStmt", "ContinueStmt", "ReturnStmt", "Exp", "Cond", "LOrExp",
  "LAndExp", "EqExp", "RelExp", "AddExp", "MulExp", "UnaryExp",
  "FunctionCall", "FuncRParams", "PrimaryExp", "LVal", "ArrayIndices",
  "Number", "StringLiteral", "ConstExp", "InitVal", "InitValList",
  "ConstInitVal", "ConstInitValList", "BType", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-102)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      50,    47,    19,  -102,  -102,    32,    50,  -102,  -102,  -102,
    -102,    33,    63,    29,  -102,  -102,  -102,    20,    91,  -102,
       3,   101,  -102,    -6,   261,    17,   162,    24,    68,  -102,
     268,    34,    63,  -102,    53,   -10,  -102,    72,  -102,  -102,
      -2,  -102,   162,   162,   162,   162,   228,  -102,   128,    96,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,    53,    -7,   128,
      62,   261,   162,    49,  -102,   235,  -102,  -102,   268,  -102,
     150,  -102,    53,    47,    66,    52,   162,    85,  -102,  -102,
    -102,    73,  -102,  -102,   112,   162,   162,   162,   162,   162,
    -102,    53,  -102,  -102,    86,  -102,  -102,   115,  -102,  -102,
    -102,   103,   105,   200,    94,   108,  -102,  -102,   188,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,   127,   156,    68,
    -102,  -102,   146,  -102,  -102,    22,   153,   162,  -102,  -102,
     261,    96,    96,  -102,  -102,  -102,  -102,  -102,  -102,   268,
     162,   162,  -102,   165,  -102,  -102,  -102,  -102,  -102,  -102,
     162,   158,  -102,   162,  -102,   172,  -102,  -102,   175,   178,
     180,   141,    78,   128,   183,  -102,   182,   185,  -102,  -102,
      95,   162,   162,   162,   162,   162,   162,   162,   162,    95,
    -102,   184,   180,   141,    78,    78,   128,   128,   128,   128,
    -102,    95,  -102
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,   108,   109,     0,     2,     3,     7,     8,
       4,     0,     0,     0,     1,     5,     6,    17,     0,    15,
       0,     0,    10,     0,     0,     0,     0,    18,     0,    14,
       0,     0,     0,     9,     0,     0,    27,     0,    94,    95,
      90,    96,     0,     0,     0,     0,     0,    98,    56,    70,
      73,    89,    77,    86,    87,    88,    19,     0,     0,    97,
       0,     0,     0,    17,    16,     0,   103,    12,     0,    11,
       0,    23,     0,     0,    29,     0,     0,    91,    78,    79,
      80,     0,    99,   101,     0,     0,     0,     0,     0,     0,
      25,     0,    21,    20,     0,   104,   106,     0,    13,    32,
      47,     0,     0,     0,     0,     0,    34,    40,     0,    35,
      38,    39,    41,    42,    43,    44,    45,     0,    86,     0,
      24,    28,     0,    81,    83,     0,     0,     0,    85,   100,
       0,    71,    72,    74,    75,    76,    26,    22,   105,     0,
       0,     0,    54,     0,    52,    53,    33,    36,    37,    48,
       0,    30,    82,     0,    92,     0,   102,   107,     0,    57,
      58,    60,    62,    65,     0,    55,     0,    31,    84,    93,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      46,    49,    59,    61,    63,    64,    66,    67,    68,    69,
      51,     0,    50
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -102,  -102,  -102,    -3,  -102,  -102,   181,  -102,  -102,   193,
     -19,   223,   210,   173,   -28,  -102,  -101,  -102,  -102,  -102,
    -102,  -102,  -102,  -102,   -11,   104,  -102,    76,    80,     1,
     -26,    77,   -17,  -102,  -102,  -102,   -68,  -102,  -102,  -102,
     -14,   -41,  -102,   -57,  -102,     9
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,     7,     8,    21,    22,     9,    18,    19,
      27,    10,    35,    36,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   158,   159,   160,   161,   162,
      48,    49,    50,    51,   125,    52,    53,    77,    54,    55,
      66,    56,    84,    67,    97,    37
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      59,    31,   118,    15,    59,    83,    71,   148,    96,    11,
      12,    98,    60,    47,    72,    11,    30,    91,    34,    73,
      93,    75,    73,    76,    13,    78,    79,    80,    26,    90,
       3,     4,    14,    24,    81,    47,    59,    61,    17,    59,
     118,    57,    59,    25,   120,    26,   152,    68,    94,    62,
      47,   153,    23,     3,     4,    38,    39,    40,    41,    62,
      42,    43,    24,   136,   124,   126,    44,   106,    20,   181,
     133,   134,   135,    63,    26,    45,   123,    74,   190,   119,
      70,     1,   157,     3,     4,     2,     3,     4,    92,   156,
     192,   122,   143,   175,   176,   177,   178,   128,    38,    39,
      40,    41,   118,    42,    43,   147,    87,    88,    89,    44,
     127,   118,   137,    59,   163,   163,   155,   119,    45,    47,
      28,    29,    70,   118,   144,   100,   140,   101,   141,   102,
      32,    33,   167,   103,   104,   105,    85,    86,   145,   166,
     129,   130,   168,   138,   139,   163,   163,   163,   163,   186,
     187,   188,   189,    38,    39,    40,    41,   149,    42,    43,
     173,   174,   131,   132,    44,    38,    39,    40,    41,   150,
      42,    43,   151,    45,   184,   185,    44,    70,    99,   154,
     100,     1,   101,    26,   102,    45,     3,     4,   103,   104,
     105,    38,    39,    40,    41,   165,    42,    43,   169,   170,
     171,   172,    44,    38,    39,    40,    41,   179,    42,    43,
      62,    45,   180,    69,    44,    70,   146,   191,   100,     1,
     101,    64,   102,    45,     3,     4,   103,   104,   105,    16,
     142,    38,    39,    40,    41,    58,    42,    43,    38,    39,
      40,    41,    44,    42,    43,   164,   121,   182,     0,    44,
       0,    45,   183,     0,     0,    46,    82,     0,    45,     0,
       0,     0,    65,    95,    38,    39,    40,    41,     0,    42,
      43,    38,    39,    40,    41,    44,    42,    43,     0,     0,
       0,     0,    44,     0,    45,     0,     0,     0,    46,     0,
       0,    45,     0,     0,     0,    65
};

static const yytype_int16 yycheck[] =
{
      26,    20,    70,     6,    30,    46,    34,   108,    65,     0,
       1,    68,    26,    24,    24,     6,    13,    24,    24,    29,
      61,    23,    29,    25,     5,    42,    43,    44,    25,    57,
      36,    37,     0,    13,    45,    46,    62,    13,     5,    65,
     108,    24,    68,    23,    72,    25,    24,    13,    62,    25,
      61,    29,    23,    36,    37,     3,     4,     5,     6,    25,
       8,     9,    13,    91,    75,    76,    14,    70,     5,   170,
      87,    88,    89,     5,    25,    23,    24,     5,   179,    70,
      27,    31,   139,    36,    37,    35,    36,    37,    26,   130,
     191,    25,   103,    15,    16,    17,    18,    24,     3,     4,
       5,     6,   170,     8,     9,   108,    10,    11,    12,    14,
      25,   179,    26,   139,   140,   141,   127,   108,    23,   130,
      29,    30,    27,   191,    30,    30,    23,    32,    23,    34,
      29,    30,   151,    38,    39,    40,     8,     9,    30,   150,
      28,    29,   153,    28,    29,   171,   172,   173,   174,   175,
     176,   177,   178,     3,     4,     5,     6,    30,     8,     9,
      19,    20,    85,    86,    14,     3,     4,     5,     6,    13,
       8,     9,    26,    23,   173,   174,    14,    27,    28,    26,
      30,    31,    32,    25,    34,    23,    36,    37,    38,    39,
      40,     3,     4,     5,     6,    30,     8,     9,    26,    24,
      22,    21,    14,     3,     4,     5,     6,    24,     8,     9,
      25,    23,    30,    32,    14,    27,    28,    33,    30,    31,
      32,    28,    34,    23,    36,    37,    38,    39,    40,     6,
      30,     3,     4,     5,     6,    25,     8,     9,     3,     4,
       5,     6,    14,     8,     9,   141,    73,   171,    -1,    14,
      -1,    23,   172,    -1,    -1,    27,    28,    -1,    23,    -1,
      -1,    -1,    27,    28,     3,     4,     5,     6,    -1,     8,
       9,     3,     4,     5,     6,    14,     8,     9,    -1,    -1,
      -1,    -1,    14,    -1,    23,    -1,    -1,    -1,    27,    -1,
      -1,    23,    -1,    -1,    -1,    27
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    31,    35,    36,    37,    46,    47,    48,    49,    52,
      56,    90,    90,     5,     0,    48,    56,     5,    53,    54,
       5,    50,    51,    23,    13,    23,    25,    55,    29,    30,
      13,    55,    29,    30,    24,    57,    58,    90,     3,     4,
       5,     6,     8,     9,    14,    23,    27,    69,    75,    76,
      77,    78,    80,    81,    83,    84,    86,    24,    57,    75,
      85,    13,    25,     5,    54,    27,    85,    88,    13,    51,
      27,    59,    24,    29,     5,    23,    25,    82,    77,    77,
      77,    69,    28,    86,    87,     8,     9,    10,    11,    12,
      59,    24,    26,    86,    85,    28,    88,    89,    88,    28,
      30,    32,    34,    38,    39,    40,    48,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    81,    90,
      59,    58,    25,    24,    69,    79,    69,    25,    24,    28,
      29,    76,    76,    77,    77,    77,    59,    26,    28,    29,
      23,    23,    30,    69,    30,    30,    28,    48,    61,    30,
      13,    26,    24,    29,    26,    69,    86,    88,    70,    71,
      72,    73,    74,    75,    70,    30,    69,    55,    69,    26,
      24,    22,    21,    19,    20,    15,    16,    17,    18,    24,
      30,    61,    72,    73,    74,    74,    75,    75,    75,    75,
      61,    33,    61
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    45,    46,    47,    47,    47,    47,    48,    48,    49,
      50,    50,    51,    51,    52,    53,    53,    54,    54,    54,
      54,    55,    55,    56,    56,    56,    56,    57,    57,    58,
      58,    58,    59,    59,    60,    60,    60,    60,    61,    61,
      61,    61,    61,    61,    61,    61,    62,    63,    63,    64,
      64,    65,    66,    67,    68,    68,    69,    70,    71,    71,
      72,    72,    73,    73,    73,    74,    74,    74,    74,    74,
      75,    75,    75,    76,    76,    76,    76,    77,    77,    77,
      77,    78,    78,    79,    79,    80,    80,    80,    80,    80,
      81,    81,    82,    82,    83,    83,    84,    85,    86,    86,
      86,    87,    87,    88,    88,    88,    89,    89,    90,    90
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     2,     1,     1,     4,
       1,     3,     3,     4,     3,     1,     3,     1,     2,     3,
       4,     3,     4,     5,     6,     5,     6,     1,     3,     2,
       4,     5,     2,     3,     1,     1,     2,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     1,     2,     5,
       7,     5,     2,     2,     2,     3,     1,     1,     1,     3,
       1,     3,     1,     3,     3,     1,     3,     3,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     1,     2,     2,
       2,     3,     4,     1,     3,     3,     1,     1,     1,     1,
       1,     2,     3,     4,     1,     1,     1,     1,     1,     2,
       3,     1,     3,     1,     2,     3,     1,     3,     1,     1
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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
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
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
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

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

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
#  undef YYSTACK_RELOCATE
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
  case 2: /* CompUnit: CompItemList  */
#line 137 "parser.y"
                 {
        (yyval.comp_unit) = new CompUnit({line_number});
        (yyval.comp_unit)->items = std::move(*(yyvsp[0].comp_item_list));
        delete (yyvsp[0].comp_item_list);
        (yyvsp[0].comp_item_list) = nullptr;
        root = make_unique_from_ptr((yyval.comp_unit));
        (yyval.comp_unit) = nullptr;
    }
#line 1337 "parser.tab.c"
    break;

  case 3: /* CompItemList: Decl  */
#line 148 "parser.y"
         {
        (yyval.comp_item_list) = new std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>();
        std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>> item;
        item = make_unique_from_ptr((yyvsp[0].decl));
        (yyval.comp_item_list)->push_back(std::move(item));
        (yyvsp[0].decl) = nullptr;
    }
#line 1349 "parser.tab.c"
    break;

  case 4: /* CompItemList: FuncDef  */
#line 155 "parser.y"
              {
        (yyval.comp_item_list) = new std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>();
        std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>> item;
        item = make_unique_from_ptr((yyvsp[0].func_def));
        (yyval.comp_item_list)->push_back(std::move(item));
        (yyvsp[0].func_def) = nullptr;
    }
#line 1361 "parser.tab.c"
    break;

  case 5: /* CompItemList: CompItemList Decl  */
#line 162 "parser.y"
                        {
        (yyvsp[-1].comp_item_list)->emplace_back(make_unique_from_ptr((yyvsp[0].decl)));
        (yyval.comp_item_list) = (yyvsp[-1].comp_item_list);
        (yyvsp[0].decl) = nullptr;
    }
#line 1371 "parser.tab.c"
    break;

  case 6: /* CompItemList: CompItemList FuncDef  */
#line 167 "parser.y"
                           {
        (yyvsp[-1].comp_item_list)->emplace_back(make_unique_from_ptr((yyvsp[0].func_def)));
        (yyval.comp_item_list) = (yyvsp[-1].comp_item_list);
        (yyvsp[0].func_def) = nullptr; 
    }
#line 1381 "parser.tab.c"
    break;

  case 7: /* Decl: ConstDecl  */
#line 176 "parser.y"
              { (yyval.decl) = (yyvsp[0].const_decl); }
#line 1387 "parser.tab.c"
    break;

  case 8: /* Decl: VarDecl  */
#line 177 "parser.y"
              { (yyval.decl) = (yyvsp[0].var_decl); }
#line 1393 "parser.tab.c"
    break;

  case 9: /* ConstDecl: CONST BType ConstDefList SEMI  */
#line 182 "parser.y"
                                  {
        std::vector<std::unique_ptr<ConstDef>> defs;
        for (auto& def : *(yyvsp[-1].const_def_list)) {
            defs.push_back(std::move(def));
        }
        delete (yyvsp[-1].const_def_list);
        (yyvsp[-1].const_def_list) = nullptr;
        (yyval.const_decl) = new ConstDecl((yyvsp[-2].base_type), std::move(defs), {line_number});
    }
#line 1407 "parser.tab.c"
    break;

  case 10: /* ConstDefList: ConstDef  */
#line 194 "parser.y"
             {
        (yyval.const_def_list) = new std::vector<std::unique_ptr<ConstDef>>();
        (yyval.const_def_list)->push_back(make_unique_from_ptr((yyvsp[0].const_def)));
        (yyvsp[0].const_def) = nullptr;
    }
#line 1417 "parser.tab.c"
    break;

  case 11: /* ConstDefList: ConstDefList COMMA ConstDef  */
#line 199 "parser.y"
                                  {
        (yyvsp[-2].const_def_list)->push_back(make_unique_from_ptr((yyvsp[0].const_def)));
        (yyval.const_def_list) = (yyvsp[-2].const_def_list);
        (yyvsp[0].const_def) = nullptr; 
    }
#line 1427 "parser.tab.c"
    break;

  case 12: /* ConstDef: IDENT ASSIGN ConstInitVal  */
#line 208 "parser.y"
                              {
        std::vector<std::unique_ptr<Exp>> dims;
        (yyval.const_def) = new ConstDef(std::string((yyvsp[-2].str_val)), std::move(dims), 
                         make_unique_from_ptr((yyvsp[0].const_init_val)), {line_number});
        free((yyvsp[-2].str_val));
        (yyvsp[0].const_init_val) = nullptr; 
    }
#line 1439 "parser.tab.c"
    break;

  case 13: /* ConstDef: IDENT ArrayDims ASSIGN ConstInitVal  */
#line 215 "parser.y"
                                          {
        (yyval.const_def) = new ConstDef(std::string((yyvsp[-3].str_val)), std::move(*(yyvsp[-2].exp_list)), 
                         make_unique_from_ptr((yyvsp[0].const_init_val)), {line_number});
        free((yyvsp[-3].str_val));
        delete (yyvsp[-2].exp_list);
        (yyvsp[-2].exp_list) = nullptr;
        (yyvsp[0].const_init_val) = nullptr; 
    }
#line 1452 "parser.tab.c"
    break;

  case 14: /* VarDecl: BType VarDefList SEMI  */
#line 227 "parser.y"
                          {
        std::vector<std::unique_ptr<VarDef>> defs;
        for (auto& def : *(yyvsp[-1].var_def_list)) {
            defs.push_back(std::move(def));
        }
        delete (yyvsp[-1].var_def_list);
        (yyvsp[-1].var_def_list) = nullptr;
        (yyval.var_decl) = new VarDecl((yyvsp[-2].base_type), std::move(defs), {line_number});
    }
#line 1466 "parser.tab.c"
    break;

  case 15: /* VarDefList: VarDef  */
#line 239 "parser.y"
           {
        (yyval.var_def_list) = new std::vector<std::unique_ptr<VarDef>>();
        (yyval.var_def_list)->push_back(make_unique_from_ptr((yyvsp[0].var_def)));
        (yyvsp[0].var_def) = nullptr;
    }
#line 1476 "parser.tab.c"
    break;

  case 16: /* VarDefList: VarDefList COMMA VarDef  */
#line 244 "parser.y"
                              {
        (yyvsp[-2].var_def_list)->push_back(make_unique_from_ptr((yyvsp[0].var_def)));
        (yyval.var_def_list) = (yyvsp[-2].var_def_list);
        (yyvsp[0].var_def) = nullptr;
    }
#line 1486 "parser.tab.c"
    break;

  case 17: /* VarDef: IDENT  */
#line 253 "parser.y"
          {
        std::vector<std::unique_ptr<Exp>> dims;
        std::optional<std::unique_ptr<InitVal>> init;
        (yyval.var_def) = new VarDef(std::string((yyvsp[0].str_val)), std::move(dims), std::move(init), {line_number});
        free((yyvsp[0].str_val));
    }
#line 1497 "parser.tab.c"
    break;

  case 18: /* VarDef: IDENT ArrayDims  */
#line 259 "parser.y"
                      {
        std::optional<std::unique_ptr<InitVal>> init;
        (yyval.var_def) = new VarDef(std::string((yyvsp[-1].str_val)), std::move(*(yyvsp[0].exp_list)), std::move(init), {line_number});
        free((yyvsp[-1].str_val));
        delete (yyvsp[0].exp_list);
        (yyvsp[0].exp_list) = nullptr;
    }
#line 1509 "parser.tab.c"
    break;

  case 19: /* VarDef: IDENT ASSIGN InitVal  */
#line 266 "parser.y"
                           {
        std::vector<std::unique_ptr<Exp>> dims;
        std::optional<std::unique_ptr<InitVal>> init = make_unique_from_ptr((yyvsp[0].init_val));
        (yyval.var_def) = new VarDef(std::string((yyvsp[-2].str_val)), std::move(dims), std::move(init), {line_number});
        free((yyvsp[-2].str_val));
        (yyvsp[0].init_val) = nullptr; 
    }
#line 1521 "parser.tab.c"
    break;

  case 20: /* VarDef: IDENT ArrayDims ASSIGN InitVal  */
#line 273 "parser.y"
                                     {
        std::optional<std::unique_ptr<InitVal>> init = make_unique_from_ptr((yyvsp[0].init_val));
        (yyval.var_def) = new VarDef(std::string((yyvsp[-3].str_val)), std::move(*(yyvsp[-2].exp_list)), std::move(init), {line_number});
        free((yyvsp[-3].str_val));
        delete (yyvsp[-2].exp_list);
        (yyvsp[-2].exp_list) = nullptr;
        (yyvsp[0].init_val) = nullptr; 
    }
#line 1534 "parser.tab.c"
    break;

  case 21: /* ArrayDims: LBRACKET ConstExp RBRACKET  */
#line 284 "parser.y"
                               {
        (yyval.exp_list) = new std::vector<std::unique_ptr<Exp>>();
        (yyval.exp_list)->push_back(make_unique_from_ptr((yyvsp[-1].exp)));
        (yyvsp[-1].exp) = nullptr;
    }
#line 1544 "parser.tab.c"
    break;

  case 22: /* ArrayDims: ArrayDims LBRACKET ConstExp RBRACKET  */
#line 289 "parser.y"
                                           {
        (yyvsp[-3].exp_list)->push_back(make_unique_from_ptr((yyvsp[-1].exp)));
        (yyval.exp_list) = (yyvsp[-3].exp_list);
        (yyvsp[-1].exp) = nullptr; 
    }
#line 1554 "parser.tab.c"
    break;

  case 23: /* FuncDef: VOID IDENT LPAREN RPAREN Block  */
#line 298 "parser.y"
                                   {
        std::vector<std::unique_ptr<FuncFParam>> params;
        (yyval.func_def) = new FuncDef(BaseType::VOID, std::string((yyvsp[-3].str_val)), std::move(params), 
                        make_unique_from_ptr((yyvsp[0].block)), {line_number});
        free((yyvsp[-3].str_val));
        (yyvsp[0].block) = nullptr; 
    }
#line 1566 "parser.tab.c"
    break;

  case 24: /* FuncDef: VOID IDENT LPAREN FuncFParams RPAREN Block  */
#line 305 "parser.y"
                                                 {
        std::vector<std::unique_ptr<FuncFParam>> params;
        for (auto& param : *(yyvsp[-2].param_list)) {
            params.push_back(std::move(param));
        }
        delete (yyvsp[-2].param_list);
        (yyvsp[-2].param_list) = nullptr;
        (yyval.func_def) = new FuncDef(BaseType::VOID, std::string((yyvsp[-4].str_val)), std::move(params), 
                        make_unique_from_ptr((yyvsp[0].block)), {line_number});
        free((yyvsp[-4].str_val));
        (yyvsp[0].block) = nullptr; 
    }
#line 1583 "parser.tab.c"
    break;

  case 25: /* FuncDef: BType IDENT LPAREN RPAREN Block  */
#line 317 "parser.y"
                                      {
        std::vector<std::unique_ptr<FuncFParam>> params;
        (yyval.func_def) = new FuncDef((yyvsp[-4].base_type), std::string((yyvsp[-3].str_val)), std::move(params), 
                        make_unique_from_ptr((yyvsp[0].block)), {line_number});
        free((yyvsp[-3].str_val));
        (yyvsp[0].block) = nullptr; 
    }
#line 1595 "parser.tab.c"
    break;

  case 26: /* FuncDef: BType IDENT LPAREN FuncFParams RPAREN Block  */
#line 324 "parser.y"
                                                  {
        std::vector<std::unique_ptr<FuncFParam>> params;
        for (auto& param : *(yyvsp[-2].param_list)) {
            params.push_back(std::move(param));
        }
        delete (yyvsp[-2].param_list);
        (yyvsp[-2].param_list) = nullptr;
        (yyval.func_def) = new FuncDef((yyvsp[-5].base_type), std::string((yyvsp[-4].str_val)), std::move(params), 
                        make_unique_from_ptr((yyvsp[0].block)), {line_number});
        free((yyvsp[-4].str_val));
        (yyvsp[0].block) = nullptr; 
    }
#line 1612 "parser.tab.c"
    break;

  case 27: /* FuncFParams: FuncFParam  */
#line 339 "parser.y"
               {
        (yyval.param_list) = new std::vector<std::unique_ptr<FuncFParam>>();
        (yyval.param_list)->push_back(make_unique_from_ptr((yyvsp[0].func_param)));
        (yyvsp[0].func_param) = nullptr;
    }
#line 1622 "parser.tab.c"
    break;

  case 28: /* FuncFParams: FuncFParams COMMA FuncFParam  */
#line 344 "parser.y"
                                   {
        (yyvsp[-2].param_list)->push_back(make_unique_from_ptr((yyvsp[0].func_param)));
        (yyval.param_list) = (yyvsp[-2].param_list);
        (yyvsp[0].func_param) = nullptr;
    }
#line 1632 "parser.tab.c"
    break;

  case 29: /* FuncFParam: BType IDENT  */
#line 353 "parser.y"
                {
        std::vector<std::unique_ptr<Exp>> dims;
        (yyval.func_param) = new FuncFParam((yyvsp[-1].base_type), std::string((yyvsp[0].str_val)), false, std::move(dims), {line_number});
        free((yyvsp[0].str_val));
    }
#line 1642 "parser.tab.c"
    break;

  case 30: /* FuncFParam: BType IDENT LBRACKET RBRACKET  */
#line 358 "parser.y"
                                    {
        std::vector<std::unique_ptr<Exp>> dims;
        (yyval.func_param) = new FuncFParam((yyvsp[-3].base_type), std::string((yyvsp[-2].str_val)), true, std::move(dims), {line_number});
        free((yyvsp[-2].str_val));
    }
#line 1652 "parser.tab.c"
    break;

  case 31: /* FuncFParam: BType IDENT LBRACKET RBRACKET ArrayDims  */
#line 363 "parser.y"
                                              {
        (yyval.func_param) = new FuncFParam((yyvsp[-4].base_type), std::string((yyvsp[-3].str_val)), true, std::move(*(yyvsp[0].exp_list)), {line_number});
        free((yyvsp[-3].str_val));
        delete (yyvsp[0].exp_list);
        (yyvsp[0].exp_list) = nullptr;
    }
#line 1663 "parser.tab.c"
    break;

  case 32: /* Block: LBRACE RBRACE  */
#line 373 "parser.y"
                  {
        (yyval.block) = new Block({line_number});
    }
#line 1671 "parser.tab.c"
    break;

  case 33: /* Block: LBRACE BlockItemList RBRACE  */
#line 376 "parser.y"
                                  {
        (yyval.block) = new Block({line_number});
        (yyval.block)->items = std::move(*(yyvsp[-1].block_item_list));
        delete (yyvsp[-1].block_item_list);
        (yyvsp[-1].block_item_list) = nullptr;
    }
#line 1682 "parser.tab.c"
    break;

  case 34: /* BlockItemList: Decl  */
#line 385 "parser.y"
         {
        (yyval.block_item_list) = new std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>();
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr((yyvsp[0].decl));
        (yyval.block_item_list)->push_back(std::move(item));
        (yyvsp[0].decl) = nullptr; 
    }
#line 1694 "parser.tab.c"
    break;

  case 35: /* BlockItemList: Stmt  */
#line 392 "parser.y"
           {
        (yyval.block_item_list) = new std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>();
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr((yyvsp[0].stmt));
        (yyval.block_item_list)->push_back(std::move(item));
        (yyvsp[0].stmt) = nullptr; 
    }
#line 1706 "parser.tab.c"
    break;

  case 36: /* BlockItemList: BlockItemList Decl  */
#line 399 "parser.y"
                         {
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr((yyvsp[0].decl));
        (yyvsp[-1].block_item_list)->push_back(std::move(item));
        (yyval.block_item_list) = (yyvsp[-1].block_item_list);
        (yyvsp[0].decl) = nullptr; 
    }
#line 1718 "parser.tab.c"
    break;

  case 37: /* BlockItemList: BlockItemList Stmt  */
#line 406 "parser.y"
                         {
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr((yyvsp[0].stmt));
        (yyvsp[-1].block_item_list)->push_back(std::move(item));
        (yyval.block_item_list) = (yyvsp[-1].block_item_list);
        (yyvsp[0].stmt) = nullptr;
    }
#line 1730 "parser.tab.c"
    break;

  case 38: /* Stmt: AssignStmt  */
#line 417 "parser.y"
               { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1736 "parser.tab.c"
    break;

  case 39: /* Stmt: ExpStmt  */
#line 418 "parser.y"
              { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1742 "parser.tab.c"
    break;

  case 40: /* Stmt: Block  */
#line 419 "parser.y"
            { (yyval.stmt) = (yyvsp[0].block); }
#line 1748 "parser.tab.c"
    break;

  case 41: /* Stmt: IfStmt  */
#line 420 "parser.y"
             { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1754 "parser.tab.c"
    break;

  case 42: /* Stmt: WhileStmt  */
#line 421 "parser.y"
                { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1760 "parser.tab.c"
    break;

  case 43: /* Stmt: BreakStmt  */
#line 422 "parser.y"
                { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1766 "parser.tab.c"
    break;

  case 44: /* Stmt: ContinueStmt  */
#line 423 "parser.y"
                   { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1772 "parser.tab.c"
    break;

  case 45: /* Stmt: ReturnStmt  */
#line 424 "parser.y"
                 { (yyval.stmt) = (yyvsp[0].stmt); }
#line 1778 "parser.tab.c"
    break;

  case 46: /* AssignStmt: LVal ASSIGN Exp SEMI  */
#line 429 "parser.y"
                         {
        (yyval.stmt) = new AssignStmt(make_unique_from_ptr((yyvsp[-3].lval)), make_unique_from_ptr((yyvsp[-1].exp)), {line_number});
        (yyvsp[-3].lval) = nullptr; 
        (yyvsp[-1].exp) = nullptr; 
    }
#line 1788 "parser.tab.c"
    break;

  case 47: /* ExpStmt: SEMI  */
#line 438 "parser.y"
         {
        std::optional<std::unique_ptr<Exp>> expr;
        (yyval.stmt) = new ExpStmt(std::move(expr), {line_number});
    }
#line 1797 "parser.tab.c"
    break;

  case 48: /* ExpStmt: Exp SEMI  */
#line 442 "parser.y"
               {
        std::optional<std::unique_ptr<Exp>> expr = make_unique_from_ptr((yyvsp[-1].exp));
        (yyval.stmt) = new ExpStmt(std::move(expr), {line_number});
        (yyvsp[-1].exp) = nullptr; 
    }
#line 1807 "parser.tab.c"
    break;

  case 49: /* IfStmt: IF LPAREN Cond RPAREN Stmt  */
#line 451 "parser.y"
                                                     {
        std::optional<std::unique_ptr<Stmt>> else_stmt;
        (yyval.stmt) = new IfStmt(make_unique_from_ptr((yyvsp[-2].exp)), make_unique_from_ptr((yyvsp[0].stmt)), 
                       std::move(else_stmt), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].stmt) = nullptr; 
    }
#line 1819 "parser.tab.c"
    break;

  case 50: /* IfStmt: IF LPAREN Cond RPAREN Stmt ELSE Stmt  */
#line 458 "parser.y"
                                           {
        std::optional<std::unique_ptr<Stmt>> else_stmt = make_unique_from_ptr((yyvsp[0].stmt));
        (yyval.stmt) = new IfStmt(make_unique_from_ptr((yyvsp[-4].exp)), make_unique_from_ptr((yyvsp[-2].stmt)), 
                       std::move(else_stmt), {line_number});
        (yyvsp[-4].exp) = nullptr; 
        (yyvsp[-2].stmt) = nullptr; 
        (yyvsp[0].stmt) = nullptr; 
    }
#line 1832 "parser.tab.c"
    break;

  case 51: /* WhileStmt: WHILE LPAREN Cond RPAREN Stmt  */
#line 470 "parser.y"
                                  {
        (yyval.stmt) = new WhileStmt(make_unique_from_ptr((yyvsp[-2].exp)), make_unique_from_ptr((yyvsp[0].stmt)), {line_number});
        (yyvsp[-2].exp) = nullptr;
        (yyvsp[0].stmt) = nullptr;  
    }
#line 1842 "parser.tab.c"
    break;

  case 52: /* BreakStmt: BREAK SEMI  */
#line 479 "parser.y"
               {
        (yyval.stmt) = new BreakStmt({line_number});
    }
#line 1850 "parser.tab.c"
    break;

  case 53: /* ContinueStmt: CONTINUE SEMI  */
#line 486 "parser.y"
                  {
        (yyval.stmt) = new ContinueStmt({line_number});
    }
#line 1858 "parser.tab.c"
    break;

  case 54: /* ReturnStmt: RETURN SEMI  */
#line 493 "parser.y"
                {
        std::optional<std::unique_ptr<Exp>> expr;
        (yyval.stmt) = new ReturnStmt(std::move(expr), {line_number});
    }
#line 1867 "parser.tab.c"
    break;

  case 55: /* ReturnStmt: RETURN Exp SEMI  */
#line 497 "parser.y"
                      {
        std::optional<std::unique_ptr<Exp>> expr = make_unique_from_ptr((yyvsp[-1].exp));
        (yyval.stmt) = new ReturnStmt(std::move(expr), {line_number});
        (yyvsp[-1].exp) = nullptr; 
    }
#line 1877 "parser.tab.c"
    break;

  case 56: /* Exp: AddExp  */
#line 506 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 1883 "parser.tab.c"
    break;

  case 57: /* Cond: LOrExp  */
#line 511 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 1889 "parser.tab.c"
    break;

  case 58: /* LOrExp: LAndExp  */
#line 516 "parser.y"
            { (yyval.exp) = (yyvsp[0].exp); }
#line 1895 "parser.tab.c"
    break;

  case 59: /* LOrExp: LOrExp OR LAndExp  */
#line 517 "parser.y"
                        {
        (yyval.exp) = new BinaryExp(BinaryOp::OR, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1906 "parser.tab.c"
    break;

  case 60: /* LAndExp: EqExp  */
#line 527 "parser.y"
          { (yyval.exp) = (yyvsp[0].exp); }
#line 1912 "parser.tab.c"
    break;

  case 61: /* LAndExp: LAndExp AND EqExp  */
#line 528 "parser.y"
                        {
        (yyval.exp) = new BinaryExp(BinaryOp::AND, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1923 "parser.tab.c"
    break;

  case 62: /* EqExp: RelExp  */
#line 538 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 1929 "parser.tab.c"
    break;

  case 63: /* EqExp: EqExp EQ RelExp  */
#line 539 "parser.y"
                      {
        (yyval.exp) = new BinaryExp(BinaryOp::EQ, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1940 "parser.tab.c"
    break;

  case 64: /* EqExp: EqExp NE RelExp  */
#line 545 "parser.y"
                      {
        (yyval.exp) = new BinaryExp(BinaryOp::NEQ, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1951 "parser.tab.c"
    break;

  case 65: /* RelExp: AddExp  */
#line 555 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 1957 "parser.tab.c"
    break;

  case 66: /* RelExp: RelExp LT AddExp  */
#line 556 "parser.y"
                       {
        (yyval.exp) = new BinaryExp(BinaryOp::LT, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1968 "parser.tab.c"
    break;

  case 67: /* RelExp: RelExp GT AddExp  */
#line 562 "parser.y"
                       {
        (yyval.exp) = new BinaryExp(BinaryOp::GT, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1979 "parser.tab.c"
    break;

  case 68: /* RelExp: RelExp LEQ AddExp  */
#line 568 "parser.y"
                        {
        (yyval.exp) = new BinaryExp(BinaryOp::LTE, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 1990 "parser.tab.c"
    break;

  case 69: /* RelExp: RelExp GEQ AddExp  */
#line 574 "parser.y"
                        {
        (yyval.exp) = new BinaryExp(BinaryOp::GTE, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2001 "parser.tab.c"
    break;

  case 70: /* AddExp: MulExp  */
#line 584 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 2007 "parser.tab.c"
    break;

  case 71: /* AddExp: AddExp PLUS MulExp  */
#line 585 "parser.y"
                         {
        (yyval.exp) = new BinaryExp(BinaryOp::ADD, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2018 "parser.tab.c"
    break;

  case 72: /* AddExp: AddExp MINUS MulExp  */
#line 591 "parser.y"
                          {
        (yyval.exp) = new BinaryExp(BinaryOp::SUB, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2029 "parser.tab.c"
    break;

  case 73: /* MulExp: UnaryExp  */
#line 601 "parser.y"
             { (yyval.exp) = (yyvsp[0].exp); }
#line 2035 "parser.tab.c"
    break;

  case 74: /* MulExp: MulExp MUL UnaryExp  */
#line 602 "parser.y"
                          {
        (yyval.exp) = new BinaryExp(BinaryOp::MUL, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2046 "parser.tab.c"
    break;

  case 75: /* MulExp: MulExp DIV UnaryExp  */
#line 608 "parser.y"
                          {
        (yyval.exp) = new BinaryExp(BinaryOp::DIV, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2057 "parser.tab.c"
    break;

  case 76: /* MulExp: MulExp MOD UnaryExp  */
#line 614 "parser.y"
                          {
        (yyval.exp) = new BinaryExp(BinaryOp::MOD, make_unique_from_ptr((yyvsp[-2].exp)), 
                          make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[-2].exp) = nullptr; 
        (yyvsp[0].exp) = nullptr; 
    }
#line 2068 "parser.tab.c"
    break;

  case 77: /* UnaryExp: PrimaryExp  */
#line 624 "parser.y"
               { (yyval.exp) = (yyvsp[0].exp); }
#line 2074 "parser.tab.c"
    break;

  case 78: /* UnaryExp: PLUS UnaryExp  */
#line 625 "parser.y"
                                {
        (yyval.exp) = new UnaryExp(UnaryOp::PLUS, make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[0].exp) = nullptr; 
    }
#line 2083 "parser.tab.c"
    break;

  case 79: /* UnaryExp: MINUS UnaryExp  */
#line 629 "parser.y"
                                  {
        (yyval.exp) = new UnaryExp(UnaryOp::MINUS, make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[0].exp) = nullptr; 
    }
#line 2092 "parser.tab.c"
    break;

  case 80: /* UnaryExp: NOT UnaryExp  */
#line 633 "parser.y"
                              {
        (yyval.exp) = new UnaryExp(UnaryOp::NOT, make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[0].exp) = nullptr; 
    }
#line 2101 "parser.tab.c"
    break;

  case 81: /* FunctionCall: IDENT LPAREN RPAREN  */
#line 641 "parser.y"
                        {
        std::vector<std::unique_ptr<Exp>> args;
        (yyval.exp) = new FunctionCall(std::string((yyvsp[-2].str_val)), std::move(args), {line_number});
        free((yyvsp[-2].str_val));
    }
#line 2111 "parser.tab.c"
    break;

  case 82: /* FunctionCall: IDENT LPAREN FuncRParams RPAREN  */
#line 646 "parser.y"
                                      {
        std::vector<std::unique_ptr<Exp>> args;
        for (auto& arg : *(yyvsp[-1].exp_list)) {
            args.push_back(std::move(arg));
        }
        delete (yyvsp[-1].exp_list); 
        (yyvsp[-1].exp_list) = nullptr;
        (yyval.exp) = new FunctionCall(std::string((yyvsp[-3].str_val)), std::move(args), {line_number});
        free((yyvsp[-3].str_val));
    }
#line 2126 "parser.tab.c"
    break;

  case 83: /* FuncRParams: Exp  */
#line 660 "parser.y"
        {
        (yyval.exp_list) = new std::vector<std::unique_ptr<Exp>>();
        (yyval.exp_list)->push_back(make_unique_from_ptr((yyvsp[0].exp)));
        (yyvsp[0].exp) = nullptr;
    }
#line 2136 "parser.tab.c"
    break;

  case 84: /* FuncRParams: FuncRParams COMMA Exp  */
#line 665 "parser.y"
                            {
        (yyvsp[-2].exp_list)->push_back(make_unique_from_ptr((yyvsp[0].exp)));
        (yyval.exp_list) = (yyvsp[-2].exp_list);
        (yyvsp[0].exp) = nullptr;
    }
#line 2146 "parser.tab.c"
    break;

  case 85: /* PrimaryExp: LPAREN Exp RPAREN  */
#line 674 "parser.y"
                      { 
        (yyval.exp) = (yyvsp[-1].exp);
        (yyvsp[-1].exp) = nullptr;
    }
#line 2155 "parser.tab.c"
    break;

  case 86: /* PrimaryExp: LVal  */
#line 678 "parser.y"
           { (yyval.exp) = (yyvsp[0].lval); }
#line 2161 "parser.tab.c"
    break;

  case 87: /* PrimaryExp: Number  */
#line 679 "parser.y"
             { (yyval.exp) = (yyvsp[0].exp); }
#line 2167 "parser.tab.c"
    break;

  case 88: /* PrimaryExp: StringLiteral  */
#line 680 "parser.y"
                    { (yyval.exp) = (yyvsp[0].exp); }
#line 2173 "parser.tab.c"
    break;

  case 89: /* PrimaryExp: FunctionCall  */
#line 681 "parser.y"
                   { (yyval.exp) = (yyvsp[0].exp); }
#line 2179 "parser.tab.c"
    break;

  case 90: /* LVal: IDENT  */
#line 686 "parser.y"
          {
        std::vector<std::unique_ptr<Exp>> indices;
        (yyval.lval) = new LVal(std::string((yyvsp[0].str_val)), std::move(indices), {line_number});
        free((yyvsp[0].str_val));
    }
#line 2189 "parser.tab.c"
    break;

  case 91: /* LVal: IDENT ArrayIndices  */
#line 691 "parser.y"
                         {
        (yyval.lval) = new LVal(std::string((yyvsp[-1].str_val)), std::move(*(yyvsp[0].exp_list)), {line_number});
        free((yyvsp[-1].str_val));
        delete (yyvsp[0].exp_list);
        (yyvsp[0].exp_list) = nullptr; 
    }
#line 2200 "parser.tab.c"
    break;

  case 92: /* ArrayIndices: LBRACKET Exp RBRACKET  */
#line 700 "parser.y"
                          {
        (yyval.exp_list) = new std::vector<std::unique_ptr<Exp>>();
        (yyval.exp_list)->push_back(make_unique_from_ptr((yyvsp[-1].exp)));
        (yyvsp[-1].exp) = nullptr; 
    }
#line 2210 "parser.tab.c"
    break;

  case 93: /* ArrayIndices: ArrayIndices LBRACKET Exp RBRACKET  */
#line 705 "parser.y"
                                         {
        (yyvsp[-3].exp_list)->push_back(make_unique_from_ptr((yyvsp[-1].exp)));
        (yyval.exp_list) = (yyvsp[-3].exp_list);
        (yyvsp[-1].exp) = nullptr;
    }
#line 2220 "parser.tab.c"
    break;

  case 94: /* Number: INT_CONST  */
#line 714 "parser.y"
              {
        (yyval.exp) = new Number((yyvsp[0].int_val), {line_number});
    }
#line 2228 "parser.tab.c"
    break;

  case 95: /* Number: FLOAT_CONST  */
#line 717 "parser.y"
                  {
        (yyval.exp) = new Number((yyvsp[0].float_val), {line_number});
    }
#line 2236 "parser.tab.c"
    break;

  case 96: /* StringLiteral: STR_CONST  */
#line 724 "parser.y"
              {
        (yyval.exp) = new StringLiteral(std::string((yyvsp[0].str_val)), {line_number});
        free((yyvsp[0].str_val));
    }
#line 2245 "parser.tab.c"
    break;

  case 97: /* ConstExp: AddExp  */
#line 732 "parser.y"
           { (yyval.exp) = (yyvsp[0].exp); }
#line 2251 "parser.tab.c"
    break;

  case 98: /* InitVal: Exp  */
#line 737 "parser.y"
        {
        (yyval.init_val) = new InitVal(make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[0].exp) = nullptr;
    }
#line 2260 "parser.tab.c"
    break;

  case 99: /* InitVal: LBRACE RBRACE  */
#line 741 "parser.y"
                    {
        std::vector<std::unique_ptr<InitVal>> inits;
        (yyval.init_val) = new InitVal(std::move(inits), {line_number});
    }
#line 2269 "parser.tab.c"
    break;

  case 100: /* InitVal: LBRACE InitValList RBRACE  */
#line 745 "parser.y"
                                {
        std::vector<std::unique_ptr<InitVal>> inits;
        for (auto& init : *(yyvsp[-1].init_list)) {
            inits.push_back(std::move(init));
        }
        delete (yyvsp[-1].init_list);
        (yyvsp[-1].init_list) = nullptr;
        (yyval.init_val) = new InitVal(std::move(inits), {line_number});
    }
#line 2283 "parser.tab.c"
    break;

  case 101: /* InitValList: InitVal  */
#line 757 "parser.y"
            {
        (yyval.init_list) = new std::vector<std::unique_ptr<InitVal>>();
        (yyval.init_list)->push_back(make_unique_from_ptr((yyvsp[0].init_val)));
        (yyvsp[0].init_val) = nullptr; 
    }
#line 2293 "parser.tab.c"
    break;

  case 102: /* InitValList: InitValList COMMA InitVal  */
#line 762 "parser.y"
                                {
        (yyvsp[-2].init_list)->push_back(make_unique_from_ptr((yyvsp[0].init_val)));
        (yyval.init_list) = (yyvsp[-2].init_list);
        (yyvsp[0].init_val) = nullptr;
    }
#line 2303 "parser.tab.c"
    break;

  case 103: /* ConstInitVal: ConstExp  */
#line 771 "parser.y"
             {
        (yyval.const_init_val) = new ConstInitVal(make_unique_from_ptr((yyvsp[0].exp)), {line_number});
        (yyvsp[0].exp) = nullptr;
    }
#line 2312 "parser.tab.c"
    break;

  case 104: /* ConstInitVal: LBRACE RBRACE  */
#line 775 "parser.y"
                    {
        std::vector<std::unique_ptr<ConstInitVal>> inits;
        (yyval.const_init_val) = new ConstInitVal(std::move(inits), {line_number});
    }
#line 2321 "parser.tab.c"
    break;

  case 105: /* ConstInitVal: LBRACE ConstInitValList RBRACE  */
#line 779 "parser.y"
                                     {
        std::vector<std::unique_ptr<ConstInitVal>> inits;
        for (auto& init : *(yyvsp[-1].const_init_list)) {
            inits.push_back(std::move(init));
        }
        delete (yyvsp[-1].const_init_list);
        (yyvsp[-1].const_init_list) = nullptr;
        (yyval.const_init_val) = new ConstInitVal(std::move(inits), {line_number});
    }
#line 2335 "parser.tab.c"
    break;

  case 106: /* ConstInitValList: ConstInitVal  */
#line 791 "parser.y"
                 {
        (yyval.const_init_list) = new std::vector<std::unique_ptr<ConstInitVal>>();
        (yyval.const_init_list)->push_back(make_unique_from_ptr((yyvsp[0].const_init_val)));
        (yyvsp[0].const_init_val) = nullptr;
    }
#line 2345 "parser.tab.c"
    break;

  case 107: /* ConstInitValList: ConstInitValList COMMA ConstInitVal  */
#line 796 "parser.y"
                                          {
        (yyvsp[-2].const_init_list)->push_back(make_unique_from_ptr((yyvsp[0].const_init_val)));
        (yyval.const_init_list) = (yyvsp[-2].const_init_list);
        (yyvsp[0].const_init_val) = nullptr;
    }
#line 2355 "parser.tab.c"
    break;

  case 108: /* BType: INT  */
#line 805 "parser.y"
        { (yyval.base_type) = BaseType::INT; }
#line 2361 "parser.tab.c"
    break;

  case 109: /* BType: FLOAT  */
#line 806 "parser.y"
            { (yyval.base_type) = BaseType::FLOAT; }
#line 2367 "parser.tab.c"
    break;


#line 2371 "parser.tab.c"

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
      yyerror (YY_("syntax error"));
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
                      yytoken, &yylval);
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


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 809 "parser.y"


void yyerror(const char *s) {
    fprintf(stderr, "Parse error at line %d: %s\n", line_number, s);
}
