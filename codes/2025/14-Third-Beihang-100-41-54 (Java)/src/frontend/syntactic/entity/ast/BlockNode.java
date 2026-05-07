package frontend.syntactic.entity.ast;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class BlockNode {

    public ArrayList<BlockItemNode> blockItemNodeArrayList = new ArrayList<>();

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<Block>\n");
        for(BlockItemNode blockItemNode: blockItemNodeArrayList) blockItemNode.syntacticDebug(writer);
    }
}
