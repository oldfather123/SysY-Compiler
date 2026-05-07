#include "symbol.h"
#include <assert.h>

// #define NULL ((void*)0)  // 显式定义NULL指针

Scope *current_scope;          // 全局作用域
static int current_offset = 0; // 全局变量跟踪偏移

/* 初始化全局作用域 */
// void init_global_scope() {
//     current_scope = (Scope*)malloc(sizeof(Scope));
//     current_scope->name = "global";
//     current_scope->symbols = NULL;
//     current_scope->child = current_scope->next = current_scope->parent = NULL;
//     current_scope->scope_level = 0;
// }
/* 在当前作用域子作用域中查找特定作用域 */
Scope *find_scope(char *name)
{
    Scope *child = current_scope->child;
    while (child)
    {
        if (!strcmp(child->name, name))
            return child;
        child = child->next;
    }
    return child;
}

/* 进入新作用域 */
Scope *enter_scope(char *name, int need_unique)
{
    // 首先在当前作用域子作用域中查找目标作用域，找到则直接返回，否则创建新作用域
    Scope *child = find_scope(name);
    if (child)
    {
        current_scope = child;
        return child;
    }
    char unique_name[256];
    if (need_unique)
    {
        // 为if while 以及单独的block生成唯一的作用域名称（添加计数器）
        static int scope_counter = 0;
        snprintf(unique_name, sizeof(unique_name), "%s_%d", name, scope_counter++);
    }

    // 创建新的作用域
    Scope *new_scope = (Scope *)malloc(sizeof(Scope));
    assert(new_scope != NULL);

    new_scope->name = need_unique ? strdup(unique_name) : strdup(name);
    new_scope->symbols = NULL;
    new_scope->parent = current_scope;
    new_scope->child = NULL;
    new_scope->next = NULL;

    // 设置作用域层级：递增父层级
    new_scope->scope_level = current_scope->scope_level + 1;
    new_scope->current_offset = -4;

    // 插入当前作用域的子作用域链表
    if (!current_scope->child)
    {
        current_scope->child = new_scope;
    }
    else
    {
        Scope *child = current_scope->child;
        while (child->next)
            child = child->next;
        child->next = new_scope;
    }

    current_scope = new_scope;
    return new_scope;
}

/* 退出当前作用域 */
void exit_scope(void)
{
    assert(current_scope != NULL);

    // 释放当前作用域的所有符号
    // Symbol *sym = current_scope->symbols;
    // while (sym != NULL) {
    //     Symbol *next = sym->next;
    //     free(sym->name);
    //     if (sym->array_dims) free(sym->array_dims);
    //     if (sym->current_indices) free(sym->current_indices);
    //     free(sym);
    //     sym = next;
    // }

    // 释放作用域名称
    // free(current_scope->name);

    Scope *parent = current_scope->parent;
    // free(current_scope);
    current_scope = parent;
}

/* 查找符号（从当前作用域向外查找） */
Symbol *find_symbol(const char *name)
{
    Scope *scope = current_scope;

    // 从当前作用域开始，逐层向父作用域查找
    while (scope != NULL)
    {
        Symbol *sym = scope->symbols;
        while (sym != NULL)
        {
            if (strcmp(sym->name, name) == 0)
            {
                // 找到了第一个匹配的符号（即最内层的符号），立即返回
                sym->use_count++; // 引用计数增加
                return sym;
            }
            sym = sym->next;
        }
        scope = scope->parent; // 移动到父作用域继续查找
    }

    // 在所有作用域中都未找到该符号
    return NULL;
}

/* 添加新符号到当前作用域 */
Symbol *add_symbol(char *name, SymType sym_type, DataType data_type)
{
    assert(current_scope != NULL);
    // printf("adding symbol %s in scope %s \n", name, current_scope->name);
    Symbol *sym = find_symbol(name);
    if (sym && sym->scope_level == current_scope->scope_level)
    {
        fprintf(stderr, "错误：标识符 '%s' 重复定义\n", name);
        return NULL;
    }

    Symbol *new_sym = (Symbol *)malloc(sizeof(Symbol));
    assert(new_sym != NULL);

    new_sym->name = strdup(name);
    new_sym->sym_type = sym_type;
    new_sym->data_type = data_type;
    new_sym->scope_level = current_scope->scope_level; // 直接使用当前作用域的层级
    new_sym->use_count = 0;                            // 初始化引用计数为0
    new_sym->next = NULL;

    // new_sym->next = current_scope->symbols;
    // current_scope->symbols = new_sym;
    sym = current_scope->symbols;
    if (!sym)
        current_scope->symbols = new_sym;
    else
    {
        while (sym->next)
            sym = sym->next;
        sym->next = new_sym;
    }
    new_sym->offset = current_scope->current_offset;
    current_scope->current_offset -= 4; // 每个变量占4字节

    return new_sym;
}

/* 获取当前作用域名称 */
char *get_current_scope_name(void)
{
    char *unknown_name = "unknown";
    return current_scope ? current_scope->name : unknown_name;
}