#pragma once

#include "BasicBlock.hpp"
#include "Type.hpp"
#include "User.hpp"
#include "ilist.hpp"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <memory>

class Module;
class Argument;
class Type;
class FunctionType;

class Function : public Value, public ilist<Function>::node {
  public:
    Function(const Function &) = delete;
    Function(FunctionType *ty, const std::string &name, Module *parent);
    ~Function() = default;
    static Function *create(FunctionType *ty, const std::string &name,
                            Module *parent);

    FunctionType *get_function_type() const;
    Type *get_return_type() const;

    void add_basic_block(BasicBlock *bb);

    unsigned get_num_of_args() const;
    unsigned get_num_basic_blocks() const;

    Module *get_parent() const;

    void remove(BasicBlock *bb);
    BasicBlock *get_entry_block() { return &*basic_blocks_.begin(); }

    ilist<BasicBlock> &get_basic_blocks() { return basic_blocks_; }
    std::list<Argument> &get_args() { return arguments_; }

    bool is_declaration() { return basic_blocks_.empty(); }

    void set_instr_name();
    std::string print();

  private:
    ilist<BasicBlock> basic_blocks_;
    std::list<Argument> arguments_;
    Module *parent_;
    unsigned seq_cnt_; // print use
};

// Argument of Function, does not contain actual value
class Argument : public Value {
  public:
    Argument(const Argument &) = delete;
    explicit Argument(Type *ty, const std::string &name = "",
                      Function *f = nullptr, unsigned arg_no = 0)
        : Value(ty, name), parent_(f), arg_no_(arg_no) {}
    virtual ~Argument() {}

    inline const Function *get_parent() const { return parent_; }
    inline Function *get_parent() { return parent_; }

    /// For example in "void foo(int a, float b)" a is 0 and b is 1.
    unsigned get_arg_no() const {
        assert(parent_ && "can't get number of unparented arg");
        return arg_no_;
    }

    virtual std::string print() override;

  private:
    Function *parent_;
    unsigned arg_no_; // argument No.
};

// class Argument : public Value {
//   public:
//       // 默认构造函数
//       Argument(Type *ty, const std::string &name = "",
//                Function *f = nullptr, unsigned arg_no = 0)
//           : Value(ty, name), parent_(f), arg_no_(arg_no) {}
  
//       // 拷贝构造函数
//       Argument(const Argument &other)
//           : Value(other), parent_(other.parent_), arg_no_(other.arg_no_) {}
  
//       // 拷贝赋值运算符
//       Argument& operator=(const Argument &other) {
//           if (this != &other) {  // 防止自我赋值
//               Value::operator=(other);  // 调用父类的赋值运算符（如果有）
//               parent_ = other.parent_;
//               arg_no_ = other.arg_no_;
//           }
//           return *this;
//       }
  
//       virtual ~Argument() {}
  
//       inline const Function *get_parent() const { return parent_; }
//       inline Function *get_parent() { return parent_; }
  
//       unsigned get_arg_no() const {
//           assert(parent_ && "can't get number of unparented arg");
//           return arg_no_;
//       }
  
//       virtual std::string print() override;
  
//   private:
//       Function *parent_;  // 参数所属的函数
//       unsigned arg_no_;   // 参数编号
//   };
  