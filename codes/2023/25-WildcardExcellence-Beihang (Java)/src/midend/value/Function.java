package midend.value;

import midend.Type;
import util.nodelist.Node;
import util.nodelist.NodeList;

import java.util.ArrayList;

public class Function extends Value {
    private ArrayList<Value> argument;

    public ArrayList<Value> getArgument() {
        return argument;
    }

    public void setArgument(ArrayList<Value> argument) {
        this.argument = argument;
    }

    private NodeList<BasicBlock> basicBlocks;

    public Function(Type type, String name, ArrayList<Value> argument) {
        super(type, name);
        this.argument = argument;
        this.basicBlocks = new NodeList<>();
    }

    public void addBlock(BasicBlock basicBlock) {
        this.basicBlocks.addLast(basicBlock);
    }

    public void removeBlock(Node<BasicBlock> basicBlock) {
        basicBlock.remove();
    }

    public BasicBlock getFirstBlock() {
        return this.basicBlocks.firstElement();
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getType());
        sb.append(" @").append(getName());
        sb.append("(");
        boolean start = false;
        for (Value value : argument) {
            if (start) {
                sb.append(", ");
            }
            start = true;
            sb.append(value.toString());
        }
        sb.append(")").append(" ");
        return sb.toString();
    }

    public NodeList<BasicBlock> getBasicBlocks() {
        return basicBlocks;
    }

    public void setBasicBlocks(NodeList<BasicBlock> basicBlocks) {
        this.basicBlocks = basicBlocks;
    }
}
