package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class MulExpNode {
    public ArrayList<UnaryExpNode> unaryExpNodeArrayList = new ArrayList<>();

    public ArrayList<Token> opList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<MulExp>\n");
        int size = unaryExpNodeArrayList.size();
        for(int i = 0; i < size; i++){
            UnaryExpNode unaryExpNode = unaryExpNodeArrayList.get(i);
            unaryExpNode.syntacticDebug(writer);
            if(i < opList.size()) writer.write(opList.get(i).content+ "\n");
        }
    }
}
