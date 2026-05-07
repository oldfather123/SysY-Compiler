package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class FuncFParamNode {

    public BTypeNode bTypeNode;

    public Token identToken;

    public ArrayList<ExpNode> expNodeArrayList = new ArrayList<>();

    public boolean isArrayParam;

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<FuncFParam> ");
        bTypeNode.syntacticDebug(writer);
        writer.write(identToken.content + "\n");
        if(isArrayParam){
            for(ExpNode expNode: expNodeArrayList){
                expNode.syntacticDebug(writer);
            }
        }
    }
}
