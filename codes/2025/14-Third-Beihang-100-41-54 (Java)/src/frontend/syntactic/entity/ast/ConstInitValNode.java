package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class ConstInitValNode {

    public ConstExpNode constExpNode;

    public ArrayList<ConstInitValNode> constInitValNodeArrayList = new ArrayList<>();

    public String nodeType;

    public static final String constExpNodeType = "constExp";

    public static final String constInitValListType = "constInitValList";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<ConstInitVal>\n");
        if(nodeType.equals(ConstInitValNode.constExpNodeType)) constExpNode.syntacticDebug(writer);
        else{
            for(ConstInitValNode constInitValNode: constInitValNodeArrayList) constInitValNode.syntacticDebug(writer);
        }
    }
}
