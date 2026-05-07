package frontend.lexer.entity;

public class LexerError {

    public int lineNumber;

    public String message;

    public LexerError(int lineNumber,String message){
        this.lineNumber = lineNumber;this.message = message;
    }

}
