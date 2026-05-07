#pragma once

#include "Value.hpp"

#include <vector>

class User : public Value {
  public:
    User(Type *ty, const std::string &name = "") : Value(ty, name){};
    virtual ~User() { remove_all_operands(); }

    const std::vector<Value *> &get_operands() const { return operands_; }
    unsigned get_num_operand() const { return operands_.size(); }

    // start from 0
    Value *get_operand(unsigned i) const { return operands_.at(i); };
    // start from 0
    void set_operand(unsigned i, Value *v);
    void add_operand(Value *v);
    void replace_operand(Value *old_val, Value *new_val) {
        set_operand_for_each_if([&](Value *op) -> std::pair<bool, Value *> {
            if (op == old_val) {
                return {true, new_val};
            } else {
                return {false, nullptr};
            }
        });
    }
    void set_operand_for_each_if(
        std::function<std::pair<bool, Value *>(Value *)> check) {
        for (unsigned i = 0; i < operands_.size(); ++i) {
            auto [change, new_value] = check(operands_[i]);
            if (change)
                set_operand(i, new_value);
        }
    }
    void remove_all_operands();
    void remove_operand(unsigned i);

  private:
    std::vector<Value *> operands_; // operands of this value
};

/* For example: op = func(a, b)
 *  for a: Use(op, 0)
 *  for b: Use(op, 1)
 */
struct Use {
    User *val_;       // used by whom
    unsigned arg_no_; // the no. of operand

    Use(User *val, unsigned no) : val_(val), arg_no_(no) {}

    bool operator==(const Use &other) const {
        return val_ == other.val_ and arg_no_ == other.arg_no_;
    }

    bool operator<(const Use &other) const {
        return arg_no_ < other.arg_no_ or
               (arg_no_ == other.arg_no_ and val_ < other.val_);
    }
};
