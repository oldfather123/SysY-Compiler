package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class InitValNode {
    public ExpNode expNode;

    public ArrayList<InitValNode> initValNodeArrayList = new ArrayList<>();

    public String nodeType;

    public static final String expNodeType = "Exp";

    public static final String initValListType = "initValList";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<InitVal>\n");
        if(nodeType.equals(expNodeType)){
            expNode.syntacticDebug(writer);
        }
        else if(nodeType.equals(initValListType)){
            for(InitValNode initValNode: initValNodeArrayList){
                initValNode.syntacticDebug(writer);
            }
        }
    }
}
