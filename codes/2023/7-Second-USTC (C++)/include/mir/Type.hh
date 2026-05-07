#pragma once

#include <string>
#include <vector>
#include <iterator>

class Module;
class IntegerType;
class FloatType;
class FunctionType;
class ArrayType;
class PointerType;

class Type {
public:
    enum TypeID {
      VoidTyID,         //// Void
      LabelTyID,        //// Labels, e.g., BasicBlock
      IntegerTyID,      //// Integers, include 32 bits and 1 bit
      FloatTyID,        //// Float
      FunctionTyID,     //// Functions
      ArrayTyID,        //// Arrays
      PointerTyID,      //// Pointer
    };

public:
    explicit Type(TypeID tid, Module *m): tid_(tid), m_(m) {};
    ~Type() = default;

    TypeID get_type_id() const { return tid_; }

    bool is_void_type() const { return get_type_id() == VoidTyID; }
    bool is_label_type() const { return get_type_id() == LabelTyID; }
    bool is_integer_type() const { return get_type_id() == IntegerTyID; }
    bool is_float_type() const { return get_type_id() == FloatTyID; }
    bool is_function_type() const { return get_type_id() == FunctionTyID; }
    bool is_array_type() const { return get_type_id() == ArrayTyID; }
    bool is_pointer_type() const { return get_type_id() == PointerTyID; }

    static bool is_eq_type(Type *ty1, Type *ty2) { return ty1 == ty2; };

    static Type *get_void_type(Module *m);
    static Type *get_label_type(Module *m);
    static IntegerType *get_int1_type(Module *m);
    static IntegerType *get_int32_type(Module *m);
    static PointerType *get_int32_ptr_type(Module *m);
    static FloatType *get_float_type(Module *m);
    static PointerType *get_float_ptr_type(Module *m);
    static PointerType *get_pointer_type(Type *contained);
    static ArrayType *get_array_type(Type *contained, unsigned num_elements);
    
    Type *get_pointer_element_type();
    Type *get_array_element_type();

    int get_size();

    Module *get_module() { return m_; }

    std::string print();

private:
    TypeID tid_;
    Module *m_;
};


class IntegerType : public Type {
public:
    explicit IntegerType(unsigned num_bits, Module *m): Type(Type::IntegerTyID, m), num_bits_(num_bits) {}
    
    static IntegerType *get(unsigned num_bits, Module *m);

    unsigned get_num_bits();

private:
    unsigned num_bits_;  
};

class FloatType : public Type {
public:
    FloatType(Module *m) : Type(Type::FloatTyID, m) {}
    static FloatType *get(Module *m);

private:
};

class PointerType : public Type {
public:
    PointerType(Type *contained): Type(Type::PointerTyID, contained->get_module()), contained_(contained) {}

    static PointerType *get(Type *contained); 
    
    Type *get_element_type() const { return contained_; }

private:
    Type *contained_; //& The element type of the pointer
};


class ArrayType : public Type {
public:
    ArrayType(Type *contained, unsigned num_elements);

    static ArrayType *get(Type *contained, unsigned num_elements);

    static bool is_valid_element_type(Type *ty);

    Type *get_element_type() const { return contained_; }
    unsigned get_num_of_elements() const { return num_elements_; }

private:
    Type *contained_;             //& The element type of the array.
    unsigned num_elements_;       //& Number of elements in the array.
};


class FunctionType : public Type {
public:    
    FunctionType(Type *result, std::vector<Type *> params);

    static FunctionType *get(Type *result, std::vector<Type *> params);

    static bool is_valid_return_type(Type *ty);
    static bool is_valid_argument_type(Type *params);

    unsigned get_num_of_args() const { return args_.size(); };
    std::vector<Type *>::iterator param_begin() { return args_.begin(); }
    std::vector<Type *>::iterator param_end() { return args_.end(); }

    Type *get_param_type(unsigned i) const { return args_[i]; };
    Type *get_return_type() const { return result_; };

private:
    Type *result_;
    std::vector<Type *> args_;
};


