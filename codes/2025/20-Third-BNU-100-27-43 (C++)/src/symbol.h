#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 基础类型定义 */
typedef enum {
    TYPE_INT,     // 整型
    TYPE_FLOAT,   // 浮点型
    TYPE_VOID,    // void类型
    TYPE_ARRAY    // 数组类型（需结合data_type使用）
} DataType;

/* 符号类型 */
typedef enum {
    SYM_VAR,      // 变量
    SYM_CONST,    // 常量
    SYM_FUNC      // 函数
} SymType;

/* 符号表条目 */
typedef struct Symbol {
    char *name;          // 标识符名称
    SymType sym_type;    // 符号类型（变量/常量/函数）
    DataType data_type;  // 数据类型（int/float/void）
    int scope_level;     // 作用域层级
    int use_count;       // 引用计数，用于符号定值-引用层面的死代码删除
    
    /* 常量专用字段 */
    int const_val;
    float const_float_val;

    /* 数组专用字段 */
    int* array_dims;     // 数组维度信息（动态数组）
    int dim_count;       // 数组维度数
    int first_dim_unknown; // 第一维大小是否已知
    int* current_indices;     // 当前正在处理的下标位置
    int current_dim;          // 当前维度
    int is_param;
    
    /* 函数专用字段 */
    struct Symbol *next; // 链表指针
    struct Symbol* func_params; // 函数参数链表
    struct Symbol* next_param; // 函数参数链表指针
    int param_count;            // 参数数量
    
    int offset; // 变量在栈中的偏移量
} Symbol;

/* 作用域结构 */
typedef struct Scope {
    Symbol *symbols;     // 当前作用域的符号表
    char *name;          // 当前作用域名称，通常为函数名
    struct Scope *parent;// 父作用域指针
    struct Scope *child; // 子作用域指针
    struct Scope *next;  // 兄弟作用域指针
    int scope_level;     // 作用域层级
    int current_offset;  // 作用域中符号的栈帧偏移量
} Scope;

/* 全局当前作用域指针 */
extern Scope *current_scope;

/* 函数声明 */
Scope *enter_scope(char *name, int need_unique);
void exit_scope(void);
Scope *find_scope(char *name);
Symbol* find_symbol(const char *name);
Symbol* add_symbol(char* name, SymType type, DataType datatype);
char* get_current_scope_name(void);

#endif // SYMBOL_H