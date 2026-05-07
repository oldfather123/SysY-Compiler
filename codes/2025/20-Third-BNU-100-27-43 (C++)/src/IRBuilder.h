#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>

#include "IR.h"

// extern "C"
// {
#include "ast.h"
#include "symbol.h"
// }

struct ArrayInfo_IRBuilder
{
    std::vector<int> dimensions;
    bool is_param;
};

class IRBuilder
{
public:
    IRBuilder();
    std::unique_ptr<MyIR::IRUnit> build(ASTNode *root);
    std::string getIR(const MyIR::IRUnit &unit);

private:
    // --- Builder State ---
    std::unique_ptr<MyIR::IRUnit> unit_;
    MyIR::Function *current_function_;
    MyIR::BasicBlock *current_block_;

    int temp_reg_counter_;
    int label_counter_;

    std::vector<std::map<std::string, MyIR::Value *>> scoped_values_;
    std::vector<std::map<std::string, ArrayInfo_IRBuilder>> scoped_array_info_cache_;

    std::vector<MyIR::BasicBlock *> loop_cond_blocks_;
    std::vector<MyIR::BasicBlock *> loop_end_blocks_;

    // --- State to match IRGenerator ---
    std::string float_const_declarations_;
    std::map<double, MyIR::GlobalVariable *> float_globals_;
    int float_global_counter_;
    bool block_is_terminated_;
    std::string current_function_ret_type_;

    std::map<std::string, ASTNode *> function_definitions_;
    std::map<ASTNode *, MyIR::AllocaInst *> local_var_allocas_;
    std::map<std::string, std::vector<DataType>> sylib_param_types_;

    // --- Builder Helpers ---
    void initialize();
    MyIR::Value *visit(ASTNode *node);

    void declare_sylib_functions();

    void visitCompUnit(ASTNode *node);
    void visitFuncDef(ASTNode *node);
    void visitVarDecl(ASTNode *node, bool is_local);
    void visitBlock(ASTNode *node);
    MyIR::Value *visitBinOp(ASTNode *node);
    MyIR::Value *visitUnaryOp(ASTNode *node);
    MyIR::Value *visitLVal(ASTNode *node);
    MyIR::Value *visitAssign(ASTNode *node);
    MyIR::Value *visitConstExpr(ASTNode *node);
    void visitReturn(ASTNode *node);
    MyIR::Value *visitFuncCall(ASTNode *node);
    void visitIf(ASTNode *node);
    void visitWhile(ASTNode *node);
    void visitExprStmt(ASTNode *node);
    void visitBreak(ASTNode *node);
    void visitContinue(ASTNode *node);
    MyIR::Value *visitInitVal(ASTNode *node);
    MyIR::Value *visitTypeCast(ASTNode *node);
    MyIR::Value *visitConstInit(ASTNode *node);

    MyIR::Value *visitLVal_as_pointer(ASTNode *node);
    void findLocalDecls(ASTNode *node, std::vector<ASTNode *> &decls);

    void enter_ir_scope();
    void exit_ir_scope();
    MyIR::Value *find_value(const std::string &name);
    const ArrayInfo_IRBuilder *find_array_info(const std::string &name);

    std::shared_ptr<MyIR::Type> get_myir_type(DataType dt);
    std::shared_ptr<MyIR::Type> get_myir_type_from_dims(DataType base_dt, const std::vector<int> &dims);

    std::string new_temp_reg_name();
    std::string new_label_name(const std::string &prefix);

    double evaluate_float_constant_expression(ASTNode *node);
    int evaluate_constant_expression(ASTNode *node);

    std::shared_ptr<MyIR::Constant> generate_constant_array_initializer(ASTNode *init_node, const std::vector<int> &dimensions, DataType base_dt);
    void generate_array_initialization(MyIR::Value *array_ptr, ASTNode *init_node, const std::vector<int> &dimensions, std::shared_ptr<MyIR::Type> base_type);

    // --- Printer State ---
    std::stringstream ir_stream_;
    std::map<const MyIR::Value *, std::string> value_names_;
    int print_temp_counter_;

    // --- Printer Helpers ---
    void print(const MyIR::IRUnit &unit);
    void print_global_variable(const MyIR::GlobalVariable &gv);
    void print_function(const MyIR::Function &func);
    void print_basic_block(const MyIR::BasicBlock &bb);
    void print_instruction(const MyIR::Instruction &inst);
    std::string get_value_name(const MyIR::Value *val);
    std::string get_type_string(const MyIR::Type *type);
    std::string get_const_string(const MyIR::Constant *c);
};

#endif // IR_BUILDER_H