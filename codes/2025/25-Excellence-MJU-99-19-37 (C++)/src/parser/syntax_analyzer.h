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

#ifndef YY_YY_SYNTAX_ANALYZER_H_INCLUDED
# define YY_YY_SYNTAX_ANALYZER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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
    YYUNDEF = 285,                 /* "invalid token"  */
    ERROR = 286,                   /* ERROR  */
    ADD = 287,                     /* ADD  */
    SUB = 288,                     /* SUB  */
    MUL = 289,                     /* MUL  */
    DIV = 290,                     /* DIV  */
    MOD = 291,                     /* MOD  */
    LT = 292,                      /* LT  */
    LET = 293,                     /* LET  */
    GT = 294,                      /* GT  */
    GET = 295,                     /* GET  */
    EQ = 296,                      /* EQ  */
    NEQ = 297,                     /* NEQ  */
    ASSIGN = 298,                  /* ASSIGN  */
    COMMA = 299,                   /* COMMA  */
    SEMICOLON = 270,               /* SEMICOLON  */
    NOT = 300,                     /* NOT  */
    LPARENTHESIS = 272,            /* LPARENTHESIS  */
    RPARENTHESIS = 273,            /* RPARENTHESIS  */
    LBRACKET = 274,                /* LBRACKET  */
    RBRACKET = 275,                /* RBRACKET  */
    LBRACE = 276,                  /* LBRACE  */
    RBRACE = 277,                  /* RBRACE  */
    AND = 301,                     /* AND  */
    OR = 302,                      /* OR  */
    IF = 303,                      /* IF  */
    ELSE = 304,                    /* ELSE  */
    WHILE = 305,                   /* WHILE  */
    BREAK = 306,                   /* BREAK  */
    CONTINUE = 307,                /* CONTINUE  */
    RETURN = 281,                  /* RETURN  */
    VOID = 282,                    /* VOID  */
    INT = 308,                     /* INT  */
    FLOAT = 309,                   /* FLOAT  */
    CONST = 310,                   /* CONST  */
    IDENT = 284,                   /* IDENT  */
    INTCONST = 311,                /* INTCONST  */
    FLOATCONST = 312               /* FLOATCONST  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 36 "syntax_analyzer.y"

    struct _syntax_tree_node* node;

#line 109 "syntax_analyzer.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);

/* Additional declarations for the parse function */
struct _syntax_tree;
extern struct _syntax_tree *gt;
syntax_tree *parse(const char *input_path);

#ifdef __cplusplus
}
#endif

#endif /* !YY_YY_SYNTAX_ANALYZER_H_INCLUDED  */