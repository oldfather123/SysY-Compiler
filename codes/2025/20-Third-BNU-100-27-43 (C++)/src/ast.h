#ifndef AST_H
#define AST_H

#include "symbol.h"

typedef enum {
    /* 声明相关节点 */
    NODE_COMP_UNIT,    // 编译单元
    NODE_FUNC_DEF,     // 函数定义
    NODE_VAR_DECL,     // 变量声明
    NODE_CONST_DECL,   // 常量声明
    NODE_ARRAY_DECL,   // 数组声明4
    NODE_CONST_ARRAY_DECL, // 常量数组声明

    /* 表达式节点 */
    NODE_CONST_EXPR,   // 常量表达式 
    NODE_LVAL,         // 左值7
    NODE_BIN_OP,       // 二元运算8
    NODE_UNARY_OP,     // 一元运算
    NODE_FUNC_CALL,    // 函数调用
    NODE_TYPE_CAST,    // 类型转换
    NODE_ARRAY_INDEX,  // 数组下标节点12

    /* 语句节点 */
    NODE_ASSIGN,       // 赋值
    NODE_BLOCK,        // 语句块
    NODE_IF,           // if语句
    NODE_WHILE,        // while语句
    NODE_BREAK,        // break
    NODE_CONTINUE,     // continue
    NODE_RETURN,       // return
    NODE_EXPR_STMT,    // 表达式语句20

    /* 类型节点 */
    NODE_TYPE,         // 类型节点 21
    NODE_ARRAY_DIMS,   // 数组维度
    NODE_INIT_VAL,     // 初始化值
    NODE_INIT_LIST,    // 初始化列表
    NODE_CONST_INIT,   // 常量初始化
    NODE_CONST_INIT_LIST // 常量初始化列表
} NodeType;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,
    OP_AND, OP_OR, OP_NOT, OP_POS, OP_NEG,
    OP_BAND,  // 按位与
    OP_BOR,   // 按位或
    OP_BXOR,  // 按位异或
    OP_SHL,   // 左移
    OP_SHR,   // 右移
    OP_BNOT   // 按位取反
} Operator;

typedef struct ASTNode {
    NodeType type;
    DataType data_type;
    int line_no;

    struct ASTNode *next;  // 确保有next字段用于链接
    union {
        /* 声明 */
        struct {
            struct ASTNode *children;
        } comp;
        struct {
            char *name;
            struct ASTNode *decl;
            struct ASTNode *init;
            struct ASTNode *dims;
        } decl;
        struct {
            DataType ret_type;
            char *name;
            struct ASTNode *params;
            struct ASTNode *body;
        } func_def;
        
        /* 表达式 */
        struct {
            Operator op;
            struct ASTNode *left;
            struct ASTNode *right;
        } bin_op;
        struct {
            Operator op;
            struct ASTNode *expr;
        } unary_op;
        struct {
            char *name;
            struct ASTNode *indices;
        } lval;
        struct {
            int int_val;
            double float_val;
        } const_expr;
        struct {
            char *name;
            struct ASTNode *args;
        } func_call;
        struct {
            struct ASTNode* expr;  // 下标表达式
        } array_index;
        
        /* 语句 */
        struct {
            struct ASTNode *lval;
            struct ASTNode *rval;
        } assign;
        struct {
            struct ASTNode *cond;
            struct ASTNode *then;
            struct ASTNode *els;
            Scope *then_scope, *els_scope;
        } if_stmt;
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
            Scope *body_scope;
        } while_stmt;
        struct {
            struct ASTNode *items;
            Scope *scope;
        } block;
        struct {
            struct ASTNode *expr;
        } return_stmt;

        /* 类型节点 */
        struct {
            DataType basic_type;
        } type_node;

        /* 数组维度 */
        struct {
            struct ASTNode *size;    // 当前维度大小
            struct ASTNode *next_dim;// 下一个维度
        } array_dims;

        /* 初始化值 */
        struct {
            struct ASTNode *expr;    // 单个表达式或列表第一个元素
            struct ASTNode *next;    // 列表的下一个元素
        } init_val;

        struct {
            struct ASTNode* first; // 列表的第一个元素
        } init_list;

        struct {
            struct ASTNode* expr;  // 常量表达式
            struct ASTNode* next; // 列表的下一个元素
        } const_init;

        struct {
            struct ASTNode* first; // 常量初始化列表的第一个元素
        } const_init_list;
    };
} ASTNode;

/* 构造函数声明 */
ASTNode* new_comp_unit(ASTNode *decl, ASTNode *func);
ASTNode* new_func_def(DataType ret_type, char *name, ASTNode *params, ASTNode *body, int line_no);
ASTNode* new_var_decl(char *name, DataType dtype, ASTNode *dims, ASTNode *init, int line_no);
ASTNode* new_const_decl(char *name, DataType dtype, ASTNode *dims, ASTNode *init, int line_no);
// #ifdef __cplusplus
// extern "C" {
// #endif

// AST节点构造函数声明
ASTNode *new_const_expr(int value, int line_no);
// ...其他函数声明...

// #ifdef __cplusplus
// }
// #endif
// ASTNode* new_const_expr(int int_val, int line_no);
ASTNode* new_float_const_expr(double float_val, int line_no);
ASTNode* new_lval(char *name, ASTNode *indices, int line_no);
ASTNode* new_bin_op(Operator op, ASTNode *left, ASTNode *right, int line_no);
ASTNode* new_unary_op(Operator op, ASTNode *expr, int line_no);
ASTNode* new_func_call(char *name, ASTNode *args, int line_no);
ASTNode* new_type_cast(ASTNode *expr, DataType target_type, int line_no);
ASTNode* new_assign(ASTNode *lval, ASTNode *rval, int line_no);
ASTNode* new_block(ASTNode *items, int line_no);
ASTNode* new_if(ASTNode *cond, ASTNode *then, ASTNode *els, int line_no);
ASTNode* new_while(ASTNode *cond, ASTNode *body, int line_no);
ASTNode* new_return(ASTNode *expr, int line_no);
ASTNode* new_break(int line_no);
ASTNode* new_continue(int line_no);
ASTNode* new_expr_stmt(ASTNode *expr, int line_no);
ASTNode* new_type_node(DataType type);
ASTNode* new_array_dims(ASTNode *size, ASTNode *next_dim);
// ASTNode* new_init_val(ASTNode *expr, ASTNode *next);
ASTNode* new_decl(NodeType type, char *name, ASTNode *dims, ASTNode *init, int line_no);

/* 添加新的函数声明 */
ASTNode* new_init_val(ASTNode* expr, ASTNode* next, int line_no);
ASTNode* new_init_list(ASTNode* init, int line_no);
ASTNode* new_const_init(ASTNode* expr, int line_no);
ASTNode* new_const_init_list(ASTNode* init, int line_no);
// ASTNode* list_append(ASTNode* list, ASTNode* item);
ASTNode* new_array_index(ASTNode* expr, int line_no);
ASTNode* new_lval(char* name, ASTNode* indices, int line_no);

/* 实用函数 */
void free_ast(ASTNode *node);
ASTNode* list_append(ASTNode *list, ASTNode *node);

/* 打印AST结构 */
void print_ast(ASTNode *node, int depth);

// extern int count;

#endif // AST_H