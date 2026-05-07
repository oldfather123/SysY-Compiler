package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class ConstDefNode {

    public Token identToken;

    public Integer arrayDimension;

    public ArrayList<ConstExpNode> dimensionExpList = new ArrayList<>();

    public ConstInitValNode constInitValNode;

    public boolean isArray(){return !dimensionExpList.isEmpty();}
    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<ConstDef> " + identToken.content + "\n");
        if(isArray()){
            for(ConstExpNode constExpNode:dimensionExpList) constExpNode.syntacticDebug(writer);
        }
        constInitValNode.syntacticDebug(writer);

    }
}
