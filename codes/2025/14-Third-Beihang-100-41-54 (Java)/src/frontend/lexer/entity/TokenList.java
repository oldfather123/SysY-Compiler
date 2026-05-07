package frontend.lexer.entity;

import java.util.ArrayList;

public class TokenList {

    public ArrayList<Token> tokenList = new ArrayList<>();

    public Integer tokenPlace = 0;

    public Token getNowToken(){return tokenPlace >= tokenList.size()?null:tokenList.get(tokenPlace);}

    public boolean isEnd(){return this.tokenPlace == tokenList.size();}

    public void goToNextToken(){if(tokenPlace < tokenList.size()) tokenPlace ++ ;}

    public Token preRead(int preReadPlace){
        if(tokenPlace + preReadPlace < tokenList.size()) return tokenList.get(tokenPlace + preReadPlace);
        else return null;
    }

    public void addNewToken(Token token){this.tokenList.add(token);}
}
