#pragma once
#include <iostream>
#include <assert.h>

#ifdef LOCAL
#include "debug.h"
#else
#define debug(...) \
    do             \
    {              \
    } while (0)
#define pause(...) \
    do             \
    {              \
    } while (0)
#endif

namespace init
{
    const int AND = 256, TRUE = 257, FALSE = 258, NOT = 259, OR = 260,
              INTNUM = 261, ID = 262, IF = 263, ELSE = 264, WHILE = 265,
              BREAK = 266, CONTINUE = 267, FOR = 268, DO = 269, ACCESS = 276,
              NE = 270, EQ = 271, LE = 272, GE = 273, BASIC = 274, RETURN = 275, CONST = 276, END = 277, FLOATNUM = 278, ARRAY = 279;
}
// A token is a single unit of input. It is a pair of a type and a value.
class Token
{
    /*
        Type:
            - Token
            - Number
            - Word
                - Type
                - Array     TODO
    */
public:
    int type;
    int value;
    float value_f;
    std::string lexme;
    Token(int t);
    virtual std::string to_string();
};

class Word : public Token
{
public:
    std::string lexme;
    // Constructor for a token with only a token_type.
    Word(int t, std::string s);
    std::string to_string();
    static Word *IF, *ELSE, *WHILE, *BREAK, *CONTINUE, *FOR, *TEMP, *NOTHING, *RETURN, *CONST;
    static Word *GETINT, *GETCH, *GETFLOAT, *GETARRAY, *GETFARRAY, *PUTINT, *PUTCH, *PUTFLOAT, *PUTARRAY, *PUTFARRAY, *PUTF;
    static Word *STARTTIME, *STOPTIME;
    static Word *MEMSET;
};

class IntNumber : public Token
{
public:
    // Constructor for a token Number(value).
    IntNumber(int v);
    std::string to_string();
};

class FloatNumber : public Token
{
public:
    // Constructor for a token Number(value).
    FloatNumber(float v);
    std::string to_string();
};

class Type : public Word
{
public:
    static Type *Int;
    static Type *Void;
    static Type *Float;
    static Type *Bool;
    static Type *ArrayInt;
    static Type *ArrayFloat;
    int width = 0;
    int length = 0;
    int dimension = 0;
    Type *next;
    // Constructor for a token Type(token_type, typename, width).
    Type(std::string s, int t, int w);
    Type(std::string s, int t, int w, Type *next);
    static bool numeric(Type *p);
    static Type *max(Type *p1, Type *p2);
    static bool isInt(Type *p) { return p == Type::Int || p == Type::Bool; }
    static bool isFloat(Type *p) { return p == Type::Float; }
};
