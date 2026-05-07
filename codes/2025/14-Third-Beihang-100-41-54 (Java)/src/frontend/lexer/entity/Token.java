package frontend.lexer.entity;

import java.util.Objects;

public class Token {

    public static int lineNumber = 1;
    public TokenType wordType;
    public String content;
    public int line;

    public Token(TokenType wordType, String content){
        this.wordType = wordType;this.content = content;this.line = lineNumber;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Token token = (Token) o;
        return wordType == token.wordType && Objects.equals(content, token.content);
    }

    @Override
    public int hashCode() {
        return Objects.hash(wordType, content, line);
    }

    public static void addLineNumber(){Token.lineNumber ++ ;}
}
