package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;

public class UnaryOpNode {

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<UnaryOp>\n");
    }

    public void semanticParser(SymbolTable symbolTable){

    }
}
