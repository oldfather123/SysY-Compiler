#pragma once

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "ast.hpp"

#include <map>
#include <memory>
#include <vector>
#include <stack>

// 常量值结构体，用于存储编译时常量
struct const_val {
    int i_val;
    float f_val;
};

class Scope {
public:
    // 进入新作用域
    void enter() { 
        inner.emplace_back();
        const_map.emplace_back();
    }

    // 退出当前作用域
    void exit() { 
        inner.pop_back();
        const_map.pop_back();
    }

    // 判断是否在全局作用域
    bool in_global() { return inner.size() == 1; }

    // 添加变量到当前作用域
    bool push(const std::string& name, Value *val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    // 添加常量到当前作用域
    bool push(const std::pair<std::string, std::vector<unsigned int>>& name, const_val val) {
        auto result = const_map[const_map.size() - 1].insert({name, val});
        return result.second;
    }

    // 查找变量
    Value *find(const std::string& name) {
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }
        // 未找到返回nullptr
        return nullptr;
    }

    // 查找常量
    const_val find_const(const std::pair<std::string, std::vector<unsigned int>>& name) {
        for (auto s = const_map.rbegin(); s != const_map.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }
        // 未找到返回默认值
        return {0, 0.0f};
    }

private:
    // 变量符号表
    std::vector<std::map<std::string, Value *>> inner;
    // 常量符号表
    std::vector<std::map<std::pair<std::string, std::vector<unsigned int>>, const_val>> const_map;
};

// 数组初始化辅助类
class ArrayInitHelper {
public:
    ArrayInitHelper(const std::vector<int>& dims, Type* elem_type) 
        : dimensions(dims), element_type(elem_type) {
        total_size = 1;
        for (int d : dimensions) {
            total_size *= d;
        }
        current_pos = 0;
    }
    
    // 获取当前位置的索引
    std::vector<int> getCurrentIndices() {
        std::vector<int> indices;
        int temp = current_pos;
        for (auto it = dimensions.rbegin(); it != dimensions.rend(); ++it) {
            indices.insert(indices.begin(), temp % *it);
            temp /= *it;
        }
        return indices;
    }
    
    // 前进到下一个位置
    void advance() {
        current_pos++;
    }
    
    // 前进到下一个子数组的开始
    void advanceToNextSubarray(int level) {
    if (level >= dimensions.size()) return;
    
    // 计算当前级别的子数组大小
    int subarray_size = 1;
    for (int i = dimensions.size() - 1 - level; i < dimensions.size(); i++) {
        subarray_size *= dimensions[i];
    }
    
    // 对齐到下一个子数组边界
    int remainder = current_pos % subarray_size;
    if (remainder != 0) {
        current_pos += subarray_size - remainder;
    }
}
    
    bool isComplete() const {
        return current_pos >= total_size;
    }
    
    int getCurrentPosition() const {
        return current_pos;
    }
    
    int getTotalSize() const {
        return total_size;
    }
    
    Type* getElementType() const {
        return element_type;
    }

private:
    std::vector<int> dimensions;
    Type* element_type;
    int total_size;
    int current_pos;
};

class CminusfBuilder : public ASTVisitor {
public:
    CminusfBuilder() {
        module = std::make_unique<Module>();
        builder = std::make_unique<IRBuilder>(nullptr, module.get());
        auto *TyVoid = module->get_void_type();
        auto *TyInt32 = module->get_int32_type();
        auto *TyFloat = module->get_float_type();
        auto *TyInt32Ptr = module->get_int32_ptr_type();
        auto *TyFloatPtr = module->get_float_ptr_type();

        // 系统函数定义 - 与测试文件一致
        // int getint()
        auto *getint_type = FunctionType::get(TyInt32, {});
        auto *getint_fun = Function::create(getint_type, "getint", module.get());

        // void putint(int)
        std::vector<Type *> putint_params;
        putint_params.push_back(TyInt32);
        auto *putint_type = FunctionType::get(TyVoid, putint_params);
        auto *putint_fun = Function::create(putint_type, "putint", module.get());

        // void putch(int)
        std::vector<Type *> putch_params;
        putch_params.push_back(TyInt32);
        auto *putch_type = FunctionType::get(TyVoid, putch_params);
        auto *putch_fun = Function::create(putch_type, "putch", module.get());

        // int getch()
        auto *getch_type = FunctionType::get(TyInt32, {});
        auto *getch_fun = Function::create(getch_type, "getch", module.get());

        // void starttime() - 注意函数名与测试文件一致
        auto *starttime_type = FunctionType::get(TyVoid, {});
        auto *starttime_fun = Function::create(starttime_type, "starttime", module.get());

        // void stoptime()
        auto *stoptime_type = FunctionType::get(TyVoid, {});
        auto *stoptime_fun = Function::create(stoptime_type, "stoptime", module.get());

        // 其他可能用到的系统函数
        // float getfloat()
        auto *getfloat_type = FunctionType::get(TyFloat, {});
        auto *getfloat_fun = Function::create(getfloat_type, "getfloat", module.get());

        // void putfloat(float)
        std::vector<Type *> putfloat_params;
        putfloat_params.push_back(TyFloat);
        auto *putfloat_type = FunctionType::get(TyVoid, putfloat_params);
        auto *putfloat_fun = Function::create(putfloat_type, "putfloat", module.get());

        // int getarray(int a[])
        std::vector<Type *> getarray_params;
        getarray_params.push_back(TyInt32Ptr);
        auto *getarray_type = FunctionType::get(TyInt32, getarray_params);
        auto *getarray_fun = Function::create(getarray_type, "getarray", module.get());

        // void putarray(int n, int a[])
        std::vector<Type *> putarray_params;
        putarray_params.push_back(TyInt32);
        putarray_params.push_back(TyInt32Ptr);
        auto *putarray_type = FunctionType::get(TyVoid, putarray_params);
        auto *putarray_fun = Function::create(putarray_type, "putarray", module.get());

        // int getfarray(float a[])
        std::vector<Type *> getfarray_params;
        getfarray_params.push_back(TyFloatPtr);
        auto *getfarray_type = FunctionType::get(TyInt32, getfarray_params);
        auto *getfarray_fun = Function::create(getfarray_type, "getfarray", module.get());

        // void putfarray(int n, float a[])
        std::vector<Type *> putfarray_params;
        putfarray_params.push_back(TyInt32);
        putfarray_params.push_back(TyFloatPtr);
        auto *putfarray_type = FunctionType::get(TyVoid, putfarray_params);
        auto *putfarray_fun = Function::create(putfarray_type, "putfarray", module.get());

        // 兼容旧函数名
        // int input()
        auto *input_type = FunctionType::get(TyInt32, {});
        auto *input_fun = Function::create(input_type, "input", module.get());

        // void output(int)
        std::vector<Type *> output_params;
        output_params.push_back(TyInt32);
        auto *output_type = FunctionType::get(TyVoid, output_params);
        auto *output_fun = Function::create(output_type, "output", module.get());

        // void outputFloat(float)
        std::vector<Type *> output_float_params;
        output_float_params.push_back(TyFloat);
        auto *output_float_type = FunctionType::get(TyVoid, output_float_params);
        auto *output_float_fun = Function::create(output_float_type, "outputFloat", module.get());

        // void neg_idx_except()
        auto *neg_idx_except_type = FunctionType::get(TyVoid, {});
        auto *neg_idx_except_fun = Function::create(neg_idx_except_type, "neg_idx_except", module.get());

        scope.enter();
        // 新函数名
        scope.push("getint", getint_fun);
        scope.push("putint", putint_fun);
        scope.push("putch", putch_fun);
        scope.push("getch", getch_fun);
        scope.push("starttime", starttime_fun);
        scope.push("stoptime", stoptime_fun);
        scope.push("getfloat", getfloat_fun);
        scope.push("putfloat", putfloat_fun);
        scope.push("getarray", getarray_fun);
        scope.push("putarray", putarray_fun);
        scope.push("getfarray", getfarray_fun);
        scope.push("putfarray", putfarray_fun);
        // 旧函数名
        scope.push("input", input_fun);
        scope.push("output", output_fun);
        scope.push("outputFloat", output_float_fun);
        scope.push("neg_idx_except", neg_idx_except_fun);
    }

    std::unique_ptr<Module> getModule() { return std::move(module); }

private:
    // Program和CompUnit
    virtual Value *visit(ASTProgram &) override final;
    virtual Value *visit(ASTConstDecl &) override final;
    virtual Value *visit(ASTConstDef &) override final;
    virtual Value *visit(ASTConstInitVal &) override final;
    virtual Value *visit(ASTVarDecl &) override final;
    virtual Value *visit(ASTVarDef &) override final;
    virtual Value *visit(ASTInitVal &) override final;
    virtual Value *visit(ASTFuncDef &) override final;
    virtual Value *visit(ASTFuncFParam &) override final;
    virtual Value *visit(ASTBlock &) override final;
    
    // 语句
    virtual Value *visit(ASTAssignStmt &) override final;
    virtual Value *visit(ASTExpStmt &) override final;
    virtual Value *visit(ASTBlockStmt &) override final;
    virtual Value *visit(ASTIfStmt &) override final;
    virtual Value *visit(ASTWhileStmt &) override final;
    virtual Value *visit(ASTBreakStmt &) override final;
    virtual Value *visit(ASTContinueStmt &) override final;
    virtual Value *visit(ASTReturnStmt &) override final;
    
    // 表达式
    virtual Value *visit(ASTExp &) override final;
    virtual Value *visit(ASTCond &) override final;
    virtual Value *visit(ASTLVal &) override final;
    virtual Value *visit(ASTParenExp &) override final;
    virtual Value *visit(ASTLValExp &) override final;
    virtual Value *visit(ASTNumber &) override final;
    virtual Value *visit(ASTPrimaryUnaryExp &) override final;
    virtual Value *visit(ASTCallExp &) override final;
    virtual Value *visit(ASTUnaryOpExp &) override final;
    virtual Value *visit(ASTFuncRParams &) override final;
    virtual Value *visit(ASTMulExp &) override final;
    virtual Value *visit(ASTAddExp &) override final;
    virtual Value *visit(ASTRelExp &) override final;
    virtual Value *visit(ASTEqExp &) override final;
    virtual Value *visit(ASTLAndExp &) override final;
    virtual Value *visit(ASTLOrExp &) override final;
    virtual Value *visit(ASTConstExp &) override final;

    // 辅助函数
    bool promote(Value **l_val_p, Value **r_val_p, const_val *l_num, const_val *r_num);
    Constant *get_const_array(Type *array_type_t, std::vector<const_val> val);
    Value *cast_value(Value *val, Type *target_type);
    std::map<std::string, std::vector<int>> array_param_dims;
    
    // 新增辅助函数
    bool can_insert_instruction();
    void ensure_valid_insert_point();
    
    // 处理数组初始化
    void process_array_init_val(ASTInitVal &node, ArrayInitHelper &helper, 
                              std::vector<Value*> &init_vals, 
                              std::vector<const_val> &const_init_vals,
                              bool is_const);
    void process_const_array_init_val(ASTConstInitVal &node, ArrayInitHelper &helper);

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;

    // 上下文信息
    struct {
        // 当前正在构建的函数
        Function *func = nullptr;
        // 循环相关的基本块
        BasicBlock *loop_cond = nullptr;  // 循环条件块
        BasicBlock *loop_body = nullptr;  // 循环体块
        BasicBlock *loop_end = nullptr;   // 循环结束块
        // 用于常量表达式求值
        bool in_const_exp = false;
        // 条件表达式上下文
        bool in_cond = false;
        // 当前值（用于表达式求值）
        Value *cur_val = nullptr;
        // 当前类型（用于变量声明）
        Type *cur_type = nullptr;
        // 左值标记
        bool require_lvalue = false;
        // 常量值
        const_val val = {0, 0.0f};
        // 数组初始化相关
        std::vector<Value *> array_init_vals;
        std::vector<const_val> const_array_init_vals;
        std::vector<int> array_dims;
        bool in_function_call = false;
        Type *expected_arg_type = nullptr;
        // 数组初始化助手
        ArrayInitHelper* array_init_helper = nullptr;
    } context;
};