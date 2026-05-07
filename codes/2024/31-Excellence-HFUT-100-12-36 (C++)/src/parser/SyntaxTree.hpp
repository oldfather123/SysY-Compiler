#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
 
using std::cout;
using std::string;
using std::vector;

template <typename T>
using ptr = std::shared_ptr<T>;

template <typename T>
using ptr_list = std::vector<ptr<T>>;

  //一些枚举类
enum class relop
{
    equal = 0,
    non_equal,
    less,
    less_equal,
    greater,
    greater_equal,
    op_and,
    op_or
};
enum class binop
{
    plus = 0,
    minus,
    multiply,
    divide,
    modulo
};

enum class unaryop
{
    plus = 0,
    minus,
    op_not
};
enum class vartype
{
  INT=0,
  VOID,
  FLOAT,
  FLOATADDR,
  INTADDR,
//   FBOOL,
//   FBOOLADDR,
  BOOL,
  BOOLADDR
};

//ast结点
namespace ast{

struct syntax_tree_node;
struct compunit_syntax;
struct func_def_syntax;
struct expr_syntax;
struct expr_syntax;
struct binop_expr_syntax;
struct unaryop_expr_syntax;
struct lval_syntax;  
struct literal_syntax;
struct stmt_syntax;
struct assign_stmt_syntax;
struct block_syntax;
struct if_stmt_syntax;
struct return_stmt_syntax;
struct var_def_stmt_syntax;
struct var_decl_stmt_syntax;
struct func_f_param_syntax;
struct var_dimension_syntax;
struct exp_stmt_syntax;
struct while_stmt_syntax;
struct empty_stmt_syntax;
struct break_stmt_syntax;
struct continue_stmt_syntax;
struct init_syntax;
struct func_call_syntax;
//访问者模板
struct syntax_tree_visitor;//访问者模板
//语法树本树


//ast结点
struct syntax_tree_node {
  public:
    int line;
    //用于访问者模式
    virtual void accept(syntax_tree_visitor &visitor) = 0;
    //打印
    virtual void print() = 0;
};

class SyntaxTreePrinter {
    public:
    int cur_level = 0;
    void LevelPrint(std::ostream &out,std::string type_name,bool is_terminal){
        for(int i = 0 ; i < cur_level; ++i) out << "|  ";
        out << ">--" << (is_terminal ? "*" : "+") << type_name;
        out << std::endl;
    }
};

class SyntaxTree {
  public:
    syntax_tree_node *root;             // 上一次提交（61aa8eab52c2379c9ed58462c70bb47a7f7a4fda）已经检测过了，换为智能指针后会有double_free错误，应该是Bison会释放
    void print() { //
        this->root->print();
    }
    void accept(syntax_tree_visitor &visitor){
        this->root->accept(visitor);
    };
    bool insert(std::pair<string, ptr<ast::init_syntax>> pair) {
        auto res = const_val_map.insert(pair);
        return res.second;
    };
    ptr<ast::init_syntax> find(string name) {
        auto res = const_val_map.find(name);
        if(res != const_val_map.end()) {
            return res->second;
        }
        else {
            return nullptr;
        }
    };
    // ptr<expr_syntax> get_const_val(string name, ptr_list<expr_syntax> dimension) {
    //     auto const_ini = this->find(name);
    //     if(const_ini) {
    //         if(!dimension.empty()) {
    //             ptr<ast::init_syntax> ini_pointer = const_ini;
    //             for(auto it = dimension.begin(); it != dimension.end(); it++) {
    //                 auto tmp = ini_pointer->initializer[(*it)->calc_res()];
    //                 ini_pointer = std::dynamic_pointer_cast<init_syntax>(tmp);
    //                 if(!ini_pointer) {
    //                     abort();
    //                 }
    //             }
    //             return ini_pointer->initializer.front()->calc_res();
    //         }
    //         else {
    //             return const_ini->initializer.front()->calc_res();
    //         }
    //     }
    // };
private:
    std::unordered_map<string, ptr<ast::init_syntax>> const_val_map;
};

//编译单位，就是一个文件
struct compunit_syntax : syntax_tree_node
{
    // ptr_list<func_def_syntax> global_defs;
    ptr_list<syntax_tree_node> global_defs;         // 接受FuncDef和decl
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

// 函数定义
struct func_def_syntax : syntax_tree_node
{
    std::string name;
    ptr<block_syntax> body;
    vartype rettype;
    ptr_list<func_f_param_syntax> params;
    std::vector<vartype> arg_types;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

//算术表达式
struct expr_syntax : virtual syntax_tree_node
{
    virtual void accept(syntax_tree_visitor &visitor) = 0;
    virtual void print() = 0;
    virtual int calc_res() = 0;
};

//条件表达式
struct logic_cond_syntax: expr_syntax
{
    relop op;
    ptr<expr_syntax> lhs;
    ptr<expr_syntax> rhs;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};
struct rel_cond_syntax: expr_syntax
{
    relop op;
    ptr<expr_syntax> lhs;
    ptr<expr_syntax> rhs;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};



//二元算术表达式
struct binop_expr_syntax : expr_syntax
{
    binop op;
    ptr<expr_syntax> lhs, rhs;
    vartype restype;
    int intConst;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

//单元算术表达式
struct unaryop_expr_syntax : expr_syntax
{
    unaryop op;
    ptr<expr_syntax> rhs;
    vartype restype;
    int intConst;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

//求值表达式，比如算术表达式中的一个变量a
struct lval_syntax : expr_syntax
{
    std::string name;
    vartype restype;
    ptr<var_dimension_syntax> dimension;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

//常数
struct literal_syntax : expr_syntax
{
    vartype restype;
    int intConst;
    float floatConst;
    literal_syntax() {}
    literal_syntax(int intConst) : intConst(intConst), restype(vartype::INT) {}
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

//比如if,定义，赋值
struct stmt_syntax : virtual syntax_tree_node
{
    virtual void accept(syntax_tree_visitor &visitor) = 0;
    virtual void print() = 0;
};
//变量定义，可以有初始值也可以没有
struct var_def_stmt_syntax : stmt_syntax
{
    vartype restype;
    std::string name;
    ptr<init_syntax> initializer;
    bool is_const;
    ptr<var_dimension_syntax> dimension;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct var_decl_stmt_syntax : stmt_syntax
{
    ptr_list<var_def_stmt_syntax> var_def_list;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

//赋值语句
struct assign_stmt_syntax : stmt_syntax
{
    ptr<lval_syntax> target;
    ptr<expr_syntax> value;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

//大括号里面的东西
struct block_syntax : stmt_syntax
{
    ptr_list<stmt_syntax> body;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

// If statement.
struct if_stmt_syntax : stmt_syntax
{
    ptr<expr_syntax> pred;
    ptr<stmt_syntax> then_body;
    ptr<stmt_syntax> else_body;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

//return
struct return_stmt_syntax: stmt_syntax
{
    ptr<expr_syntax> exp;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

//空语句
struct empty_stmt_syntax : stmt_syntax
{
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct func_f_param_syntax : syntax_tree_node {
    vartype accept_type;
    std::string name;
    ptr<ast::var_dimension_syntax> dimension;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct var_dimension_syntax : syntax_tree_node {
    bool has_first_dim = true;
    ptr_list<ast::expr_syntax> dimensions;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct exp_stmt_syntax : stmt_syntax {
    ptr<expr_syntax> exp;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct while_stmt_syntax : stmt_syntax {
    ptr<expr_syntax> cond;
    ptr<stmt_syntax> while_body;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct break_stmt_syntax : stmt_syntax
{
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct continue_stmt_syntax : stmt_syntax
{
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
};

struct init_syntax : expr_syntax
{
    bool is_array = false;
    bool is_zero_initializer = false;
    ptr_list<expr_syntax> initializer;
    ptr<expr_syntax> designed_size;
    int child_cnt = 1;
    ptr_list<expr_syntax> current_dim;
    // int to_bottom;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

struct func_call_syntax : expr_syntax {
    string func_name;
    ptr_list<expr_syntax> params;
    virtual void accept(syntax_tree_visitor &visitor) override final;
    virtual void print() override final;
    virtual int calc_res() override final;
};

//访问者模板
class syntax_tree_visitor
{
  public:
    virtual void visit(compunit_syntax &node) = 0;
    virtual void visit(func_def_syntax &node) = 0;
    virtual void visit(rel_cond_syntax &node) = 0;
    virtual void visit(logic_cond_syntax &node) = 0;
    virtual void visit(binop_expr_syntax &node) = 0;
    virtual void visit(unaryop_expr_syntax &node) = 0;
    virtual void visit(lval_syntax &node) = 0;
    virtual void visit(literal_syntax &node) = 0;
    virtual void visit(var_def_stmt_syntax &node) = 0;
    virtual void visit(assign_stmt_syntax &node) = 0;
    virtual void visit(block_syntax &node) = 0;
    virtual void visit(if_stmt_syntax &node) = 0;
    virtual void visit(return_stmt_syntax &node) = 0;
    virtual void visit(func_f_param_syntax &node) = 0;

    virtual void visit(var_decl_stmt_syntax &node) = 0;
    virtual void visit(var_dimension_syntax &node) = 0;
    virtual void visit(exp_stmt_syntax &node) = 0;
    virtual void visit(while_stmt_syntax &node) = 0;
    virtual void visit(empty_stmt_syntax &node) = 0;
    virtual void visit(break_stmt_syntax &node) = 0;
    virtual void visit(continue_stmt_syntax &node) = 0;
    virtual void visit(init_syntax &node) = 0;
    virtual void visit(func_call_syntax &node) = 0;
};

void parse_file(string input_file_path);
void parse_file(std::istream& in);

}//end namespace ast

#endif
