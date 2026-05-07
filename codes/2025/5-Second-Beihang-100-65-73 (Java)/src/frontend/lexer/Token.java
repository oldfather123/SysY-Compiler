package frontend.lexer;

public class Token {
    private final TokenType type;
    private final String content;
    private final int lineno;
    
    public Token(TokenType type, String content, int lineno) {
        this.type = type;
        this.content = content;
        this.lineno = lineno;
    }
    
    public TokenType getType() {
        return this.type;
    }
    
    public String getContent() {
        return this.content;
    }
    
    public int getLineno() {
        return this.lineno;
    }
    
    public boolean isUnaryOpTk() {
        return switch (this.type) {
            case NOT, ADD, SUB -> true;
            default -> false;
        };
    }
    
    @Override
    public String toString() {
        return this.type.getClassCode() + " " + this.content;
    }
}
