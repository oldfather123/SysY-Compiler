package cn.edu.bit.newnewcc.frontend;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TreeNormalizer {
    private TreeNormalizer() {
    }

    public static Node normalize(Node root, List<Integer> shape) {
        if (shape.isEmpty()) {
            return root;
        }
        Map<Node, Integer> heights = new HashMap<>();
        root.accept(new TreeListener() {
            @Override
            public void enter(Node node) {
            }

            @Override
            public void exit(Node node) {
                if (node instanceof TerminalNode) {
                    heights.put(node, 0);
                } else {
                    int height = 0;
                    for (Node child : node.getChildren()) {
                        height = Math.max(height, heights.get(child));
                    }
                    ++height;
                    heights.put(node, height);
                }
            }
        });
        TreeBuilder builder = new TreeBuilder(shape);
        root.accept(new TreeListener() {
            @Override
            public void enter(Node node) {
                if (node != root) {
                    while (builder.getDepth() >= shape.size() - heights.get(node)) {
                        builder.pop();
                    }
                    if (node instanceof TerminalNode) {
                        while (builder.getDepth() < shape.size() - 1) {
                            builder.push();
                        }
                        builder.push(((TerminalNode) node).getValue());
                    } else {
                        builder.push();
                    }
                }
            }

            @Override
            public void exit(Node node) {
                builder.pop();
            }
        });
        return builder.getRoot();
    }
}
