#include "../include/ast.h"
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

#ifndef YY_YY_FRONT_END_SYSY_PARSER_TAB_HH_INCLUDED
# define YY_YY_FRONT_END_SYSY_PARSER_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
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
    IDENT = 258,                   /* IDENT  */
    FLOAT_CONST = 259,             /* FLOAT_CONST  */
    INT_CONST = 260,               /* INT_CONST  */
    LEQ = 261,                     /* LEQ  */
    GEQ = 262,                     /* GEQ  */
    EQ = 263,                      /* EQ  */
    NE = 264,                      /* NE  */
    AND = 265,                     /* AND  */
    OR = 266,                      /* OR  */
    NOT = 267,                     /* NOT  */
    CONST = 268,                   /* CONST  */
    IF = 269,                      /* IF  */
    ELSE = 270,                    /* ELSE  */
    WHILE = 271,                   /* WHILE  */
    NONE_TYPE = 272,               /* NONE_TYPE  */
    INT = 273,                     /* INT  */
    FLOAT = 274,                   /* FLOAT  */
    RETURN = 275,                  /* RETURN  */
    BREAK = 276,                   /* BREAK  */
    CONTINUE = 277,                /* CONTINUE  */
    ERROR = 278,                   /* ERROR  */
    THEN = 279                     /* THEN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 16 "front_end/sysy_parser.y"

    char* error_msg;
    Symbol* symbol_token;
    int int_token;         double float_token; // 对于SysY的浮点常量，我们需要先以double类型计算，再在语法树节点创建的时候转为float
    Program program;
    CompUnit comp_unit;    std::vector<CompUnit>* comp_list;
    ExprBase expression;   std::vector<ExprBase>* expression_list;
    DeclBase decl;
    Def def;               std::vector<Def>* def_list;
    FuncDef func_def;
    FuncFParam formal;   std::vector<FuncFParam>* formal_list;
    Stmt stmt;
    Block block;
    BlockItem block_item;   std::vector<BlockItem>* block_item_list;
    InitValBase initval;  std::vector<InitValBase>* initval_list;

#line 105 "front_end/sysy_parser.tab.hh"

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


#endif /* !YY_YY_FRONT_END_SYSY_PARSER_TAB_HH_INCLUDED  */
