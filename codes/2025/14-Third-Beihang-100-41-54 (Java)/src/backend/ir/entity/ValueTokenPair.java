package backend.ir.entity;

import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;

/**
 * &#064;Classname ValueTokenPair
 * &#064;Description  TODO
 * &#064;Date 2025/7/26 16:50
 * &#064;Created MuJue
 */
public class ValueTokenPair {
    private Value value;
    private TokenType tokenType;
    public ValueTokenPair(Value value, TokenType tokenType){
        this.value = value;
        this.tokenType = tokenType;
    }

    public void setValue(Value value) {
        this.value = value;
    }

    public void setTokenType(TokenType tokenType) {
        this.tokenType = tokenType;
    }

    public Value getValue() {
        return value;
    }

    public TokenType getTokenType() {
        return tokenType;
    }
}
