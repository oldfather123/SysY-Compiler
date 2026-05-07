package frontend.lexer;

import java.util.ArrayList;
import java.util.Arrays;

public class TokenList {
    public final ArrayList<Token> tokens = new ArrayList<>();
    private int index = 0;

    public void append(Token token) {
        tokens.add(token);
    }

    public Token checkAheadChar(int count) {
        return tokens.get(index + count);
    }

    public boolean hasNext() {
        return index < tokens.size();
    }

    public Token getChar() {
        return tokens.get(index++);
    }

    public Token getCharWithJudge(TokenType... types) {
        //TODO: 需要异常处理吗？另外，要加入EOF判断和自定义异常
        Token token = tokens.get(index);
        for (TokenType type : types) {
            if (token.getType().equals(type)) {
                index++;
                return token;
            }
        }
        for (int i = 0; i < index; i++) {
            System.err.println(tokens.get(i));
        }
        throw new RuntimeException("Expected " + Arrays.toString(types) + " but got " + token);
    }

    public int getIndex(){
        return index;
    }

    public void setIndex(int index){
        this.index = index;
    }
}
//