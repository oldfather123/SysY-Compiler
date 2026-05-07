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
#line 2 "syntax_analyzer.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../include/common/syntax_tree.h"
#include "syntax_analyzer.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart(FILE *input_file);
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);

#line 102 "syntax_analyzer.c"

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

#include "syntax_analyzer.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ERROR = 3,                      /* ERROR  */
  YYSYMBOL_ADD = 4,                        /* ADD  */
  YYSYMBOL_SUB = 5,                        /* SUB  */
  YYSYMBOL_MUL = 6,                        /* MUL  */
  YYSYMBOL_DIV = 7,                        /* DIV  */
  YYSYMBOL_MOD = 8,                        /* MOD  */
  YYSYMBOL_LT = 9,                         /* LT  */
  YYSYMBOL_LET = 10,                       /* LET  */
  YYSYMBOL_GT = 11,                        /* GT  */
  YYSYMBOL_GET = 12,                       /* GET  */
  YYSYMBOL_EQ = 13,                        /* EQ  */
  YYSYMBOL_NEQ = 14,                       /* NEQ  */
  YYSYMBOL_ASSIGN = 15,                    /* ASSIGN  */
  YYSYMBOL_COMMA = 16,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 17,                 /* SEMICOLON  */
  YYSYMBOL_NOT = 18,                       /* NOT  */
  YYSYMBOL_LPARENTHESIS = 19,              /* LPARENTHESIS  */
  YYSYMBOL_RPARENTHESIS = 20,              /* RPARENTHESIS  */
  YYSYMBOL_LBRACKET = 21,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 22,                  /* RBRACKET  */
  YYSYMBOL_LBRACE = 23,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 24,                    /* RBRACE  */
  YYSYMBOL_AND = 25,                       /* AND  */
  YYSYMBOL_OR = 26,                        /* OR  */
  YYSYMBOL_IF = 27,                        /* IF  */
  YYSYMBOL_ELSE = 28,                      /* ELSE  */
  YYSYMBOL_WHILE = 29,                     /* WHILE  */
  YYSYMBOL_BREAK = 30,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 31,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 32,                    /* RETURN  */
  YYSYMBOL_VOID = 33,                      /* VOID  */
  YYSYMBOL_INT = 34,                       /* INT  */
  YYSYMBOL_FLOAT = 35,                     /* FLOAT  */
  YYSYMBOL_CONST = 36,                     /* CONST  */
  YYSYMBOL_IDENT = 37,                     /* IDENT  */
  YYSYMBOL_INTCONST = 38,                  /* INTCONST  */
  YYSYMBOL_FLOATCONST = 39,                /* FLOATCONST  */
  YYSYMBOL_YYACCEPT = 40,                  /* $accept  */
  YYSYMBOL_Program = 41,                   /* Program  */
  YYSYMBOL_CompUnit = 42,                  /* CompUnit  */
  YYSYMBOL_Decl = 43,                      /* Decl  */
  YYSYMBOL_ConstDecl = 44,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 45,              /* ConstDefList  */
  YYSYMBOL_BType = 46,                     /* BType  */
  YYSYMBOL_ConstDef = 47,                  /* ConstDef  */
  YYSYMBOL_ConstExpList = 48,              /* ConstExpList  */
  YYSYMBOL_ConstInitVal = 49,              /* ConstInitVal  */
  YYSYMBOL_ConstInitValList = 50,          /* ConstInitValList  */
  YYSYMBOL_VarDecl = 51,                   /* VarDecl  */
  YYSYMBOL_VarDeclList = 52,               /* VarDeclList  */
  YYSYMBOL_VarDef = 53,                    /* VarDef  */
  YYSYMBOL_InitVal = 54,                   /* InitVal  */
  YYSYMBOL_InitValList = 55,               /* InitValList  */
  YYSYMBOL_FuncDef = 56,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 57,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 58,                /* FuncFParam  */
  YYSYMBOL_ExpList = 59,                   /* ExpList  */
  YYSYMBOL_Block = 60,                     /* Block  */
  YYSYMBOL_BlockList = 61,                 /* BlockList  */
  YYSYMBOL_BlockItem = 62,                 /* BlockItem  */
  YYSYMBOL_Stmt = 63,                      /* Stmt  */
  YYSYMBOL_Exp = 64,                       /* Exp  */
  YYSYMBOL_Cond = 65,                      /* Cond  */
  YYSYMBOL_LVal = 66,                      /* LVal  */
  YYSYMBOL_PrimaryExp = 67,                /* PrimaryExp  */
  YYSYMBOL_Number = 68,                    /* Number  */
  YYSYMBOL_UnaryExp = 69,                  /* UnaryExp  */
  YYSYMBOL_UnaryOp = 70,                   /* UnaryOp  */
  YYSYMBOL_FuncRParams = 71,               /* FuncRParams  */
  YYSYMBOL_MulExp = 72,                    /* MulExp  */
  YYSYMBOL_AddExp = 73,                    /* AddExp  */
  YYSYMBOL_RelExp = 74,                    /* RelExp  */
  YYSYMBOL_EqExp = 75,                     /* EqExp  */
  YYSYMBOL_LAndExp = 76,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 77,                    /* LOrExp  */
  YYSYMBOL_ConstExp = 78,                  /* ConstExp  */
  YYSYMBOL_Integer = 79,                   /* Integer  */
  YYSYMBOL_Floatnum = 80                   /* Floatnum  */
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
#define YYLAST   225

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  97
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  175

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   312


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
       2,     2,     2,     2,     2,     2,     1,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      17,     2,    19,    20,    21,    22,    23,    24,     2,     2,
       2,    32,    33,     2,    37,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      18,    25,    26,    27,    28,    29,    30,    31,    34,    35,
      36,    38,    39
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   117,   117,   121,   122,   123,   124,   128,   129,   133,
     136,   137,   141,   142,   146,   149,   150,   154,   155,   156,
     159,   160,   164,   167,   168,   172,   173,   177,   178,   179,
     182,   183,   187,   188,   189,   190,   191,   201,   202,   206,
     207,   210,   211,   215,   218,   219,   223,   224,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   242,
     246,   250,   254,   255,   256,   260,   261,   265,   266,   267,
     268,   272,   273,   274,   278,   279,   283,   284,   285,   286,
     290,   291,   292,   296,   297,   298,   299,   300,   304,   305,
     306,   310,   311,   315,   316,   320,   323,   326
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
  "\"end of file\"", "error", "\"invalid token\"", "ERROR", "ADD", "SUB",
  "MUL", "DIV", "MOD", "LT", "LET", "GT", "GET", "EQ", "NEQ", "ASSIGN",
  "COMMA", "SEMICOLON", "NOT", "LPARENTHESIS", "RPARENTHESIS", "LBRACKET",
  "RBRACKET", "LBRACE", "RBRACE", "AND", "OR", "IF", "ELSE", "WHILE",
  "BREAK", "CONTINUE", "RETURN", "VOID", "INT", "FLOAT", "CONST", "IDENT",
  "INTCONST", "FLOATCONST", "$accept", "Program", "CompUnit", "Decl",
  "ConstDecl", "ConstDefList", "BType", "ConstDef", "ConstExpList",
  "ConstInitVal", "ConstInitValList", "VarDecl", "VarDeclList", "VarDef",
  "InitVal", "InitValList", "FuncDef", "FuncFParams", "FuncFParam",
  "ExpList", "Block", "BlockList", "BlockItem", "Stmt", "Exp", "Cond",
  "LVal", "PrimaryExp", "Number", "UnaryExp", "UnaryOp", "FuncRParams",
  "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", "ConstExp",
  "Integer", "Floatnum", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-97)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      66,   -16,   -97,   -97,   -11,    33,    66,   -97,   -97,    -1,
     -97,   -97,    60,    23,   -97,   -97,   -97,    65,   -97,   172,
     -97,    52,   -97,    41,    -4,   126,    48,    61,    53,    42,
     -97,    36,    23,   -97,    48,    43,    11,   152,    56,   -97,
     -97,   -97,    48,    75,   -11,    48,    68,   -97,   -97,    48,
     -97,   -97,   -97,   152,    90,    91,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,   152,   125,   145,   -97,   -97,   145,
      93,   -97,   -97,     8,   -97,   113,   -97,   -97,   136,   -97,
     -97,   -97,   133,   -97,   -97,    -6,   143,   137,   -97,   152,
     152,   152,   152,   152,   -97,   -97,   -97,   150,   159,   155,
     166,   147,   -97,    56,   -97,   -97,   -97,   171,   178,   -97,
     -97,   -97,    -2,   -97,    11,   -97,   -97,   -97,    62,   152,
     -97,   -97,   -97,   125,   125,   152,   152,   -97,   -97,   -97,
     177,   -97,   152,   137,    68,   -97,   -97,   152,   -97,   179,
     180,   145,   199,   154,   174,   186,   183,   -97,   187,   -97,
     -97,   -97,   107,   152,   152,   152,   152,   152,   152,   152,
     152,   107,   -97,   185,   145,   145,   145,   145,   199,   199,
     154,   174,   -97,   107,   -97
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,    12,    13,     0,     0,     2,     5,     7,     0,
       8,     6,     0,     0,     1,     3,     4,    15,    23,     0,
      15,     0,    10,     0,    25,     0,     0,     0,     0,     0,
      37,     0,     0,     9,     0,     0,     0,     0,     0,    22,
      44,    33,     0,    39,     0,     0,     0,    11,    32,     0,
      71,    72,    73,     0,     0,    41,    96,    97,    26,    27,
      63,    67,    64,    76,     0,    80,    59,    65,    66,    95,
       0,    15,    24,     0,    34,     0,    38,    35,     0,    14,
      17,    36,     0,    29,    30,     0,     0,    61,    70,     0,
       0,     0,     0,     0,    16,    50,    43,     0,     0,     0,
       0,     0,    46,     0,    51,    45,    47,     0,    63,    41,
      19,    20,     0,    62,     0,    28,    69,    74,     0,     0,
      77,    78,    79,    81,    82,     0,     0,    55,    56,    58,
       0,    49,     0,    40,     0,    18,    31,     0,    68,     0,
       0,    83,    88,    91,    93,    60,     0,    57,     0,    21,
      75,    42,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    48,    52,    84,    85,    86,    87,    89,    90,
      92,    94,    54,     0,    53
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -97,   -97,   -97,    -3,   -97,   -97,     1,   182,   195,   -70,
     -97,   -97,   -97,   181,   -48,   -97,   210,   194,   176,   109,
     153,   -97,   -97,   -96,   -34,    95,   -69,   -97,   -97,   -36,
     -97,   -97,    84,   -37,    39,    63,    64,   -97,   188,   -97,
     -97
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,     7,     8,    21,    28,    22,    24,    79,
     112,    10,    25,    18,    58,    85,    11,    29,    30,    87,
     104,    73,   105,   106,   107,   140,    60,    61,    62,    63,
      64,   118,    65,    66,   142,   143,   144,   145,    80,    67,
      68
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      69,     9,    59,    15,   108,    13,    84,     9,   111,    69,
     114,    36,    50,    51,   134,    50,    51,    37,   115,    82,
      59,    12,   135,     2,     3,    95,    52,    53,    88,    52,
      53,    40,    96,    14,    54,    97,    17,    98,    99,   100,
     101,    69,     2,     3,     4,    55,    56,    57,    55,    56,
      57,    46,   117,   120,   121,   122,   163,    37,    44,    44,
      20,    34,    45,    49,   149,   172,   136,   130,    32,    33,
     102,    40,    50,    51,   103,     2,     3,   174,   137,    19,
      59,    42,   138,   108,    23,   139,    52,    53,   141,   141,
      43,    78,   108,    71,    50,    51,    75,    69,   148,     1,
       2,     3,     4,   150,   108,    55,    56,    57,    52,    53,
      86,    50,    51,    54,    83,    94,   164,   165,   166,   167,
     141,   141,   141,   141,    95,    52,    53,    55,    56,    57,
      40,    89,    90,    91,    97,   109,    98,    99,   100,   101,
      50,    51,    38,    39,    55,    56,    57,    50,    51,    92,
      93,    50,    51,   113,    52,    53,    50,    51,   119,    78,
     110,    52,    53,   116,   129,    52,    53,   157,   158,   125,
      52,    53,   127,    55,    56,    57,   123,   124,   126,    41,
      55,    56,    57,   128,    55,    56,    57,    48,   131,    55,
      56,    57,    26,   132,   147,    74,   168,   169,    77,   159,
     152,   151,    81,   161,   162,    27,     2,     3,   153,   154,
     155,   156,   160,   173,    47,    31,    16,    35,   133,    72,
      76,   146,   170,     0,   171,    70
};

static const yytype_int16 yycheck[] =
{
      37,     0,    36,     6,    73,     4,    54,     6,    78,    46,
      16,    15,     4,     5,    16,     4,     5,    21,    24,    53,
      54,    37,    24,    34,    35,    17,    18,    19,    64,    18,
      19,    23,    24,     0,    23,    27,    37,    29,    30,    31,
      32,    78,    34,    35,    36,    37,    38,    39,    37,    38,
      39,    15,    86,    89,    90,    91,   152,    21,    16,    16,
      37,    20,    20,    20,   134,   161,   114,   101,    16,    17,
      73,    23,     4,     5,    73,    34,    35,   173,    16,    19,
     114,    20,    20,   152,    19,   119,    18,    19,   125,   126,
      37,    23,   161,    37,     4,     5,    21,   134,   132,    33,
      34,    35,    36,   137,   173,    37,    38,    39,    18,    19,
      19,     4,     5,    23,    24,    22,   153,   154,   155,   156,
     157,   158,   159,   160,    17,    18,    19,    37,    38,    39,
      23,     6,     7,     8,    27,    22,    29,    30,    31,    32,
       4,     5,    16,    17,    37,    38,    39,     4,     5,     4,
       5,     4,     5,    20,    18,    19,     4,     5,    21,    23,
      24,    18,    19,    20,    17,    18,    19,    13,    14,    19,
      18,    19,    17,    37,    38,    39,    92,    93,    19,    26,
      37,    38,    39,    17,    37,    38,    39,    34,    17,    37,
      38,    39,    20,    15,    17,    42,   157,   158,    45,    25,
      20,    22,    49,    20,    17,    33,    34,    35,     9,    10,
      11,    12,    26,    28,    32,    20,     6,    23,   109,    38,
      44,   126,   159,    -1,   160,    37
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    33,    34,    35,    36,    41,    42,    43,    44,    46,
      51,    56,    37,    46,     0,    43,    56,    37,    53,    19,
      37,    45,    47,    19,    48,    52,    20,    33,    46,    57,
      58,    48,    16,    17,    20,    57,    15,    21,    16,    17,
      23,    60,    20,    37,    16,    20,    15,    47,    60,    20,
       4,     5,    18,    19,    23,    37,    38,    39,    54,    64,
      66,    67,    68,    69,    70,    72,    73,    79,    80,    73,
      78,    37,    53,    61,    60,    21,    58,    60,    23,    49,
      78,    60,    64,    24,    54,    55,    19,    59,    69,     6,
       7,     8,     4,     5,    22,    17,    24,    27,    29,    30,
      31,    32,    43,    46,    60,    62,    63,    64,    66,    22,
      24,    49,    50,    20,    16,    24,    20,    64,    71,    21,
      69,    69,    69,    72,    72,    19,    19,    17,    17,    17,
      64,    17,    15,    59,    16,    24,    54,    16,    20,    64,
      65,    73,    74,    75,    76,    77,    65,    17,    64,    49,
      64,    22,    20,     9,    10,    11,    12,    13,    14,    25,
      26,    20,    17,    63,    73,    73,    73,    73,    74,    74,
      75,    76,    63,    28,    63
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    40,    41,    42,    42,    42,    42,    43,    43,    44,
      45,    45,    46,    46,    47,    48,    48,    49,    49,    49,
      50,    50,    51,    52,    52,    53,    53,    54,    54,    54,
      55,    55,    56,    56,    56,    56,    56,    57,    57,    58,
      58,    59,    59,    60,    61,    61,    62,    62,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    64,
      65,    66,    67,    67,    67,    68,    68,    69,    69,    69,
      69,    70,    70,    70,    71,    71,    72,    72,    72,    72,
      73,    73,    73,    74,    74,    74,    74,    74,    75,    75,
      75,    76,    76,    77,    77,    78,    79,    80
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     1,     1,     4,
       1,     3,     1,     1,     4,     0,     4,     1,     3,     2,
       1,     3,     4,     0,     3,     2,     4,     1,     3,     2,
       1,     3,     5,     5,     6,     6,     6,     1,     3,     2,
       5,     0,     4,     3,     0,     2,     1,     1,     4,     2,
       1,     1,     5,     7,     5,     2,     2,     3,     2,     1,
       1,     2,     3,     1,     1,     1,     1,     1,     4,     3,
       2,     1,     1,     1,     1,     3,     1,     3,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     1,     3,     1,     1,     1
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
  case 2: /* Program: CompUnit  */
#line 117 "syntax_analyzer.y"
                   {(yyval.node) = node("program", 1, (yyvsp[0].node)); gt->root = (yyval.node);}
#line 1302 "syntax_analyzer.c"
    break;

  case 3: /* CompUnit: CompUnit Decl  */
#line 121 "syntax_analyzer.y"
                {(yyval.node) = node("CompUnit", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1308 "syntax_analyzer.c"
    break;

  case 4: /* CompUnit: CompUnit FuncDef  */
#line 122 "syntax_analyzer.y"
                    {(yyval.node) = node("CompUnit", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1314 "syntax_analyzer.c"
    break;

  case 5: /* CompUnit: Decl  */
#line 123 "syntax_analyzer.y"
       {(yyval.node) = node("CompUnit", 1, (yyvsp[0].node));}
#line 1320 "syntax_analyzer.c"
    break;

  case 6: /* CompUnit: FuncDef  */
#line 124 "syntax_analyzer.y"
          {(yyval.node) = node("CompUnit", 1, (yyvsp[0].node));}
#line 1326 "syntax_analyzer.c"
    break;

  case 7: /* Decl: ConstDecl  */
#line 128 "syntax_analyzer.y"
            {(yyval.node) = node("Decl", 1, (yyvsp[0].node));}
#line 1332 "syntax_analyzer.c"
    break;

  case 8: /* Decl: VarDecl  */
#line 129 "syntax_analyzer.y"
            {(yyval.node) = node("Decl", 1, (yyvsp[0].node));}
#line 1338 "syntax_analyzer.c"
    break;

  case 9: /* ConstDecl: CONST BType ConstDefList SEMICOLON  */
#line 133 "syntax_analyzer.y"
                                     {(yyval.node) = node("ConstDecl", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1344 "syntax_analyzer.c"
    break;

  case 10: /* ConstDefList: ConstDef  */
#line 136 "syntax_analyzer.y"
           {(yyval.node) = node("ConstDefList", 1, (yyvsp[0].node));}
#line 1350 "syntax_analyzer.c"
    break;

  case 11: /* ConstDefList: ConstDefList COMMA ConstDef  */
#line 137 "syntax_analyzer.y"
                              {(yyval.node) = node("ConstDefList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1356 "syntax_analyzer.c"
    break;

  case 12: /* BType: INT  */
#line 141 "syntax_analyzer.y"
      {(yyval.node) = node("BType", 1, (yyvsp[0].node));}
#line 1362 "syntax_analyzer.c"
    break;

  case 13: /* BType: FLOAT  */
#line 142 "syntax_analyzer.y"
        {(yyval.node) = node("BType", 1, (yyvsp[0].node));}
#line 1368 "syntax_analyzer.c"
    break;

  case 14: /* ConstDef: IDENT ConstExpList ASSIGN ConstInitVal  */
#line 146 "syntax_analyzer.y"
                                         {(yyval.node) = node("ConstDef", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1374 "syntax_analyzer.c"
    break;

  case 15: /* ConstExpList: %empty  */
#line 149 "syntax_analyzer.y"
   {(yyval.node) = node("ConstExpList", 0);}
#line 1380 "syntax_analyzer.c"
    break;

  case 16: /* ConstExpList: ConstExpList LBRACKET ConstExp RBRACKET  */
#line 150 "syntax_analyzer.y"
                                          {(yyval.node) = node("ConstExpList", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1386 "syntax_analyzer.c"
    break;

  case 17: /* ConstInitVal: ConstExp  */
#line 154 "syntax_analyzer.y"
            {(yyval.node) = node("ConstInitVal", 1, (yyvsp[0].node));}
#line 1392 "syntax_analyzer.c"
    break;

  case 18: /* ConstInitVal: LBRACE ConstInitValList RBRACE  */
#line 155 "syntax_analyzer.y"
                                 {(yyval.node) = node("ConstInitVal", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1398 "syntax_analyzer.c"
    break;

  case 19: /* ConstInitVal: LBRACE RBRACE  */
#line 156 "syntax_analyzer.y"
                {(yyval.node) = node("ConstInitVal", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1404 "syntax_analyzer.c"
    break;

  case 20: /* ConstInitValList: ConstInitVal  */
#line 159 "syntax_analyzer.y"
               {(yyval.node) = node("ConstInitValList", 1, (yyvsp[0].node));}
#line 1410 "syntax_analyzer.c"
    break;

  case 21: /* ConstInitValList: ConstInitValList COMMA ConstInitVal  */
#line 160 "syntax_analyzer.y"
                                      {(yyval.node) = node("ConstInitValList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1416 "syntax_analyzer.c"
    break;

  case 22: /* VarDecl: BType VarDef VarDeclList SEMICOLON  */
#line 164 "syntax_analyzer.y"
                                     {(yyval.node) = node("VarDecl", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1422 "syntax_analyzer.c"
    break;

  case 23: /* VarDeclList: %empty  */
#line 167 "syntax_analyzer.y"
  {(yyval.node) = node("VarDeclList", 0);}
#line 1428 "syntax_analyzer.c"
    break;

  case 24: /* VarDeclList: VarDeclList COMMA VarDef  */
#line 168 "syntax_analyzer.y"
                           {(yyval.node) = node("VarDeclList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1434 "syntax_analyzer.c"
    break;

  case 25: /* VarDef: IDENT ConstExpList  */
#line 172 "syntax_analyzer.y"
                     {(yyval.node) = node("VarDef", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1440 "syntax_analyzer.c"
    break;

  case 26: /* VarDef: IDENT ConstExpList ASSIGN InitVal  */
#line 173 "syntax_analyzer.y"
                                    {(yyval.node) = node("VarDef", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1446 "syntax_analyzer.c"
    break;

  case 27: /* InitVal: Exp  */
#line 177 "syntax_analyzer.y"
      {(yyval.node) = node("InitVal", 1, (yyvsp[0].node));}
#line 1452 "syntax_analyzer.c"
    break;

  case 28: /* InitVal: LBRACE InitValList RBRACE  */
#line 178 "syntax_analyzer.y"
                            {(yyval.node) = node("InitVal", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1458 "syntax_analyzer.c"
    break;

  case 29: /* InitVal: LBRACE RBRACE  */
#line 179 "syntax_analyzer.y"
                {(yyval.node) = node("InitVal", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1464 "syntax_analyzer.c"
    break;

  case 30: /* InitValList: InitVal  */
#line 182 "syntax_analyzer.y"
          {(yyval.node) = node("InitValList", 1, (yyvsp[0].node));}
#line 1470 "syntax_analyzer.c"
    break;

  case 31: /* InitValList: InitValList COMMA InitVal  */
#line 183 "syntax_analyzer.y"
                            {(yyval.node) = node("InitValList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1476 "syntax_analyzer.c"
    break;

  case 32: /* FuncDef: BType IDENT LPARENTHESIS RPARENTHESIS Block  */
#line 187 "syntax_analyzer.y"
                                              {(yyval.node) = node("FuncDef", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1482 "syntax_analyzer.c"
    break;

  case 33: /* FuncDef: VOID IDENT LPARENTHESIS RPARENTHESIS Block  */
#line 188 "syntax_analyzer.y"
                                             {(yyval.node) = node("FuncDef", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1488 "syntax_analyzer.c"
    break;

  case 34: /* FuncDef: VOID IDENT LPARENTHESIS VOID RPARENTHESIS Block  */
#line 189 "syntax_analyzer.y"
                                                  {(yyval.node) = node("FuncDef", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1494 "syntax_analyzer.c"
    break;

  case 35: /* FuncDef: VOID IDENT LPARENTHESIS FuncFParams RPARENTHESIS Block  */
#line 190 "syntax_analyzer.y"
                                                         {(yyval.node) = node("FuncDef", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1500 "syntax_analyzer.c"
    break;

  case 36: /* FuncDef: BType IDENT LPARENTHESIS FuncFParams RPARENTHESIS Block  */
#line 191 "syntax_analyzer.y"
                                                          {(yyval.node) = node("FuncDef", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1506 "syntax_analyzer.c"
    break;

  case 37: /* FuncFParams: FuncFParam  */
#line 201 "syntax_analyzer.y"
             {(yyval.node) = node("FuncFParams", 1, (yyvsp[0].node));}
#line 1512 "syntax_analyzer.c"
    break;

  case 38: /* FuncFParams: FuncFParams COMMA FuncFParam  */
#line 202 "syntax_analyzer.y"
                               {(yyval.node) = node("FuncFParams", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1518 "syntax_analyzer.c"
    break;

  case 39: /* FuncFParam: BType IDENT  */
#line 206 "syntax_analyzer.y"
              {(yyval.node) = node("FuncFParam", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1524 "syntax_analyzer.c"
    break;

  case 40: /* FuncFParam: BType IDENT LBRACKET RBRACKET ExpList  */
#line 207 "syntax_analyzer.y"
                                        {(yyval.node) = node("FuncFParam", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1530 "syntax_analyzer.c"
    break;

  case 41: /* ExpList: %empty  */
#line 210 "syntax_analyzer.y"
  {(yyval.node) = node("ExpList", 0);}
#line 1536 "syntax_analyzer.c"
    break;

  case 42: /* ExpList: ExpList LBRACKET Exp RBRACKET  */
#line 211 "syntax_analyzer.y"
                                {(yyval.node) = node("ExpList", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1542 "syntax_analyzer.c"
    break;

  case 43: /* Block: LBRACE BlockList RBRACE  */
#line 215 "syntax_analyzer.y"
                          {(yyval.node) = node("Block", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1548 "syntax_analyzer.c"
    break;

  case 44: /* BlockList: %empty  */
#line 218 "syntax_analyzer.y"
  {(yyval.node) = node("BlockList", 0);}
#line 1554 "syntax_analyzer.c"
    break;

  case 45: /* BlockList: BlockList BlockItem  */
#line 219 "syntax_analyzer.y"
                      {(yyval.node) = node("BlockList", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1560 "syntax_analyzer.c"
    break;

  case 46: /* BlockItem: Decl  */
#line 223 "syntax_analyzer.y"
       {(yyval.node) = node("BlockItem", 1, (yyvsp[0].node));}
#line 1566 "syntax_analyzer.c"
    break;

  case 47: /* BlockItem: Stmt  */
#line 224 "syntax_analyzer.y"
       {(yyval.node) = node("BlockItem", 1, (yyvsp[0].node));}
#line 1572 "syntax_analyzer.c"
    break;

  case 48: /* Stmt: LVal ASSIGN Exp SEMICOLON  */
#line 228 "syntax_analyzer.y"
                            {(yyval.node) = node("Stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1578 "syntax_analyzer.c"
    break;

  case 49: /* Stmt: Exp SEMICOLON  */
#line 229 "syntax_analyzer.y"
                {(yyval.node) = node("Stmt", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1584 "syntax_analyzer.c"
    break;

  case 50: /* Stmt: SEMICOLON  */
#line 230 "syntax_analyzer.y"
            {(yyval.node) = node("Stmt", 1, (yyvsp[0].node));}
#line 1590 "syntax_analyzer.c"
    break;

  case 51: /* Stmt: Block  */
#line 231 "syntax_analyzer.y"
        {(yyval.node) = node("Stmt", 1, (yyvsp[0].node));}
#line 1596 "syntax_analyzer.c"
    break;

  case 52: /* Stmt: IF LPARENTHESIS Cond RPARENTHESIS Stmt  */
#line 232 "syntax_analyzer.y"
                                         {(yyval.node) = node("Stmt", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1602 "syntax_analyzer.c"
    break;

  case 53: /* Stmt: IF LPARENTHESIS Cond RPARENTHESIS Stmt ELSE Stmt  */
#line 233 "syntax_analyzer.y"
                                                   {(yyval.node) = node("Stmt", 7, (yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1608 "syntax_analyzer.c"
    break;

  case 54: /* Stmt: WHILE LPARENTHESIS Cond RPARENTHESIS Stmt  */
#line 234 "syntax_analyzer.y"
                                            {(yyval.node) = node("Stmt", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1614 "syntax_analyzer.c"
    break;

  case 55: /* Stmt: BREAK SEMICOLON  */
#line 235 "syntax_analyzer.y"
                  {(yyval.node) = node("Stmt", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1620 "syntax_analyzer.c"
    break;

  case 56: /* Stmt: CONTINUE SEMICOLON  */
#line 236 "syntax_analyzer.y"
                     {(yyval.node) = node("Stmt", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1626 "syntax_analyzer.c"
    break;

  case 57: /* Stmt: RETURN Exp SEMICOLON  */
#line 237 "syntax_analyzer.y"
                       {(yyval.node) = node("Stmt", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1632 "syntax_analyzer.c"
    break;

  case 58: /* Stmt: RETURN SEMICOLON  */
#line 238 "syntax_analyzer.y"
                   {(yyval.node) = node("Stmt", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1638 "syntax_analyzer.c"
    break;

  case 59: /* Exp: AddExp  */
#line 242 "syntax_analyzer.y"
         {(yyval.node) = node("Exp", 1, (yyvsp[0].node));}
#line 1644 "syntax_analyzer.c"
    break;

  case 60: /* Cond: LOrExp  */
#line 246 "syntax_analyzer.y"
         {(yyval.node) = node("Cond", 1, (yyvsp[0].node));}
#line 1650 "syntax_analyzer.c"
    break;

  case 61: /* LVal: IDENT ExpList  */
#line 250 "syntax_analyzer.y"
                {(yyval.node) = node("LVal", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1656 "syntax_analyzer.c"
    break;

  case 62: /* PrimaryExp: LPARENTHESIS Exp RPARENTHESIS  */
#line 254 "syntax_analyzer.y"
                                {(yyval.node) = node("PrimaryExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1662 "syntax_analyzer.c"
    break;

  case 63: /* PrimaryExp: LVal  */
#line 255 "syntax_analyzer.y"
       {(yyval.node) = node("PrimaryExp", 1, (yyvsp[0].node));}
#line 1668 "syntax_analyzer.c"
    break;

  case 64: /* PrimaryExp: Number  */
#line 256 "syntax_analyzer.y"
         {(yyval.node) = node("PrimaryExp", 1, (yyvsp[0].node));}
#line 1674 "syntax_analyzer.c"
    break;

  case 65: /* Number: Integer  */
#line 260 "syntax_analyzer.y"
          {(yyval.node) = node("Number", 1, (yyvsp[0].node));}
#line 1680 "syntax_analyzer.c"
    break;

  case 66: /* Number: Floatnum  */
#line 261 "syntax_analyzer.y"
           {(yyval.node) = node("Number", 1, (yyvsp[0].node));}
#line 1686 "syntax_analyzer.c"
    break;

  case 67: /* UnaryExp: PrimaryExp  */
#line 265 "syntax_analyzer.y"
             {(yyval.node) = node("UnaryExp", 1, (yyvsp[0].node));}
#line 1692 "syntax_analyzer.c"
    break;

  case 68: /* UnaryExp: IDENT LPARENTHESIS FuncRParams RPARENTHESIS  */
#line 266 "syntax_analyzer.y"
                                              {(yyval.node) = node("UnaryExp", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1698 "syntax_analyzer.c"
    break;

  case 69: /* UnaryExp: IDENT LPARENTHESIS RPARENTHESIS  */
#line 267 "syntax_analyzer.y"
                                  {(yyval.node) = node("UnaryExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1704 "syntax_analyzer.c"
    break;

  case 70: /* UnaryExp: UnaryOp UnaryExp  */
#line 268 "syntax_analyzer.y"
                   {(yyval.node) = node("UnaryExp", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1710 "syntax_analyzer.c"
    break;

  case 71: /* UnaryOp: ADD  */
#line 272 "syntax_analyzer.y"
      {(yyval.node) = node("UnaryOp", 1, (yyvsp[0].node));}
#line 1716 "syntax_analyzer.c"
    break;

  case 72: /* UnaryOp: SUB  */
#line 273 "syntax_analyzer.y"
      {(yyval.node) = node("UnaryOp", 1, (yyvsp[0].node));}
#line 1722 "syntax_analyzer.c"
    break;

  case 73: /* UnaryOp: NOT  */
#line 274 "syntax_analyzer.y"
      {(yyval.node) = node("UnaryOp", 1, (yyvsp[0].node));}
#line 1728 "syntax_analyzer.c"
    break;

  case 74: /* FuncRParams: Exp  */
#line 278 "syntax_analyzer.y"
      {(yyval.node) = node("FuncRParams", 1, (yyvsp[0].node));}
#line 1734 "syntax_analyzer.c"
    break;

  case 75: /* FuncRParams: FuncRParams COMMA Exp  */
#line 279 "syntax_analyzer.y"
                        {(yyval.node) = node("FuncRParams", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1740 "syntax_analyzer.c"
    break;

  case 76: /* MulExp: UnaryExp  */
#line 283 "syntax_analyzer.y"
           {(yyval.node) = node("MulExp", 1, (yyvsp[0].node));}
#line 1746 "syntax_analyzer.c"
    break;

  case 77: /* MulExp: MulExp MUL UnaryExp  */
#line 284 "syntax_analyzer.y"
                      {(yyval.node) = node("MulExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1752 "syntax_analyzer.c"
    break;

  case 78: /* MulExp: MulExp DIV UnaryExp  */
#line 285 "syntax_analyzer.y"
                      {(yyval.node) = node("MulExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1758 "syntax_analyzer.c"
    break;

  case 79: /* MulExp: MulExp MOD UnaryExp  */
#line 286 "syntax_analyzer.y"
                      {(yyval.node) = node("MulExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1764 "syntax_analyzer.c"
    break;

  case 80: /* AddExp: MulExp  */
#line 290 "syntax_analyzer.y"
         {(yyval.node) = node("AddExp", 1, (yyvsp[0].node));}
#line 1770 "syntax_analyzer.c"
    break;

  case 81: /* AddExp: AddExp ADD MulExp  */
#line 291 "syntax_analyzer.y"
                    {(yyval.node) = node("AddExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1776 "syntax_analyzer.c"
    break;

  case 82: /* AddExp: AddExp SUB MulExp  */
#line 292 "syntax_analyzer.y"
                    {(yyval.node) = node("AddExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1782 "syntax_analyzer.c"
    break;

  case 83: /* RelExp: AddExp  */
#line 296 "syntax_analyzer.y"
         {(yyval.node) = node("RelExp", 1, (yyvsp[0].node));}
#line 1788 "syntax_analyzer.c"
    break;

  case 84: /* RelExp: RelExp LT AddExp  */
#line 297 "syntax_analyzer.y"
                   {(yyval.node) = node("RelExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1794 "syntax_analyzer.c"
    break;

  case 85: /* RelExp: RelExp LET AddExp  */
#line 298 "syntax_analyzer.y"
                    {(yyval.node) = node("RelExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1800 "syntax_analyzer.c"
    break;

  case 86: /* RelExp: RelExp GT AddExp  */
#line 299 "syntax_analyzer.y"
                   {(yyval.node) = node("RelExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1806 "syntax_analyzer.c"
    break;

  case 87: /* RelExp: RelExp GET AddExp  */
#line 300 "syntax_analyzer.y"
                    {(yyval.node) = node("RelExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1812 "syntax_analyzer.c"
    break;

  case 88: /* EqExp: RelExp  */
#line 304 "syntax_analyzer.y"
         {(yyval.node) = node("EqExp", 1, (yyvsp[0].node));}
#line 1818 "syntax_analyzer.c"
    break;

  case 89: /* EqExp: EqExp EQ RelExp  */
#line 305 "syntax_analyzer.y"
                  {(yyval.node) = node("EqExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1824 "syntax_analyzer.c"
    break;

  case 90: /* EqExp: EqExp NEQ RelExp  */
#line 306 "syntax_analyzer.y"
                   {(yyval.node) = node("EqExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1830 "syntax_analyzer.c"
    break;

  case 91: /* LAndExp: EqExp  */
#line 310 "syntax_analyzer.y"
        {(yyval.node) = node("LAndExp", 1, (yyvsp[0].node));}
#line 1836 "syntax_analyzer.c"
    break;

  case 92: /* LAndExp: LAndExp AND EqExp  */
#line 311 "syntax_analyzer.y"
                    {(yyval.node) = node("LAndExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1842 "syntax_analyzer.c"
    break;

  case 93: /* LOrExp: LAndExp  */
#line 315 "syntax_analyzer.y"
          {(yyval.node) = node("LOrExp", 1, (yyvsp[0].node));}
#line 1848 "syntax_analyzer.c"
    break;

  case 94: /* LOrExp: LOrExp OR LAndExp  */
#line 316 "syntax_analyzer.y"
                    {(yyval.node) = node("LOrExp", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1854 "syntax_analyzer.c"
    break;

  case 95: /* ConstExp: AddExp  */
#line 320 "syntax_analyzer.y"
         {(yyval.node) = node("ConstExp", 1, (yyvsp[0].node));}
#line 1860 "syntax_analyzer.c"
    break;

  case 96: /* Integer: INTCONST  */
#line 323 "syntax_analyzer.y"
           {(yyval.node) = node("Integer", 1, (yyvsp[0].node));}
#line 1866 "syntax_analyzer.c"
    break;

  case 97: /* Floatnum: FLOATCONST  */
#line 326 "syntax_analyzer.y"
             {(yyval.node) = node("Floatnum", 1, (yyvsp[0].node));}
#line 1872 "syntax_analyzer.c"
    break;


#line 1876 "syntax_analyzer.c"

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

#line 332 "syntax_analyzer.y"


/// The error reporting function.
void yyerror(const char * s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }
    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
