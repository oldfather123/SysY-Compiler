#pragma once

#include "../utility/System.h"
#include "../utility/Cursor.h"
#include "../frontend/Token.h"

class Scanner
{
private:
    Cursor start;
    Cursor current;

public:
    void ScanTokens();
    void ScanToken();
    void AddToken(TokenType token_type);
    void ScanNumber();
    void ScanIdentifierKeyword();

    //Tool
    bool IsAtEnd();
    char Next();
    char Last();
    bool Match(char expected);
    char Peek();
    
    bool IsDigit(char c);
    bool IsAlpha(char c);
    bool IsAlphaUnderline(char c);
    bool IsDigitAlphaUnderline(char c);
    bool IsBlank(char c);
};

extern Scanner scanner;
