#include "ir_generator.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cstdint> // for uint32_t
#include <cstring> // for memcpy
#include <iomanip>

#include "ast.h"
#include "symbol.h"

// +++ 新增辅助函数: 在IR生成期间计算常量表达式 +++

double IRGenerator::evaluate_float_constant_expression(ASTNode *node)
{
    if (!node)
        return 0.0;

    switch (node->type)
    {
    case NODE_CONST_EXPR:
        // 基础情况: 从字面量节点返回值 (提升为double)
        return node->data_type == TYPE_FLOAT ? node->const_expr.float_val : (double)node->const_expr.int_val;

    case NODE_LVAL:
    {
        // 基础情况: 从符号表返回值 (已经是double)
        Symbol *sym = find_symbol(node->lval.name);
        assert(sym && sym->sym_type == SYM_CONST);
        return sym->data_type == TYPE_FLOAT ? sym->const_float_val : (double)sym->const_val;
    }

    case NODE_BIN_OP:
    {
        // +++ 核心修复点: 运算在float精度下进行 +++
        // 1. 以double精度获取左右操作数的值
        double left_d = evaluate_float_constant_expression(node->bin_op.left);
        double right_d = evaluate_float_constant_expression(node->bin_op.right);

        // 2. 将操作数强制转换为float，模拟C语言的运算规则
        float left_f = (float)left_d;
        float right_f = (float)right_d;
        float result_f;

        // 3. 在float精度下执行运算
        switch (node->bin_op.op)
        {
        case OP_ADD:
            result_f = left_f + right_f;
            break;
        case OP_SUB:
            result_f = left_f - right_f;
            break;
        case OP_MUL:
            result_f = left_f * right_f;
            break;
        case OP_DIV:
            result_f = left_f / right_f;
            break;
        default:
            assert(0 && "Unsupported operator in float constant expression");
        }

        // 4. 将float结果转换回double返回
        return (double)result_f;
    }

    case NODE_UNARY_OP:
    {
        // 同样，一元运算也应在float精度下进行
        double val_d = evaluate_float_constant_expression(node->unary_op.expr);
        float val_f = (float)val_d;
        float result_f;

        if (node->unary_op.op == OP_NEG)
            result_f = -val_f;
        else if (node->unary_op.op == OP_POS)
            result_f = val_f;
        else
            assert(0 && "Unsupported unary operator in float constant expression");

        return (double)result_f;
    }

    // 处理包装节点和类型转换节点
    case NODE_TYPE_CAST:
        return evaluate_float_constant_expression(node->unary_op.expr);
    case NODE_INIT_VAL:
        return evaluate_float_constant_expression(node->init_val.expr);
    case NODE_CONST_INIT:
        return evaluate_float_constant_expression(node->const_init.expr);

    default:
        fprintf(stderr, "Unsupported node type for float constant evaluation: %d\n", node->type);
        assert(0 && "Unsupported node type in float constant expression");
    }
    return 0.0;
}

int IRGenerator::evaluate_constant_expression(ASTNode *node)
{
    if (!node)
        return 0;

    switch (node->type)
    {
    case NODE_CONST_EXPR:
        // +++ 修复点: 正确处理浮点和整型常量 +++
        if (node->data_type == TYPE_FLOAT)
        {
            return (int)node->const_expr.float_val;
        }
        return node->const_expr.int_val;

    case NODE_LVAL:
    {
        Symbol *sym = find_symbol(node->lval.name);
        assert(sym && sym->sym_type == SYM_CONST);
        // +++ 修复点: 支持从符号表查找浮点常量 +++
        if (sym->data_type == TYPE_FLOAT)
        {
            // 注意：这依赖于语义分析阶段正确填充了 const_float_val
            return (int)sym->const_float_val;
        }
        return sym->const_val;
    }

    case NODE_BIN_OP:
    {
        int left = evaluate_constant_expression(node->bin_op.left);
        int right = evaluate_constant_expression(node->bin_op.right);
        switch (node->bin_op.op)
        {
        case OP_ADD:
            return left + right;
        case OP_SUB:
            return left - right;
        case OP_MUL:
            return left * right;
        case OP_DIV:
            return left / right;
        case OP_MOD:
            return left % right;
        default:
            assert(0 && "Unsupported operator in constant expression");
        }
    }

    case NODE_UNARY_OP:
    {
        int val = evaluate_constant_expression(node->unary_op.expr);
        if (node->unary_op.op == OP_NEG)
            return -val;
        if (node->unary_op.op == OP_POS)
            return val;
        assert(0 && "Unsupported unary operator in constant expression");
    }

    // +++ 修复点: 添加对类型转换节点的处理, 这是导致断言失败的直接原因 +++
    case NODE_TYPE_CAST:
        // 直接对内部表达式求值。因为本函数返回int，所以类型转换被隐式正确地处理了。
        return evaluate_constant_expression(node->unary_op.expr);

    // +++ FIX: Add cases for initializer wrappers +++
    case NODE_INIT_VAL:
        return evaluate_constant_expression(node->init_val.expr);
    case NODE_CONST_INIT:
        return evaluate_constant_expression(node->const_init.expr);

    default:
        fprintf(stderr, "Unsupported node type for constant evaluation: %d\n", node->type);
        assert(0 && "Unsupported node type in constant expression");
    }
    return 0;
}

int IRGenerator::total_elements(const std::vector<int> &dimensions)
{
    if (dimensions.empty())
        return 1;
    int total = 1;
    for (int dim : dimensions)
        total *= dim > 0 ? dim : 1;
    return total;
}

std::vector<int> IRGenerator::get_multi_dim_indices(int flat_index, const std::vector<int> &dimensions)
{
    std::vector<int> indices;
    if (dimensions.empty())
        return indices;
    int temp_index = flat_index;
    for (size_t d = 0; d < dimensions.size(); ++d)
    {
        int divisor = 1;
        for (size_t k = d + 1; k < dimensions.size(); ++k)
        {
            divisor *= dimensions[k];
        }
        indices.push_back(temp_index / divisor);
        temp_index %= divisor;
    }
    return indices;
}

void IRGenerator::store_array_element(
    const std::string &array_ptr, const std::string &full_array_type, const std::string &base_type_str,
    const std::vector<int> &dimensions, int flat_index, const std::string &value_to_store)
{
    std::string elem_ptr = new_temp_reg();
    std::vector<int> multi_dim_indices = get_multi_dim_indices(flat_index, dimensions);
    std::string gep_indices_str = "i64 0";
    for (int idx : multi_dim_indices)
    {
        gep_indices_str += ", i64 " + std::to_string(idx);
    }
    ir_stream << "  " << elem_ptr << " = getelementptr inbounds " << full_array_type << ", "
              << full_array_type << "* " << array_ptr << ", " << gep_indices_str << "\n";
    ir_stream << "  store " << base_type_str << " " << value_to_store << ", "
              << base_type_str << "* " << elem_ptr << "\n";
}

// +++ 完全重写的数组初始化函数 +++
void IRGenerator::generate_array_initialization(
    const std::string &array_ptr, ASTNode *init_node, const std::vector<int> &dimensions,
    const std::string &base_type_str, const std::string &full_array_type)
{
    // --- 阶段 1: 使用 llvm.memset 将整个数组内存清零 ---
    // 这可以正确处理 C 语言的规则：未显式初始化的元素默认为零。
    int num_total_elements = total_elements(dimensions);
    long long total_bytes = (long long)num_total_elements * 4; // 假设 i32/float 均为 4 字节

    if (total_bytes > 0)
    {
        std::string bitcast_ptr = new_temp_reg();
        ir_stream << "  " << bitcast_ptr << " = bitcast " << full_array_type << "* " << array_ptr
                  << " to i8*\n";
        ir_stream << "  call void @llvm.memset.p0i8.i64(i8* " << bitcast_ptr
                  << ", i8 0, i64 " << total_bytes << ", i1 false)\n";
    }

    // 如果没有初始化节点 (例如 int a[10];)，清零后即可返回。
    if (!init_node)
        return;

    // CORREÇÃO: 处理语义分析阶段可能在初始化列表外层包裹的多余的类型转换节点
    if (init_node->type == NODE_TYPE_CAST)
    {
        init_node = init_node->unary_op.expr;
    }

    // --- 阶段 2: 使用显式初始值覆盖清零后的内存 ---
    // 处理非列表初始化, 例如: `int a[] = 1;`
    if (init_node->type != NODE_INIT_LIST && init_node->type != NODE_CONST_INIT_LIST)
    {
        ASTNode *expr_node = nullptr;
        if (init_node->type == NODE_INIT_VAL)
            expr_node = init_node->init_val.expr;
        else if (init_node->type == NODE_CONST_INIT)
            expr_node = init_node->const_init.expr;
        else
            expr_node = init_node;

        if (expr_node)
        {
            std::string val_str = visit(expr_node);
            store_array_element(array_ptr, full_array_type, base_type_str, dimensions, 0, val_str);
        }
        return;
    }

    // --- 阶段 3: 展平初始化列表并生成 store 指令 ---
    // `flatten_and_align` 的逻辑用于解析C风格的嵌套大括号初始化，计算每个值的正确“扁平化”索引
    std::vector<std::pair<int, ASTNode *>> initializers;
    int flat_index = 0;
    int max_elements = total_elements(dimensions);

    std::function<void(ASTNode *, int)> flatten_and_align =
        [&](ASTNode *item, int dim_level)
    {
        if (!item || flat_index >= max_elements)
            return;

        bool is_braced_sublist = (item->type == NODE_INIT_LIST || item->type == NODE_CONST_INIT_LIST);

        if (item->type == NODE_INIT_VAL || item->type == NODE_CONST_INIT)
        {
            ASTNode *expr = (item->type == NODE_INIT_VAL) ? item->init_val.expr : item->const_init.expr;
            if (expr && (expr->type == NODE_INIT_LIST || expr->type == NODE_CONST_INIT_LIST))
            {
                is_braced_sublist = true;
                item = expr;
            }
        }

        if (is_braced_sublist)
        {
            int sub_array_size = 1;
            if (dim_level < (int)dimensions.size())
            {
                for (size_t i = dim_level + 1; i < dimensions.size(); ++i)
                {
                    sub_array_size *= dimensions[i] > 0 ? dimensions[i] : 1;
                }
            }
            int start_of_sub_aggregate = flat_index;
            ASTNode *sub_item = (item->type == NODE_INIT_LIST) ? item->init_list.first : item->const_init_list.first;
            while (sub_item)
            {
                if (flat_index >= start_of_sub_aggregate + sub_array_size)
                    break;
                flatten_and_align(sub_item, dim_level + 1);
                sub_item = sub_item->next;
            }
            // 遇到子列表时，索引要对齐到下一个子数组的开头
            flat_index = start_of_sub_aggregate + sub_array_size;
        }
        else
        {
            ASTNode *expr_to_visit = nullptr;
            if (item->type == NODE_INIT_VAL)
                expr_to_visit = item->init_val.expr;
            else if (item->type == NODE_CONST_INIT)
                expr_to_visit = item->const_init.expr;
            else
                expr_to_visit = item;

            if (expr_to_visit)
                initializers.push_back({flat_index, expr_to_visit});
            flat_index++;
        }
    };

    ASTNode *top_level_item = (init_node->type == NODE_INIT_LIST) ? init_node->init_list.first : init_node->const_init_list.first;
    if (top_level_item)
    {
        ASTNode *current_item = top_level_item;
        while (current_item && flat_index < max_elements)
        {
            flatten_and_align(current_item, 0);
            current_item = current_item->next;
        }
    }

    // --- 关键修复: 恢复对每个初始值进行类型转换 ---
    DataType dest_type = (base_type_str == "float") ? TYPE_FLOAT : TYPE_INT;
    for (const auto &init_pair : initializers)
    {
        if (init_pair.first < max_elements)
        {
            ASTNode *expr_node = init_pair.second;
            std::string val_str = visit(expr_node);
            DataType expr_type = expr_node->data_type;

            if (expr_type != dest_type)
            {
                std::string op;
                std::string expr_llvm_type = get_llvm_type(expr_type);
                if (expr_type == TYPE_INT && dest_type == TYPE_FLOAT)
                    op = "sitofp";
                else if (expr_type == TYPE_FLOAT && dest_type == TYPE_INT)
                    op = "fptosi";

                if (!op.empty())
                {
                    std::string converted_reg = new_temp_reg();
                    ir_stream << "  " << converted_reg << " = " << op << " "
                              << expr_llvm_type << " " << val_str << " to " << base_type_str << "\n";
                    val_str = converted_reg;
                }
            }
            store_array_element(array_ptr, full_array_type, base_type_str, dimensions, init_pair.first, val_str);
        }
    }
}

// +++ 新增辅助函数: 为全局常量数组生成初始化字符串 +++
std::string IRGenerator::generate_constant_array_initializer_string(
    ASTNode *init_node, const std::vector<int> &dimensions, const std::string &base_type_str)
{
    if (!init_node)
    {
        return "zeroinitializer";
    }

    std::vector<std::string> flat_initializers(total_elements(dimensions), "0");
    int flat_index = 0;
    int max_elements = total_elements(dimensions);

    std::function<void(ASTNode *, int)> flatten_and_align =
        [&](ASTNode *item, int dim_level)
    {
        if (!item || flat_index >= max_elements)
            return;

        // Check if the current item represents a braced sublist
        bool is_braced_sublist = (item->type == NODE_INIT_LIST || item->type == NODE_CONST_INIT_LIST);
        ASTNode *expr = nullptr;
        if (item->type == NODE_INIT_VAL)
            expr = item->init_val.expr;
        if (item->type == NODE_CONST_INIT)
            expr = item->const_init.expr;
        if (expr && (expr->type == NODE_INIT_LIST || expr->type == NODE_CONST_INIT_LIST))
        {
            is_braced_sublist = true;
            item = expr;
        }

        if (is_braced_sublist)
        {
            // ... (this part of the logic is likely correct) ...
            int sub_array_size = 1;
            if (dim_level < (int)dimensions.size())
            {
                for (size_t i = dim_level + 1; i < dimensions.size(); ++i)
                {
                    sub_array_size *= dimensions[i];
                }
            }
            int start_of_sub_aggregate = flat_index;
            ASTNode *sub_item = (item->type == NODE_INIT_LIST) ? item->init_list.first : item->const_init_list.first;
            while (sub_item)
            {
                if (flat_index >= start_of_sub_aggregate + sub_array_size)
                    break;
                flatten_and_align(sub_item, dim_level + 1);
                sub_item = sub_item->next;
            }
            flat_index = start_of_sub_aggregate + sub_array_size;
        }
        else
        {
            // This is a scalar initializer
            // +++ THE FIX: Handle both INIT_VAL and CONST_INIT +++
            ASTNode *value_expr = nullptr;
            if (item->type == NODE_INIT_VAL)
                value_expr = item->init_val.expr;
            if (item->type == NODE_CONST_INIT)
                value_expr = item->const_init.expr;
            if (item->type == NODE_CONST_EXPR)
                value_expr = item; // It could be a direct expression

            if (value_expr)
            {
                flat_initializers[flat_index] = std::to_string(evaluate_constant_expression(value_expr));
            }
            flat_index++;
        }
    };

    // The top-level node could be either type of list
    ASTNode *top_level_item = (init_node->type == NODE_INIT_LIST) ? init_node->init_list.first : init_node->const_init_list.first;
    while (top_level_item && flat_index < max_elements)
    {
        flatten_and_align(top_level_item, 0);
        top_level_item = top_level_item->next;
    }

    // ... (rest of the function for building the string is correct) ...
    std::function<std::string(int, int)> build_string =
        [&](int level, int start_flat_index) -> std::string
    {
        if (level == (int)dimensions.size())
        {
            return flat_initializers[start_flat_index];
        }
        std::string result = "[";
        int dim_size = dimensions[level];
        int stride = total_elements(dimensions) / (std::accumulate(dimensions.begin(), dimensions.begin() + level + 1, 1, std::multiplies<int>()));
        std::string inner_type = base_type_str;
        for (size_t i = dimensions.size() - 1; i > (size_t)level; --i)
        {
            inner_type = "[" + std::to_string(dimensions[i]) + " x " + inner_type + "]";
        }
        for (int i = 0; i < dim_size; ++i)
        {
            result += inner_type + " " + build_string(level + 1, start_flat_index + i * stride);
            if (i < dim_size - 1)
                result += ", ";
        }
        result += "]";
        return result;
    };

    return build_string(0, 0);
}

IRGenerator::IRGenerator() : temp_reg_counter(0), label_counter(0), block_is_terminated(false), float_global_counter(0)
{
    sylib_return_types = {
        {"getint", "i32"}, {"getch", "i32"}, {"getarray", "i32"}, {"getfloat", "float"}, {"getfarray", "i32"}, {"putint", "void"}, {"putch", "void"}, {"putarray", "void"}, {"putfloat", "void"}, {"putfarray", "void"}, {"putf", "void"}, {"starttime", "void"}, {"stoptime", "void"}};
    sylib_declarations = {
        {"getint", "declare i32 @getint()\n"},
        {"getch", "declare i32 @getch()\n"},
        {"getarray", "declare i32 @getarray(i32*)\n"},
        {"getfloat", "declare float @getfloat()\n"},
        {"getfarray", "declare i32 @getfarray(float*)\n"},
        {"putint", "declare void @putint(i32)\n"},
        {"putch", "declare void @putch(i32)\n"},
        {"putarray", "declare void @putarray(i32, i32*)\n"},
        {"putfloat", "declare void @putfloat(float)\n"},
        {"putfarray", "declare void @putfarray(i32, float*)\n"},
        {"putf", "declare void @putf(i8*, ...)\n"},
        {"starttime", "declare void @_sysy_starttime(i32)\n"},
        {"stoptime", "declare void @_sysy_stoptime(i32)\n"}};

    scoped_named_values.emplace_back();
    scoped_array_info_cache.emplace_back();
}

void IRGenerator::enter_ir_scope()
{
    scoped_named_values.emplace_back(); // Push a new map for the new scope
    scoped_array_info_cache.emplace_back();
}

void IRGenerator::exit_ir_scope()
{
    if (scoped_named_values.size() > 1)
    { // Do not pop the global scope
        scoped_named_values.pop_back();
        scoped_array_info_cache.pop_back();
    }
}

std::string IRGenerator::find_named_value(const std::string &name)
{
    // Search from the innermost scope (back of vector) to the outermost
    for (auto it = scoped_named_values.rbegin(); it != scoped_named_values.rend(); ++it)
    {
        if (it->count(name))
        {
            return it->at(name);
        }
    }
    return ""; // Return empty string if not found in any local scope
}

const ArrayInfo *IRGenerator::find_array_info(const std::string &name)
{
    // Search from the innermost scope (back of vector) to the outermost
    for (auto it = scoped_array_info_cache.rbegin(); it != scoped_array_info_cache.rend(); ++it)
    {
        if (it->count(name))
        {
            return &it->at(name);
        }
    }
    return nullptr; // Return null if not found
}

std::string IRGenerator::getIR() { return float_const_declarations + ir_stream.str(); }

bool IRGenerator::is_sylib_function(const std::string &name) { return sylib_return_types.count(name) > 0; }

void IRGenerator::generate_sylib_declarations()
{
    for (auto const &[name, decl] : sylib_declarations)
        ir_stream << decl;
    ir_stream << "declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)\n\n";
}

std::string IRGenerator::new_temp_reg() { return "%" + std::to_string(temp_reg_counter++); }
std::string IRGenerator::new_label(const std::string &prefix) { return prefix + "." + std::to_string(label_counter++); }
std::string IRGenerator::get_llvm_type(DataType dt)
{
    switch (dt)
    {
    case TYPE_INT:
        return "i32";
    case TYPE_FLOAT:
        return "float";
    case TYPE_VOID:
        return "void";
    default:
        return "i32";
    }
}

void IRGenerator::generate(ASTNode *root)
{
    generate_sylib_declarations();
    if (root)
        visit(root);
}

std::string IRGenerator::visit(ASTNode *node)
{
    if (!node)
        return "";
    switch (node->type)
    {
    case NODE_COMP_UNIT:
        visitCompUnit(node);
        break;
    case NODE_FUNC_DEF:
        visitFuncDef(node);
        break;
    case NODE_BLOCK:
        visitBlock(node, true);
        break;
    case NODE_VAR_DECL:
    case NODE_ARRAY_DECL:
        visitVarDecl(node, true);
        break;
    case NODE_CONST_DECL:
    case NODE_CONST_ARRAY_DECL:
        visitVarDecl(node, true);
        break;
    case NODE_BIN_OP:
        return visitBinOp(node);
    case NODE_UNARY_OP:
        return visitUnaryOp(node);
    case NODE_LVAL:
        return visitLVal(node);
    case NODE_ASSIGN:
        return visitAssign(node);
    case NODE_CONST_EXPR:
        return visitConstExpr(node);
    case NODE_RETURN:
        visitReturn(node);
        break;
    case NODE_FUNC_CALL:
        return visitFuncCall(node);
    case NODE_IF:
        visitIf(node);
        break;
    case NODE_WHILE:
        visitWhile(node);
        break;
    case NODE_EXPR_STMT:
        visitExprStmt(node);
        break;
    case NODE_BREAK:
        visitBreak(node);
        break;
    case NODE_CONTINUE:
        visitContinue(node);
        break;
    // +++ 修复点: 添加对类型转换节点的处理 +++
    case NODE_TYPE_CAST:
    {
        std::string expr_val = visit(node->unary_op.expr);
        std::string from_type = get_llvm_type(node->unary_op.expr->data_type);
        std::string to_type = get_llvm_type(node->data_type);

        if (from_type == to_type)
            return expr_val;

        std::string result = new_temp_reg();
        std::string op;

        if (from_type == "i32" && to_type == "float")
        {
            op = "sitofp"; // signed int to float
        }
        else if (from_type == "float" && to_type == "i32")
        {
            op = "fptosi"; // float to signed int
        }
        else
        {
            // 对于其他可能的转换，例如 i1 to i32，先简单返回
            return expr_val;
        }

        ir_stream << "  " << result << " = " << op << " " << from_type << " " << expr_val << " to " << to_type << "\n";
        return result;
    }
    case NODE_INIT_VAL:
        return visitInitVal(node);
    case NODE_CONST_INIT:
    {
        // This is a wrapper. We visit the inner expression,
        // propagate its type to this node, and return its value.
        std::string result = visit(node->const_init.expr);
        if (node->const_init.expr)
        {
            node->data_type = node->const_init.expr->data_type;
        }
        return result;
    }
    case NODE_INIT_LIST:
        if (node->init_list.first)
            return visit(node->init_list.first);
        return "0";
    case NODE_CONST_INIT_LIST:
        if (node->const_init_list.first)
            return visit(node->const_init_list.first);
        return "0";
    default:
        break;
    }
    return "";
}

void IRGenerator::visitCompUnit(ASTNode *node)
{
    // 第一遍: 查找所有函数定义并存储它们的 AST 节点，以便后续在函数调用时查询参数类型。
    for (ASTNode *child = node->comp.children; child; child = child->next)
    {
        if (child->type == NODE_FUNC_DEF)
        {
            function_definitions[child->func_def.name] = child;
        }
    }

    // 第二遍: 按正常流程生成全局变量和函数的 IR 代码。
    for (ASTNode *child = node->comp.children; child; child = child->next)
    {
        if (child->type == NODE_VAR_DECL || child->type == NODE_CONST_DECL ||
            child->type == NODE_ARRAY_DECL || child->type == NODE_CONST_ARRAY_DECL)
        {
            visitVarDecl(child, false); // false 表示是全局变量
        }
        else
        {
            visit(child);
        }
        ir_stream << "\n";
    }
}

void IRGenerator::visitFuncDef(ASTNode *node)
{
    // --- 1. 准备阶段：进入新作用域，清空状态 ---
    enter_scope(node->func_def.name, 0);
    enter_ir_scope();
    temp_reg_counter = 0;
    local_var_allocas.clear();

    // --- 关键修复点 ---
    // 为每个新函数重置此状态，防止前一个函数的状态（例如，以 return 结尾）
    // 泄露到当前函数，导致当前函数的代码被错误地跳过。
    block_is_terminated = false;

    // --- 2. 函数签名处理 ---
    std::string func_name = node->func_def.name;
    std::string ret_type = get_llvm_type(node->func_def.ret_type);
    current_function_ret_type = ret_type;

    std::string params_str;
    std::vector<std::tuple<std::string, std::string, ASTNode *>> param_info;
    ASTNode *param_node = node->func_def.params;
    while (param_node)
    {
        std::string param_name = param_node->decl.name;
        std::string llvm_type = get_llvm_type(param_node->data_type);
        if (param_node->type == NODE_ARRAY_DECL)
        {
            llvm_type += "*";
            ArrayInfo info;
            info.is_param = true;
            info.dimensions.push_back(0);
            ASTNode *dim_node = param_node->decl.dims->array_dims.next_dim;
            while (dim_node)
            {
                info.dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
                dim_node = dim_node->array_dims.next_dim;
            }
            scoped_array_info_cache.back()[param_name] = info;
        }
        param_info.emplace_back(llvm_type, param_name, param_node);
        param_node = param_node->next;
    }
    for (size_t i = 0; i < param_info.size(); ++i)
    {
        params_str += std::get<0>(param_info[i]) + " %" + std::get<1>(param_info[i]);
        if (i < param_info.size() - 1)
            params_str += ", ";
    }

    ir_stream << "define " << ret_type << " @" << func_name << "(" << params_str << ") {\n";
    ir_stream << "entry:\n";

    // --- 3. 阶段一：在入口块为所有参数和局部变量生成 alloca ---
    for (const auto &p : param_info)
    {
        std::string param_type = std::get<0>(p);
        std::string param_name = std::get<1>(p);
        std::string param_reg = "%" + param_name;
        std::string ptr = "%" + param_name + ".addr";
        scoped_named_values.back()[param_name] = ptr;
        ir_stream << "  " << ptr << " = alloca " << param_type << "\n";
        ir_stream << "  store " << param_type << " " << param_reg << ", " << param_type << "* " << ptr << "\n";
    }

    std::vector<ASTNode *> local_decls;
    if (node->func_def.body)
    {
        findLocalDecls(node->func_def.body, local_decls);
    }

    for (ASTNode *decl_node : local_decls)
    {
        std::string final_type_str;
        std::string base_type_str = get_llvm_type(decl_node->data_type);
        if (decl_node->type == NODE_ARRAY_DECL || decl_node->type == NODE_CONST_ARRAY_DECL)
        {
            std::vector<int> dimensions;
            ASTNode *dim_node = decl_node->decl.dims;
            while (dim_node)
            {
                dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
                dim_node = dim_node->array_dims.next_dim;
            }

            std::string temp_type = base_type_str;
            for (int i = dimensions.size() - 1; i >= 0; --i)
            {
                temp_type = "[" + std::to_string(dimensions[i]) + " x " + temp_type + "]";
            }
            final_type_str = temp_type;
        }
        else
        {
            final_type_str = base_type_str;
        }
        std::string ptr = new_temp_reg();
        ir_stream << "  " << ptr << " = alloca " << final_type_str << "\n";
        local_var_allocas[decl_node] = ptr;
    }

    // --- 4. 阶段二：正常访问函数体，生成执行代码 ---
    if (node->func_def.body)
    {
        visit(node->func_def.body);
    }

    // --- 5. 检查并添加默认返回指令 ---
    // 如果代码块不是以一个显式的终结者指令结尾，就添加一个默认返回。
    if (!block_is_terminated)
    {
        if (ret_type == "void")
            ir_stream << "  ret void\n";
        else
            ir_stream << "  ret " << ret_type << (ret_type == "float" ? " 0.0" : " 0") << "\n";
    }

    ir_stream << "}\n";
    exit_ir_scope();
    exit_scope();
}

// +++ 修改后的 visitVarDecl, 处理全局/局部, 变量/常量 +++
void IRGenerator::visitVarDecl(ASTNode *node, bool is_local)
{
    // --- 1. 全局变量的处理逻辑 ---
    if (!is_local)
    {
        std::string var_name = node->decl.name;
        std::string base_type_str = get_llvm_type(node->data_type);
        std::string final_type_str;
        bool is_array = (node->type == NODE_ARRAY_DECL || node->type == NODE_CONST_ARRAY_DECL);
        bool is_const = (node->type == NODE_CONST_DECL || node->type == NODE_CONST_ARRAY_DECL);

        std::vector<int> dimensions;
        if (is_array)
        {
            ArrayInfo info;
            info.is_param = false;
            ASTNode *dim_node = node->decl.dims;
            while (dim_node)
            {
                dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
                dim_node = dim_node->array_dims.next_dim;
            }
            info.dimensions = dimensions;
            scoped_array_info_cache.front()[var_name] = info;

            std::string temp_type = base_type_str;
            for (int i = dimensions.size() - 1; i >= 0; --i)
            {
                temp_type = "[" + std::to_string(dimensions[i]) + " x " + temp_type + "]";
            }
            final_type_str = temp_type;
        }
        else
        {
            final_type_str = base_type_str;
        }

        std::string linkage = is_const ? "constant" : "global";
        std::string initializer_str;
        if (node->decl.init)
        {
            if (is_array)
            {
                initializer_str = generate_constant_array_initializer_string(node->decl.init, dimensions, base_type_str);
            }
            else
            {
                if (node->data_type == TYPE_FLOAT)
                {
                    // +++ 最终修复点: 对所有全局浮点常量统一使用64位十六进制位模式 +++
                    double val = evaluate_float_constant_expression(node->decl.init);
                    uint64_t u64_val;
                    memcpy(&u64_val, &val, sizeof(u64_val)); // 获取 double 的64位位模式

                    std::stringstream ss;
                    // 格式化为16位的十六进制数，不足的前面补0
                    ss << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << u64_val;
                    initializer_str = ss.str();
                }
                else
                { // TYPE_INT
                    initializer_str = std::to_string(evaluate_constant_expression(node->decl.init));
                }
            }
            // 参照Clang输出，添加dso_local和align 4
            ir_stream << "@" << var_name << " = " << linkage << " " << final_type_str << " " << initializer_str << "\n";
        }
        else
        {
            assert(!is_const && "Global constant must be initialized.");
            initializer_str = "zeroinitializer";
            ir_stream << "@" << var_name << " = common global " << final_type_str << " " << initializer_str << "\n";
        }
        return;
    }

    // --- 2. 局部变量的处理逻辑 (已修复) ---
    std::string var_name = node->decl.name;
    assert(local_var_allocas.count(node) && "FATAL: Local variable AST node not found in pre-scan allocas map!");
    std::string ptr = local_var_allocas.at(node);
    scoped_named_values.back()[var_name] = ptr;

    bool is_array = (node->type == NODE_ARRAY_DECL || node->type == NODE_CONST_ARRAY_DECL);
    if (is_array)
    {
        ArrayInfo info;
        info.is_param = false;
        ASTNode *dim_node = node->decl.dims;
        while (dim_node)
        {
            info.dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
            dim_node = dim_node->array_dims.next_dim;
        }
        scoped_array_info_cache.back()[var_name] = info;
    }

    if (node->decl.init)
    {
        std::string base_type_str = get_llvm_type(node->data_type);
        if (is_array)
        {
            const ArrayInfo *info = find_array_info(var_name);
            assert(info && "FATAL: ArrayInfo for local array not found in cache.");

            std::string full_array_type = base_type_str;
            for (int i = info->dimensions.size() - 1; i >= 0; --i)
            {
                full_array_type = "[" + std::to_string(info->dimensions[i]) + " x " + full_array_type + "]";
            }
            generate_array_initialization(ptr, node->decl.init, info->dimensions, base_type_str, full_array_type);
        }
        else
        {
            // +++ 核心修复点: 为局部变量初始化添加类型转换 +++
            // 1. 获取初始化表达式的值和类型
            std::string init_val_reg = visit(node->decl.init);
            DataType init_type = node->decl.init->data_type;
            std::string init_llvm_type = get_llvm_type(init_type);

            // 2. 获取目标变量的类型
            DataType dest_type = node->data_type;
            std::string dest_llvm_type = get_llvm_type(dest_type);

            // 3. 检查类型是否匹配，并在需要时插入转换指令
            if (init_type != dest_type)
            {
                std::string op;
                if (init_type == TYPE_INT && dest_type == TYPE_FLOAT)
                {
                    op = "sitofp"; // signed int to float
                }
                else if (init_type == TYPE_FLOAT && dest_type == TYPE_INT)
                {
                    op = "fptosi"; // float to signed int
                }

                if (!op.empty())
                {
                    std::string converted_reg = new_temp_reg();
                    ir_stream << "  " << converted_reg << " = " << op << " "
                              << init_llvm_type << " " << init_val_reg << " to " << dest_llvm_type << "\n";
                    // 后续 store 指令将使用这个新创建的、类型正确的寄存器
                    init_val_reg = converted_reg;
                }
            }

            // 4. 使用经过潜在转换的右值，生成 store 指令
            ir_stream << "  store " << dest_llvm_type << " " << init_val_reg << ", " << dest_llvm_type << "* " << ptr << "\n";
        }
    }
}

void IRGenerator::visitBlock(ASTNode *node, bool is_local)
{
    if (!node || !node->block.items)
        return;
    if (node->block.scope)
    {
        enter_scope(node->block.scope->name, 1);
        enter_ir_scope();
    }
    for (ASTNode *item = node->block.items; item; item = item->next)
    {
        // 关键修复：在处理下一条语句前，检查当前块是否已被终结。
        // 如果是，则后续所有语句都是死代码，应停止处理。
        if (block_is_terminated)
        {
            break;
        }
        visit(item);
    }
    if (node->block.scope)
    {
        exit_ir_scope();
        exit_scope();
    }
}
// ... The rest of the visit functions are unchanged and can be copied from the original ...
std::string IRGenerator::get_llvm_op_name(ASTNode *node)
{
    bool is_float = (node->bin_op.left->data_type == TYPE_FLOAT || node->bin_op.right->data_type == TYPE_FLOAT);
    switch (node->bin_op.op)
    {
    case OP_ADD:
        return is_float ? "fadd" : "add";
    case OP_SUB:
        return is_float ? "fsub" : "sub";
    case OP_MUL:
        return is_float ? "fmul" : "mul";
    case OP_DIV:
        return is_float ? "fdiv" : "sdiv";
    case OP_MOD:
        return "srem";
    case OP_EQ:
        return is_float ? "fcmp oeq" : "icmp eq";
    case OP_NE:
        return is_float ? "fcmp one" : "icmp ne";
    case OP_LT:
        return is_float ? "fcmp olt" : "icmp slt";
    case OP_GT:
        return is_float ? "fcmp ogt" : "icmp sgt";
    case OP_LE:
        return is_float ? "fcmp ole" : "icmp sle";
    case OP_GE:
        return is_float ? "fcmp oge" : "icmp sge";
    case OP_AND:
        return "and";
    case OP_OR:
        return "or";
    case OP_BAND:
        return "and";
    case OP_BOR:
        return "or";
    case OP_BXOR:
        return "xor";
    case OP_SHL:
        return "shl";
    case OP_SHR:
        return "ashr";
    default:
        return "unknown_op";
    }
}
std::string IRGenerator::visitBinOp(ASTNode *node)
{
    // --- 短路逻辑 (无变化) ---
    if (node->bin_op.op == OP_OR || node->bin_op.op == OP_AND)
    {
        bool is_or = (node->bin_op.op == OP_OR);

        std::string right_eval_label = new_label("sc.eval_right");
        std::string end_label = new_label("sc.end");

        std::string result_addr = new_temp_reg();
        ir_stream << "  " << result_addr << " = alloca i1\n";

        std::string left_val_reg = visit(node->bin_op.left);
        std::string left_bool_reg = new_temp_reg();
        if (node->bin_op.left->data_type == TYPE_FLOAT)
        {
            ir_stream << "  " << left_bool_reg << " = fcmp one float " << left_val_reg << ", 0.0\n";
        }
        else
        {
            ir_stream << "  " << left_bool_reg << " = icmp ne i32 " << left_val_reg << ", 0\n";
        }
        ir_stream << "  store i1 " << left_bool_reg << ", i1* " << result_addr << "\n";

        if (is_or)
        {
            ir_stream << "  br i1 " << left_bool_reg << ", label %" << end_label << ", label %" << right_eval_label << "\n";
        }
        else
        { // AND
            ir_stream << "  br i1 " << left_bool_reg << ", label %" << right_eval_label << ", label %" << end_label << "\n";
        }

        ir_stream << "\n"
                  << right_eval_label << ":\n";
        std::string right_val_reg = visit(node->bin_op.right);
        std::string right_bool_reg = new_temp_reg();
        if (node->bin_op.right->data_type == TYPE_FLOAT)
        {
            ir_stream << "  " << right_bool_reg << " = fcmp one float " << right_val_reg << ", 0.0\n";
        }
        else
        {
            ir_stream << "  " << right_bool_reg << " = icmp ne i32 " << right_val_reg << ", 0\n";
        }
        ir_stream << "  store i1 " << right_bool_reg << ", i1* " << result_addr << "\n";
        ir_stream << "  br label %" << end_label << "\n";

        ir_stream << "\n"
                  << end_label << ":\n";
        std::string final_bool_result = new_temp_reg();
        ir_stream << "  " << final_bool_result << " = load i1, i1* " << result_addr << "\n";
        std::string final_i32_result = new_temp_reg();
        ir_stream << "  " << final_i32_result << " = zext i1 " << final_bool_result << " to i32\n";

        node->data_type = TYPE_INT;
        return final_i32_result;
    }

    // --- 标准二元运算，带有操作数类型提升修复 ---
    std::string left = visit(node->bin_op.left);
    std::string right = visit(node->bin_op.right);

    DataType left_type = node->bin_op.left->data_type;
    DataType right_type = node->bin_op.right->data_type;
    DataType result_dt = (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;

    if (node->bin_op.op >= OP_BAND && node->bin_op.op <= OP_SHR)
    {
        result_dt = TYPE_INT;
    }

    // +++ 核心修复点: 如果是浮点运算，确保两个操作数都是浮点类型 +++
    if (result_dt == TYPE_FLOAT)
    {
        if (left_type == TYPE_INT)
        {
            std::string converted_left = new_temp_reg();
            ir_stream << "  " << converted_left << " = sitofp i32 " << left << " to float\n";
            left = converted_left;
        }
        if (right_type == TYPE_INT)
        {
            std::string converted_right = new_temp_reg();
            ir_stream << "  " << converted_right << " = sitofp i32 " << right << " to float\n";
            right = converted_right;
        }
    }

    std::string op = get_llvm_op_name(node);
    std::string type = get_llvm_type(result_dt);
    node->data_type = result_dt;

    std::string result = new_temp_reg();
    ir_stream << "  " << result << " = " << op << " " << type << " " << left << ", " << right << "\n";

    if (node->bin_op.op >= OP_EQ && node->bin_op.op <= OP_GE)
    {
        std::string final_result = new_temp_reg();
        ir_stream << "  " << final_result << " = zext i1 " << result << " to i32\n";
        node->data_type = TYPE_INT;
        return final_result;
    }
    return result;
}
std::string IRGenerator::visitUnaryOp(ASTNode *node)
{
    if (node->unary_op.op == OP_POS)
    {
        return visit(node->unary_op.expr);
    }

    std::string expr_reg = visit(node->unary_op.expr);
    std::string type = get_llvm_type(node->unary_op.expr->data_type);

    node->data_type = node->unary_op.expr->data_type;

    switch (node->unary_op.op)
    {
    case OP_NEG:
    {
        std::string result_reg = new_temp_reg();
        ir_stream << "  " << result_reg << " = " << (type == "float" ? "fsub" : "sub")
                  << " " << type << (type == "float" ? " 0.0, " : " 0, ") << expr_reg << "\n";
        return result_reg;
    }

    case OP_NOT:
    {
        // +++ 修复点: 为 '!' 操作符区分浮点和整型比较 +++
        std::string cmp_reg = new_temp_reg();
        std::string result_reg = new_temp_reg();

        if (type == "float")
        {
            // 浮点数与 0.0 比较
            ir_stream << "  " << cmp_reg << " = fcmp oeq float " << expr_reg << ", 0.0\n";
        }
        else
        {
            // 整数与 0 比较
            ir_stream << "  " << cmp_reg << " = icmp eq " << type << " " << expr_reg << ", 0\n";
        }

        ir_stream << "  " << result_reg << " = zext i1 " << cmp_reg << " to i32\n";

        node->data_type = TYPE_INT; // '!' 的结果总是 i32 类型的 0 或 1
        return result_reg;
    }

    case OP_BNOT:
    {
        std::string result_reg = new_temp_reg();
        ir_stream << "  " << result_reg << " = xor " << type << " " << expr_reg << ", -1\n";
        return result_reg;
    }

    default:
        return expr_reg;
    }
}
std::string IRGenerator::visitLVal_as_pointer(ASTNode *node)
{
    std::string var_name = node->lval.name;
    const ArrayInfo *info = find_array_info(var_name);
    Symbol *sym = find_symbol(var_name.c_str());
    assert(sym && "Symbol not found, semantic analysis should have caught this.");

    std::string base_ptr_or_name = find_named_value(var_name);
    if (base_ptr_or_name.empty())
    {
        base_ptr_or_name = "@" + var_name; // 全局变量
    }

    if (!info)
    {
        return base_ptr_or_name; // 非数组（标量），直接返回其地址
    }

    // 阶段 1: 预先计算所有索引表达式的值，避免SSA问题
    std::vector<std::string> index_regs;
    for (ASTNode *idx_node = node->lval.indices; idx_node; idx_node = idx_node->next)
    {
        index_regs.push_back(visit(idx_node->array_index.expr));
    }

    // 分支 1: 处理局部或全局数组
    if (!info->is_param)
    {
        std::string full_array_type = get_llvm_type(sym->data_type);
        for (int i = info->dimensions.size() - 1; i >= 0; --i)
        {
            full_array_type = "[" + std::to_string(info->dimensions[i]) + " x " + full_array_type + "]";
        }

        std::string gep_indices = "i64 0";
        for (const auto reg : index_regs)
        {
            gep_indices += ", i32 " + reg;
        }

        std::string elem_ptr = new_temp_reg();
        ir_stream << "  " << elem_ptr << " = getelementptr inbounds " << full_array_type
                  << ", " << full_array_type << "* " << base_ptr_or_name << ", " << gep_indices << "\n";
        return elem_ptr;
    }
    // 分支 2: 处理作为参数传递的数组 (已退化为指针)
    else
    {
        std::string base_type_str = get_llvm_type(sym->data_type);
        std::string ptr_type = base_type_str + "*";

        std::string loaded_base_ptr = new_temp_reg();
        ir_stream << "  " << loaded_base_ptr << " = load " << ptr_type << ", " << ptr_type << "* " << base_ptr_or_name << "\n";

        if (index_regs.empty())
        {
            return loaded_base_ptr;
        }

        // 计算扁平化索引: total_offset = i*D1*D2... + j*D2*D3... + ...
        std::string total_offset_reg = "0";
        for (size_t i = 0; i < index_regs.size(); ++i)
        {
            int stride = 1;
            // 参数数组的维度信息从第1个索引开始有效 (第0个是占位符)
            for (size_t j = i + 1; j < info->dimensions.size(); ++j)
            {
                stride *= info->dimensions[j];
            }

            std::string term_reg;
            if (stride == 1)
            {
                term_reg = index_regs[i];
            }
            else
            {
                term_reg = new_temp_reg();
                ir_stream << "  " << term_reg << " = mul i32 " << index_regs[i] << ", " << stride << "\n";
            }

            if (total_offset_reg == "0")
            {
                total_offset_reg = term_reg;
            }
            else
            {
                std::string new_offset = new_temp_reg();
                ir_stream << "  " << new_offset << " = add i32 " << total_offset_reg << ", " << term_reg << "\n";
                total_offset_reg = new_offset;
            }
        }

        std::string final_elem_ptr = new_temp_reg();
        ir_stream << "  " << final_elem_ptr << " = getelementptr inbounds " << base_type_str
                  << ", " << ptr_type << " " << loaded_base_ptr << ", i32 " << total_offset_reg << "\n";

        return final_elem_ptr;
    }
}

std::string IRGenerator::visitLVal(ASTNode *node)
{
    Symbol *sym = find_symbol(node->lval.name);
    assert(sym);
    node->data_type = sym->data_type;

    const ArrayInfo *info = find_array_info(node->lval.name);

    int num_indices = 0;
    if (node->lval.indices)
    {
        for (ASTNode *p = node->lval.indices; p; p = p->next)
            num_indices++;
    }

    // 如果数组没有被完全索引，它将退化为指向其第一个（或指定）元素的指针
    if (info && num_indices < info->dimensions.size())
    {
        // `visitLVal_as_pointer` 会返回指向该子数组的指针，这正是退化后的值
        return visitLVal_as_pointer(node);
    }

    // 否则，它是一个标量或一个被完全索引的数组元素，我们需要加载它的值
    std::string ptr = visitLVal_as_pointer(node);
    std::string type_to_load = get_llvm_type(sym->data_type);
    std::string result = new_temp_reg();
    ir_stream << "  " << result << " = load " << type_to_load << ", " << type_to_load << "* " << ptr << "\n";
    return result;
}

std::string IRGenerator::visitAssign(ASTNode *node)
{
    // 1. 访问右侧表达式，获取其值的寄存器和数据类型。
    //    visit() 调用会确保 node->assign.rval->data_type 被正确填充。
    std::string rval_reg = visit(node->assign.rval);
    DataType rval_type = node->assign.rval->data_type;
    std::string rval_llvm_type = get_llvm_type(rval_type);

    // 2. 获取左侧表达式的地址指针及其期望的数据类型。
    std::string lval_ptr = visitLVal_as_pointer(node->assign.lval);
    Symbol *lval_sym = find_symbol(node->assign.lval->lval.name);
    assert(lval_sym);
    DataType lval_type = lval_sym->data_type; // 数组的基类型
    std::string lval_llvm_type = get_llvm_type(lval_type);

    // 3. 核心修复点: 检查类型是否匹配，并在需要时插入转换指令。
    if (rval_type != lval_type)
    {
        std::string op;
        if (rval_type == TYPE_INT && lval_type == TYPE_FLOAT)
        {
            op = "sitofp"; // signed int to float
        }
        else if (rval_type == TYPE_FLOAT && lval_type == TYPE_INT)
        {
            op = "fptosi"; // float to signed int
        }

        // 如果存在有效的转换操作，则生成转换指令。
        if (!op.empty())
        {
            std::string converted_rval_reg = new_temp_reg();
            ir_stream << "  " << converted_rval_reg << " = " << op << " "
                      << rval_llvm_type << " " << rval_reg << " to " << lval_llvm_type << "\n";
            // 后续的 store 指令将使用这个新创建的、类型正确的寄存器。
            rval_reg = converted_rval_reg;
        }
    }

    // 4. 使用经过潜在转换的右值，生成 store 指令。
    ir_stream << "  store " << lval_llvm_type << " " << rval_reg << ", "
              << lval_llvm_type << "* " << lval_ptr << "\n";

    // C语言的赋值表达式本身会返回赋的值，其类型与左操作数相同。
    // 我们返回持有已存储值的寄存器，以支持链式赋值等情况 (例如 a = b = 5;)。
    return rval_reg;
}
std::string IRGenerator::visitConstExpr(ASTNode *node)
{
    if (node->data_type == TYPE_FLOAT)
    {
        double val = node->const_expr.float_val;
        // 0.0 是一个有效的立即操作数，可以直接使用。
        if (val == 0.0)
        {
            return "0.0";
        }

        // 检查是否已经为该浮点值创建了一个全局常量。
        if (float_globals.count(val))
        {
            std::string ptr = float_globals.at(val);
            std::string reg = new_temp_reg();
            ir_stream << "  " << reg << " = load float, float* " << ptr << ", align 4\n";
            return reg;
        }

        // 如果没有，则创建一个新的私有全局常量。
        std::string ptr = "@.fconst." + std::to_string(float_global_counter++);
        float_globals[val] = ptr;

        // LLVM 要求使用64位十六进制来表示浮点常量，即使目标类型是 float (32位)。
        uint64_t u64_val;
        memcpy(&u64_val, &val, sizeof(u64_val));
        std::stringstream hex_ss;
        hex_ss << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << u64_val;

        // 将全局常量的定义存储在一个单独的字符串中，以便稍后添加到IR的顶部。
        float_const_declarations += ptr + " = private unnamed_addr constant float " + hex_ss.str() + ", align 4\n";

        // 生成一条 load 指令，将常量值加载到寄存器中，并返回该寄存器。
        std::string reg = new_temp_reg();
        ir_stream << "  " << reg << " = load float, float* " << ptr << ", align 4\n";
        return reg;
    }
    return std::to_string(node->const_expr.int_val);
}
void IRGenerator::visitReturn(ASTNode *node)
{
    if (node->return_stmt.expr)
    {
        std::string ret_val = visit(node->return_stmt.expr);
        DataType expr_type = node->return_stmt.expr->data_type;
        std::string expr_llvm_type = get_llvm_type(expr_type);

        // 检查返回值的类型是否与函数声明的返回类型匹配
        if (expr_llvm_type != current_function_ret_type)
        {
            std::string op;
            if (expr_llvm_type == "i32" && current_function_ret_type == "float")
            {
                op = "sitofp";
            }
            else if (expr_llvm_type == "float" && current_function_ret_type == "i32")
            {
                op = "fptosi";
            }

            // 如果需要转换，则生成转换指令
            if (!op.empty())
            {
                std::string converted_val = new_temp_reg();
                ir_stream << "  " << converted_val << " = " << op << " " << expr_llvm_type << " " << ret_val << " to " << current_function_ret_type << "\n";
                ret_val = converted_val;
            }
        }
        ir_stream << "  ret " << current_function_ret_type << " " << ret_val << "\n";
    }
    else
    {
        ir_stream << "  ret void\n";
    }
    block_is_terminated = true;
}
std::string IRGenerator::visitFuncCall(ASTNode *node)
{
    std::string func_name = node->func_call.name;

    // --- 新增: 特殊处理 starttime 和 stoptime ---
    // 这两个函数被当作宏处理，直接调用外部函数并传入行号
    if (func_name == "starttime")
    {
        std::string line_no_str = std::to_string(node->line_no);
        ir_stream << "  call void @_sysy_starttime(i32 " << line_no_str << ")\n";
        node->data_type = TYPE_VOID; // 标记此节点为void类型
        return "";                   // 返回空字符串，因为它不产生值
    }
    else if (func_name == "stoptime")
    {
        std::string line_no_str = std::to_string(node->line_no);
        ir_stream << "  call void @_sysy_stoptime(i32 " << line_no_str << ")\n";
        node->data_type = TYPE_VOID; // 标记此节点为void类型
        return "";                   // 返回空字符串，因为它不产生值
    }

    std::vector<std::string> arg_vals;
    std::vector<std::string> final_arg_types;

    // --- 阶段 1: 获取函数期望的参数类型列表 ---
    std::vector<DataType> expected_param_data_types;
    if (is_sylib_function(func_name))
    {
        static const std::map<std::string, std::vector<DataType>> sylib_param_types = {
            {"putint", {TYPE_INT}},
            {"putch", {TYPE_INT}},
            {"putfloat", {TYPE_FLOAT}},
            {"putarray", {TYPE_INT}},
            {"putfarray", {TYPE_INT}},
        };
        if (sylib_param_types.count(func_name))
        {
            expected_param_data_types = sylib_param_types.at(func_name);
        }
    }
    else
    {
        assert(function_definitions.count(func_name) && "Function definition not found during call generation.");
        ASTNode *func_def_node = function_definitions.at(func_name);
        for (ASTNode *p = func_def_node->func_def.params; p; p = p->next)
        {
            expected_param_data_types.push_back(p->data_type);
        }
    }

    // --- 阶段 2: 访问参数，并进行必要的类型转换 ---
    ASTNode *arg_node = node->func_call.args;
    int arg_idx = 0;
    while (arg_node)
    {
        ASTNode *next_arg = arg_node->next;
        std::string arg_val = visit(arg_node);
        DataType actual_arg_type = arg_node->data_type;
        std::string actual_llvm_type = get_llvm_type(actual_arg_type);

        bool is_array_decay = false;
        if (arg_node->type == NODE_LVAL)
        {
            const ArrayInfo *info = find_array_info(arg_node->lval.name);
            if (info)
            {
                int num_indices = 0;
                for (ASTNode *p = arg_node->lval.indices; p; p = p->next)
                    num_indices++;
                if (num_indices < info->dimensions.size())
                {
                    is_array_decay = true;
                    // +++ 核心修复点: 在需要时才查找符号，确保sym在作用域内 +++
                    Symbol *sym = find_symbol(arg_node->lval.name);
                    assert(sym);
                    actual_llvm_type = get_llvm_type(sym->data_type) + "*";
                }
            }
        }

        if (!is_array_decay && arg_idx < expected_param_data_types.size())
        {
            DataType expected_type = expected_param_data_types[arg_idx];
            if (actual_arg_type != expected_type)
            {
                std::string expected_llvm_type = get_llvm_type(expected_type);
                std::string op;
                if (actual_arg_type == TYPE_FLOAT && expected_type == TYPE_INT)
                    op = "fptosi";
                else if (actual_arg_type == TYPE_INT && expected_type == TYPE_FLOAT)
                    op = "sitofp";

                if (!op.empty())
                {
                    std::string casted_val = new_temp_reg();
                    ir_stream << "  " << casted_val << " = " << op << " " << actual_llvm_type << " " << arg_val << " to " << expected_llvm_type << "\n";
                    arg_val = casted_val;
                    actual_llvm_type = expected_llvm_type;
                }
            }
        }

        arg_vals.push_back(arg_val);
        final_arg_types.push_back(actual_llvm_type);
        arg_node = next_arg;
        arg_idx++;
    }

    // --- 阶段 3 & 4: 构建并发出调用指令 ---
    std::string args_str;
    for (size_t i = 0; i < arg_vals.size(); ++i)
    {
        args_str += final_arg_types[i] + " " + arg_vals[i];
        if (i < arg_vals.size() - 1)
            args_str += ", ";
    }

    std::string ret_type;
    if (is_sylib_function(func_name))
    {
        ret_type = sylib_return_types.at(func_name);
        node->data_type = (ret_type == "float") ? TYPE_FLOAT : (ret_type == "void" ? TYPE_VOID : TYPE_INT);
    }
    else
    {
        Symbol *func_sym = find_symbol(func_name.c_str());
        assert(func_sym);
        ret_type = get_llvm_type(func_sym->data_type);
        node->data_type = func_sym->data_type;
    }

    if (ret_type == "void")
    {
        ir_stream << "  call void @" << func_name << "(" << args_str << ")\n";
        return "";
    }
    else
    {
        std::string result = new_temp_reg();
        ir_stream << "  " << result << " = call " << ret_type << " @" << func_name << "(" << args_str << ")\n";
        return result;
    }
}
void IRGenerator::visitIf(ASTNode *node)
{
    std::string cond_val = visit(node->if_stmt.cond);
    std::string cond_i1 = new_temp_reg();

    if (node->if_stmt.cond->data_type == TYPE_FLOAT)
    {
        ir_stream << "  " << cond_i1 << " = fcmp one float " << cond_val << ", 0.0\n";
    }
    else
    {
        ir_stream << "  " << cond_i1 << " = icmp ne i32 " << cond_val << ", 0\n";
    }

    std::string then_label = new_label("if.then");
    std::string else_label = new_label("if.else");
    std::string end_label = new_label("if.end");

    ir_stream << "  br i1 " << cond_i1 << ", label %" << then_label << ", label %" << (node->if_stmt.els ? else_label : end_label) << "\n";

    // --- then 块 ---
    ir_stream << "\n"
              << then_label << ":\n";
    block_is_terminated = false; // 重置状态，因为这是一个新的基本块
    enter_scope(node->if_stmt.then_scope->name, 1);
    enter_ir_scope();
    visit(node->if_stmt.then);
    exit_ir_scope();
    exit_scope();
    // 如果 then 块没有被 return/break/continue 终结，则需要一个分支跳转到 if.end
    if (!block_is_terminated)
    {
        ir_stream << "  br label %" << end_label << "\n";
    }

    // --- else 块 (如果存在) ---
    if (node->if_stmt.els)
    {
        ir_stream << "\n"
                  << else_label << ":\n";
        block_is_terminated = false; // 重置状态，因为这也是一个新的基本块
        enter_scope(node->if_stmt.els_scope->name, 1);
        enter_ir_scope();
        visit(node->if_stmt.els);
        exit_ir_scope();
        exit_scope();
        if (!block_is_terminated)
        {
            ir_stream << "  br label %" << end_label << "\n";
        }
    }

    // --- end 块 ---
    ir_stream << "\n"
              << end_label << ":\n";
    block_is_terminated = false; // 关键修复：重置状态，以允许 if 语句之后的代码继续生成
}
void IRGenerator::visitWhile(ASTNode *node)
{
    std::string cond_label = new_label("while.cond");
    std::string body_label = new_label("while.body");
    std::string end_label = new_label("while.end");

    loop_cond_labels.push_back(cond_label);
    loop_end_labels.push_back(end_label);

    ir_stream << "  br label %" << cond_label << "\n";
    ir_stream << "\n"
              << cond_label << ":\n";

    std::string cond_val = visit(node->while_stmt.cond);
    std::string cond_i1 = new_temp_reg();

    if (node->while_stmt.cond->data_type == TYPE_FLOAT)
    {
        ir_stream << "  " << cond_i1 << " = fcmp one float " << cond_val << ", 0.0\n";
    }
    else
    {
        ir_stream << "  " << cond_i1 << " = icmp ne i32 " << cond_val << ", 0\n";
    }

    ir_stream << "  br i1 " << cond_i1 << ", label %" << body_label << ", label %" << end_label << "\n";

    // --- body 块 ---
    ir_stream << "\n"
              << body_label << ":\n";
    block_is_terminated = false; // 重置状态，因为循环体是一个新的基本块
    enter_scope(node->while_stmt.body_scope->name, 1);
    enter_ir_scope();
    visit(node->while_stmt.body);
    exit_ir_scope();
    exit_scope();
    // 如果循环体没有被 break/continue/return 终结，则需要跳回条件检查
    if (!block_is_terminated)
    {
        ir_stream << "  br label %" << cond_label << "\n";
    }

    // --- end 块 ---
    ir_stream << "\n"
              << end_label << ":\n";
    block_is_terminated = false; // 关键修复：重置状态，以允许 while 循环之后的代码继续生成

    loop_cond_labels.pop_back();
    loop_end_labels.pop_back();
}
void IRGenerator::visitBreak(ASTNode *node)
{
    assert(!loop_end_labels.empty());
    ir_stream << "  br label %" << loop_end_labels.back() << "\n";
    // 关键修复：break 是一个终结者指令。
    block_is_terminated = true;
}
void IRGenerator::visitContinue(ASTNode *node)
{
    assert(!loop_cond_labels.empty());
    ir_stream << "  br label %" << loop_cond_labels.back() << "\n";
    // 关键修复：continue 也是一个终结者指令。
    block_is_terminated = true;
}
void IRGenerator::visitExprStmt(ASTNode *node) { visit(node->return_stmt.expr); }
std::string IRGenerator::visitInitVal(ASTNode *node)
{
    assert(node->init_val.expr);
    std::string result = visit(node->init_val.expr);

    // FIX: Propagate the data type from the actual expression node
    // to this wrapper node. This allows parent nodes in the AST (like a
    // type cast node) to correctly determine the type of this initializer.
    node->data_type = node->init_val.expr->data_type;

    return result;
}

void IRGenerator::findLocalDecls(ASTNode *node, std::vector<ASTNode *> &decls)
{
    if (!node)
        return;

    // 如果当前节点是一个声明，就将它加入列表
    if (node->type == NODE_VAR_DECL || node->type == NODE_ARRAY_DECL ||
        node->type == NODE_CONST_DECL || node->type == NODE_CONST_ARRAY_DECL)
    {
        // 使用find确保我们不重复添加同一个节点（尽管在此逻辑下不应该发生）
        if (std::find(decls.begin(), decls.end(), node) == decls.end())
        {
            decls.push_back(node);
        }
    }

    // 递归地进入可以包含声明或语句的子节点
    switch (node->type)
    {
    // 对于CompUnit和Block，遍历其子项链表
    case NODE_COMP_UNIT:
        for (ASTNode *item = node->comp.children; item; item = item->next)
            findLocalDecls(item, decls);
        break;
    case NODE_BLOCK:
        for (ASTNode *item = node->block.items; item; item = item->next)
            findLocalDecls(item, decls);
        break;
    // 对于IF和WHILE，递归进入其代码块
    case NODE_IF:
        findLocalDecls(node->if_stmt.then, decls);
        if (node->if_stmt.els)
            findLocalDecls(node->if_stmt.els, decls);
        break;
    case NODE_WHILE:
        findLocalDecls(node->while_stmt.body, decls);
        break;
    // 其他节点类型（如表达式、赋值等）不包含新的局部声明，故不处理
    default:
        break;
    }
}