#pragma once
#include "SysyBaseVisitor.h"
#include "SysyParser.h"
#include "IR.h"
#include "ast.h"
#define VOID_T (module->void_ty_)
#define INT1_T (module->int1_ty_)
#define INT32_T  (module->int32_ty_)
#define FLOAT_T  (module->float32_ty_)
namespace IR {
    class IRScope {
    public:
        void enter() { scope_content.push_back({}); }
        void exit() { scope_content.pop_back(); }
        bool global_scope() { return scope_content.size() == 1; }
        bool push(std::string name, Value *val) {
            bool result;
            result = (scope_content[scope_content.size() - 1].insert({name, val})).second;
            return result;
        }
        Value* find(std::string name) {
            for (auto id = scope_content.rbegin(); id != scope_content.rend(); id++) {
                auto iter = id->find(name);
                if (iter != id->end())
                    return iter->second;
            }
            return nullptr; 
        }
    private:
        std::vector<std::map<std::string, Value *>> scope_content;
    };
    class IRVisitor : public SysyBaseVisitor {
    public:
        [[nodiscard]] std::unique_ptr<Module> getModule();
        std::unique_ptr<Module> module; 
        IRScope scope;
        IRStmtBuilder *builder;
        void visit(frontend::ast::CompileUnit &ast);
        void visit(frontend::ast::Declaration &ast);
        void visit(frontend::ast::Function &ast);
        void visit(frontend::ast::Initializer &ast);
        void visit(frontend::ast::Parameter &ast);
        void visit(frontend::ast::Block &ast);
        void visit(frontend::ast::Statement &ast);
        void visit(frontend::ast::Return &ast);
        void visit(frontend::ast::ExprStmt &ast);
        void visit(frontend::ast::Expression &ast);

        IRVisitor()
        {
            module = std::unique_ptr<Module>(new Module());
            builder = new IRStmtBuilder(nullptr, module.get());

            auto TyInt32Ptr = module->get_pointer_IRType(module->int32_ty_);
            auto TyFloatPtr = module->get_pointer_IRType(module->float32_ty_);

            auto input_type = new FunctionIRType(INT32_T, {});
            auto get_int = new ::Function(input_type, "getint", module.get());

            input_type = new FunctionIRType(FLOAT_T, {});
            auto get_float = new ::Function(input_type, "getfloat", module.get());

            input_type = new FunctionIRType(INT32_T, {});
            auto get_char = new ::Function(input_type, "getch", module.get());

            std::vector<IRType *> input_params;
            std::vector<IRType *>().swap(input_params);
            input_params.push_back(TyInt32Ptr);
            input_type = new FunctionIRType(INT32_T, input_params);
            auto get_int_array = new ::Function(input_type, "getarray", module.get());

            std::vector<IRType *>().swap(input_params);
            input_params.push_back(TyFloatPtr);
            input_type = new FunctionIRType(INT32_T, input_params);
            auto get_float_array = new ::Function(input_type, "getfarray", module.get());

            std::vector<IRType *> output_params;
            std::vector<IRType *>().swap(output_params);
            output_params.push_back(INT32_T);
            auto output_type = new FunctionIRType(VOID_T, output_params);
            auto put_int = new ::Function(output_type, "putint", module.get());

            std::vector<IRType *>().swap(output_params);
            output_params.push_back(FLOAT_T);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto put_float = new ::Function(output_type, "putfloat", module.get());

            std::vector<IRType *>().swap(output_params);
            output_params.push_back(INT32_T);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto put_char = new ::Function(output_type, "putch", module.get());

            std::vector<IRType *>().swap(output_params);
            output_params.push_back(INT32_T);
            output_params.push_back(TyInt32Ptr);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto put_int_array = new ::Function(output_type, "putarray", module.get());

            std::vector<IRType *>().swap(output_params);
            output_params.push_back(INT32_T);
            output_params.push_back(TyFloatPtr);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto put_float_array = new ::Function(output_type, "putfarray", module.get());

            output_params.clear();
            output_params.push_back(INT32_T);
            auto time_type = new FunctionIRType(VOID_T, output_params);
            auto sysy_start_time = new ::Function(time_type, "starttime", module.get());
            auto sysy_stop_time = new ::Function(time_type, "stoptime", module.get());

            output_params.clear();
            output_params.push_back(TyInt32Ptr);
            output_params.push_back(TyInt32Ptr);
            output_params.push_back(INT32_T);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto memcpy = new ::Function(output_type, "__aeabi_memcpy4", module.get());

            output_params.clear();
            output_params.push_back(TyInt32Ptr);
            output_params.push_back(INT32_T);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto memclr = new ::Function(output_type, "__aeabi_memclr4", module.get());

            output_params.push_back(INT32_T);
            output_type = new FunctionIRType(VOID_T, output_params);
            auto memset = new ::Function(output_type, "__aeabi_memset4", module.get());

            output_params.clear();
            output_type = new FunctionIRType(VOID_T, output_params);
            auto llvm_memset = new ::Function(output_type, "llvm.memset.p0.i32", module.get());

            scope.enter();
            scope.push("getint", get_int);
            scope.push("getfloat", get_float);
            scope.push("getch", get_char);
            scope.push("getarray", get_int_array);
            scope.push("getfarray", get_float_array);
            scope.push("putint", put_int);
            scope.push("putfloat", put_float);
            scope.push("putch", put_char);
            scope.push("putarray", put_int_array);
            scope.push("putfarray", put_float_array);
            scope.push("starttime", sysy_start_time);
            scope.push("stoptime", sysy_stop_time);
            scope.push("memcpy", memcpy);
            scope.push("memclr", memclr);
            scope.push("memset", memset);
            scope.push("llvm.memset.p0.i32", llvm_memset);
        }
     };
};
