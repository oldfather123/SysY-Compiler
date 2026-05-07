package frontend.lexer;

public enum TokenType {
    /* Integer Literal: Dec / Oct / Hex */
    NUMBER,
    /* Float Literal: .??? / ???. / ???.??? */
    REAL,
    /* Identifier */
    IDENT,
    /* Keyword: Const */
    CONST,
    /* Keyword: Type */
    VOID,
    INT,   // Basic Type
    FLOAT, // Basic Type
    /* Keyword: Condition */
    IF,
    ELSE,
    /* Keyword: Loop */
    WHILE,
    CONTINUE,
    BREAK,
    /* Keyword: Function */
    RETURN,
    /* Punctuation: Bracket */
    LBRACE,  // {
    RBRACE,  // }
    LSQUARE, // [
    RSQUARE, // ]
    LPARENT, // (
    RPARENT, // )
    /* Punctuation: Common */
    SEMICOLON, // ;
    COMMA,     // ,
    DOT,       // .
    ASSIGN,    // =
    /* Punctuation: Arithmetic */
    ADD, // +
    SUB, // -
    MUL, // *
    DIV, // /
    MOD, // %
    /* Punctuation: Logic */
    OR,  // ||
    AND, // &&
    NOT, // !
    /* Punctuation: Equality */
    EQ,  // ==
    NEQ, // !=
    /* Punctuation: Partial Comparison */
    GT, // >
    GE, // >=
    LT, // <
    LE, // <=
    /* Eof is a token */
    EOF,
    // Attention: There are no string literals!
}
