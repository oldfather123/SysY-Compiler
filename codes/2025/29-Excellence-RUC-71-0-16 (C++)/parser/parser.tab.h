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

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
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
    INT_CONST = 258,               /* INT_CONST  */
    FLOAT_CONST = 259,             /* FLOAT_CONST  */
    IDENT = 260,                   /* IDENT  */
    STR_CONST = 261,               /* STR_CONST  */
    ERROR = 262,                   /* ERROR  */
    PLUS = 263,                    /* PLUS  */
    MINUS = 264,                   /* MINUS  */
    MUL = 265,                     /* MUL  */
    DIV = 266,                     /* DIV  */
    MOD = 267,                     /* MOD  */
    ASSIGN = 268,                  /* ASSIGN  */
    NOT = 269,                     /* NOT  */
    LT = 270,                      /* LT  */
    GT = 271,                      /* GT  */
    LEQ = 272,                     /* LEQ  */
    GEQ = 273,                     /* GEQ  */
    EQ = 274,                      /* EQ  */
    NE = 275,                      /* NE  */
    AND = 276,                     /* AND  */
    OR = 277,                      /* OR  */
    LPAREN = 278,                  /* LPAREN  */
    RPAREN = 279,                  /* RPAREN  */
    LBRACKET = 280,                /* LBRACKET  */
    RBRACKET = 281,                /* RBRACKET  */
    LBRACE = 282,                  /* LBRACE  */
    RBRACE = 283,                  /* RBRACE  */
    COMMA = 284,                   /* COMMA  */
    SEMI = 285,                    /* SEMI  */
    CONST = 286,                   /* CONST  */
    IF = 287,                      /* IF  */
    ELSE = 288,                    /* ELSE  */
    WHILE = 289,                   /* WHILE  */
    VOID = 290,                    /* VOID  */
    INT = 291,                     /* INT  */
    FLOAT = 292,                   /* FLOAT  */
    RETURN = 293,                  /* RETURN  */
    BREAK = 294,                   /* BREAK  */
    CONTINUE = 295,                /* CONTINUE  */
    UMINUS = 296,                  /* UMINUS  */
    UPLUS = 297,                   /* UPLUS  */
    UNOT = 298,                    /* UNOT  */
    LOWER_THAN_ELSE = 299          /* LOWER_THAN_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 29 "parser.y"

    char* str_val;           // 字符串值
    int int_val;             // 整数值
    float float_val;         // 浮点数值
    
    // AST节点指针
    CompUnit* comp_unit;
    Decl* decl;
    ConstDecl* const_decl;
    ConstDef* const_def;
    VarDecl* var_decl;
    VarDef* var_def;
    FuncDef* func_def;
    FuncFParam* func_param;
    Stmt* stmt;
    Block* block;
    Exp* exp;
    LVal* lval;
    InitVal* init_val;
    ConstInitVal* const_init_val;
    StringLiteral* string_literal;
    
    // 列表类型
    std::vector<std::unique_ptr<Decl>>* decl_list;
    std::vector<std::unique_ptr<FuncDef>>* func_list;
    std::vector<std::unique_ptr<ConstDef>>* const_def_list;
    std::vector<std::unique_ptr<VarDef>>* var_def_list;
    std::vector<std::unique_ptr<FuncFParam>>* param_list;
    std::vector<std::unique_ptr<Exp>>* exp_list;
    std::vector<std::unique_ptr<InitVal>>* init_list;
    std::vector<std::unique_ptr<ConstInitVal>>* const_init_list;
    std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>* block_item_list;
    std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>* comp_item_list;
    
    BaseType base_type;      // 基本类型枚举

#line 145 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
