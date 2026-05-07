#ifndef TOKEN_H
#define TOKEN_H

#include <string>

namespace frontend {

    // enumerate for Token type
    enum class TokenType {
        IDENFR,      // identifier
        INTLTR,      // int literal
        FLOATLTR,    // float literal
        CONSTTK,     // const
        VOIDTK,      // void
        INTTK,       // int
        FLOATTK,     // float
        IFTK,        // if
        ELSETK,      // else
        WHILETK,     // while
        CONTINUETK,  // continue
        BREAKTK,     // break
        RETURNTK,    // return
        PLUS,        // +
        MINU,        // -
        MULT,        // *
        DIV,         // slash -> /
        MOD,         // %
        LSS,         // <
        GTR,         // >
        COLON,       // :
        ASSIGN,      // =
        SEMICN,      // ;
        COMMA,       // ,
        LPARENT,     // (
        RPARENT,     // )
        LBRACK,      // [
        RBRACK,      // ]
        LBRACE,      // {
        RBRACE,      // }
        NOT,         // !
        LEQ,         // <=
        GEQ,         // >=
        EQL,         // ==
        NEQ,         // !=
        AND,         // &&
        OR,          // ||
    };
    std::string toString(TokenType);

    struct Token {
        TokenType type;
        std::string value;
    };

    namespace assist {
        TokenType get_keyword_type(const std::string& str);
        TokenType get_op_type(const std::string& str);
        bool is_double_char_op(const std::string& s);
    }  // namespace assist

}  // namespace frontend

#endif