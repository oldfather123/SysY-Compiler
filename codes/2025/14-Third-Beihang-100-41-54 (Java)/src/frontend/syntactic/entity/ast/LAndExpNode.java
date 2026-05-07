package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class LAndExpNode {
    public ArrayList<EqExpNode> eqExpNodeArrayList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<LandExp>\n");
        for(EqExpNode eqExpNode: eqExpNodeArrayList){
            eqExpNode.syntacticDebug(writer);
        }
    }
}
