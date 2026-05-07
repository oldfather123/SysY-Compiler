package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.util.DomTreeBuilder;

import java.util.*;

/**
 * 支配树
 */
public class DomTree {

    private static class Node {

        /**
         * 节点对应的基本块
         */
        private final BasicBlock basicBlock;

        /**
         * 父节点列表
         * <p>
         * 下标为i的元素代表当前节点的第2^i级父亲
         */
        // bexp = binary exponentiation
        private final List<BasicBlock> bexpParents = new ArrayList<>();

        private final List<BasicBlock> domSons = new ArrayList<>();

        private int depth;

        public Node(BasicBlock basicBlock) {
            this.basicBlock = basicBlock;
        }

    }

    private final Map<BasicBlock, Node> nodeMap = new HashMap<>();
    private final BasicBlock domRoot;

    private DomTree(Function function) {
        domRoot = function.getEntryBasicBlock();
        var builder = new DomTreeBuilder<BasicBlock>();
        builder.setRoot(function.getEntryBasicBlock());
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (BasicBlock exitBlock : basicBlock.getExitBlocks()) {
                builder.addEdge(basicBlock, exitBlock);
            }
        }
        builder.build();
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            var node = new Node(basicBlock);
            nodeMap.put(basicBlock, node);
            node.domSons.addAll(builder.getDomSons(basicBlock));
        }
        nodeMap.get(function.getEntryBasicBlock()).depth = 0;
        buildBexpArray(nodeMap.get(function.getEntryBasicBlock()));
    }

    private void buildBexpArray(Node u) {
        for (int i = 0; i < u.bexpParents.size(); i++) {
            var f = nodeMap.get(u.bexpParents.get(i));
            if (i < f.bexpParents.size()) {
                u.bexpParents.add(f.bexpParents.get(i));
            }
        }
        for (BasicBlock domSon : u.domSons) {
            var sonNode = nodeMap.get(domSon);
            sonNode.depth = u.depth + 1;
            sonNode.bexpParents.add(u.basicBlock);
            buildBexpArray(sonNode);
        }
    }

    public BasicBlock getDomRoot() {
        return domRoot;
    }

    /**
     * 获取基本块在支配树上的所有直接孩子
     *
     * @param basicBlock 基本块
     * @return 基本块在支配树上的所有直接孩子列表（只读）
     */
    public Collection<BasicBlock> getDomSons(BasicBlock basicBlock) {
        var node = nodeMap.get(basicBlock);
        if (node == null) {
            throw new IllegalArgumentException("Basic block not in this dom tree");
        }
        return Collections.unmodifiableList(node.domSons);
    }

    public BasicBlock getDomFather(BasicBlock basicBlock) {
        if (basicBlock == domRoot) return null;
        return nodeMap.get(basicBlock).bexpParents.get(0);
    }

    /**
     * 检查A在支配树上是否为B的祖先
     *
     * @param alpha A基本块
     * @param beta  B基本块
     * @return 若A在支配树上是B的祖先，返回true；否则返回false。
     */
    public boolean doesAlphaDominatesBeta(BasicBlock alpha, BasicBlock beta) {
        var a = nodeMap.get(alpha);
        var b = nodeMap.get(beta);
        if (a == null || b == null) {
            throw new IllegalArgumentException();
        }
        if (a.depth > b.depth) return false;
        int delta = b.depth - a.depth;
        for (int i = 0; delta != 0; i++) {
            if ((delta & (1 << i)) != 0) {
                b = nodeMap.get(b.bexpParents.get(i));
                delta ^= (1 << i);
            }
        }
        return a == b;
    }

    /**
     * 获取某个基本块在支配树中的深度
     *
     * @param basicBlock 基本块
     * @return 基本块在支配树中的深度
     */
    public int getDomDepth(BasicBlock basicBlock) {
        return nodeMap.get(basicBlock).depth;
    }

    /**
     * 获取基本块的第 2^i 级祖先
     *
     * @param basicBlock 基本块
     * @param level      获取第 2^level 级祖先
     * @return 第 2^level 级祖先
     */
    public BasicBlock getBexpFather(BasicBlock basicBlock, int level) {
        var p = nodeMap.get(basicBlock).bexpParents;
        if (level < p.size()) return p.get(level);
        else return null;
    }

    public BasicBlock lca(BasicBlock u_, BasicBlock v_) {
        var u = nodeMap.get(u_);
        var v = nodeMap.get(v_);
        if (u.depth < v.depth) {
            var temp = u;
            u = v;
            v = temp;
        }
        for (int i = 0; (1 << i) <= u.depth - v.depth; i++) {
            if (((u.depth - v.depth) & (1 << i)) != 0) {
                u = nodeMap.get(u.bexpParents.get(i));
            }
        }
        if (u == v) return u.basicBlock;
        for (int i = u.bexpParents.size() - 1; i >= 0; i--) {
            if (u.bexpParents.get(i) != v.bexpParents.get(i)) {
                u = nodeMap.get(u.bexpParents.get(i));
                v = nodeMap.get(v.bexpParents.get(i));
            }
            i = Integer.min(i, u.bexpParents.size());
        }
        return u.bexpParents.get(0);
    }

    public static DomTree buildOver(Function function) {
        return new DomTree(function);
    }

}
