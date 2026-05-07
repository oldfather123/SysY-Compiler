#include "abstract_syntax_tree.h"

Parser::Parser(Lexer *l)
{
    lex = l;
    top = new Env(nullptr);
    index = 0;
    move();
}

void Parser::move()
{
    if (index == (int)tokens.size())
        tokens.push_back(lex->scan());
    look = tokens[index++];
}

void Parser::back(int n)
{
    index -= n;
    move();
}

void Parser::match(int t)
{
    if (look->type == t)
    {
        move();
    }
    else
    {
        error("Syntax error: not match token type " + look->to_string() + " with " + std::to_string(t));
    }
}

void Parser::add_builtin_function(IR::Module *module, IR::IRBuilder *builder,
                                  Word *word, Type *retType, std::vector<Id *> args)
{
    SeqExpr *fun_args = nullptr;
    for (int i = (int)args.size() - 1; i >= 0; i--)
    {
        Id *x = args[i];
        fun_args = new SeqExpr(x, fun_args);
    }
    std::vector<IR::BasicType *> arg_types;
    for (auto v : args)
        arg_types.push_back(TypeToArgType(v->type));

    Function *fun = new Function(word, retType, fun_args);
    fun->value = IR::ExternalFunction::create(TypeToIRType(retType), arg_types, word->to_string());
    top->put(word, fun);
    module->addBuiltinFunction(word->to_string(), static_cast<IR::ExternalFunction *>(fun->value));
}

void Parser::init(IR::Module *module, IR::IRBuilder *builder)
{
    // TODO

    Type *intArrayType = new Type("[]", init::ARRAY, -1, Type::Int);
    Type *floatArrayType = new Type("[]", init::ARRAY, -1, Type::Float);
    // top -> put(Word::GETARRAY, new Function(Word::GETARRAY, Type::Void, ));
    add_builtin_function(module, builder, Word::GETINT, Type::Int, std::vector<Id *>());
    add_builtin_function(module, builder, Word::GETCH, Type::Int, std::vector<Id *>());
    add_builtin_function(module, builder, Word::GETFLOAT, Type::Float, std::vector<Id *>());
    add_builtin_function(module, builder, Word::GETARRAY, Type::Int, std::vector<Id *>({new Array(intArrayType, 1)}));
    add_builtin_function(module, builder, Word::GETFARRAY, Type::Int, std::vector<Id *>({new Array(floatArrayType, 1)}));
    add_builtin_function(module, builder, Word::PUTINT, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1)}));
    add_builtin_function(module, builder, Word::PUTCH, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1)}));
    add_builtin_function(module, builder, Word::PUTFLOAT, Type::Void, std::vector<Id *>({new Arg(Type::Float, 1)}));
    add_builtin_function(module, builder, Word::PUTARRAY, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1), new Array(intArrayType, 2)}));
    add_builtin_function(module, builder, Word::PUTFARRAY, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1), new Array(floatArrayType, 2)}));
    add_builtin_function(module, builder, Word::STARTTIME, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1)}));
    add_builtin_function(module, builder, Word::STOPTIME, Type::Void, std::vector<Id *>({new Arg(Type::Int, 1)}));
    // add_builtin_function(module, builder, Word::MEMSET, Type::Void, std::vector<Id *>({new Array(Type::Int, 1), new Arg(Type::Int, 2), new Arg(Type::Int, 3)}));
}

void Parser::program_to_IR(IR::Module *module, IR::IRBuilder *builder)
{
    init(module, builder);
    auto res = program(module, builder);
    res->gen(module, builder);
}

void Parser::error(std::string s)
{
    std::cout << "Error in line" << lex->lines << ": " << s << "\n";
    exit(1);
}

Stmt *Parser::program(IR::Module *module, IR::IRBuilder *builder)
{
    if (look->type == init::END)
        return Stmt::stmt_null;
    Stmt *s = global_declare(module, builder);
    return new SeqStmt(s, program(module, builder));
}

Stmt *Parser::global_declare(IR::Module *module, IR::IRBuilder *builder)
{
    if (look == Word::CONST)
    {
        move();
        Type *t = (Type *)look;
        match(init::BASIC);
        Stmt *temp = decl_variable(t, true);
        match(';');
        return temp;
    }
    move();
    move(); // int a; int a();
    if (look->type == '(')
    {
        back(3);
        return decl_function(module, builder);
    }
    else
    {
        back(3);
        Type *t = (Type *)look;
        match(init::BASIC);
        Stmt *temp = decl_variable(t, false);
        match(';');
        return temp;
    }
}

Stmt *Parser::decl_function(IR::Module *module, IR::IRBuilder *builder)
{
    Type *t = (Type *)look;
    match(init::BASIC);
    Token *tok = look;
    match(init::ID);
    match('(');
    Function *fun_id = new Function(tok, t, nullptr);
    Function::Enclosing = fun_id;
    top->put(tok, fun_id);
    Env *saved = top;
    top = new Env(saved);
    Expr *fun_args = decl_args();
    match(')');
    fun_id->set_args(fun_args);
    fun_id->used = 0;
    Stmt *s = block(true);
    delete top;
    top = saved;
    return new Declare_Function(fun_id, s);
}

Stmt *Parser::decl_variable(Type *t, bool const_decl)
{
    if (look->type == ';')
        return Stmt::stmt_null;
    else if (look->type == ',')
        move();
    Token *name = look;
    match(init::ID);
    Id *id = nullptr;
    if (look->type == '[')
    {
        std::vector<Expr *> _dims;
        while (look->type == '[')
        {
            match('[');
            _dims.push_back(add_sub());
            match(']');
        }
        id = new Array(name, t, _dims, IS_GLOBAL, const_decl);
    }
    else
    {
        id = new Id(name, t, IS_GLOBAL, const_decl);
    }
    top->put(name, id);
    Stmt *s = Stmt::stmt_null;
    if (look->type == '=')
    {
        move();
        s = new Set(id, init_list());
    }
    Stmt *temp = new Declare(t, id, s);
    return new SeqStmt(temp, decl_variable(t, const_decl));
}
// TODO
Expr *Parser::decl_args()
{
    if (look->type == ')')
        return nullptr;
    if (look->type == ',')
        move();
    Type *t = (Type *)look;

    // TODO: array declare need to be added
    match(init::BASIC);
    Token *tok = look;
    match(init::ID);
    Id *id;
    if (look->type == '[')
    {
        std::vector<Expr *> _dims;
        while (look->type == '[')
        {
            match('[');
            if (look->type == ']')
                _dims.push_back(Expr::expr_null);
            else
                _dims.push_back(add_sub());
            match(']');
        }
        id = new Array(tok, t, _dims, false, false);
    }
    else
    {
        id = new Arg(tok, t);
    }
    top->put(tok, id);
    return new SeqExpr(id, decl_args());
}

Stmt *Parser::block(bool created = false)
{
    match('{');
    Env *saved = top;
    if (!created)
        top = new Env(saved);
    Stmt *s = stmts();
    match('}');
    if (!created)
        delete top;
    top = saved;
    return s;
}

Stmt *Parser::stmts()
{
    if (look->type == '}')
    {
        return Stmt::stmt_null;
    }
    else
    {
        Stmt *s = stmt();
        return new SeqStmt(s, stmts());
    }
}

Stmt *Parser::stmt()
{
    if (look == Word::CONST)
    {
        move();
        Type *t = (Type *)look;
        match(init::BASIC);
        Stmt *temp = decl_variable(t, true);
        match(';');
        return temp;
    }
    else if (look->type == init::BASIC)
    {
        Type *t = (Type *)look;
        move();
        Stmt *temp = decl_variable(t, false);
        match(';');
        return temp;
    }
    else if (look->type == ';')
    {
        move();
        return Stmt::stmt_null;
    }
    else if (look->type == init::IF)
    {
        match(init::IF);
        match('(');
        Expr *x = equal();
        match(')');
        Stmt *s1;
        if (look->type == '{')
            s1 = block();
        else
            s1 = stmt();
        if (look->type != init::ELSE)
        {
            return new If(x, s1);
        }
        match(init::ELSE);
        Stmt *s2;
        if (look->type == '{')
            s2 = block();
        else
            s2 = stmt();
        return new Else(x, s1, s2);
    }
    else if (look->type == init::WHILE)
    {
        match(init::WHILE);
        match('(');
        Expr *x = equal();
        match(')');
        While *while_node = new While();
        Stmt *temp = Stmt::Enclosing;
        Stmt::Enclosing = while_node;
        Stmt *s;
        if (look->type == '{')
            s = block();
        else
            s = stmt();
        while_node->init(x, s);
        Stmt::Enclosing = temp;
        return while_node;
    }
    else if (look->type == init::RETURN)
    {
        match(init::RETURN);
        Expr *x = (look->type == ';' ? Expr::expr_null : equal());
        match(';');
        return new Return(x);
    }
    else if (look->type == init::CONTINUE)
    {
        match(init::CONTINUE);
        match(';');
        return new Continue();
    }
    else if (look->type == init::BREAK)
    {
        match(init::BREAK);
        match(';');
        return new Break();
    }
    else if (look->type == '{')
    {
        return block();
    }
    else
    {
        Stmt *temp = new Calculate(equal());
        match(';');
        return temp;
    }
}

/*
重写赋值表达式
需要修改的地方：
实现一个新的类，继承 Binary 类，重写 gen 方法和 reduce 方法
*/

Expr *Parser::equal()
{
    Expr *x = logic_or();
    while (look->type == '=')
    {
        move();
        x = new Assign(x, logic_or());
    }
    return x;
}

Expr *Parser::logic_or()
{
    Expr *x = logic_and();
    while (look->type == init::OR)
    {
        Token *tok = look;
        move();
        x = new Logical(tok, x, logic_and());
    }
    return x;
}

Expr *Parser::logic_and()
{
    Expr *x = bit_or();
    while (look->type == init::AND)
    {
        Token *tok = look;
        move();
        x = new Logical(tok, x, bit_or());
    }
    return x;
}

Expr *Parser::bit_or()
{
    Expr *x = bit_xor();
    while (look->type == '|')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, bit_xor());
    }
    return x;
}

Expr *Parser::bit_xor()
{
    Expr *x = bit_and();
    while (look->type == '^')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, bit_and());
    }
    return x;
}

Expr *Parser::bit_and()
{
    Expr *x = logic_equality();
    while (look->type == '&')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, logic_equality());
    }
    return x;
}

Expr *Parser::logic_equality()
{
    Expr *x = logic_ge_le();
    while (look->type == init::EQ || look->type == init::NE)
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, logic_ge_le());
    }
    return x;
}

Expr *Parser::logic_ge_le()
{
    Expr *x = add_sub();
    while (look->type == init::LE || look->type == '<' ||
           look->type == init::GE || look->type == '>')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, add_sub());
    }
    return x;
}

Expr *Parser::add_sub()
{
    Expr *x = mul_mod_div();
    while (look->type == '+' || look->type == '-')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, mul_mod_div());
    }
    return x;
}

Expr *Parser::mul_mod_div()
{
    Expr *x = unary();
    while (look->type == '*' || look->type == '/' || look->type == '%')
    {
        Token *tok = look;
        move();
        x = new Binary(tok, x, unary());
    }
    return x;
}

Expr *Parser::unary()
{
    if (look->type == '-' || look->type == '!' || look->type == '~' || look->type == '+')
    {
        Token *tok = look;
        move();
        return new Unary(tok, unary());
    }
    return factor();
}

Expr *Parser::factor()
{
    Expr *x = Expr::expr_null;
    if (look->type == '(')
    {
        move();
        x = logic_or();
        match(')');
    }
    else if (look->type == init::INTNUM)
    {
        x = new IntConstant(look, 0);
        move();
    }
    else if (look->type == init::FLOATNUM)
    {
        x = new FloatConstant(look, 0);
        move();
    }
    else if (look->type == init::TRUE)
    {
        x = IntConstant::True;
        move();
    }
    else if (look->type == init::FALSE)
    {
        x = IntConstant::False;
        move();
    }
    else if (look->type == init::ID)
    {
        Id *id = top->get(look);
        if (id == nullptr)
        {
            error(look->to_string() + " undeclared");
        }
        move();
        if (look->type != '(')
        {
            if (id->ID_type == ARRAY_ID)
            {
                Array *array_id = (Array *)id;
                std::vector<Expr *> index;
                while (look->type == '[')
                {
                    match('[');
                    index.push_back(equal());
                    match(']');
                }
                if (!index.empty())
                    x = new Access(array_id, index);
                else
                    x = array_id;
                return x;
            }
            else if (id->ID_type == FUNCTION_ID)
                error(id->to_string() + " is not a variable");
            else
                return id;
        }
        if (id->ID_type != FUNCTION_ID)
            error(id->to_string() + " is not a function");
        
        
        Function *fun_id = (Function *)id;
        if (id->op == Word::STARTTIME || id->op == Word::STOPTIME)
        {
            match('(');
            Expr *args = new SeqExpr(new IntConstant(new IntNumber(lex->lines), false), nullptr);
            x = new Call(look, id->type, fun_id, args);
            match(')');
        } else {
            match('(');
            Expr *args = get_expr();
            match(')');
            x = new Call(look, id->type, fun_id, args);
        }
    }
    else
    {
        error("Syntax error: factor error");
    }
    return x;
}

Expr *Parser::get_expr()
{
    if (look->type == ')')
    {
        return nullptr;
    }
    if (look->type == ',')
        move();
    Expr *args = new SeqExpr(equal(), get_expr());
    return args;
}

Expr *Parser::init_list()
{
    if (look->type == '{')
    {
        InitList *res = new InitList();
        match('{');
        while (true)
        {
            res->add(init_list());
            if (look->type != ',')
                break;
            match(',');
        }
        match('}');
        return res;
    }
    else if (look->type == '}')
        return Expr::expr_null;
    else
        return equal();
}