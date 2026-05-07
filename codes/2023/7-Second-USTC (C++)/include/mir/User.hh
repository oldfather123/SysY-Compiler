#pragma once

#include <vector>
#include <string>

#include "Value.hh"

class User : public Value {
public:
    User(Type *ty, const std::string &name="", unsigned num_ops = 0);
    ~User() = default;

    Value *get_operand(unsigned i) const;   //& start from 0
    void set_operand(unsigned i, Value *v);   //& start from 0
    void add_operand(Value *v);

    std::vector<Value *> &get_operands();
    unsigned get_num_operands() const;

    void remove_operands(int index1, int index2);
    void remove_use_of_ops();
    
private:
    std::vector<Value *> operands_;   //& operands of this value
    unsigned num_ops_;  

};