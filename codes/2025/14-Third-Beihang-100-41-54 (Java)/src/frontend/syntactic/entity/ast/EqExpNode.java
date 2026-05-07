package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class EqExpNode {

    public ArrayList<RelExpNode> relExpNodeArrayList = new ArrayList<>();

    public ArrayList<Token> opList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<EqExp>\n");
        for(int i = 0;i < relExpNodeArrayList.size();i ++ ){
            relExpNodeArrayList.get(i).syntacticDebug(writer);
            if(i < opList.size()) writer.write(opList.get(i).content + "\n");
        }
    }
}
