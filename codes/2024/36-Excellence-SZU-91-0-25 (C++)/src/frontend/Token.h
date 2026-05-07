#pragma once

#include "../utility/System.h"
#include "../utility/Cursor.h"
#include "../utility/Type.h"

enum TokenType
{
    //Keyword
    CONST,T_VOID,T_INT,T_FLOAT,
    IF,ELSE,WHILE,
    BREAK,CONTINUE,RETURN,

    //Identifier  
    IDENTIFIER,

    //Separator
    SEMICOLON,COMMA, // ; ,
    LEFT_PAREN,RIGHT_PAREN, // ( )
    LEFT_SQUARE,RIGHT_SQUARE, // [ ] 
    LEFT_BRACE,RIGHT_BRACE, // { }

    //Operator
    PLUS,MINUS,STAR,SLASH,PERCENT, // + - * / %
    
    ASSIGN, // =

    EQUAL,NOT_EQUAL, // == !=
    LESS,LESS_EQUAL, // <  <=
    GREATER,GREATER_EQUAL, // >  >=
    T_AND,T_OR,T_NOT, // && || !
    
    //Constant
    CONSTANT_INT,CONSTANT_FLOAT,

    //End
    CODE_EOF
};

static vector<string> TokenTypeText
{
    //Keyword
    "CONST","VOID","INT","FLOAT",
    "IF","ELSE","WHILE",
    "BREAK","CONTINUE","RETURN",

    //Identifier  
    "IDENTIFIER",

    //Separator
    "SEMICOLON","COMMA", // ; ,
    "LEFT_PAREN","RIGHT_PAREN", // ( )
    "LEFT_SQUARE","RIGHT_SQUARE", // [ ] 
    "LEFT_BRACE","RIGHT_BRACE", // { }

    //Operator
    "PLUS","MINUS","STAR","SLASH","PERCENT", // + - * / %
    
    "ASSIGN", // =

    "EQUAL","NOT_EQUAL", // == !=
    "LESS","LESS_EQUAL", // <  <=
    "GREATER","GREATER_EQUAL", // >  >=
    "AND","OR","NOT", // && || !
    
    //Constant
    "CONSTANT_INT","CONSTANT_FLOAT",

    //End
    "CODE_EOF"
};

static unordered_map<string,TokenType>KeywordMap
{
    {"const",CONST},{"void",T_VOID},{"int",T_INT},{"float",T_FLOAT},
    {"if",IF},{"else",ELSE},{"while",WHILE},
    {"break",BREAK},{"continue",CONTINUE},{"return",RETURN},
};

class Cursor;

class Token
{
public:
    bool is_valid;
    TokenType token_type;
    string lexeme;
    Cursor location;

    Token():is_valid(false){}
    Token(TokenType token_type,string lexeme,Cursor location):
        is_valid(true),token_type(token_type),lexeme(lexeme),location(location){}
    
    DataType ToDataType();
};