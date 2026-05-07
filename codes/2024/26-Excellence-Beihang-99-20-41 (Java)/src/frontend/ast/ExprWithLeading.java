package frontend.ast;

import frontend.lexer.TokenType;

public record ExprWithLeading<T>(TokenType leading, T expr) {
}
