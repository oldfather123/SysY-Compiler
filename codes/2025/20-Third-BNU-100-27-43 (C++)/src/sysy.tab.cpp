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
#line 1 "src/bisonFile/sysy.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symbol.h"
ASTNode* root_node;

/* 声明yylex函数避免隐式声明警告 */
int yylex(void);
void yyerror(const char *s);

/* 声明需要的函数 */
ASTNode *merge_decl(ASTNode *root, ASTNode* decl);
ASTNode *merge_func_def(ASTNode *root, ASTNode* func);
ASTNode* append_func_param(ASTNode* params, ASTNode* param);
ASTNode* new_func_call(char* name, ASTNode* args, int line_no);
ASTNode* append_arg(ASTNode* list, ASTNode* arg);
ASTNode* append_block_item(ASTNode* items, ASTNode* item);
ASTNode* validate_const_exp(ASTNode* expr);
ASTNode* append_init_list(ASTNode* list, ASTNode* item);
void check_main(char* name);

/* 添加一个类型定义，用于BType和FuncType */
typedef int NodeDataType;

#line 98 "src/sysy.tab.c"

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

#include "sysy.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_LOWER_THAN_ELSE = 3,            /* LOWER_THAN_ELSE  */
  YYSYMBOL_4_ = 4,                         /* '<'  */
  YYSYMBOL_5_ = 5,                         /* '>'  */
  YYSYMBOL_6_ = 6,                         /* '!'  */
  YYSYMBOL_7_ = 7,                         /* '~'  */
  YYSYMBOL_NEG = 8,                        /* NEG  */
  YYSYMBOL_POS = 9,                        /* POS  */
  YYSYMBOL_FUNC_IDENT = 10,                /* FUNC_IDENT  */
  YYSYMBOL_INT = 11,                       /* INT  */
  YYSYMBOL_FLOAT = 12,                     /* FLOAT  */
  YYSYMBOL_CONST = 13,                     /* CONST  */
  YYSYMBOL_VOID = 14,                      /* VOID  */
  YYSYMBOL_IF = 15,                        /* IF  */
  YYSYMBOL_ELSE = 16,                      /* ELSE  */
  YYSYMBOL_WHILE = 17,                     /* WHILE  */
  YYSYMBOL_BREAK = 18,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 19,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 20,                    /* RETURN  */
  YYSYMBOL_INTCONST = 21,                  /* INTCONST  */
  YYSYMBOL_FLOATCONST = 22,                /* FLOATCONST  */
  YYSYMBOL_IDENT = 23,                     /* IDENT  */
  YYSYMBOL_EQ = 24,                        /* EQ  */
  YYSYMBOL_NE = 25,                        /* NE  */
  YYSYMBOL_LE = 26,                        /* LE  */
  YYSYMBOL_GE = 27,                        /* GE  */
  YYSYMBOL_AND = 28,                       /* AND  */
  YYSYMBOL_OR = 29,                        /* OR  */
  YYSYMBOL_30_ = 30,                       /* '('  */
  YYSYMBOL_31_ = 31,                       /* ')'  */
  YYSYMBOL_32_ = 32,                       /* '{'  */
  YYSYMBOL_33_ = 33,                       /* '}'  */
  YYSYMBOL_34_ = 34,                       /* '['  */
  YYSYMBOL_35_ = 35,                       /* ']'  */
  YYSYMBOL_36_ = 36,                       /* ';'  */
  YYSYMBOL_37_ = 37,                       /* ','  */
  YYSYMBOL_38_ = 38,                       /* '='  */
  YYSYMBOL_39_ = 39,                       /* '+'  */
  YYSYMBOL_40_ = 40,                       /* '-'  */
  YYSYMBOL_41_ = 41,                       /* '*'  */
  YYSYMBOL_42_ = 42,                       /* '/'  */
  YYSYMBOL_43_ = 43,                       /* '%'  */
  YYSYMBOL_UNARY_MINUS = 44,               /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 45,                /* UNARY_PLUS  */
  YYSYMBOL_SHL = 46,                       /* SHL  */
  YYSYMBOL_SHR = 47,                       /* SHR  */
  YYSYMBOL_BAND = 48,                      /* BAND  */
  YYSYMBOL_BOR = 49,                       /* BOR  */
  YYSYMBOL_BXOR = 50,                      /* BXOR  */
  YYSYMBOL_YYACCEPT = 51,                  /* $accept  */
  YYSYMBOL_CompUnit = 52,                  /* CompUnit  */
  YYSYMBOL_Decl = 53,                      /* Decl  */
  YYSYMBOL_Type = 54,                      /* Type  */
  YYSYMBOL_VarDecl = 55,                   /* VarDecl  */
  YYSYMBOL_VarDefList = 56,                /* VarDefList  */
  YYSYMBOL_VarDef = 57,                    /* VarDef  */
  YYSYMBOL_ConstDecl = 58,                 /* ConstDecl  */
  YYSYMBOL_ConstDefList = 59,              /* ConstDefList  */
  YYSYMBOL_ConstDef = 60,                  /* ConstDef  */
  YYSYMBOL_ConstInitVal = 61,              /* ConstInitVal  */
  YYSYMBOL_ConstInitArray = 62,            /* ConstInitArray  */
  YYSYMBOL_ConstInitValList = 63,          /* ConstInitValList  */
  YYSYMBOL_FuncDef = 64,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 65,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 66,                /* FuncFParam  */
  YYSYMBOL_Block = 67,                     /* Block  */
  YYSYMBOL_BlockItems = 68,                /* BlockItems  */
  YYSYMBOL_BlockItem = 69,                 /* BlockItem  */
  YYSYMBOL_Stmt = 70,                      /* Stmt  */
  YYSYMBOL_Exp = 71,                       /* Exp  */
  YYSYMBOL_ConstExp = 72,                  /* ConstExp  */
  YYSYMBOL_BitExp = 73,                    /* BitExp  */
  YYSYMBOL_BitOrExp = 74,                  /* BitOrExp  */
  YYSYMBOL_BitXorExp = 75,                 /* BitXorExp  */
  YYSYMBOL_BitAndExp = 76,                 /* BitAndExp  */
  YYSYMBOL_ShiftExp = 77,                  /* ShiftExp  */
  YYSYMBOL_AddExp = 78,                    /* AddExp  */
  YYSYMBOL_MulExp = 79,                    /* MulExp  */
  YYSYMBOL_UnaryExp = 80,                  /* UnaryExp  */
  YYSYMBOL_PrimaryExp = 81,                /* PrimaryExp  */
  YYSYMBOL_LVal = 82,                      /* LVal  */
  YYSYMBOL_ArrayAccess = 83,               /* ArrayAccess  */
  YYSYMBOL_FuncCall = 84,                  /* FuncCall  */
  YYSYMBOL_FuncArgs = 85,                  /* FuncArgs  */
  YYSYMBOL_Cond = 86,                      /* Cond  */
  YYSYMBOL_LOrExp = 87,                    /* LOrExp  */
  YYSYMBOL_LAndExp = 88,                   /* LAndExp  */
  YYSYMBOL_EqExp = 89,                     /* EqExp  */
  YYSYMBOL_RelExp = 90,                    /* RelExp  */
  YYSYMBOL_InitVal = 91,                   /* InitVal  */
  YYSYMBOL_InitValList = 92,               /* InitValList  */
  YYSYMBOL_InitArray = 93,                 /* InitArray  */
  YYSYMBOL_ArrayDims = 94                  /* ArrayDims  */
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
#define YYFINAL  12
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   266

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  44
/* YYNRULES -- Number of rules.  */
#define YYNRULES  110
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  194

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   287


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
       2,     2,     2,     6,     2,     2,     2,    43,     2,     2,
      30,    31,    41,    39,    37,    40,     2,    42,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    36,
       4,    38,     5,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    34,     2,    35,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    32,     2,    33,     7,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    44,    45,    46,    47,    48,    49,    50
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   114,   114,   115,   116,   117,   118,   123,   124,   129,
     130,   131,   136,   150,   151,   161,   164,   167,   171,   178,
     192,   193,   203,   204,   211,   212,   216,   217,   221,   222,
     227,   231,   239,   240,   241,   245,   253,   267,   271,   272,
     276,   277,   282,   283,   284,   285,   286,   288,   290,   291,
     292,   293,   294,   299,   304,   312,   316,   317,   321,   322,
     326,   327,   331,   332,   333,   337,   338,   339,   344,   345,
     352,   358,   364,   365,   366,   370,   371,   376,   377,   378,
     379,   380,   385,   389,   392,   409,   412,   424,   425,   426,
     431,   435,   436,   440,   441,   445,   446,   447,   451,   452,
     453,   454,   455,   461,   462,   466,   467,   471,   472,   476,
     477
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
  "\"end of file\"", "error", "\"invalid token\"", "LOWER_THAN_ELSE",
  "'<'", "'>'", "'!'", "'~'", "NEG", "POS", "FUNC_IDENT", "INT", "FLOAT",
  "CONST", "VOID", "IF", "ELSE", "WHILE", "BREAK", "CONTINUE", "RETURN",
  "INTCONST", "FLOATCONST", "IDENT", "EQ", "NE", "LE", "GE", "AND", "OR",
  "'('", "')'", "'{'", "'}'", "'['", "']'", "';'", "','", "'='", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY_MINUS", "UNARY_PLUS", "SHL", "SHR",
  "BAND", "BOR", "BXOR", "$accept", "CompUnit", "Decl", "Type", "VarDecl",
  "VarDefList", "VarDef", "ConstDecl", "ConstDefList", "ConstDef",
  "ConstInitVal", "ConstInitArray", "ConstInitValList", "FuncDef",
  "FuncFParams", "FuncFParam", "Block", "BlockItems", "BlockItem", "Stmt",
  "Exp", "ConstExp", "BitExp", "BitOrExp", "BitXorExp", "BitAndExp",
  "ShiftExp", "AddExp", "MulExp", "UnaryExp", "PrimaryExp", "LVal",
  "ArrayAccess", "FuncCall", "FuncArgs", "Cond", "LOrExp", "LAndExp",
  "EqExp", "RelExp", "InitVal", "InitValList", "InitArray", "ArrayDims", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-123)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      81,  -123,  -123,    87,  -123,   176,  -123,   -10,  -123,  -123,
    -123,    -8,  -123,  -123,  -123,   -16,    26,  -123,    21,    47,
    -123,    42,   226,   152,  -123,     0,   226,   174,  -123,    -8,
      -4,    46,    -7,  -123,   226,   226,  -123,  -123,    55,   226,
     226,   226,    61,  -123,    53,    56,    63,    85,   112,    -5,
    -123,  -123,  -123,    80,  -123,    94,   101,   130,     6,   103,
    -123,  -123,  -123,    37,  -123,   104,   139,  -123,  -123,  -123,
    -123,  -123,  -123,  -123,   107,    -4,    87,  -123,  -123,   179,
     146,  -123,  -123,   149,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,  -123,  -123,   -21,   149,  -123,  -123,    43,
      28,   201,  -123,  -123,  -123,  -123,  -123,    -6,  -123,   226,
     160,    56,    63,    85,   112,   112,    -5,    -5,  -123,  -123,
    -123,   164,   101,   130,     6,     6,  -123,  -123,  -123,  -123,
    -123,   152,   165,  -123,   174,   175,   181,   180,   184,   221,
    -123,  -123,  -123,     0,  -123,  -123,  -123,   185,   177,   149,
     182,  -123,   226,   152,  -123,  -123,   174,  -123,   226,   226,
    -123,  -123,  -123,   189,  -123,   226,  -123,   149,  -123,  -123,
    -123,   195,    94,   198,  -123,   194,  -123,    98,    98,  -123,
     196,  -123,    98,  -123
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       6,     9,    10,     0,    11,     0,     4,     0,     7,     8,
       5,     0,     1,     2,     3,    15,     0,    13,     0,     0,
      20,    34,     0,     0,    12,     0,     0,     0,    19,     0,
       0,     0,     0,    32,     0,     0,    79,    80,    83,     0,
       0,     0,     0,    98,    55,    56,    58,    60,    62,    65,
      68,    72,    78,    82,    81,    53,    91,    93,    95,     0,
     103,    16,   104,    15,    14,     0,     0,    22,    25,    54,
      24,    21,    39,    31,   109,     0,     0,    75,    76,    89,
       0,    73,    74,   109,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,   105,     0,   109,    26,    28,     0,
       0,     0,    35,    30,    33,    86,    87,     0,    77,     0,
      17,    57,    59,    61,    63,    64,    66,    67,    69,    70,
      71,     0,    92,    94,    96,    97,    99,   100,   101,   102,
     108,     0,     0,    27,     0,     0,     0,     0,     0,     0,
      37,    44,    40,     0,    45,    38,    41,     0,    78,   109,
       0,    85,     0,     0,    84,   106,     0,    29,     0,     0,
      49,    50,    51,     0,    43,     0,    36,   109,    88,    18,
      23,     0,    90,     0,    52,     0,   110,     0,     0,    42,
      47,    48,     0,    46
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -123,  -123,    -3,    -2,  -123,  -123,   209,  -123,  -123,   206,
     -63,  -123,  -123,   232,  -123,   162,   -23,  -123,  -123,  -122,
     -22,  -123,    48,  -123,   155,   161,   159,    69,    74,   -14,
    -123,  -101,  -123,  -123,  -123,    83,    -1,   158,   154,    96,
     -51,  -123,  -123,   -77
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     5,     6,     7,     8,    16,    17,     9,    19,    20,
      67,    68,   109,    10,    32,    33,   154,   110,   155,   156,
      60,    70,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,   117,   181,    55,    56,    57,    58,
      61,   105,    62,   112
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      42,    11,    13,   108,    65,    69,   120,    73,   104,   158,
      99,   100,   140,    15,    21,    18,   141,    80,    22,    31,
      77,    78,    23,    63,    75,   161,    81,    82,    72,   142,
      76,   162,   101,   102,    34,    35,    91,    92,    93,     1,
       2,     3,     4,   145,    69,   146,   147,   148,   149,    36,
      37,    38,   113,     1,     2,    26,     4,   116,    39,    27,
      72,   150,    24,    25,   151,   190,   191,    40,    41,    74,
     193,    22,   131,    30,    31,    23,   143,   128,   129,   130,
     144,   167,   176,    28,    29,    79,   158,   158,   157,   160,
     165,   158,     1,     2,     3,     4,    83,   160,     1,     2,
     186,     4,    84,   180,    34,    35,    85,   152,   153,    34,
      35,    86,   179,   145,    94,   146,   147,   148,   149,    36,
      37,    38,    69,    95,    36,    37,    38,   173,    39,    96,
      72,    87,    88,    39,   151,    59,   103,    40,    41,   106,
     178,   111,    40,    41,    69,    34,    35,   136,   137,   138,
     139,    89,    90,   185,    97,    98,   124,   125,    34,    35,
      36,    37,    38,   126,   127,   157,   157,   182,   182,    39,
     157,    66,   107,    36,    37,    38,    12,   118,    40,    41,
      34,    35,    39,   119,    59,    34,    35,     1,     2,     3,
       4,    40,    41,   134,   135,    36,    37,    38,   163,   164,
      36,    37,    38,   166,    39,   168,    66,    34,    35,    39,
     115,   169,   192,    40,    41,   175,   170,   177,    40,    41,
     171,   174,    36,    37,    38,   184,   187,    34,    35,   188,
     189,    39,    34,    35,    64,    71,   159,    14,   114,   121,
      40,    41,    36,    37,    38,   123,   122,    36,    37,    38,
     133,    39,   183,   132,     0,     0,    39,   172,     0,     0,
      40,    41,     0,     0,     0,    40,    41
};

static const yytype_int16 yycheck[] =
{
      22,     3,     5,    66,    26,    27,    83,    30,    59,   110,
       4,     5,    33,    23,    30,    23,    37,    39,    34,    21,
      34,    35,    38,    23,    31,    31,    40,    41,    32,   106,
      37,    37,    26,    27,     6,     7,    41,    42,    43,    11,
      12,    13,    14,    15,    66,    17,    18,    19,    20,    21,
      22,    23,    75,    11,    12,    34,    14,    79,    30,    38,
      32,    33,    36,    37,    36,   187,   188,    39,    40,    23,
     192,    34,    94,    31,    76,    38,    33,    91,    92,    93,
      37,   144,   159,    36,    37,    30,   187,   188,   110,   111,
     141,   192,    11,    12,    13,    14,    35,   119,    11,    12,
     177,    14,    49,   166,     6,     7,    50,   110,   110,     6,
       7,    48,   163,    15,    34,    17,    18,    19,    20,    21,
      22,    23,   144,    29,    21,    22,    23,   149,    30,    28,
      32,    46,    47,    30,    36,    32,    33,    39,    40,    35,
     162,    34,    39,    40,   166,     6,     7,    99,   100,   101,
     102,    39,    40,   175,    24,    25,    87,    88,     6,     7,
      21,    22,    23,    89,    90,   187,   188,   168,   169,    30,
     192,    32,    33,    21,    22,    23,     0,    31,    39,    40,
       6,     7,    30,    34,    32,     6,     7,    11,    12,    13,
      14,    39,    40,    97,    98,    21,    22,    23,    38,    35,
      21,    22,    23,    38,    30,    30,    32,     6,     7,    30,
      31,    30,    16,    39,    40,    38,    36,    35,    39,    40,
      36,    36,    21,    22,    23,    36,    31,     6,     7,    31,
      36,    30,     6,     7,    25,    29,    35,     5,    76,    84,
      39,    40,    21,    22,    23,    86,    85,    21,    22,    23,
      96,    30,   169,    95,    -1,    -1,    30,    36,    -1,    -1,
      39,    40,    -1,    -1,    -1,    39,    40
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    11,    12,    13,    14,    52,    53,    54,    55,    58,
      64,    54,     0,    53,    64,    23,    56,    57,    23,    59,
      60,    30,    34,    38,    36,    37,    34,    38,    36,    37,
      31,    54,    65,    66,     6,     7,    21,    22,    23,    30,
      39,    40,    71,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    87,    88,    89,    90,    32,
      71,    91,    93,    23,    57,    71,    32,    61,    62,    71,
      72,    60,    32,    67,    23,    31,    37,    80,    80,    30,
      71,    80,    80,    35,    49,    50,    48,    46,    47,    39,
      40,    41,    42,    43,    34,    29,    28,    24,    25,     4,
       5,    26,    27,    33,    91,    92,    35,    33,    61,    63,
      68,    34,    94,    67,    66,    31,    71,    85,    31,    34,
      94,    75,    76,    77,    78,    78,    79,    79,    80,    80,
      80,    71,    88,    89,    90,    90,    73,    73,    73,    73,
      33,    37,    94,    33,    37,    15,    17,    18,    19,    20,
      33,    36,    53,    54,    67,    69,    70,    71,    82,    35,
      71,    31,    37,    38,    35,    91,    38,    61,    30,    30,
      36,    36,    36,    71,    36,    38,    94,    35,    71,    91,
      61,    86,    87,    86,    36,    71,    94,    31,    31,    36,
      70,    70,    16,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    51,    52,    52,    52,    52,    52,    53,    53,    54,
      54,    54,    55,    56,    56,    57,    57,    57,    57,    58,
      59,    59,    60,    60,    61,    61,    62,    62,    63,    63,
      64,    64,    65,    65,    65,    66,    66,    67,    68,    68,
      69,    69,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    71,    72,    73,    74,    74,    75,    75,
      76,    76,    77,    77,    77,    78,    78,    78,    79,    79,
      79,    79,    80,    80,    80,    80,    80,    81,    81,    81,
      81,    81,    82,    83,    83,    84,    84,    85,    85,    85,
      86,    87,    87,    88,    88,    89,    89,    89,    90,    90,
      90,    90,    90,    91,    91,    92,    92,    93,    93,    94,
      94
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     2,     1,     1,     0,     1,     1,     1,
       1,     1,     3,     1,     3,     1,     3,     5,     7,     4,
       1,     3,     3,     7,     1,     1,     2,     3,     1,     3,
       6,     5,     1,     3,     0,     3,     5,     3,     2,     0,
       1,     1,     4,     2,     1,     1,     7,     5,     5,     2,
       2,     2,     3,     1,     1,     1,     1,     3,     1,     3,
       1,     3,     1,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     1,     2,     2,     2,     2,     3,     1,     1,
       1,     1,     1,     1,     4,     4,     3,     1,     3,     0,
       1,     1,     3,     1,     3,     1,     3,     3,     1,     3,
       3,     3,     3,     1,     1,     1,     3,     2,     3,     0,
       4
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
  case 2: /* CompUnit: CompUnit Decl  */
#line 114 "src/bisonFile/sysy.y"
                           { root_node = merge_decl((yyvsp[-1].node), (yyvsp[0].node)); (yyval.node) = root_node; }
#line 1446 "src/sysy.tab.c"
    break;

  case 3: /* CompUnit: CompUnit FuncDef  */
#line 115 "src/bisonFile/sysy.y"
                           { root_node = merge_func_def((yyvsp[-1].node), (yyvsp[0].node)); (yyval.node) = root_node; }
#line 1452 "src/sysy.tab.c"
    break;

  case 4: /* CompUnit: Decl  */
#line 116 "src/bisonFile/sysy.y"
                           { root_node = new_comp_unit((yyvsp[0].node), NULL); (yyval.node) = root_node; }
#line 1458 "src/sysy.tab.c"
    break;

  case 5: /* CompUnit: FuncDef  */
#line 117 "src/bisonFile/sysy.y"
                           { root_node = new_comp_unit(NULL, (yyvsp[0].node)); (yyval.node) = root_node; }
#line 1464 "src/sysy.tab.c"
    break;

  case 6: /* CompUnit: %empty  */
#line 118 "src/bisonFile/sysy.y"
                           { (yyval.node) = NULL; }
#line 1470 "src/sysy.tab.c"
    break;

  case 7: /* Decl: VarDecl  */
#line 123 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1476 "src/sysy.tab.c"
    break;

  case 8: /* Decl: ConstDecl  */
#line 124 "src/bisonFile/sysy.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 1482 "src/sysy.tab.c"
    break;

  case 9: /* Type: INT  */
#line 129 "src/bisonFile/sysy.y"
                           { (yyval.data_type) = TYPE_INT; }
#line 1488 "src/sysy.tab.c"
    break;

  case 10: /* Type: FLOAT  */
#line 130 "src/bisonFile/sysy.y"
                           { (yyval.data_type) = TYPE_FLOAT; }
#line 1494 "src/sysy.tab.c"
    break;

  case 11: /* Type: VOID  */
#line 131 "src/bisonFile/sysy.y"
                           { (yyval.data_type) = TYPE_VOID; }
#line 1500 "src/sysy.tab.c"
    break;

  case 12: /* VarDecl: Type VarDefList ';'  */
#line 136 "src/bisonFile/sysy.y"
                           { 
        if ((yyvsp[-2].data_type) == TYPE_VOID) {
            yyerror("Cannot declare variables of type void");
            YYERROR;
        }
        // 设置链表中所有节点的数据类型
        for (ASTNode* p = (yyvsp[-1].node); p; p = p->next) {
            p->data_type = (yyvsp[-2].data_type);
        }
        (yyval.node) = (yyvsp[-1].node); 
    }
#line 1516 "src/sysy.tab.c"
    break;

  case 13: /* VarDefList: VarDef  */
#line 150 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1522 "src/sysy.tab.c"
    break;

  case 14: /* VarDefList: VarDefList ',' VarDef  */
#line 151 "src/bisonFile/sysy.y"
                              { 
        /* 将新的变量定义节点链接到列表末尾 */
        (yyval.node) = (yyvsp[-2].node);
        ASTNode *p = (yyvsp[-2].node);
        while (p->next) p = p->next;
        p->next = (yyvsp[0].node);
    }
#line 1534 "src/sysy.tab.c"
    break;

  case 15: /* VarDef: IDENT  */
#line 161 "src/bisonFile/sysy.y"
                             { 
        (yyval.node) = new_decl(NODE_VAR_DECL, (yyvsp[0].str), NULL, NULL, (yylsp[0]).first_line); 
    }
#line 1542 "src/sysy.tab.c"
    break;

  case 16: /* VarDef: IDENT '=' InitVal  */
#line 164 "src/bisonFile/sysy.y"
                            { 
        (yyval.node) = new_decl(NODE_VAR_DECL, (yyvsp[-2].str), NULL, (yyvsp[0].node), (yylsp[-2]).first_line); 
    }
#line 1550 "src/sysy.tab.c"
    break;

  case 17: /* VarDef: IDENT '[' Exp ']' ArrayDims  */
#line 167 "src/bisonFile/sysy.y"
                                     { 
        ASTNode* first_dim = new_array_dims((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = new_decl(NODE_ARRAY_DECL, (yyvsp[-4].str), first_dim, NULL, (yylsp[-4]).first_line); 
    }
#line 1559 "src/sysy.tab.c"
    break;

  case 18: /* VarDef: IDENT '[' Exp ']' ArrayDims '=' InitVal  */
#line 171 "src/bisonFile/sysy.y"
                                              { 
        ASTNode* first_dim = new_array_dims((yyvsp[-4].node), (yyvsp[-2].node));
        (yyval.node) = new_decl(NODE_ARRAY_DECL, (yyvsp[-6].str), first_dim, (yyvsp[0].node), (yylsp[-6]).first_line); 
    }
#line 1568 "src/sysy.tab.c"
    break;

  case 19: /* ConstDecl: CONST Type ConstDefList ';'  */
#line 178 "src/bisonFile/sysy.y"
                                { 
        if ((yyvsp[-2].data_type) == TYPE_VOID) {
            yyerror("Cannot declare constants of type void");
            YYERROR;
        }
         // 设置链表中所有节点的数据类型
        for (ASTNode* p = (yyvsp[-1].node); p; p = p->next) {
            p->data_type = (yyvsp[-2].data_type);
        }
        (yyval.node) = (yyvsp[-1].node); 
    }
#line 1584 "src/sysy.tab.c"
    break;

  case 20: /* ConstDefList: ConstDef  */
#line 192 "src/bisonFile/sysy.y"
             { (yyval.node) = (yyvsp[0].node); }
#line 1590 "src/sysy.tab.c"
    break;

  case 21: /* ConstDefList: ConstDefList ',' ConstDef  */
#line 193 "src/bisonFile/sysy.y"
                                {
        /* 将新的变量定义节点链接到列表末尾 */
        (yyval.node) = (yyvsp[-2].node);
        ASTNode *p = (yyvsp[-2].node);
        while (p->next) p = p->next;
        p->next = (yyvsp[0].node);
    }
#line 1602 "src/sysy.tab.c"
    break;

  case 22: /* ConstDef: IDENT '=' ConstInitVal  */
#line 203 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_decl(NODE_CONST_DECL, (yyvsp[-2].str), NULL, (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 1608 "src/sysy.tab.c"
    break;

  case 23: /* ConstDef: IDENT '[' Exp ']' ArrayDims '=' ConstInitVal  */
#line 204 "src/bisonFile/sysy.y"
                                                   { 
        ASTNode* first_dim = new_array_dims((yyvsp[-4].node), (yyvsp[-2].node));
        (yyval.node) = new_decl(NODE_CONST_ARRAY_DECL, (yyvsp[-6].str), first_dim, (yyvsp[0].node), (yylsp[-6]).first_line); 
    }
#line 1617 "src/sysy.tab.c"
    break;

  case 24: /* ConstInitVal: ConstExp  */
#line 211 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_init_val((yyvsp[0].node), NULL, (yylsp[0]).first_line); }
#line 1623 "src/sysy.tab.c"
    break;

  case 25: /* ConstInitVal: ConstInitArray  */
#line 212 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1629 "src/sysy.tab.c"
    break;

  case 26: /* ConstInitArray: '{' '}'  */
#line 216 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_init_list(NULL, (yylsp[-1]).first_line); }
#line 1635 "src/sysy.tab.c"
    break;

  case 27: /* ConstInitArray: '{' ConstInitValList '}'  */
#line 217 "src/bisonFile/sysy.y"
                             { (yyval.node) = (yyvsp[-1].node); }
#line 1641 "src/sysy.tab.c"
    break;

  case 28: /* ConstInitValList: ConstInitVal  */
#line 221 "src/bisonFile/sysy.y"
                                  { (yyval.node) = new_init_list((yyvsp[0].node), (yylsp[0]).first_line); }
#line 1647 "src/sysy.tab.c"
    break;

  case 29: /* ConstInitValList: ConstInitValList ',' ConstInitVal  */
#line 222 "src/bisonFile/sysy.y"
                                      { (yyval.node) = append_init_list((yyvsp[-2].node), (yyvsp[0].node)); }
#line 1653 "src/sysy.tab.c"
    break;

  case 30: /* FuncDef: Type IDENT '(' FuncFParams ')' Block  */
#line 227 "src/bisonFile/sysy.y"
                                         {
        (yyval.node) = new_func_def((yyvsp[-5].data_type), (yyvsp[-4].str), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]).first_line);
        check_main((yyvsp[-4].str));
    }
#line 1662 "src/sysy.tab.c"
    break;

  case 31: /* FuncDef: Type IDENT '(' ')' Block  */
#line 231 "src/bisonFile/sysy.y"
                             {
        (yyval.node) = new_func_def((yyvsp[-4].data_type), (yyvsp[-3].str), NULL, (yyvsp[0].node), (yylsp[-4]).first_line);
        check_main((yyvsp[-3].str));
    }
#line 1671 "src/sysy.tab.c"
    break;

  case 32: /* FuncFParams: FuncFParam  */
#line 239 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1677 "src/sysy.tab.c"
    break;

  case 33: /* FuncFParams: FuncFParams ',' FuncFParam  */
#line 240 "src/bisonFile/sysy.y"
                               { (yyval.node) = append_func_param((yyvsp[-2].node), (yyvsp[0].node)); }
#line 1683 "src/sysy.tab.c"
    break;

  case 34: /* FuncFParams: %empty  */
#line 241 "src/bisonFile/sysy.y"
                           { (yyval.node) = NULL; }
#line 1689 "src/sysy.tab.c"
    break;

  case 35: /* FuncFParam: Type IDENT ArrayDims  */
#line 245 "src/bisonFile/sysy.y"
                               { 
        // 标量参数或指定所有维度的数组参数
        if ((yyvsp[-2].data_type) == TYPE_VOID) {
            yyerror("Function parameter cannot be void");
            YYERROR;
        }
        (yyval.node) = new_var_decl((yyvsp[-1].str), (yyvsp[-2].data_type), (yyvsp[0].node), NULL, (yylsp[-2]).first_line); 
    }
#line 1702 "src/sysy.tab.c"
    break;

  case 36: /* FuncFParam: Type IDENT '[' ']' ArrayDims  */
#line 253 "src/bisonFile/sysy.y"
                                    {
        // 第一维未知的数组参数
        if ((yyvsp[-4].data_type) == TYPE_VOID) {
            yyerror("Function parameter cannot be void");
            YYERROR;
        }
        // 创建一个特殊的数组维度节点表示第一维未知
        ASTNode *first_dim = new_array_dims(NULL, (yyvsp[0].node));
        (yyval.node) = new_var_decl((yyvsp[-3].str), (yyvsp[-4].data_type), first_dim, NULL, (yylsp[-4]).first_line);
    }
#line 1717 "src/sysy.tab.c"
    break;

  case 37: /* Block: '{' BlockItems '}'  */
#line 267 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_block((yyvsp[-1].node), (yylsp[-2]).first_line); }
#line 1723 "src/sysy.tab.c"
    break;

  case 38: /* BlockItems: BlockItems BlockItem  */
#line 271 "src/bisonFile/sysy.y"
                           { (yyval.node) = append_block_item((yyvsp[-1].node), (yyvsp[0].node)); }
#line 1729 "src/sysy.tab.c"
    break;

  case 39: /* BlockItems: %empty  */
#line 272 "src/bisonFile/sysy.y"
                           { (yyval.node) = NULL; }
#line 1735 "src/sysy.tab.c"
    break;

  case 40: /* BlockItem: Decl  */
#line 276 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1741 "src/sysy.tab.c"
    break;

  case 41: /* BlockItem: Stmt  */
#line 277 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1747 "src/sysy.tab.c"
    break;

  case 42: /* Stmt: LVal '=' Exp ';'  */
#line 282 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_assign((yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-3]).first_line); }
#line 1753 "src/sysy.tab.c"
    break;

  case 43: /* Stmt: Exp ';'  */
#line 283 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_expr_stmt((yyvsp[-1].node), (yylsp[-1]).first_line); }
#line 1759 "src/sysy.tab.c"
    break;

  case 44: /* Stmt: ';'  */
#line 284 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_expr_stmt(NULL, (yylsp[0]).first_line); }
#line 1765 "src/sysy.tab.c"
    break;

  case 45: /* Stmt: Block  */
#line 285 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1771 "src/sysy.tab.c"
    break;

  case 46: /* Stmt: IF '(' Cond ')' Stmt ELSE Stmt  */
#line 287 "src/bisonFile/sysy.y"
                          { (yyval.node) = new_if((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-6]).first_line); }
#line 1777 "src/sysy.tab.c"
    break;

  case 47: /* Stmt: IF '(' Cond ')' Stmt  */
#line 289 "src/bisonFile/sysy.y"
                          { (yyval.node) = new_if((yyvsp[-2].node), (yyvsp[0].node), NULL, (yylsp[-4]).first_line); }
#line 1783 "src/sysy.tab.c"
    break;

  case 48: /* Stmt: WHILE '(' Cond ')' Stmt  */
#line 290 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_while((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]).first_line); }
#line 1789 "src/sysy.tab.c"
    break;

  case 49: /* Stmt: BREAK ';'  */
#line 291 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_break((yylsp[-1]).first_line); }
#line 1795 "src/sysy.tab.c"
    break;

  case 50: /* Stmt: CONTINUE ';'  */
#line 292 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_continue((yylsp[-1]).first_line); }
#line 1801 "src/sysy.tab.c"
    break;

  case 51: /* Stmt: RETURN ';'  */
#line 293 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_return(NULL, (yylsp[-1]).first_line); }
#line 1807 "src/sysy.tab.c"
    break;

  case 52: /* Stmt: RETURN Exp ';'  */
#line 294 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_return((yyvsp[-1].node), (yylsp[-2]).first_line); }
#line 1813 "src/sysy.tab.c"
    break;

  case 53: /* Exp: LOrExp  */
#line 299 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1819 "src/sysy.tab.c"
    break;

  case 54: /* ConstExp: Exp  */
#line 304 "src/bisonFile/sysy.y"
                           { 
        (yyval.node) = validate_const_exp((yyvsp[0].node)); 
    }
#line 1827 "src/sysy.tab.c"
    break;

  case 55: /* BitExp: BitOrExp  */
#line 312 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1833 "src/sysy.tab.c"
    break;

  case 56: /* BitOrExp: BitXorExp  */
#line 316 "src/bisonFile/sysy.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 1839 "src/sysy.tab.c"
    break;

  case 57: /* BitOrExp: BitOrExp BOR BitXorExp  */
#line 317 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_bin_op(OP_BOR, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1845 "src/sysy.tab.c"
    break;

  case 58: /* BitXorExp: BitAndExp  */
#line 321 "src/bisonFile/sysy.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 1851 "src/sysy.tab.c"
    break;

  case 59: /* BitXorExp: BitXorExp BXOR BitAndExp  */
#line 322 "src/bisonFile/sysy.y"
                             { (yyval.node) = new_bin_op(OP_BXOR, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1857 "src/sysy.tab.c"
    break;

  case 60: /* BitAndExp: ShiftExp  */
#line 326 "src/bisonFile/sysy.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 1863 "src/sysy.tab.c"
    break;

  case 61: /* BitAndExp: BitAndExp BAND ShiftExp  */
#line 327 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_BAND, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1869 "src/sysy.tab.c"
    break;

  case 62: /* ShiftExp: AddExp  */
#line 331 "src/bisonFile/sysy.y"
                         { (yyval.node) = (yyvsp[0].node); }
#line 1875 "src/sysy.tab.c"
    break;

  case 63: /* ShiftExp: ShiftExp SHL AddExp  */
#line 332 "src/bisonFile/sysy.y"
                         { (yyval.node) = new_bin_op(OP_SHL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1881 "src/sysy.tab.c"
    break;

  case 64: /* ShiftExp: ShiftExp SHR AddExp  */
#line 333 "src/bisonFile/sysy.y"
                         { (yyval.node) = new_bin_op(OP_SHR, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1887 "src/sysy.tab.c"
    break;

  case 65: /* AddExp: MulExp  */
#line 337 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1893 "src/sysy.tab.c"
    break;

  case 66: /* AddExp: AddExp '+' MulExp  */
#line 338 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_bin_op(OP_ADD, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 1899 "src/sysy.tab.c"
    break;

  case 67: /* AddExp: AddExp '-' MulExp  */
#line 339 "src/bisonFile/sysy.y"
                           { (yyval.node) = new_bin_op(OP_SUB, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 1905 "src/sysy.tab.c"
    break;

  case 68: /* MulExp: UnaryExp  */
#line 344 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 1911 "src/sysy.tab.c"
    break;

  case 69: /* MulExp: MulExp '*' UnaryExp  */
#line 345 "src/bisonFile/sysy.y"
                           { 
        (yyval.node) = new_bin_op(OP_MUL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line);
        // 处理类型转换
        if ((yyvsp[-2].node)->data_type == TYPE_FLOAT || (yyvsp[0].node)->data_type == TYPE_FLOAT) {
            (yyval.node)->data_type = TYPE_FLOAT;
        }
    }
#line 1923 "src/sysy.tab.c"
    break;

  case 70: /* MulExp: MulExp '/' UnaryExp  */
#line 352 "src/bisonFile/sysy.y"
                           { 
        (yyval.node) = new_bin_op(OP_DIV, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); 
        if ((yyvsp[-2].node)->data_type == TYPE_FLOAT || (yyvsp[0].node)->data_type == TYPE_FLOAT) {
            (yyval.node)->data_type = TYPE_FLOAT;
        }
    }
#line 1934 "src/sysy.tab.c"
    break;

  case 71: /* MulExp: MulExp '%' UnaryExp  */
#line 358 "src/bisonFile/sysy.y"
                           { 
        (yyval.node) = new_bin_op(OP_MOD, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]).first_line); 
    }
#line 1942 "src/sysy.tab.c"
    break;

  case 72: /* UnaryExp: PrimaryExp  */
#line 364 "src/bisonFile/sysy.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 1948 "src/sysy.tab.c"
    break;

  case 73: /* UnaryExp: '+' UnaryExp  */
#line 365 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1954 "src/sysy.tab.c"
    break;

  case 74: /* UnaryExp: '-' UnaryExp  */
#line 366 "src/bisonFile/sysy.y"
                            { 
        (yyval.node) = new_unary_op(OP_NEG, (yyvsp[0].node), (yylsp[-1]).first_line);
        (yyval.node)->data_type = (yyvsp[0].node)->data_type;
    }
#line 1963 "src/sysy.tab.c"
    break;

  case 75: /* UnaryExp: '!' UnaryExp  */
#line 370 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_unary_op(OP_NOT, (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1969 "src/sysy.tab.c"
    break;

  case 76: /* UnaryExp: '~' UnaryExp  */
#line 371 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_unary_op(OP_BNOT, (yyvsp[0].node), (yylsp[-1]).first_line); }
#line 1975 "src/sysy.tab.c"
    break;

  case 77: /* PrimaryExp: '(' Exp ')'  */
#line 376 "src/bisonFile/sysy.y"
                          { (yyval.node) = (yyvsp[-1].node); }
#line 1981 "src/sysy.tab.c"
    break;

  case 78: /* PrimaryExp: LVal  */
#line 377 "src/bisonFile/sysy.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 1987 "src/sysy.tab.c"
    break;

  case 79: /* PrimaryExp: INTCONST  */
#line 378 "src/bisonFile/sysy.y"
                         { (yyval.node) = new_const_expr((yyvsp[0].int_val), (yylsp[0]).first_line); }
#line 1993 "src/sysy.tab.c"
    break;

  case 80: /* PrimaryExp: FLOATCONST  */
#line 379 "src/bisonFile/sysy.y"
                         { (yyval.node) = new_float_const_expr((yyvsp[0].float_val), (yylsp[0]).first_line); }
#line 1999 "src/sysy.tab.c"
    break;

  case 81: /* PrimaryExp: FuncCall  */
#line 380 "src/bisonFile/sysy.y"
                         { (yyval.node) = (yyvsp[0].node); }
#line 2005 "src/sysy.tab.c"
    break;

  case 82: /* LVal: ArrayAccess  */
#line 385 "src/bisonFile/sysy.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 2011 "src/sysy.tab.c"
    break;

  case 83: /* ArrayAccess: IDENT  */
#line 389 "src/bisonFile/sysy.y"
                          { 
        (yyval.node) = new_lval((yyvsp[0].str), NULL, (yylsp[0]).first_line); 
    }
#line 2019 "src/sysy.tab.c"
    break;

  case 84: /* ArrayAccess: ArrayAccess '[' Exp ']'  */
#line 392 "src/bisonFile/sysy.y"
                               { 
        // 构建数组访问节点
        (yyval.node) = (yyvsp[-3].node);  // 保持原有的LVal节点
        // 将新的下标添加到索引链表中
        ASTNode* index = new_array_index((yyvsp[-1].node), (yylsp[-2]).first_line);
        // 将新的索引添加到已有索引链表的末尾
        if (!(yyval.node)->lval.indices) {
            (yyval.node)->lval.indices = index;
        } else {
            ASTNode* p = (yyval.node)->lval.indices;
            while (p->next) p = p->next;
            p->next = index;
        }
    }
#line 2038 "src/sysy.tab.c"
    break;

  case 85: /* FuncCall: IDENT '(' FuncArgs ')'  */
#line 409 "src/bisonFile/sysy.y"
                              { 
        (yyval.node) = new_func_call((yyvsp[-3].str), (yyvsp[-1].node), (yylsp[-3]).first_line);
    }
#line 2046 "src/sysy.tab.c"
    break;

  case 86: /* FuncCall: IDENT '(' ')'  */
#line 412 "src/bisonFile/sysy.y"
                                { 
        (yyval.node) = new_func_call((yyvsp[-2].str), NULL, (yylsp[-2]).first_line);
        // 查找函数符号，检查返回类型
        Symbol *sym = find_symbol((yyvsp[-2].str));
        if (sym && sym->sym_type == SYM_FUNC) {
            (yyval.node)->data_type = sym->data_type;
        }
    }
#line 2059 "src/sysy.tab.c"
    break;

  case 87: /* FuncArgs: Exp  */
#line 424 "src/bisonFile/sysy.y"
                                { (yyval.node) = append_arg(NULL, (yyvsp[0].node)); }
#line 2065 "src/sysy.tab.c"
    break;

  case 88: /* FuncArgs: FuncArgs ',' Exp  */
#line 425 "src/bisonFile/sysy.y"
                             { (yyval.node) = append_arg((yyvsp[-2].node), (yyvsp[0].node)); }
#line 2071 "src/sysy.tab.c"
    break;

  case 89: /* FuncArgs: %empty  */
#line 426 "src/bisonFile/sysy.y"
                             { (yyval.node) = NULL; }
#line 2077 "src/sysy.tab.c"
    break;

  case 90: /* Cond: LOrExp  */
#line 431 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 2083 "src/sysy.tab.c"
    break;

  case 91: /* LOrExp: LAndExp  */
#line 435 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 2089 "src/sysy.tab.c"
    break;

  case 92: /* LOrExp: LOrExp OR LAndExp  */
#line 436 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_OR, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2095 "src/sysy.tab.c"
    break;

  case 93: /* LAndExp: EqExp  */
#line 440 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 2101 "src/sysy.tab.c"
    break;

  case 94: /* LAndExp: LAndExp AND EqExp  */
#line 441 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_AND, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2107 "src/sysy.tab.c"
    break;

  case 95: /* EqExp: RelExp  */
#line 445 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 2113 "src/sysy.tab.c"
    break;

  case 96: /* EqExp: EqExp EQ RelExp  */
#line 446 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_EQ, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2119 "src/sysy.tab.c"
    break;

  case 97: /* EqExp: EqExp NE RelExp  */
#line 447 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_NE, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2125 "src/sysy.tab.c"
    break;

  case 98: /* RelExp: BitExp  */
#line 451 "src/bisonFile/sysy.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 2131 "src/sysy.tab.c"
    break;

  case 99: /* RelExp: RelExp '<' BitExp  */
#line 452 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_LT, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2137 "src/sysy.tab.c"
    break;

  case 100: /* RelExp: RelExp '>' BitExp  */
#line 453 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_GT, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2143 "src/sysy.tab.c"
    break;

  case 101: /* RelExp: RelExp LE BitExp  */
#line 454 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_LE, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2149 "src/sysy.tab.c"
    break;

  case 102: /* RelExp: RelExp GE BitExp  */
#line 455 "src/bisonFile/sysy.y"
                            { (yyval.node) = new_bin_op(OP_GE, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-2]).first_line); }
#line 2155 "src/sysy.tab.c"
    break;

  case 103: /* InitVal: Exp  */
#line 461 "src/bisonFile/sysy.y"
                                   { (yyval.node) = new_init_val((yyvsp[0].node), NULL, (yylsp[0]).first_line); }
#line 2161 "src/sysy.tab.c"
    break;

  case 104: /* InitVal: InitArray  */
#line 462 "src/bisonFile/sysy.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 2167 "src/sysy.tab.c"
    break;

  case 105: /* InitValList: InitVal  */
#line 466 "src/bisonFile/sysy.y"
                                 { (yyval.node) = new_init_list((yyvsp[0].node), (yylsp[0]).first_line); }
#line 2173 "src/sysy.tab.c"
    break;

  case 106: /* InitValList: InitValList ',' InitVal  */
#line 467 "src/bisonFile/sysy.y"
                                 { (yyval.node) = append_init_list((yyvsp[-2].node), (yyvsp[0].node)); }
#line 2179 "src/sysy.tab.c"
    break;

  case 107: /* InitArray: '{' '}'  */
#line 471 "src/bisonFile/sysy.y"
                                   { (yyval.node) = new_init_list(NULL, (yylsp[-1]).first_line); }
#line 2185 "src/sysy.tab.c"
    break;

  case 108: /* InitArray: '{' InitValList '}'  */
#line 472 "src/bisonFile/sysy.y"
                                   { (yyval.node) = (yyvsp[-1].node); }
#line 2191 "src/sysy.tab.c"
    break;

  case 109: /* ArrayDims: %empty  */
#line 476 "src/bisonFile/sysy.y"
                                   { (yyval.node) = NULL; }
#line 2197 "src/sysy.tab.c"
    break;

  case 110: /* ArrayDims: '[' Exp ']' ArrayDims  */
#line 477 "src/bisonFile/sysy.y"
                                { 
        (yyval.node) = new_array_dims((yyvsp[-2].node), (yyvsp[0].node));
    }
#line 2205 "src/sysy.tab.c"
    break;


#line 2209 "src/sysy.tab.c"

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

#line 482 "src/bisonFile/sysy.y"


/* 错误处理函数 */
void yyerror(const char *s) {
    fprintf(stderr, "语法错误: %s\n", s);
}

/* 实现辅助函数 */
ASTNode *merge_decl(ASTNode *root, ASTNode* decl) {
    ASTNode* last = root->comp.children;
    while (last->next) {
        last = last->next;
    }
    last->next = decl;
    return root;
}

ASTNode *merge_func_def(ASTNode *root, ASTNode* func) {
    return merge_decl(root, func);
}

ASTNode* append_func_param(ASTNode* params, ASTNode* param) {
    return list_append(params, param);
}

ASTNode* append_arg(ASTNode* list, ASTNode* arg) {
    if (!list) list = arg;
    else {
        ASTNode* p = list;
        while (p->next) p = p->next;
        p->next = arg;
    }
    return list;
}

ASTNode* append_block_item(ASTNode* items, ASTNode* item) {
    if (!items) {
        items = (ASTNode *)calloc(1, sizeof(ASTNode));
        items->type = NODE_BLOCK;
        items->block.items = item;
        items->line_no = item->line_no;
        items->next = NULL;
        return items;
    }
    // 追加到块项目的链表末尾
    ASTNode *last = items->block.items;
    if (!last) {
        items->block.items = item;
    } else {
        while (last->next) last = last->next;
        last->next = item;
    }
    return items;
}

ASTNode* validate_const_exp(ASTNode* expr) {
    // 这里应该实现验证常量表达式的函数
    return expr;
}

ASTNode* append_init_list(ASTNode* list, ASTNode* item) {
    // return list_append(list, item);
    ASTNode *p = list->init_list.first;
    while (p->next) p = p->next;
    p->next = item;
    return list;
}

void check_main(char* name) {
    // 检查是否是main函数
    if (strcmp(name, "main") == 0) {
        // 做一些验证
    }
}
