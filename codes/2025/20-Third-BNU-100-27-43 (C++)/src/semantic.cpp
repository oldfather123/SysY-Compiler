// semantic.c
#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// static Scope *current_scope = NULL;
static int has_main_function = 0;
static int loop_nesting_depth = 0; // 在文件顶部定义
int need_new_scope = 1;

/* 语义错误报告 */
void semantic_error(int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "语义错误 [行%d]: ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/* 常量表达式求值 */
// 注意：这个函数现在可以处理在符号表中查找常量值
double eval_const_expr(ASTNode *node)
{
    if (!node)
        return 0.0;

    switch (node->type)
    {
    case NODE_INIT_VAL:
        return eval_const_expr(node->init_val.expr);
    case NODE_CONST_INIT:
        return eval_const_expr(node->const_init.expr);

    case NODE_LVAL:
    {
        Symbol *sym = find_symbol(node->lval.name);
        if (sym && sym->sym_type == SYM_CONST)
        {
            // 从符号表返回预先计算好的值
            if (sym->data_type == TYPE_FLOAT)
                return sym->const_float_val;
            else
                return (double)sym->const_val;
        }
        semantic_error(node->line_no, "在常量表达式中使用了非常量标识符 '%s'", node->lval.name);
        return 0.0;
    }
    case NODE_CONST_EXPR:
        return (node->data_type == TYPE_INT) ? (double)node->const_expr.int_val : node->const_expr.float_val;

    case NODE_BIN_OP:
    {
        double left = eval_const_expr(node->bin_op.left);
        double right = eval_const_expr(node->bin_op.right);
        switch (node->bin_op.op)
        {
        case OP_ADD:
            return left + right;
        case OP_SUB:
            return left - right;
        case OP_MUL:
            return left * right;
        case OP_DIV:
            if (right == 0.0)
            {
                semantic_error(node->line_no, "常量表达式中出现除以0");
                return 0.0;
            }
            return left / right;
        case OP_MOD: // 注意: C语言的%操作符不直接用于浮点数
            return (double)((long)left % (long)right);
        default:
            semantic_error(node->line_no, "常量表达式中包含不支持的运算符");
            return 0.0;
        }
    }
    case NODE_UNARY_OP:
    {
        double val = eval_const_expr(node->unary_op.expr);
        switch (node->unary_op.op)
        {
        case OP_NEG:
            return -val;
        case OP_POS:
            return val;
        default:
            semantic_error(node->line_no, "常量表达式中包含不支持的一元运算符");
            return 0.0;
        }
    }
    // 处理在语义分析中插入的类型转换节点
    case NODE_TYPE_CAST:
    {
        double val = eval_const_expr(node->unary_op.expr);
        if (node->data_type == TYPE_INT)
            return (double)((int)val); // 模拟向int的转换
        if (node->data_type == TYPE_FLOAT)
            return val; // 已经是double，无需操作
    }

    default:
        semantic_error(node->line_no, "无效的常量表达式节点类型 %d", node->type);
        return 0.0;
    }
}

/* 类型兼容性检查,修改支持float到int的隐式转换 */
static int type_compatible(DataType expected, DataType actual, int line)
{
    if (expected == actual)
        return 1;
    if (expected == TYPE_FLOAT && actual == TYPE_INT)
        return 1;
    if (expected == TYPE_INT && actual == TYPE_FLOAT)
        return 1;

    semantic_error(line, "类型不兼容: 预期 %s, 实际 %s",
                   (expected == TYPE_INT) ? "int" : "float",
                   (actual == TYPE_INT) ? "int" : "float");
    return 0;
}

/* 初始化全局作用域 */
static void init_global_scope(void)
{
    current_scope = (Scope *)malloc(sizeof(Scope));
    current_scope->name = "global";
    // memcpy(current_scope->name, "global", strlen("global"));
    current_scope->symbols = NULL;
    current_scope->child = current_scope->next = current_scope->parent = NULL;
    current_scope->scope_level = current_scope->current_offset = 0;
}

/* 判断是否为 sylib 库函数 */
bool is_sylib_func(const char *name)
{
    for (int i = 0; sylib_funcs[i]; ++i)
    {
        if (strcmp(name, sylib_funcs[i]) == 0)
            return true;
    }
    return false;
}

/* 递归语义检查 */
static void check_node(ASTNode *node); // 前向声明

static void check_children(ASTNode *node)
{
    if (!node)
        return;
    ASTNode *child = NULL;
    switch (node->type)
    {
    case NODE_COMP_UNIT:
        child = node->comp.children;
        break;
    case NODE_BLOCK:
        child = node->block.items;
        break;
    // ... 其他需要遍历子节点链表的节点
    default:
        break;
    }
    while (child)
    {
        check_node(child);
        child = child->next;
    }
}

static void check_children_in_list(ASTNode *list_head)
{
    ASTNode *item = list_head;
    while (item)
    {
        check_node(item);
        item = item->next;
    }
}

/* 递归语义检查 */
static void check_node(ASTNode *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case NODE_COMP_UNIT:
        check_children_in_list(node->comp.children);
        break;

    case NODE_FUNC_DEF:
    {
        Symbol *func_sym = add_symbol(node->func_def.name, SYM_FUNC, node->func_def.ret_type);
        if (!func_sym)
        {
            break;
        }

        Symbol *param_sym_list_head = NULL;
        Symbol **param_sym_list_tail_ptr = &param_sym_list_head;
        ASTNode *ast_param = node->func_def.params;

        while (ast_param)
        {
            // 为每个AST参数节点创建一个新的符号
            Symbol *new_param_sym = (Symbol *)malloc(sizeof(Symbol));
            if (!new_param_sym)
            { /* handle malloc failure */
                exit(1);
            }

            new_param_sym->name = strdup(ast_param->decl.name);
            new_param_sym->sym_type = SYM_VAR; // 参数在函数内表现为变量
            new_param_sym->data_type = ast_param->data_type;
            // 其他字段可以根据需要初始化
            new_param_sym->scope_level = current_scope->scope_level + 1; // 参数在函数作用域内
            new_param_sym->next = NULL;
            // (我们不需要填充数组维度等信息，因为这里只关心类型检查)

            // 将新创建的符号附加到链表尾部
            *param_sym_list_tail_ptr = new_param_sym;
            param_sym_list_tail_ptr = &new_param_sym->next;

            ast_param = ast_param->next;
        }

        // 将构建好的符号链表附加到函数符号上
        func_sym->func_params = param_sym_list_head;

        if (strcmp(node->func_def.name, "main") == 0)
        {
            has_main_function = 1;
            if (node->func_def.ret_type != TYPE_INT)
            {
                semantic_error(node->line_no, "main函数必须返回int类型");
            }
        }

        enter_scope(node->func_def.name, 0);

        if (node->func_def.params)
        {
            check_children_in_list(node->func_def.params);
        }

        if (node->func_def.body)
        {
            need_new_scope = 0;
            check_node(node->func_def.body);
            need_new_scope = 1;
        }

        exit_scope();
        break;
    }

    case NODE_BLOCK:
    {
        int new_scope_created = 0;
        // 如需要，则为语句块创建新的作用域
        if (need_new_scope)
            node->block.scope = enter_scope("block", 1), new_scope_created = 1;

        // 检查语句块中的所有项目
        ASTNode *child = node->block.items;
        while (child)
        {
            // printf("child type : %d\n", (int)child->type);
            need_new_scope = 1;
            check_node(child);
            child = child->next;
        }

        if (new_scope_created)
            exit_scope();
        break;
    }

    // This case handles local variables, global variables, AND parameters
    case NODE_VAR_DECL:
    case NODE_ARRAY_DECL:
    {
        Symbol *sym = add_symbol(node->decl.name, SYM_VAR, node->data_type);
        if (!sym)
            break; // Error already reported by add_symbol

        if (node->decl.dims)
        {
            check_children_in_list(node->decl.dims);
        }
        if (node->decl.init)
        {
            check_node(node->decl.init);
            // 添加类型兼容性检查和转换
            if (type_compatible(node->data_type, node->decl.init->data_type, node->line_no))
            {
                // 如果初始值类型与变量类型不同，需要插入类型转换节点
                if (node->data_type != node->decl.init->data_type)
                {
                    if (node->data_type == TYPE_INT &&
                        node->decl.init->data_type == TYPE_FLOAT)
                    {
                        // float -> int 转换
                        node->decl.init = new_type_cast(node->decl.init,
                                                        TYPE_INT,
                                                        node->line_no);
                    }
                    else if (node->data_type == TYPE_FLOAT &&
                             node->decl.init->data_type == TYPE_INT)
                    {
                        // int -> float 转换
                        node->decl.init = new_type_cast(node->decl.init,
                                                        TYPE_FLOAT,
                                                        node->line_no);
                    }
                }
            }
            else
            {
                semantic_error(node->line_no,
                               "变量初始化类型不匹配: 变量类型 %s, 初始值类型 %s",
                               (node->data_type == TYPE_INT) ? "int" : "float",
                               (node->decl.init->data_type == TYPE_INT) ? "int" : "float");
            }
        }
        break;
    }

    case NODE_CONST_DECL:
    case NODE_CONST_ARRAY_DECL:
    {
        Symbol *sym = add_symbol(node->decl.name, SYM_CONST, node->data_type);
        if (!sym)
            break;

        if (!node->decl.init)
        {
            semantic_error(node->line_no, "常量 '%s' 必须被初始化", node->decl.name);
            break;
        }
        check_node(node->decl.init);
        // 添加类型兼容性检查和转换
        if (type_compatible(node->data_type, node->decl.init->data_type, node->line_no))
        {
            if (node->data_type != node->decl.init->data_type)
            {
                if (node->data_type == TYPE_INT &&
                    node->decl.init->data_type == TYPE_FLOAT)
                {
                    node->decl.init = new_type_cast(node->decl.init,
                                                    TYPE_INT,
                                                    node->line_no);
                }
                else if (node->data_type == TYPE_FLOAT &&
                         node->decl.init->data_type == TYPE_INT)
                {
                    node->decl.init = new_type_cast(node->decl.init,
                                                    TYPE_FLOAT,
                                                    node->line_no);
                }
            }
        }
        else
        {
            semantic_error(node->line_no,
                           "变量初始化类型不匹配: 变量类型 %s, 初始值类型 %s",
                           (node->data_type == TYPE_INT) ? "int" : "float",
                           (node->decl.init->data_type == TYPE_INT) ? "int" : "float");
        }

        // +++ 修复点: 使用修复后的eval_const_expr函数计算并存储常量值 +++
        if (node->type == NODE_CONST_DECL)
        { // 仅为标量常量存储值
            if (node->data_type == TYPE_INT)
            {
                sym->const_val = (int)eval_const_expr(node->decl.init);
            }
            else if (node->data_type == TYPE_FLOAT)
            {
                sym->const_float_val = eval_const_expr(node->decl.init);
            }
        }

        if (node->decl.dims)
        {
            check_children_in_list(node->decl.dims);
        }
        break;
    }

    case NODE_LVAL:
    {
        Symbol *sym = find_symbol(node->lval.name);
        if (!sym)
        {
            semantic_error(node->line_no, "未定义的标识符: %s", node->lval.name);
            node->data_type = TYPE_INT; // Avoid cascading errors
            return;
        }
        node->data_type = sym->data_type;
        if (node->lval.indices)
        {
            check_children_in_list(node->lval.indices);
        }
        break;
    }

    case NODE_ASSIGN:
    {
        check_node(node->assign.lval);
        // Check for assignment to const *after* checking the lval
        Symbol *sym = find_symbol(node->assign.lval->lval.name);
        if (sym && sym->sym_type == SYM_CONST)
        {
            semantic_error(node->line_no, "不能对常量 '%s' 赋值", sym->name);
        }
        check_node(node->assign.rval);
        // 检查类型兼容性
        if (type_compatible(sym->data_type, node->assign.rval->data_type, node->line_no))
        {
            // 如果需要float到int的转换,插入类型转换节点
            if (sym->data_type == TYPE_INT && node->assign.rval->data_type == TYPE_FLOAT)
            {
                node->assign.rval = new_type_cast(node->assign.rval, TYPE_INT, node->line_no);
            }
        }
        break;
    }

    case NODE_BIN_OP:
        check_node(node->bin_op.left);
        check_node(node->bin_op.right);
        // 位运算只能用于整型
        switch (node->bin_op.op)
        {
        case OP_BAND:
        case OP_BOR:
        case OP_BXOR:
        case OP_SHL:
        case OP_SHR:
            if (node->bin_op.left->data_type != TYPE_INT ||
                node->bin_op.right->data_type != TYPE_INT)
            {
                semantic_error(node->line_no, "位运算操作数必须是整型");
            }
            node->data_type = TYPE_INT;
            break;
        /* 其他运算符处理... */
        default:
        {
            /* 处理类型提升 */
            if (node->bin_op.left->data_type != node->bin_op.right->data_type)
            {
                if (node->bin_op.left->data_type == TYPE_FLOAT)
                {
                    node->data_type = TYPE_FLOAT;
                    if (node->bin_op.right->data_type == TYPE_INT)
                    {
                        node->bin_op.right = new_type_cast(node->bin_op.right, TYPE_FLOAT, node->line_no);
                    }
                }
                else
                {
                    node->data_type = TYPE_FLOAT;
                    node->bin_op.left = new_type_cast(node->bin_op.left, TYPE_FLOAT, node->line_no);
                }
            }
            else
            {
                node->data_type = node->bin_op.left->data_type;
            }
        }
        }
        break;

    // --- Other simple recursive cases ---
    case NODE_UNARY_OP:
        check_node(node->unary_op.expr);
        node->data_type = node->unary_op.expr->data_type;
        break;
    case NODE_FUNC_CALL:
    {
        if (is_sylib_func(node->func_call.name))
        {
            // 对于库函数，只递归检查其参数，不做复杂的类型匹配
            check_children_in_list(node->func_call.args);
            break;
        }

        Symbol *func_sym = find_symbol(node->func_call.name);
        if (!func_sym || func_sym->sym_type != SYM_FUNC)
        {
            semantic_error(node->line_no, "未定义的函数: %s", node->func_call.name);
            break;
        }
        node->data_type = func_sym->data_type;

        // +++ 最终修复点: 防御性地遍历参数链表 +++
        ASTNode *arg_node = node->func_call.args;
        Symbol *param_sym = func_sym->func_params;
        ASTNode **arg_ptr_ref = &node->func_call.args; // 用于可能的类型转换插入

        while (arg_node && param_sym)
        {
            // 关键修复: 在递归调用 check_node 之前保存 next 指针
            ASTNode *next_arg = arg_node->next;

            // 递归检查当前参数节点
            check_node(arg_node);

            // 比较实参AST节点的类型和形参Symbol的类型
            if (arg_node->data_type != param_sym->data_type)
            {
                if (type_compatible(param_sym->data_type, arg_node->data_type, arg_node->line_no))
                {
                    // 如果类型不匹配但兼容，插入类型转换节点
                    // 注意：这里需要通过指向指针的指针来修改链表
                    *arg_ptr_ref = new_type_cast(arg_node, param_sym->data_type, arg_node->line_no);
                    // 确保新节点的 next 指针正确连接
                    (*arg_ptr_ref)->next = next_arg;
                }
                else
                {
                    // 如果不兼容，则报错
                    semantic_error(arg_node->line_no, "函数 '%s' 的参数类型不匹配", func_sym->name);
                }
            }

            // 移动到下一个实参和形参
            arg_ptr_ref = &((*arg_ptr_ref)->next);
            arg_node = next_arg;
            param_sym = param_sym->next;
        }

        // 检查参数数量是否匹配
        if (arg_node || param_sym)
        {
            semantic_error(node->line_no, "函数 '%s' 的参数数量不匹配", node->func_call.name);
        }

        break;
    }

    case NODE_IF:
    {
        // 检查条件表达式
        check_node(node->if_stmt.cond);

        // 为then分支创建新的作用域
        node->if_stmt.then_scope = enter_scope("if_then", 1);
        if (node->if_stmt.then->type == NODE_BLOCK)
            need_new_scope = 0;
        check_node(node->if_stmt.then);
        need_new_scope = 1;
        exit_scope();

        // 检查else分支（如果有）
        if (node->if_stmt.els)
        {
            node->if_stmt.els_scope = enter_scope("if_else", 1);
            if (node->if_stmt.els->type == NODE_BLOCK)
                need_new_scope = 0;
            check_node(node->if_stmt.els);
            need_new_scope = 1;
            exit_scope();
        }
        break;
    }
    case NODE_WHILE:
    {
        // 检查条件表达式
        check_node(node->while_stmt.cond);

        // 进入循环上下文
        loop_nesting_depth++;

        // 为while循环体创建新的作用域
        node->while_stmt.body_scope = enter_scope("while_body", 1);
        if (node->while_stmt.body->type == NODE_BLOCK)
            need_new_scope = 0;
        check_node(node->while_stmt.body);
        need_new_scope = 1;
        exit_scope();

        loop_nesting_depth--;
        break;
    }
    case NODE_RETURN:
    {
        Symbol *current_func = find_symbol(current_scope->name);
        Scope *func_scope = current_scope;
        while (!current_func)
        {
            func_scope = func_scope->parent;
            current_func = find_symbol(func_scope->name);
        }
        // current_func->use_count--; // 减去上述查找操作引起的引用计数递增

        if (current_func == NULL)
            break;
        // printf("current func name : %s\n", current_func->name);
        if (node->return_stmt.expr)
        {
            // 有返回值的情况
            check_node(node->return_stmt.expr);

            if (current_func->data_type == TYPE_VOID)
            {
                semantic_error(node->line_no, "void函数不能返回数值");
            }
            else
            {
                // 检查类型兼容性并进行必要的类型转换
                if (type_compatible(current_func->data_type,
                                    node->return_stmt.expr->data_type,
                                    node->line_no))
                {
                    // 如果类型不同但兼容，插入类型转换节点
                    if (current_func->data_type != node->return_stmt.expr->data_type)
                    {
                        if (current_func->data_type == TYPE_INT &&
                            node->return_stmt.expr->data_type == TYPE_FLOAT)
                        {
                            // float -> int 转换
                            // printf("警告 [行%d]: 函数返回值从float隐式转换为int可能损失精度\n",
                            //   node->line_no);
                            node->return_stmt.expr = new_type_cast(node->return_stmt.expr,
                                                                   TYPE_INT,
                                                                   node->line_no);
                        }
                        else if (current_func->data_type == TYPE_FLOAT &&
                                 node->return_stmt.expr->data_type == TYPE_INT)
                        {
                            // int -> float 转换
                            node->return_stmt.expr = new_type_cast(node->return_stmt.expr,
                                                                   TYPE_FLOAT,
                                                                   node->line_no);
                        }
                    }
                }
            }
        }
        else
        {
            // 无返回值的情况
            if (current_func->data_type != TYPE_VOID)
            {
                semantic_error(node->line_no, "非void函数必须返回数值");
            }
        }
        break;
    }
    case NODE_EXPR_STMT:
        check_node(node->return_stmt.expr);
        break;
    case NODE_INIT_LIST:
        check_children_in_list(node->init_list.first);
        break;
    case NODE_CONST_INIT_LIST:
        check_children_in_list(node->const_init_list.first);
        break;
    case NODE_INIT_VAL:
        check_node(node->init_val.expr);
        break;
    case NODE_CONST_INIT:
        check_node(node->const_init.expr);
        break;
    case NODE_ARRAY_INDEX:
        check_node(node->array_index.expr);
        break;
    case NODE_ARRAY_DIMS:
        check_node(node->array_dims.size);
        break;
    case NODE_TYPE_CAST:
    {
        check_node(node->unary_op.expr);
        node->data_type = node->data_type; // 已在构造函数中设置目标类型
        break;
    }
    default:
        // No action needed for leaf nodes like BREAK, CONTINUE, CONST_EXPR
        break;
    }
}

/* 主检查入口 */
void check_semantics(ASTNode *root)
{
    init_global_scope();
    if (root)
    {
        check_node(root);
    }

    Symbol *main_sym = find_symbol("main");
    if (!main_sym || main_sym->sym_type != SYM_FUNC)
    {
        semantic_error(0, "程序缺少main函数");
    }
}