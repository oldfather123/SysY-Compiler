package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class VarDefNode {
    public Token identToken;

    public ArrayList<ConstExpNode> constExpNodeArrayList = new ArrayList<>();

    public InitValNode initValNode = null;

    public boolean hasInitVal(){return initValNode != null;}

    public boolean isArrayDef(){return !constExpNodeArrayList.isEmpty();}

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        // 我们就不打印维度、初始值那些内容了
        writer.write("<VarDef> " + identToken.content + "\n");
        if(isArrayDef()){
            for(ConstExpNode constExpNode:constExpNodeArrayList) constExpNode.syntacticDebug(writer);
        }
        if(hasInitVal()){
            initValNode.syntacticDebug(writer);
        }
    }
}
