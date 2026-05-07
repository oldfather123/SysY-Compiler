package cn.edu.bit.newnewcc.util;

import java.util.*;

/**
 * 支配树构建类
 * <p>
 * 采用Tarjan提出的支配树算法
 *
 * @param <T> 用于索引节点的类，该类的每个实例对应图中的一个节点
 * @see <a href="https://dl.acm.org/doi/pdf/10.1145/357062.357071">Tarjan论文原文</a>
 * @see <a href="https://www.luogu.com.cn/record/42013604">C++参考实现</a>
 */
public class DomTreeBuilder<T> {

    private class BuilderNode {

        /**
         * 索引对象
         */
        final T t;

        /**
         * 原图中该节点的前驱列表
         */
        Collection<BuilderNode> entries = new ArrayList<>();

        /**
         * 原图中该节点的后继列表
         */
        Collection<BuilderNode> exits = new ArrayList<>();

        /**
         * 支配树上该节点的儿子列表
         */
        Collection<BuilderNode> domSons = new ArrayList<>();

        /**
         * DFS序
         */
        int dfn = -1;

        /**
         * 半支配点
         */
        BuilderNode semi;

        /**
         * 支配点
         */
        BuilderNode idom;

        /**
         * emm, 我也不好描述是干啥用的，总之需要它...
         */
        final List<BuilderNode> proc = new ArrayList<>();

        public BuilderNode(T t) {
            this.t = t;
        }

    }

    private class WeightedUnionFind {
        private class WufNode {
            BuilderNode father;
            BuilderNode weight;

            public WufNode(BuilderNode node) {
                this.father = node;
            }
        }

        private final Map<BuilderNode, WeightedUnionFind.WufNode> nodeMap = new HashMap<>();

        public WeightedUnionFind(Collection<BuilderNode> nodes) {
            for (BuilderNode node : nodes) {
                nodeMap.put(node, new WeightedUnionFind.WufNode(node));
            }
        }

        public BuilderNode getFather(BuilderNode u) {
            var nodeU = nodeMap.get(u);
            if (nodeU.father != u) {
                var temp = nodeU.father;
                nodeU.father = getFather(nodeU.father);
                if (getWeight(temp).semi.dfn < getWeight(u).semi.dfn) {
                    setWeight(u, getWeight(temp));
                }
            }
            return nodeMap.get(u).father;
        }

        public BuilderNode getWeight(BuilderNode u) {
            return nodeMap.get(u).weight;
        }

        public void setWeight(BuilderNode u, BuilderNode weight) {
            nodeMap.get(u).weight = weight;
        }

        public void unite(BuilderNode u, BuilderNode v) {
            getFather(v);
            nodeMap.get(v).father = u;
        }

    }

    private boolean hasTreeBuilt = false;

    private BuilderNode root;

    private final Map<T, BuilderNode> nodeMap = new HashMap<>();

    /**
     * 通过dfn查找节点使用的数组
     */
    private final List<BuilderNode> antiDfn = new ArrayList<>();

    private BuilderNode tToNode(T t) {
        if (!nodeMap.containsKey(t)) {
            nodeMap.put(t, new BuilderNode(t));
        }
        return nodeMap.get(t);
    }

    /**
     * 添加一条图中的有向边
     * <p>
     * 你只需要添加正向边，算法所需的反向边是自动添加的
     *
     * @param source      边的起点
     * @param destination 边的终点
     */
    public void addEdge(T source, T destination) {
        if (hasTreeBuilt) {
            throw new UnsupportedOperationException();
        }
        if (source == destination) return;
        var u = tToNode(source);
        var v = tToNode(destination);
        u.exits.add(v);
        v.entries.add(u);
    }

    /**
     * 设置支配树的根节点
     *
     * @param t 根节点
     */
    public void setRoot(T t) {
        if (hasTreeBuilt) {
            throw new UnsupportedOperationException();
        }
        root = tToNode(t);
    }

    /**
     * 开始构建支配树
     * <p>
     * 一旦开始构建，支配树的所有信息均不可被修改了
     */
    public void build() {
        if (hasTreeBuilt) {
            throw new UnsupportedOperationException();
        }
        hasTreeBuilt = true;
        dfsForDfn(root);
        var wuf = new WeightedUnionFind(nodeMap.values());
        for (int i = antiDfn.size() - 1; i >= 1; i--) {
            BuilderNode nw = antiDfn.get(i);
            for (BuilderNode x : nw.proc) {
                wuf.getFather(x);
                if (wuf.getWeight(x) == null || x.semi != wuf.getWeight(x).semi) {
                    x.idom = wuf.getWeight(x);
                } else {
                    x.idom = x.semi;
                }
            }
            for (BuilderNode entry : nw.entries) {
                if (entry.dfn < nw.dfn) {
                    if (entry.dfn < nw.semi.dfn) {
                        nw.semi = entry;
                    }
                } else {
                    wuf.getFather(entry);
                    if (wuf.getWeight(entry).semi.dfn < nw.semi.dfn) {
                        nw.semi = wuf.getWeight(entry).semi;
                    }
                }
            }
            wuf.setWeight(nw, nw);
            nw.semi.proc.add(nw);
            for (BuilderNode exit : nw.exits) {
                if (exit.dfn > nw.dfn) {
                    wuf.unite(nw, exit);
                }
            }
        }
        for (BuilderNode node : antiDfn.get(0).proc) {
            node.idom = antiDfn.get(0);
        }
        for (int i = 1; i < antiDfn.size(); i++) {
            BuilderNode nw = antiDfn.get(i);
            if (nw.semi != nw.idom) {
                nw.idom = nw.idom.idom;
            }
            nw.idom.domSons.add(nw);
        }
    }

    /**
     * 获取一个节点支配的其他节点
     *
     * @param t 查询的节点
     * @return 该节点支配的节点
     */
    public Collection<T> getDomSons(T t) {
        if (!hasTreeBuilt) {
            throw new UnsupportedOperationException();
        }
        List<T> result = new ArrayList<>();
        tToNode(t).domSons.forEach(node -> result.add(node.t));
        // 将儿子按照dfs序排序，保证每个节点被访问时，至少有一个已经访问过的父节点
        // 此性质有利于加快值域分析的收敛
        result.sort((o1, o2) -> Integer.compare(nodeMap.get(o1).dfn, nodeMap.get(o2).dfn));
        return result;
    }

    private void dfsForDfn(BuilderNode node) {
        node.dfn = antiDfn.size();
        antiDfn.add(node);
        node.semi = node;
        for (BuilderNode exit : node.exits) {
            if (exit.dfn == -1) {
                dfsForDfn(exit);
            }
        }
    }

}
