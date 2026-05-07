package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;

public class CondNode {

    public LOrExpNode lOrExpNode;

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<Cond>\n");
        lOrExpNode.syntacticDebug(writer);
    }
}
