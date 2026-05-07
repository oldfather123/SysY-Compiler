#pragma once
#include "lexer.h"
#include "environment.h"

#define IS_GLOBAL (Env::num == 1)

class Parser
{
private:
    Token *look;
    Lexer *lex;
    std::vector<Token *> tokens;
    int index;
    Env *top;

public:
    ~Parser() {}
    void add_builtin_function(IR::Module *module, IR::IRBuilder *builder, Word *, Type *, std::vector<Id *>);
    void init(IR::Module *module, IR::IRBuilder *builder);
    void move();
    void back(int n);
    Parser(Lexer *l);
    void error(std::string s);
    void match(int t);
    void program_to_IR(IR::Module *module, IR::IRBuilder *builder);
    Stmt *program(IR::Module *module, IR::IRBuilder *builder);
    Stmt *decl_function(IR::Module *module, IR::IRBuilder *builder);  // function declaration
    Stmt *global_declare(IR::Module *module, IR::IRBuilder *builder); // function and global variable declaration
    Stmt *decl_variable(Type *t, bool const_decl);                    // variable declaration
    Stmt *block(bool);
    Stmt *stmts();
    Stmt *stmt();
    // Stmt* assign();
    Expr *decl_args();
    Expr *get_expr();
    Expr *init_list();
    Expr *equal();
    Expr *logic_or();
    Expr *logic_and();
    Expr *bit_or();
    Expr *bit_xor();
    Expr *bit_and();
    Expr *logic_equality();
    Expr *logic_ge_le();
    Expr *add_sub();
    Expr *mul_mod_div();
    Expr *unary();
    Expr *factor();
};
