// semantic.h
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <stdbool.h>
// #include "symbol.h"

/* 整型常量或变量符号信息结构体 */
typedef struct IntIdentInfo
{
    char *name;
    int val;
    struct IntIdentInfo *next;
} IntIdentInfo;

extern IntIdentInfo *intIdentList;

void append_int_ident(IntIdentInfo *info);
IntIdentInfo *find_int_ident(char *name);

void semantic_error(int line, const char *fmt, ...);
double eval_const_expr(ASTNode *node);
void check_semantics(ASTNode *root);

// sylib 库函数名列表
static const char *sylib_funcs[] = {
    "getint", "getch", "getarray", "getfloat", "getfarray",
    "putint", "putch", "putarray", "putfloat", "putfarray", "putf",
    "starttime", "stoptime",
    NULL};

// 判断是否为 sylib 库函数
bool is_sylib_func(const char *name);

#endif