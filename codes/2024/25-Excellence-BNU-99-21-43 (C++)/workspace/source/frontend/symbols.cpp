#include "symbols.h"

Type *Type::Int = new Type("int", init::BASIC, 4);
Type *Type::Void = new Type("void", init::BASIC, 0);
Type *Type::Float = new Type("float", init::BASIC, 4);
Type *Type::Bool = new Type("bool", init::BASIC, 4);
Word *Word::IF = new Word(init::IF, "if");
Word *Word::ELSE = new Word(init::ELSE, "else");
Word *Word::WHILE = new Word(init::WHILE, "while");
Word *Word::BREAK = new Word(init::BREAK, "break");
Word *Word::CONTINUE = new Word(init::CONTINUE, "continue");
Word *Word::FOR = new Word(init::FOR, "for");
Word *Word::CONST = new Word(init::CONST, "const");
Word *Word::TEMP = new Word(init::ID, "__temp__");
Word *Word::NOTHING = new Word(init::ID, "__nothing__");
Word *Word::RETURN = new Word(init::RETURN, "return");
Word *Word::GETINT = new Word(init::ID, "getint");
Word *Word::GETCH = new Word(init::ID, "getch");
Word *Word::GETFLOAT = new Word(init::ID, "getfloat");
Word *Word::GETARRAY = new Word(init::ID, "getarray");
Word *Word::GETFARRAY = new Word(init::ID, "getfarray");
Word *Word::PUTINT = new Word(init::ID, "putint");
Word *Word::PUTCH = new Word(init::ID, "putch");
Word *Word::PUTFLOAT = new Word(init::ID, "putfloat");
Word *Word::PUTARRAY = new Word(init::ID, "putarray");
Word *Word::PUTFARRAY = new Word(init::ID, "putfarray");
Word *Word::PUTF = new Word(init::ID, "putf");
Word *Word::STARTTIME = new Word(init::ID, "_sysy_starttime");
Word *Word::STOPTIME = new Word(init::ID, "_sysy_stoptime");
Word *Word::MEMSET = new Word(init::ID, "memset");

/*
    Constructor for a token with only a token_type.
*/
Token::Token(int t)
{
    type = t;
}

std::string Token::to_string()
{
    std::string s = "";
    return s + char(type);
}

/*
    Constructor for a token Word(token_type, lexme).
*/
Word::Word(int t, std::string l) : Token(t)
{
    lexme = l;
}

std::string Word::to_string()
{
    return lexme;
}

/*
    Constructor for a token Number(token_type, value).
*/
IntNumber::IntNumber(int x) : Token(init::INTNUM)
{
    value = x;
}

std::string IntNumber::to_string()
{
    return std::to_string(value);
}

/*
    Constructor for a token Number(token_type, value).
*/
FloatNumber::FloatNumber(float x) : Token(init::FLOATNUM)
{
    value_f = x;
}

std::string FloatNumber::to_string()
{
    return std::to_string(value_f);
}
/*
    Constructor for a token Type(token_type, typename, width).
*/
Type::Type(std::string type, int t, int w) : Word(t, type)
{
    width = w;
    length = 1;
    dimension = 0;
    next = nullptr;
}

Type::Type(std::string type, int t, int w, Type *ne) : Word(t, type)
{
    assert(ne != nullptr);
    width = w * ne->width;
    dimension = ne->dimension + 1;
    length = w;
    next = ne;
}

bool Type::numeric(Type *p)
{
    if (p == Type::Int || p == Type::Float || p == Type::Bool)
        return true;
    else
        return false;
}

Type *Type::max(Type *p1, Type *p2)
{
    if (!numeric(p1) || !numeric(p2))
        return nullptr;
    else if (p1 == Type::Float || p2 == Type::Bool)
        return p1;
    else
        return p2;
}