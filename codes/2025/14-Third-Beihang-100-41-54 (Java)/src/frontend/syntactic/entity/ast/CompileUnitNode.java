package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class CompileUnitNode {

    public ArrayList<CompileUnitDeclNode> declList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<CompileUnit>\n");
        for(CompileUnitDeclNode compileUnitDeclNode: declList) compileUnitDeclNode.syntacticDebug(writer);
    }
}
