// m
#pragma once

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "ast.hpp"
#include "logging.hpp"

#include <map>
#include <memory>
#include <stack>

class Scope
{
public:
    // enter a new scope
    void enter() { inner.emplace_back(); }

    // exit a scope
    void exit() { inner.pop_back(); }

    bool in_global() { return inner.size() == 1; }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(const std::string &name, Value *val, bool is_const_decl = false)
    {
        auto result = inner[(int)inner.size() - 1].insert(
            {name, std::make_pair(val, is_const_decl)});
        return result.second;
    }

    std::pair<Value *, bool> find(const std::string &name)
    {
        for (auto s = inner.rbegin(); s != inner.rend(); s++)
        {
            auto iter = s->find(name);
            if (iter != s->end())
            {
                return iter->second;
            }
        }

        // Name not found: handled here?
        ASSERT(false && "Name not found in scope");
        return std::make_pair(nullptr, false);
    }

private:
    std::vector<std::map<std::string, std::pair<Value *, bool>>> inner;
};

// struct InitValCalc {
//     Type *type;
//     std::vector<int> array_size;
//     std::vector<int> suffix_product;
//     std::vector<Value *> vals;
//     Value *single_val;
//     int cur_idx = 0;
//     bool is_const;
//     bool is_single_exp;
//     InitValCalc(Module *module, IRBuilder *builder, std::vector<int> array_size,
//                 Type *type, bool is_const, bool is_single_exp) {
//         this->array_size = array_size;
//         suffix_product = array_size;
//         for (int i = (int)array_size.size() - 2; i >= 0; i--) {
//             suffix_product[i] = suffix_product[i] * suffix_product[i + 1];
//         }
//         if (suffix_product.size() != 0)
//             vals = std::vector<Value *>(suffix_product[0],
//                                         ConstantInt::get(0, module));
//         this->type = type;
//         this->is_const = is_const;
//         this->is_single_exp = is_single_exp;
//         cur_idx = 0;
//         single_val = nullptr;
//     }
//     bool is_single_val() { return is_single_exp; }
//     Constant *get_const_value(Module *module);
//     void store_value(Module *module, IRBuilder *builder, AllocaInst *alloca_inst, Scope &scope);
// };

class SyBuilder : public ASTVisitor
{
public:
    SyBuilder()
    {
        module = std::make_unique<Module>();
        builder = std::make_unique<IRBuilder>(nullptr, module.get());

        auto *TyVoid = module->get_void_type();
        auto *TyInt32 = module->get_int32_type();
        auto *TyFloat = module->get_float_type();
        auto *TyInt32Ptr = module->get_int32_ptr_type();
        auto *TyFloatPtr = module->get_float_ptr_type();
        auto *TyInt8Ptr = module->get_int8_ptr_type();
        auto *Tyint64 = module->get_int64_type();

        auto *getint_type = FunctionType::get(TyInt32, {});
        auto *getint_fun =
            Function::create(getint_type, "getint", module.get());

        auto *getch_type = FunctionType::get(TyInt32, {});
        auto *getch_fun = Function::create(getch_type, "getch", module.get());

        auto *getfloat_type = FunctionType::get(TyFloat, {});
        auto *getfloat_fun =
            Function::create(getfloat_type, "getfloat", module.get());

        auto *getarray_type = FunctionType::get(TyInt32, {TyInt32Ptr});
        auto *getarray_fun =
            Function::create(getarray_type, "getarray", module.get());

        auto *getfarray_type = FunctionType::get(TyInt32, {TyFloatPtr});
        auto *getfarray_fun =
            Function::create(getfarray_type, "getfarray", module.get());

        auto *putint_type = FunctionType::get(TyVoid, {TyInt32});
        auto *putint_fun =
            Function::create(putint_type, "putint", module.get());

        auto *putch_type = FunctionType::get(TyVoid, {TyInt32});
        auto *putch_fun = Function::create(putch_type, "putch", module.get());

        auto *putfloat_type = FunctionType::get(TyVoid, {TyFloat});
        auto *putfloat_fun =
            Function::create(putfloat_type, "putfloat", module.get());

        auto *putarray_type = FunctionType::get(TyVoid, {TyInt32, TyInt32Ptr});
        auto *putarray_fun =
            Function::create(putarray_type, "putarray", module.get());

        auto *putfarray_type = FunctionType::get(TyVoid, {TyInt32, TyFloatPtr});
        auto *putfarray_fun =
            Function::create(putfarray_type, "putfarray", module.get());

        // auto *systarttime_type = FunctionType::get(TyVoid, {TyInt32});
        // auto *systarttime_fun =
        //     Function::create(systarttime_type, "_sysy_starttime", module.get());

        // auto *systoptime_type = FunctionType::get(TyVoid, {TyInt32});
        // auto *systoptime_fun =
        //     Function::create(systoptime_type, "_sysy_stoptime", module.get());

        auto *memset_type = FunctionType::get(TyVoid, {TyInt8Ptr, TyInt32, Tyint64});
        auto *memset_fun =
            Function::create(memset_type, "memset", module.get());

        auto *starttime_type = FunctionType::get(TyVoid, {});
        auto *starttime_fun =
            Function::create(starttime_type, "_sysy_starttime", module.get());

        auto *stoptime_type = FunctionType::get(TyVoid, {});
        auto *stoptime_fun =
            Function::create(stoptime_type, "_sysy_stoptime", module.get());

        scope.enter();
        scope.push("getint", getint_fun);
        scope.push("getch", getch_fun);
        scope.push("getfloat", getfloat_fun);
        scope.push("getarray", getarray_fun);
        scope.push("getfarray", getfarray_fun);
        scope.push("putint", putint_fun);
        scope.push("putch", putch_fun);
        scope.push("putfloat", putfloat_fun);
        scope.push("putarray", putarray_fun);
        scope.push("putfarray", putfarray_fun);
        // scope.push("_sysy_starttime", systarttime_fun);
        // scope.push("_sysy_stoptime", systoptime_fun);
        scope.push("memset", memset_fun);
        scope.push("starttime", starttime_fun);
        scope.push("stoptime", stoptime_fun);
    }
    std::vector<Value *> process_function_args(const std::vector<ASTExp *> &arguments, Function *function);
    Value *convert_argument_type(Value *arg_val, Type *expected_type);
    Value *eval_and_record(Value *val, const_val &num);
    void store2scope(Module *module, IRBuilder *builder,
                     AllocaInst *alloca_inst, Scope &scope);
    Constant *put_const(Module *module);
    void set(Module *module, IRBuilder *builder, std::vector<int> array_size);
    void bascibranch(BasicBlock *bb, BasicBlock *nextBB, ASTStmt *stmt);
    void createbranch(BasicBlock *targetBB);
    Value *trans_returntype(Value *ret_val, Type *fun_ret_type);
    std::unique_ptr<Module> getModule() { return std::move(module); }

private:
    virtual Value *visit(ASTProgram &) override final;
    // virtual Value *visit(ASTMainDef &) override final;
    virtual Value *visit(ASTFuncDef &) override final;
    virtual Value *visit(ASTDecl &) override final;
    virtual Value *visit(ASTConstDef &) override final;
    virtual Value *visit(ASTVarDef &) override final;
    virtual Value *visit(ASTParam &) override final;
    virtual Value *visit(ASTBlock &) override final;
    virtual Value *visit(ASTAssignStmt &) override final;
    virtual Value *visit(ASTCompoundAssignStmt &) override final;
    virtual Value *visit(ASTSelectionStmt &) override final;
    virtual Value *visit(ASTIterationStmt &) override final;
    virtual Value *visit(ASTBreak &) override final;
    virtual Value *visit(ASTContinue &) override final;
    virtual Value *visit(ASTReturnStmt &) override final;
    virtual Value *visit(ASTExp &) override final;
    virtual Value *visit(ASTVar &) override final;
    virtual Value *visit(ASTNum &) override final;
    virtual Value *visit(ASTUnaryExp &) override final;
    virtual Value *visit(ASTFuncExp &) override final;
    virtual Value *visit(ASTAddExp &) override final;
    virtual Value *visit(ASTMulExp &) override final;
    virtual Value *visit(ASTRelExp &) override final;
    virtual Value *visit(ASTEqExp &) override final;
    virtual Value *visit(ASTAndExp &) override final;
    virtual Value *visit(ASTOrExp &) override final;
    virtual Value *visit(ASTInitVal &) override final;

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;
    bool promote(Value **l_val_p, Value **r_val_p, const_val *l_num, const_val *r_num);

    struct
    {
        // function that is being built
        Function *func = nullptr;
        Type *type_return = nullptr;
        bool is_const_exp = false;
        bool is_const = false;
        bool is_func_block = false;
        bool is_array = false;
        // std::shared_ptr<InitValCalc> init_val_calc;

        Type *type;
        std::vector<int> array_size;
        std::vector<int> suffix_product;
        std::vector<Value *> vals;
        Value *single_val;
        int cur_idx = 0;
        bool is_single_exp;

        const_val val;

        bool require_lvalue = false;
        std::stack<BasicBlock *> next_bb_stk;
        std::stack<BasicBlock *> cond_bb_stk;

        // 用于处理标号回填
        std::stack<BasicBlock *> true_bb_stk;
        std::stack<BasicBlock *> false_bb_stk;
        bool unary_op = true;
        bool is_not_op = true;
        bool pre_enter_scope = false;
        // TODO: you should add more fields to store state
    } context;
};
