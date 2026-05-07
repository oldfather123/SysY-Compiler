package frontend.AST;

import java.util.ArrayList;

//Block -> '{' { BlockItem } '}'
public class Block implements Stmt {
    private ArrayList<BlockItem> blockItems;

    public Block(ArrayList<BlockItem> blockItems) {
        this.blockItems = blockItems;
    }

    public ArrayList<BlockItem> getBlockItems() {
        return this.blockItems;
    }
}