#pragma once

#include "common/type.hpp"

#include <string>

/// @brief Basic class for temperary register
class Temp {
protected:
    int index_;
    Type temp_type_;
public:
    Temp(int index, Type temp_type) : index_(index), temp_type_(temp_type) {}
    std::string to_str() const { return type_string(temp_type_) + " _T" + std::to_string(index_); }
    std::string to_llvm_str() const { return "%T" + std::to_string(index_); }
    int get_index() const { return index_; }
    void set_index(int index) { index_ = index; }
    Type get_type() const { return temp_type_; }
    void set_type(Type type) { temp_type_ = type; }
    bool operator < (const Temp &b) const { return index_ < b.index_; }
    bool operator ==(const Temp &b) const { return index_ == b.index_; }
};

// /// @brief temp for variable
// class TempVar : public Temp {
// public:
//     TempVar(int index, Type var_type) : Temp(index, var_type) {}
// };

// /// @brief temp for array (you are expected only using it in arguments)
// class TempArray : public Temp {
// public:
//     TempArray(int index, Type var_type) : Temp(index, var_type) {}
// };

// /// @brief temp for single array element
// class TempArrayElement : public Temp {
// protected:
//     TempArray *parent_;
// public:
//     explicit TempArrayElement() = default;
//     TempArrayElement(TempArray *parent, Type var_type) : 
//         Temp(parent->get_index(), var_type), parent_(parent) {}
//     std::string to_str() { 
//         std::string ret = parent_->to_str();
//         for (auto dim : temp_type_.dims) {
//             ret += "[" + std::to_string(dim) + "]";
//         }
//         return ret;    
//     }
// };