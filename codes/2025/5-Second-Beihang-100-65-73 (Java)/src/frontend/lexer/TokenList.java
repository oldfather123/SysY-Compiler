package frontend.lexer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

public class TokenList {
    private final ArrayList<Token> tokens = new ArrayList<>();
    private int curHead = 0;
    
    public void append(Token tk) {
        tokens.add(tk);
    }
    
    public boolean hasNext() {
        return curHead < tokens.size();
    }
    
    public Token preview(int step) {
        return tokens.get(curHead + step);
    }
    
    /**
     * 这个方法用于在已知下一个符号是什么（一般是终结符），并且不需要其具体信息，可以直接忽略时使用。
     * 分号，以及右小、中括号往往都在此列，预计用这个方法来进行报错
     */
    public void passNextToken(TokenType typeExpected) {
        Token token = tokens.get(curHead++);
        // 符合预期，直接返回
        if (token.getType() == typeExpected) {
            return;
        }
        
        // 不符合预期，报错
        throw new RuntimeException("Expected " + typeExpected + " but got " + token
                + " at line " + tokens.get(curHead - 1).getLineno());
    }
    
    public void passTokens(TokenType... typesExpected) {
        for (TokenType tt : typesExpected) {
            passNextToken(tt);
        }
    }
    
    public Token getNextToken() {
        return tokens.get(curHead++);
    }
    
    public Token getNextToken(TokenType... types) {
        if (types == null) {
            throw new NullPointerException();
        }
        Token token = tokens.get(curHead++);
        HashSet<TokenType> typeSet = new HashSet<>(List.of(types));
        
        if (typeSet.contains(token.getType())) {
            return token;
        }
        
        throw new RuntimeException("Expected " + Arrays.toString(types) + " but got " + token);
    }
    
    public int getLatestLineno() {
        return curHead > 0 ? tokens.get(curHead - 1).getLineno() : tokens.get(curHead).getLineno();
    }
    
    public void setCurHead(int curHead) {
        this.curHead = curHead;
    }
    
    public int getCurHead() {
        return curHead;
    }
}
