package front.AST;

import utils.type.SyntaxType;

import java.util.ArrayList;

// Block ==> '{' { VarDecl | ConstDecl | Stmt } '}'
public class Block extends Node {

    public Block(SyntaxType syntaxType, ArrayList<Node> children) {
        super(syntaxType, children);
    }

}
