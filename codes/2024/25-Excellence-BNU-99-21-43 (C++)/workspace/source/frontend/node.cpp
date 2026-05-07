#include "node.h"
#include <cstdio>

#define HANDLE_BINARY_GEN(opt)                                  \
    if ((x->getType()->isInt32()))                              \
        res = builder->Create##opt(x, y, __PRETTY_FUNCTION__);  \
    else if (x->getType()->isFloat())                           \
        res = builder->CreateF##opt(x, y, __PRETTY_FUNCTION__); \
    else                                                        \
        error("type error");

#define HANDLE_BIANTY_GEN_WITHOUT_FLOAT(opt)                   \
    if (x->getType()->isInt32())                               \
        res = builder->Create##opt(x, y, __PRETTY_FUNCTION__); \
    else                                                       \
        error("type error");

int Node::labels = 0;
int Arg::num_counts = 0;
Stmt *Stmt::stmt_null = new Stmt();
Expr *Expr::expr_null = new Expr(Word::NOTHING, Type::Void);
Stmt *Stmt::Enclosing = Stmt::stmt_null;
Function *Function::Enclosing = nullptr;
Function *Function::PrintInt = nullptr;
IntConstant *IntConstant::True = new IntConstant(new IntNumber(1), false);
IntConstant *IntConstant::False = new IntConstant(new IntNumber(0), false);

// Change the frontend Type to IR Type
IR::BasicType *TypeToIRType(Type *x)
{
    if (x == Type::Int || x == Type::Bool)
        return IR::BasicType::getInt32Type();
    else if (x == Type::Void)
        return IR::BasicType::getVoidType();
    else if (x == Type::Float)
        return IR::BasicType::getFloatType();
    else
    {
        return IR::ArrayType::getArrayType(TypeToIRType(x->next), x->length);
    }
    assert(false);
}

IR::BasicType *TypeToArgType(Type *x)
{
    if (x == Type::Int || x == Type::Bool)
        return IR::BasicType::getInt32Type();
    else if (x == Type::Void)
        return IR::BasicType::getVoidType();
    else if (x == Type::Float)
        return IR::BasicType::getFloatType();
    else
    {
        if (x->length == -1)
            return IR::PointerType::get(TypeToIRType(x->next));
        else
            return IR::PointerType::get(TypeToIRType(x));
    }
    assert(false);
}

Node::Node()
{
    lexline = Lexer::lines;
}

void Node::emit(std::string s)
{
    // std::cout << '\t' << s << '\n';
}

int Node::new_label()
{
    label = ++labels;
    return label;
}

void Node::emit_label(int l)
{
    // std::cout << "L" + std::to_string(l) << ": \n";
}

void Node::error(std::string s)
{
    Error::Error(__PRETTY_FUNCTION__, lexline, s);
}

Expr::Expr(Token *tok, Type *p) : Node()
{
    op = tok;
    type = p;
}

IR::Value *Expr::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    if (value == nullptr)
        error("gen error");
    return builder->CreateLoad(TypeToIRType(type), value, __PRETTY_FUNCTION__);
}

IR::Value *Expr::getAddress(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    if (value == nullptr)
        error("gen error");
    assert(value->getType()->isPoint());
    return value;
}

std::string Expr::to_string()
{
    return op->to_string();
}

void Expr::jumping(IR::Module *module, IR::IRBuilder *builder, IR::BasicBlock *t, IR::BasicBlock *f)
{
    IR::Value *x = gen(module, builder);
    if (x == nullptr)
    {
        error("jumping error");
    }
    IR::Value *res = nullptr;
    // 类型转换
    if (CHECKTYPE(type, Int))
    {
        res = builder->CreateNe(x, IR::ConstantInt32::get(0), __PRETTY_FUNCTION__);
    }
    else if (CHECKTYPE(type, Float))
        res = builder->CreateFNe(x, IR::ConstantFloat::get(0), __PRETTY_FUNCTION__);
    else if (CHECKTYPE(type, Bool))
        res = x;
    else
        error("type error");
    builder->CreateCondBr(res, t, f, __PRETTY_FUNCTION__);
}

Expr *Expr::get_expr1()
{
    return this;
}

Expr *Expr::get_expr2()
{
    return nullptr;
}

Stmt::Stmt() {}

IR::Value *Stmt::gen(IR::Module *module, IR::IRBuilder *builder, const char *from) { return nullptr; }

Id::Id(Token *tok, Type *p, bool glo, bool is_cst) : Expr(tok, p)
{
    ExprType = IDEXPR;
    ID_type = VAR_ID;
    is_const = is_cst;
    is_global = glo;
    is_const = is_cst;
    value = nullptr;
}

std::string Id::to_string()
{
    return op->to_string();
}

Arg::Arg(Token *tok, Type *p) : Id(tok, p, false, false)
{
    ID_type = ARG_ID;
    type = p;
    num = num_counts++;
    saved = false;
}

// Only used in __builtin__function arg declaration
Arg::Arg(Type *p, int idx) : Id(nullptr, p, false, false)
{
    saved = false;
    ID_type = ARG_ID;
    type = p;
    num = idx;
    op = new Word(init::ID, "arg" + std::to_string(num));
}

Function::Function(Token *tok, Type *t, Expr *args) : Id(tok, t, true, false)
{
    this->args = args;
    ID_type = FUNCTION_ID;
    used = 0;
    totsz = 128;
}

void Function::emit_label()
{
    std::cout << ".globl " + to_string() + "\n";
    std::cout << to_string() + ":\n";
}

std::string Function::to_string()
{
    std::string res = op->to_string();
    return res;
}

void Function::set_args(Expr *a)
{
    args = a;
}

Op::Op(Token *tok, Type *p) : Expr(tok, p) { ExprType = OPEXPR; }

// IR::Value* Op::reduce() {
//     IR::Value* x = gen();
//     return x
//     Temp* t = new Temp(type);
//     emit(t -> to_string() + " = " + x -> to_string());
//     return t;
// }

Unary::Unary(Token *tok, Expr *expr1) : Op(tok, expr1->type)
{
    is_const = expr1->is_const;
    expr = expr1;
    if (op->type == '!')
        type = Type::Bool;
}

IR::Value *Unary::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Value *x = expr->gen(module, builder);
    IR::Value *res = nullptr;
    if (op->type == '-')
    {
        if (CHECKTYPE(expr->type, Int) || CHECKTYPE(expr->type, Bool))
            res = builder->CreateNeg(x, __PRETTY_FUNCTION__);
        else if (CHECKTYPE(expr->type, Float))
            res = builder->CreateFNeg(x, __PRETTY_FUNCTION__);
        else
            error("type error");
    }
    else if (op->type == '!')
    {
        // 类型转换
        if (CHECKTYPE(expr->type, Int))
            x = builder->CreateNe(x, IR::ConstantInt32::get(0), __PRETTY_FUNCTION__);
        else if (CHECKTYPE(expr->type, Float))
            x = builder->CreateFNe(x, IR::ConstantFloat::get(0), __PRETTY_FUNCTION__);
        else if (CHECKTYPE(expr->type, Bool))
            ;
        else
            error("type error");
        res = builder->CreateNot(x, __PRETTY_FUNCTION__);
    }
    else if (op->type == '+')
    {
        res = x;
    }
    else if (op->type == '~')
    {
        error("~ is not a valid operator");
    }
    else
    {
        error("Unary operator error");
    }
    return res;
}

std::string Unary::to_string()
{
    return op->to_string() + expr->to_string();
}

Binary::Binary(Token *tok, Expr *e1, Expr *e2) : Op(tok, nullptr)
{
    type = Type::max(e1->type, e2->type);
    if (op->type == init::EQ || op->type == init::NE || op->type == '<' || op->type == init::LE || op->type == '>' || op->type == init::GE)
    {
        if (!Type::numeric(e1->type) || !Type::numeric(e2->type))
            error("type error");
        type = Type::Bool;
    }
    is_const = e1->is_const && e2->is_const;
    if (type == nullptr)
        error("Binary type error");
    expr1 = e1;
    expr2 = e2;
}

IR::Value *Binary::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Value *x = expr1->gen(module, builder);
    IR::Value *y = expr2->gen(module, builder);
    // 类型转换
    Type *maxtype = Type::max(expr1->type, expr2->type);
    if (CHECKTYPE(maxtype, Int))
    {
        if (CHECKTYPE(expr1->type, Float))
            x = builder->CreateFPtoSI(x, __PRETTY_FUNCTION__);
        if (CHECKTYPE(expr2->type, Float))
            y = builder->CreateFPtoSI(y, __PRETTY_FUNCTION__);
    }
    else if (CHECKTYPE(maxtype, Float))
    {
        if (CHECKTYPE(expr1->type, Int) || CHECKTYPE(expr1->type, Bool))
            x = builder->CreateSItoFP(x, __PRETTY_FUNCTION__);
        if (CHECKTYPE(expr2->type, Int) || CHECKTYPE(expr2->type, Bool))
            y = builder->CreateSItoFP(y, __PRETTY_FUNCTION__);
    }

    assert(x->getType() == y->getType());
    IR::Value *res = nullptr;
    if (op->type == '+')
    {
        HANDLE_BINARY_GEN(Add);
    }
    else if (op->type == '-')
    {
        HANDLE_BINARY_GEN(Sub);
    }
    else if (op->type == '*')
    {
        HANDLE_BINARY_GEN(Mul);
    }
    else if (op->type == '/')
    {
        HANDLE_BINARY_GEN(Div);
    }
    else if (op->type == '%')
    {
        HANDLE_BIANTY_GEN_WITHOUT_FLOAT(Rem);
    }
    else if (op->type == '&')
    {
        HANDLE_BIANTY_GEN_WITHOUT_FLOAT(And);
    }
    else if (op->type == '|')
    {
        HANDLE_BIANTY_GEN_WITHOUT_FLOAT(Or);
    }
    else if (op->type == '^')
    {
        HANDLE_BIANTY_GEN_WITHOUT_FLOAT(Xor);
    }
    // else if (op -> type == init::AND) res = builder -> CreateLogicAnd(x, y, __PRETTY_FUNCTION__);
    // else if (op -> type == init::OR) res = builder -> CreateLogicOr(x, y, __PRETTY_FUNCTION__);
    else if (op->type == init::EQ)
    {
        HANDLE_BINARY_GEN(Eq);
    }
    else if (op->type == init::NE)
    {
        HANDLE_BINARY_GEN(Ne);
    }
    else if (op->type == '<')
    {
        if ((x->getType()->isInt32()))
            res = builder->CreateLt(x, y, __PRETTY_FUNCTION__);
        else if (x->getType()->isFloat())
            res = builder->CreateFLt(x, y, __PRETTY_FUNCTION__);
        else
            error("type error");
    }
    else if (op->type == init::LE)
    {
        HANDLE_BINARY_GEN(Le);
    }
    else if (op->type == '>')
    {
        HANDLE_BINARY_GEN(Gt);
    }
    else if (op->type == init::GE)
    {
        HANDLE_BINARY_GEN(Ge);
    }
    else
        error(op->to_string() + " is not a valid operator");
    return res;
}

std::string Binary::to_string()
{
    return expr1->to_string() + " " + op->to_string() + " " + expr2->to_string();
}

Assign::Assign(Expr *i, Expr *x) : Op(new Token('='), i->type)
{
    assert(dynamic_cast<Id *>(i) || dynamic_cast<Access *>(i));
    id = i;
    expr = x;
}

IR::Value *Assign::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Value *x = expr->gen(module, builder);
    IR::Value *y = id->getAddress(module, builder);
    // 类型转换
    if ((CHECKTYPE(expr->type, Int) || CHECKTYPE(expr->type, Bool)) && CHECKTYPE(id->type, Float))
    {
        x = builder->CreateSItoFP(x, __PRETTY_FUNCTION__);
    }
    else if (CHECKTYPE(expr->type, Float) && CHECKTYPE(id->type, Int))
    {
        x = builder->CreateFPtoSI(x, __PRETTY_FUNCTION__);
    }
    IR::Value *res = builder->CreateStore(x, y, __PRETTY_FUNCTION__);
    return res;
}

std::string Assign::to_string()
{
    return id->to_string() + " = " + expr->to_string();
}

Access::Access(Id *e1, std::vector<Expr *> e2) : Op(new Token(init::ACCESS), e1->type)
{
    array = e1;
    index = e2;

    if (array->ID_type != ARRAY_ID)
        error("not an array");
}

Access::Access(Id *e1, const std::vector<int> &e2) : Op(new Token(init::ACCESS), e1->type)
{
    array = e1;
    std::vector<Expr *> e;
    for (auto i : e2)
        e.push_back(new IntConstant(new IntNumber(i), false));
    index = e;
    if (array->ID_type != ARRAY_ID)
        error("not an array");
}

IR::Value *Access::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    std::vector<IR::Value *> newindex;
    if (static_cast<Array *>(array)->dims[0] != Expr::expr_null)
        newindex.push_back(IR::ConstantInt32::get(0));
    // Access type 创建
    type = array->type;
    for (Expr *idx : index)
    {
        IR::Value *x = idx->gen(module, builder);
        type = type->next;
        newindex.push_back(x);
    }
    if (Type::numeric(type))
    {
        IR::Value *point = builder->CreateGEP(array->getAddress(module, builder), newindex);
        auto temp = builder->CreateLoad(TypeToIRType(type), point, __PRETTY_FUNCTION__);
        return temp;
    }
    else
    {
        return builder->CreateGEP(array->getAddress(module, builder), newindex, __PRETTY_FUNCTION__);
    }
}

IR::Value *Access::getAddress(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    std::vector<IR::Value *> newindex;
    if (static_cast<Array *>(array)->dims[0] != Expr::expr_null)
        newindex.push_back(IR::ConstantInt32::get(0));
    // Access type 创建
    type = array->type;
    for (Expr *idx : index)
    {
        IR::Value *x = idx->gen(module, builder);
        type = type->next;
        newindex.push_back(x);
    }
    auto temp = builder->CreateGEP(array->getAddress(module, builder), newindex, __PRETTY_FUNCTION__);
    return temp;
}

std::string Access::to_string()
{
    std::string res = array->to_string();
    for (auto idx : index)
    {
        res += "[" + idx->to_string() + "]";
    }
    return res;
}

Logical::Logical(Token *tok, Expr *e1, Expr *e2) : Binary(tok, e1, e2)
{
    if (Type::numeric(e1->type) && Type::numeric(e2->type))
    {
        type = Type::Bool;
    }
    else
    {
        error("type error");
    }
}

void Logical::jumping(IR::Module *module, IR::IRBuilder *builder, IR::BasicBlock *t, IR::BasicBlock *f)
{
    IR::BasicBlock *next = IR::BasicBlock::get("next");
    if (op->type == init::OR)
    {
        expr1->jumping(module, builder, t, next);
        builder->SetInsertPoint(next);
        IR::Function *currentfun = static_cast<IR::Function *>(Function::Enclosing->value);
        currentfun->addBlock(next);
        expr2->jumping(module, builder, t, f);
    }
    else if (op->type == init::AND)
    {
        expr1->jumping(module, builder, next, f);
        builder->SetInsertPoint(next);
        IR::Function *currentfun = static_cast<IR::Function *>(Function::Enclosing->value);
        currentfun->addBlock(next);
        expr2->jumping(module, builder, t, f);
    }
}

Call::Call(Token *tok, Type *t, Function *f, Expr *a) : Expr(tok, t)
{
    function = f;
    args = a;
    fun_args = f->args;
}

IR::Value *Call::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    Expr *args_idx = this->args;
    Expr *fun_args_idx = this->fun_args;
    std::vector<IR::Value *> args;
    while (args_idx != nullptr && fun_args_idx != nullptr)
    {
        Expr *nowExpr = args_idx->get_expr1();
        Expr *nowArg = fun_args_idx->get_expr1();
        IR::Value *x = nowExpr->gen(module, builder);
        // 类型转换
        if (CHECKTYPE(nowArg->type, Int) && CHECKTYPE(nowExpr->type, Float))
        {
            x = builder->CreateFPtoSI(x, __PRETTY_FUNCTION__);
        }
        else if (CHECKTYPE(nowArg->type, Float) && (CHECKTYPE(nowExpr->type, Int) || CHECKTYPE(nowExpr->type, Bool)))
        {
            x = builder->CreateSItoFP(x, __PRETTY_FUNCTION__);
        }
        else if (static_cast<Id *>(nowArg)->ID_type == ARRAY_ID)
        {
            if (x->isInstruction())
            {
                auto instr = static_cast<IR::Instruction *>(x);
                if (instr->getOpcode() == IR::Instruction::GEP)
                {
                    x->type = TypeToArgType(nowArg->type);
                }
                else if (instr->getOpcode() == IR::Instruction::Alloca && instr->getType()->getBaseType()->isArray())
                {
                    x = builder->CreateGEP(TypeToArgType(nowArg->type), instr, {IR::ConstantInt32::get(0)});
                }
                else
                {
                    assert(x->getType() == TypeToArgType(nowArg->type));
                }
            }
            else if (x->isGlobalVariable())
            {
                assert(x->getType()->getBaseType()->isArray());
                x = builder->CreateGEP(TypeToArgType(nowArg->type), x, {IR::ConstantInt32::get(0)});
            }
            else
            {
                error("value must be array or gep");
            }
        }
        args.push_back(x);

        // change to the next sequence
        args_idx = args_idx->get_expr2();
        fun_args_idx = fun_args_idx->get_expr2();
    }
    if (args_idx != nullptr || fun_args_idx != nullptr)
        error("function call args error");

    return builder->CreateCall(static_cast<IR::Function *>(function->value), args, __PRETTY_FUNCTION__);
}

std::string Call::to_string()
{
    return "call " + function->to_string();
}

InitList::InitList(std::vector<Expr *> e) : Expr(nullptr, nullptr)
{
    ExprType = INITEXPR;
    exprs = e;
}

InitList::InitList() : Expr(nullptr, nullptr)
{
    ExprType = INITEXPR;
    exprs = std::vector<Expr *>();
}

std::string InitList::to_string()
{
    std::string res = "{}";
    return res;
}

void InitList::add(Expr *e)
{
    exprs.push_back(e);
}

IntConstant::IntConstant(Token *tok, bool glob) : Id(tok, Type::Int, glob, true)
{
    my_value.ivar = tok->value;
    ID_type = VAR_ID;
    value = IR::ConstantInt32::get(my_value.ivar);
}

IR::Value *IntConstant::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    return value;
}

std::string IntConstant::to_string()
{
    return std::to_string(my_value.ivar);
}

FloatConstant::FloatConstant(Token *tok, bool glob) : Id(tok, Type::Float, glob, true)
{
    my_value.fvar = tok->value_f;
    ID_type = VAR_ID;
    value = IR::ConstantFloat::get(my_value.fvar);
}

IR::Value *FloatConstant::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    return value;
}

std::string FloatConstant::to_string()
{
    return std::to_string(my_value.fvar);
}

Array::Array(Token *x, Type *p, std::vector<Expr *> _dims, bool glob, bool is_cst) : Id(x, p, glob, is_cst)
{
    dims = _dims;
    ID_type = ARRAY_ID;
}

Array::Array(Type *p, int idx) : Id(nullptr, p, false, false)
{
    // arg must
    ID_type = ARRAY_ID;
    dims = std::vector<Expr *>({Expr::expr_null});
    op = new Word(init::ID, "arg" + std::to_string(idx));
}

Set::Set(Id *i, Expr *x)
{
    id = i;
    expr = x;
}

void Set::store(IR::Module *module, IR::IRBuilder *builder, Expr *id, Expr *expr)
{
    IR::Value *x = expr->gen(module, builder);
    IR::Value *lval = id->getAddress(module, builder);
    if ((CHECKTYPE(expr->type, Int) || CHECKTYPE(expr->type, Bool)) && CHECKTYPE(id->type, Float))
    {
        x = builder->CreateSItoFP(x, __PRETTY_FUNCTION__);
    }
    else if (CHECKTYPE(expr->type, Float) && CHECKTYPE(id->type, Int))
    {
        x = builder->CreateFPtoSI(x, __PRETTY_FUNCTION__);
    }
    builder->CreateStore(x, lval, __PRETTY_FUNCTION__);
}

bool Set::arrayAdd(IR::Module *module, IR::IRBuilder *builder, std::vector<int> &idx, std::vector<int> &dim_size, int edge, int where)
{
    int now = where;
    for (int i = where + 1; i < (int)dim_size.size(); i++)
    {
        idx[i] = 0;
    }
    idx[now]++;
    while (now > 0 && idx[now] == dim_size[now])
    {
        idx[now] = 0;
        now--;
        idx[now]++;
    }
    return true;
}

void Set::parseInitlist(IR::Module *module,
                        IR::IRBuilder *builder, Array *id,
                        std::vector<int> &dims, int edge,
                        InitList *expr, std::vector<pve> &initres)
{
    std::vector<Expr *> inits = expr->exprs;
    int purpose = -1;
    if (edge == 1)
        purpose = dims[edge - 1] + 1;
    else if (edge > 1)
        purpose = (dims[edge - 1] + 1) % id->dim_size[edge - 1];
    for (auto v : inits)
    {
        if (v->ExprType == INITEXPR)
        {
            parseInitlist(module, builder, id, dims, edge + 1, dynamic_cast<InitList *>(v), initres);
        }
        else
        {
            if (v != Expr::expr_null)
            {
                if (v->type == Type::Void)
                    error("type error");
                std::vector<int> temp = dims;
                initres.push_back({temp, v});
                arrayAdd(module, builder, dims, id->dim_size, edge, dims.size() - 1);
            }
        }
    }
    if (edge && dims[edge - 1] != purpose)
    {
        if (edge == 1)
            assert(purpose == dims[edge - 1] + 1);
        else if (edge > 1)
            assert(purpose == (dims[edge - 1] + 1) % id->dim_size[edge - 1]);
        arrayAdd(module, builder, dims, id->dim_size, edge, edge - 1);
    }
}

// 分为 const 直接初始化，如global定义和const alloca。
// 和非const初始化，如alloca
void Set::arrayStore(IR::Module *module, IR::IRBuilder *builder, Array *id, InitList *expr)
{
    std::vector<int> dim_size = id->dim_size;
    std::vector<int> dims(dim_size.size(), 0);
    std::vector<pve> initres;
    parseInitlist(module, builder, id, dims, 0, expr, initres);
    if (id->is_global || id->is_const)
    {
        IR::ConstantArray *init = IR::ConstantArray::get(TypeToIRType(id->type));
        for (auto &[dim, expr] : initres)
        {
            if (!expr->is_const)
                error("global or const variable must be initialized with constant");
            auto value = static_cast<IR::Constant *>(expr->gen(module, builder));
            auto idx = init;
            auto typeidx = id->type;
            for (int x : dim)
            {
                typeidx = typeidx->next;
                if (Type::numeric(typeidx))
                {
                    if (Type::isInt(typeidx) && value->isConstantFloat())
                        value = dynamic_cast<IR::Constant *>(builder->CreateFPtoSI(value, __PRETTY_FUNCTION__));
                    else if (Type::isFloat(typeidx) && value->isConstantInt32())
                        value = dynamic_cast<IR::Constant *>(builder->CreateSItoFP(value, __PRETTY_FUNCTION__));
                    idx->addIndex(TypeToIRType(typeidx), x, value);
                }
                else
                {
                    idx = idx->addIndex(TypeToIRType(typeidx), x);
                }
            }
            if (!id->is_global)
            {
                auto yue = new Access(id, dim);
                store(module, builder, yue, expr);
            }
        }
        id->value->setInitializer(init);
    }
    else
    {
        for (auto &[dim, expr] : initres)
        {
            auto yue = new Access(id, dim);
            store(module, builder, yue, expr);
        }
    }
}

IR::Value *Set::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Value *dest = id->getAddress(module, builder);
    if (dest == nullptr)
        Error::Error(__PRETTY_FUNCTION__, lexline, "Array is not allocated");
    if (id->ID_type == ARRAY_ID)
    {
        if (expr->ExprType != INITEXPR)
            Error::Error(__PRETTY_FUNCTION__, lexline, "Array must be initialized with a list");
        arrayStore(module, builder, static_cast<Array *>(id), static_cast<InitList *>(expr));
    }
    else
    {
        Expr *setExpr = expr;
        if (setExpr->ExprType == INITEXPR)
        {
            InitList *initExpr = static_cast<InitList *>(setExpr);
            if (initExpr->exprs.size() > 1)
            {
                Error::Error(__PRETTY_FUNCTION__, lexline, "Multiple values in initialization of a non-array variable");
            }
            else if (initExpr->exprs.size() == 0)
                Error::Error(__PRETTY_FUNCTION__, "InitList error");
            else
            {
                setExpr = initExpr->exprs[0];
                if (setExpr->ExprType == INITEXPR)
                    Error::Error(__PRETTY_FUNCTION__, lexline, "Multiple {} in initialization of a non-array variable");
                if (setExpr == Expr::expr_null)
                    setExpr = new IntConstant(new IntNumber(0), false);
            }
        }

        if (id->is_global || id->is_const)
        {
            IR::Value *res = (setExpr->gen(module, builder));
            assert(res->isConstantFloat() || res->isConstantInt32());
            if ((CHECKTYPE(id->type, Int) || CHECKTYPE(id->type, Bool)) && CHECKTYPE(setExpr->type, Float))
                res = builder->CreateFPtoSI(res, __PRETTY_FUNCTION__);
            else if (CHECKTYPE(id->type, Float) && (CHECKTYPE(setExpr->type, Int) || CHECKTYPE(setExpr->type, Bool)))
                res = builder->CreateSItoFP(res, __PRETTY_FUNCTION__);
            IR::Constant *constVal;
            if (res->isConstantFloat())
                constVal = static_cast<IR::ConstantFloat *>(res);
            else
                constVal = static_cast<IR::ConstantInt32 *>(res);
            id->value->setInitializer(constVal);
            if (!id->is_global)
                store(module, builder, id, setExpr);
        }
        else
        {
            store(module, builder, id, setExpr);
        }
    }

    return nullptr;
}

Calculate::Calculate(Expr *x)
{
    expr = x;
}

IR::Value *Calculate::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    expr->gen(module, builder);
    return nullptr;
}

std::string Calculate::to_string()
{
    return expr->to_string();
}

Declare::Declare(Type *t, Id *i, Stmt *x)
{
    type = t;
    id = i;
    expr = x;
}

IR::Value *Declare::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    if (id->ID_type == ARRAY_ID)
    {
        std::vector<Expr *> dims = ((Array *)id)->dims;
        std::vector<int> dim_values;
        Type *prev = type;
        for (int i = dims.size() - 1; i >= 0; i--)
        {
            IR::Value *temp = dims[i]->gen(module, builder);
            assert(temp);
            assert(temp->isConstantInt32());
            int x = static_cast<IR::ConstantInt32 *>(temp)->getValue();
            dim_values.push_back(x);
            prev = new Type("[" + std::to_string(x) + "]", init::ARRAY, x, prev);
        }
        std::reverse(dim_values.begin(), dim_values.end());
        type = prev;
        id->type = prev;
        ((Array *)id)->set_size(dim_values);
    }
    if (id->is_global)
    {
        id->value = IR::GlobalVariable::create(TypeToIRType(type), id->to_string(), id->is_const);
        module->addGlobal(static_cast<IR::GlobalVariable *>(id->value));
    }
    else
    {
        std::string id_str = id->to_string();
        id->value = builder->CreateAlloca(TypeToIRType(type), id_str, id->is_const, __PRETTY_FUNCTION__);
        if (id->ID_type == ARRAY_ID)
        {
            int arraySize = id->value->getType()->getBaseType()->getSize();
            IR::Function *memsetFunc = module->getBuiltinFunction("memset");
            builder->CreateCall(memsetFunc, {id->value, IR::ConstantInt32::get(0), IR::ConstantInt32::get(arraySize)}, __PRETTY_FUNCTION__);
        }
    }
    emit((id->is_const ? "Const " : "") + type->to_string() + " " + id->to_string());
    expr->gen(module, builder);
    return nullptr;
}

Declare_Function::Declare_Function(Function *f, Stmt *s, bool is_builtin)
{
    function = f;
    stmt = s;
    builtin_def = false;
}

IR::Value *Declare_Function::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    // function -> emit_label();
    // 创建函数, 初始化参数等
    std::string func_str = function->to_string();
    function->value = IR::Function::create(TypeToIRType(function->type), func_str);
    IR::Function *currentFunction = static_cast<IR::Function *>(function->value);

    Expr *idx = function->args;
    std::vector<Id *> yue;
    while (idx != nullptr)
    {
        Id *arg = static_cast<Id *>(idx->get_expr1());
        yue.push_back(arg);
        std::string arg_str = arg->to_string();
        if (arg->ID_type == ARRAY_ID)
        {
            std::vector<Expr *> dims = ((Array *)arg)->dims;
            Type *prev = arg->type;
            for (int i = dims.size() - 1; i >= 0; i--)
            {
                int length = -1;
                std::string name;
                if (dims[i] != Expr::expr_null)
                {
                    IR::Value *temp = dims[i]->gen(module, builder);
                    assert(temp && temp->isConstantInt32());
                    name = temp->getIRName();
                    length = static_cast<IR::ConstantInt32 *>(temp)->getValue();
                }
                prev = new Type("[" + name + "]", init::ARRAY, length, prev);
            }
            arg->type = prev;
        }
        currentFunction->addArgument(TypeToArgType(arg->type));
        arg->value = currentFunction->args().back();
        idx = idx->get_expr2();
    }

    module->addGlobal(currentFunction);

    Function::Enclosing = function;
    IR::BasicBlock *entry = IR::BasicBlock::get(function->to_string() + "_entry");
    builder->SetInsertPoint(entry);
    currentFunction->addBlock(entry);
    for (auto v : yue)
    {
        auto res = builder->CreateAlloca(TypeToArgType(v->type), v->to_string(), false, __PRETTY_FUNCTION__);
        builder->CreateStore(v->value, res, __PRETTY_FUNCTION__);
        v->value = res;
        if (v->ID_type == ARRAY_ID)
            static_cast<Array *>(v)->useforarg = true;
    }
    // IR::BasicBlock* start =  IR::BasicBlock::get(function->to_string() + "start");
    // builder->SetInsertPoint(start);
    // currentFunction->addBlock(start);
    stmt->gen(module, builder, __PRETTY_FUNCTION__);
    if (currentFunction->getReturnType()->isVoid())
    {
        builder->CreateRetVoid(currentFunction);
        IR::BasicBlock *next = IR::BasicBlock::get(function->to_string() + "_unreachable");
        builder->SetInsertPoint(next);
        currentFunction->addBlock(next);
    }
    else
    {
        // builder->CreateRet(IR::Constant::getZeroValueForType(currentFunction->getReturnType()), currentFunction);
    }

    return nullptr;
}

SeqStmt::SeqStmt(Stmt *s1, Stmt *s2) : Stmt()
{
    stmt1 = s1;
    stmt2 = s2;
}

IR::Value *SeqStmt::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    if (stmt1 == Stmt::stmt_null)
    {
        stmt2->gen(module, builder);
    }
    else if (stmt2 == Stmt::stmt_null)
    {
        stmt1->gen(module, builder);
    }
    else
    {
        stmt1->gen(module, builder);
        stmt2->gen(module, builder);
    }
    return nullptr;
}

SeqExpr::SeqExpr(Expr *e1, Expr *e2) : Expr(nullptr, nullptr)
{
    expr1 = e1;
    expr2 = e2;
}

IR::Value *SeqExpr::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    error("SeqExpr gen error");
    return nullptr;
}

Expr *SeqExpr::get_expr1()
{
    return expr1;
}

Expr *SeqExpr::get_expr2()
{
    return expr2;
}

If::If(Expr *x, Stmt *s)
{
    expr = x;
    stmt = s;
    // if (expr -> type != Type::Bool) error("boolean required in if");
}

IR::Value *If::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::BasicBlock *then, *merge;
    then = IR::BasicBlock::get("if_then");
    merge = IR::BasicBlock::get("if_merge");
    expr->jumping(module, builder, then, merge);
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    builder->SetInsertPoint(then);
    currentFunction->addBlock(then);
    stmt->gen(module, builder);
    builder->SetInsertPoint(merge);
    currentFunction->addBlock(merge);
    return nullptr;
}

Else::Else(Expr *x, Stmt *s1, Stmt *s2)
{
    expr = x;
    stmt1 = s1;
    stmt2 = s2;
    // if (expr -> type != Type::Bool) error("boolean required in if");
}

IR::Value *Else::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    IR::BasicBlock *then, *else_, *merge;
    then = IR::BasicBlock::get("if_then");
    else_ = IR::BasicBlock::get("if_else");
    merge = IR::BasicBlock::get("if_else_merge");
    expr->jumping(module, builder, then, else_);
    builder->SetInsertPoint(then);
    currentFunction->addBlock(then);
    stmt1->gen(module, builder);
    builder->CreateBr(merge);
    builder->SetInsertPoint(else_);
    currentFunction->addBlock(else_);
    stmt2->gen(module, builder);
    builder->CreateBr(merge);
    builder->SetInsertPoint(merge);
    currentFunction->addBlock(merge);
    return nullptr;
}

While::While(Expr *x, Stmt *s)
{
    expr = x;
    stmt = s;
}

While::While()
{
    expr = Expr::expr_null;
    stmt = Stmt::stmt_null;
}

void While::init(Expr *x, Stmt *s)
{
    expr = x;
    stmt = s;
}

IR::Value *While::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    IR::BasicBlock *CondBlock, *Then, *AfterWhile;
    CondBlock = IR::BasicBlock::get("while_Cond");
    Then = IR::BasicBlock::get("While_then");
    AfterWhile = IR::BasicBlock::get("While_merge");
    after = AfterWhile;
    begin = CondBlock;
    builder->SetInsertPoint(CondBlock);
    currentFunction->addBlock(CondBlock);
    expr->jumping(module, builder, Then, AfterWhile);
    builder->SetInsertPoint(Then);
    currentFunction->addBlock(Then);
    stmt->gen(module, builder);
    builder->CreateBr(CondBlock);
    builder->SetInsertPoint(AfterWhile);
    currentFunction->addBlock(AfterWhile);
    return nullptr;
}

Continue::Continue()
{
    if (Stmt::Enclosing == Stmt::stmt_null)
        error("unenclosed continue");
    stmt = Stmt::Enclosing;
}

IR::Value *Continue::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    builder->CreateBr(stmt->begin, __PRETTY_FUNCTION__);
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    IR::BasicBlock *next = IR::BasicBlock::get("unreachable");
    builder->SetInsertPoint(next);
    currentFunction->addBlock(next);
    return nullptr;
}

Break::Break()
{
    if (Stmt::Enclosing == Stmt::stmt_null)
        error("unenclosed break");
    stmt = Stmt::Enclosing;
}

IR::Value *Break::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    builder->CreateBr(stmt->after, __PRETTY_FUNCTION__);
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    IR::BasicBlock *next = IR::BasicBlock::get("unreachable");
    builder->SetInsertPoint(next);
    currentFunction->addBlock(next);
    return nullptr;
}

Return::Return(Expr *x)
{
    expr = x;
    if (Function::Enclosing == nullptr)
        error("unenclosed return");
    func = Function::Enclosing;
}

Return::Return()
{
    expr = Expr::expr_null;
    if (Function::Enclosing == nullptr)
        error("?unenclosed return?");
    func = Function::Enclosing;
}

IR::Value *Return::gen(IR::Module *module, IR::IRBuilder *builder, const char *from)
{
    IR::Function *retFun = static_cast<IR::Function *>(func->value);
    if (expr == Expr::expr_null)
    {
        builder->CreateRetVoid(retFun, __PRETTY_FUNCTION__);
    }
    else
    {
        IR::Value *x = expr->gen(module, builder);
        // 类型转换
        if (CHECKTYPE(func->type, Int) && CHECKTYPE(expr->type, Float))
        {
            x = builder->CreateFPtoSI(x, __PRETTY_FUNCTION__);
        }
        else if (CHECKTYPE(func->type, Float) && (CHECKTYPE(expr->type, Int) || CHECKTYPE(expr->type, Bool)))
        {
            x = builder->CreateSItoFP(x, __PRETTY_FUNCTION__);
        }
        builder->CreateRet(x, retFun, __PRETTY_FUNCTION__);
    }

    IR::BasicBlock *next = IR::BasicBlock::get("unreachable");
    builder->SetInsertPoint(next);
    IR::Function *currentFunction = static_cast<IR::Function *>(Function::Enclosing->value);
    currentFunction->addBlock(next);
    return nullptr;
}

#undef HANDLE_BINARY_GEN
#undef HANDLE_BIANTY_GEN_WITHOUT_FLOAT