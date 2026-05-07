package cn.edu.bit.newnewcc.frontend;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.StringJoiner;

public class Node {
    private final List<Node> children = new ArrayList<>();

    public List<Node> getChildren() {
        return Collections.unmodifiableList(children);
    }

    public void addChild(Node child) {
        children.add(child);
    }

    public void addChild(int index, Node child) {
        children.add(index, child);
    }

    public void removeChild(Node child) {
        children.remove(child);
    }

    public void removeChild(int index) {
        children.remove(index);
    }

    public String toString() {
        return "Node";
    }

    public void accept(TreeListener listener) {
        listener.enter(this);
        for (Node child : children) {
            child.accept(listener);
        }
        listener.exit(this);
    }

    public String toStringTree() {
        StringJoiner joiner = new StringJoiner("\n").add(toString());
        for (Node child : getChildren()) {
            for (String line : child.toStringTree().split("\\n")) {
                joiner.add("  " + line);
            }
        }
        return joiner.toString();
    }
}
