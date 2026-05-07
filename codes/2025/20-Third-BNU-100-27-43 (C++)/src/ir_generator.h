#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <set>

// extern "C"
// {
#include "symbol.h"
// }

// 向前声明C语言的结构体
struct ASTNode;

struct ArrayInfo
{
    std::vector<int> dimensions;
    bool is_param;
    // 如果需要，还可以添加其他信息，如是否是参数等
};

class IRGenerator
{
public:
    IRGenerator();

    // 从AST根节点开始生成IR的主入口
    void generate(ASTNode *root);

    // 获取已生成的IR代码字符串
    std::string getIR();

private:
    std::map<ASTNode *, std::string> local_var_allocas;
    void findLocalDecls(ASTNode *node, std::vector<ASTNode *> &decls);

    // --- 状态管理 ---
    std::stringstream ir_stream;
    int temp_reg_counter;
    int label_counter;
    std::vector<std::map<std::string, std::string>> scoped_named_values;
    std::vector<std::map<std::string, ArrayInfo>> scoped_array_info_cache;
    std::string current_function_ret_type;

    // 用于处理 break 和 continue 的标签栈
    std::vector<std::string> loop_cond_labels;
    std::vector<std::string> loop_end_labels;

    // --- Sylib 库函数管理 ---
    std::map<std::string, std::string> sylib_return_types;
    std::map<std::string, std::string> sylib_declarations;
    bool is_sylib_function(const std::string &name);
    void generate_sylib_declarations();

    // +++ fundamental fix: new helper functions for scope management +++
    void enter_ir_scope();
    void exit_ir_scope();
    std::string find_named_value(const std::string &name);
    const ArrayInfo *find_array_info(const std::string &name);

    // --- 辅助方法 ---
    std::string new_temp_reg();
    std::string new_label(const std::string &prefix);
    std::string get_llvm_type(DataType dt);
    std::string get_llvm_op_name(ASTNode *node);
    double evaluate_float_constant_expression(ASTNode *node);
    int evaluate_constant_expression(ASTNode *node);
    std::string generate_constant_array_initializer_string(
        ASTNode *init_node, const std::vector<int> &dimensions, const std::string &base_type_str);
    void generate_array_initialization_recursive(
        const std::string &array_ptr,
        const std::string &full_array_type,
        const std::string &base_type_str,
        ASTNode **current_item_ptr, // Use pointer-to-pointer to advance through the list
        const std::vector<int> &dimensions,
        int dim_level,
        int &current_flat_index);

    void generate_array_initialization(
        const std::string &array_ptr,
        ASTNode *init_node,
        const std::vector<int> &dimensions,
        const std::string &base_type_str,
        const std::string &full_array_type);
    int total_elements(const std::vector<int> &dimensions);
    std::vector<int> get_multi_dim_indices(int flat_index, const std::vector<int> &dimensions);
    void store_array_element(
        const std::string &array_ptr,
        const std::string &full_array_type,
        const std::string &base_type_str,
        const std::vector<int> &dimensions,
        int flat_index,
        const std::string &value_to_store);

    // --- AST遍历 (Visitor) 方法 ---
    std::string visit(ASTNode *node);

    // 针对不同节点类型的具体visit方法
    void visitCompUnit(ASTNode *node);
    void visitFuncDef(ASTNode *node);
    void visitVarDecl(ASTNode *node, bool is_local);
    void visitBlock(ASTNode *node, bool is_local);
    std::string visitBinOp(ASTNode *node);
    std::string visitUnaryOp(ASTNode *node);
    std::string visitLVal(ASTNode *node);
    std::string visitAssign(ASTNode *node);
    std::string visitConstExpr(ASTNode *node);
    void visitReturn(ASTNode *node);
    std::string visitFuncCall(ASTNode *node);
    void visitIf(ASTNode *node);
    void visitWhile(ASTNode *node);
    void visitExprStmt(ASTNode *node);
    void visitBreak(ASTNode *node);
    void visitContinue(ASTNode *node);
    std::string visitInitVal(ASTNode *node);

    std::string visitLVal_as_pointer(ASTNode *node);

    bool block_is_terminated;

    std::string float_const_declarations;
    std::map<double, std::string> float_globals;
    int float_global_counter;

    std::map<std::string, ASTNode *> function_definitions;
};

#endif // IR_GENERATOR_H