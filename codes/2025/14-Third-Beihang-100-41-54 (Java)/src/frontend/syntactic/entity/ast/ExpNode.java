package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;

public class ExpNode {
    public AddExpNode addExpNode;

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<Exp>\n");
        addExpNode.syntacticDebug(writer);
    }
}
