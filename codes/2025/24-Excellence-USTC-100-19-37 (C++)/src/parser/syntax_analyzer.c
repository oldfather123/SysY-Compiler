extern "C" {
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
#line 1 "syntax_analyzer.y"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "syntax_tree.h"


// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart(FILE * input_file);
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree * gt;

// Error reporting
void yyerror(const char * s);

// Helper functions written for you with love
syntax_tree_node * node(const char * node_name, int children_num, ...);

#line 101 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"

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
  YYSYMBOL_Error = 3,                      /* Error  */
  YYSYMBOL_Add = 4,                        /* Add  */
  YYSYMBOL_Sub = 5,                        /* Sub  */
  YYSYMBOL_Mul = 6,                        /* Mul  */
  YYSYMBOL_Div = 7,                        /* Div  */
  YYSYMBOL_Mod = 8,                        /* Mod  */
  YYSYMBOL_LT = 9,                         /* LT  */
  YYSYMBOL_LTE = 10,                       /* LTE  */
  YYSYMBOL_GT = 11,                        /* GT  */
  YYSYMBOL_GTE = 12,                       /* GTE  */
  YYSYMBOL_Eq = 13,                        /* Eq  */
  YYSYMBOL_NEq = 14,                       /* NEq  */
  YYSYMBOL_Not = 15,                       /* Not  */
  YYSYMBOL_And = 16,                       /* And  */
  YYSYMBOL_Or = 17,                        /* Or  */
  YYSYMBOL_Assign = 18,                    /* Assign  */
  YYSYMBOL_Semicolon = 19,                 /* Semicolon  */
  YYSYMBOL_Comma = 20,                     /* Comma  */
  YYSYMBOL_LParenthese = 21,               /* LParenthese  */
  YYSYMBOL_RParenthese = 22,               /* RParenthese  */
  YYSYMBOL_LBracket = 23,                  /* LBracket  */
  YYSYMBOL_RBracket = 24,                  /* RBracket  */
  YYSYMBOL_LBrace = 25,                    /* LBrace  */
  YYSYMBOL_RBrace = 26,                    /* RBrace  */
  YYSYMBOL_Const = 27,                     /* Const  */
  YYSYMBOL_Int = 28,                       /* Int  */
  YYSYMBOL_Float = 29,                     /* Float  */
  YYSYMBOL_If = 30,                        /* If  */
  YYSYMBOL_Else = 31,                      /* Else  */
  YYSYMBOL_Return = 32,                    /* Return  */
  YYSYMBOL_Void = 33,                      /* Void  */
  YYSYMBOL_While = 34,                     /* While  */
  YYSYMBOL_Break = 35,                     /* Break  */
  YYSYMBOL_Continue = 36,                  /* Continue  */
  YYSYMBOL_Ident = 37,                     /* Ident  */
  YYSYMBOL_DecIntConst = 38,               /* DecIntConst  */
  YYSYMBOL_OctIntConst = 39,               /* OctIntConst  */
  YYSYMBOL_HexIntConst = 40,               /* HexIntConst  */
  YYSYMBOL_DecFloatConst = 41,             /* DecFloatConst  */
  YYSYMBOL_HexFloatConst = 42,             /* HexFloatConst  */
  YYSYMBOL_Epsilon = 43,                   /* Epsilon  */
  YYSYMBOL_YYACCEPT = 44,                  /* $accept  */
  YYSYMBOL_Program = 45,                   /* Program  */
  YYSYMBOL_CompUnit = 46,                  /* CompUnit  */
  YYSYMBOL_BType = 47,                     /* BType  */
  YYSYMBOL_Decl = 48,                      /* Decl  */
  YYSYMBOL_ConstDecl = 49,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 50,              /* ConstDefList  */
  YYSYMBOL_ConstDef = 51,                  /* ConstDef  */
  YYSYMBOL_ConstExpList = 52,              /* ConstExpList  */
  YYSYMBOL_ConstInitVal = 53,              /* ConstInitVal  */
  YYSYMBOL_ConstInitValList = 54,          /* ConstInitValList  */
  YYSYMBOL_VarDecl = 55,                   /* VarDecl  */
  YYSYMBOL_VarDefList = 56,                /* VarDefList  */
  YYSYMBOL_VarDef = 57,                    /* VarDef  */
  YYSYMBOL_InitVal = 58,                   /* InitVal  */
  YYSYMBOL_InitValList = 59,               /* InitValList  */
  YYSYMBOL_FuncDef = 60,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 61,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 62,                /* FuncFParam  */
  YYSYMBOL_ExpList = 63,                   /* ExpList  */
  YYSYMBOL_Block = 64,                     /* Block  */
  YYSYMBOL_BlockItemList = 65,             /* BlockItemList  */
  YYSYMBOL_BlockItem = 66,                 /* BlockItem  */
  YYSYMBOL_Stmt = 67,                      /* Stmt  */
  YYSYMBOL_Exp = 68,                       /* Exp  */
  YYSYMBOL_Cond = 69,                      /* Cond  */
  YYSYMBOL_LVal = 70,                      /* LVal  */
  YYSYMBOL_PrimaryExp = 71,                /* PrimaryExp  */
  YYSYMBOL_DecIntNum = 72,                 /* DecIntNum  */
  YYSYMBOL_OctIntNum = 73,                 /* OctIntNum  */
  YYSYMBOL_HexIntNum = 74,                 /* HexIntNum  */
  YYSYMBOL_DecFloatNum = 75,               /* DecFloatNum  */
  YYSYMBOL_HexFloatNum = 76,               /* HexFloatNum  */
  YYSYMBOL_UnaryExp = 77,                  /* UnaryExp  */
  YYSYMBOL_UnaryOp = 78,                   /* UnaryOp  */
  YYSYMBOL_FuncRParams = 79,               /* FuncRParams  */
  YYSYMBOL_MulExp = 80,                    /* MulExp  */
  YYSYMBOL_AddExp = 81,                    /* AddExp  */
  YYSYMBOL_RelExp = 82,                    /* RelExp  */
  YYSYMBOL_EqExp = 83,                     /* EqExp  */
  YYSYMBOL_LAndExp = 84,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 85,                    /* LOrExp  */
  YYSYMBOL_ConstExp = 86                   /* ConstExp  */
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
#define YYLAST   283

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  43
/* YYNRULES -- Number of rules.  */
#define YYNRULES  101
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  177

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   298


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
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,   109,   109,   111,   112,   113,   114,   116,   117,   119,
     120,   122,   124,   125,   127,   129,   130,   132,   133,   134,
     136,   137,   139,   141,   142,   144,   145,   147,   148,   149,
     151,   152,   154,   155,   156,   157,   160,   161,   163,   164,
     166,   167,   169,   171,   172,   174,   175,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   189,   191,
     193,   195,   196,   197,   198,   199,   200,   201,   203,   205,
     207,   209,   211,   213,   214,   215,   216,   218,   219,   220,
     222,   223,   225,   226,   227,   228,   230,   231,   232,   234,
     235,   236,   237,   238,   240,   241,   242,   244,   245,   247,
     248,   250
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
  "\"end of file\"", "error", "\"invalid token\"", "Error", "Add", "Sub",
  "Mul", "Div", "Mod", "LT", "LTE", "GT", "GTE", "Eq", "NEq", "Not", "And",
  "Or", "Assign", "Semicolon", "Comma", "LParenthese", "RParenthese",
  "LBracket", "RBracket", "LBrace", "RBrace", "Const", "Int", "Float",
  "If", "Else", "Return", "Void", "While", "Break", "Continue", "Ident",
  "DecIntConst", "OctIntConst", "HexIntConst", "DecFloatConst",
  "HexFloatConst", "Epsilon", "$accept", "Program", "CompUnit", "BType",
  "Decl", "ConstDecl", "ConstDefList", "ConstDef", "ConstExpList",
  "ConstInitVal", "ConstInitValList", "VarDecl", "VarDefList", "VarDef",
  "InitVal", "InitValList", "FuncDef", "FuncFParams", "FuncFParam",
  "ExpList", "Block", "BlockItemList", "BlockItem", "Stmt", "Exp", "Cond",
  "LVal", "PrimaryExp", "DecIntNum", "OctIntNum", "HexIntNum",
  "DecFloatNum", "HexFloatNum", "UnaryExp", "UnaryOp", "FuncRParams",
  "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", "ConstExp", YY_NULLPTR
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
      68,   104,  -102,  -102,   -10,    40,    68,    36,  -102,  -102,
    -102,  -102,    47,    60,  -102,  -102,  -102,    66,    44,  -102,
    -102,   117,  -102,    -9,    29,    42,  -102,    53,    52,  -102,
      47,    31,    62,    -6,  -102,    31,    56,   156,   241,  -102,
    -102,   195,  -102,  -102,  -102,    79,   104,    31,  -102,    31,
    -102,  -102,  -102,   241,    67,    95,  -102,  -102,  -102,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
    -102,   241,    17,   134,   134,   103,   149,  -102,  -102,     7,
     119,  -102,  -102,  -102,   108,  -102,  -102,    -5,   202,   111,
    -102,   241,   241,   241,   241,   241,  -102,  -102,  -102,    35,
    -102,  -102,   120,   210,   138,   146,   147,    53,  -102,  -102,
    -102,  -102,   148,   150,  -102,  -102,   156,  -102,  -102,  -102,
     106,   241,  -102,  -102,  -102,    17,    17,   195,  -102,   241,
    -102,   153,   241,  -102,  -102,  -102,   241,   111,  -102,   241,
    -102,   145,  -102,   151,   134,   101,   142,   160,   161,  -102,
     157,   163,  -102,  -102,   110,   241,   241,   241,   241,   241,
     241,   241,   241,   110,  -102,   152,   134,   134,   134,   134,
     101,   101,   142,   160,  -102,   110,  -102
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     7,     8,     0,     0,     2,     0,     5,     9,
      10,     6,     0,     0,     1,     3,     4,    16,     0,    23,
      16,     0,    12,     0,     0,    25,    22,     0,     0,    11,
       0,     0,     0,     0,    36,     0,     0,     0,     0,    16,
      24,     0,    13,    44,    33,    38,     0,     0,    32,     0,
      77,    78,    79,     0,     0,    41,    68,    69,    70,    71,
      72,    26,    27,    62,    73,    63,    64,    65,    66,    67,
      82,     0,    86,    58,   101,     0,     0,    14,    17,     0,
       0,    37,    35,    34,     0,    28,    30,     0,     0,    60,
      76,     0,     0,     0,     0,     0,    15,    18,    20,     0,
      49,    42,     0,     0,     0,     0,     0,     0,    45,    50,
      43,    46,     0,    62,    41,    61,     0,    29,    74,    80,
       0,     0,    83,    84,    85,    87,    88,     0,    19,     0,
      56,     0,     0,    54,    55,    48,     0,    39,    31,     0,
      75,     0,    21,     0,    89,    94,    97,    99,    59,    57,
       0,     0,    81,    40,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    47,    51,    90,    91,    92,    93,
      95,    96,    98,   100,    53,     0,    52
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -102,  -102,  -102,     4,     0,  -102,  -102,   154,   165,   -68,
    -102,  -102,  -102,   174,   -47,  -102,   186,   178,   158,    89,
      19,  -102,  -102,  -101,   -36,    48,   -77,  -102,  -102,  -102,
    -102,  -102,  -102,   -62,  -102,  -102,    63,   -38,     3,    50,
      43,  -102,   170
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,    32,     8,     9,    21,    22,    25,    77,
      99,    10,    18,    19,    61,    87,    11,    33,    34,    89,
     109,    79,   110,   111,   112,   143,    63,    64,    65,    66,
      67,    68,    69,    70,    71,   120,    72,    73,   145,   146,
     147,   148,    78
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      74,    62,   113,    74,     7,    12,    15,    86,    98,    90,
       7,    50,    51,    31,    46,   116,    47,    84,    62,     2,
       3,   117,    52,    91,    92,    93,   100,    13,    53,   122,
     123,   124,    43,   101,     1,     2,     3,   102,    74,   103,
      14,   104,   105,   106,    55,    56,    57,    58,    59,    60,
      44,    35,   119,   165,    48,   127,    43,     2,     3,   142,
      37,   128,   174,    26,    27,    38,    82,   131,    83,   138,
      41,    50,    51,    17,   176,    38,    46,   113,    49,   108,
      62,    23,    52,   107,    20,   141,   113,    24,    53,    74,
      39,   144,    54,    85,   144,     1,     2,     3,   113,    45,
     151,     4,    80,   152,    55,    56,    57,    58,    59,    60,
     155,   156,   157,   158,    50,    51,    88,   166,   167,   168,
     169,   144,   144,   144,   144,    52,   139,    96,   140,   100,
     115,    53,     2,     3,   121,    43,    29,    30,    94,    95,
     102,   129,   103,   114,   104,   105,   106,    55,    56,    57,
      58,    59,    60,    50,    51,   159,   160,   125,   126,   132,
      50,    51,   170,   171,    52,   133,   134,   135,   136,   153,
      53,    52,   149,   154,    76,    97,   161,    53,   162,   163,
     150,    54,   164,   175,    42,    28,    55,    56,    57,    58,
      59,    60,    16,    55,    56,    57,    58,    59,    60,    50,
      51,    40,    36,   137,    81,   173,    50,    51,    75,     0,
      52,   172,     0,     0,    50,    51,    53,    52,     0,     0,
      76,     0,     0,    53,   118,    52,     0,     0,     0,   130,
       0,    53,    55,    56,    57,    58,    59,    60,     0,    55,
      56,    57,    58,    59,    60,    50,    51,    55,    56,    57,
      58,    59,    60,     0,     0,     0,    52,     0,     0,     0,
       0,     0,    53,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    55,    56,
      57,    58,    59,    60
};

static const yytype_int16 yycheck[] =
{
      38,    37,    79,    41,     0,     1,     6,    54,    76,    71,
       6,     4,     5,    22,    20,    20,    22,    53,    54,    28,
      29,    26,    15,     6,     7,     8,    19,    37,    21,    91,
      92,    93,    25,    26,    27,    28,    29,    30,    76,    32,
       0,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      31,    22,    88,   154,    35,    20,    25,    28,    29,   127,
      18,    26,   163,    19,    20,    23,    47,   103,    49,   116,
      18,     4,     5,    37,   175,    23,    20,   154,    22,    79,
     116,    21,    15,    79,    37,   121,   163,    21,    21,   127,
      37,   129,    25,    26,   132,    27,    28,    29,   175,    37,
     136,    33,    23,   139,    37,    38,    39,    40,    41,    42,
       9,    10,    11,    12,     4,     5,    21,   155,   156,   157,
     158,   159,   160,   161,   162,    15,    20,    24,    22,    19,
      22,    21,    28,    29,    23,    25,    19,    20,     4,     5,
      30,    21,    32,    24,    34,    35,    36,    37,    38,    39,
      40,    41,    42,     4,     5,    13,    14,    94,    95,    21,
       4,     5,   159,   160,    15,    19,    19,    19,    18,    24,
      21,    15,    19,    22,    25,    26,    16,    21,    17,    22,
     132,    25,    19,    31,    30,    20,    37,    38,    39,    40,
      41,    42,     6,    37,    38,    39,    40,    41,    42,     4,
       5,    27,    24,   114,    46,   162,     4,     5,    38,    -1,
      15,   161,    -1,    -1,     4,     5,    21,    15,    -1,    -1,
      25,    -1,    -1,    21,    22,    15,    -1,    -1,    -1,    19,
      -1,    21,    37,    38,    39,    40,    41,    42,    -1,    37,
      38,    39,    40,    41,    42,     4,     5,    37,    38,    39,
      40,    41,    42,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,
      39,    40,    41,    42
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    27,    28,    29,    33,    45,    46,    47,    48,    49,
      55,    60,    47,    37,     0,    48,    60,    37,    56,    57,
      37,    50,    51,    21,    21,    52,    19,    20,    52,    19,
      20,    22,    47,    61,    62,    22,    61,    18,    23,    37,
      57,    18,    51,    25,    64,    37,    20,    22,    64,    22,
       4,     5,    15,    21,    25,    37,    38,    39,    40,    41,
      42,    58,    68,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    80,    81,    81,    86,    25,    53,    86,    65,
      23,    62,    64,    64,    68,    26,    58,    59,    21,    63,
      77,     6,     7,     8,     4,     5,    24,    26,    53,    54,
      19,    26,    30,    32,    34,    35,    36,    47,    48,    64,
      66,    67,    68,    70,    24,    22,    20,    26,    22,    68,
      79,    23,    77,    77,    77,    80,    80,    20,    26,    21,
      19,    68,    21,    19,    19,    19,    18,    63,    58,    20,
      22,    68,    53,    69,    81,    82,    83,    84,    85,    19,
      69,    68,    68,    24,    22,     9,    10,    11,    12,    13,
      14,    16,    17,    22,    19,    67,    81,    81,    81,    81,
      82,    82,    83,    84,    67,    31,    67
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    44,    45,    46,    46,    46,    46,    47,    47,    48,
      48,    49,    50,    50,    51,    52,    52,    53,    53,    53,
      54,    54,    55,    56,    56,    57,    57,    58,    58,    58,
      59,    59,    60,    60,    60,    60,    61,    61,    62,    62,
      63,    63,    64,    65,    65,    66,    66,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    68,    69,
      70,    71,    71,    71,    71,    71,    71,    71,    72,    73,
      74,    75,    76,    77,    77,    77,    77,    78,    78,    78,
      79,    79,    80,    80,    80,    80,    81,    81,    81,    82,
      82,    82,    82,    82,    83,    83,    83,    84,    84,    85,
      85,    86
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     4,     1,     3,     4,     4,     0,     1,     2,     3,
       1,     3,     3,     1,     3,     2,     4,     1,     2,     3,
       1,     3,     5,     5,     6,     6,     1,     3,     2,     5,
       4,     0,     3,     2,     0,     1,     1,     4,     2,     1,
       1,     5,     7,     5,     2,     2,     2,     3,     1,     1,
       2,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     4,     2,     1,     1,     1,
       1,     3,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1
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
#line 109 "syntax_analyzer.y"
                             { (yyval.node)=node("Program",1,(yyvsp[0].node)); gt->root=(yyval.node); }
#line 1321 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 3: /* CompUnit: CompUnit Decl  */
#line 111 "syntax_analyzer.y"
                                  { (yyval.node)=node("CompUnit",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1327 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 4: /* CompUnit: CompUnit FuncDef  */
#line 112 "syntax_analyzer.y"
                                     { (yyval.node)=node("CompUnit",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1333 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 5: /* CompUnit: Decl  */
#line 113 "syntax_analyzer.y"
                         { (yyval.node)=node("CompUnit",1,(yyvsp[0].node)); }
#line 1339 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 6: /* CompUnit: FuncDef  */
#line 114 "syntax_analyzer.y"
                            { (yyval.node)=node("CompUnit",1,(yyvsp[0].node)); }
#line 1345 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 7: /* BType: Int  */
#line 116 "syntax_analyzer.y"
                        { (yyval.node)=node("Btype",1,(yyvsp[0].node)); }
#line 1351 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 8: /* BType: Float  */
#line 117 "syntax_analyzer.y"
                          { (yyval.node)=node("Btype",1,(yyvsp[0].node)); }
#line 1357 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 9: /* Decl: ConstDecl  */
#line 119 "syntax_analyzer.y"
                              { (yyval.node)=node("Decl",1,(yyvsp[0].node)); }
#line 1363 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 10: /* Decl: VarDecl  */
#line 120 "syntax_analyzer.y"
                            { (yyval.node)=node("Decl",1,(yyvsp[0].node)); }
#line 1369 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 11: /* ConstDecl: Const BType ConstDefList Semicolon  */
#line 122 "syntax_analyzer.y"
                                                       { (yyval.node)=node("ConstDecl",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1375 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 12: /* ConstDefList: ConstDef  */
#line 124 "syntax_analyzer.y"
                             {(yyval.node)=node("ConstDefList",1,(yyvsp[0].node));}
#line 1381 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 13: /* ConstDefList: ConstDefList Comma ConstDef  */
#line 125 "syntax_analyzer.y"
                                                { (yyval.node)=node("ConstDefList",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1387 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 14: /* ConstDef: Ident ConstExpList Assign ConstInitVal  */
#line 127 "syntax_analyzer.y"
                                                           { (yyval.node)=node("ConstDef",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1393 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 15: /* ConstExpList: ConstExpList LBracket ConstExp RBracket  */
#line 129 "syntax_analyzer.y"
                                                            { (yyval.node)=node("ConstExpList",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1399 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 16: /* ConstExpList: %empty  */
#line 130 "syntax_analyzer.y"
                    { (yyval.node)=node("ConstExpList",0); }
#line 1405 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 17: /* ConstInitVal: ConstExp  */
#line 132 "syntax_analyzer.y"
                             { (yyval.node)=node("ConstInitVal",1,(yyvsp[0].node)); }
#line 1411 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 18: /* ConstInitVal: LBrace RBrace  */
#line 133 "syntax_analyzer.y"
                                  { (yyval.node)=node("ConstInitVal",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1417 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 19: /* ConstInitVal: LBrace ConstInitValList RBrace  */
#line 134 "syntax_analyzer.y"
                                                   { (yyval.node)=node("ConstInitVal",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1423 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 20: /* ConstInitValList: ConstInitVal  */
#line 136 "syntax_analyzer.y"
                                 { (yyval.node)=node("ConstInitValList",1,(yyvsp[0].node)); }
#line 1429 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 21: /* ConstInitValList: ConstInitValList Comma ConstInitVal  */
#line 137 "syntax_analyzer.y"
                                                        { (yyval.node)=node("ConstInitValList",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1435 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 22: /* VarDecl: BType VarDefList Semicolon  */
#line 139 "syntax_analyzer.y"
                                               { (yyval.node)=node("VarDecl",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1441 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 23: /* VarDefList: VarDef  */
#line 141 "syntax_analyzer.y"
                           { (yyval.node)=node("VarDefList",1,(yyvsp[0].node)); }
#line 1447 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 24: /* VarDefList: VarDefList Comma VarDef  */
#line 142 "syntax_analyzer.y"
                                            { (yyval.node)=node("VarDefList",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1453 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 25: /* VarDef: Ident ConstExpList  */
#line 144 "syntax_analyzer.y"
                                       { (yyval.node)=node("VarDef",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1459 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 26: /* VarDef: Ident ConstExpList Assign InitVal  */
#line 145 "syntax_analyzer.y"
                                                      { (yyval.node)=node("VarDef",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1465 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 27: /* InitVal: Exp  */
#line 147 "syntax_analyzer.y"
                        { (yyval.node)=node("InitVal",1,(yyvsp[0].node)); }
#line 1471 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 28: /* InitVal: LBrace RBrace  */
#line 148 "syntax_analyzer.y"
                                  { (yyval.node)=node("InitVal",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1477 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 29: /* InitVal: LBrace InitValList RBrace  */
#line 149 "syntax_analyzer.y"
                                              { (yyval.node)=node("InitVal",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1483 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 30: /* InitValList: InitVal  */
#line 151 "syntax_analyzer.y"
                            { (yyval.node)=node("InitValList",1,(yyvsp[0].node)); }
#line 1489 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 31: /* InitValList: InitValList Comma InitVal  */
#line 152 "syntax_analyzer.y"
                                              { (yyval.node)=node("InitValList",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1495 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 32: /* FuncDef: BType Ident LParenthese RParenthese Block  */
#line 154 "syntax_analyzer.y"
                                                              { (yyval.node)=node("FuncDef",5,(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1501 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 33: /* FuncDef: Void Ident LParenthese RParenthese Block  */
#line 155 "syntax_analyzer.y"
                                                             { (yyval.node)=node("FuncDef",5,(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1507 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 34: /* FuncDef: BType Ident LParenthese FuncFParams RParenthese Block  */
#line 156 "syntax_analyzer.y"
                                                                          { (yyval.node)=node("FuncDef",6,(yyvsp[-5].node),(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1513 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 35: /* FuncDef: Void Ident LParenthese FuncFParams RParenthese Block  */
#line 157 "syntax_analyzer.y"
                                                                         { (yyval.node)=node("FuncDef",6,(yyvsp[-5].node),(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1519 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 36: /* FuncFParams: FuncFParam  */
#line 160 "syntax_analyzer.y"
                               { (yyval.node)=node("FuncFParams",1,(yyvsp[0].node)); }
#line 1525 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 37: /* FuncFParams: FuncFParams Comma FuncFParam  */
#line 161 "syntax_analyzer.y"
                                                 { (yyval.node)=node("FuncFParams",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1531 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 38: /* FuncFParam: BType Ident  */
#line 163 "syntax_analyzer.y"
                                { (yyval.node)=node("FuncFParam",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1537 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 39: /* FuncFParam: BType Ident LBracket RBracket ExpList  */
#line 164 "syntax_analyzer.y"
                                                          { (yyval.node)=node("FuncFParam",5,(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1543 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 40: /* ExpList: ExpList LBracket Exp RBracket  */
#line 166 "syntax_analyzer.y"
                                                  { (yyval.node)=node("ExpList",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1549 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 41: /* ExpList: %empty  */
#line 167 "syntax_analyzer.y"
                    { (yyval.node)=node("ExpList",0); }
#line 1555 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 42: /* Block: LBrace BlockItemList RBrace  */
#line 169 "syntax_analyzer.y"
                                                { (yyval.node)=node("Block",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1561 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 43: /* BlockItemList: BlockItemList BlockItem  */
#line 171 "syntax_analyzer.y"
                                            { (yyval.node)=node("BlockItemList",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1567 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 44: /* BlockItemList: %empty  */
#line 172 "syntax_analyzer.y"
                    { (yyval.node)=node("BlockItemList",0); }
#line 1573 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 45: /* BlockItem: Decl  */
#line 174 "syntax_analyzer.y"
                         { (yyval.node)=node("BlockItem",1,(yyvsp[0].node)); }
#line 1579 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 46: /* BlockItem: Stmt  */
#line 175 "syntax_analyzer.y"
                         { (yyval.node)=node("BlockItem",1,(yyvsp[0].node)); }
#line 1585 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 47: /* Stmt: LVal Assign Exp Semicolon  */
#line 177 "syntax_analyzer.y"
                                              { (yyval.node)=node("Stmt",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1591 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 48: /* Stmt: Exp Semicolon  */
#line 178 "syntax_analyzer.y"
                                  { (yyval.node)=node("Stmt",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1597 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 49: /* Stmt: Semicolon  */
#line 179 "syntax_analyzer.y"
                              { (yyval.node)=node("Stmt",1,(yyvsp[0].node)); }
#line 1603 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 50: /* Stmt: Block  */
#line 180 "syntax_analyzer.y"
                          { (yyval.node)=node("Stmt",1,(yyvsp[0].node)); }
#line 1609 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 51: /* Stmt: If LParenthese Cond RParenthese Stmt  */
#line 181 "syntax_analyzer.y"
                                                                       { (yyval.node)=node("Stmt",5,(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1615 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 52: /* Stmt: If LParenthese Cond RParenthese Stmt Else Stmt  */
#line 182 "syntax_analyzer.y"
                                                                   { (yyval.node)=node("Stmt",7,(yyvsp[-6].node),(yyvsp[-5].node),(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1621 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 53: /* Stmt: While LParenthese Cond RParenthese Stmt  */
#line 183 "syntax_analyzer.y"
                                                            { (yyval.node)=node("Stmt",5,(yyvsp[-4].node),(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1627 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 54: /* Stmt: Break Semicolon  */
#line 184 "syntax_analyzer.y"
                                    { (yyval.node)=node("Stmt",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1633 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 55: /* Stmt: Continue Semicolon  */
#line 185 "syntax_analyzer.y"
                                       { (yyval.node)=node("Stmt",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1639 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 56: /* Stmt: Return Semicolon  */
#line 186 "syntax_analyzer.y"
                                     { (yyval.node)=node("Stmt",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1645 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 57: /* Stmt: Return Exp Semicolon  */
#line 187 "syntax_analyzer.y"
                                         { (yyval.node)=node("Stmt",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1651 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 58: /* Exp: AddExp  */
#line 189 "syntax_analyzer.y"
                           { (yyval.node)=node("Exp",1,(yyvsp[0].node)); }
#line 1657 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 59: /* Cond: LOrExp  */
#line 191 "syntax_analyzer.y"
                           { (yyval.node)=node("Cond",1,(yyvsp[0].node)); }
#line 1663 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 60: /* LVal: Ident ExpList  */
#line 193 "syntax_analyzer.y"
                                  { (yyval.node)=node("LVal",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1669 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 61: /* PrimaryExp: LParenthese Exp RParenthese  */
#line 195 "syntax_analyzer.y"
                                                { (yyval.node)=node("PrimaryExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1675 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 62: /* PrimaryExp: LVal  */
#line 196 "syntax_analyzer.y"
                         { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1681 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 63: /* PrimaryExp: DecIntNum  */
#line 197 "syntax_analyzer.y"
                              { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1687 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 64: /* PrimaryExp: OctIntNum  */
#line 198 "syntax_analyzer.y"
                              { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1693 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 65: /* PrimaryExp: HexIntNum  */
#line 199 "syntax_analyzer.y"
                              { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1699 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 66: /* PrimaryExp: DecFloatNum  */
#line 200 "syntax_analyzer.y"
                                { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1705 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 67: /* PrimaryExp: HexFloatNum  */
#line 201 "syntax_analyzer.y"
                                { (yyval.node)=node("PrimaryExp",1,(yyvsp[0].node)); }
#line 1711 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 68: /* DecIntNum: DecIntConst  */
#line 203 "syntax_analyzer.y"
                                { (yyval.node)=node("DecIntNum",1,(yyvsp[0].node)); }
#line 1717 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 69: /* OctIntNum: OctIntConst  */
#line 205 "syntax_analyzer.y"
                                { (yyval.node)=node("OctIntNum",1,(yyvsp[0].node)); }
#line 1723 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 70: /* HexIntNum: HexIntConst  */
#line 207 "syntax_analyzer.y"
                                { (yyval.node)=node("HexIntNum",1,(yyvsp[0].node)); }
#line 1729 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 71: /* DecFloatNum: DecFloatConst  */
#line 209 "syntax_analyzer.y"
                                  { (yyval.node)=node("DecFloatNum",1,(yyvsp[0].node)); }
#line 1735 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 72: /* HexFloatNum: HexFloatConst  */
#line 211 "syntax_analyzer.y"
                                  { (yyval.node)=node("HexFloatNum",1,(yyvsp[0].node)); }
#line 1741 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 73: /* UnaryExp: PrimaryExp  */
#line 213 "syntax_analyzer.y"
                               { (yyval.node)=node("UnaryExp",1,(yyvsp[0].node)); }
#line 1747 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 74: /* UnaryExp: Ident LParenthese RParenthese  */
#line 214 "syntax_analyzer.y"
                                                  { (yyval.node)=node("UnaryExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1753 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 75: /* UnaryExp: Ident LParenthese FuncRParams RParenthese  */
#line 215 "syntax_analyzer.y"
                                                              { (yyval.node)=node("UnaryExp",4,(yyvsp[-3].node),(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1759 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 76: /* UnaryExp: UnaryOp UnaryExp  */
#line 216 "syntax_analyzer.y"
                                     { (yyval.node)=node("UnaryExp",2,(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1765 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 77: /* UnaryOp: Add  */
#line 218 "syntax_analyzer.y"
                        { (yyval.node)=node("UnaryOp",1,(yyvsp[0].node)); }
#line 1771 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 78: /* UnaryOp: Sub  */
#line 219 "syntax_analyzer.y"
                        { (yyval.node)=node("UnaryOp",1,(yyvsp[0].node)); }
#line 1777 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 79: /* UnaryOp: Not  */
#line 220 "syntax_analyzer.y"
                        { (yyval.node)=node("UnaryOp",1,(yyvsp[0].node)); }
#line 1783 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 80: /* FuncRParams: Exp  */
#line 222 "syntax_analyzer.y"
                        { (yyval.node)=node("FuncRParams",1,(yyvsp[0].node)); }
#line 1789 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 81: /* FuncRParams: FuncRParams Comma Exp  */
#line 223 "syntax_analyzer.y"
                                          { (yyval.node)=node("FuncRParams",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1795 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 82: /* MulExp: UnaryExp  */
#line 225 "syntax_analyzer.y"
                             { (yyval.node)=node("MulExp",1,(yyvsp[0].node)); }
#line 1801 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 83: /* MulExp: MulExp Mul UnaryExp  */
#line 226 "syntax_analyzer.y"
                                        { (yyval.node)=node("MulExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1807 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 84: /* MulExp: MulExp Div UnaryExp  */
#line 227 "syntax_analyzer.y"
                                        { (yyval.node)=node("MulExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1813 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 85: /* MulExp: MulExp Mod UnaryExp  */
#line 228 "syntax_analyzer.y"
                                        { (yyval.node)=node("MulExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1819 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 86: /* AddExp: MulExp  */
#line 230 "syntax_analyzer.y"
                           { (yyval.node)=node("AddExp",1,(yyvsp[0].node)); }
#line 1825 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 87: /* AddExp: AddExp Add MulExp  */
#line 231 "syntax_analyzer.y"
                                      { (yyval.node)=node("AddExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1831 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 88: /* AddExp: AddExp Sub MulExp  */
#line 232 "syntax_analyzer.y"
                                      { (yyval.node)=node("AddExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1837 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 89: /* RelExp: AddExp  */
#line 234 "syntax_analyzer.y"
                           { (yyval.node)=node("RelExp",1,(yyvsp[0].node)); }
#line 1843 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 90: /* RelExp: RelExp LT AddExp  */
#line 235 "syntax_analyzer.y"
                                     { (yyval.node)=node("RelExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1849 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 91: /* RelExp: RelExp LTE AddExp  */
#line 236 "syntax_analyzer.y"
                                      { (yyval.node)=node("RelExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1855 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 92: /* RelExp: RelExp GT AddExp  */
#line 237 "syntax_analyzer.y"
                                     { (yyval.node)=node("RelExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1861 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 93: /* RelExp: RelExp GTE AddExp  */
#line 238 "syntax_analyzer.y"
                                      { (yyval.node)=node("RelExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1867 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 94: /* EqExp: RelExp  */
#line 240 "syntax_analyzer.y"
                           { (yyval.node)=node("EqExp",1,(yyvsp[0].node)); }
#line 1873 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 95: /* EqExp: EqExp Eq RelExp  */
#line 241 "syntax_analyzer.y"
                                    { (yyval.node)=node("EqExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1879 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 96: /* EqExp: EqExp NEq RelExp  */
#line 242 "syntax_analyzer.y"
                                     { (yyval.node)=node("EqExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1885 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 97: /* LAndExp: EqExp  */
#line 244 "syntax_analyzer.y"
                          { (yyval.node)=node("LAndExp",1,(yyvsp[0].node)); }
#line 1891 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 98: /* LAndExp: LAndExp And EqExp  */
#line 245 "syntax_analyzer.y"
                                      { (yyval.node)=node("LAndExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1897 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 99: /* LOrExp: LAndExp  */
#line 247 "syntax_analyzer.y"
                            { (yyval.node)=node("LOrExp",1,(yyvsp[0].node)); }
#line 1903 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 100: /* LOrExp: LOrExp Or LAndExp  */
#line 248 "syntax_analyzer.y"
                                      { (yyval.node)=node("LOrExp",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node)); }
#line 1909 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;

  case 101: /* ConstExp: AddExp  */
#line 250 "syntax_analyzer.y"
                           { (yyval.node)=node("ConstExp",1,(yyvsp[0].node)); }
#line 1915 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"
    break;


#line 1919 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.c"

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

#line 253 "syntax_analyzer.y"


// The error reporting function.
void yyerror(const char * s) {
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

// Parse input from file `input_path`, and prints the parsing results
// to stdout.  If input_path is NULL, read from stdin.
//
// This function initializes essential states before running yyparse().
syntax_tree * parse(const char * input_path) {
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

// A helper function to quickly construct a tree node.
//
// e.g. $$ = node("Program", 1, $1);
syntax_tree_node * node(const char *name, int children_num, ...) {
    syntax_tree_node * p = new_syntax_tree_node(name);
    syntax_tree_node * child;
    // 这里表示 epsilon 结点是通过 children_num == 0 来判断的
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; i++) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}

}