package front.lexer;

import utils.type.TokenType;

public class Token {

    private final TokenType tokenType;
    private final String value;
    private final int line;

    public Token(TokenType tokenType, String value, int line) {
        this.tokenType = tokenType;
        this.value = value;
        this.line = line;
    }

    public TokenType getTokenType() {
        return tokenType;
    }

    public int getLine() {
        return line;
    }

    public String getValue() {
        return value;
    }

    public String toString() {
        return tokenType + " " + value;
    }

}
