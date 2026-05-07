package front.AST;

import front.lexer.Token;
import utils.type.SyntaxType;

import java.util.ArrayList;

// token
public class TokenNode extends Node {

    private final Token token;

    public TokenNode(SyntaxType sType, ArrayList<Node> children, Token token) {
        super(sType, children);
        this.token = token;
    }

    public Token getToken() {
        return token;
    }

}
