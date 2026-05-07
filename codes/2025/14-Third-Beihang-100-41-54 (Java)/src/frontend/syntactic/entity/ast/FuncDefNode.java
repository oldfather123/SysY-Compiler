package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class FuncDefNode {
    public FuncTypeNode funcTypeNode;

    public Token identToken;

    public ArrayList<FuncFParamNode> funcFParamNodeArrayList = new ArrayList<>();

    public BlockNode blockNode;

    public boolean hasParam(){return !funcFParamNodeArrayList.isEmpty();}

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<FuncDef> ");
        funcTypeNode.syntacticDebug(writer);
        writer.write(" " + identToken.content + "\n");
        if(hasParam()){
            for(FuncFParamNode funcFParamNode: funcFParamNodeArrayList){
                funcFParamNode.syntacticDebug(writer);
            }
        }
        blockNode.syntacticDebug(writer);
    }
}
