#include "../frontend/Scanner.h"
#include "../utility/Product.h"
#include "../utility/Diagnostor.h"

Scanner scanner;

void Scanner::ScanTokens()
{
    while(!IsAtEnd())
    {
        start=current;
        ScanToken();
    }

    //Mark token(code) end
    tokens.push_back(Token(CODE_EOF,"",current));
}

void Scanner::ScanToken()
{
    char c=Next();
    switch(c)
    {
        //Separator
        case ';': AddToken(SEMICOLON);      break;
        case ',': AddToken(COMMA);          break;
        case '(': AddToken(LEFT_PAREN);     break;
        case ')': AddToken(RIGHT_PAREN);    break;
        case '[': AddToken(LEFT_SQUARE);    break;
        case ']': AddToken(RIGHT_SQUARE);   break;
        case '{': AddToken(LEFT_BRACE);     break;
        case '}': AddToken(RIGHT_BRACE);    break;

        //Operator
        case '+': AddToken(PLUS);           break;
        case '-': AddToken(MINUS);          break;
        case '*': AddToken(STAR);           break;
        case '%': AddToken(PERCENT);        break;
        case '=': AddToken(Match('=')?EQUAL:ASSIGN);            break;
        case '!': AddToken(Match('=')?NOT_EQUAL:T_NOT);         break;
        case '<': AddToken(Match('=')?LESS_EQUAL:LESS);         break;
        case '>': AddToken(Match('=')?GREATER_EQUAL:GREATER);   break;
        case '&':
            if(Match('&'))AddToken(T_AND);
            else ScanError("Only '&' is illegal",start);
            break;
        case '|':
            if(Match('|'))AddToken(T_OR);
            else ScanError("Only '|' is illegal",start);
            break;

        //Comment
        case '/':
            if(Match('/'))while(!IsAtEnd()&&Peek()!='\n')Next();
            else if(Match('*'))
            {
                while(!IsAtEnd()&&(Next()!='*'||Peek()!='/'));
                if(IsAtEnd())ScanError("Incomplete comment",current);
                else Next();
            }
            else AddToken(SLASH);
            break;

        //Blank
        case ' ': case '\r': case '\t': case '\n':  
        break;
        
        default:
        //Constant int float
        if(IsDigit(c)||c=='.')ScanNumber();

        //Identifier and Keyword
        else if(IsAlphaUnderline(c))ScanIdentifierKeyword();

        else ScanError("Unexpected character",start);
        break;
    }
}

void Scanner::AddToken(TokenType token_type)
{
    string lexeme=source[current.line].substr(start.column,current.column-start.column);
    tokens.push_back(Token(token_type,lexeme,start));
}

//@Todo:Error check & complex number scan
void Scanner::ScanNumber()
{
    bool is_float=Last()=='.';
    
    //As you see, it is very rough... ...Complete grammar is difficult
    while(!IsAtEnd()&&(IsAlpha(Peek())||IsDigit(Peek())||Peek()=='.'
        ||((Peek()=='-'||Peek()=='+')&&(Last()=='e'||Last()=='E'||Last()=='p'||Last()=='P'))))
    {
        if(Peek()=='.'||Peek()=='e'||Peek()=='E'||Peek()=='p'||Peek()=='P')
            is_float=true;
        Next();
    }

    if(is_float)AddToken(CONSTANT_FLOAT);
    else AddToken(CONSTANT_INT);
}

void Scanner::ScanIdentifierKeyword()
{
    while(!IsAtEnd()&&IsDigitAlphaUnderline(Peek()))Next();
    string text=source[current.line].substr(start.column,current.column-start.column);
    
    auto it=KeywordMap.find(text);
    if(it==KeywordMap.end())AddToken(IDENTIFIER);
    else AddToken(it->second);
}


    
//Tool
bool Scanner::IsAtEnd()
{
    return current.line>=source.size();
}

char Scanner::Next()
{
    char next=source[current.line][current.column++];
    if(current.column>=source[current.line].size())
    {
        current.line++;
        current.column=0;
    }

    return next;
}

char Scanner::Last()
{
    return source[current.line][current.column-1];
}

bool Scanner::Match(char expected)
{
    if(IsAtEnd()||source[current.line][current.column]!=expected)
        return false;
    
    Next();
    return true;
}

char Scanner::Peek()
{
    return source[current.line][current.column];
}

bool Scanner::IsDigit(char c)
{
    return c>='0'&&c<='9';
}
    
bool Scanner::IsAlpha(char c)
{
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

bool Scanner::IsAlphaUnderline(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')||c=='_';
}

bool Scanner::IsDigitAlphaUnderline(char c)
{
	return IsDigit(c) || IsAlphaUnderline(c);
}

bool Scanner::IsBlank(char c)
{
    return c=='\t'||c=='\n'||c=='\r'||c==' ';
}
