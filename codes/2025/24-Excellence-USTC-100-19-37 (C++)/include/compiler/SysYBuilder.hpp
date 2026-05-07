#pragma once

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "ast.hpp"

#include <map>
#include <memory>
#include <stack>

class Scope {
  public:
    // enter a new scope
    void enter() {
        inner.emplace_back();
        const_map.emplace_back();
    }

    // exit a scope
    void exit() {
        inner.pop_back(); 
        const_map.pop_back();
    }

    bool in_global() { return inner.size() == 1; }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(const std::string & name, Value * val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    std::pair<int, Value *> find(const std::string & name) {
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return {std::distance(inner.begin(), s.base() - 1), iter->second};
            }
        }

        // Name not found: handled here?
        assert(false && "Name not found in scope");

        return {0, nullptr};
    }

    bool push(const std::string & name, const std::vector<int> & indices, Constant * val) {
        auto result = const_map[const_map.size() - 1].insert({{name, indices}, val});
        return result.second;
    }

    std::pair<int, Constant *> find(std::string name, std::vector<Value *> indices) {
        std::vector<int> target_indices;
        for (auto &index : indices) {
            auto index_int = dynamic_cast<ConstantInt *>(index);
            if (index_int) {
                target_indices.push_back(index_int->get_value());
            } else {
                return {-1, nullptr};
            }
        }

        for (auto s = const_map.rbegin(); s != const_map.rend(); s++) {
            auto iter = s->find(std::pair<std::string, std::vector<int>>(name, target_indices));
            if (iter != s->end()) {
                return {std::distance(const_map.begin(), s.base() - 1), iter->second};
            }
        }

        // Name not found: handled here?
        // assert(false && "Name not found in const_map");

        return {-1, nullptr};
    }

  private:
    std::vector<std::map<std::string, Value *>> inner;
    std::vector<std::map<std::pair<std::string, std::vector<int>>, Constant *>> const_map;
};

class SysYBuilder : public ASTVisitor {
  public:
    SysYBuilder() {
        module = std::make_unique<Module>();
        builder = std::make_unique<IRBuilder>(nullptr, module.get());
        auto *Void = module->get_void_type();
        auto *Int = module->get_int32_type();
        auto *Float = module->get_float_type();
        auto *IntPtr = module->get_int32_ptr_type();
        auto *FloatPtr = module->get_float_ptr_type();
        auto *Int64 = module->get_int64_type();
        auto *Int8Ptr = module->get_int8_ptr_type();

        auto *getint_type = FunctionType::get(Int, {});
        auto *getint = Function::create(getint_type, "getint", module.get());
        auto *getch_type = FunctionType::get(Int, {});
        auto *getch = Function::create(getch_type, "getch", module.get());
        auto *getarray_type = FunctionType::get(Int, {IntPtr});
        auto *getarray = Function::create(getarray_type, "getarray", module.get());
        auto *getfloat_type = FunctionType::get(Float, {});
        auto *getfloat = Function::create(getfloat_type, "getfloat", module.get());
        auto *getfarray_type = FunctionType::get(Int, {FloatPtr});
        auto *getfarray = Function::create(getfarray_type, "getfarray", module.get());

        auto *putint_type = FunctionType::get(Void, {Int});
        auto *putint = Function::create(putint_type, "putint", module.get());
        auto *putch_type = FunctionType::get(Void, {Int});
        auto *putch = Function::create(putch_type, "putch", module.get());
        auto *putarray_type = FunctionType::get(Void, {Int, IntPtr});
        auto *putarray = Function::create(putarray_type, "putarray", module.get());
        auto *putfloat_type = FunctionType::get(Void, {Float});
        auto *putfloat = Function::create(putfloat_type, "putfloat", module.get());
        auto *putfarray_type = FunctionType::get(Void, {Int, FloatPtr});
        auto *putfarray = Function::create(putfarray_type, "putfarray", module.get());
        auto *putf_type = FunctionType::get(Void, {IntPtr});
        auto *putf = Function::create(putf_type, "putf", module.get());

        auto *memset_type = FunctionType::get(Void, {Int8Ptr, Int, Int64});
        auto *memset = Function::create(memset_type, "memset", module.get());
        auto *starttime_type = FunctionType::get(Void, {Int});
        auto *starttime = Function::create(starttime_type, "_sysy_starttime", module.get());
        auto *stoptime_type = FunctionType::get(Void, {Int});
        auto *stoptime = Function::create(stoptime_type, "_sysy_stoptime", module.get());

        scope.enter();
        scope.push("getint", getint);
        scope.push("getch", getch);
        scope.push("getarray", getarray);
        scope.push("getfloat", getfloat);
        scope.push("getfarray", getfarray);
        scope.push("putint", putint);
        scope.push("putch", putch);
        scope.push("putarray", putarray);
        scope.push("putfloat", putfloat);
        scope.push("putfarray", putfarray);
        scope.push("putf", putf);
        scope.push("memset", memset);
        scope.push("_sysy_starttime", starttime);
        scope.push("_sysy_stoptime", stoptime);
    }

    std::unique_ptr<Module> getModule() { return std::move(module); }

  private:
    virtual Value * visit(ASTProgram &) override final;
    virtual Value * visit(ASTAddExp &) override final;
    virtual Value * visit(ASTAssignStmt &) override final;
    virtual Value * visit(ASTBlockItem &) override final;
    virtual Value * visit(ASTBlockStmt &) override final;
    virtual Value * visit(ASTBreakStmt &) override final;
    virtual Value * visit(ASTCall &) override final;
    virtual Value * visit(ASTContinueStmt &) override final;
    virtual Value * visit(ASTDecl &) override final;
    virtual Value * visit(ASTDef &) override final;
    virtual Value * visit(ASTEqExp &) override final;
    virtual Value * visit(ASTFuncDef &) override final;
    virtual Value * visit(ASTFuncParam &) override final;
    virtual Value * visit(ASTInitVal &) override final;
    virtual Value * visit(ASTIterationStmt &) override final;
    virtual Value * visit(ASTLAndExp &) override final;
    virtual Value * visit(ASTLOrExp &) override final;
    virtual Value * visit(ASTLVal &) override final;
    virtual Value * visit(ASTMulExp &) override final;
    virtual Value * visit(ASTNum &) override final;
    virtual Value * visit(ASTParenthesizedExp &) override final;
    virtual Value * visit(ASTRelExp &) override final;
    virtual Value * visit(ASTReturnStmt &) override final;
    virtual Value * visit(ASTSelectionStmt &) override final;
    virtual Value * visit(ASTUnaryExp &) override final;
    virtual Value * visit(ASTExpStmt &) override final;

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;
    struct condList {
        Value * val = nullptr;
        BasicBlock * myBB = nullptr;
        BasicBlock * trueBB = nullptr;
        BasicBlock * falseBB = nullptr;
    };
    struct {
        // function that is being built
        Function * func = nullptr;
        Value * val = nullptr;
        bool iffunc = false;
        bool islval = false;//是否要给左值赋值
        // TODO: you should add more fields to store state
        bool is_const = false; // 是否是常量
        bool is_const_exp = false; // 是否是常量表达式
        SysYType type = SysYType::TYPE_INT; // 当前类型
        
        std::vector<int> init_val_counts;
        int depth = 0; // 用于处理嵌套的数组初始化
        std::vector<Value *> init_vals; // 用于存储数组初始化的值

        ArrayType* array_type = nullptr; // 当前数组类型

        std::stack<BasicBlock *> beginwhile;
        std::stack<BasicBlock *> endwhile;
        std::vector<condList> list;

        std::vector<Instruction *> breakir;
        
        std::stack<BasicBlock *> while_entry_bbs; // 用于存储while的入口基本块
        std::stack<BasicBlock *> while_next_bbs; // 用于存储while的下一个基本块

        bool isfirstLandexp;
        bool isFirstFuncBB = false;  // 是否是第一个函数基本块
        BasicBlock * firstBrBB = nullptr; // 第一个函数基本块
        bool is_all_zero = true; // 是否所有的初始化值都是0

        int cond_count = 0; // 用于处理条件表达式的计数
        int next_count = 0; // 用于处理下一个基本块的计数
        int true_count = 0; // 用于处理条件表达式的计数
        int false_count = 0; // 用于处理条件表达式的计数
        int else_count = 0;
        std::set<BasicBlock *> next_set;
        bool has_break_or_continue = false; // 是否有break或continue语句

        std::vector<std::pair<BasicBlock *, BasicBlock *>> while_info;
    } context;
};
