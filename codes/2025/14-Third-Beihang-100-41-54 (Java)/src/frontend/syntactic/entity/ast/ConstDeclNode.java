package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class ConstDeclNode {

    public BTypeNode bTypeNode;

    public ArrayList<ConstDefNode> constDefNodeList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<ConstDecl> ");
        bTypeNode.syntacticDebug(writer);
        writer.write("\n");
        for(ConstDefNode constDefNode: constDefNodeList) constDefNode.syntacticDebug(writer);
    }
}
