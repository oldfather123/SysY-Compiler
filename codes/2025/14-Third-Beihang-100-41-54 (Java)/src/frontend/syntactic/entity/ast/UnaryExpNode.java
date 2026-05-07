package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Objects;

public class UnaryExpNode {

    //UnaryExp -> PrimaryExp
    public PrimaryExpNode primaryExpNode;

    //UnaryExp -> Ident(FuncRParamsExp)
    public Token identToken;

    public ArrayList<ExpNode> expParamNodeList = new ArrayList<>();

    //UnaryExp -> UnaryOP UnaryExp
    public Token opToken;

    public UnaryExpNode unaryExpNode;

    public String nodeType;

    public static final String primaryExpNodeType = "primaryExpNodeType";

    public static final String identExpNodeType = "identExpNodeType";

    public static final String unaryExpNodeType = "unaryExpNodeType";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<UnaryExp>\n");
        if(Objects.equals(nodeType, primaryExpNodeType)){
            primaryExpNode.syntacticDebug(writer);
        }
        else if(nodeType.equals(identExpNodeType)){
            writer.write("call func: " + identToken.content + "\n");
        }
        else if(nodeType.equals(unaryExpNodeType)){
            writer.write(opToken.content + "\n");
            unaryExpNode.syntacticDebug(writer);
        }
    }

}