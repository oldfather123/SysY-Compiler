package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class LOrExpNode {

    public ArrayList<LAndExpNode> lAndExpNodeArrayList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<LOrExp>\n");
        for(LAndExpNode lAndExpNode : lAndExpNodeArrayList){
            lAndExpNode.syntacticDebug(writer);
        }
    }
}
