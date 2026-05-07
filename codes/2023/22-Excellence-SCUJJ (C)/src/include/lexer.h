#ifndef _LEXER_H
#define _LEXER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "symbol.h"
#include "uthash.h"
#include "vector.h"

typedef enum Token {
    TOK_CONST,
    TOK_INT,
    TOK_FLOAT,
    TOK_IF,
    TOK_WHILE,
    TOK_ELSE,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_RETURN,
    TOK_VOID,
    TOK_IDENT,    // 标识符
    TOK_IMM_INT,  // 常量
    TOK_IMM_FLOAT,
    TOK_STRING,
    // +       -       *       /       %
    TOK_Add,
    TOK_Sub,
    TOK_Mul,
    TOK_Div,
    TOK_Mod,
    // &       ~       |       <<      >>      //
    TOK_BitAnd,
    TOK_BitXor,
    TOK_BitOr,
    TOK_ShiftL,
    TOK_ShiftR,
    TOK_Idiv,
    // ==       !=     <=      >=      <       >        =
    TOK_Equal,
    TOK_NotEq,
    TOK_LesEq,
    TOK_GreEq,
    TOK_Less,
    TOK_Greater,
    TOK_Assign,
    // (       )       {       }       [       ]       ::
    TOK_ParL,
    TOK_ParR,
    TOK_CurlyL,
    TOK_CurlyR,
    TOK_SqurL,
    TOK_SqurR,
    // ;     ,       .
    TOK_SemiColon,
    TOK_Comma,
    TOK_Dot,
    //
    TOK_NOT,      //!
    TOK_AND,      //&&
    TOK_OR,       //||
    TOK_ASS_ADD,  //
    TOK_EOF
} Token;

/**
 * @brief 获取下一个token
 */
Token next();
/**
 * @brief 匹配参数t是否匹配当前token
 * @param[in] t
 */
bool  match(Token t);

/**
 * @brief 与预期token不至于则发出错误
 * @param[in] t
 * @param[in] *msg
 */
void  consume(Token t, const char *msg);

typedef struct tokenIdent {
    uint32_t       tok;
    char          *str;
    Symbol        *sym;
    UT_hash_handle hh;
} tokenIdent;

tokenIdent                 *getToken(uint32_t tok);

tokenIdent                 *defineToken(char *str, size_t len);

/**
 * @brief 标识符的hash表
 */
extern tokenIdent          *tokenMap;

/**
 * @brief 记录当前的标识符
 */
extern uint32_t             curId;

/**
 * @brief 记录当前的token
 */
extern Token                curTok;

/**
 * @brief 字符串
 */
extern  char *CurString;

/**
 * @brief 整数、浮点数字面量
 */
extern size_t               immInt;
extern float                immFloat;

extern int                  yylineno;

extern FILE                *yyin;
extern int                  yylineno;

#endif