#include "SyntaxTree.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <cassert>

using namespace ast;

extern SyntaxTree syntax_tree;

SyntaxTreePrinter ast_printer;

void ast::parse_file(string input_file_path) {
    const char *input_file_path_cstr = input_file_path.c_str();
    if (input_file_path != "") {
        auto input_file = fopen(input_file_path_cstr, "r");
        if (!input_file) {
            std::cout << "Error: Cannot open file " << input_file_path_cstr << "\n";
            perror("fault");
            exit(110);
        }
        std::ifstream t(input_file_path);
        std::string buffer;
        //预设缓冲区大小
        t.seekg(0, std::ios::end);
        buffer.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        buffer.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        //自带的函数
        yy_scan_string(buffer.c_str());
        // yyrestart(buffer_file);
        fclose(input_file);
    }
    //开始语法分析，用的是自带的函数
    yyparse();
}
void ast::parse_file(std::istream &in)
{
    std::string buffer;
    std::string str;
    while(in.peek() != EOF){
        std::getline(in,str);
        buffer += str;
        buffer += '\n';
    } 
    yy_scan_string(buffer.c_str());
    yyparse();
}


void compunit_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void compunit_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"CompUnit",false);
    ast_printer.cur_level++;
    for(auto child : this->global_defs){
        child.get()->print();
    }
    ast_printer.cur_level--;
}

void func_def_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void func_def_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"FuncDef",false);

    ast_printer.cur_level++;
    std::string type = (this->rettype == vartype::INT ? "int" : (this->rettype == vartype::FLOAT ? "float" : "void"));
    ast_printer.LevelPrint(std::cout,type,true);
    ast_printer.LevelPrint(std::cout,this->name,true);
    ast_printer.LevelPrint(std::cout,"(",true);

    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout, "func_param", false);
    for(auto a : this->params) {
        a->print();
    }
    ast_printer.cur_level--;

    ast_printer.LevelPrint(std::cout,")",true);
    ast_printer.LevelPrint(std::cout,"{",true);
    
    ast_printer.cur_level++;
    this->body.get()->print();
    ast_printer.cur_level--;

    ast_printer.LevelPrint(std::cout,"}",true);
    ast_printer.cur_level--;
}

void binop_expr_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void binop_expr_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"binop",false);
    ast_printer.cur_level++;
    this->lhs->print();
    std::vector<std::string> op={"+","-","*","/","%"};
    ast_printer.LevelPrint(std::cout,op[int(this->op)],true);
    this->rhs->print();
    ast_printer.cur_level--;
}

void unaryop_expr_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void unaryop_expr_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"unary",false);
    ast_printer.cur_level++;
    std::vector<std::string> op={"+","-","!"};
    ast_printer.LevelPrint(std::cout,op[int(this->op)],true);
    this->rhs->print();
    ast_printer.cur_level--;
}

void lval_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void lval_syntax::print()
{
    ast_printer.LevelPrint(std::cout,this->name,true);
    if(this->dimension) {
        this->dimension->print();
    }
}

void literal_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void literal_syntax::print()
{
    // ast_printer.LevelPrint(std::cout,"IntConst",false);
    // ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout,std::to_string(this->intConst),true);
    // ast_printer.cur_level--;
}

void var_def_stmt_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void var_def_stmt_syntax::print()
{

    ast_printer.LevelPrint(std::cout,this->name,true);
    
    if(this->dimension) {
        this->dimension->print();
    }
    
    if(this->initializer)
    {
        ast_printer.LevelPrint(std::cout,"=",true);
        this->initializer->print();
    }
        
}

void assign_stmt_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void assign_stmt_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"assign_stmt",false);
    ast_printer.cur_level++; 
    this->target->print();
    ast_printer.LevelPrint(std::cout,"=",true);
    this->value->print();
    ast_printer.LevelPrint(std::cout,";",true);
    ast_printer.cur_level--; 
}

void block_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void block_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"Block",false);
    ast_printer.cur_level++;
    for(auto & content: this->body){
        content.get()->print();
    }
    ast_printer.cur_level--;
}

void if_stmt_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void if_stmt_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"if_stmt",false);
    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout,"(",true);
    this->pred->print();
    ast_printer.LevelPrint(std::cout,")",true);
    ast_printer.LevelPrint(std::cout,"then_block",false);
    ast_printer.cur_level++;
    if(this->then_body)
        this->then_body->print();
    ast_printer.cur_level--;
    if(this->else_body)
    {
         ast_printer.LevelPrint(std::cout,"else_body",false);
         ast_printer.cur_level++;
         this->else_body->print();
         ast_printer.cur_level--;
    }
    ast_printer.cur_level--;
}

void return_stmt_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void return_stmt_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"stmt",false);

    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout,"return",true);
    this->exp.get()->print();
    ast_printer.LevelPrint(std::cout,";",true);
    ast_printer.cur_level--;
}

void empty_stmt_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void empty_stmt_syntax::print() {
    ast_printer.LevelPrint(std::cout, "empty_stmt", true);
    // ast_printer.cur_level++;
    // ast_printer.cur_level--;
}

void ast::func_f_param_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void ast::func_f_param_syntax::print() {
    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout, this->accept_type == vartype::INT ? "int" : "float", true);
    ast_printer.LevelPrint(std::cout, this->name , true);
    if(this->dimension) {
        this->dimension->print();
    }
    ast_printer.cur_level--;
}

void ast::var_dimension_syntax::accept(syntax_tree_visitor &visitor){
    visitor.visit(*this);
}

void ast::var_dimension_syntax::print() {
    ast_printer.cur_level++;
    if(!this->has_first_dim) {
        ast_printer.LevelPrint(std::cout, "[", false);
        ast_printer.LevelPrint(std::cout, "]", false);
    }
    for(auto a : this->dimensions) {
        ast_printer.LevelPrint(std::cout, "[", false);
        ast_printer.cur_level++;
        a->print();
        ast_printer.cur_level--;
        ast_printer.LevelPrint(std::cout, "]", false);
    }
    ast_printer.cur_level--;
}

void ast::exp_stmt_syntax::accept(syntax_tree_visitor &visitor){
    visitor.visit(*this);
}

void ast::exp_stmt_syntax::print() {
    ast_printer.LevelPrint(std::cout, "exp_stmt", false);
    ast_printer.cur_level++;
    this->exp->print();
    ast_printer.LevelPrint(std::cout, ";", true);
    ast_printer.cur_level--;
}

void ast::while_stmt_syntax::accept(syntax_tree_visitor &visitor){
    visitor.visit(*this);
}

void ast::while_stmt_syntax::print() {
    ast_printer.LevelPrint(std::cout, "while_stmt", false);
    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout,"(",true);
    this->cond->print();
    ast_printer.LevelPrint(std::cout,")",true);
    ast_printer.LevelPrint(std::cout,"while_block",true);
    ast_printer.cur_level++;
    this->while_body->print();
    ast_printer.cur_level--;
    ast_printer.cur_level--;
}

void break_stmt_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void break_stmt_syntax::print() {
    ast_printer.LevelPrint(std::cout, "break", true);
    ast_printer.LevelPrint(std::cout, ";", true);
    // ast_printer.cur_level++;
    // ast_printer.cur_level--;
}

void continue_stmt_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void continue_stmt_syntax::print() {
    ast_printer.LevelPrint(std::cout, "continue", true);
    ast_printer.LevelPrint(std::cout, ";", true);
    // ast_printer.cur_level++;
    // ast_printer.cur_level--;
}

void init_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void init_syntax::print() {
    // ast_printer.LevelPrint(std::cout, "init_stmt", false);
    // ast_printer.cur_level++;
    if(this->is_array) {
        ast_printer.LevelPrint(std::cout, "{", true);
    }
    ast_printer.cur_level++;
    for(auto a : this->initializer) {
        a->print();
    }
    ast_printer.cur_level--;
    if(this->is_array) {
        ast_printer.LevelPrint(std::cout, "}", true);
    }
    // ast_printer.cur_level--;
}


void func_call_syntax::accept(syntax_tree_visitor &visitor) {
    visitor.visit(*this);
}

void func_call_syntax::print() {
    ast_printer.LevelPrint(std::cout, "func_call", false);
    ast_printer.cur_level++;
    ast_printer.LevelPrint(std::cout, this->func_name, true);
    for(auto a : this->params) {
        a->print();
    }
    ast_printer.cur_level--;
}

void ast::var_decl_stmt_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void ast::var_decl_stmt_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"decl",false);
    ast_printer.cur_level++;
    std::vector<std::string> typeMapping = {"int","void","float"};

    auto type = this->var_def_list.back()->restype;

    if(var_def_list.back()->is_const) {
        ast_printer.LevelPrint(std::cout,"const",true);
    }

    ast_printer.LevelPrint(std::cout,typeMapping[int(type)],true);
    for(int i = 0; i < this->var_def_list.size(); ++i){

        var_def_list[i]->print();

        if(i != this->var_def_list.size() - 1){
            ast_printer.LevelPrint(std::cout,",",true);
        }else{
            ast_printer.LevelPrint(std::cout,";",true);
        }
    }
    ast_printer.cur_level--;
}

void ast::logic_cond_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void ast::logic_cond_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"logic_cond",false);
    ast_printer.cur_level++;
    this->lhs->print();
    std::vector<std::string> op={"==","!=","<","<=",">",">=","&&","||"};
    ast_printer.LevelPrint(std::cout,op[int(this->op)],true);
    this->rhs->print();
    ast_printer.cur_level--;
}

void ast::rel_cond_syntax::accept(syntax_tree_visitor &visitor)
{
    visitor.visit(*this);
}

void ast::rel_cond_syntax::print()
{
    ast_printer.LevelPrint(std::cout,"rel_cond",false);
    ast_printer.cur_level++;
    this->lhs->print();
    std::vector<std::string> op={"==","!=","<","<=",">",">=","&&","||"};
    ast_printer.LevelPrint(std::cout,op[int(this->op)],true);
    this->rhs->print();
    ast_printer.cur_level--;
}

int ast::logic_cond_syntax::calc_res() {
    auto l = this->lhs->calc_res();
    auto r = this->rhs->calc_res();
    switch (this->op) {
        case relop::equal: return l == r;
        case relop::non_equal: return l != r;
        case relop::less: return l < r;
        case relop::less_equal: return l <= r;
        case relop::greater: return l > r;
        case relop::greater_equal: return l >= r;
        case relop::op_and: return l && r;
        case relop::op_or: return l || r;
        break;
    }
    return -1;
}

int ast::rel_cond_syntax::calc_res() {
    auto l = this->lhs->calc_res();
    auto r = this->rhs->calc_res();
    switch (this->op) {
        case relop::equal: return l == r;
        case relop::non_equal: return l != r;
        case relop::less: return l < r;
        case relop::less_equal: return l <= r;
        case relop::greater: return l > r;
        case relop::greater_equal: return l >= r;
        case relop::op_and: return l && r;
        case relop::op_or: return l || r;
        break;
    }
    return -1;
}

int ast::binop_expr_syntax::calc_res() {
    auto l = this->lhs->calc_res();
    auto r = this->rhs->calc_res();
    switch (this->op) {
        case binop::plus: return l + r;
        case binop::minus: return l - r;
        case binop::multiply: return l * r;
        case binop::divide: return l / r;
        case binop::modulo: return l % r;
        break;
    }
    return -1;
}

int ast::unaryop_expr_syntax::calc_res() {
    auto r = this->rhs->calc_res();
    switch (this->op) {
        case unaryop::plus: return +r;
        case unaryop::minus: return -r;
        case unaryop::op_not: return !r;
        break;
    }
    return -1;
}

int ast::lval_syntax::calc_res() {
    // calc_res只是用来计算常量表达式
    auto const_ini = syntax_tree.find(this->name);
    if(const_ini) {
        if(this->dimension) {
            ptr<ast::init_syntax> ini_pointer = const_ini;
            for(auto it = this->dimension->dimensions.begin(); it != this->dimension->dimensions.end(); it++) {
                auto tmp = ini_pointer->initializer[(*it)->calc_res()];
                ini_pointer = std::dynamic_pointer_cast<init_syntax>(tmp);
                if(!ini_pointer) {
                    abort();
                }
            }
            return ini_pointer->initializer.front()->calc_res();
        }
        else {
            return const_ini->initializer.front()->calc_res();
        }
    }
    std::abort();
    return -1;
}

int ast::literal_syntax::calc_res() {
    switch (this->restype) {
        case vartype::INT: return this->intConst;
        case vartype::FLOAT: return this->floatConst;
        case vartype::VOID:
        case vartype::FLOATADDR:
        case vartype::INTADDR:
        // case vartype::FBOOL:
        // case vartype::FBOOLADDR:
        case vartype::BOOL:
        case vartype::BOOLADDR: std::abort(); // calc_res只是用来计算常量表达式
        break;
        }
    return -1;
}

int ast::init_syntax::calc_res() {
    // calc_res只是用来计算常量表达式
    std::abort();
    return -1;
}

int ast::func_call_syntax::calc_res() {
    // calc_res只是用来计算常量表达式
    std::abort();
    return -1;
}
