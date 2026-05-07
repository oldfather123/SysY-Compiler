package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class VarDeclNode {
    public BTypeNode bTypeNode;
    public ArrayList<VarDefNode> varDefNodeList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<VarDecl> ");
        bTypeNode.syntacticDebug(writer);
        writer.write("\n");
        for(VarDefNode varDefNode : varDefNodeList){
            varDefNode.syntacticDebug(writer);
        }
    }

    public void semanticParser(SymbolTable symbolTable){

    }
}
