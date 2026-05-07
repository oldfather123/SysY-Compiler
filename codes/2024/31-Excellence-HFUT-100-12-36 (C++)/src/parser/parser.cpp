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

    #include "SyntaxTree.hpp"
    #include "SyntaxAnalyse.hpp"
    #include <iostream>
    #include <cstring>

    int yylex();
    int yyparse();
    int yyrestart();
    
    extern FILE* yyin;
    extern char* yytext;
    extern int line_number;
    extern int column_end_number;
    extern int column_start_number;

    void yyerror(const char *s) {
        std::cerr << s << std::endl;
        std::cerr << "Error at line " << line_number << ": " << column_end_number << std::endl;
        std::cerr << "Error: " << yytext << std::endl;
        std::abort();
    }

    using namespace ast;

#line 97 "parser.cpp"

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
  YYSYMBOL_VOID = 5,                       /* VOID  */
  YYSYMBOL_IF = 6,                         /* IF  */
  YYSYMBOL_ELSE = 7,                       /* ELSE  */
  YYSYMBOL_WHILE = 8,                      /* WHILE  */
  YYSYMBOL_BREAK = 9,                      /* BREAK  */
  YYSYMBOL_CONTINUE = 10,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 11,                    /* RETURN  */
  YYSYMBOL_Ident = 12,                     /* Ident  */
  YYSYMBOL_CONST = 13,                     /* CONST  */
  YYSYMBOL_ADD = 14,                       /* ADD  */
  YYSYMBOL_SUB = 15,                       /* SUB  */
  YYSYMBOL_MUL = 16,                       /* MUL  */
  YYSYMBOL_DIV = 17,                       /* DIV  */
  YYSYMBOL_MOD = 18,                       /* MOD  */
  YYSYMBOL_LPAREN = 19,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 20,                    /* RPAREN  */
  YYSYMBOL_LBRACKET = 21,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 22,                  /* RBRACKET  */
  YYSYMBOL_LBRACE = 23,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 24,                    /* RBRACE  */
  YYSYMBOL_IntConst = 25,                  /* IntConst  */
  YYSYMBOL_FloatConst = 26,                /* FloatConst  */
  YYSYMBOL_LESS = 27,                      /* LESS  */
  YYSYMBOL_GREATER = 28,                   /* GREATER  */
  YYSYMBOL_EQUAL = 29,                     /* EQUAL  */
  YYSYMBOL_NOT = 30,                       /* NOT  */
  YYSYMBOL_LESS_EQUAL = 31,                /* LESS_EQUAL  */
  YYSYMBOL_GREATER_EQUAL = 32,             /* GREATER_EQUAL  */
  YYSYMBOL_NOT_EQUAL = 33,                 /* NOT_EQUAL  */
  YYSYMBOL_AND = 34,                       /* AND  */
  YYSYMBOL_OR = 35,                        /* OR  */
  YYSYMBOL_ASSIGN = 36,                    /* ASSIGN  */
  YYSYMBOL_COMMA = 37,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 38,                 /* SEMICOLON  */
  YYSYMBOL_ERROR = 39,                     /* ERROR  */
  YYSYMBOL_YYACCEPT = 40,                  /* $accept  */
  YYSYMBOL_CompUnit = 41,                  /* CompUnit  */
  YYSYMBOL_FuncDef = 42,                   /* FuncDef  */
  YYSYMBOL_FuncFParam = 43,                /* FuncFParam  */
  YYSYMBOL_ExpGroup = 44,                  /* ExpGroup  */
  YYSYMBOL_FuncFParamsGroup = 45,          /* FuncFParamsGroup  */
  YYSYMBOL_Block = 46,                     /* Block  */
  YYSYMBOL_BlockItems = 47,                /* BlockItems  */
  YYSYMBOL_Stmt = 48,                      /* Stmt  */
  YYSYMBOL_PrimaryExp = 49,                /* PrimaryExp  */
  YYSYMBOL_Number = 50,                    /* Number  */
  YYSYMBOL_Decl = 51,                      /* Decl  */
  YYSYMBOL_VarDecl = 52,                   /* VarDecl  */
  YYSYMBOL_VarDefGroup = 53,               /* VarDefGroup  */
  YYSYMBOL_VarDef = 54,                    /* VarDef  */
  YYSYMBOL_InitVal = 55,                   /* InitVal  */
  YYSYMBOL_InitValGroup = 56,              /* InitValGroup  */
  YYSYMBOL_ConstDecl = 57,                 /* ConstDecl  */
  YYSYMBOL_ConstDefGroup = 58,             /* ConstDefGroup  */
  YYSYMBOL_ConstDef = 59,                  /* ConstDef  */
  YYSYMBOL_ConstInitVal = 60,              /* ConstInitVal  */
  YYSYMBOL_ConstInitValGroup = 61,         /* ConstInitValGroup  */
  YYSYMBOL_ConstExpGroup = 62,             /* ConstExpGroup  */
  YYSYMBOL_ConstExp = 63,                  /* ConstExp  */
  YYSYMBOL_BType = 64,                     /* BType  */
  YYSYMBOL_AddExp = 65,                    /* AddExp  */
  YYSYMBOL_Exp = 66,                       /* Exp  */
  YYSYMBOL_MulExp = 67,                    /* MulExp  */
  YYSYMBOL_Lval = 68,                      /* Lval  */
  YYSYMBOL_Cond = 69,                      /* Cond  */
  YYSYMBOL_LOrExp = 70,                    /* LOrExp  */
  YYSYMBOL_LAndExp = 71,                   /* LAndExp  */
  YYSYMBOL_EqExp = 72,                     /* EqExp  */
  YYSYMBOL_RelExp = 73,                    /* RelExp  */
  YYSYMBOL_UnaryExp = 74,                  /* UnaryExp  */
  YYSYMBOL_FuncRParamsGroup = 75,          /* FuncRParamsGroup  */
  YYSYMBOL_UnaryOp = 76                    /* UnaryOp  */
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
#define YYFINAL  13
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   244

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  94
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  179

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
       0,   113,   113,   114,   115,   116,   118,   123,   129,   135,
     143,   146,   152,   157,   161,   165,   175,   181,   182,   184,
     189,   195,   198,   205,   212,   218,   225,   229,   234,   240,
     246,   263,   266,   271,   277,   278,   282,   285,   289,   294,
     298,   302,   305,   309,   312,   317,   320,   325,   331,   335,
     339,   345,   349,   353,   358,   361,   366,   372,   376,   380,
     385,   389,   393,   394,   396,   399,   402,   406,   412,   415,
     418,   421,   425,   431,   435,   438,   443,   446,   451,   454,
     457,   461,   464,   467,   470,   473,   480,   483,   486,   491,
     497,   501,   505,   509,   513
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
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "VOID",
  "IF", "ELSE", "WHILE", "BREAK", "CONTINUE", "RETURN", "Ident", "CONST",
  "ADD", "SUB", "MUL", "DIV", "MOD", "LPAREN", "RPAREN", "LBRACKET",
  "RBRACKET", "LBRACE", "RBRACE", "IntConst", "FloatConst", "LESS",
  "GREATER", "EQUAL", "NOT", "LESS_EQUAL", "GREATER_EQUAL", "NOT_EQUAL",
  "AND", "OR", "ASSIGN", "COMMA", "SEMICOLON", "ERROR", "$accept",
  "CompUnit", "FuncDef", "FuncFParam", "ExpGroup", "FuncFParamsGroup",
  "Block", "BlockItems", "Stmt", "PrimaryExp", "Number", "Decl", "VarDecl",
  "VarDefGroup", "VarDef", "InitVal", "InitValGroup", "ConstDecl",
  "ConstDefGroup", "ConstDef", "ConstInitVal", "ConstInitValGroup",
  "ConstExpGroup", "ConstExp", "BType", "AddExp", "Exp", "MulExp", "Lval",
  "Cond", "LOrExp", "LAndExp", "EqExp", "RelExp", "UnaryExp",
  "FuncRParamsGroup", "UnaryOp", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-126)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      82,  -126,  -126,    17,    66,    76,  -126,  -126,  -126,  -126,
      20,    -8,    44,  -126,  -126,  -126,    32,    27,    51,    70,
      57,    55,   214,   171,    62,    90,    67,    86,    74,   102,
      89,    44,   106,    86,    74,    80,  -126,  -126,   214,  -126,
    -126,  -126,  -126,  -126,   128,    63,   141,  -126,  -126,   214,
      98,  -126,    63,  -126,   171,     7,    27,  -126,  -126,  -126,
      66,   133,   134,   188,    57,  -126,  -126,   144,   205,   214,
    -126,   145,    70,   214,   214,   214,   214,   214,  -126,  -126,
     117,  -126,  -126,    12,    74,    86,   139,   154,  -126,  -126,
    -126,    86,  -126,   135,   148,  -126,  -126,   141,   141,  -126,
    -126,  -126,   171,   147,   155,   157,   143,   149,    78,  -126,
    -126,  -126,  -126,  -126,    90,   150,   146,  -126,  -126,   168,
    -126,   156,  -126,   214,   172,   168,   117,  -126,   214,   214,
    -126,  -126,  -126,   153,  -126,   214,  -126,   188,   174,   135,
    -126,  -126,  -126,    63,   175,   164,   170,    11,    99,   185,
    -126,   177,   156,  -126,  -126,   137,   214,   214,   214,   214,
     214,   214,   214,   214,   137,  -126,  -126,   199,   170,    11,
      99,    99,    63,    63,    63,    63,  -126,   137,  -126
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    62,    63,     0,     0,     0,     4,     5,    36,    37,
       0,     0,     0,     1,     2,     3,    41,    40,     0,    60,
      52,     0,     0,     0,    43,     0,     0,     0,    15,     0,
       0,     0,     0,     0,    15,    13,    92,    93,     0,    34,
      35,    94,    86,    31,     0,    61,    64,    32,    68,     0,
       0,    42,    67,    45,     0,    41,    40,    38,    18,     7,
       0,     0,    10,     0,    52,    50,     6,     0,     0,     0,
      72,     0,    60,     0,     0,     0,     0,     0,    87,    47,
      49,    44,    39,     0,    15,     0,     0,     0,    53,    54,
      51,     0,    89,    91,     0,    33,    59,    65,    66,    69,
      70,    71,     0,     0,     0,     0,     0,     0,     0,    16,
      27,    21,    17,    19,     0,     0,    32,    14,     9,    13,
      56,    58,     8,     0,     0,    13,    49,    46,     0,     0,
      29,    30,    22,     0,    26,     0,    11,     0,     0,    91,
      88,    12,    48,    81,     0,    73,    74,    76,    78,     0,
      20,     0,    58,    55,    90,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,    57,    24,    75,    77,
      79,    80,    82,    83,    84,    85,    28,     0,    25
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -126,  -126,   203,   -11,   -62,   -17,   -19,  -126,  -125,  -126,
    -126,    -1,  -126,   160,   184,   -41,    84,  -126,   158,   181,
     -75,    69,   -12,   201,     1,   -22,   -35,    45,   -81,   103,
    -126,    71,    77,   -26,   -30,    97,  -126
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,    28,    70,    61,   111,    83,   112,    42,
      43,     7,     8,    26,    17,    51,   103,     9,    32,    20,
      88,   138,    24,    89,    29,    52,    53,    46,    47,   144,
     145,   146,   147,   148,    48,   124,    49
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      45,    10,   116,    71,    15,    12,    10,    30,    59,    80,
      34,    18,   121,    81,    66,     1,     2,    67,   104,    78,
     105,   106,   107,   108,    35,     4,    36,    37,    22,    11,
     167,    38,    16,    93,    94,    58,   109,    39,    40,   176,
     158,    45,    41,    23,   159,    99,   100,   101,   115,    84,
     110,    21,   178,    22,     1,     2,    19,   136,     1,     2,
      96,   126,   152,   141,    25,    45,   118,   117,    23,     1,
       2,    27,   122,   133,   116,    33,    13,    73,    74,     1,
       2,     3,   113,   116,   114,     1,     2,     3,   139,     4,
      35,    22,    36,    37,    31,     4,   116,    38,    54,    68,
     151,    69,    55,    39,    40,    57,   143,   143,    41,    58,
      35,    60,    36,    37,    62,    45,   132,    38,    97,    98,
     115,    50,    79,    39,    40,    63,   160,   161,    41,   115,
     162,   163,   170,   171,   143,   143,   143,   143,   172,   173,
     174,   175,   115,   104,    65,   105,   106,   107,   108,    35,
      72,    36,    37,    85,   102,    86,    38,    75,    76,    77,
      58,   119,    39,    40,    91,    95,    35,    41,    36,    37,
     125,   127,   123,    38,   128,   110,   129,    87,   120,    39,
      40,   130,   135,    35,    41,    36,    37,   131,   134,    69,
      38,   150,   140,   137,    50,   155,    39,    40,   153,   156,
      35,    41,    36,    37,   157,   164,   177,    38,    14,    56,
     142,    87,    64,    39,    40,   165,    82,    35,    41,    36,
      37,   166,    90,    44,    38,    92,    35,   168,    36,    37,
      39,    40,   149,    38,   169,    41,   154,     0,     0,    39,
      40,     0,     0,     0,    41
};

static const yytype_int16 yycheck[] =
{
      22,     0,    83,    38,     5,     4,     5,    19,    27,    50,
      21,    19,    87,    54,    33,     3,     4,    34,     6,    49,
       8,     9,    10,    11,    12,    13,    14,    15,    21,    12,
     155,    19,    12,    68,    69,    23,    24,    25,    26,   164,
      29,    63,    30,    36,    33,    75,    76,    77,    83,    60,
      38,    19,   177,    21,     3,     4,    12,   119,     3,     4,
      72,   102,   137,   125,    37,    87,    85,    84,    36,     3,
       4,    20,    91,   108,   155,    20,     0,    14,    15,     3,
       4,     5,    83,   164,    83,     3,     4,     5,   123,    13,
      12,    21,    14,    15,    37,    13,   177,    19,    36,    19,
     135,    21,    12,    25,    26,    38,   128,   129,    30,    23,
      12,    37,    14,    15,    12,   137,    38,    19,    73,    74,
     155,    23,    24,    25,    26,    36,    27,    28,    30,   164,
      31,    32,   158,   159,   156,   157,   158,   159,   160,   161,
     162,   163,   177,     6,    38,     8,     9,    10,    11,    12,
      22,    14,    15,    20,    37,    21,    19,    16,    17,    18,
      23,    22,    25,    26,    20,    20,    12,    30,    14,    15,
      22,    24,    37,    19,    19,    38,    19,    23,    24,    25,
      26,    38,    36,    12,    30,    14,    15,    38,    38,    21,
      19,    38,    20,    37,    23,    20,    25,    26,    24,    35,
      12,    30,    14,    15,    34,    20,     7,    19,     5,    25,
     126,    23,    31,    25,    26,    38,    56,    12,    30,    14,
      15,   152,    64,    22,    19,    20,    12,   156,    14,    15,
      25,    26,   129,    19,   157,    30,   139,    -1,    -1,    25,
      26,    -1,    -1,    -1,    30
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,    13,    41,    42,    51,    52,    57,
      64,    12,    64,     0,    42,    51,    12,    54,    19,    12,
      59,    19,    21,    36,    62,    37,    53,    20,    43,    64,
      62,    37,    58,    20,    43,    12,    14,    15,    19,    25,
      26,    30,    49,    50,    63,    65,    67,    68,    74,    76,
      23,    55,    65,    66,    36,    12,    54,    38,    23,    46,
      37,    45,    12,    36,    59,    38,    46,    45,    19,    21,
      44,    66,    22,    14,    15,    16,    17,    18,    74,    24,
      55,    55,    53,    47,    43,    20,    21,    23,    60,    63,
      58,    20,    20,    66,    66,    20,    62,    67,    67,    74,
      74,    74,    37,    56,     6,     8,     9,    10,    11,    24,
      38,    46,    48,    51,    64,    66,    68,    45,    46,    22,
      24,    60,    46,    37,    75,    22,    55,    24,    19,    19,
      38,    38,    38,    66,    38,    36,    44,    37,    61,    66,
      20,    44,    56,    65,    69,    70,    71,    72,    73,    69,
      38,    66,    60,    24,    75,    20,    35,    34,    29,    33,
      27,    28,    31,    32,    20,    38,    61,    48,    71,    72,
      73,    73,    65,    65,    65,    65,    48,     7,    48
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    40,    41,    41,    41,    41,    42,    42,    42,    42,
      43,    43,    44,    44,    45,    45,    46,    47,    47,    47,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    49,    50,    50,    51,    51,    52,    53,
      53,    54,    54,    54,    54,    55,    55,    55,    56,    56,
      57,    58,    58,    59,    60,    60,    60,    61,    61,    62,
      62,    63,    64,    64,    65,    65,    65,    66,    67,    67,
      67,    67,    68,    69,    70,    70,    71,    71,    72,    72,
      72,    73,    73,    73,    73,    73,    74,    74,    74,    74,
      75,    75,    76,    76,    76
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     2,     1,     1,     5,     5,     7,     7,
       2,     5,     4,     0,     3,     0,     3,     2,     0,     2,
       3,     1,     2,     4,     5,     7,     2,     1,     5,     2,
       2,     1,     1,     3,     1,     1,     1,     1,     4,     3,
       0,     1,     3,     2,     4,     1,     4,     2,     3,     0,
       5,     3,     0,     4,     1,     4,     2,     3,     0,     4,
       0,     1,     1,     1,     1,     3,     3,     1,     1,     3,
       3,     3,     2,     1,     1,     3,     1,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     2,     5,     3,
       3,     0,     1,     1,     1
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
  case 2: /* CompUnit: CompUnit FuncDef  */
#line 113 "parser.y"
                      { SyntaxAnalyseCompUnit((yyval.compunit),(yyvsp[-1].compunit),(yyvsp[0].func_def));}
#line 1293 "parser.cpp"
    break;

  case 3: /* CompUnit: CompUnit Decl  */
#line 114 "parser.y"
                   {SyntaxAnalyseCompUnit((yyval.compunit), (yyvsp[-1].compunit), (yyvsp[0].stmt));}
#line 1299 "parser.cpp"
    break;

  case 4: /* CompUnit: FuncDef  */
#line 115 "parser.y"
             { SyntaxAnalyseCompUnit((yyval.compunit),nullptr,(yyvsp[0].func_def)); }
#line 1305 "parser.cpp"
    break;

  case 5: /* CompUnit: Decl  */
#line 116 "parser.y"
          {SyntaxAnalyseCompUnit((yyval.compunit), nullptr, (yyvsp[0].stmt));}
#line 1311 "parser.cpp"
    break;

  case 6: /* FuncDef: BType Ident LPAREN RPAREN Block  */
#line 118 "parser.y"
                                             {
        SyntaxAnalyseFuncDef((yyval.func_def),(yyvsp[-4].var_type),(yyvsp[-3].current_symbol),(yyvsp[0].block));
        free((yyvsp[-2].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1321 "parser.cpp"
    break;

  case 7: /* FuncDef: VOID Ident LPAREN RPAREN Block  */
#line 123 "parser.y"
                                     {
        SyntaxAnalyseFuncDef((yyval.func_def), vartype::VOID, (yyvsp[-3].current_symbol), (yyvsp[0].block));
        free((yyvsp[-4].current_symbol));
        free((yyvsp[-2].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1332 "parser.cpp"
    break;

  case 8: /* FuncDef: BType Ident LPAREN FuncFParam FuncFParamsGroup RPAREN Block  */
#line 129 "parser.y"
                                                                  {
        SyntaxAnalyseFuncDef((yyval.func_def), (yyvsp[-6].var_type), (yyvsp[-5].current_symbol), (yyvsp[0].block));
        SyntaxAnalyseFuncFDecl((yyval.func_def), (yyvsp[-3].func_f_param), (yyvsp[-2].func_def));
        free((yyvsp[-4].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1343 "parser.cpp"
    break;

  case 9: /* FuncDef: VOID Ident LPAREN FuncFParam FuncFParamsGroup RPAREN Block  */
#line 135 "parser.y"
                                                                 {
        SyntaxAnalyseFuncDef((yyval.func_def), vartype::VOID, (yyvsp[-5].current_symbol), (yyvsp[0].block));
        SyntaxAnalyseFuncFDecl((yyval.func_def), (yyvsp[-3].func_f_param), (yyvsp[-2].func_def));
        free((yyvsp[-6].current_symbol));
        free((yyvsp[-4].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1355 "parser.cpp"
    break;

  case 10: /* FuncFParam: BType Ident  */
#line 143 "parser.y"
                            {
        SyntaxAnalyseFuncFDef((yyval.func_f_param), (yyvsp[-1].var_type), (yyvsp[0].current_symbol), nullptr, false);
    }
#line 1363 "parser.cpp"
    break;

  case 11: /* FuncFParam: BType Ident LBRACKET RBRACKET ExpGroup  */
#line 146 "parser.y"
                                             {
        SyntaxAnalyseFuncFDef((yyval.func_f_param), (yyvsp[-4].var_type), (yyvsp[-3].current_symbol), (yyvsp[0].var_dimension), true);
        free((yyvsp[-2].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1373 "parser.cpp"
    break;

  case 12: /* ExpGroup: LBRACKET Exp RBRACKET ExpGroup  */
#line 152 "parser.y"
                                             {
        SyntaxAnalyseVarDimension((yyval.var_dimension), (yyvsp[-2].expr), (yyvsp[0].var_dimension));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1383 "parser.cpp"
    break;

  case 13: /* ExpGroup: %empty  */
#line 157 "parser.y"
             {
        (yyval.var_dimension) = nullptr;
    }
#line 1391 "parser.cpp"
    break;

  case 14: /* FuncFParamsGroup: COMMA FuncFParam FuncFParamsGroup  */
#line 161 "parser.y"
                                                        {
        SyntaxAnalyseFuncFDeclGroup((yyval.func_def), (yyvsp[-1].func_f_param), (yyvsp[0].func_def));
        free((yyvsp[-2].current_symbol));
    }
#line 1400 "parser.cpp"
    break;

  case 15: /* FuncFParamsGroup: %empty  */
#line 165 "parser.y"
             {
        (yyval.func_def) = nullptr;
    }
#line 1408 "parser.cpp"
    break;

  case 16: /* Block: LBRACE BlockItems RBRACE  */
#line 175 "parser.y"
                                    {
        SynataxAnalyseBlock((yyval.block),(yyvsp[-1].block));
        free((yyvsp[-2].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1418 "parser.cpp"
    break;

  case 17: /* BlockItems: BlockItems Stmt  */
#line 181 "parser.y"
                                { SynataxAnalyseBlockItems((yyval.block),(yyvsp[-1].block),(yyvsp[0].stmt));}
#line 1424 "parser.cpp"
    break;

  case 18: /* BlockItems: %empty  */
#line 182 "parser.y"
             { SynataxAnalyseBlockItems((yyval.block),nullptr,nullptr);}
#line 1430 "parser.cpp"
    break;

  case 19: /* BlockItems: BlockItems Decl  */
#line 184 "parser.y"
                     {
        SynataxAnalyseBlockItems((yyval.block),(yyvsp[-1].block),(yyvsp[0].stmt));
    }
#line 1438 "parser.cpp"
    break;

  case 20: /* Stmt: RETURN Exp SEMICOLON  */
#line 189 "parser.y"
                               {
        SynataxAnalyseStmtReturn((yyval.stmt),(yyvsp[-1].expr));
        free((yyvsp[-2].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1448 "parser.cpp"
    break;

  case 21: /* Stmt: Block  */
#line 195 "parser.y"
           {
        SynataxAnalyseStmtBlock((yyval.stmt),(yyvsp[0].block));
    }
#line 1456 "parser.cpp"
    break;

  case 22: /* Stmt: RETURN SEMICOLON  */
#line 198 "parser.y"
                     {
        SynataxAnalyseStmtReturn((yyval.stmt),nullptr);
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1466 "parser.cpp"
    break;

  case 23: /* Stmt: Lval ASSIGN Exp SEMICOLON  */
#line 205 "parser.y"
                               {
        SynataxAnalyseStmtAssign((yyval.stmt),(yyvsp[-3].lval),(yyvsp[-1].expr));
        free((yyvsp[-2].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1476 "parser.cpp"
    break;

  case 24: /* Stmt: IF LPAREN Cond RPAREN Stmt  */
#line 212 "parser.y"
                                 {
        SynataxAnalyseStmtIf((yyval.stmt),(yyvsp[-2].expr),(yyvsp[0].stmt),nullptr);
        free((yyvsp[-4].current_symbol));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1487 "parser.cpp"
    break;

  case 25: /* Stmt: IF LPAREN Cond RPAREN Stmt ELSE Stmt  */
#line 218 "parser.y"
                                          {
        SynataxAnalyseStmtIf((yyval.stmt),(yyvsp[-4].expr),(yyvsp[-2].stmt),(yyvsp[0].stmt));
        free((yyvsp[-6].current_symbol));
        free((yyvsp[-5].current_symbol));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1499 "parser.cpp"
    break;

  case 26: /* Stmt: Exp SEMICOLON  */
#line 225 "parser.y"
                    {
        SynataxAnalyseStmtExp((yyval.stmt), (yyvsp[-1].expr));
        free((yyvsp[0].current_symbol));
    }
#line 1508 "parser.cpp"
    break;

  case 27: /* Stmt: SEMICOLON  */
#line 229 "parser.y"
               {
        // SynataxAnalyseStmtEmpty($$);
        (yyval.stmt) = new ast::empty_stmt_syntax;
        free((yyvsp[0].current_symbol));
    }
#line 1518 "parser.cpp"
    break;

  case 28: /* Stmt: WHILE LPAREN Cond RPAREN Stmt  */
#line 234 "parser.y"
                                    {
        SynataxAnalyseStmtWhile((yyval.stmt), (yyvsp[-2].expr), (yyvsp[0].stmt));
        free((yyvsp[-4].current_symbol));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1529 "parser.cpp"
    break;

  case 29: /* Stmt: BREAK SEMICOLON  */
#line 240 "parser.y"
                      {
        // SynataxAnalyseStmtBoC(&&, $1);
        (yyval.stmt) = new ast::break_stmt_syntax;
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1540 "parser.cpp"
    break;

  case 30: /* Stmt: CONTINUE SEMICOLON  */
#line 246 "parser.y"
                         {
        // SynataxAnalyseStmtBoC(&&, $1);
        (yyval.stmt) = new ast::continue_stmt_syntax;
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1551 "parser.cpp"
    break;

  case 31: /* PrimaryExp: Number  */
#line 263 "parser.y"
                       {
        (yyval.expr) = (yyvsp[0].expr);
    }
#line 1559 "parser.cpp"
    break;

  case 32: /* PrimaryExp: Lval  */
#line 266 "parser.y"
           {
        (yyval.expr) = (yyvsp[0].lval);
    }
#line 1567 "parser.cpp"
    break;

  case 33: /* PrimaryExp: LPAREN Exp RPAREN  */
#line 271 "parser.y"
                       {
        (yyval.expr)=(yyvsp[-1].expr);
        free((yyvsp[-2].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1577 "parser.cpp"
    break;

  case 34: /* Number: IntConst  */
#line 277 "parser.y"
                     {SynataxAnalysePrimaryExpIntConst((yyval.expr),(yyvsp[0].current_symbol));}
#line 1583 "parser.cpp"
    break;

  case 35: /* Number: FloatConst  */
#line 278 "parser.y"
                 {SynataxAnalysePrimaryExpFloatConst((yyval.expr),(yyvsp[0].current_symbol));}
#line 1589 "parser.cpp"
    break;

  case 36: /* Decl: VarDecl  */
#line 282 "parser.y"
                 {
        (yyval.stmt)=(yyvsp[0].stmt);
    }
#line 1597 "parser.cpp"
    break;

  case 37: /* Decl: ConstDecl  */
#line 285 "parser.y"
                {
        (yyval.stmt) = (yyvsp[0].stmt);
    }
#line 1605 "parser.cpp"
    break;

  case 38: /* VarDecl: BType VarDef VarDefGroup SEMICOLON  */
#line 289 "parser.y"
                                               {
        SynataxAnalyseVarDecl((yyval.stmt), (yyvsp[-3].var_type), (yyvsp[-2].var_def_stmt), (yyvsp[-1].var_decl_stmt), false);
        free((yyvsp[0].current_symbol));
    }
#line 1614 "parser.cpp"
    break;

  case 39: /* VarDefGroup: COMMA VarDef VarDefGroup  */
#line 294 "parser.y"
                                          {
        SynataxAnalyseVarDefGroup((yyval.var_decl_stmt),(yyvsp[-1].var_def_stmt),(yyvsp[0].var_decl_stmt));
        free((yyvsp[-2].current_symbol));
    }
#line 1623 "parser.cpp"
    break;

  case 40: /* VarDefGroup: %empty  */
#line 298 "parser.y"
             {
        (yyval.var_decl_stmt)=nullptr;
    }
#line 1631 "parser.cpp"
    break;

  case 41: /* VarDef: Ident  */
#line 302 "parser.y"
                   {
        SynataxAnalyseVarDef((yyval.var_def_stmt),(yyvsp[0].current_symbol), nullptr,nullptr);
    }
#line 1639 "parser.cpp"
    break;

  case 42: /* VarDef: Ident ASSIGN InitVal  */
#line 305 "parser.y"
                          {
        SynataxAnalyseVarDef((yyval.var_def_stmt),(yyvsp[-2].current_symbol), nullptr,(yyvsp[0].init));
        free((yyvsp[-1].current_symbol));
    }
#line 1648 "parser.cpp"
    break;

  case 43: /* VarDef: Ident ConstExpGroup  */
#line 309 "parser.y"
                          {
        SynataxAnalyseVarDef((yyval.var_def_stmt),(yyvsp[-1].current_symbol), (yyvsp[0].var_dimension), nullptr);
    }
#line 1656 "parser.cpp"
    break;

  case 44: /* VarDef: Ident ConstExpGroup ASSIGN InitVal  */
#line 312 "parser.y"
                                         {
        SynataxAnalyseVarDef((yyval.var_def_stmt),(yyvsp[-3].current_symbol), (yyvsp[-2].var_dimension), (yyvsp[0].init));
        free((yyvsp[-1].current_symbol));
    }
#line 1665 "parser.cpp"
    break;

  case 45: /* InitVal: Exp  */
#line 317 "parser.y"
                {
        SynataxAnalyseInitVal((yyval.init), (yyvsp[0].expr), nullptr, nullptr);
    }
#line 1673 "parser.cpp"
    break;

  case 46: /* InitVal: LBRACE InitVal InitValGroup RBRACE  */
#line 320 "parser.y"
                                         {
        SynataxAnalyseInitVal((yyval.init), nullptr, (yyvsp[-2].init), (yyvsp[-1].init));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1683 "parser.cpp"
    break;

  case 47: /* InitVal: LBRACE RBRACE  */
#line 325 "parser.y"
                    {
        SynataxAnalyseInitVal((yyval.init), nullptr, nullptr, nullptr);
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1693 "parser.cpp"
    break;

  case 48: /* InitValGroup: COMMA InitVal InitValGroup  */
#line 331 "parser.y"
                                             {
        SynataxAnalyseInitValGroup((yyval.init), (yyvsp[-1].init), (yyvsp[0].init));
        free((yyvsp[-2].current_symbol));
    }
#line 1702 "parser.cpp"
    break;

  case 49: /* InitValGroup: %empty  */
#line 335 "parser.y"
             {
        (yyval.init) = nullptr;
    }
#line 1710 "parser.cpp"
    break;

  case 50: /* ConstDecl: CONST BType ConstDef ConstDefGroup SEMICOLON  */
#line 339 "parser.y"
                                                            {
        SynataxAnalyseVarDecl((yyval.stmt), (yyvsp[-3].var_type), (yyvsp[-2].var_def_stmt), (yyvsp[-1].var_decl_stmt), true);
        free((yyvsp[-4].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1720 "parser.cpp"
    break;

  case 51: /* ConstDefGroup: COMMA ConstDef ConstDefGroup  */
#line 345 "parser.y"
                                                {
        SynataxAnalyseVarDefGroup((yyval.var_decl_stmt), (yyvsp[-1].var_def_stmt), (yyvsp[0].var_decl_stmt));
        free((yyvsp[-2].current_symbol));
    }
#line 1729 "parser.cpp"
    break;

  case 52: /* ConstDefGroup: %empty  */
#line 349 "parser.y"
             {
        (yyval.var_decl_stmt) = nullptr;
    }
#line 1737 "parser.cpp"
    break;

  case 53: /* ConstDef: Ident ConstExpGroup ASSIGN ConstInitVal  */
#line 353 "parser.y"
                                                      {
        SynataxAnalyseVarDef((yyval.var_def_stmt), (yyvsp[-3].current_symbol), (yyvsp[-2].var_dimension), (yyvsp[0].init));
        free((yyvsp[-1].current_symbol));
    }
#line 1746 "parser.cpp"
    break;

  case 54: /* ConstInitVal: ConstExp  */
#line 358 "parser.y"
                           {
        SynataxAnalyseInitVal((yyval.init), (yyvsp[0].expr), nullptr, nullptr);
    }
#line 1754 "parser.cpp"
    break;

  case 55: /* ConstInitVal: LBRACE ConstInitVal ConstInitValGroup RBRACE  */
#line 361 "parser.y"
                                                   {
        SynataxAnalyseInitVal((yyval.init), nullptr, (yyvsp[-2].init), (yyvsp[-1].init));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1764 "parser.cpp"
    break;

  case 56: /* ConstInitVal: LBRACE RBRACE  */
#line 366 "parser.y"
                    {
        SynataxAnalyseInitVal((yyval.init), nullptr, nullptr, nullptr);
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 1774 "parser.cpp"
    break;

  case 57: /* ConstInitValGroup: COMMA ConstInitVal ConstInitValGroup  */
#line 372 "parser.y"
                                                            {
        SynataxAnalyseInitValGroup((yyval.init), (yyvsp[-1].init), (yyvsp[0].init));
        free((yyvsp[-2].current_symbol));
    }
#line 1783 "parser.cpp"
    break;

  case 58: /* ConstInitValGroup: %empty  */
#line 376 "parser.y"
             {
        (yyval.init) = nullptr;
    }
#line 1791 "parser.cpp"
    break;

  case 59: /* ConstExpGroup: LBRACKET ConstExp RBRACKET ConstExpGroup  */
#line 380 "parser.y"
                                                            {
        SyntaxAnalyseVarDimension((yyval.var_dimension), (yyvsp[-2].expr), (yyvsp[0].var_dimension));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[-1].current_symbol));
    }
#line 1801 "parser.cpp"
    break;

  case 60: /* ConstExpGroup: %empty  */
#line 385 "parser.y"
             {
        (yyval.var_dimension) = nullptr;
    }
#line 1809 "parser.cpp"
    break;

  case 61: /* ConstExp: AddExp  */
#line 389 "parser.y"
                     {
        (yyval.expr) = (yyvsp[0].expr);
    }
#line 1817 "parser.cpp"
    break;

  case 62: /* BType: INT  */
#line 393 "parser.y"
               {SynataxAnalyseVarType((yyval.var_type), (yyvsp[0].current_symbol));}
#line 1823 "parser.cpp"
    break;

  case 63: /* BType: FLOAT  */
#line 394 "parser.y"
            {SynataxAnalyseVarType((yyval.var_type), (yyvsp[0].current_symbol));}
#line 1829 "parser.cpp"
    break;

  case 64: /* AddExp: MulExp  */
#line 396 "parser.y"
                  {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1837 "parser.cpp"
    break;

  case 65: /* AddExp: AddExp ADD MulExp  */
#line 399 "parser.y"
                       {
        SynataxAnalyseAddExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1845 "parser.cpp"
    break;

  case 66: /* AddExp: AddExp SUB MulExp  */
#line 402 "parser.y"
                       {
        SynataxAnalyseAddExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1853 "parser.cpp"
    break;

  case 67: /* Exp: AddExp  */
#line 406 "parser.y"
               {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1861 "parser.cpp"
    break;

  case 68: /* MulExp: UnaryExp  */
#line 412 "parser.y"
                    {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1869 "parser.cpp"
    break;

  case 69: /* MulExp: MulExp MUL UnaryExp  */
#line 415 "parser.y"
                          {
         SynataxAnalyseMulExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1877 "parser.cpp"
    break;

  case 70: /* MulExp: MulExp DIV UnaryExp  */
#line 418 "parser.y"
                          {
         SynataxAnalyseMulExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1885 "parser.cpp"
    break;

  case 71: /* MulExp: MulExp MOD UnaryExp  */
#line 421 "parser.y"
                          {
        SynataxAnalyseMulExp((yyval.expr), (yyvsp[-2].expr), (yyvsp[-1].current_symbol), (yyvsp[0].expr));
    }
#line 1893 "parser.cpp"
    break;

  case 72: /* Lval: Ident ExpGroup  */
#line 425 "parser.y"
                        {
        SynataxAnalyseLval((yyval.lval),(yyvsp[-1].current_symbol), (yyvsp[0].var_dimension));
    }
#line 1901 "parser.cpp"
    break;

  case 73: /* Cond: LOrExp  */
#line 431 "parser.y"
               {
    (yyval.expr)=(yyvsp[0].expr);
   }
#line 1909 "parser.cpp"
    break;

  case 74: /* LOrExp: LAndExp  */
#line 435 "parser.y"
                  {
    (yyval.expr)=(yyvsp[0].expr);
   }
#line 1917 "parser.cpp"
    break;

  case 75: /* LOrExp: LOrExp OR LAndExp  */
#line 438 "parser.y"
                     {
    SynataxAnalyseLOrExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[0].expr));
    free((yyvsp[-1].current_symbol));
   }
#line 1926 "parser.cpp"
    break;

  case 76: /* LAndExp: EqExp  */
#line 443 "parser.y"
                  {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1934 "parser.cpp"
    break;

  case 77: /* LAndExp: LAndExp AND EqExp  */
#line 446 "parser.y"
                        {
        SynataxAnalyseLAndExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[0].expr));
        free((yyvsp[-1].current_symbol));
    }
#line 1943 "parser.cpp"
    break;

  case 78: /* EqExp: RelExp  */
#line 451 "parser.y"
                 {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1951 "parser.cpp"
    break;

  case 79: /* EqExp: EqExp EQUAL RelExp  */
#line 454 "parser.y"
                       {
        SynataxAnalyseEqExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1959 "parser.cpp"
    break;

  case 80: /* EqExp: EqExp NOT_EQUAL RelExp  */
#line 457 "parser.y"
                           {
        SynataxAnalyseEqExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1967 "parser.cpp"
    break;

  case 81: /* RelExp: AddExp  */
#line 461 "parser.y"
                  {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 1975 "parser.cpp"
    break;

  case 82: /* RelExp: RelExp LESS AddExp  */
#line 464 "parser.y"
                         {
        SynataxAnalyseRelExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1983 "parser.cpp"
    break;

  case 83: /* RelExp: RelExp GREATER AddExp  */
#line 467 "parser.y"
                            {
        SynataxAnalyseRelExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1991 "parser.cpp"
    break;

  case 84: /* RelExp: RelExp LESS_EQUAL AddExp  */
#line 470 "parser.y"
                               {
        SynataxAnalyseRelExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 1999 "parser.cpp"
    break;

  case 85: /* RelExp: RelExp GREATER_EQUAL AddExp  */
#line 473 "parser.y"
                                  {
        SynataxAnalyseRelExp((yyval.expr),(yyvsp[-2].expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 2007 "parser.cpp"
    break;

  case 86: /* UnaryExp: PrimaryExp  */
#line 480 "parser.y"
                        {
        (yyval.expr)=(yyvsp[0].expr);
    }
#line 2015 "parser.cpp"
    break;

  case 87: /* UnaryExp: UnaryOp UnaryExp  */
#line 483 "parser.y"
                      {
        SynataxAnalyseUnaryExp((yyval.expr),(yyvsp[-1].current_symbol),(yyvsp[0].expr));
    }
#line 2023 "parser.cpp"
    break;

  case 88: /* UnaryExp: Ident LPAREN Exp FuncRParamsGroup RPAREN  */
#line 486 "parser.y"
                                               {
        SyntaxAnalyseFuncCall((yyval.expr), (yyvsp[-4].current_symbol), (yyvsp[-2].expr), (yyvsp[-1].func_call));
        free((yyvsp[-3].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 2033 "parser.cpp"
    break;

  case 89: /* UnaryExp: Ident LPAREN RPAREN  */
#line 491 "parser.y"
                          {
        SyntaxAnalyseFuncCall((yyval.expr), (yyvsp[-2].current_symbol), nullptr, nullptr);
        free((yyvsp[-1].current_symbol));
        free((yyvsp[0].current_symbol));
    }
#line 2043 "parser.cpp"
    break;

  case 90: /* FuncRParamsGroup: COMMA Exp FuncRParamsGroup  */
#line 497 "parser.y"
                                                 {
        SyntaxAnalyseFuncCallGroup((yyval.func_call), (yyvsp[-1].expr), (yyvsp[0].func_call));
        free((yyvsp[-2].current_symbol));
    }
#line 2052 "parser.cpp"
    break;

  case 91: /* FuncRParamsGroup: %empty  */
#line 501 "parser.y"
             {
        (yyval.func_call) = nullptr;
    }
#line 2060 "parser.cpp"
    break;

  case 92: /* UnaryOp: ADD  */
#line 505 "parser.y"
               {
        (yyval.current_symbol)=(yyvsp[0].current_symbol);
        // strcpy($$, $1);
    }
#line 2069 "parser.cpp"
    break;

  case 93: /* UnaryOp: SUB  */
#line 509 "parser.y"
         {
        (yyval.current_symbol)=(yyvsp[0].current_symbol);
        // strcpy($$, $1);
    }
#line 2078 "parser.cpp"
    break;

  case 94: /* UnaryOp: NOT  */
#line 513 "parser.y"
         {
        (yyval.current_symbol)=(yyvsp[0].current_symbol);
        // strcpy($$, $1);
    }
#line 2087 "parser.cpp"
    break;


#line 2091 "parser.cpp"

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

#line 519 "parser.y"


