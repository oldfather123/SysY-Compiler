package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;
public class BlockItemNode {

    public DeclNode declNode;

    public StmtNode stmtNode;

    public String nodeType;

    public static final String declNodeType = "declNode";

    public static final String stmtNodeType = "stmtNode";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        if(nodeType.equals(BlockItemNode.declNodeType)) declNode.syntacticDebug(writer);
        else stmtNode.syntacticDebug(writer);
//        writer.write("<BlockItem>\n");
    }
}
