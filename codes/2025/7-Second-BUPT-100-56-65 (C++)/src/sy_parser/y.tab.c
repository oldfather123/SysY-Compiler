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
#line 1 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <ctype.h>
    #include <math.h>
    #include "sy_parser/y.tab.h"
    #include "sy_parser/utils.h"
    #include "sy_parser/AST.h"
    #include "sy_parser/symbol_table.h"

    extern int yylineno;
    extern FILE *yyin;
    ASTNodePtr root;

    extern int yylex(void);
    void yyerror(const char *s);

    SymbolType node_type_to_sym_type(NodeType node_type);
    ASTNodePtr initer_process(SymbolPtr symbol, ASTNodePtr initer);
    SymbolPtr var_def(ASTNodePtr var_node, DataType data_type);
    ASTNodePtr function_def(char *name, DataType type, ASTNodePtr params);
    ASTNodePtr get_const_value(ASTNodePtr node);
    ASTNodePtr fold_unary_exp(ASTNodePtr node);
    ASTNodePtr fold_binary_exp(ASTNodePtr node);

#line 99 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"

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

#include "y.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENTIFIER = 3,                 /* IDENTIFIER  */
  YYSYMBOL_INT_CONST = 4,                  /* INT_CONST  */
  YYSYMBOL_FLOAT_CONST = 5,                /* FLOAT_CONST  */
  YYSYMBOL_STRING_CONST = 6,               /* STRING_CONST  */
  YYSYMBOL_7_ = 7,                         /* '-'  */
  YYSYMBOL_8_ = 8,                         /* '+'  */
  YYSYMBOL_9_ = 9,                         /* '*'  */
  YYSYMBOL_10_ = 10,                       /* '/'  */
  YYSYMBOL_11_ = 11,                       /* '%'  */
  YYSYMBOL_12_ = 12,                       /* ';'  */
  YYSYMBOL_13_ = 13,                       /* ','  */
  YYSYMBOL_14_ = 14,                       /* ':'  */
  YYSYMBOL_15_ = 15,                       /* '?'  */
  YYSYMBOL_16_ = 16,                       /* '!'  */
  YYSYMBOL_17_ = 17,                       /* '<'  */
  YYSYMBOL_18_ = 18,                       /* '>'  */
  YYSYMBOL_19_ = 19,                       /* '('  */
  YYSYMBOL_20_ = 20,                       /* ')'  */
  YYSYMBOL_21_ = 21,                       /* '['  */
  YYSYMBOL_22_ = 22,                       /* ']'  */
  YYSYMBOL_23_ = 23,                       /* '{'  */
  YYSYMBOL_24_ = 24,                       /* '}'  */
  YYSYMBOL_25_ = 25,                       /* '.'  */
  YYSYMBOL_26_ = 26,                       /* '='  */
  YYSYMBOL_LEQUAL = 27,                    /* LEQUAL  */
  YYSYMBOL_GEQUAL = 28,                    /* GEQUAL  */
  YYSYMBOL_EQUAL = 29,                     /* EQUAL  */
  YYSYMBOL_NEQUAL = 30,                    /* NEQUAL  */
  YYSYMBOL_AND = 31,                       /* AND  */
  YYSYMBOL_OR = 32,                        /* OR  */
  YYSYMBOL_INT = 33,                       /* INT  */
  YYSYMBOL_FLOAT = 34,                     /* FLOAT  */
  YYSYMBOL_VOID = 35,                      /* VOID  */
  YYSYMBOL_CONST = 36,                     /* CONST  */
  YYSYMBOL_IF = 37,                        /* IF  */
  YYSYMBOL_ELSE = 38,                      /* ELSE  */
  YYSYMBOL_WHILE = 39,                     /* WHILE  */
  YYSYMBOL_BREAK = 40,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 41,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 42,                    /* RETURN  */
  YYSYMBOL_ENDMARKER = 43,                 /* ENDMARKER  */
  YYSYMBOL_YYACCEPT = 44,                  /* $accept  */
  YYSYMBOL_file = 45,                      /* file  */
  YYSYMBOL_CompUnit = 46,                  /* CompUnit  */
  YYSYMBOL_Decl = 47,                      /* Decl  */
  YYSYMBOL_BType = 48,                     /* BType  */
  YYSYMBOL_DimBrackets = 49,               /* DimBrackets  */
  YYSYMBOL_ConstDimBrackets = 50,          /* ConstDimBrackets  */
  YYSYMBOL_ConstDecl = 51,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 52,              /* ConstDefList  */
  YYSYMBOL_ConstDef = 53,                  /* ConstDef  */
  YYSYMBOL_ConstInitVal = 54,              /* ConstInitVal  */
  YYSYMBOL_ConstInitValList = 55,          /* ConstInitValList  */
  YYSYMBOL_VarDecl = 56,                   /* VarDecl  */
  YYSYMBOL_VarDefList = 57,                /* VarDefList  */
  YYSYMBOL_VarDef = 58,                    /* VarDef  */
  YYSYMBOL_InitVal = 59,                   /* InitVal  */
  YYSYMBOL_InitValList = 60,               /* InitValList  */
  YYSYMBOL_FuncDef = 61,                   /* FuncDef  */
  YYSYMBOL_FuncHead = 62,                  /* FuncHead  */
  YYSYMBOL_FuncFParams = 63,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 64,                /* FuncFParam  */
  YYSYMBOL_BlockEnter = 65,                /* BlockEnter  */
  YYSYMBOL_BlockItem = 66,                 /* BlockItem  */
  YYSYMBOL_Block = 67,                     /* Block  */
  YYSYMBOL_Stmt = 68,                      /* Stmt  */
  YYSYMBOL_Cond = 69,                      /* Cond  */
  YYSYMBOL_Exp = 70,                       /* Exp  */
  YYSYMBOL_ConstExp = 71,                  /* ConstExp  */
  YYSYMBOL_LVal = 72,                      /* LVal  */
  YYSYMBOL_PrimaryExp = 73,                /* PrimaryExp  */
  YYSYMBOL_Number = 74,                    /* Number  */
  YYSYMBOL_UnaryExp = 75,                  /* UnaryExp  */
  YYSYMBOL_UnaryOp = 76,                   /* UnaryOp  */
  YYSYMBOL_FuncRParams = 77,               /* FuncRParams  */
  YYSYMBOL_MulExp = 78,                    /* MulExp  */
  YYSYMBOL_AddExp = 79,                    /* AddExp  */
  YYSYMBOL_RelExp = 80,                    /* RelExp  */
  YYSYMBOL_EqExp = 81,                     /* EqExp  */
  YYSYMBOL_LAndExp = 82,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 83                     /* LOrExp  */
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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   237

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  100
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  177

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   278


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
       2,     2,     2,    16,     2,     2,     2,    11,     2,     2,
      19,    20,     9,     8,    13,     7,    25,    10,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    14,    12,
      17,    26,    18,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    21,     2,    22,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    23,     2,    24,     2,     2,     2,     2,
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
       5,     6,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    52,    52,    56,    57,    58,    62,    63,    67,    73,
      82,    89,    90,    94,   101,   102,   106,   110,   114,   126,
     129,   135,   136,   140,   141,   142,   146,   150,   154,   167,
     168,   169,   172,   178,   179,   183,   184,   185,   189,   198,
     203,   211,   212,   213,   217,   221,   228,   232,   233,   234,
     243,   247,   248,   249,   250,   251,   260,   273,   282,   283,
     284,   290,   299,   303,   307,   311,   339,   363,   372,   373,
     374,   378,   384,   393,   394,   400,   427,   428,   429,   433,
     434,   435,   439,   440,   444,   448,   455,   456,   460,   467,
     468,   472,   476,   480,   487,   488,   492,   499,   500,   507,
     508
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
  "\"end of file\"", "error", "\"invalid token\"", "IDENTIFIER",
  "INT_CONST", "FLOAT_CONST", "STRING_CONST", "'-'", "'+'", "'*'", "'/'",
  "'%'", "';'", "','", "':'", "'?'", "'!'", "'<'", "'>'", "'('", "')'",
  "'['", "']'", "'{'", "'}'", "'.'", "'='", "LEQUAL", "GEQUAL", "EQUAL",
  "NEQUAL", "AND", "OR", "INT", "FLOAT", "VOID", "CONST", "IF", "ELSE",
  "WHILE", "BREAK", "CONTINUE", "RETURN", "ENDMARKER", "$accept", "file",
  "CompUnit", "Decl", "BType", "DimBrackets", "ConstDimBrackets",
  "ConstDecl", "ConstDefList", "ConstDef", "ConstInitVal",
  "ConstInitValList", "VarDecl", "VarDefList", "VarDef", "InitVal",
  "InitValList", "FuncDef", "FuncHead", "FuncFParams", "FuncFParam",
  "BlockEnter", "BlockItem", "Block", "Stmt", "Cond", "Exp", "ConstExp",
  "LVal", "PrimaryExp", "Number", "UnaryExp", "UnaryOp", "FuncRParams",
  "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-139)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -139,     5,    57,  -139,  -139,  -139,    14,    80,  -139,  -139,
      36,  -139,   113,  -139,   123,  -139,    51,    41,    65,    75,
    -139,  -139,    65,  -139,    85,  -139,  -139,  -139,    80,    45,
    -139,    80,   174,   218,    46,  -139,    54,  -139,    22,    94,
      11,  -139,   218,    78,    31,    89,  -139,  -139,  -139,  -139,
    -139,  -139,   218,  -139,    81,  -139,  -139,  -139,  -139,   218,
      67,   132,  -139,   132,   218,    86,  -139,  -139,    93,    96,
     115,   129,   201,  -139,    85,  -139,  -139,   136,   127,   131,
      80,  -139,  -139,   139,  -139,   218,   181,   133,   135,  -139,
    -139,   218,   218,   218,   218,   218,   142,    79,   218,   218,
    -139,  -139,  -139,   158,  -139,   218,   150,  -139,   153,  -139,
      37,  -139,   151,   218,  -139,  -139,  -139,  -139,    67,    67,
    -139,    79,  -139,    -3,  -139,   154,   132,    -9,   114,   144,
     159,   163,  -139,   180,   153,  -139,    -1,  -139,   218,  -139,
    -139,   172,     9,    79,  -139,   126,   218,   218,   218,   218,
     218,   218,   218,   218,   126,  -139,    19,   153,  -139,  -139,
    -139,  -139,  -139,   157,   132,   132,   132,   132,    -9,    -9,
     114,   144,  -139,  -139,  -139,   126,  -139
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     0,     0,     1,     8,     9,     0,     0,     2,     4,
       0,     6,     0,     7,     0,     5,     0,     0,     0,    29,
      27,    16,     0,    26,     0,    46,    47,    38,    41,     0,
      17,    41,     0,     0,    31,    18,    29,    28,     0,     0,
       0,    42,     0,     0,     0,    65,    71,    72,    67,    77,
      76,    78,     0,    13,     0,    68,    73,    70,    82,     0,
      86,    64,    30,    63,     0,     0,    54,    50,     0,     0,
       0,     0,     0,    48,     0,    53,    49,     0,    68,    44,
       0,    40,    19,     0,    39,    79,     0,    66,     0,    14,
      74,     0,     0,     0,     0,     0,     0,    35,     0,     0,
      58,    59,    60,     0,    52,     0,    45,    43,    23,    80,
       0,    10,     0,     0,    69,    83,    84,    85,    88,    87,
      15,    35,    36,     0,    33,     0,    89,    94,    97,    99,
      62,     0,    61,     0,    23,    24,     0,    21,     0,    75,
      11,     0,     0,     0,    32,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    51,     0,     0,    20,    81,
      12,    34,    37,    55,    90,    91,    92,    93,    95,    96,
      98,   100,    57,    22,    25,     0,    56
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -139,  -139,  -139,   160,     4,  -139,   -27,  -139,  -139,   177,
      44,    68,  -139,  -139,   186,    69,    90,  -139,  -139,   183,
     138,  -139,  -139,   199,  -138,   117,   -32,   -28,   -38,  -139,
    -139,   -44,  -139,  -139,    52,   -29,     0,    76,    66,  -139
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     2,     9,    39,    87,    34,    11,    12,    30,
     135,   136,    13,    14,    20,   122,   123,    15,    16,    40,
      41,    26,    38,    75,    76,   125,    77,   137,    55,    56,
      57,    58,    59,   110,    60,    63,   127,   128,   129,   130
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      78,    62,    43,    61,    54,     3,    10,   163,   146,   147,
     143,    18,   157,    61,    82,    90,   172,    17,   148,   149,
      88,   144,   143,   158,    80,    45,    46,    47,    48,    49,
      50,    81,   157,   161,    66,    61,    96,   176,    51,    19,
     103,    52,    74,   173,    80,    25,    67,   115,   116,   117,
     138,    84,   106,   109,   112,     4,     5,   139,     7,    68,
      28,    69,    70,    71,    72,   124,    32,    64,    29,   126,
     126,    42,    65,   133,    25,    32,    91,    92,    93,    61,
      33,   141,    45,    46,    47,    48,    49,    50,    36,   124,
       4,     5,     6,     7,    31,    51,    32,    79,    52,    64,
       8,    33,   121,    89,    83,    61,   159,    78,    85,    97,
      86,   124,    98,     4,     5,    99,    78,   164,   165,   166,
     167,   126,   126,   126,   126,    21,    22,   100,    61,    45,
      46,    47,    48,    49,    50,    23,    24,    78,    66,    94,
      95,   101,    51,   150,   151,    52,   118,   119,   104,    25,
     168,   169,    32,   105,   113,   114,    45,    46,    47,    48,
      49,    50,   108,    68,   120,    69,    70,    71,    72,    51,
     132,    64,    52,   140,   145,   152,   134,    45,    46,    47,
      48,    49,    50,   154,    45,    46,    47,    48,    49,    50,
      51,   153,   155,    52,   160,   175,    53,    51,    73,    35,
      52,   174,   156,   111,    45,    46,    47,    48,    49,    50,
      37,   142,   162,   102,    44,    27,   131,    51,   107,   171,
      52,    45,    46,    47,    48,    49,    50,     0,   170,     0,
       0,     0,     0,     0,    51,     0,     0,    52
};

static const yytype_int16 yycheck[] =
{
      38,    33,    29,    32,    32,     0,     2,   145,    17,    18,
      13,     7,    13,    42,    42,    59,   154,     3,    27,    28,
      52,    24,    13,    24,    13,     3,     4,     5,     6,     7,
       8,    20,    13,    24,    12,    64,    64,   175,    16,     3,
      72,    19,    38,    24,    13,    23,    24,    91,    92,    93,
      13,    20,    79,    85,    86,    33,    34,    20,    36,    37,
      19,    39,    40,    41,    42,    97,    21,    21,     3,    98,
      99,    26,    26,   105,    23,    21,     9,    10,    11,   108,
      26,   113,     3,     4,     5,     6,     7,     8,     3,   121,
      33,    34,    35,    36,    19,    16,    21,     3,    19,    21,
      43,    26,    23,    22,    26,   134,   138,   145,    19,    23,
      21,   143,    19,    33,    34,    19,   154,   146,   147,   148,
     149,   150,   151,   152,   153,    12,    13,    12,   157,     3,
       4,     5,     6,     7,     8,    12,    13,   175,    12,     7,
       8,    12,    16,    29,    30,    19,    94,    95,    12,    23,
     150,   151,    21,    26,    21,    20,     3,     4,     5,     6,
       7,     8,    23,    37,    22,    39,    40,    41,    42,    16,
      12,    21,    19,    22,    20,    31,    23,     3,     4,     5,
       6,     7,     8,    20,     3,     4,     5,     6,     7,     8,
      16,    32,    12,    19,    22,    38,    22,    16,    38,    22,
      19,   157,   134,    22,     3,     4,     5,     6,     7,     8,
      24,   121,   143,    12,    31,    16,    99,    16,    80,   153,
      19,     3,     4,     5,     6,     7,     8,    -1,   152,    -1,
      -1,    -1,    -1,    -1,    16,    -1,    -1,    19
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    45,    46,     0,    33,    34,    35,    36,    43,    47,
      48,    51,    52,    56,    57,    61,    62,     3,    48,     3,
      58,    12,    13,    12,    13,    23,    65,    67,    19,     3,
      53,    19,    21,    26,    50,    53,     3,    58,    66,    48,
      63,    64,    26,    50,    63,     3,     4,     5,     6,     7,
       8,    16,    19,    22,    71,    72,    73,    74,    75,    76,
      78,    79,    70,    79,    21,    26,    12,    24,    37,    39,
      40,    41,    42,    47,    48,    67,    68,    70,    72,     3,
      13,    20,    71,    26,    20,    19,    21,    49,    70,    22,
      75,     9,    10,    11,     7,     8,    71,    23,    19,    19,
      12,    12,    12,    70,    12,    26,    50,    64,    23,    70,
      77,    22,    70,    21,    20,    75,    75,    75,    78,    78,
      22,    23,    59,    60,    70,    69,    79,    80,    81,    82,
      83,    69,    12,    70,    23,    54,    55,    71,    13,    20,
      22,    70,    60,    13,    24,    20,    17,    18,    27,    28,
      29,    30,    31,    32,    20,    12,    55,    13,    24,    70,
      22,    24,    59,    68,    79,    79,    79,    79,    80,    80,
      81,    82,    68,    24,    54,    38,    68
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    44,    45,    46,    46,    46,    47,    47,    48,    48,
      49,    49,    49,    50,    50,    50,    51,    52,    52,    53,
      53,    54,    54,    55,    55,    55,    56,    57,    57,    58,
      58,    58,    58,    59,    59,    60,    60,    60,    61,    62,
      62,    63,    63,    63,    64,    64,    65,    66,    66,    66,
      67,    68,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    69,    70,    71,    72,    72,    72,    73,    73,
      73,    74,    74,    75,    75,    75,    76,    76,    76,    77,
      77,    77,    78,    78,    78,    78,    79,    79,    79,    80,
      80,    80,    80,    80,    81,    81,    81,    82,    82,    83,
      83
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     0,     2,     2,     1,     1,     1,     1,
       2,     3,     4,     2,     3,     4,     2,     3,     3,     3,
       6,     1,     3,     0,     1,     3,     2,     2,     3,     1,
       3,     2,     6,     1,     3,     0,     1,     3,     2,     5,
       5,     0,     1,     3,     2,     3,     1,     0,     2,     2,
       3,     4,     2,     1,     1,     5,     7,     5,     2,     2,
       2,     3,     1,     1,     1,     1,     2,     1,     1,     3,
       1,     1,     1,     1,     2,     4,     1,     1,     1,     0,
       1,     3,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3
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
  case 2: /* file: CompUnit ENDMARKER  */
#line 52 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                       { root = (yyvsp[-1].node); YYACCEPT; }
#line 1301 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 3: /* CompUnit: %empty  */
#line 56 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_ROOT, NULL, yylineno, 0); }
#line 1307 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 4: /* CompUnit: CompUnit Decl  */
#line 57 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                    { shift_child((yyvsp[0].node), (yyvsp[-1].node)); free_ast((yyvsp[0].node)); (yyval.node) = (yyvsp[-1].node); }
#line 1313 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 5: /* CompUnit: CompUnit FuncDef  */
#line 58 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                       { add_child((yyvsp[-1].node), (yyvsp[0].node)); (yyval.node) = (yyvsp[-1].node); }
#line 1319 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 6: /* Decl: ConstDecl  */
#line 62 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
              { (yyval.node) = (yyvsp[0].node); }
#line 1325 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 7: /* Decl: VarDecl  */
#line 63 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
              { (yyval.node) = (yyvsp[0].node); }
#line 1331 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 8: /* BType: INT  */
#line 67 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
        {
        NodeData data;
        data.data_type = DATA_INT;
        (yyval.node) = create_ast_node(NODE_TYPE, NULL, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_TYPE, -1);
    }
#line 1342 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 9: /* BType: FLOAT  */
#line 73 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
            {
        NodeData data;
        data.data_type = DATA_FLOAT;
        (yyval.node) = create_ast_node(NODE_TYPE, NULL, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_TYPE, -1);
    }
#line 1353 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 10: /* DimBrackets: '[' ']'  */
#line 82 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
            {
        NodeData data;
        data.direct_int = 0;
        ASTNodePtr first_dim_node = create_ast_node(NODE_CONST, NULL, yylineno, 0);
        set_ast_node_data(first_dim_node, HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
        (yyval.node) = create_ast_node(NODE_LIST, "Dims", yylineno, 1, first_dim_node);
    }
#line 1365 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 11: /* DimBrackets: '[' Exp ']'  */
#line 89 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                  { (yyval.node) = create_ast_node(NODE_LIST, "Dims", yylineno, 1, (yyvsp[-1].node)); }
#line 1371 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 12: /* DimBrackets: DimBrackets '[' Exp ']'  */
#line 90 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                              { add_child((yyvsp[-3].node), (yyvsp[-1].node)); (yyval.node) = (yyvsp[-3].node); }
#line 1377 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 13: /* ConstDimBrackets: '[' ']'  */
#line 94 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
            {
        NodeData data;
        data.direct_int = 0;
        ASTNodePtr first_dim_node = create_ast_node(NODE_CONST, NULL, yylineno, 0);
        set_ast_node_data(first_dim_node, HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
        (yyval.node) = create_ast_node(NODE_LIST, "Dims", yylineno, 1, first_dim_node);
    }
#line 1389 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 14: /* ConstDimBrackets: '[' ConstExp ']'  */
#line 101 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                       { (yyval.node) = create_ast_node(NODE_LIST, "Dims", yylineno, 1, (yyvsp[-1].node)); }
#line 1395 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 15: /* ConstDimBrackets: ConstDimBrackets '[' ConstExp ']'  */
#line 102 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                        { add_child((yyvsp[-3].node), (yyvsp[-1].node)); (yyval.node) = (yyvsp[-3].node); }
#line 1401 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 16: /* ConstDecl: ConstDefList ';'  */
#line 106 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     { (yyval.node) = (yyvsp[-1].node); }
#line 1407 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 17: /* ConstDefList: CONST BType ConstDef  */
#line 110 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                         {
        var_def((yyvsp[0].node), (yyvsp[-1].node)->data.data_type);
        (yyval.node) = create_ast_node(NODE_LIST, "ConstDefs", yylineno, 1, (yyvsp[0].node));
    }
#line 1416 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 18: /* ConstDefList: ConstDefList ',' ConstDef  */
#line 114 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                {
        DataType data_type = DATA_INT;
        if ((yyvsp[-2].node)->children[0])
            if ((yyvsp[-2].node)->children[0]->data.symb_ptr)
                data_type = (yyvsp[-2].node)->children[0]->data.symb_ptr->data_type;
        var_def((yyvsp[0].node), data_type);
        add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 1430 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 19: /* ConstDef: IDENTIFIER '=' ConstExp  */
#line 126 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                            {
        (yyval.node) = create_ast_node(NODE_CONST_VAR_DEF, (yyvsp[-2].str), yylineno, 1, (yyvsp[0].node));
    }
#line 1438 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 20: /* ConstDef: IDENTIFIER ConstDimBrackets '=' '{' ConstInitValList '}'  */
#line 129 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                                               {
        (yyval.node) = create_ast_node(NODE_CONST_ARRAY_DEF, (yyvsp[-5].str), yylineno, 2, (yyvsp[-4].node), (yyvsp[-1].node));
    }
#line 1446 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 21: /* ConstInitVal: ConstExp  */
#line 135 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
             { (yyval.node) = (yyvsp[0].node); }
#line 1452 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 22: /* ConstInitVal: '{' ConstInitValList '}'  */
#line 136 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                               { (yyval.node) = (yyvsp[-1].node); }
#line 1458 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 23: /* ConstInitValList: %empty  */
#line 140 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_LIST, "ConstArrayIniter", yylineno, 0); }
#line 1464 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 24: /* ConstInitValList: ConstInitVal  */
#line 141 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                   { (yyval.node) = create_ast_node(NODE_LIST, "ConstArrayIniter", yylineno, 1, (yyvsp[0].node)); }
#line 1470 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 25: /* ConstInitValList: ConstInitValList ',' ConstInitVal  */
#line 142 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                        { add_child((yyvsp[-2].node), (yyvsp[0].node)); (yyval.node) = (yyvsp[-2].node); }
#line 1476 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 26: /* VarDecl: VarDefList ';'  */
#line 146 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                   { (yyval.node) = (yyvsp[-1].node); }
#line 1482 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 27: /* VarDefList: BType VarDef  */
#line 150 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                 {
        var_def((yyvsp[0].node), (yyvsp[-1].node)->data.data_type);
        (yyval.node) = create_ast_node(NODE_LIST, "VarDefs", yylineno, 1, (yyvsp[0].node));
    }
#line 1491 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 28: /* VarDefList: VarDefList ',' VarDef  */
#line 154 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                            {
        DataType data_type = DATA_INT;
        if ((yyvsp[-2].node)->children[0]) {
            if ((yyvsp[-2].node)->children[0]->data.symb_ptr)
                data_type = (yyvsp[-2].node)->children[0]->data.symb_ptr->data_type;
        }
        var_def((yyvsp[0].node), data_type);
        add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 1506 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 29: /* VarDef: IDENTIFIER  */
#line 167 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
               { (yyval.node) = create_ast_node(NODE_VAR_DEF, (yyvsp[0].str), yylineno, 0); }
#line 1512 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 30: /* VarDef: IDENTIFIER '=' Exp  */
#line 168 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                         { (yyval.node) = create_ast_node(NODE_VAR_DEF, (yyvsp[-2].str), yylineno, 1, (yyvsp[0].node)); }
#line 1518 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 31: /* VarDef: IDENTIFIER ConstDimBrackets  */
#line 169 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                  {
        (yyval.node) = create_ast_node(NODE_ARRAY_DEF, (yyvsp[-1].str), yylineno, 1, (yyvsp[0].node));
    }
#line 1526 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 32: /* VarDef: IDENTIFIER ConstDimBrackets '=' '{' InitValList '}'  */
#line 172 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                                          {
        (yyval.node) = create_ast_node(NODE_ARRAY_DEF, (yyvsp[-5].str), yylineno, 2, (yyvsp[-4].node), (yyvsp[-1].node));
    }
#line 1534 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 33: /* InitVal: Exp  */
#line 178 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
        { (yyval.node) = (yyvsp[0].node); }
#line 1540 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 34: /* InitVal: '{' InitValList '}'  */
#line 179 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          { (yyval.node) = (yyvsp[-1].node); }
#line 1546 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 35: /* InitValList: %empty  */
#line 183 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_LIST, "ArrayIniter", yylineno, 0); }
#line 1552 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 36: /* InitValList: InitVal  */
#line 184 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
              { (yyval.node) = create_ast_node(NODE_LIST, "ArrayIniter", yylineno, 1, (yyvsp[0].node)); }
#line 1558 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 37: /* InitValList: InitValList ',' InitVal  */
#line 185 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                              { add_child((yyvsp[-2].node), (yyvsp[0].node)); (yyval.node) = (yyvsp[-2].node); }
#line 1564 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 38: /* FuncDef: FuncHead Block  */
#line 189 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                   {
        add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
        exit_function();
        exit_scope(); // Pop scope for params and function body
    }
#line 1575 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 39: /* FuncHead: BType IDENTIFIER '(' FuncFParams ')'  */
#line 198 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                         {
        (yyval.node) = function_def((yyvsp[-3].str), (yyvsp[-4].node)->data.data_type, (yyvsp[-1].node));
        if ((yyval.node)->data_type == NODEDATA_SYMB && (yyval.node)->data.symb_ptr)
            enter_function((yyval.node)->data.symb_ptr);
    }
#line 1585 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 40: /* FuncHead: VOID IDENTIFIER '(' FuncFParams ')'  */
#line 203 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                          {
        (yyval.node) = function_def((yyvsp[-3].str), DATA_VOID, (yyvsp[-1].node));
        if ((yyval.node)->data_type == NODEDATA_SYMB && (yyval.node)->data.symb_ptr)
            enter_function((yyval.node)->data.symb_ptr);
    }
#line 1595 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 41: /* FuncFParams: %empty  */
#line 211 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_LIST, "FParams", yylineno, 0); }
#line 1601 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 42: /* FuncFParams: FuncFParam  */
#line 212 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                 { (yyval.node) = create_ast_node(NODE_LIST, "FParams", yylineno, 1, (yyvsp[0].node)); }
#line 1607 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 43: /* FuncFParams: FuncFParams ',' FuncFParam  */
#line 213 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                 { add_child((yyvsp[-2].node), (yyvsp[0].node)); (yyval.node) = (yyvsp[-2].node); }
#line 1613 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 44: /* FuncFParam: BType IDENTIFIER  */
#line 217 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     {
        ASTNodePtr var_node = create_ast_node(NODE_VAR_DEF, (yyvsp[0].str), yylineno, 0);
        (yyval.node) = create_ast_node(NODE_LIST, "Param", yylineno, 2, (yyvsp[-1].node), var_node);
    }
#line 1622 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 45: /* FuncFParam: BType IDENTIFIER ConstDimBrackets  */
#line 221 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                        {
        ASTNodePtr var_node = create_ast_node(NODE_ARRAY_DEF, (yyvsp[-1].str), yylineno, 1, (yyvsp[0].node));
        (yyval.node) = create_ast_node(NODE_LIST, "Param", yylineno, 2, (yyvsp[-2].node), var_node);
    }
#line 1631 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 46: /* BlockEnter: '{'  */
#line 228 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
        { enter_scope(); }
#line 1637 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 47: /* BlockItem: %empty  */
#line 232 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                 { (yyval.node) = create_ast_node(NODE_LIST, "Block", yylineno, 0); }
#line 1643 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 48: /* BlockItem: BlockItem Decl  */
#line 233 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     { shift_child((yyvsp[0].node), (yyvsp[-1].node)); free_ast((yyvsp[0].node)); (yyval.node) = (yyvsp[-1].node); }
#line 1649 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 49: /* BlockItem: BlockItem Stmt  */
#line 234 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     {
        if ((yyvsp[0].node)->node_type == NODE_LIST) {
            if ((yyvsp[0].node)->child_count) add_child((yyvsp[-1].node), (yyvsp[0].node));
        } else add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 1660 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 50: /* Block: BlockEnter BlockItem '}'  */
#line 243 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                             { (yyval.node) = (yyvsp[-1].node); exit_scope(); }
#line 1666 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 51: /* Stmt: LVal '=' Exp ';'  */
#line 247 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     { (yyval.node) = create_ast_node(NODE_ASSIGN_STMT, NULL, yylineno, 2, (yyvsp[-3].node), (yyvsp[-1].node)); }
#line 1672 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 52: /* Stmt: Exp ';'  */
#line 248 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
              { (yyval.node) = (yyvsp[-1].node); }
#line 1678 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 53: /* Stmt: Block  */
#line 249 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
            { (yyval.node) = (yyvsp[0].node); }
#line 1684 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 54: /* Stmt: ';'  */
#line 250 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
          { (yyval.node) = create_ast_node(NODE_LIST, "Empty", yylineno, 0); }
#line 1690 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 55: /* Stmt: IF '(' Cond ')' Stmt  */
#line 251 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                           {
        ASTNodePtr if_2;
        NodeData foo;
        if ((yyvsp[0].node)->node_type == NODE_LIST) {
            set_ast_node_data((yyvsp[0].node), HOLD_NODETYPE, "If-2", foo, HOLD_NODEDATATYPE, -1);
            if_2 = (yyvsp[0].node);
        } else if_2 = create_ast_node(NODE_LIST, "If-2", yylineno, 1, (yyvsp[0].node));
        (yyval.node) = create_ast_node(NODE_IF_STMT, NULL, yylineno, 2, (yyvsp[-2].node), if_2);
    }
#line 1704 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 56: /* Stmt: IF '(' Cond ')' Stmt ELSE Stmt  */
#line 260 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                     {
        ASTNodePtr if_2, if_3;
        NodeData foo;
        if ((yyvsp[-2].node)->node_type == NODE_LIST) {
            set_ast_node_data((yyvsp[-2].node), HOLD_NODETYPE, "If-2", foo, HOLD_NODEDATATYPE, -1);
            if_2 = (yyvsp[-2].node);
        } else if_2 = create_ast_node(NODE_LIST, "If-2", yylineno, 1, (yyvsp[-2].node));
        if ((yyvsp[0].node)->node_type == NODE_LIST) {
            set_ast_node_data((yyvsp[0].node), HOLD_NODETYPE, "If-3", foo, HOLD_NODEDATATYPE, -1);
            if_3 = (yyvsp[0].node);
        } else if_3 = create_ast_node(NODE_LIST, "If-3", yylineno, 1, (yyvsp[0].node));
        (yyval.node) = create_ast_node(NODE_IF_ELSE_STMT, NULL, yylineno, 3, (yyvsp[-4].node), if_2, if_3);
    }
#line 1722 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 57: /* Stmt: WHILE '(' Cond ')' Stmt  */
#line 273 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                              {
        ASTNodePtr while_2;
        NodeData foo;
        if ((yyvsp[0].node)->node_type == NODE_LIST) {
            set_ast_node_data((yyvsp[0].node), HOLD_NODETYPE, "While-2", foo, HOLD_NODEDATATYPE, -1);
            while_2 = (yyvsp[0].node);
        } else while_2 = create_ast_node(NODE_LIST, "While-2", yylineno, 1, (yyvsp[0].node));
        (yyval.node) = create_ast_node(NODE_WHILE_STMT, NULL, yylineno, 2, (yyvsp[-2].node), while_2);
    }
#line 1736 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 58: /* Stmt: BREAK ';'  */
#line 282 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_BREAK_STMT, NULL, yylineno, 0); }
#line 1742 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 59: /* Stmt: CONTINUE ';'  */
#line 283 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                   { (yyval.node) = create_ast_node(NODE_CONTINUE_STMT, NULL, yylineno, 0); }
#line 1748 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 60: /* Stmt: RETURN ';'  */
#line 284 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                 {
        NodeData data;
        data.symb_ptr = get_current_function_scope();
        (yyval.node) = create_ast_node(NODE_RETURN_STMT, data.symb_ptr->name, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, -1);
    }
#line 1759 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 61: /* Stmt: RETURN Exp ';'  */
#line 290 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                     {
        NodeData data;
        data.symb_ptr = get_current_function_scope();
        (yyval.node) = create_ast_node(NODE_RETURN_STMT, data.symb_ptr->name, yylineno, 1, (yyvsp[-1].node));
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, -1);
    }
#line 1770 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 62: /* Cond: LOrExp  */
#line 299 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 1776 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 63: /* Exp: AddExp  */
#line 303 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 1782 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 64: /* ConstExp: AddExp  */
#line 307 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 1788 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 65: /* LVal: IDENTIFIER  */
#line 311 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
               {
        SymbolPtr sym = lookup_symbol((yyvsp[0].str));
        NodeData data;
        if (!sym) {
            yyerror("Undeclared identifier");
            YYERROR;
        }
        data.symb_ptr = sym;
        switch (sym->symbol_type) {
            case SYMB_VAR:
                (yyval.node) = create_ast_node(NODE_VAR, NULL, yylineno, 0);
                break;
            case SYMB_CONST_VAR:
                (yyval.node) = create_ast_node(NODE_CONST_VAR, NULL, yylineno, 0);
                break;
            case SYMB_ARRAY:
                (yyval.node) = create_ast_node(NODE_ARRAY, NULL, yylineno, 0);
                break;
            case SYMB_CONST_ARRAY:
                (yyval.node) = create_ast_node(NODE_CONST_ARRAY, NULL, yylineno, 0);
                break;
            default:
                yyerror("Invalid symbol type");
                YYERROR; 
                break;
        }
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, -1);
    }
#line 1821 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 66: /* LVal: IDENTIFIER DimBrackets  */
#line 339 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                             {
        SymbolPtr sym = lookup_symbol((yyvsp[-1].str));
        NodeData data;
        if (!sym) {
            yyerror("Undeclared identifier");
            YYERROR;
        }
        data.symb_ptr = sym;
        switch (sym->symbol_type) {
            case SYMB_ARRAY:
                set_ast_node_data(
                    (yyvsp[0].node), NODE_ARRAY_ACCESS, NULL, data, NODEDATA_SYMB, yylineno);
                break;
            case SYMB_CONST_ARRAY:
                set_ast_node_data(
                    (yyvsp[0].node), NODE_CONST_ARRAY_ACCESS, NULL, data, NODEDATA_SYMB, yylineno);
                break;
            default:
                yyerror("Invalid symbol type");
                YYERROR;
                break;
        }
        (yyval.node) = (yyvsp[0].node);
    }
#line 1850 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 67: /* LVal: STRING_CONST  */
#line 363 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                   {
        NodeData data;
        data.direct_str = (yyvsp[0].str);
        (yyval.node) = create_ast_node(NODE_CONST, NULL, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_STRING, -1);
    }
#line 1861 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 68: /* PrimaryExp: LVal  */
#line 372 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
         { (yyval.node) = get_const_value((yyvsp[0].node)); }
#line 1867 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 69: /* PrimaryExp: '(' Exp ')'  */
#line 373 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                  { (yyval.node) = (yyvsp[-1].node); }
#line 1873 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 70: /* PrimaryExp: Number  */
#line 374 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
             { (yyval.node) = (yyvsp[0].node); }
#line 1879 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 71: /* Number: INT_CONST  */
#line 378 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
              {
        NodeData data;
        data.direct_int = (int)strtol((yyvsp[0].str), NULL, 0);
        (yyval.node) = create_ast_node(NODE_CONST, NULL, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
    }
#line 1890 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 72: /* Number: FLOAT_CONST  */
#line 384 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                  {
        NodeData data;
        data.direct_float = (float)strtof((yyvsp[0].str), NULL);
        (yyval.node) = create_ast_node(NODE_CONST, NULL, yylineno, 0);
        set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_FLOAT, -1);
    }
#line 1901 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 73: /* UnaryExp: PrimaryExp  */
#line 393 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 1907 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 74: /* UnaryExp: UnaryOp UnaryExp  */
#line 394 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                       {
        NodeData data;
        set_ast_node_data((yyvsp[-1].node), HOLD_NODETYPE, NULL, data, HOLD_NODEDATATYPE, yylineno);
        add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = fold_unary_exp((yyvsp[-1].node));
    }
#line 1918 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 75: /* UnaryExp: IDENTIFIER '(' FuncRParams ')'  */
#line 400 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                                     {
        SymbolPtr sym = lookup_symbol((yyvsp[-3].str));
        NodeData data;
        if (!sym) {
            yyerror("Undeclared function");
            YYERROR;
        }
        sym->attributes.func_info.call_count++;
        data.symb_ptr = sym;
        // 单独处理starttime(__LINE__)和stoptime(__LINE__)
        if (strcmp((yyvsp[-3].str), "starttime") == 0 || strcmp((yyvsp[-3].str), "stoptime") == 0) {
            NodeData line_node_data;
            ASTNodePtr line_node;
            line_node_data.direct_int = yylineno;
            line_node = create_ast_node(NODE_CONST, "line", yylineno, 0);
            set_ast_node_data(
                line_node, HOLD_NODETYPE, NULL, line_node_data, NODEDATA_INT, yylineno);
            (yyval.node) = create_ast_node(NODE_FUNC_CALL, (yyvsp[-3].str), yylineno, 1, line_node);
            set_ast_node_data((yyval.node), HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, yylineno);
        } else {
            set_ast_node_data((yyvsp[-1].node), NODE_FUNC_CALL, (yyvsp[-3].str), data, NODEDATA_SYMB, yylineno);
            (yyval.node) = (yyvsp[-1].node);
        }
    }
#line 1947 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 76: /* UnaryOp: '+'  */
#line 427 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
        { (yyval.node) = create_ast_node(NODE_UNARY_OP, "+", yylineno, 0); }
#line 1953 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 77: /* UnaryOp: '-'  */
#line 428 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
          { (yyval.node) = create_ast_node(NODE_UNARY_OP, "-", yylineno, 0); }
#line 1959 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 78: /* UnaryOp: '!'  */
#line 429 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
          { (yyval.node) = create_ast_node(NODE_UNARY_OP, "!", yylineno, 0); }
#line 1965 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 79: /* FuncRParams: %empty  */
#line 433 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                { (yyval.node) = create_ast_node(NODE_LIST, "RParams", yylineno, 0); }
#line 1971 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 80: /* FuncRParams: Exp  */
#line 434 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
          { (yyval.node) = create_ast_node(NODE_LIST, "RParams", yylineno, 1, (yyvsp[0].node)); }
#line 1977 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 81: /* FuncRParams: FuncRParams ',' Exp  */
#line 435 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          { add_child((yyvsp[-2].node), (yyvsp[0].node)); (yyval.node) = (yyvsp[-2].node); }
#line 1983 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 82: /* MulExp: UnaryExp  */
#line 439 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
             { (yyval.node) = (yyvsp[0].node); }
#line 1989 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 83: /* MulExp: MulExp '*' UnaryExp  */
#line 440 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "*", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 1998 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 84: /* MulExp: MulExp '/' UnaryExp  */
#line 444 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "/", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2007 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 85: /* MulExp: MulExp '%' UnaryExp  */
#line 448 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "%", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2016 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 86: /* AddExp: MulExp  */
#line 455 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 2022 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 87: /* AddExp: AddExp '+' MulExp  */
#line 456 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "+", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2031 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 88: /* AddExp: AddExp '-' MulExp  */
#line 460 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "-", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2040 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 89: /* RelExp: AddExp  */
#line 467 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 2046 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 90: /* RelExp: RelExp '<' AddExp  */
#line 468 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "<", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2055 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 91: /* RelExp: RelExp '>' AddExp  */
#line 472 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, ">", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2064 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 92: /* RelExp: RelExp LEQUAL AddExp  */
#line 476 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                           {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "<=", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2073 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 93: /* RelExp: RelExp GEQUAL AddExp  */
#line 480 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                           {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, ">=", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2082 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 94: /* EqExp: RelExp  */
#line 487 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 2088 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 95: /* EqExp: EqExp EQUAL RelExp  */
#line 488 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                         {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "==", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2097 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 96: /* EqExp: EqExp NEQUAL RelExp  */
#line 492 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                          {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "!=", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2106 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 97: /* LAndExp: EqExp  */
#line 499 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
          { (yyval.node) = (yyvsp[0].node); }
#line 2112 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 98: /* LAndExp: LAndExp AND EqExp  */
#line 500 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "&&", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2121 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 99: /* LOrExp: LAndExp  */
#line 507 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
            { (yyval.node) = (yyvsp[0].node); }
#line 2127 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;

  case 100: /* LOrExp: LOrExp OR LAndExp  */
#line 508 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"
                        {
        (yyval.node) = create_ast_node(NODE_BINARY_OP, "||", yylineno, 2, (yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = fold_binary_exp((yyval.node));
    }
#line 2136 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"
    break;


#line 2140 "/home/runner/work/compiler/compiler/compiler/modules/frontend/src/sy_parser/y.tab.c"

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

#line 514 "/home/runner/work/compiler/compiler/compiler/modules/frontend/flex_yacc/sysy_yacc.y"


void yyerror(const char *s) {
    fprintf(stderr, "%d %s\n", yylineno, s);
}

SymbolType node_type_to_sym_type(NodeType node_type) {
    switch (node_type) {
        case NODE_VAR_DEF:
            return SYMB_VAR;
        case NODE_CONST_VAR_DEF:
            return SYMB_CONST_VAR;
        case NODE_ARRAY_DEF:
            return SYMB_ARRAY;
        case NODE_CONST_ARRAY_DEF:
            return SYMB_CONST_ARRAY;
        default:
            return SYMB_UNKNOWN;
    }
}

// 辅助函数：计算数组第dim_start维之后的总大小
int calculate_array_size(SymbolPtr symbol, int dim_start) {
    if (symbol->symbol_type != SYMB_ARRAY) return 1;
    int output = 1;
    ArrayInfo array_info = symbol->attributes.array_info;
    for (int i = dim_start; i < array_info.dimensions; i++)
        if (array_info.shape[i]) output = output * array_info.shape[i];
    return output;
}

// 辅助函数：补全初始化器隐含的维度
ASTNodePtr recursive_reshape_initer(ASTNodePtr initer, SymbolPtr symbol,
                                    int current_dim, const char *name) {
    if (!initer) return NULL;
    if (current_dim >= symbol->attributes.array_info.dimensions - 1)
        return initer;

    ASTNodePtr output = create_ast_node(NODE_LIST, my_strdup(name), yylineno, 0);
    ASTNodePtr piece;
    int current_array_size = calculate_array_size(symbol, current_dim + 1);
    int acc_item_num = 0;
    for (int i = 0; i < initer->child_count; i++) {
        ASTNodePtr child = initer->children[i];
        if (child->node_type == NODE_LIST) {
            if (acc_item_num) { add_child(output, piece); acc_item_num = 0; }
            add_child(output, child);
            initer->children[i] = NULL;
        } else {
            if (!acc_item_num)
                piece = create_ast_node(NODE_LIST, my_strdup(name), yylineno, 0);
            add_child(piece, child);
            initer->children[i] = NULL;
            acc_item_num++;
            if (acc_item_num >= current_array_size) {
                add_child(output, piece);
                acc_item_num = 0;
            }
        }
    }
    if (acc_item_num) { add_child(output, piece); acc_item_num = 0; }
    free_ast(initer);
    for (int i = 0; i < output->child_count; i++)
        output->children[i] = recursive_reshape_initer(
            output->children[i], symbol, current_dim + 1, name);
    return output;
}

SymbolPtr var_def(ASTNodePtr var_node, DataType data_type) {
    if (!var_node) return NULL;

    NodeData data;
    SymbolType sym_type = node_type_to_sym_type(var_node->node_type);
    SymbolPtr sym = define_symbol(
        var_node->name, sym_type, data_type, var_node->lineno);
    // var for VAR
    ASTNodePtr var_init_node;
    int int_init_value;
    float float_init_value;
    // var for ARRAY
    ASTNodePtr dim_node, single_dim_node;
    int dim_count, dim_size;

    switch (var_node->node_type) {
        case NODE_CONST_VAR_DEF:
            if (var_node->child_count == 0) return NULL;
            var_init_node = var_node->children[0];
            if (data_type == DATA_INT) {
                int_init_value = 0;
                if (var_init_node->data_type == NODEDATA_INT)
                    int_init_value = var_init_node->data.direct_int;
                else if (var_init_node->data_type == NODEDATA_FLOAT)
                    int_init_value = (int)var_init_node->data.direct_float;
                sym->attributes.const_info.int_value = int_init_value;
            } else if (data_type == DATA_FLOAT) {
                float_init_value = 0.0;
                if (var_init_node->data_type == NODEDATA_INT)
                    float_init_value = (float)var_init_node->data.direct_int;
                else if (var_init_node->data_type == NODEDATA_FLOAT)
                    float_init_value = var_init_node->data.direct_float;
                sym->attributes.const_info.float_value = float_init_value;
            }
            break;
        case NODE_ARRAY_DEF:
        case NODE_CONST_ARRAY_DEF:
            dim_node = var_node->children[0];
            dim_count = dim_node->child_count;
            sym->attributes.array_info.dimensions = dim_count;
            sym->attributes.array_info.elem_num = 1;
            sym->attributes.array_info.shape = (int*)malloc(sizeof(int) * dim_count);

            for (int j = 0; j < dim_count; ++j) {
                single_dim_node = dim_node->children[j];
                dim_size = 0;
                if (single_dim_node->data_type == NODEDATA_INT)
                    dim_size = single_dim_node->data.direct_int;
                else if (single_dim_node->data_type == NODEDATA_FLOAT)
                    dim_size = (int)single_dim_node->data.direct_float;
                sym->attributes.array_info.shape[j] = dim_size;
                if (dim_size)
                    sym->attributes.array_info.elem_num =
                        sym->attributes.array_info.elem_num * dim_size;
            }

            free_ast(dim_node);
            var_node->children[0] = NULL;
            // 可根据 valid 变量决定后续处理
            if (var_node->child_count > 1) {
                ASTNodePtr initer = var_node->children[1];
                var_node->children[1] = recursive_reshape_initer(
                    initer, sym, 0, my_strdup(initer->name));
            }
            break;
        default:
            break;
    }
    data.symb_ptr = sym;
    set_ast_node_data(var_node, HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, -1);
    return sym;
}

ASTNodePtr function_def(char *name, DataType type, ASTNodePtr params) {
    SymbolPtr func_sym = define_symbol(name, SYMB_FUNCTION, type, yylineno);
    NodeData data;
    ASTNodePtr output;
    // var in loop
    int param_count;
    ASTNodePtr param_node, type_node, var_node;

    enter_scope();
    if (params) {
        param_count = params->child_count;
        if (param_count > 0) {
            func_sym->attributes.func_info.param_count = param_count;
            func_sym->attributes.func_info.params = (SymbolPtr*)malloc(
                sizeof(SymbolPtr) * param_count);
            for (int i = 0; i < param_count; ++i) {
                param_node = params->children[i];
                type_node = param_node->children[0];
                var_node = param_node->children[1];
                var_def(var_node, type_node->data.data_type);
                var_node->data.symb_ptr->function = func_sym;
                func_sym->attributes.func_info.params[i] = var_node->data.symb_ptr;

                param_node->children[1] = NULL;
                free_ast(param_node);
                params->children[i] = var_node;
            }
        }
    }
    data.symb_ptr = func_sym;
    output = create_ast_node(NODE_FUNC_DEF, name, yylineno, 1, params);
    set_ast_node_data(output, HOLD_NODETYPE, NULL, data, NODEDATA_SYMB, -1);
    return output;
}

ASTNodePtr get_const_value(ASTNodePtr node) {
    SymbolPtr sym = NULL;
    ASTNodePtr valued = NULL;
    NodeData data;

    if (!node) return node;
    if (!node->data.symb_ptr) return node;
    sym = node->data.symb_ptr;

    if (node->node_type == NODE_CONST_VAR) {
        if (sym->data_type == DATA_INT) {
            valued = create_ast_node(NODE_CONST, NULL, node->lineno, 0);
            data.direct_int = sym->attributes.const_info.int_value;
            set_ast_node_data(valued, HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
        } else if (sym->data_type == DATA_FLOAT) {
            valued = create_ast_node(NODE_CONST, NULL, node->lineno, 0);
            data.direct_float = sym->attributes.const_info.float_value;
            set_ast_node_data(valued, HOLD_NODETYPE, NULL, data, NODEDATA_FLOAT, -1);
        }
        return valued;
    }
    return node;
}

ASTNodePtr fold_unary_exp(ASTNodePtr node) {
    ASTNodePtr child;
    ASTNodePtr folded;
    NodeData data;
    int valid, is_float;
    int int_val = 0, int_result = 0;
    float float_val = 0.0, float_result = 0.0;

    if (!node) return node;
    if (node->node_type != NODE_UNARY_OP) return node;

    child = node->children[0];
    if (child && child->node_type == NODE_CONST) {
        is_float = (child->data_type == NODEDATA_FLOAT);
        if (is_float) float_val = child->data.direct_float;
        else int_val = child->data.direct_int;
        valid = 1;
        if (strcmp(node->name, "+") == 0) {
            int_result = int_val;
            float_result = float_val;
        } else if (strcmp(node->name, "-") == 0) {
            int_result = -int_val;
            float_result = -float_val;
        } else if (strcmp(node->name, "!") == 0) {
            int_result = !int_val;
            float_result = !float_val;
        } else valid = 0;
        if (valid) {
            folded = create_ast_node(NODE_CONST, NULL, node->lineno, 0);
            if (is_float) {
                data.direct_float = float_result;
                set_ast_node_data(folded, HOLD_NODETYPE, NULL, data, NODEDATA_FLOAT, -1);
            } else {
                data.direct_int = int_result;
                set_ast_node_data(folded, HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
            }
            return folded;
        }
    }

    return node;
}

ASTNodePtr fold_binary_exp(ASTNodePtr node) {
    ASTNodePtr lhs, rhs;
    ASTNodePtr folded;
    NodeData data;
    int is_float, valid;
    int l_int_val = 0, r_int_val = 0, int_result = 0;
    float l_float_val = 0.0, r_float_val = 0.0, float_result = 0.0;

    if (!node) return node;
    if (node->node_type != NODE_BINARY_OP) return node;

    lhs = node->children[0];
    rhs = node->children[1];
    // 只处理左右都是常量的情况
    if (lhs && rhs && lhs->node_type == NODE_CONST && rhs->node_type == NODE_CONST) {
        is_float = (lhs->data_type == NODEDATA_FLOAT || rhs->data_type == NODEDATA_FLOAT);
        if (lhs->data_type == NODEDATA_FLOAT) {
            l_int_val = (int)lhs->data.direct_float;
            l_float_val = lhs->data.direct_float;
        } else {
            l_int_val = lhs->data.direct_int;
            l_float_val = (float)lhs->data.direct_int;
        }
        if (rhs->data_type == NODEDATA_FLOAT) {
            r_int_val = (int)rhs->data.direct_float;
            r_float_val = rhs->data.direct_float;
        } else {
            r_int_val = rhs->data.direct_int;
            r_float_val = (float)rhs->data.direct_int;
        }
        valid = 1;
        if (strcmp(node->name, "+") == 0) {
            int_result = l_int_val + r_int_val;
            float_result = l_float_val + r_float_val;
        } else if (strcmp(node->name, "-") == 0) {
            int_result = l_int_val - r_int_val;
            float_result = l_float_val - r_float_val;
        } else if (strcmp(node->name, "*") == 0) {
            int_result = l_int_val * r_int_val;
            float_result = l_float_val * r_float_val;
        } else if (strcmp(node->name, "/") == 0) {
            if (is_float) {
                if (r_float_val == 0) valid = 0;
                else float_result = l_float_val / r_float_val;
            } else {
                if (r_int_val == 0) valid = 0;
                else int_result = l_int_val / r_int_val;
            }
        } else if (strcmp(node->name, "%") == 0) {
            if (!is_float && r_int_val != 0)  {
                int_result = l_int_val % r_int_val;
            } else valid = 0;
        } else if (strcmp(node->name, "<") == 0) {
            if (is_float)
                int_result = l_float_val < r_float_val;
            else
                int_result = l_int_val < r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, ">") == 0) {
            if (is_float)
                int_result = l_float_val > r_float_val;
            else
                int_result = l_int_val > r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, "<=") == 0) {
            if (is_float)
                int_result = l_float_val <= r_float_val;
            else
                int_result = l_int_val <= r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, ">=") == 0) {
            if (is_float)
                int_result = l_float_val >= r_float_val;
            else
                int_result = l_int_val >= r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, "==") == 0) {
            if (is_float)
                int_result = l_float_val == r_float_val;
            else
                int_result = l_int_val == r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, "!=") == 0) {
            if (is_float)
                int_result = l_float_val != r_float_val;
            else
                int_result = l_int_val != r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, "&&") == 0) {
            if (is_float)
                int_result = l_float_val && r_float_val;
            else
                int_result = l_int_val && r_int_val;
            is_float = 0;
        } else if (strcmp(node->name, "||") == 0) {
            if (is_float)
                int_result = l_float_val || r_float_val;
            else
                int_result = l_int_val || r_int_val;
            is_float = 0;
        } else valid = 0;
        if (valid) {
            folded = create_ast_node(NODE_CONST, NULL, node->lineno, 0);
            if (is_float) {
                data.direct_float = float_result;
                set_ast_node_data(folded, HOLD_NODETYPE, NULL, data, NODEDATA_FLOAT, -1);
            } else {
                data.direct_int = int_result;
                set_ast_node_data(folded, HOLD_NODETYPE, NULL, data, NODEDATA_INT, -1);
            }
            return folded;
        }
    }

    return node;
}
