package cn.edu.bit.newnewcc.frontend;

import java.util.Collections;
import java.util.Deque;
import java.util.LinkedList;
import java.util.List;

public class TreeBuilder {
    private final List<Integer> shape;
    private final Node root = new Node();
    private final Deque<Node> stack = new LinkedList<>();

    {
        stack.push(root);
    }

    public TreeBuilder(List<Integer> shape) {
        this.shape = shape;
    }

    public List<Integer> getShape() {
        return Collections.unmodifiableList(shape);
    }

    public Node getRoot() {
        return root;
    }

    public Node getTop() {
        return stack.element();
    }

    public int getDepth() {
        return stack.size() - 1;
    }

    private void push(Node node) {
        int count = 0;
        while (stack.element().getChildren().size() == shape.get(getDepth())) {
            ++count;
            stack.pop();
        }
        for (int i = 0; i < count; ++i) {
            Node child = new Node();
            stack.element().addChild(child);
            stack.push(child);
        }
        stack.element().addChild(node);
        stack.push(node);
    }

    public void push() {
        push(new Node());
    }

    public void push(Object value) {
        push(new TerminalNode(value));
    }

    public void pop() {
        stack.pop();
    }
}
