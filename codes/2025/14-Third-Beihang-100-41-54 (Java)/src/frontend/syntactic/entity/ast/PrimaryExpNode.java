package frontend.syntactic.entity.ast;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
public class PrimaryExpNode {

    public ExpNode expNode;
    public LValNode lValNode;
    public NumberNode numberNode;
    public String nodeType;

    public static final String expNodeType = "expNode";
    public static final String lValNodeType = "lValNodeType";
    public static final String numberNodeType = "numberNodeType";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<PrimaryExp>\n");
        if(nodeType.equals(expNodeType)){
            expNode.syntacticDebug(writer);
        }
        else if(nodeType.equals(lValNodeType)){
            lValNode.syntacticDebug(writer);
        }
        else if(nodeType.equals(numberNodeType)){
            numberNode.syntacticDebug(writer);
        }
    }
}

