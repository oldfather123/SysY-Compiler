package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;

public class ConstExpNode {

    public AddExpNode addExpNode;

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<ConstExp>\n");
        addExpNode.syntacticDebug(writer);
    }
}
