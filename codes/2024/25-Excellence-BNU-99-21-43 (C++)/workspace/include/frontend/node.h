#pragma once
#include "lexer.h"
#include "IR.h"
#include <algorithm>
#include <vector>

#define VAR_ID 0
#define TEMP_ID 1
#define ARG_ID 2
#define FUNCTION_ID 3
#define ARRAY_ID 4

#define OPEXPR 5
#define INITEXPR 6
#define IDEXPR 7

#define CHECKTYPE(x, y) ((x) == Type::y)
// IR code generation

IR::BasicType *TypeToIRType(Type *x);
IR::BasicType *TypeToArgType(Type *x);

class Node
{
public:
    static int labels;
    int label;
    int lexline;
    IR::Value *value;
    Node();
    int new_label();
    void emit_label(int l);

protected:
    void emit(std::string s);
    void error(std::string s);
};

class Expr : public Node
{
public:
    union int_float
    {
        int ivar;
        float fvar;
    };
    int_float my_value;
    static Expr *expr_null;
    Token *op;
    Type *type;
    int ExprType;
    bool is_const;
    Expr(Token *tok, Type *p);
    virtual IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
    virtual IR::Value *getAddress(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
    virtual std::string to_string();
    virtual void jumping(IR::Module *module, IR::IRBuilder *builder, IR::BasicBlock *t, IR::BasicBlock *f);
    virtual Expr *get_expr1();
    virtual Expr *get_expr2();
};

using pve = std::pair<std::vector<int>, Expr *>; // for array init

class SeqExpr : public Expr
{
public:
    int number;
    Expr *expr1;
    Expr *expr2;
    SeqExpr(Expr *e1, Expr *e2);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    Expr *get_expr1() override;
    Expr *get_expr2() override;
};

class Id : public Expr
{
protected:
    int offset;

public:
    int ID_type;
    bool is_global;
    Id(Token *tok, Type *p, bool is_glob, bool is_const);
    std::string to_string() override;
};

class Arg : public Id
{
private:
    static int num_counts;
    int num;
    bool saved = false;

public:
    Arg(Token *tok, Type *p);
    Arg(Type *p, int idx);
};

class Function : public Id
{
public:
    Expr *args;
    static Function *Enclosing;
    static Function *PrintInt;
    int used;
    int totsz;
    Function(Token *tok, Type *t, Expr *args);
    void emit_label();
    friend class Call;
    std::string to_string() override;
    void set_args(Expr *a);
};

class IntConstant : public Id
{
public:
    static IntConstant *True;
    static IntConstant *False;
    IntConstant(Token *tok, bool glob);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    std::string to_string() override;
};

class FloatConstant : public Id
{
public:
    FloatConstant(Token *tok, bool glob);
    std::string to_string() override;
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
};

class Array : public Id
{
public:
    bool useforarg = false;
    std::vector<Expr *> dims;
    std::vector<int> dim_size;
    Array(Token *tok, Type *p, std::vector<Expr *> _dims, bool glob, bool is_const);
    // only used for building (array)args in builtin functions
    Array(Type *p, int idx);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override
    {
        if (value == nullptr)
            error("Array::gen: value is nullptr");
        if (!useforarg)
            return value;
        else
            return builder->CreateLoad(value->getType()->getBaseType(), value);
    }
    IR::Value *getAddress(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override
    {
        return gen(module, builder);
    }
    void set_size(std::vector<int> size) { dim_size = size; };
};

class Op : public Expr
{
public:
    Op(Token *tok, Type *p);
};

class Unary : public Op
{
private:
    Expr *expr;

public:
    Unary(Token *tok, Expr *expr);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    std::string to_string() override;
};

class Binary : public Op
{
protected:
    Expr *expr1;
    Expr *expr2;

public:
    Binary(Token *tok, Expr *expr1, Expr *expr2);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    std::string to_string() override;
};

class Assign : public Op
{
private:
    Expr *id;
    Expr *expr;

public:
    Assign(Expr *i, Expr *x);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    std::string to_string() override;
};

class Access : public Op
{
private:
    Id *array;
    std::vector<Expr *> index;

public:
    Access(Id *a, std::vector<Expr *> i);
    Access(Id *a, const std::vector<int> &i);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "") override;
    IR::Value *getAddress(IR::Module *module, IR::IRBuilder *builder, const char *from) override;
    std::string to_string() override;
};

class Logical : public Binary
{
public:
    Logical(Token *tok, Expr *expr1, Expr *expr2);
    void jumping(IR::Module *module, IR::IRBuilder *builder, IR::BasicBlock *t, IR::BasicBlock *f);
};

// class Offset: public Id {
// private:
//     Id* array;
//     Expr* index;
// };

class Call : public Expr
{
private:
    Function *function;
    Expr *args;
    Expr *fun_args;

public:
    Call(Token *tok, Type *t, Function *f, Expr *a);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
    std::string to_string();
};

class InitList : public Expr
{
public:
    bool is_const;
    std::vector<Expr *> exprs;
    InitList(std::vector<Expr *> e);
    InitList();
    std::string to_string();
    void add(Expr *e);
};

class Stmt : public Node
{
private:
public:
    static Stmt *stmt_null;
    static Stmt *Enclosing;
    IR::BasicBlock *after;
    IR::BasicBlock *begin;
    Stmt();
    virtual IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Set : public Stmt
{
private:
    Id *id;
    Expr *expr;

public:
    Set(Id *i, Expr *x);
    void parseInitlist(IR::Module *module,
                       IR::IRBuilder *builder, Array *id,
                       std::vector<int> &dims, int edge,
                       InitList *expr, std::vector<pve> &initres);
    bool arrayAdd(IR::Module *module, IR::IRBuilder *builder, std::vector<int> &idx, std::vector<int> &dim_size, int edge, int where);
    void arrayStore(IR::Module *module, IR::IRBuilder *builder, Array *id, InitList *expr);
    void store(IR::Module *module, IR::IRBuilder *builder, Expr *id, Expr *expr);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Calculate : public Stmt
{
private:
    Expr *expr;

public:
    Calculate(Expr *x);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
    std::string to_string();
};

class Declare : public Stmt
{
private:
    Type *type;
    Id *id;
    Stmt *expr;

public:
    Declare(Type *t, Id *i, Stmt *x);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Declare_Function : public Stmt
{
private:
    Function *function;
    Stmt *stmt;
    bool builtin_def;

public:
    Declare_Function(Function *f, Stmt *s, bool builtin = false);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class SeqStmt : public Stmt
{
private:
    Stmt *stmt1;
    Stmt *stmt2;

public:
    SeqStmt(Stmt *s1, Stmt *s2);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class If : public Stmt
{
private:
    Expr *expr;
    Stmt *stmt;

public:
    If(Expr *x, Stmt *s);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
    std::string to_string();
};

class Else : public Stmt
{
private:
    Expr *expr;
    Stmt *stmt1;
    Stmt *stmt2;

public:
    Else(Expr *x, Stmt *s1, Stmt *s2);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class While : public Stmt
{
private:
    Expr *expr;
    Stmt *stmt;

public:
    While(Expr *x, Stmt *s);
    While();
    void init(Expr *x, Stmt *s);
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Continue : public Stmt
{
private:
    Stmt *stmt;

public:
    Continue();
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Break : public Stmt
{
private:
    Stmt *stmt;

public:
    Break();
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};

class Return : public Stmt
{
private:
    Function *func;
    Expr *expr;

public:
    Return(Expr *x);
    Return();
    IR::Value *gen(IR::Module *module, IR::IRBuilder *builder, const char *from = "");
};
