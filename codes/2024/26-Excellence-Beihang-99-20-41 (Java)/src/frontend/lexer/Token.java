package frontend.lexer;

public class Token {
    private final TokenType type;
    private final Object value;
    private final int line;

    public Token(TokenType type, int line) {
        this(type, null, line);
    }

    public Token(TokenType type, Object value, int line) {
        this.type = type;
        this.value = value;
        this.line = line;
    }

    public int getLine() {
        return line;
    }

    public TokenType getType() {
        return type;
    }

    public Float getFloat() {
        return (Float) value;
    }

    public Integer getInt() {
        return (Integer) value;
    }

    public String getIdent() {
        return (String) value;
    }

    public boolean in(TokenType... type) {
        for (TokenType t : type) {
            if (this.type == t) {
                return true;
            }
        }
        return false;
    }

    @Override
    public String toString() {
        return "<" + type + (value == null ? "" : " : " + value) + ">";
    }
}
