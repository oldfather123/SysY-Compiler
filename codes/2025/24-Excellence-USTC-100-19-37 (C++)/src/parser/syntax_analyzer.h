#ifdef __cplusplus
extern "C" {
#endif

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

#ifndef YY_YY_MNT_HGFS_COMPILE_MATCH_DELETE_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
# define YY_YY_MNT_HGFS_COMPILE_MATCH_DELETE_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED
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
    Error = 258,                   /* Error  */
    Add = 259,                     /* Add  */
    Sub = 260,                     /* Sub  */
    Mul = 261,                     /* Mul  */
    Div = 262,                     /* Div  */
    Mod = 263,                     /* Mod  */
    LT = 264,                      /* LT  */
    LTE = 265,                     /* LTE  */
    GT = 266,                      /* GT  */
    GTE = 267,                     /* GTE  */
    Eq = 268,                      /* Eq  */
    NEq = 269,                     /* NEq  */
    Not = 270,                     /* Not  */
    And = 271,                     /* And  */
    Or = 272,                      /* Or  */
    Assign = 273,                  /* Assign  */
    Semicolon = 274,               /* Semicolon  */
    Comma = 275,                   /* Comma  */
    LParenthese = 276,             /* LParenthese  */
    RParenthese = 277,             /* RParenthese  */
    LBracket = 278,                /* LBracket  */
    RBracket = 279,                /* RBracket  */
    LBrace = 280,                  /* LBrace  */
    RBrace = 281,                  /* RBrace  */
    Const = 282,                   /* Const  */
    Int = 283,                     /* Int  */
    Float = 284,                   /* Float  */
    If = 285,                      /* If  */
    Else = 286,                    /* Else  */
    Return = 287,                  /* Return  */
    Void = 288,                    /* Void  */
    While = 289,                   /* While  */
    Break = 290,                   /* Break  */
    Continue = 291,                /* Continue  */
    Ident = 292,                   /* Ident  */
    DecIntConst = 293,             /* DecIntConst  */
    OctIntConst = 294,             /* OctIntConst  */
    HexIntConst = 295,             /* HexIntConst  */
    DecFloatConst = 296,           /* DecFloatConst  */
    HexFloatConst = 297,           /* HexFloatConst  */
    Epsilon = 298                  /* Epsilon  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 34 "syntax_analyzer.y"

    struct _syntax_tree_node * node;
	char * name;

#line 112 "/mnt/hgfs/compile_match/delete/src/parser/syntax_analyzer.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MNT_HGFS_COMPILE_MATCH_DELETE_SRC_PARSER_SYNTAX_ANALYZER_H_INCLUDED  */
#ifdef __cplusplus
}
#endif
