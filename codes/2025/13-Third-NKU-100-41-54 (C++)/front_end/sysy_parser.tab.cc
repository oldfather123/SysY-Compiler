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
#line 1 "front_end/sysy_parser.y"

#include <fstream>
#include "../include/ast.h"
#include "../include/symbol.h"
#include "../include/type.h"

int yylex();
void yyerror(char *s, ...);
int error_num = 0;
Program ASTroot;

extern int line;
// extern std::ofstream fout;

#line 86 "front_end/sysy_parser.tab.cc"

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

#include "sysy_parser.tab.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENT = 3,                      /* IDENT  */
  YYSYMBOL_FLOAT_CONST = 4,                /* FLOAT_CONST  */
  YYSYMBOL_INT_CONST = 5,                  /* INT_CONST  */
  YYSYMBOL_LEQ = 6,                        /* LEQ  */
  YYSYMBOL_GEQ = 7,                        /* GEQ  */
  YYSYMBOL_EQ = 8,                         /* EQ  */
  YYSYMBOL_NE = 9,                         /* NE  */
  YYSYMBOL_AND = 10,                       /* AND  */
  YYSYMBOL_OR = 11,                        /* OR  */
  YYSYMBOL_NOT = 12,                       /* NOT  */
  YYSYMBOL_CONST = 13,                     /* CONST  */
  YYSYMBOL_IF = 14,                        /* IF  */
  YYSYMBOL_ELSE = 15,                      /* ELSE  */
  YYSYMBOL_WHILE = 16,                     /* WHILE  */
  YYSYMBOL_NONE_TYPE = 17,                 /* NONE_TYPE  */
  YYSYMBOL_INT = 18,                       /* INT  */
  YYSYMBOL_FLOAT = 19,                     /* FLOAT  */
  YYSYMBOL_RETURN = 20,                    /* RETURN  */
  YYSYMBOL_BREAK = 21,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 22,                  /* CONTINUE  */
  YYSYMBOL_ERROR = 23,                     /* ERROR  */
  YYSYMBOL_THEN = 24,                      /* THEN  */
  YYSYMBOL_25_ = 25,                       /* ','  */
  YYSYMBOL_26_ = 26,                       /* '*'  */
  YYSYMBOL_27_ = 27,                       /* '/'  */
  YYSYMBOL_28_ = 28,                       /* '%'  */
  YYSYMBOL_29_ = 29,                       /* '+'  */
  YYSYMBOL_30_ = 30,                       /* '-'  */
  YYSYMBOL_31_ = 31,                       /* '<'  */
  YYSYMBOL_32_ = 32,                       /* '>'  */
  YYSYMBOL_33_ = 33,                       /* '('  */
  YYSYMBOL_34_ = 34,                       /* ')'  */
  YYSYMBOL_35_ = 35,                       /* '!'  */
  YYSYMBOL_36_ = 36,                       /* '['  */
  YYSYMBOL_37_ = 37,                       /* ']'  */
  YYSYMBOL_38_ = 38,                       /* '='  */
  YYSYMBOL_39_ = 39,                       /* ';'  */
  YYSYMBOL_40_ = 40,                       /* '{'  */
  YYSYMBOL_41_ = 41,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 42,                  /* $accept  */
  YYSYMBOL_Program = 43,                   /* Program  */
  YYSYMBOL_CompUnit = 44,                  /* CompUnit  */
  YYSYMBOL_Comp_list = 45,                 /* Comp_list  */
  YYSYMBOL_Exp_list = 46,                  /* Exp_list  */
  YYSYMBOL_Exp = 47,                       /* Exp  */
  YYSYMBOL_ConstExp = 48,                  /* ConstExp  */
  YYSYMBOL_MulExp = 49,                    /* MulExp  */
  YYSYMBOL_AddExp = 50,                    /* AddExp  */
  YYSYMBOL_RelExp = 51,                    /* RelExp  */
  YYSYMBOL_EqExp = 52,                     /* EqExp  */
  YYSYMBOL_LAndExp = 53,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 54,                    /* LOrExp  */
  YYSYMBOL_Cond = 55,                      /* Cond  */
  YYSYMBOL_PrimaryExp = 56,                /* PrimaryExp  */
  YYSYMBOL_IntConst = 57,                  /* IntConst  */
  YYSYMBOL_FloatConst = 58,                /* FloatConst  */
  YYSYMBOL_Lval = 59,                      /* Lval  */
  YYSYMBOL_UnaryExp = 60,                  /* UnaryExp  */
  YYSYMBOL_FuncRParams = 61,               /* FuncRParams  */
  YYSYMBOL_FuncFParams = 62,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 63,                /* FuncFParam  */
  YYSYMBOL_FuncDef = 64,                   /* FuncDef  */
  YYSYMBOL_Stmt = 65,                      /* Stmt  */
  YYSYMBOL_ConstInitVal_list = 66,         /* ConstInitVal_list  */
  YYSYMBOL_ConstInitVal = 67,              /* ConstInitVal  */
  YYSYMBOL_VarInitVal_list = 68,           /* VarInitVal_list  */
  YYSYMBOL_VarInitVal = 69,                /* VarInitVal  */
  YYSYMBOL_VarDef = 70,                    /* VarDef  */
  YYSYMBOL_ConstDef = 71,                  /* ConstDef  */
  YYSYMBOL_ArrayDim = 72,                  /* ArrayDim  */
  YYSYMBOL_ArrayDim_list = 73,             /* ArrayDim_list  */
  YYSYMBOL_Decl = 74,                      /* Decl  */
  YYSYMBOL_VarDecl = 75,                   /* VarDecl  */
  YYSYMBOL_VarDef_list = 76,               /* VarDef_list  */
  YYSYMBOL_ConstDecl = 77,                 /* ConstDecl  */
  YYSYMBOL_ConstDef_list = 78,             /* ConstDef_list  */
  YYSYMBOL_Block = 79,                     /* Block  */
  YYSYMBOL_BlockItem_list = 80,            /* BlockItem_list  */
  YYSYMBOL_BlockItem = 81                  /* BlockItem  */
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
#define YYFINAL  20
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   299

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  105
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  195

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   279


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
       2,     2,     2,    35,     2,     2,     2,    28,     2,     2,
      33,    34,    26,    29,    25,    30,     2,    27,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    39,
      31,    38,    32,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    36,     2,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    40,     2,    41,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    74,    74,    82,    83,    86,    87,    92,    93,    96,
      99,   102,   103,   104,   105,   108,   109,   110,   114,   115,
     116,   117,   118,   121,   122,   123,   126,   127,   130,   131,
     134,   140,   143,   146,   149,   155,   158,   161,   165,   171,
     172,   175,   196,   197,   198,   204,   208,   209,   212,   217,
     222,   229,   236,   242,   251,   257,   263,   269,   275,   281,
     290,   292,   294,   296,   298,   300,   302,   304,   306,   308,
     310,   316,   318,   322,   324,   326,   330,   332,   336,   338,
     340,   347,   349,   352,   354,   358,   360,   365,   369,   371,
     377,   379,   384,   387,   392,   394,   399,   402,   407,   409,
     415,   417,   422,   424,   429,   431
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
  "\"end of file\"", "error", "\"invalid token\"", "IDENT", "FLOAT_CONST",
  "INT_CONST", "LEQ", "GEQ", "EQ", "NE", "AND", "OR", "NOT", "CONST", "IF",
  "ELSE", "WHILE", "NONE_TYPE", "INT", "FLOAT", "RETURN", "BREAK",
  "CONTINUE", "ERROR", "THEN", "','", "'*'", "'/'", "'%'", "'+'", "'-'",
  "'<'", "'>'", "'('", "')'", "'!'", "'['", "']'", "'='", "';'", "'{'",
  "'}'", "$accept", "Program", "CompUnit", "Comp_list", "Exp_list", "Exp",
  "ConstExp", "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp",
  "Cond", "PrimaryExp", "IntConst", "FloatConst", "Lval", "UnaryExp",
  "FuncRParams", "FuncFParams", "FuncFParam", "FuncDef", "Stmt",
  "ConstInitVal_list", "ConstInitVal", "VarInitVal_list", "VarInitVal",
  "VarDef", "ConstDef", "ArrayDim", "ArrayDim_list", "Decl", "VarDecl",
  "VarDef_list", "ConstDecl", "ConstDef_list", "Block", "BlockItem_list",
  "BlockItem", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-98)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     104,    92,    12,    49,    70,    86,   -98,   104,   -98,   -98,
     -98,   -98,    90,    90,    66,   101,   -98,     2,   122,    38,
     -98,   -98,    20,   -98,    50,    51,    -2,     4,   264,    62,
     -98,   121,   102,   -98,    36,   -98,   103,   128,    90,   -98,
     -98,   112,   117,   100,     3,   -98,   100,    60,    65,   -98,
     -98,   264,   264,   264,   264,    98,     7,    95,   -98,   -98,
     -98,   -98,   -98,    39,   -98,    95,   -98,    62,   -98,   136,
     -98,   100,    75,   202,   -98,   -98,   103,   -98,   120,   125,
     149,   -98,   123,   100,   -98,   100,   257,   137,   -98,   -98,
     143,   -98,   -98,   264,   264,   264,   264,   264,   -98,   -12,
     -98,   -98,   -98,   100,   -98,     5,   -98,   -98,   146,   156,
     161,   164,   102,   102,   244,   160,   169,   -98,   -98,   170,
     172,   -98,   -98,   -98,   182,   -98,   -98,   -98,   -98,   -98,
     188,   -98,   180,   -98,   -98,   -98,   -98,     7,     7,    62,
     -98,   -98,   103,   -98,   137,   137,   264,   264,   -98,   177,
     -98,   -98,   -98,   264,   -98,   -98,   264,   -98,   -98,   -98,
     137,   137,    95,    18,   167,   208,   209,   185,   190,   -98,
     186,   -98,   264,   264,   264,   264,   264,   264,   264,   264,
     224,   224,   -98,    95,    95,    95,    95,    18,    18,   167,
     208,   211,   -98,   224,   -98
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     5,     2,     4,     3,
      90,    91,     0,     0,     0,    82,    94,     0,    82,     0,
       1,     6,     0,    98,     0,     0,     0,     0,     0,     0,
      88,    83,     0,    92,     0,    93,     0,     0,     0,    96,
      97,     0,     0,     0,     0,    46,     0,     0,    37,    36,
      35,     0,     0,     0,     0,     0,    15,    10,    39,    31,
      32,    33,    11,     0,    78,     9,    81,     0,    89,    82,
      95,     0,     0,     0,    73,    85,     0,    99,    48,    49,
       0,    59,     0,     0,    55,     0,     0,    38,    42,    43,
       0,    44,    87,     0,     0,     0,     0,     0,    80,     0,
      76,    84,    57,     0,    75,     0,    71,    86,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    62,   101,     0,
      33,   105,   104,    63,     0,   102,    47,    58,    54,    41,
      45,     7,     0,    34,    12,    13,    14,    16,    17,     0,
      79,    56,     0,    74,    50,    51,     0,     0,    70,     0,
      67,    68,    61,     0,   100,   103,     0,    40,    77,    72,
      52,    53,    18,    23,    26,    28,    30,     0,     0,    69,
       0,     8,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    60,    21,    22,    19,    20,    24,    25,    27,
      29,    64,    66,     0,    65
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -98,   -98,   223,   -98,   -98,   -27,   205,    84,   -28,    15,
      56,    57,   -98,    94,   -98,   -98,   -98,   -77,   -33,   -98,
      54,   157,   -98,   -97,   -98,   -64,   -98,   -57,   218,   213,
     -30,   -17,   -60,   -98,     0,   -98,   239,   -32,   -98,   131
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,     7,   130,   119,    74,    56,    65,   163,
     164,   165,   166,   167,    58,    59,    60,    61,    62,   132,
      44,    45,     8,   121,   105,    75,    99,    66,    16,    23,
      30,    31,     9,    10,    17,    11,    24,   123,   124,   125
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      57,    68,    64,   120,    19,    37,   100,    68,    57,   106,
     101,    81,   107,   139,    84,    14,    41,    42,    88,    89,
     122,    91,    41,    42,   172,   173,    90,    32,    82,   140,
     142,    87,    43,    93,    94,    95,    64,    83,    46,   102,
      64,    33,    48,    49,    50,    57,   143,   120,    57,   174,
     175,   127,    15,   128,    41,    42,    28,    68,    36,   131,
     134,   135,   136,    32,   122,    48,    49,    50,    51,    52,
      71,   141,    53,    18,    54,    38,    38,    35,   159,    63,
      98,    47,   158,   191,   192,    82,    20,   149,    72,    39,
      40,    51,    52,    22,    85,    53,   194,    54,    86,    26,
      82,    28,    63,   120,   120,    69,    48,    49,    50,   103,
      12,    13,    64,    19,    57,    78,   120,     1,   162,   162,
      79,     2,     3,     4,    96,    97,   170,   160,   161,   171,
      68,    68,    51,    52,    27,    92,    53,    28,    54,    29,
      80,    41,    42,    73,   183,   184,   185,   186,   162,   162,
     162,   162,    48,    49,    50,    34,   108,    28,    28,    67,
      29,   109,     1,   110,    28,   111,    76,   112,   113,   114,
     115,   116,    28,    28,    29,   176,   177,   133,    51,    52,
     137,   138,    53,   144,    54,    48,    49,    50,   117,    80,
     118,   187,   188,   145,   146,     1,   110,   147,   111,   150,
     112,   113,   114,   115,   116,    48,    49,    50,   151,   152,
     153,    51,    52,   156,   157,    53,   169,    54,   178,   180,
     179,   117,    80,   154,   181,   182,   193,    48,    49,    50,
      21,    51,    52,    55,   189,    53,   190,    54,   110,   126,
     111,   168,    73,   104,   114,   115,   116,    48,    49,    50,
      70,    77,    25,    51,    52,   155,     0,    53,     0,    54,
      48,    49,    50,   117,    80,     0,     0,    48,    49,    50,
       0,     0,     0,    51,    52,     0,     0,    53,     0,    54,
       0,     0,     0,   148,     0,     0,    51,    52,     0,     0,
      53,   129,    54,    51,    52,     0,     0,    53,     0,    54
};

static const yytype_int16 yycheck[] =
{
      28,    31,    29,    80,     4,    22,    63,    37,    36,    73,
      67,    43,    76,    25,    46,     3,    18,    19,    51,    52,
      80,    54,    18,    19,     6,     7,    53,    25,    25,    41,
      25,    48,    34,    26,    27,    28,    63,    34,    34,    71,
      67,    39,     3,     4,     5,    73,    41,   124,    76,    31,
      32,    83,     3,    85,    18,    19,    36,    87,    38,    86,
      93,    94,    95,    25,   124,     3,     4,     5,    29,    30,
      34,   103,    33,     3,    35,    25,    25,    39,   142,    40,
      41,    27,   139,   180,   181,    25,     0,   114,    34,    39,
      39,    29,    30,     3,    34,    33,   193,    35,    33,    33,
      25,    36,    40,   180,   181,     3,     3,     4,     5,    34,
      18,    19,   139,   113,   142,     3,   193,    13,   146,   147,
       3,    17,    18,    19,    29,    30,   153,   144,   145,   156,
     160,   161,    29,    30,    33,    37,    33,    36,    35,    38,
      40,    18,    19,    40,   172,   173,   174,   175,   176,   177,
     178,   179,     3,     4,     5,    33,    36,    36,    36,    38,
      38,    36,    13,    14,    36,    16,    38,    18,    19,    20,
      21,    22,    36,    36,    38,     8,     9,    34,    29,    30,
      96,    97,    33,    37,    35,     3,     4,     5,    39,    40,
      41,   176,   177,    37,    33,    13,    14,    33,    16,    39,
      18,    19,    20,    21,    22,     3,     4,     5,    39,    39,
      38,    29,    30,    25,    34,    33,    39,    35,    10,    34,
      11,    39,    40,    41,    34,    39,    15,     3,     4,     5,
       7,    29,    30,    28,   178,    33,   179,    35,    14,    82,
      16,   147,    40,    41,    20,    21,    22,     3,     4,     5,
      32,    38,    13,    29,    30,   124,    -1,    33,    -1,    35,
       3,     4,     5,    39,    40,    -1,    -1,     3,     4,     5,
      -1,    -1,    -1,    29,    30,    -1,    -1,    33,    -1,    35,
      -1,    -1,    -1,    39,    -1,    -1,    29,    30,    -1,    -1,
      33,    34,    35,    29,    30,    -1,    -1,    33,    -1,    35
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    13,    17,    18,    19,    43,    44,    45,    64,    74,
      75,    77,    18,    19,     3,     3,    70,    76,     3,    76,
       0,    44,     3,    71,    78,    78,    33,    33,    36,    38,
      72,    73,    25,    39,    33,    39,    38,    73,    25,    39,
      39,    18,    19,    34,    62,    63,    34,    62,     3,     4,
       5,    29,    30,    33,    35,    48,    49,    50,    56,    57,
      58,    59,    60,    40,    47,    50,    69,    38,    72,     3,
      70,    34,    62,    40,    48,    67,    38,    71,     3,     3,
      40,    79,    25,    34,    79,    34,    33,    73,    60,    60,
      47,    60,    37,    26,    27,    28,    29,    30,    41,    68,
      69,    69,    79,    34,    41,    66,    67,    67,    36,    36,
      14,    16,    18,    19,    20,    21,    22,    39,    41,    47,
      59,    65,    74,    79,    80,    81,    63,    79,    79,    34,
      46,    47,    61,    34,    60,    60,    60,    49,    49,    25,
      41,    79,    25,    41,    37,    37,    33,    33,    39,    47,
      39,    39,    39,    38,    41,    81,    25,    34,    69,    67,
      73,    73,    50,    51,    52,    53,    54,    55,    55,    39,
      47,    47,     6,     7,    31,    32,     8,     9,    10,    11,
      34,    34,    39,    50,    50,    50,    50,    51,    51,    52,
      53,    65,    65,    15,    65
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    42,    43,    44,    44,    45,    45,    46,    46,    47,
      48,    49,    49,    49,    49,    50,    50,    50,    51,    51,
      51,    51,    51,    52,    52,    52,    53,    53,    54,    54,
      55,    56,    56,    56,    56,    57,    58,    59,    59,    60,
      60,    60,    60,    60,    60,    61,    62,    62,    63,    63,
      63,    63,    63,    63,    64,    64,    64,    64,    64,    64,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    66,    66,    67,    67,    67,    68,    68,    69,    69,
      69,    70,    70,    70,    70,    71,    71,    72,    73,    73,
      74,    74,    75,    75,    76,    76,    77,    77,    78,    78,
      79,    79,    80,    80,    81,    81
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     2,     1,     3,     1,
       1,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     3,     1,     3,     3,     1,     3,     1,     3,
       1,     1,     1,     1,     3,     1,     1,     1,     2,     1,
       4,     3,     2,     2,     2,     1,     1,     3,     2,     2,
       4,     4,     5,     5,     6,     5,     6,     5,     6,     5,
       4,     2,     1,     1,     5,     7,     5,     2,     2,     3,
       2,     1,     3,     1,     3,     2,     1,     3,     1,     3,
       2,     3,     1,     2,     4,     3,     4,     3,     1,     2,
       1,     1,     3,     3,     1,     3,     4,     4,     1,     3,
       3,     2,     1,     2,     1,     1
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
  case 2: /* Program: Comp_list  */
#line 75 "front_end/sysy_parser.y"
{
    (yyloc) = (yylsp[0]);      
    ASTroot = new __Program((yyvsp[0].comp_list));
    ASTroot->SetLineNumber(line);
}
#line 1424 "front_end/sysy_parser.tab.cc"
    break;

  case 3: /* CompUnit: Decl  */
#line 82 "front_end/sysy_parser.y"
      {(yyval.comp_unit) = new CompUnit_Decl((yyvsp[0].decl)); (yyval.comp_unit)->SetLineNumber(line);}
#line 1430 "front_end/sysy_parser.tab.cc"
    break;

  case 4: /* CompUnit: FuncDef  */
#line 83 "front_end/sysy_parser.y"
         {(yyval.comp_unit) = new CompUnit_FuncDef((yyvsp[0].func_def)); (yyval.comp_unit)->SetLineNumber(line);}
#line 1436 "front_end/sysy_parser.tab.cc"
    break;

  case 5: /* Comp_list: CompUnit  */
#line 86 "front_end/sysy_parser.y"
          {(yyval.comp_list) = new std::vector<CompUnit>; ((yyval.comp_list))->push_back((yyvsp[0].comp_unit));}
#line 1442 "front_end/sysy_parser.tab.cc"
    break;

  case 6: /* Comp_list: Comp_list CompUnit  */
#line 87 "front_end/sysy_parser.y"
                    {((yyvsp[-1].comp_list))->push_back((yyvsp[0].comp_unit)); (yyval.comp_list) = (yyvsp[-1].comp_list);}
#line 1448 "front_end/sysy_parser.tab.cc"
    break;

  case 7: /* Exp_list: Exp  */
#line 92 "front_end/sysy_parser.y"
    {(yyval.expression_list) = new std::vector<ExprBase>;((yyval.expression_list))->push_back((yyvsp[0].expression));}
#line 1454 "front_end/sysy_parser.tab.cc"
    break;

  case 8: /* Exp_list: Exp_list ',' Exp  */
#line 93 "front_end/sysy_parser.y"
                 {((yyvsp[-2].expression_list))->push_back((yyvsp[0].expression));(yyval.expression_list) = (yyvsp[-2].expression_list);}
#line 1460 "front_end/sysy_parser.tab.cc"
    break;

  case 9: /* Exp: AddExp  */
#line 96 "front_end/sysy_parser.y"
       { (yyval.expression) = (yyvsp[0].expression); (yyval.expression)->SetLineNumber(line);}
#line 1466 "front_end/sysy_parser.tab.cc"
    break;

  case 10: /* ConstExp: AddExp  */
#line 99 "front_end/sysy_parser.y"
       { (yyval.expression) = (yyvsp[0].expression);(yyval.expression)->SetLineNumber(line);}
#line 1472 "front_end/sysy_parser.tab.cc"
    break;

  case 11: /* MulExp: UnaryExp  */
#line 102 "front_end/sysy_parser.y"
         { (yyval.expression) = (yyvsp[0].expression); (yyval.expression)->SetLineNumber(line);}
#line 1478 "front_end/sysy_parser.tab.cc"
    break;

  case 12: /* MulExp: MulExp '*' UnaryExp  */
#line 103 "front_end/sysy_parser.y"
                    {(yyval.expression) = new MulExp((yyvsp[-2].expression),OpType::Mul,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1484 "front_end/sysy_parser.tab.cc"
    break;

  case 13: /* MulExp: MulExp '/' UnaryExp  */
#line 104 "front_end/sysy_parser.y"
                    {(yyval.expression) = new MulExp((yyvsp[-2].expression),OpType::Div,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1490 "front_end/sysy_parser.tab.cc"
    break;

  case 14: /* MulExp: MulExp '%' UnaryExp  */
#line 105 "front_end/sysy_parser.y"
                    {(yyval.expression) = new MulExp((yyvsp[-2].expression),OpType::Mod,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1496 "front_end/sysy_parser.tab.cc"
    break;

  case 15: /* AddExp: MulExp  */
#line 108 "front_end/sysy_parser.y"
       { (yyval.expression) = (yyvsp[0].expression); (yyval.expression)->SetLineNumber(line);}
#line 1502 "front_end/sysy_parser.tab.cc"
    break;

  case 16: /* AddExp: AddExp '+' MulExp  */
#line 109 "front_end/sysy_parser.y"
                  {(yyval.expression) = new AddExp((yyvsp[-2].expression),OpType::Add,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1508 "front_end/sysy_parser.tab.cc"
    break;

  case 17: /* AddExp: AddExp '-' MulExp  */
#line 110 "front_end/sysy_parser.y"
                  {(yyval.expression) = new AddExp((yyvsp[-2].expression),OpType::Sub,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1514 "front_end/sysy_parser.tab.cc"
    break;

  case 18: /* RelExp: AddExp  */
#line 114 "front_end/sysy_parser.y"
       {(yyval.expression) = (yyvsp[0].expression);(yyval.expression)->SetLineNumber(line);}
#line 1520 "front_end/sysy_parser.tab.cc"
    break;

  case 19: /* RelExp: RelExp '<' AddExp  */
#line 115 "front_end/sysy_parser.y"
                  {(yyval.expression) = new RelExp((yyvsp[-2].expression),OpType::Lt,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1526 "front_end/sysy_parser.tab.cc"
    break;

  case 20: /* RelExp: RelExp '>' AddExp  */
#line 116 "front_end/sysy_parser.y"
                  {(yyval.expression) = new RelExp((yyvsp[-2].expression),OpType::Gt,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1532 "front_end/sysy_parser.tab.cc"
    break;

  case 21: /* RelExp: RelExp LEQ AddExp  */
#line 117 "front_end/sysy_parser.y"
                  {(yyval.expression) = new RelExp((yyvsp[-2].expression),OpType::Le,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1538 "front_end/sysy_parser.tab.cc"
    break;

  case 22: /* RelExp: RelExp GEQ AddExp  */
#line 118 "front_end/sysy_parser.y"
                  {(yyval.expression) = new RelExp((yyvsp[-2].expression),OpType::Ge,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1544 "front_end/sysy_parser.tab.cc"
    break;

  case 23: /* EqExp: RelExp  */
#line 121 "front_end/sysy_parser.y"
       {(yyval.expression) = (yyvsp[0].expression);(yyval.expression)->SetLineNumber(line);}
#line 1550 "front_end/sysy_parser.tab.cc"
    break;

  case 24: /* EqExp: EqExp EQ RelExp  */
#line 122 "front_end/sysy_parser.y"
                {(yyval.expression) = new EqExp((yyvsp[-2].expression),OpType::Eq,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1556 "front_end/sysy_parser.tab.cc"
    break;

  case 25: /* EqExp: EqExp NE RelExp  */
#line 123 "front_end/sysy_parser.y"
                {(yyval.expression) = new EqExp((yyvsp[-2].expression),OpType::Neq,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1562 "front_end/sysy_parser.tab.cc"
    break;

  case 26: /* LAndExp: EqExp  */
#line 126 "front_end/sysy_parser.y"
      {(yyval.expression) = (yyvsp[0].expression);(yyval.expression)->SetLineNumber(line);}
#line 1568 "front_end/sysy_parser.tab.cc"
    break;

  case 27: /* LAndExp: LAndExp AND EqExp  */
#line 127 "front_end/sysy_parser.y"
                  {(yyval.expression) = new LAndExp((yyvsp[-2].expression),OpType::And,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1574 "front_end/sysy_parser.tab.cc"
    break;

  case 28: /* LOrExp: LAndExp  */
#line 130 "front_end/sysy_parser.y"
        {(yyval.expression) = (yyvsp[0].expression);(yyval.expression)->SetLineNumber(line);}
#line 1580 "front_end/sysy_parser.tab.cc"
    break;

  case 29: /* LOrExp: LOrExp OR LAndExp  */
#line 131 "front_end/sysy_parser.y"
                  {(yyval.expression) = new LOrExp((yyvsp[-2].expression),OpType::Or,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1586 "front_end/sysy_parser.tab.cc"
    break;

  case 30: /* Cond: LOrExp  */
#line 134 "front_end/sysy_parser.y"
       { (yyval.expression) = (yyvsp[0].expression); (yyval.expression)->SetLineNumber(line);}
#line 1592 "front_end/sysy_parser.tab.cc"
    break;

  case 31: /* PrimaryExp: IntConst  */
#line 140 "front_end/sysy_parser.y"
         { 
    (yyval.expression) = (yyvsp[0].expression); 
    (yyval.expression)->SetLineNumber(line);}
#line 1600 "front_end/sysy_parser.tab.cc"
    break;

  case 32: /* PrimaryExp: FloatConst  */
#line 143 "front_end/sysy_parser.y"
           {
    (yyval.expression) = (yyvsp[0].expression); 
    (yyval.expression)->SetLineNumber(line);}
#line 1608 "front_end/sysy_parser.tab.cc"
    break;

  case 33: /* PrimaryExp: Lval  */
#line 146 "front_end/sysy_parser.y"
     {
    (yyval.expression) = (yyvsp[0].expression); 
    (yyval.expression)->SetLineNumber(line);}
#line 1616 "front_end/sysy_parser.tab.cc"
    break;

  case 34: /* PrimaryExp: '(' Exp ')'  */
#line 149 "front_end/sysy_parser.y"
            {
    (yyval.expression) = new PrimaryExp((yyvsp[-1].expression));
    (yyval.expression)->SetLineNumber(line);
}
#line 1625 "front_end/sysy_parser.tab.cc"
    break;

  case 35: /* IntConst: INT_CONST  */
#line 155 "front_end/sysy_parser.y"
          {(yyval.expression) = new IntConst((yyvsp[0].int_token));(yyval.expression)->SetLineNumber(line);}
#line 1631 "front_end/sysy_parser.tab.cc"
    break;

  case 36: /* FloatConst: FLOAT_CONST  */
#line 158 "front_end/sysy_parser.y"
            {(yyval.expression) = new FloatConst((yyvsp[0].float_token));(yyval.expression)->SetLineNumber(line);}
#line 1637 "front_end/sysy_parser.tab.cc"
    break;

  case 37: /* Lval: IDENT  */
#line 161 "front_end/sysy_parser.y"
      {//普通变量
    (yyval.expression) = new Lval((yyvsp[0].symbol_token),nullptr);
    (yyval.expression)->SetLineNumber(line);
}
#line 1646 "front_end/sysy_parser.tab.cc"
    break;

  case 38: /* Lval: IDENT ArrayDim_list  */
#line 165 "front_end/sysy_parser.y"
                    { //数组
    (yyval.expression) = new Lval((yyvsp[-1].symbol_token),(yyvsp[0].expression_list));
    (yyval.expression)->SetLineNumber(line);}
#line 1654 "front_end/sysy_parser.tab.cc"
    break;

  case 39: /* UnaryExp: PrimaryExp  */
#line 171 "front_end/sysy_parser.y"
           { (yyval.expression) = (yyvsp[0].expression); }
#line 1660 "front_end/sysy_parser.tab.cc"
    break;

  case 40: /* UnaryExp: IDENT '(' FuncRParams ')'  */
#line 172 "front_end/sysy_parser.y"
                          {
    (yyval.expression) = new FuncCall((yyvsp[-3].symbol_token),(yyvsp[-1].expression));
    (yyval.expression)->SetLineNumber(line);}
#line 1668 "front_end/sysy_parser.tab.cc"
    break;

  case 41: /* UnaryExp: IDENT '(' ')'  */
#line 175 "front_end/sysy_parser.y"
              {
    // 将sylib.h中的宏定义starttime()替换为_sysy_starttime(line)
    if((yyvsp[-2].symbol_token)->getName() == "starttime"){
        auto params = new std::vector<ExprBase>;
        params->push_back(new IntConst(line));
        ExprBase temp = new FuncRParams(params);
        (yyval.expression) = new FuncCall(new Symbol("_sysy_starttime"),temp);
        (yyval.expression)->SetLineNumber(line);
    }
    else if((yyvsp[-2].symbol_token)->getName() == "stoptime"){
        auto params = new std::vector<ExprBase>;
        params->push_back(new IntConst(line));
        ExprBase temp = new FuncRParams(params);
        (yyval.expression) = new FuncCall(new Symbol("_sysy_stoptime"),temp);
        (yyval.expression)->SetLineNumber(line);
    }
    else{
        (yyval.expression) = new FuncCall((yyvsp[-2].symbol_token),nullptr);
        (yyval.expression)->SetLineNumber(line);
    }
}
#line 1694 "front_end/sysy_parser.tab.cc"
    break;

  case 42: /* UnaryExp: '+' UnaryExp  */
#line 196 "front_end/sysy_parser.y"
             { (yyval.expression) = new UnaryExp(OpType::Add,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1700 "front_end/sysy_parser.tab.cc"
    break;

  case 43: /* UnaryExp: '-' UnaryExp  */
#line 197 "front_end/sysy_parser.y"
             { (yyval.expression) = new UnaryExp(OpType::Sub,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1706 "front_end/sysy_parser.tab.cc"
    break;

  case 44: /* UnaryExp: '!' UnaryExp  */
#line 198 "front_end/sysy_parser.y"
             { (yyval.expression) = new UnaryExp(OpType::Not,(yyvsp[0].expression)); (yyval.expression)->SetLineNumber(line);}
#line 1712 "front_end/sysy_parser.tab.cc"
    break;

  case 45: /* FuncRParams: Exp_list  */
#line 204 "front_end/sysy_parser.y"
         { (yyval.expression) = new FuncRParams((yyvsp[0].expression_list));(yyval.expression)->SetLineNumber(line);}
#line 1718 "front_end/sysy_parser.tab.cc"
    break;

  case 46: /* FuncFParams: FuncFParam  */
#line 208 "front_end/sysy_parser.y"
           { (yyval.formal_list) = new std::vector<FuncFParam>;((yyval.formal_list))->push_back((yyvsp[0].formal));}
#line 1724 "front_end/sysy_parser.tab.cc"
    break;

  case 47: /* FuncFParams: FuncFParams ',' FuncFParam  */
#line 209 "front_end/sysy_parser.y"
                           { ((yyvsp[-2].formal_list))->push_back((yyvsp[0].formal));(yyval.formal_list) = (yyvsp[-2].formal_list);}
#line 1730 "front_end/sysy_parser.tab.cc"
    break;

  case 48: /* FuncFParam: INT IDENT  */
#line 212 "front_end/sysy_parser.y"
          {
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[0].symbol_token),nullptr);
    (yyval.formal)->SetLineNumber(line);
}
#line 1740 "front_end/sysy_parser.tab.cc"
    break;

  case 49: /* FuncFParam: FLOAT IDENT  */
#line 217 "front_end/sysy_parser.y"
            {
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[0].symbol_token),nullptr);
    (yyval.formal)->SetLineNumber(line);
}
#line 1750 "front_end/sysy_parser.tab.cc"
    break;

  case 50: /* FuncFParam: INT IDENT '[' ']'  */
#line 222 "front_end/sysy_parser.y"
                    {
    std::vector<ExprBase>* temp = new std::vector<ExprBase>;
    temp->push_back(nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[-2].symbol_token),temp);
    (yyval.formal)->SetLineNumber(line);
}
#line 1762 "front_end/sysy_parser.tab.cc"
    break;

  case 51: /* FuncFParam: FLOAT IDENT '[' ']'  */
#line 229 "front_end/sysy_parser.y"
                      {
    std::vector<ExprBase>* temp = new std::vector<ExprBase>;
    temp->push_back(nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[-2].symbol_token),temp);
    (yyval.formal)->SetLineNumber(line);
}
#line 1774 "front_end/sysy_parser.tab.cc"
    break;

  case 52: /* FuncFParam: INT IDENT '[' ']' ArrayDim_list  */
#line 236 "front_end/sysy_parser.y"
                                {
    (yyvsp[0].expression_list)->insert((yyvsp[0].expression_list)->begin(),nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[-3].symbol_token),(yyvsp[0].expression_list));
    (yyval.formal)->SetLineNumber(line);
}
#line 1785 "front_end/sysy_parser.tab.cc"
    break;

  case 53: /* FuncFParam: FLOAT IDENT '[' ']' ArrayDim_list  */
#line 242 "front_end/sysy_parser.y"
                                  {
    (yyvsp[0].expression_list)->insert((yyvsp[0].expression_list)->begin(),nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.formal) = new __FuncFParam(type,(yyvsp[-3].symbol_token),(yyvsp[0].expression_list));
    (yyval.formal)->SetLineNumber(line);
}
#line 1796 "front_end/sysy_parser.tab.cc"
    break;

  case 54: /* FuncDef: INT IDENT '(' FuncFParams ')' Block  */
#line 252 "front_end/sysy_parser.y"
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-4].symbol_token),(yyvsp[-2].formal_list),(yyvsp[0].block));
    (yyval.func_def)->SetLineNumber(line);
}
#line 1806 "front_end/sysy_parser.tab.cc"
    break;

  case 55: /* FuncDef: INT IDENT '(' ')' Block  */
#line 258 "front_end/sysy_parser.y"
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-3].symbol_token),new std::vector<FuncFParam>(),(yyvsp[0].block)); 
    (yyval.func_def)->SetLineNumber(line);
}
#line 1816 "front_end/sysy_parser.tab.cc"
    break;

  case 56: /* FuncDef: FLOAT IDENT '(' FuncFParams ')' Block  */
#line 264 "front_end/sysy_parser.y"
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-4].symbol_token),(yyvsp[-2].formal_list),(yyvsp[0].block));
    (yyval.func_def)->SetLineNumber(line);
}
#line 1826 "front_end/sysy_parser.tab.cc"
    break;

  case 57: /* FuncDef: FLOAT IDENT '(' ')' Block  */
#line 270 "front_end/sysy_parser.y"
{
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-3].symbol_token),new std::vector<FuncFParam>(),(yyvsp[0].block)); 
    (yyval.func_def)->SetLineNumber(line);
}
#line 1836 "front_end/sysy_parser.tab.cc"
    break;

  case 58: /* FuncDef: NONE_TYPE IDENT '(' FuncFParams ')' Block  */
#line 276 "front_end/sysy_parser.y"
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Void);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-4].symbol_token),(yyvsp[-2].formal_list),(yyvsp[0].block));
    (yyval.func_def)->SetLineNumber(line);
}
#line 1846 "front_end/sysy_parser.tab.cc"
    break;

  case 59: /* FuncDef: NONE_TYPE IDENT '(' ')' Block  */
#line 282 "front_end/sysy_parser.y"
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Void);
    (yyval.func_def) = new __FuncDef(type,(yyvsp[-3].symbol_token),new std::vector<FuncFParam>(),(yyvsp[0].block)); 
    (yyval.func_def)->SetLineNumber(line);
}
#line 1856 "front_end/sysy_parser.tab.cc"
    break;

  case 60: /* Stmt: Lval '=' Exp ';'  */
#line 290 "front_end/sysy_parser.y"
                 {//赋值
    (yyval.stmt) = new AssignStmt((yyvsp[-3].expression),(yyvsp[-1].expression));(yyval.stmt)->SetLineNumber(line);}
#line 1863 "front_end/sysy_parser.tab.cc"
    break;

  case 61: /* Stmt: Exp ';'  */
#line 292 "front_end/sysy_parser.y"
        {
    (yyval.stmt) = new ExprStmt((yyvsp[-1].expression)); (yyval.stmt)->SetLineNumber(line);}
#line 1870 "front_end/sysy_parser.tab.cc"
    break;

  case 62: /* Stmt: ';'  */
#line 294 "front_end/sysy_parser.y"
    {
    (yyval.stmt) = new ExprStmt(NULL);(yyval.stmt)->SetLineNumber(line);}
#line 1877 "front_end/sysy_parser.tab.cc"
    break;

  case 63: /* Stmt: Block  */
#line 296 "front_end/sysy_parser.y"
      {
    (yyval.stmt) = new BlockStmt((yyvsp[0].block));(yyval.stmt)->SetLineNumber(line);}
#line 1884 "front_end/sysy_parser.tab.cc"
    break;

  case 64: /* Stmt: IF '(' Cond ')' Stmt  */
#line 298 "front_end/sysy_parser.y"
                                {
    (yyval.stmt) = new IfStmt((yyvsp[-2].expression),(yyvsp[0].stmt),NULL);(yyval.stmt)->SetLineNumber(line);}
#line 1891 "front_end/sysy_parser.tab.cc"
    break;

  case 65: /* Stmt: IF '(' Cond ')' Stmt ELSE Stmt  */
#line 300 "front_end/sysy_parser.y"
                               {
    (yyval.stmt) = new IfStmt((yyvsp[-4].expression),(yyvsp[-2].stmt),(yyvsp[0].stmt));(yyval.stmt)->SetLineNumber(line);}
#line 1898 "front_end/sysy_parser.tab.cc"
    break;

  case 66: /* Stmt: WHILE '(' Cond ')' Stmt  */
#line 302 "front_end/sysy_parser.y"
                        {
    (yyval.stmt) = new WhileStmt((yyvsp[-2].expression),(yyvsp[0].stmt));(yyval.stmt)->SetLineNumber(line);}
#line 1905 "front_end/sysy_parser.tab.cc"
    break;

  case 67: /* Stmt: BREAK ';'  */
#line 304 "front_end/sysy_parser.y"
          {
    (yyval.stmt) = new BreakStmt();(yyval.stmt)->SetLineNumber(line);}
#line 1912 "front_end/sysy_parser.tab.cc"
    break;

  case 68: /* Stmt: CONTINUE ';'  */
#line 306 "front_end/sysy_parser.y"
             {
    (yyval.stmt) = new ContinueStmt();(yyval.stmt)->SetLineNumber(line);}
#line 1919 "front_end/sysy_parser.tab.cc"
    break;

  case 69: /* Stmt: RETURN Exp ';'  */
#line 308 "front_end/sysy_parser.y"
               {
    (yyval.stmt) = new RetStmt((yyvsp[-1].expression));(yyval.stmt)->SetLineNumber(line);}
#line 1926 "front_end/sysy_parser.tab.cc"
    break;

  case 70: /* Stmt: RETURN ';'  */
#line 310 "front_end/sysy_parser.y"
           {
    (yyval.stmt) = new RetStmt(NULL);(yyval.stmt)->SetLineNumber(line);}
#line 1933 "front_end/sysy_parser.tab.cc"
    break;

  case 71: /* ConstInitVal_list: ConstInitVal  */
#line 316 "front_end/sysy_parser.y"
             { 
    (yyval.initval_list) = new std::vector<InitValBase>;((yyval.initval_list))->push_back((yyvsp[0].initval));}
#line 1940 "front_end/sysy_parser.tab.cc"
    break;

  case 72: /* ConstInitVal_list: ConstInitVal_list ',' ConstInitVal  */
#line 318 "front_end/sysy_parser.y"
                                   {
    ((yyvsp[-2].initval_list))->push_back((yyvsp[0].initval)); (yyval.initval_list) = (yyvsp[-2].initval_list);}
#line 1947 "front_end/sysy_parser.tab.cc"
    break;

  case 73: /* ConstInitVal: ConstExp  */
#line 322 "front_end/sysy_parser.y"
         { //单值初始化
    (yyval.initval) = new ConstInitVal((yyvsp[0].expression)); (yyval.initval)->SetLineNumber(line);}
#line 1954 "front_end/sysy_parser.tab.cc"
    break;

  case 74: /* ConstInitVal: '{' ConstInitVal_list '}'  */
#line 324 "front_end/sysy_parser.y"
                          { //初始化列表
    (yyval.initval) = new ConstInitValList((yyvsp[-1].initval_list));(yyval.initval)->SetLineNumber(line);}
#line 1961 "front_end/sysy_parser.tab.cc"
    break;

  case 75: /* ConstInitVal: '{' '}'  */
#line 326 "front_end/sysy_parser.y"
        {//初始化为空
    (yyval.initval) = new ConstInitValList(new std::vector<InitValBase>()); (yyval.initval)->SetLineNumber(line);}
#line 1968 "front_end/sysy_parser.tab.cc"
    break;

  case 76: /* VarInitVal_list: VarInitVal  */
#line 330 "front_end/sysy_parser.y"
           {
    (yyval.initval_list) = new std::vector<InitValBase>;((yyval.initval_list))->push_back((yyvsp[0].initval));}
#line 1975 "front_end/sysy_parser.tab.cc"
    break;

  case 77: /* VarInitVal_list: VarInitVal_list ',' VarInitVal  */
#line 332 "front_end/sysy_parser.y"
                               {
    ((yyvsp[-2].initval_list))->push_back((yyvsp[0].initval));(yyval.initval_list) = (yyvsp[-2].initval_list);}
#line 1982 "front_end/sysy_parser.tab.cc"
    break;

  case 78: /* VarInitVal: Exp  */
#line 336 "front_end/sysy_parser.y"
    { 
    (yyval.initval) = new VarInitVal((yyvsp[0].expression)); (yyval.initval)->SetLineNumber(line);}
#line 1989 "front_end/sysy_parser.tab.cc"
    break;

  case 79: /* VarInitVal: '{' VarInitVal_list '}'  */
#line 338 "front_end/sysy_parser.y"
                        { 
    (yyval.initval) = new VarInitValList((yyvsp[-1].initval_list));(yyval.initval)->SetLineNumber(line);}
#line 1996 "front_end/sysy_parser.tab.cc"
    break;

  case 80: /* VarInitVal: '{' '}'  */
#line 340 "front_end/sysy_parser.y"
        {
    (yyval.initval) = new VarInitValList(new std::vector<InitValBase>()); (yyval.initval)->SetLineNumber(line);}
#line 2003 "front_end/sysy_parser.tab.cc"
    break;

  case 81: /* VarDef: IDENT '=' VarInitVal  */
#line 347 "front_end/sysy_parser.y"
                     {  
    (yyval.def) = new VarDef((yyvsp[-2].symbol_token),nullptr,(yyvsp[0].initval));(yyval.def)->SetLineNumber(line);}
#line 2010 "front_end/sysy_parser.tab.cc"
    break;

  case 82: /* VarDef: IDENT  */
#line 349 "front_end/sysy_parser.y"
      {  
    (yyval.def) = new VarDef_no_init((yyvsp[0].symbol_token),nullptr); (yyval.def)->SetLineNumber(line);}
#line 2017 "front_end/sysy_parser.tab.cc"
    break;

  case 83: /* VarDef: IDENT ArrayDim_list  */
#line 352 "front_end/sysy_parser.y"
                     {
    (yyval.def) = new VarDef_no_init((yyvsp[-1].symbol_token), (yyvsp[0].expression_list)); (yyval.def)->SetLineNumber(line);}
#line 2024 "front_end/sysy_parser.tab.cc"
    break;

  case 84: /* VarDef: IDENT ArrayDim_list '=' VarInitVal  */
#line 354 "front_end/sysy_parser.y"
                                    {
    (yyval.def) = new VarDef((yyvsp[-3].symbol_token), (yyvsp[-2].expression_list), (yyvsp[0].initval)); (yyval.def)->SetLineNumber(line);}
#line 2031 "front_end/sysy_parser.tab.cc"
    break;

  case 85: /* ConstDef: IDENT '=' ConstInitVal  */
#line 358 "front_end/sysy_parser.y"
                        { //CONST声明时必须初始化 
    (yyval.def) = new ConstDef((yyvsp[-2].symbol_token),nullptr,(yyvsp[0].initval)); (yyval.def)->SetLineNumber(line);}
#line 2038 "front_end/sysy_parser.tab.cc"
    break;

  case 86: /* ConstDef: IDENT ArrayDim_list '=' ConstInitVal  */
#line 360 "front_end/sysy_parser.y"
                                      {
    (yyval.def) = new ConstDef((yyvsp[-3].symbol_token), (yyvsp[-2].expression_list), (yyvsp[0].initval)); (yyval.def)->SetLineNumber(line); }
#line 2045 "front_end/sysy_parser.tab.cc"
    break;

  case 87: /* ArrayDim: '[' ConstExp ']'  */
#line 365 "front_end/sysy_parser.y"
                   {
    (yyval.expression)=(yyvsp[-1].expression);(yyval.expression)->SetLineNumber(line);}
#line 2052 "front_end/sysy_parser.tab.cc"
    break;

  case 88: /* ArrayDim_list: ArrayDim  */
#line 369 "front_end/sysy_parser.y"
         {
    (yyval.expression_list) = new std::vector<ExprBase>;(yyval.expression_list)->push_back((yyvsp[0].expression)); }
#line 2059 "front_end/sysy_parser.tab.cc"
    break;

  case 89: /* ArrayDim_list: ArrayDim_list ArrayDim  */
#line 371 "front_end/sysy_parser.y"
                       {
    (yyval.expression_list) = (yyvsp[-1].expression_list); (yyval.expression_list)->push_back((yyvsp[0].expression));}
#line 2066 "front_end/sysy_parser.tab.cc"
    break;

  case 90: /* Decl: VarDecl  */
#line 377 "front_end/sysy_parser.y"
        {
    (yyval.decl) = (yyvsp[0].decl); (yyval.decl)->SetLineNumber(line);}
#line 2073 "front_end/sysy_parser.tab.cc"
    break;

  case 91: /* Decl: ConstDecl  */
#line 379 "front_end/sysy_parser.y"
          {
    (yyval.decl) = (yyvsp[0].decl); (yyval.decl)->SetLineNumber(line);}
#line 2080 "front_end/sysy_parser.tab.cc"
    break;

  case 92: /* VarDecl: INT VarDef_list ';'  */
#line 384 "front_end/sysy_parser.y"
                    {
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.decl) = new VarDecl(type,(yyvsp[-1].def_list)); (yyval.decl)->SetLineNumber(line);}
#line 2088 "front_end/sysy_parser.tab.cc"
    break;

  case 93: /* VarDecl: FLOAT VarDef_list ';'  */
#line 387 "front_end/sysy_parser.y"
                       {
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.decl) = new VarDecl(type,(yyvsp[-1].def_list));(yyval.decl)->SetLineNumber(line);}
#line 2096 "front_end/sysy_parser.tab.cc"
    break;

  case 94: /* VarDef_list: VarDef  */
#line 392 "front_end/sysy_parser.y"
         {
    (yyval.def_list) = new std::vector<Def>;((yyval.def_list))->push_back((yyvsp[0].def));}
#line 2103 "front_end/sysy_parser.tab.cc"
    break;

  case 95: /* VarDef_list: VarDef_list ',' VarDef  */
#line 394 "front_end/sysy_parser.y"
                        {
    ((yyvsp[-2].def_list))->push_back((yyvsp[0].def));(yyval.def_list) = (yyvsp[-2].def_list);}
#line 2110 "front_end/sysy_parser.tab.cc"
    break;

  case 96: /* ConstDecl: CONST INT ConstDef_list ';'  */
#line 399 "front_end/sysy_parser.y"
                            {
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    (yyval.decl) = new ConstDecl(type,(yyvsp[-1].def_list)); (yyval.decl)->SetLineNumber(line);}
#line 2118 "front_end/sysy_parser.tab.cc"
    break;

  case 97: /* ConstDecl: CONST FLOAT ConstDef_list ';'  */
#line 402 "front_end/sysy_parser.y"
                              {
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    (yyval.decl) = new ConstDecl(type,(yyvsp[-1].def_list));(yyval.decl)->SetLineNumber(line);}
#line 2126 "front_end/sysy_parser.tab.cc"
    break;

  case 98: /* ConstDef_list: ConstDef  */
#line 407 "front_end/sysy_parser.y"
          {
    (yyval.def_list) = new std::vector<Def>; ((yyval.def_list))->push_back((yyvsp[0].def));}
#line 2133 "front_end/sysy_parser.tab.cc"
    break;

  case 99: /* ConstDef_list: ConstDef_list ',' ConstDef  */
#line 409 "front_end/sysy_parser.y"
                            {
    ((yyvsp[-2].def_list))->push_back((yyvsp[0].def));(yyval.def_list) = (yyvsp[-2].def_list);}
#line 2140 "front_end/sysy_parser.tab.cc"
    break;

  case 100: /* Block: '{' BlockItem_list '}'  */
#line 415 "front_end/sysy_parser.y"
                       {
    (yyval.block) = new __Block((yyvsp[-1].block_item_list));(yyval.block)->SetLineNumber(line);}
#line 2147 "front_end/sysy_parser.tab.cc"
    break;

  case 101: /* Block: '{' '}'  */
#line 417 "front_end/sysy_parser.y"
        {
    (yyval.block) = new __Block(new std::vector<BlockItem>);(yyval.block)->SetLineNumber(line);}
#line 2154 "front_end/sysy_parser.tab.cc"
    break;

  case 102: /* BlockItem_list: BlockItem  */
#line 422 "front_end/sysy_parser.y"
          {
    (yyval.block_item_list) = new std::vector<BlockItem>;((yyval.block_item_list))->push_back((yyvsp[0].block_item));}
#line 2161 "front_end/sysy_parser.tab.cc"
    break;

  case 103: /* BlockItem_list: BlockItem_list BlockItem  */
#line 424 "front_end/sysy_parser.y"
                          {
    ((yyvsp[-1].block_item_list))->push_back((yyvsp[0].block_item));(yyval.block_item_list) = (yyvsp[-1].block_item_list);}
#line 2168 "front_end/sysy_parser.tab.cc"
    break;

  case 104: /* BlockItem: Decl  */
#line 429 "front_end/sysy_parser.y"
     {
    (yyval.block_item) = new BlockItem_Decl((yyvsp[0].decl));(yyval.block_item)->SetLineNumber(line);}
#line 2175 "front_end/sysy_parser.tab.cc"
    break;

  case 105: /* BlockItem: Stmt  */
#line 431 "front_end/sysy_parser.y"
     {
    (yyval.block_item) = new BlockItem_Stmt((yyvsp[0].stmt));(yyval.block_item)->SetLineNumber(line);}
#line 2182 "front_end/sysy_parser.tab.cc"
    break;


#line 2186 "front_end/sysy_parser.tab.cc"

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
      yyerror (YY_("syntax error"));
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

  return yyresult;
}

#line 435 "front_end/sysy_parser.y"

void yyerror(char* s, ...)
{
    ++error_num;
    std::cout << "parser error in line " << line << std::endl;
}
