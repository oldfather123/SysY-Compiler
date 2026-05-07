package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class RelExpNode {

    public ArrayList<AddExpNode> addExpNodeArrayList = new ArrayList<>();
    public ArrayList<Token> opList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<RelExp>\n");
        int size = addExpNodeArrayList.size();
        for(int i = 0; i < size; i++){
            addExpNodeArrayList.get(i).syntacticDebug(writer);
            if(i < opList.size()) writer.write(opList.get(i).content + "\n");
        }
    }

    public void semanticParser(SymbolTable symbolTable){

    }
}
