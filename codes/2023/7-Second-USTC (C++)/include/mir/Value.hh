#pragma once

#include <list>
#include <string>
#include <functional>

class Type;
class Value;
class User;

struct Use {

  Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}

  friend bool operator==(const Use &lhs, const Use &rhs) {
      return lhs.val_ == rhs.val_ && lhs.arg_no_ == rhs.arg_no_;
  }

  Value *val_;
  unsigned arg_no_;   //& the no. of operand, e.g., func(a, b), a is 0, b is 1
};

class UseHash {
public:
  size_t operator()(const Use &u) const {
    return (std::hash<Value *>()(u.val_)) ^ (std::hash<unsigned>()(u.arg_no_));
  }
};


class Value {
public:
    explicit Value(Type *ty, const std::string &name=""): type_(ty), name_(name) {}
    virtual ~Value() = default;

    Type *get_type() const { return type_; } 

    std::list<Use> &get_use_list() { return use_list_; }
    void add_use(Value* val, unsigned arg_no = 0);

    bool set_name(std::string name) {
      if(name_ == "") {
        name_ = name;
        return  true;
      } 
      return false;
    }

    void take_name(Value* v){
        name_ = v->get_name();
        v->name_= "";
    }
    
    std::string get_name() const { return name_; }

    void replace_all_use_with(Value *new_val);
    void replace_use_with_when(Value *new_val, std::function<bool(User *)> pred); //& replace `value` with `new_val` when the user of value satisfies predicate `pred`
    void remove_use(Value *val);

    virtual std::string print() = 0;

private:
    Type *type_;
    std::list<Use> use_list_;   //& store the users who use this value
    std::string name_;          
};