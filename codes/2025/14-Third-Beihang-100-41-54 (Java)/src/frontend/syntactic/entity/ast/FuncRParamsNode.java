package frontend.syntactic.entity.ast;

import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;

public class FuncRParamsNode {

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<FuncRParams>\n");
    }
}
