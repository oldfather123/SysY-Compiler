package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;

public class CompileUnitDeclNode {

    public DeclNode declNode;

    public FuncDefNode funcDefNode;

    public String nodeType;

    public static final String declType = "Decl";

    public static final String funcDefType = "FuncDef";

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<CompileUnitDecl>\n");
        if(nodeType.equals(CompileUnitDeclNode.declType)) declNode.syntacticDebug(writer);
        else funcDefNode.syntacticDebug(writer);
    }
}
