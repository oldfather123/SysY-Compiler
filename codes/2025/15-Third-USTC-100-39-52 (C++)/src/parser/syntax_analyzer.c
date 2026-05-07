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

#line 102 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"

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
  YYSYMBOL_ADASS = 9,                      /* ADASS  */
  YYSYMBOL_SUASS = 10,                     /* SUASS  */
  YYSYMBOL_MUASS = 11,                     /* MUASS  */
  YYSYMBOL_DIASS = 12,                     /* DIASS  */
  YYSYMBOL_LT = 13,                        /* LT  */
  YYSYMBOL_LET = 14,                       /* LET  */
  YYSYMBOL_GT = 15,                        /* GT  */
  YYSYMBOL_GET = 16,                       /* GET  */
  YYSYMBOL_EQ = 17,                        /* EQ  */
  YYSYMBOL_NEQ = 18,                       /* NEQ  */
  YYSYMBOL_LPARENTHESIS = 19,              /* LPARENTHESIS  */
  YYSYMBOL_RPARENTHESIS = 20,              /* RPARENTHESIS  */
  YYSYMBOL_LBRACKET = 21,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 22,                  /* RBRACKET  */
  YYSYMBOL_LBRACE = 23,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 24,                    /* RBRACE  */
  YYSYMBOL_ASSIGN = 25,                    /* ASSIGN  */
  YYSYMBOL_COMMA = 26,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 27,                 /* SEMICOLON  */
  YYSYMBOL_NOT = 28,                       /* NOT  */
  YYSYMBOL_AND = 29,                       /* AND  */
  YYSYMBOL_OR = 30,                        /* OR  */
  YYSYMBOL_INT = 31,                       /* INT  */
  YYSYMBOL_FLOAT = 32,                     /* FLOAT  */
  YYSYMBOL_VOID = 33,                      /* VOID  */
  YYSYMBOL_CONST = 34,                     /* CONST  */
  YYSYMBOL_MAIN = 35,                      /* MAIN  */
  YYSYMBOL_IF = 36,                        /* IF  */
  YYSYMBOL_ELSE = 37,                      /* ELSE  */
  YYSYMBOL_WHILE = 38,                     /* WHILE  */
  YYSYMBOL_BREAK = 39,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 40,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 41,                    /* RETURN  */
  YYSYMBOL_FOR = 42,                       /* FOR  */
  YYSYMBOL_IDENT = 43,                     /* IDENT  */
  YYSYMBOL_INTCONST = 44,                  /* INTCONST  */
  YYSYMBOL_FLOATCONST = 45,                /* FLOATCONST  */
  YYSYMBOL_LOWER_THAN_ASIGN = 46,          /* LOWER_THAN_ASIGN  */
  YYSYMBOL_YYACCEPT = 47,                  /* $accept  */
  YYSYMBOL_Program = 48,                   /* Program  */
  YYSYMBOL_CompMod = 49,                   /* CompMod  */
  YYSYMBOL_BType = 50,                     /* BType  */
  YYSYMBOL_Decl = 51,                      /* Decl  */
  YYSYMBOL_FuncDecl = 52,                  /* FuncDecl  */
  YYSYMBOL_ConstDecl = 53,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 54,              /* ConstDefList  */
  YYSYMBOL_ConstDef = 55,                  /* ConstDef  */
  YYSYMBOL_ConstList = 56,                 /* ConstList  */
  YYSYMBOL_ConstInit = 57,                 /* ConstInit  */
  YYSYMBOL_ConstInitList = 58,             /* ConstInitList  */
  YYSYMBOL_ConstExp = 59,                  /* ConstExp  */
  YYSYMBOL_VarDecl = 60,                   /* VarDecl  */
  YYSYMBOL_VarDeclList = 61,               /* VarDeclList  */
  YYSYMBOL_VarDef = 62,                    /* VarDef  */
  YYSYMBOL_InitVal = 63,                   /* InitVal  */
  YYSYMBOL_InitValList = 64,               /* InitValList  */
  YYSYMBOL_FuncDef = 65,                   /* FuncDef  */
  YYSYMBOL_FuncParams = 66,                /* FuncParams  */
  YYSYMBOL_FuncParam = 67,                 /* FuncParam  */
  YYSYMBOL_ExpList = 68,                   /* ExpList  */
  YYSYMBOL_Body = 69,                      /* Body  */
  YYSYMBOL_BodyList = 70,                  /* BodyList  */
  YYSYMBOL_BodyItem = 71,                  /* BodyItem  */
  YYSYMBOL_stmt = 72,                      /* stmt  */
  YYSYMBOL_Forstmt = 73,                   /* Forstmt  */
  YYSYMBOL_ForInit = 74,                   /* ForInit  */
  YYSYMBOL_ForCond = 75,                   /* ForCond  */
  YYSYMBOL_ForIncr = 76,                   /* ForIncr  */
  YYSYMBOL_IFItem = 77,                    /* IFItem  */
  YYSYMBOL_WhileItem = 78,                 /* WhileItem  */
  YYSYMBOL_jump_stmt = 79,                 /* jump_stmt  */
  YYSYMBOL_Expr = 80,                      /* Expr  */
  YYSYMBOL_Cond = 81,                      /* Cond  */
  YYSYMBOL_LVal = 82,                      /* LVal  */
  YYSYMBOL_Primary_Expr = 83,              /* Primary_Expr  */
  YYSYMBOL_Number = 84,                    /* Number  */
  YYSYMBOL_Unary_Expr = 85,                /* Unary_Expr  */
  YYSYMBOL_Func_Expr = 86,                 /* Func_Expr  */
  YYSYMBOL_FuncRParams = 87,               /* FuncRParams  */
  YYSYMBOL_MDM_Expr = 88,                  /* MDM_Expr  */
  YYSYMBOL_Add_Expr = 89,                  /* Add_Expr  */
  YYSYMBOL_Rel_Expr = 90,                  /* Rel_Expr  */
  YYSYMBOL_Eq_Expr = 91,                   /* Eq_Expr  */
  YYSYMBOL_And_expr = 92,                  /* And_expr  */
  YYSYMBOL_OR_Expr = 93,                   /* OR_Expr  */
  YYSYMBOL_Integer = 94,                   /* Integer  */
  YYSYMBOL_Floatnum = 95                   /* Floatnum  */
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
#define YYFINAL  15
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   279

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  49
/* YYNRULES -- Number of rules.  */
#define YYNRULES  117
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  219

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   301


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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   105,   105,   109,   110,   111,   112,   116,   117,   121,
     122,   123,   127,   128,   129,   130,   134,   137,   138,   142,
     146,   147,   151,   152,   153,   156,   157,   161,   165,   168,
     169,   173,   174,   178,   179,   180,   183,   184,   188,   189,
     190,   191,   195,   196,   197,   201,   202,   205,   206,   210,
     213,   214,   218,   219,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   238,   241,   242,   245,
     246,   249,   250,   254,   255,   259,   263,   264,   265,   266,
     270,   274,   278,   282,   283,   284,   288,   289,   293,   294,
     295,   296,   297,   301,   302,   306,   307,   311,   312,   313,
     314,   318,   319,   320,   324,   325,   326,   327,   328,   332,
     333,   334,   338,   339,   343,   344,   348,   352
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
  "MUL", "DIV", "MOD", "ADASS", "SUASS", "MUASS", "DIASS", "LT", "LET",
  "GT", "GET", "EQ", "NEQ", "LPARENTHESIS", "RPARENTHESIS", "LBRACKET",
  "RBRACKET", "LBRACE", "RBRACE", "ASSIGN", "COMMA", "SEMICOLON", "NOT",
  "AND", "OR", "INT", "FLOAT", "VOID", "CONST", "MAIN", "IF", "ELSE",
  "WHILE", "BREAK", "CONTINUE", "RETURN", "FOR", "IDENT", "INTCONST",
  "FLOATCONST", "LOWER_THAN_ASIGN", "$accept", "Program", "CompMod",
  "BType", "Decl", "FuncDecl", "ConstDecl", "ConstDefList", "ConstDef",
  "ConstList", "ConstInit", "ConstInitList", "ConstExp", "VarDecl",
  "VarDeclList", "VarDef", "InitVal", "InitValList", "FuncDef",
  "FuncParams", "FuncParam", "ExpList", "Body", "BodyList", "BodyItem",
  "stmt", "Forstmt", "ForInit", "ForCond", "ForIncr", "IFItem",
  "WhileItem", "jump_stmt", "Expr", "Cond", "LVal", "Primary_Expr",
  "Number", "Unary_Expr", "Func_Expr", "FuncRParams", "MDM_Expr",
  "Add_Expr", "Rel_Expr", "Eq_Expr", "And_expr", "OR_Expr", "Integer",
  "Floatnum", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-130)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     200,  -130,  -130,   -13,    43,    13,   200,    -4,  -130,  -130,
    -130,  -130,  -130,    32,    25,  -130,  -130,  -130,    72,   113,
    -130,    49,  -130,   141,  -130,    53,   -11,    51,  -130,   -12,
    -130,    56,     6,  -130,    19,    25,  -130,    60,     8,   202,
     192,  -130,  -130,  -130,  -130,  -130,   112,    69,    43,   195,
    -130,  -130,  -130,   111,   202,   202,   202,   202,   167,  -130,
    -130,   153,  -130,  -130,  -130,  -130,  -130,   156,   107,  -130,
    -130,    74,  -130,  -130,   107,    14,   170,  -130,  -130,  -130,
     165,  -130,  -130,  -130,  -130,  -130,  -130,   174,  -130,    85,
     177,  -130,   202,   202,   202,   202,   202,  -130,  -130,   132,
    -130,  -130,   152,   182,   186,   189,   197,   198,   194,   176,
    -130,  -130,  -130,  -130,  -130,  -130,  -130,  -130,   217,    11,
    -130,  -130,  -130,   161,  -130,  -130,  -130,     9,   202,  -130,
    -130,  -130,   156,   156,  -130,   192,   208,   202,   202,  -130,
    -130,  -130,   225,   202,   210,  -130,   202,   202,   202,   202,
     202,   177,  -130,   195,  -130,   202,   231,  -130,    94,   234,
     107,   235,   103,   226,   227,   236,  -130,   232,  -130,   104,
     233,   237,   238,   239,   240,  -130,  -130,  -130,   241,    40,
     138,   202,   202,   202,   202,   202,   202,   202,   202,   138,
     202,   242,    45,  -130,  -130,  -130,  -130,  -130,   243,   221,
     107,   107,   107,   107,   235,   235,   103,   226,  -130,   244,
    -130,   245,   138,   202,  -130,   253,  -130,   138,  -130
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     7,     8,     0,     0,     0,     2,     0,     5,    11,
       9,    10,     6,     0,     0,     1,     3,     4,    20,     0,
      29,     0,    20,     0,    17,     0,    31,     0,    28,     0,
      44,     0,     0,    42,     0,     0,    16,     0,     0,     0,
       0,    20,    30,    50,    14,    39,    45,     0,     0,     0,
      18,    15,    38,     0,     0,     0,     0,     0,    47,   116,
     117,     0,    84,    88,    85,    97,    89,   101,    27,    86,
      87,     0,    32,    33,    80,     0,     0,    13,    41,    43,
       0,    19,    22,    12,    40,    90,    91,     0,    92,     0,
      82,    21,     0,     0,     0,     0,     0,    35,    36,     0,
      49,    60,     0,     0,     0,     0,     0,     0,     0,     0,
      52,    61,    51,    53,    65,    62,    63,    64,     0,    84,
      47,    24,    25,     0,    83,    94,    95,     0,     0,    98,
      99,   100,   102,   103,    34,     0,     0,     0,     0,    78,
      79,    77,     0,    68,    20,    59,     0,     0,     0,     0,
       0,    46,    23,     0,    93,     0,     0,    37,     0,     0,
     104,   109,   112,   114,    81,     0,    76,     0,    67,     0,
       0,     0,     0,     0,     0,    26,    96,    48,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      70,     0,     0,    55,    56,    57,    58,    54,     0,    73,
     105,   106,   107,   108,   110,   111,   113,   115,    75,     0,
      69,     0,     0,    72,    74,     0,    71,     0,    66
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -130,  -130,  -130,     1,    -3,  -130,  -130,  -130,   228,   252,
     -76,  -130,   222,  -130,  -130,   248,   -65,  -130,   256,   -17,
     229,   158,   175,  -130,  -130,   -57,  -130,  -130,  -130,  -130,
    -130,  -130,  -130,   -40,  -129,   -58,  -130,  -130,   -30,  -130,
    -130,    76,   -37,     5,    89,    91,  -130,  -130,  -130
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,    31,     8,     9,    10,    23,    24,    26,
      81,   123,    82,    11,    19,    20,    72,    99,    12,    32,
      33,    90,   111,    75,   112,   113,   114,   167,   209,   215,
     115,   116,   117,   118,   159,    62,    63,    64,    65,    66,
     127,    67,    74,   161,   162,   163,   164,    69,    70
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      73,     7,    68,    16,   122,    14,    98,     7,    38,   165,
      39,    43,    68,    15,    40,    44,    87,   119,    54,    55,
     146,   147,   148,   149,    85,    86,    47,    88,    53,   154,
      13,    73,    48,    56,    48,   155,   150,    43,   100,    18,
      39,   101,    57,    68,    49,     1,     2,   102,     4,   126,
     103,    21,   104,   105,   106,   107,   108,    58,    59,    60,
     198,   210,   129,   130,   131,   211,    48,   142,    22,    29,
     157,    48,   110,    37,     1,     2,   109,   175,    54,    55,
       1,     2,    30,    43,     1,     2,    30,    51,   156,    54,
      55,    25,    43,    56,    41,    73,    77,    71,    97,    46,
     160,   160,    57,   168,    56,   125,   170,   171,   172,   173,
     174,    95,    96,    57,   178,   176,    68,    58,    59,    60,
     185,   186,   119,   199,   191,     1,     2,    30,    58,    59,
      60,   119,   208,    76,    43,     1,     2,    30,    83,    27,
      28,   179,    54,    55,   200,   201,   202,   203,   160,   160,
     160,   160,   192,   160,   119,   214,   134,    56,   135,   119,
     218,    43,    92,    93,    94,   101,    57,    35,    36,    54,
      55,   132,   133,   216,   103,    91,   104,   105,   106,   107,
     108,    58,    59,    60,    56,   152,    89,   153,    80,   121,
     204,   205,   120,    57,   124,   136,    54,    55,   128,    54,
      55,   137,    54,    55,    45,   138,    54,    55,    58,    59,
      60,    56,    52,   143,    56,    71,   139,    56,    80,   144,
      57,    56,    78,    57,   140,   141,    57,   158,    84,   169,
      57,     1,     2,     3,     4,    58,    59,    60,    58,    59,
      60,    58,    59,    60,   145,    58,    59,    60,   181,   182,
     183,   184,   166,   177,   180,   187,   189,   188,   212,   190,
     193,    61,    17,    50,   194,   195,   196,   197,    44,    51,
      77,   213,    83,   217,    34,    42,   206,    79,   151,   207
};

static const yytype_uint8 yycheck[] =
{
      40,     0,    39,     6,    80,     4,    71,     6,    25,   138,
      21,    23,    49,     0,    25,    27,    56,    75,     4,     5,
       9,    10,    11,    12,    54,    55,    20,    57,    20,    20,
      43,    71,    26,    19,    26,    26,    25,    23,    24,    43,
      21,    27,    28,    80,    25,    31,    32,    33,    34,    89,
      36,    19,    38,    39,    40,    41,    42,    43,    44,    45,
      20,   190,    92,    93,    94,    20,    26,   107,    43,    20,
     135,    26,    75,    20,    31,    32,    75,   153,     4,     5,
      31,    32,    33,    23,    31,    32,    33,    27,   128,     4,
       5,    19,    23,    19,    43,   135,    27,    23,    24,    43,
     137,   138,    28,   143,    19,    20,   146,   147,   148,   149,
     150,     4,     5,    28,    20,   155,   153,    43,    44,    45,
      17,    18,   180,   180,    20,    31,    32,    33,    43,    44,
      45,   189,   189,    21,    23,    31,    32,    33,    27,    26,
      27,   158,     4,     5,   181,   182,   183,   184,   185,   186,
     187,   188,   169,   190,   212,   212,    24,    19,    26,   217,
     217,    23,     6,     7,     8,    27,    28,    26,    27,     4,
       5,    95,    96,   213,    36,    22,    38,    39,    40,    41,
      42,    43,    44,    45,    19,    24,    19,    26,    23,    24,
     185,   186,    22,    28,    20,    43,     4,     5,    21,     4,
       5,    19,     4,     5,    29,    19,     4,     5,    43,    44,
      45,    19,    37,    19,    19,    23,    27,    19,    23,    43,
      28,    19,    47,    28,    27,    27,    28,    19,    53,    19,
      28,    31,    32,    33,    34,    43,    44,    45,    43,    44,
      45,    43,    44,    45,    27,    43,    44,    45,    13,    14,
      15,    16,    27,    22,    20,    29,    20,    30,    37,    27,
      27,    39,     6,    35,    27,    27,    27,    27,    27,    27,
      27,    27,    27,    20,    22,    27,   187,    48,   120,   188
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    31,    32,    33,    34,    48,    49,    50,    51,    52,
      53,    60,    65,    43,    50,     0,    51,    65,    43,    61,
      62,    19,    43,    54,    55,    19,    56,    26,    27,    20,
      33,    50,    66,    67,    56,    26,    27,    20,    66,    21,
      25,    43,    62,    23,    27,    69,    43,    20,    26,    25,
      55,    27,    69,    20,     4,     5,    19,    28,    43,    44,
      45,    59,    82,    83,    84,    85,    86,    88,    89,    94,
      95,    23,    63,    80,    89,    70,    21,    27,    69,    67,
      23,    57,    59,    27,    69,    85,    85,    80,    85,    19,
      68,    22,     6,     7,     8,     4,     5,    24,    63,    64,
      24,    27,    33,    36,    38,    39,    40,    41,    42,    50,
      51,    69,    71,    72,    73,    77,    78,    79,    80,    82,
      22,    24,    57,    58,    20,    20,    80,    87,    21,    85,
      85,    85,    88,    88,    24,    26,    43,    19,    19,    27,
      27,    27,    80,    19,    43,    27,     9,    10,    11,    12,
      25,    68,    24,    26,    20,    26,    80,    63,    19,    81,
      89,    90,    91,    92,    93,    81,    27,    74,    80,    19,
      80,    80,    80,    80,    80,    57,    80,    22,    20,    66,
      20,    13,    14,    15,    16,    17,    18,    29,    30,    20,
      27,    20,    66,    27,    27,    27,    27,    27,    20,    72,
      89,    89,    89,    89,    90,    90,    91,    92,    72,    75,
      81,    20,    37,    27,    72,    76,    80,    20,    72
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    47,    48,    49,    49,    49,    49,    50,    50,    51,
      51,    51,    52,    52,    52,    52,    53,    54,    54,    55,
      56,    56,    57,    57,    57,    58,    58,    59,    60,    61,
      61,    62,    62,    63,    63,    63,    64,    64,    65,    65,
      65,    65,    66,    66,    66,    67,    67,    68,    68,    69,
      70,    70,    71,    71,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    73,    74,    74,    75,
      75,    76,    76,    77,    77,    78,    79,    79,    79,    79,
      80,    81,    82,    83,    83,    83,    84,    84,    85,    85,
      85,    85,    85,    86,    86,    87,    87,    88,    88,    88,
      88,    89,    89,    89,    90,    90,    90,    90,    90,    91,
      91,    91,    92,    92,    93,    93,    94,    95
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     6,     6,     5,     5,     4,     1,     3,     4,
       0,     4,     1,     3,     2,     1,     3,     1,     3,     1,
       3,     2,     4,     1,     3,     2,     1,     3,     5,     5,
       6,     6,     1,     3,     1,     2,     5,     0,     4,     3,
       0,     2,     1,     1,     4,     4,     4,     4,     4,     2,
       1,     1,     1,     1,     1,     1,     9,     1,     0,     1,
       0,     1,     0,     5,     7,     5,     3,     2,     2,     2,
       1,     1,     2,     3,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     4,     3,     1,     3,     1,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     1,     3,     1,     1
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
  case 2: /* Program: CompMod  */
#line 105 "syntax_analyzer.y"
                  {(yyval.node) = node("program", 1, (yyvsp[0].node)); gt->root = (yyval.node);}
#line 1347 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 3: /* CompMod: CompMod Decl  */
#line 109 "syntax_analyzer.y"
               {(yyval.node) = node("CompMod", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1353 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 4: /* CompMod: CompMod FuncDef  */
#line 110 "syntax_analyzer.y"
                   {(yyval.node) = node("CompMod", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1359 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 5: /* CompMod: Decl  */
#line 111 "syntax_analyzer.y"
       {(yyval.node) = node("CompMod", 1, (yyvsp[0].node));}
#line 1365 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 6: /* CompMod: FuncDef  */
#line 112 "syntax_analyzer.y"
          {(yyval.node) = node("CompMod", 1, (yyvsp[0].node));}
#line 1371 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 7: /* BType: INT  */
#line 116 "syntax_analyzer.y"
      {(yyval.node) = node("BType", 1, (yyvsp[0].node));}
#line 1377 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 8: /* BType: FLOAT  */
#line 117 "syntax_analyzer.y"
        {(yyval.node) = node("BType", 1, (yyvsp[0].node));}
#line 1383 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 9: /* Decl: ConstDecl  */
#line 121 "syntax_analyzer.y"
            {(yyval.node) = node("Decl", 1, (yyvsp[0].node));}
#line 1389 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 10: /* Decl: VarDecl  */
#line 122 "syntax_analyzer.y"
            {(yyval.node) = node("Decl", 1, (yyvsp[0].node));}
#line 1395 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 11: /* Decl: FuncDecl  */
#line 123 "syntax_analyzer.y"
            {(yyval.node) = node("Decl", 1, (yyvsp[0].node));}
#line 1401 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 12: /* FuncDecl: BType IDENT LPARENTHESIS FuncParams RPARENTHESIS SEMICOLON  */
#line 127 "syntax_analyzer.y"
                                                             {(yyval.node) = node("FuncDecl", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1407 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 13: /* FuncDecl: VOID IDENT LPARENTHESIS FuncParams RPARENTHESIS SEMICOLON  */
#line 128 "syntax_analyzer.y"
                                                            {(yyval.node) = node("FuncDecl", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1413 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 14: /* FuncDecl: VOID IDENT LPARENTHESIS RPARENTHESIS SEMICOLON  */
#line 129 "syntax_analyzer.y"
                                                 {(yyval.node) = node("FuncDecl", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1419 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 15: /* FuncDecl: BType IDENT LPARENTHESIS RPARENTHESIS SEMICOLON  */
#line 130 "syntax_analyzer.y"
                                                  {(yyval.node) = node("FuncDecl", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1425 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 16: /* ConstDecl: CONST BType ConstDefList SEMICOLON  */
#line 134 "syntax_analyzer.y"
                                     {(yyval.node) = node("ConstDecl", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1431 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 17: /* ConstDefList: ConstDef  */
#line 137 "syntax_analyzer.y"
           {(yyval.node) = node("ConstDefList", 1, (yyvsp[0].node));}
#line 1437 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 18: /* ConstDefList: ConstDefList COMMA ConstDef  */
#line 138 "syntax_analyzer.y"
                              {(yyval.node) = node("ConstDefList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1443 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 19: /* ConstDef: IDENT ConstList ASSIGN ConstInit  */
#line 142 "syntax_analyzer.y"
                                   {(yyval.node) = node("ConstDef", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1449 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 20: /* ConstList: %empty  */
#line 146 "syntax_analyzer.y"
   {(yyval.node) = node("ConstList", 0);}
#line 1455 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 21: /* ConstList: ConstList LBRACKET ConstExp RBRACKET  */
#line 147 "syntax_analyzer.y"
                                       {(yyval.node) = node("ConstList", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1461 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 22: /* ConstInit: ConstExp  */
#line 151 "syntax_analyzer.y"
            {(yyval.node) = node("ConstInit", 1, (yyvsp[0].node));}
#line 1467 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 23: /* ConstInit: LBRACE ConstInitList RBRACE  */
#line 152 "syntax_analyzer.y"
                              {(yyval.node) = node("ConstInit", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1473 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 24: /* ConstInit: LBRACE RBRACE  */
#line 153 "syntax_analyzer.y"
                {(yyval.node) = node("ConstInit", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1479 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 25: /* ConstInitList: ConstInit  */
#line 156 "syntax_analyzer.y"
            {(yyval.node) = node("ConstInitList", 1, (yyvsp[0].node));}
#line 1485 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 26: /* ConstInitList: ConstInitList COMMA ConstInit  */
#line 157 "syntax_analyzer.y"
                                {(yyval.node) = node("ConstInitList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1491 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 27: /* ConstExp: Add_Expr  */
#line 161 "syntax_analyzer.y"
           {(yyval.node) = node("ConstExp", 1, (yyvsp[0].node));}
#line 1497 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 28: /* VarDecl: BType VarDeclList SEMICOLON  */
#line 165 "syntax_analyzer.y"
                              {(yyval.node) = node("VarDecl", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1503 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 29: /* VarDeclList: VarDef  */
#line 168 "syntax_analyzer.y"
          { (yyval.node) = node("VarDefList", 1, (yyvsp[0].node)); }
#line 1509 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 30: /* VarDeclList: VarDeclList COMMA VarDef  */
#line 169 "syntax_analyzer.y"
                           {(yyval.node) = node("VarDeclList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1515 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 31: /* VarDef: IDENT ConstList  */
#line 173 "syntax_analyzer.y"
                  {(yyval.node) = node("VarDef", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1521 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 32: /* VarDef: IDENT ConstList ASSIGN InitVal  */
#line 174 "syntax_analyzer.y"
                                 {(yyval.node) = node("VarDef", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1527 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 33: /* InitVal: Expr  */
#line 178 "syntax_analyzer.y"
       {(yyval.node) = node("InitVal", 1, (yyvsp[0].node));}
#line 1533 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 34: /* InitVal: LBRACE InitValList RBRACE  */
#line 179 "syntax_analyzer.y"
                            {(yyval.node) = node("InitVal", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1539 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 35: /* InitVal: LBRACE RBRACE  */
#line 180 "syntax_analyzer.y"
                {(yyval.node) = node("InitVal", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1545 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 36: /* InitValList: InitVal  */
#line 183 "syntax_analyzer.y"
          {(yyval.node) = node("InitValList", 1, (yyvsp[0].node));}
#line 1551 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 37: /* InitValList: InitValList COMMA InitVal  */
#line 184 "syntax_analyzer.y"
                            {(yyval.node) = node("InitValList", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1557 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 38: /* FuncDef: BType IDENT LPARENTHESIS RPARENTHESIS Body  */
#line 188 "syntax_analyzer.y"
                                             {(yyval.node) = node("FuncDef", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1563 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 39: /* FuncDef: VOID IDENT LPARENTHESIS RPARENTHESIS Body  */
#line 189 "syntax_analyzer.y"
                                            {(yyval.node) = node("FuncDef", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1569 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 40: /* FuncDef: BType IDENT LPARENTHESIS FuncParams RPARENTHESIS Body  */
#line 190 "syntax_analyzer.y"
                                                        {(yyval.node) = node("FuncDef", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1575 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 41: /* FuncDef: VOID IDENT LPARENTHESIS FuncParams RPARENTHESIS Body  */
#line 191 "syntax_analyzer.y"
                                                       {(yyval.node) = node("FuncDef", 6, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1581 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 42: /* FuncParams: FuncParam  */
#line 195 "syntax_analyzer.y"
            {(yyval.node) = node("FuncParams", 1, (yyvsp[0].node));}
#line 1587 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 43: /* FuncParams: FuncParams COMMA FuncParam  */
#line 196 "syntax_analyzer.y"
                             {(yyval.node) = node("FuncParams", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1593 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 44: /* FuncParams: VOID  */
#line 197 "syntax_analyzer.y"
       {(yyval.node) = node("FuncParams", 1, (yyvsp[0].node));}
#line 1599 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 45: /* FuncParam: BType IDENT  */
#line 201 "syntax_analyzer.y"
              {(yyval.node) = node("FuncParam", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1605 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 46: /* FuncParam: BType IDENT LBRACKET RBRACKET ExpList  */
#line 202 "syntax_analyzer.y"
                                        {(yyval.node) = node("FuncParam", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1611 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 47: /* ExpList: %empty  */
#line 205 "syntax_analyzer.y"
  {(yyval.node) = node("ExpList", 0);}
#line 1617 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 48: /* ExpList: ExpList LBRACKET Expr RBRACKET  */
#line 206 "syntax_analyzer.y"
                                 {(yyval.node) = node("ExpList", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1623 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 49: /* Body: LBRACE BodyList RBRACE  */
#line 210 "syntax_analyzer.y"
                         {(yyval.node) = node("Body", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1629 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 50: /* BodyList: %empty  */
#line 213 "syntax_analyzer.y"
  {(yyval.node) = node("BodyList", 0);}
#line 1635 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 51: /* BodyList: BodyList BodyItem  */
#line 214 "syntax_analyzer.y"
                    {(yyval.node) = node("BodyList", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1641 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 52: /* BodyItem: Decl  */
#line 218 "syntax_analyzer.y"
       {(yyval.node) = node("BodyItem", 1, (yyvsp[0].node));}
#line 1647 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 53: /* BodyItem: stmt  */
#line 219 "syntax_analyzer.y"
       {(yyval.node) = node("BodyItem", 1, (yyvsp[0].node));}
#line 1653 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 54: /* stmt: LVal ASSIGN Expr SEMICOLON  */
#line 223 "syntax_analyzer.y"
                             {(yyval.node) = node("stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1659 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 55: /* stmt: LVal ADASS Expr SEMICOLON  */
#line 224 "syntax_analyzer.y"
                            {(yyval.node) = node("stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1665 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 56: /* stmt: LVal SUASS Expr SEMICOLON  */
#line 225 "syntax_analyzer.y"
                            {(yyval.node) = node("stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1671 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 57: /* stmt: LVal MUASS Expr SEMICOLON  */
#line 226 "syntax_analyzer.y"
                            {(yyval.node) = node("stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1677 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 58: /* stmt: LVal DIASS Expr SEMICOLON  */
#line 227 "syntax_analyzer.y"
                            {(yyval.node) = node("stmt", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1683 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 59: /* stmt: Expr SEMICOLON  */
#line 228 "syntax_analyzer.y"
                 {(yyval.node) = node("stmt", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1689 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 60: /* stmt: SEMICOLON  */
#line 229 "syntax_analyzer.y"
            {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1695 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 61: /* stmt: Body  */
#line 230 "syntax_analyzer.y"
       {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1701 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 62: /* stmt: IFItem  */
#line 231 "syntax_analyzer.y"
         {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1707 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 63: /* stmt: WhileItem  */
#line 232 "syntax_analyzer.y"
            {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1713 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 64: /* stmt: jump_stmt  */
#line 233 "syntax_analyzer.y"
            {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1719 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 65: /* stmt: Forstmt  */
#line 234 "syntax_analyzer.y"
          {(yyval.node) = node("stmt", 1, (yyvsp[0].node));}
#line 1725 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 66: /* Forstmt: FOR LPARENTHESIS ForInit SEMICOLON ForCond SEMICOLON ForIncr RPARENTHESIS stmt  */
#line 238 "syntax_analyzer.y"
                                                                                 { (yyval.node) = node("Forstmt", 5, (yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1731 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 67: /* ForInit: Expr  */
#line 241 "syntax_analyzer.y"
       { (yyval.node) = node("ForInit", 1, (yyvsp[0].node)); }
#line 1737 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 68: /* ForInit: %empty  */
#line 242 "syntax_analyzer.y"
  { (yyval.node) = node("ForInit", 0); }
#line 1743 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 69: /* ForCond: Cond  */
#line 245 "syntax_analyzer.y"
       { (yyval.node) = node("ForCond", 1, (yyvsp[0].node)); }
#line 1749 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 70: /* ForCond: %empty  */
#line 246 "syntax_analyzer.y"
  { (yyval.node) = node("ForCond", 0); }
#line 1755 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 71: /* ForIncr: Expr  */
#line 249 "syntax_analyzer.y"
       { (yyval.node) = node("ForIncr", 1, (yyvsp[0].node)); }
#line 1761 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 72: /* ForIncr: %empty  */
#line 250 "syntax_analyzer.y"
  { (yyval.node) = node("ForIncr", 0); }
#line 1767 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 73: /* IFItem: IF LPARENTHESIS Cond RPARENTHESIS stmt  */
#line 254 "syntax_analyzer.y"
                                         {(yyval.node) = node("IFItem", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1773 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 74: /* IFItem: IF LPARENTHESIS Cond RPARENTHESIS stmt ELSE stmt  */
#line 255 "syntax_analyzer.y"
                                                   {(yyval.node) = node("IFItem", 7, (yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1779 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 75: /* WhileItem: WHILE LPARENTHESIS Cond RPARENTHESIS stmt  */
#line 259 "syntax_analyzer.y"
                                            {(yyval.node) = node("WhileItem", 5, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1785 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 76: /* jump_stmt: RETURN Expr SEMICOLON  */
#line 263 "syntax_analyzer.y"
                        { (yyval.node) = node("jump_stmt", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)); }
#line 1791 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 77: /* jump_stmt: RETURN SEMICOLON  */
#line 264 "syntax_analyzer.y"
                   { (yyval.node) = node("jump_stmt", 2, (yyvsp[-1].node), (yyvsp[0].node)); }
#line 1797 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 78: /* jump_stmt: BREAK SEMICOLON  */
#line 265 "syntax_analyzer.y"
                  { (yyval.node) = node("jump_stmt", 2, (yyvsp[-1].node), (yyvsp[0].node)); }
#line 1803 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 79: /* jump_stmt: CONTINUE SEMICOLON  */
#line 266 "syntax_analyzer.y"
                     { (yyval.node) = node("jump_stmt", 2, (yyvsp[-1].node), (yyvsp[0].node)); }
#line 1809 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 80: /* Expr: Add_Expr  */
#line 270 "syntax_analyzer.y"
           {(yyval.node) = node("Expr", 1, (yyvsp[0].node));}
#line 1815 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 81: /* Cond: OR_Expr  */
#line 274 "syntax_analyzer.y"
          {(yyval.node) = node("Cond", 1, (yyvsp[0].node));}
#line 1821 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 82: /* LVal: IDENT ExpList  */
#line 278 "syntax_analyzer.y"
                {(yyval.node) = node("LVal", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1827 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 83: /* Primary_Expr: LPARENTHESIS Expr RPARENTHESIS  */
#line 282 "syntax_analyzer.y"
                                 {(yyval.node) = node("Primary_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1833 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 84: /* Primary_Expr: LVal  */
#line 283 "syntax_analyzer.y"
       {(yyval.node) = node("Primary_Expr", 1, (yyvsp[0].node));}
#line 1839 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 85: /* Primary_Expr: Number  */
#line 284 "syntax_analyzer.y"
         {(yyval.node) = node("Primary_Expr", 1, (yyvsp[0].node));}
#line 1845 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 86: /* Number: Integer  */
#line 288 "syntax_analyzer.y"
          {(yyval.node) = node("Number", 1, (yyvsp[0].node));}
#line 1851 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 87: /* Number: Floatnum  */
#line 289 "syntax_analyzer.y"
           {(yyval.node) = node("Number", 1, (yyvsp[0].node));}
#line 1857 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 88: /* Unary_Expr: Primary_Expr  */
#line 293 "syntax_analyzer.y"
               {(yyval.node) = node("Unary_Expr", 1, (yyvsp[0].node));}
#line 1863 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 89: /* Unary_Expr: Func_Expr  */
#line 294 "syntax_analyzer.y"
                {(yyval.node) = node("Unary_Expr", 1, (yyvsp[0].node));}
#line 1869 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 90: /* Unary_Expr: ADD Unary_Expr  */
#line 295 "syntax_analyzer.y"
                 {(yyval.node) = node("Unary_Expr", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1875 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 91: /* Unary_Expr: SUB Unary_Expr  */
#line 296 "syntax_analyzer.y"
                 {(yyval.node) = node("Unary_Expr", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1881 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 92: /* Unary_Expr: NOT Unary_Expr  */
#line 297 "syntax_analyzer.y"
                 {(yyval.node) = node("Unary_Expr", 2, (yyvsp[-1].node), (yyvsp[0].node));}
#line 1887 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 93: /* Func_Expr: IDENT LPARENTHESIS FuncRParams RPARENTHESIS  */
#line 301 "syntax_analyzer.y"
                                              {(yyval.node) = node("Func_Expr", 4, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1893 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 94: /* Func_Expr: IDENT LPARENTHESIS RPARENTHESIS  */
#line 302 "syntax_analyzer.y"
                                  {(yyval.node) = node("Func_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1899 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 95: /* FuncRParams: Expr  */
#line 306 "syntax_analyzer.y"
       {(yyval.node) = node("FuncRParams", 1, (yyvsp[0].node));}
#line 1905 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 96: /* FuncRParams: FuncRParams COMMA Expr  */
#line 307 "syntax_analyzer.y"
                         {(yyval.node) = node("FuncRParams", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1911 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 97: /* MDM_Expr: Unary_Expr  */
#line 311 "syntax_analyzer.y"
             {(yyval.node) = node("MDM_Expr", 1, (yyvsp[0].node));}
#line 1917 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 98: /* MDM_Expr: MDM_Expr MUL Unary_Expr  */
#line 312 "syntax_analyzer.y"
                          {(yyval.node) = node("MDM_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1923 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 99: /* MDM_Expr: MDM_Expr DIV Unary_Expr  */
#line 313 "syntax_analyzer.y"
                          {(yyval.node) = node("MDM_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1929 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 100: /* MDM_Expr: MDM_Expr MOD Unary_Expr  */
#line 314 "syntax_analyzer.y"
                          {(yyval.node) = node("MDM_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1935 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 101: /* Add_Expr: MDM_Expr  */
#line 318 "syntax_analyzer.y"
           {(yyval.node) = node("Add_Expr", 1, (yyvsp[0].node));}
#line 1941 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 102: /* Add_Expr: Add_Expr ADD MDM_Expr  */
#line 319 "syntax_analyzer.y"
                        {(yyval.node) = node("Add_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1947 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 103: /* Add_Expr: Add_Expr SUB MDM_Expr  */
#line 320 "syntax_analyzer.y"
                        {(yyval.node) = node("Add_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1953 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 104: /* Rel_Expr: Add_Expr  */
#line 324 "syntax_analyzer.y"
           {(yyval.node) = node("Rel_Expr", 1, (yyvsp[0].node));}
#line 1959 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 105: /* Rel_Expr: Rel_Expr LT Add_Expr  */
#line 325 "syntax_analyzer.y"
                       {(yyval.node) = node("Rel_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1965 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 106: /* Rel_Expr: Rel_Expr LET Add_Expr  */
#line 326 "syntax_analyzer.y"
                        {(yyval.node) = node("Rel_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1971 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 107: /* Rel_Expr: Rel_Expr GT Add_Expr  */
#line 327 "syntax_analyzer.y"
                       {(yyval.node) = node("Rel_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1977 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 108: /* Rel_Expr: Rel_Expr GET Add_Expr  */
#line 328 "syntax_analyzer.y"
                        {(yyval.node) = node("Rel_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1983 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 109: /* Eq_Expr: Rel_Expr  */
#line 332 "syntax_analyzer.y"
           {(yyval.node) = node("Eq_Expr", 1, (yyvsp[0].node));}
#line 1989 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 110: /* Eq_Expr: Eq_Expr EQ Rel_Expr  */
#line 333 "syntax_analyzer.y"
                      {(yyval.node) = node("Eq_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 1995 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 111: /* Eq_Expr: Eq_Expr NEQ Rel_Expr  */
#line 334 "syntax_analyzer.y"
                       {(yyval.node) = node("Eq_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 2001 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 112: /* And_expr: Eq_Expr  */
#line 338 "syntax_analyzer.y"
          {(yyval.node) = node("And_expr", 1, (yyvsp[0].node));}
#line 2007 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 113: /* And_expr: And_expr AND Eq_Expr  */
#line 339 "syntax_analyzer.y"
                       {(yyval.node) = node("And_expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 2013 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 114: /* OR_Expr: And_expr  */
#line 343 "syntax_analyzer.y"
           {(yyval.node) = node("OR_Expr", 1, (yyvsp[0].node));}
#line 2019 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 115: /* OR_Expr: OR_Expr OR And_expr  */
#line 344 "syntax_analyzer.y"
                      {(yyval.node) = node("OR_Expr", 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));}
#line 2025 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 116: /* Integer: INTCONST  */
#line 348 "syntax_analyzer.y"
           {(yyval.node) = node("Integer", 1, (yyvsp[0].node));}
#line 2031 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;

  case 117: /* Floatnum: FLOATCONST  */
#line 352 "syntax_analyzer.y"
             {(yyval.node) = node("Floatnum", 1, (yyvsp[0].node));}
#line 2037 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"
    break;


#line 2041 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.c"

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

#line 354 "syntax_analyzer.y"


/// The error reporting function.
void yyerror(const char * s)
{   
    // TO sTUDENTs: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "%d:%d: error:%s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes esential states before running yyparse().
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
