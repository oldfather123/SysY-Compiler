#pragma once

#include <list>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <unordered_map>

#include "Type.hh"
#include "GlobalVariable.hh"
#include "Value.hh"
#include "Function.hh"
#include "Instruction.hh"

class GlobalVariable;
class Function;
class Instruction;

struct pair_hash {
    template <typename T>
    std::size_t operator()(const std::pair<T, Module *> val) const {
        auto lhs = std::hash<T>()(val.first);
        auto rhs = std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(val.second));
        return lhs ^ rhs;
    }
};

class Module {
public:
    explicit Module(std::string name);
    ~Module();

    Type *get_void_type();
    Type *get_label_type();
    IntegerType *get_int1_type();
    IntegerType *get_int32_type();
    FloatType *get_float_type();
    PointerType *get_int32_ptr_type();
    PointerType *get_float_ptr_type();
    PointerType *get_pointer_type(Type *contained);
    ArrayType *get_array_type(Type *contained, unsigned num_elements);
    FunctionType *get_function_type(Type *retty, std::vector<Type *>&args);

    void add_function(Function *f);
    std::list<Function*> &get_functions() { return functions_list_; }

    Function *get_main_function();

    void add_global_variable(GlobalVariable *g);
    std::list<GlobalVariable*> &get_global_variables() { return globals_list_; }

    std::string get_instr_op_name(Instruction::OpID instr) { return instr_id2string_[instr]; }

    void set_print_name();
    void set_file_name(std::string name) { source_file_name_ = name; }
    std::string get_file_name() { return source_file_name_; }

    virtual std::string print();

public:
    std::unordered_map<std::pair<int, Module *>, std::unique_ptr<ConstantInt>, pair_hash> cached_int;
    std::unordered_map<std::pair<bool, Module *>, std::unique_ptr<ConstantInt>, pair_hash> cached_bool;
    std::unordered_map<std::pair<float, Module *>, std::unique_ptr<ConstantFP>, pair_hash> cached_float;
    std::unordered_map<Type *, std::unique_ptr<ConstantZero>> cached_zero;

private:
    std::list<GlobalVariable *> globals_list_;                  //& The Global Variables in the module
    std::list<Function *> functions_list_;                      //& The Functions in the module
    std::map<std::string, Value*> value_symbol_table_;          //& Symbol table for values
    std::map<Instruction::OpID, std::string> instr_id2string_;  //& Instruction from opid to string

    std::string module_name_;                                   //& Human readable identifier for the module
    std::string source_file_name_;                              //& Original source file name for module, for test and debug

private:
    Type *label_ty_;
    Type *void_ty_;
    IntegerType *int1_ty_;
    IntegerType *int32_ty_;
    FloatType *float32_ty_;

    std::map<Type *, PointerType *> pointer_map_;
    std::map<std::pair<Type*, int>, ArrayType*> array_map_;
    std::map<std::pair<Type*, std::vector<Type*>>, FunctionType*> function_map_;
};

