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

#ifndef YY_YY_HOME_MANBO_MANBO_MANBO_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
# define YY_YY_HOME_MANBO_MANBO_MANBO_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
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
    ADD = 259,                     /* ADD  */
    SUB = 260,                     /* SUB  */
    MUL = 261,                     /* MUL  */
    DIV = 262,                     /* DIV  */
    MOD = 263,                     /* MOD  */
    ADASS = 264,                   /* ADASS  */
    SUASS = 265,                   /* SUASS  */
    MUASS = 266,                   /* MUASS  */
    DIASS = 267,                   /* DIASS  */
    LT = 268,                      /* LT  */
    LET = 269,                     /* LET  */
    GT = 270,                      /* GT  */
    GET = 271,                     /* GET  */
    EQ = 272,                      /* EQ  */
    NEQ = 273,                     /* NEQ  */
    LPARENTHESIS = 274,            /* LPARENTHESIS  */
    RPARENTHESIS = 275,            /* RPARENTHESIS  */
    LBRACKET = 276,                /* LBRACKET  */
    RBRACKET = 277,                /* RBRACKET  */
    LBRACE = 278,                  /* LBRACE  */
    RBRACE = 279,                  /* RBRACE  */
    ASSIGN = 280,                  /* ASSIGN  */
    COMMA = 281,                   /* COMMA  */
    SEMICOLON = 282,               /* SEMICOLON  */
    NOT = 283,                     /* NOT  */
    AND = 284,                     /* AND  */
    OR = 285,                      /* OR  */
    INT = 286,                     /* INT  */
    FLOAT = 287,                   /* FLOAT  */
    VOID = 288,                    /* VOID  */
    CONST = 289,                   /* CONST  */
    MAIN = 290,                    /* MAIN  */
    IF = 291,                      /* IF  */
    ELSE = 292,                    /* ELSE  */
    WHILE = 293,                   /* WHILE  */
    BREAK = 294,                   /* BREAK  */
    CONTINUE = 295,                /* CONTINUE  */
    RETURN = 296,                  /* RETURN  */
    FOR = 297,                     /* FOR  */
    IDENT = 298,                   /* IDENT  */
    INTCONST = 299,                /* INTCONST  */
    FLOATCONST = 300,              /* FLOATCONST  */
    LOWER_THAN_ASIGN = 301         /* LOWER_THAN_ASIGN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 33 "syntax_analyzer.y"

    struct _syntax_tree_node* node;

#line 114 "/home/manbo/manbo/manbo/src/parser/syntax_analyzer.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_HOME_MANBO_MANBO_MANBO_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED  */
