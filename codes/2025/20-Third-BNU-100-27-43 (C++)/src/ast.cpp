#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 创建编译单元节点 */
ASTNode *new_comp_unit(ASTNode *decl, ASTNode *func)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_COMP_UNIT;
    node->comp.children = decl ? decl : func;
    node->next = NULL;
    return node;
}

/* 创建函数定义节点 */
ASTNode *new_func_def(DataType ret_type, char *name, ASTNode *params, ASTNode *body, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_FUNC_DEF;
    node->data_type = ret_type;
    node->line_no = line_no;
    node->func_def.name = strdup(name);
    node->func_def.params = params;
    node->func_def.body = body;
    node->func_def.ret_type = ret_type;
    return node;
}

/* 创建变量声明节点 */
ASTNode *new_var_decl(char *name, DataType dtype, ASTNode *dims, ASTNode *init, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = dims ? NODE_ARRAY_DECL : NODE_VAR_DECL;
    node->data_type = dtype;
    node->line_no = line_no;
    node->decl.name = strdup(name);
    node->decl.dims = dims;
    node->decl.init = init;
    node->next = NULL;
    return node;
}

/* 创建常量声明节点 */
ASTNode *new_const_decl(char *name, DataType dtype, ASTNode *dims, ASTNode *init, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONST_DECL;
    node->data_type = dtype;
    node->line_no = line_no;
    node->decl.name = strdup(name);
    node->decl.dims = dims;
    node->decl.init = init;
    return node;
}

/* 创建整型常量节点 */
ASTNode *new_const_expr(int int_val, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONST_EXPR;
    node->data_type = TYPE_INT;
    node->line_no = line_no;
    node->const_expr.int_val = int_val;
    return node;
}

/* 创建浮点常量节点 */
ASTNode *new_float_const_expr(double float_val, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONST_EXPR;
    node->data_type = TYPE_FLOAT;
    node->line_no = line_no;
    node->const_expr.float_val = float_val;
    return node;
}

/* 创建数组下标节点 */
ASTNode *new_array_index(ASTNode *expr, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_ARRAY_INDEX;
    node->line_no = line_no;
    node->array_index.expr = expr;
    node->next = NULL;
    return node;
}

/* 创建左值节点 */
ASTNode *new_lval(char *name, ASTNode *indices, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_LVAL;
    node->line_no = line_no;
    node->lval.name = strdup(name);
    node->lval.indices = indices;
    return node;
}

/* 创建二元运算节点 */
ASTNode *new_bin_op(Operator op, ASTNode *left, ASTNode *right, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_BIN_OP;
    node->line_no = line_no;
    node->bin_op.op = op;
    node->bin_op.left = left;
    node->bin_op.right = right;
    // if (left->type == NODE_LVAL) printf("left is lval\n");
    // 推导数据类型（以左操作数为准）
    node->data_type = (left->data_type == TYPE_FLOAT || right->data_type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
    node->next = NULL;
    return node;
}

/* 创建一元运算节点 */
ASTNode *new_unary_op(Operator op, ASTNode *expr, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_UNARY_OP;
    node->line_no = line_no;
    node->unary_op.op = op;
    node->unary_op.expr = expr;
    if (expr)
        node->data_type = expr->data_type;
    return node;
}

/* 创建函数调用节点 */
ASTNode *new_func_call(char *name, ASTNode *args, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_FUNC_CALL;
    node->line_no = line_no;
    node->func_call.name = strdup(name);
    node->func_call.args = args;
    return node; // 数据类型需后续通过符号表解析
}

/* 创建类型转换节点 */
ASTNode *new_type_cast(ASTNode *expr, DataType target_type, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_TYPE_CAST;
    node->data_type = target_type;
    node->line_no = line_no;
    node->unary_op.expr = expr;
    return node;
}

/* 创建赋值语句节点 */
ASTNode *new_assign(ASTNode *lval, ASTNode *rval, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_ASSIGN;
    node->line_no = line_no;
    node->assign.lval = lval;
    node->assign.rval = rval;
    return node;
}

/* 创建语句块节点 */
ASTNode *new_block(ASTNode *items, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_BLOCK;
    node->line_no = line_no;
    node->block.items = items ? items->block.items : NULL;
    node->block.scope = NULL;
    return node;
}

/* 创建if语句节点 */
ASTNode *new_if(ASTNode *cond, ASTNode *then, ASTNode *els, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_IF;
    node->line_no = line_no;
    node->if_stmt.cond = cond;
    node->if_stmt.then = then;
    node->if_stmt.els = els;
    node->if_stmt.then_scope = node->if_stmt.els_scope = NULL;
    return node;
}

/* 创建while语句节点 */
ASTNode *new_while(ASTNode *cond, ASTNode *body, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_WHILE;
    node->line_no = line_no;
    node->while_stmt.cond = cond;
    node->while_stmt.body = body;
    node->while_stmt.body_scope = NULL;
    return node;
}

/* 创建return语句节点 */
ASTNode *new_return(ASTNode *expr, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_RETURN;
    node->line_no = line_no;
    node->return_stmt.expr = expr;
    return node;
}

/* 创建break节点 */
ASTNode *new_break(int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_BREAK;
    node->line_no = line_no;
    return node;
}

/* 创建continue节点 */
ASTNode *new_continue(int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONTINUE;
    node->line_no = line_no;
    return node;
}

/* 创建表达式语句节点 */
ASTNode *new_expr_stmt(ASTNode *expr, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_EXPR_STMT;
    node->line_no = line_no;
    node->return_stmt.expr = expr; // 复用return_stmt结构体
    return node;
}

/* 创建类型节点 */
ASTNode *new_type_node(DataType type)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_TYPE;
    node->data_type = type;
    node->type_node.basic_type = type;
    return node;
}

/* 创建数组维度节点 */
ASTNode *new_array_dims(ASTNode *size, ASTNode *head)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_ARRAY_DIMS;
    node->array_dims.size = size;
    node->array_dims.next_dim = NULL;
    if (head)
    {
        // printf("head not null\n");
        node->array_dims.next_dim = head;
        return node;
    }
    else
    {
        // printf("head null\n");
        head = node;
    }
    return head;
}

/* 创建初始化值节点 */
ASTNode *new_init_val(ASTNode *expr, ASTNode *next, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_INIT_VAL;
    node->line_no = line_no;
    node->init_val.expr = expr;
    node->init_val.next = next;
    return node;
}

ASTNode *new_init_list(ASTNode *init, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_INIT_LIST;
    node->line_no = line_no;
    node->init_list.first = init;
    node->next = NULL;
    return node;
}

ASTNode *new_const_init(ASTNode *expr, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONST_INIT;
    node->line_no = line_no;
    node->const_init.expr = expr;
    node->next = NULL;
    return node;
}

ASTNode *new_const_init_list(ASTNode *init, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = NODE_CONST_INIT_LIST;
    node->line_no = line_no;
    node->const_init_list.first = init;
    node->next = NULL;
    return node;
}

/* 创建声明节点 */
ASTNode *new_decl(NodeType type, char *name, ASTNode *dims, ASTNode *init, int line_no)
{
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line_no = line_no;
    node->decl.name = strdup(name);
    node->decl.dims = dims;
    node->decl.init = init;
    node->next = NULL;
    return node;
}

/* 链表追加函数 */
ASTNode *list_append(ASTNode *list, ASTNode *item)
{
    if (!list)
        return item;
    ASTNode *p = list;
    while (p->next)
        p = p->next;
    p->next = item;
    return list;
}

// int count = 0;

/* 递归释放AST内存 */
void free_ast(ASTNode *node)
{
    if (!node)
        return;
    // printf("%d\n", node->type);
    switch (node->type)
    {
    case NODE_COMP_UNIT:
        for (ASTNode *child = node->comp.children; child; child = child->next)
            free_ast(child);
        break;
    case NODE_FUNC_DEF:
        free(node->func_def.name);
        free_ast(node->func_def.params);
        free_ast(node->func_def.body);
        break;
    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
    case NODE_ARRAY_DECL:
    case NODE_CONST_ARRAY_DECL:
        free(node->decl.name);
        free_ast(node->decl.dims);
        free_ast(node->decl.init);
        break;
    case NODE_LVAL:
        free(node->lval.name);
        free_ast(node->lval.indices);
        break;
    case NODE_FUNC_CALL:
        free(node->func_call.name);
        free_ast(node->func_call.args);
        break;
    /* 表达式类节点 */
    case NODE_BIN_OP:
    {
        free_ast(node->bin_op.left);
        free_ast(node->bin_op.right);
        break;
    }

    case NODE_UNARY_OP:
    {
        free_ast(node->unary_op.expr);
        break;
    }

    case NODE_TYPE_CAST:
    {
        free_ast(node->unary_op.expr); // 复用 unary_op 结构体
        break;
    }

    /* 语句类节点 */
    case NODE_ASSIGN:
    {
        free_ast(node->assign.lval);
        free_ast(node->assign.rval);
        break;
    }

    case NODE_BLOCK:
    {
        ASTNode *item = node->block.items;
        while (item)
        {
            ASTNode *next = item->next;
            free_ast(item);
            item = next;
        }
        break;
    }

    case NODE_IF:
    {
        free_ast(node->if_stmt.cond);
        free_ast(node->if_stmt.then);
        free_ast(node->if_stmt.els);
        break;
    }

    case NODE_WHILE:
    {
        free_ast(node->while_stmt.cond);
        free_ast(node->while_stmt.body);
        break;
    }

    case NODE_RETURN:
    {
        free_ast(node->return_stmt.expr);
        break;
    }

    case NODE_EXPR_STMT:
    {
        free_ast(node->return_stmt.expr); // 复用 return_stmt 结构体
        break;
    }

    case NODE_INIT_VAL:
        free_ast(node->init_val.expr);
        free_ast(node->init_val.next);
        break;
    case NODE_INIT_LIST:
        free_ast(node->init_list.first);
        break;
    case NODE_CONST_INIT:
        free_ast(node->const_init.expr);
        break;
    case NODE_CONST_INIT_LIST:
        free_ast(node->const_init_list.first);
        break;

    /* 基础节点（无额外资源） */
    case NODE_BREAK:
    case NODE_CONTINUE:
    case NODE_CONST_EXPR:
        // 无动态分配内存需要释放
        break;
    case NODE_ARRAY_INDEX:
        free_ast(node->array_index.expr);
        free_ast(node->next);
        break;
    case NODE_ARRAY_DIMS:
        free_ast(node->array_dims.size);
        free_ast(node->array_dims.next_dim);
        break;
    default:
        fprintf(stderr, "警告：未处理的节点类型 %d 的资源释放\n", (int)node->type);
    }
    // free_ast(node->next);
    free(node);
}

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
}

static const char *get_op_str(Operator op)
{
    switch (op)
    {
    case OP_ADD:
        return "+";
    case OP_NEG:
    case OP_SUB:
        return "-";
    case OP_MUL:
        return "*";
    case OP_DIV:
        return "/";
    case OP_MOD:
        return "%";
    case OP_AND:
        return "&&";
    case OP_OR:
        return "||";
    case OP_NOT:
        return "!";
    case OP_EQ:
        return "==";
    case OP_NE:
        return "!=";
    case OP_LT:
        return "<";
    case OP_GT:
        return ">";
    case OP_LE:
        return "<=";
    case OP_GE:
        return ">=";
    case OP_BAND:
        return "&";
    case OP_BNOT:
        return "~";
    case OP_BOR:
        return "|";
    case OP_BXOR:
        return "^";
    case OP_SHL:
        return "<<";
    case OP_SHR:
        return ">>";
    default:
        return "unknown";
    }
}

static const char *get_type_str(DataType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_VOID:
        return "void";
    default:
        return "unknown";
    }
}

void print_ast(ASTNode *node, int depth)
{
    if (!node)
        return;

    print_indent(depth);

    switch (node->type)
    {
    case NODE_COMP_UNIT:
        printf("CompUnit\n");
        // for (ASTNode *child = node->comp.children; child; child = child->next)
        // {
        //     print_ast(child, depth + 1);
        // }
        if (node->comp.children) {
            print_ast(node->comp.children, depth + 1);
        }
        break;

    case NODE_FUNC_DEF:
        printf("FuncDef: %s %s\n", get_type_str(node->data_type), node->func_def.name);
        if (node->func_def.params)
        {
            print_indent(depth + 1);
            printf("Params:\n");
            print_ast(node->func_def.params, depth + 2);
        }
        print_indent(depth + 1);
        printf("Body:\n");
        print_ast(node->func_def.body, depth + 2);
        break;

    /* 在 print_ast 函数的 switch 语句中添加 NODE_BLOCK case */
    case NODE_BLOCK: {
        printf("Block:\n");
        // 遍历并打印块中的所有语句
        if (node->block.items)
        {
            ASTNode *item = node->block.items;
            print_ast(item, depth + 1);
            // while (item)
            // {
            //     print_ast(item, depth + 1);
            //     item = item->next;
            // }
        }
        break;

    }
    case NODE_VAR_DECL:
        printf("VarDecl: %s %s\n", get_type_str(node->data_type), node->decl.name);
        if (node->decl.init)
        {
            print_indent(depth + 1);
            printf("Init:\n");
            print_ast(node->decl.init, depth + 2);
        }
        // if (node->next)
        //     print_ast(node->next, depth);
        break;

    case NODE_CONST_DECL:
        printf("ConstDecl: %s %s\n", get_type_str(node->data_type), node->decl.name);
        if (node->decl.init)
        {
            print_indent(depth + 1);
            printf("Init:\n");
            print_ast(node->decl.init, depth + 2);
        }
        // if (node->next)
        //     print_ast(node->next, depth);
        break;

    case NODE_ARRAY_DECL:
        printf("ArrayDecl: %s %s\n", get_type_str(node->data_type), node->decl.name);
        if (node->decl.dims)
        {
            print_indent(depth + 1);
            printf("Dimensions:\n");
            print_ast(node->decl.dims, depth + 2);
        }
        if (node->decl.init)
        {
            print_indent(depth + 1);
            printf("Init:\n");
            print_ast(node->decl.init, depth + 2);
        }
        // if (node->next)
        //     print_ast(node->next, depth);
        break;

    case NODE_CONST_ARRAY_DECL:
        printf("ConstArrayDecl: %s %s\n", get_type_str(node->data_type), node->decl.name);
        if (node->decl.dims)
        {
            print_indent(depth + 1);
            printf("Dimensions:\n");
            print_ast(node->decl.dims, depth + 2);
        }
        if (node->decl.init)
        {
            print_indent(depth + 1);
            printf("Init:\n");
            print_ast(node->decl.init, depth + 2);
        }
        // if (node->next)
        //     print_ast(node->next, depth);
        break;

    case NODE_BIN_OP:
        printf("BinOp: %s\n", get_op_str(node->bin_op.op));
        print_ast(node->bin_op.left, depth + 1);
        print_ast(node->bin_op.right, depth + 1);
        break;

    case NODE_CONST_EXPR:
        if (node->data_type == TYPE_FLOAT)
        {
            printf("FloatConst: %f\n", node->const_expr.float_val);
        }
        else
        {
            printf("IntConst: %d\n", node->const_expr.int_val);
        }
        break;

    case NODE_LVAL:
        printf("LVal: %s\n", node->lval.name);
        if (node->lval.indices)
        {
            print_indent(depth + 1);
            printf("Indices:\n");
            print_ast(node->lval.indices, depth + 2);
        }
        break;

    case NODE_INIT_LIST:
    case NODE_CONST_INIT_LIST:
        printf("InitList:\n");
        if (node->type == NODE_INIT_LIST)
        {
            ASTNode *p = node->init_list.first;
            print_ast(p, depth + 1);
            // while (p)
            // {
            //     print_ast(p, depth + 1);
            //     p = p->next;
            // }
        }
        else
        {
            ASTNode *p = node->const_init_list.first;
            print_ast(p, depth + 1);
            // while (p)
            // {
            //     print_ast(p, depth + 1);
            //     p = p->next;
            // }
        }
        break;

    case NODE_ARRAY_INDEX: {
        printf("ArrayIndex:\n");
        ASTNode *p = node;
        print_ast(p->array_index.expr, depth + 1);
        // while (p)
        // {
        //     print_ast(p->array_index.expr, depth + 1);
        //     p = p->next;
        // }
        break;
    }

    /* 在 print_ast 函数的 switch 语句中添加以下 case */
    case NODE_TYPE_CAST:
        printf("TypeCast: %s\n", get_type_str(node->data_type));
        print_ast(node->unary_op.expr, depth + 1);
        break;

    case NODE_RETURN:
        printf("Return:\n");
        if (node->return_stmt.expr)
        {
            print_ast(node->return_stmt.expr, depth + 1);
        }
        break;

    case NODE_ARRAY_DIMS:
        printf("ArrayDim: size=");
        print_ast(node->array_dims.size, depth);
        if (node->array_dims.next_dim)
        {
            printf(", next=");
            print_ast(node->array_dims.next_dim, depth);
        }
        printf("\n");
        break;

    case NODE_INIT_VAL:
        printf("InitVal:\n");
        if (node->init_val.expr)
        {
            print_ast(node->init_val.expr, depth + 1);
        }
        if (node->init_val.next)
        {
            print_ast(node->init_val.next, depth);
        }
        break;

    case NODE_CONST_INIT:
        printf("ConstInit:\n");
        if (node->const_init.expr)
        {
            print_ast(node->const_init.expr, depth + 1);
        }
        break;

    case NODE_UNARY_OP:
        printf("UnaryOp: %s\n", get_op_str(node->unary_op.op));
        print_ast(node->unary_op.expr, depth + 1);
        break;

    case NODE_ASSIGN:
        printf("Assign:\n");
        print_indent(depth + 1);
        printf("Left:\n");
        print_ast(node->assign.lval, depth + 2);
        print_indent(depth + 1);
        printf("Right:\n");
        print_ast(node->assign.rval, depth + 2);
        break;

    case NODE_EXPR_STMT:
        printf("ExprStmt:\n");
        print_ast(node->return_stmt.expr, depth + 1);
        break;

    case NODE_FUNC_CALL:
        printf("FuncCall: %s\n", node->func_call.name);
        if (node->func_call.args)
        {
            print_indent(depth + 1);
            printf("Args:\n");
            ASTNode *arg = node->func_call.args;
            print_ast(arg, depth + 2);
            // while (arg)
            // {
            //     print_ast(arg, depth + 2);
            //     arg = arg->next;
            // }
        }
        break;

    case NODE_IF:
        printf("If:\n");
        print_indent(depth + 1);
        printf("Condition:\n");
        print_ast(node->if_stmt.cond, depth + 2);
        print_indent(depth + 1);
        printf("Then:\n");
        print_ast(node->if_stmt.then, depth + 2);
        if (node->if_stmt.els)
        {
            print_indent(depth + 1);
            printf("Else:\n");
            print_ast(node->if_stmt.els, depth + 2);
        }
        break;

    case NODE_WHILE:
        printf("While:\n");
        print_indent(depth + 1);
        printf("Condition:\n");
        print_ast(node->while_stmt.cond, depth + 2);
        print_indent(depth + 1);
        printf("Body:\n");
        print_ast(node->while_stmt.body, depth + 2);
        break;
    case NODE_BREAK:
        printf("Break\n");
        break;

    default:
        printf("Unknown node type: %d\n", node->type);
    }

    // 处理链表中的下一个节点
    if (node->next) {
        print_ast(node->next, depth);
    }
}
