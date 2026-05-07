package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class LValNode {

    public Token identToken;

    public ArrayList<ExpNode> expNodeArrayList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<LVal> " + identToken.content + "\n");
        for(ExpNode expNode: expNodeArrayList){
            expNode.syntacticDebug(writer);
        }
    }
}
