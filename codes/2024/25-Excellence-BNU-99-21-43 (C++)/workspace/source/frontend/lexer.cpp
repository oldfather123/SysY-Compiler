#include "lexer.h"

/*
    Constructor for the Lexer.
        - Initializes the input stream and the position.
        - Initializes the key words.
*/

int Lexer::lines = 1;
Lexer::Lexer(std::string code)
{
    input = code;
    peek = ' ';
    position = 0;
    len = input.size();
    words["if"] = Word::IF;
    words["else"] = Word::ELSE;
    words["while"] = Word::WHILE;
    words["break"] = Word::BREAK;
    words["continue"] = Word::CONTINUE;
    words["int"] = Type::Int;
    words["float"] = Type::Float;
    words["void"] = Type::Void;
    words["return"] = Word::RETURN;
    words["const"] = Word::CONST;
    words["getint"] = Word::GETINT;
    words["getch"] = Word::GETCH;
    words["getfloat"] = Word::GETFLOAT;
    words["getarray"] = Word::GETARRAY;
    words["getfarray"] = Word::GETFARRAY;
    words["putint"] = Word::PUTINT;
    words["putch"] = Word::PUTCH;
    words["putfloat"] = Word::PUTFLOAT;
    words["putarray"] = Word::PUTARRAY;
    words["putfarray"] = Word::PUTFARRAY;
    words["putf"] = Word::PUTF;
    words["starttime"] = Word::STARTTIME;
    words["stoptime"] = Word::STOPTIME;
    words["memset"] = Word::MEMSET;
}

void Lexer::readch()
{
    if (position < len)
    {
        peek = input[position];
        position++;
    }
    else
    {
        peek = EOF;
    }
}

bool Lexer::readch(char c)
{
    readch();
    if (peek != c)
    {
        return false;
    }
    peek = ' ';
    return true;
}

void Lexer::comment(bool is_block)
{
    if (is_block)
    {
        for (; peek != EOF;)
        {
            readch();
            if (peek == '*')
            {
                if (readch('/'))
                    break;
            }
        }
    }
    else
    {
        for (; peek != EOF;)
        {
            if (readch('\n'))
            {
                lines++;
                break;
            }
        }
    }
}

Token *Lexer::scan_number(std::string temp = "")
{
    if (temp.size())
        goto floatlable;
    do
    {
        temp += peek;
        readch();
    } while (std::isdigit(peek));
    if (peek == 'x' || peek == 'X')
    {
        temp += peek;
        readch();
        while (std::isxdigit(peek) || ('A' <= peek && peek <= 'F') || ('a' <= peek && peek <= 'f'))
        {
            temp += peek;
            readch();
        }
        if (peek == '.')
        {
            temp += peek;
            readch();
            while (std::isxdigit(peek) || ('A' <= peek && peek <= 'F') || ('a' <= peek && peek <= 'f'))
            {
                temp += peek;
                readch();
            }
            if (peek == 'p' || peek == 'P')
                goto label1;
            return new FloatNumber(std::stof(temp));
        }
        if (peek == 'p' || peek == 'P')
        {
        label1:
            temp += peek;
            readch();
            if (peek == '+' || peek == '-')
            {
                temp += peek;
                readch();
            }
            while (std::isxdigit(peek) || ('A' <= peek && peek <= 'F') || ('a' <= peek && peek <= 'f'))
            {
                temp += peek;
                readch();
            }
            return new FloatNumber(std::stof(temp));
        }
        return new IntNumber(std::stoi(temp, nullptr, 16));
    }
    else if (peek == '.')
    {
        temp += peek;
        readch();
    floatlable:
        while (std::isdigit(peek))
        {
            temp += peek;
            readch();
        }
        if (peek == 'e' || peek == 'E')
            goto label;
        return new FloatNumber(std::stof(temp));
    }
    else if (peek == 'e' || peek == 'E')
    {
    label:
        temp += peek;
        readch();
        if (peek == '+' || peek == '-')
        {
            temp += peek;
            readch();
        }
        while (std::isdigit(peek))
        {
            temp += peek;
            readch();
        }
        return new FloatNumber(std::stof(temp));
    }
    if (temp[0] == '0')
        return new IntNumber(std::stoi(temp, nullptr, 8));
    else
        return new IntNumber(std::stoi(temp));
}

Token *Lexer::scan()
{
    for (;; readch())
    {
        if (peek == ' ' || peek == '\t' || peek == '\r')
            continue;
        else if (peek == '\n')
        {
            lines++;
            continue;
        }
        else
            break;
    }
    if (peek == '>')
    {
        if (readch('='))
            return new Word(init::GE, ">=");
        else
            return new Token('>');
    }
    else if (peek == '<')
    {
        if (readch('='))
            return new Word(init::LE, "<=");
        else
            return new Token('<');
    }
    else if (peek == '!')
    {
        if (readch('='))
            return new Word(init::NE, "!=");
        else
            return new Token('!');
    }
    else if (peek == '=')
    {
        if (readch('='))
            return new Word(init::EQ, "==");
        else
            return new Token('=');
    }
    else if (peek == '&')
    {
        if (readch('&'))
            return new Word(init::AND, "&&");
        else
            return new Token('&');
    }
    else if (peek == '|')
    {
        if (readch('|'))
            return new Word(init::OR, "||");
        else
            return new Token('|');
    }
    else if (peek == '~')
    {
        if (readch('='))
            return new Word(init::NE, "~=");
        else
            return new Token('~');
    }
    else if (peek == '/')
    {
        readch();
        if (peek == '/')
        {
            comment(0);
            return scan();
        }
        else if (peek == '*')
        {
            comment(1);
            return scan();
        }
        else
            return new Token('/');
    }

    if (std::isdigit(peek))
    {
        return scan_number();
    }

    if (peek == '.')
    {
        readch();
        if (isdigit(peek))
        {
            return scan_number("0.");
        }
        return new Token('.');
    }

    if (std::isalpha(peek) || peek == '_')
    {
        std::string id_name = "";
        do
        {
            id_name += peek;
            readch();
        } while (std::isalnum(peek) || peek == '_');
        if (words.find(id_name) == words.end())
        {
            words[id_name] = new Word(init::ID, id_name);
        }
        return words[id_name];
    }
    Token *Tok;
    if (peek == EOF)
    {
        Tok = new Token(init::END);
    }
    else
        Tok = new Token(peek);
    peek = ' ';
    return Tok;
}
