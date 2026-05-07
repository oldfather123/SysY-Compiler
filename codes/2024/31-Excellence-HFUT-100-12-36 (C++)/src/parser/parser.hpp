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
    VOID = 260,                    /* VOID  */
    IF = 261,                      /* IF  */
    ELSE = 262,                    /* ELSE  */
    WHILE = 263,                   /* WHILE  */
    BREAK = 264,                   /* BREAK  */
    CONTINUE = 265,                /* CONTINUE  */
    RETURN = 266,                  /* RETURN  */
    Ident = 267,                   /* Ident  */
    CONST = 268,                   /* CONST  */
    ADD = 269,                     /* ADD  */
    SUB = 270,                     /* SUB  */
    MUL = 271,                     /* MUL  */
    DIV = 272,                     /* DIV  */
    MOD = 273,                     /* MOD  */
    LPAREN = 274,                  /* LPAREN  */
    RPAREN = 275,                  /* RPAREN  */
    LBRACKET = 276,                /* LBRACKET  */
    RBRACKET = 277,                /* RBRACKET  */
    LBRACE = 278,                  /* LBRACE  */
    RBRACE = 279,                  /* RBRACE  */
    IntConst = 280,                /* IntConst  */
    FloatConst = 281,              /* FloatConst  */
    LESS = 282,                    /* LESS  */
    GREATER = 283,                 /* GREATER  */
    EQUAL = 284,                   /* EQUAL  */
    NOT = 285,                     /* NOT  */
    LESS_EQUAL = 286,              /* LESS_EQUAL  */
    GREATER_EQUAL = 287,           /* GREATER_EQUAL  */
    NOT_EQUAL = 288,               /* NOT_EQUAL  */
    AND = 289,                     /* AND  */
    OR = 290,                      /* OR  */
    ASSIGN = 291,                  /* ASSIGN  */
    COMMA = 292,                   /* COMMA  */
    SEMICOLON = 293,               /* SEMICOLON  */
    ERROR = 294                    /* ERROR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 27 "parser.y"

    char *current_symbol; //we can't use string or any other object with construct function in union. 
    int symbol_size;
    struct ast::compunit_syntax *compunit ;
    struct ast::func_def_syntax *func_def;
    struct ast::expr_syntax *expr;
    struct ast::binop_expr_syntax *binop_expr;
    struct ast::unaryop_expr_syntax *unaryop_expr;
    struct ast::lval_syntax *lval;  
    struct ast::literal_syntax *literal;
    struct ast::stmt_syntax *stmt;
    struct ast::assign_stmt_syntax *assign;
    struct ast::block_syntax *block;
    struct ast::if_stmt_syntax *if_stmt;
    struct ast::return_stmt_syntax *return_stmt;
    struct ast::var_def_stmt_syntax *var_def_stmt;
    struct ast::var_decl_stmt_syntax *var_decl_stmt;
    struct ast::func_f_param_syntax *func_f_param;
    struct ast::var_dimension_syntax *var_dimension;
    struct ast::exp_stmt_syntax *exp_stmt;
    struct ast::while_stmt_syntax *while_stmt;
    struct ast::empty_stmt_syntax *empty_stmt;
    struct ast::break_stmt_syntax *break_stmt;
    struct ast::continue_stmt_syntax *continue_stmt;
    struct ast::init_syntax *init;
    struct ast::func_call_syntax *func_call;
    enum vartype var_type;

#line 132 "parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_HPP_INCLUDED  */
