package pass.utils;

import ir.instr.TermInstr;
import ir.value.Function;
import ir.value.BasicBlock;
import ir.value.Value;

import java.util.*;

public class DominatorTree {
    static HashSet<String> visited = new HashSet<>();
    static HashMap<String, BasicBlock> nameToBlockMap = new HashMap<>();

    // 方便查询支配树，可以来个实例，也可以像以前那样直接调用静态方法，没有额外的开销
    private final HashMap<BasicBlock, BasicBlock> parents = new HashMap<>();
    private final BasicBlock entry;

    private final HashMap<BasicBlock, HashSet<BasicBlock>> domers;
    private final HashMap<BasicBlock, List<BasicBlock>> idoms;

    public DominatorTree(Function func) {
        this.entry = func.getBasicBlocks().getFirst();
        HashMap<BasicBlock, HashSet<BasicBlock>> domers = run(func);
        this.domers = domers;

        // Get IDom for each block
        this.idoms = new HashMap<>();

        for (var block : func.getBasicBlocks()) {
            idoms.put(block, new ArrayList<>());
        }

        for (var entry : domers.entrySet()) {
            BasicBlock block = entry.getKey();
            HashSet<BasicBlock> domerSet = entry.getValue();
            for (var domer : domerSet) {
                if (domer == block) {
                    continue;
                }

                boolean isIdom = true;
                for (var otherDomer : domerSet) {
                    if (otherDomer != block && otherDomer != domer && domers.get(otherDomer).contains(domer)) {
                        isIdom = false;
                        break;
                    }
                }
                if (isIdom) {
                    idoms.get(domer).add(block);
                    parents.put(block, domer);
                    break;
                }
            }
        }
    }

    public List<BasicBlock> getPostOrderIdom() {
        List<BasicBlock> postOrder = new ArrayList<>();
        HashSet<BasicBlock> visited = new HashSet<>();
        Stack<BasicBlock> stack = new Stack<>();
        stack.push(entry);
        while (!stack.isEmpty()) {
            BasicBlock block = stack.peek();
            if (visited.contains(block)) {
                postOrder.add(block);
                stack.pop();
                continue;
            }
            for (BasicBlock child : idoms.get(block)) {
                stack.push(child);
            }
            visited.add(block);
        }
        return postOrder;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (BasicBlock block : parents.keySet()) {
            sb.append("  ").append(block.getName()).append(" -> ").append(parents.get(block).getName()).append("\n");
        }
        return sb.toString();
    }

    public BasicBlock getParent(BasicBlock block) {
        return parents.get(block);
    }

    /**
     * 判断 a 是否支配 b
     * @param a 节点 a，支配者
     * @param b 节点 b，被支配者
     * @return 是否支配
     */
    public boolean isAncestor(BasicBlock a, BasicBlock b) {
        while (b != null) {
            if (a == b) {
                return true;
            }
            b = parents.get(b);
        }
        return false;
    }

    private static HashMap<BasicBlock, HashSet<BasicBlock>> run(Function function) {
        HashMap<BasicBlock, HashSet<BasicBlock>> domers = new HashMap<>();
        BasicBlock root = function.getBasicBlocks().getFirst();
        for (BasicBlock block : function.getBasicBlocks()) {
            nameToBlockMap.put(block.getName(), block);
            domers.put(block, new HashSet<>());
        }
        for (BasicBlock block : function.getBasicBlocks()) {
            visited.clear();
            visited.add(block.getName());
            dfs(root);
            for (BasicBlock other : function.getBasicBlocks()) {
                if (!visited.contains(other.getName())) {
                    domers.get(other).add(block);
                }
            }
        }
        return domers;
    }

    public static void run(Function func, HashMap<String,HashSet<String>> dominatorSonsInput) {
        /*
        * <https://www.cs.au.dk/~gerth/advising/thesis/henrik-knakkegaard-christensen.pdf, Page 25, Algorithm 3>:
        * for w in V do
        *     traverse V', V' <- V - {w}
        *     w dominates vertices not visited
        * end for
        *
        * For each vertex w in V, traverse the graph starting from *the root*.
        *
        * */
        LinkedList<BasicBlock> blocks = func.getBasicBlocks();
        HashMap<String,HashSet<String>> dominatorSons = new HashMap<>();
        String ifDelete;
        BasicBlock root = blocks.getFirst();
        // 节点删除法求支配树
        for (BasicBlock block : blocks) {
            nameToBlockMap.put(block.getName(), block);
            dominatorSons.put(block.getName(), new HashSet<>());
        }
        for (BasicBlock block : blocks) {
            ifDelete = block.getName();
            visited.clear();
            visited.add(ifDelete);
            dfs(root);
            for (BasicBlock block1 : blocks) {
                if (!visited.contains(block1.getName())) {
                    dominatorSons.get(ifDelete).add(block1.getName());
                }
            }
        }
        dominatorSonsInput.putAll(dominatorSons);
    }

    private static void dfs(BasicBlock block) {
        if (visited.contains(block.getName())) {
            return;
        }
        visited.add(block.getName());
        Value[] target = ((TermInstr)(block.getTerminator())).getJumpTargets();
        for (Value value : target) {
            if (value == null) {
                continue;
            }
            dfs(nameToBlockMap.get(value.getName()));
        }
    }
}
