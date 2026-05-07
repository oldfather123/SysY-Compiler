/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SRC_SYSY_TAB_H_INCLUDED
# define YY_YY_SRC_SYSY_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    LOWER_THAN_ELSE = 258,         /* LOWER_THAN_ELSE  */
    NEG = 259,                     /* NEG  */
    POS = 260,                     /* POS  */
    FUNC_IDENT = 261,              /* FUNC_IDENT  */
    INT = 262,                     /* INT  */
    FLOAT = 263,                   /* FLOAT  */
    CONST = 264,                   /* CONST  */
    VOID = 265,                    /* VOID  */
    IF = 266,                      /* IF  */
    ELSE = 267,                    /* ELSE  */
    WHILE = 268,                   /* WHILE  */
    BREAK = 269,                   /* BREAK  */
    CONTINUE = 270,                /* CONTINUE  */
    RETURN = 271,                  /* RETURN  */
    INTCONST = 272,                /* INTCONST  */
    FLOATCONST = 273,              /* FLOATCONST  */
    IDENT = 274,                   /* IDENT  */
    EQ = 275,                      /* EQ  */
    NE = 276,                      /* NE  */
    LE = 277,                      /* LE  */
    GE = 278,                      /* GE  */
    AND = 279,                     /* AND  */
    OR = 280,                      /* OR  */
    UNARY_MINUS = 281,             /* UNARY_MINUS  */
    UNARY_PLUS = 282,              /* UNARY_PLUS  */
    SHL = 283,                     /* SHL  */
    SHR = 284,                     /* SHR  */
    BAND = 285,                    /* BAND  */
    BOR = 286,                     /* BOR  */
    BXOR = 287                     /* BXOR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 66 "src/bisonFile/sysy.y"

    int int_val;
    double float_val;
    char* str;
    struct ASTNode* node;  /* 修改为struct ASTNode* */
    DataType data_type;         /* 添加一个用于基本类型的字段 */

#line 104 "src/sysy.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_SRC_SYSY_TAB_H_INCLUDED  */
