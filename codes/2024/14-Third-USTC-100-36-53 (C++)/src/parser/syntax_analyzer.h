/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Skeleton interface for Bison GLR parsers in C

   Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

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

#ifndef YY_YY_HOME_LVSH_PROGRAMMING_DEV_SEGFAULT_COMPILER_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
# define YY_YY_HOME_LVSH_PROGRAMMING_DEV_SEGFAULT_COMPILER_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
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
    ERROR = 258,                   /* ERROR  */
    LP = 259,                      /* LP  */
    RP = 260,                      /* RP  */
    LBRACK = 261,                  /* LBRACK  */
    RBRACK = 262,                  /* RBRACK  */
    MUL = 263,                     /* MUL  */
    DIV = 264,                     /* DIV  */
    MOD = 265,                     /* MOD  */
    ADD = 266,                     /* ADD  */
    SUB = 267,                     /* SUB  */
    AND = 268,                     /* AND  */
    OR = 269,                      /* OR  */
    NOT = 270,                     /* NOT  */
    LT = 271,                      /* LT  */
    GT = 272,                      /* GT  */
    LE = 273,                      /* LE  */
    GE = 274,                      /* GE  */
    EQ = 275,                      /* EQ  */
    NE = 276,                      /* NE  */
    ASSIGN = 277,                  /* ASSIGN  */
    COMMA = 278,                   /* COMMA  */
    LBRACE = 279,                  /* LBRACE  */
    RBRACE = 280,                  /* RBRACE  */
    SEMI = 281,                    /* SEMI  */
    ELSE = 282,                    /* ELSE  */
    IF = 283,                      /* IF  */
    INT = 284,                     /* INT  */
    RETURN = 285,                  /* RETURN  */
    VOID = 286,                    /* VOID  */
    WHILE = 287,                   /* WHILE  */
    FLOAT = 288,                   /* FLOAT  */
    CONST = 289,                   /* CONST  */
    BREAK = 290,                   /* BREAK  */
    CONTINUE = 291,                /* CONTINUE  */
    IDENT = 292,                   /* IDENT  */
    INTCONST = 293,                /* INTCONST  */
    FLOATCONST = 294,              /* FLOATCONST  */
    LOWER_THAN_ELSE = 295          /* LOWER_THAN_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 35 "syntax_analyzer.y"

    struct _syntax_tree_node *node;

#line 103 "/home/lvsh/Programming/dev/segfault-compiler/src/parser/syntax_analyzer.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_HOME_LVSH_PROGRAMMING_DEV_SEGFAULT_COMPILER_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED  */
