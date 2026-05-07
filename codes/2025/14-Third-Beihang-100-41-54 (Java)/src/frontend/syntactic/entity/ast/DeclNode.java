package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;

public class DeclNode {

    public ConstDeclNode constDeclNode;

    public VarDeclNode varDeclNode;

    public String nodeType;

    public boolean isGlobal;

    public static final String varDeclType = "varDecl";

    public static final String constDeclType = "constDecl";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<Decl>\n");
        if(nodeType.equals(DeclNode.varDeclType)) varDeclNode.syntacticDebug(writer);
        else constDeclNode.syntacticDebug(writer);

    }
}
