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

#ifndef YY_YY_PARSER_HPP_INCLUDED
# define YY_YY_PARSER_HPP_INCLUDED
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
    INT = 258,                     /* INT  */
    FLOAT = 259,                   /* FLOAT  */
    INTTYPE = 260,                 /* INTTYPE  */
    FLOATTYPE = 261,               /* FLOATTYPE  */
    VOID = 262,                    /* VOID  */
    CONST = 263,                   /* CONST  */
    RETURN = 264,                  /* RETURN  */
    IF = 265,                      /* IF  */
    ELSE = 266,                    /* ELSE  */
    WHILE = 267,                   /* WHILE  */
    BREAK = 268,                   /* BREAK  */
    CONTINUE = 269,                /* CONTINUE  */
    ID = 270,                      /* ID  */
    GTE = 271,                     /* GTE  */
    LTE = 272,                     /* LTE  */
    GT = 273,                      /* GT  */
    LT = 274,                      /* LT  */
    EQ = 275,                      /* EQ  */
    NEQ = 276,                     /* NEQ  */
    LP = 277,                      /* LP  */
    RP = 278,                      /* RP  */
    LB = 279,                      /* LB  */
    RB = 280,                      /* RB  */
    LC = 281,                      /* LC  */
    RC = 282,                      /* RC  */
    COMMA = 283,                   /* COMMA  */
    SEMICOLON = 284,               /* SEMICOLON  */
    NOT = 285,                     /* NOT  */
    ASSIGN = 286,                  /* ASSIGN  */
    MINUS = 287,                   /* MINUS  */
    ADD = 288,                     /* ADD  */
    MUL = 289,                     /* MUL  */
    DIV = 290,                     /* DIV  */
    MOD = 291,                     /* MOD  */
    AND = 292,                     /* AND  */
    OR = 293,                      /* OR  */
    LOWER_THEN_ELSE = 294          /* LOWER_THEN_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 21 "parser.y"

    CompUnitAST* comp_unit;
    DeclDefAST* decl_def;
    DeclAST* decl;
    DefListAST* def_list;
    DefAST* def;
    ArraysAST* arrays;
    InitValListAST* init_val_list;
    InitValAST* init_val;
    FuncDefAST* func_def;
    FuncParamListAST* func_param_list;
    FuncParamAST* func_param;
    BlockAST* block;
    BlockItemListAST* block_item_list;
    BlockItemAST* block_it;
    StmtAST* stmt;
    ReturnStmtAST* ret_stmt;
    CondStmtAST* cond_stmt;
    LoopStmtAST* loop_stmt;
    LValAST* l_val;
    PrimaryExpAST* pri_exp;
    NumberAST* number;
    UnaryExpAST* unary_exp;
    CallAST* call;
    FuncArgListAST* func_arg_list;
    MulExpAST* mul_exp;
    AddExpAST* add_exp;
    RelExpAST* rel_exp;
    EqExpAST* eq_exp;
    LAndExpAST* l_and_exp;
    LOrExpAST* l_or_exp;

    TYPE type;
    UOP op;
    string* token;
    int int_val;
    float float_val;

#line 142 "parser.hpp"

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


#endif /* !YY_YY_PARSER_HPP_INCLUDED  */
