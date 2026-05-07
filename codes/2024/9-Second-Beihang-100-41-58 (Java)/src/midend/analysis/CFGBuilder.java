package midend.analysis;

import midend.BasicBlock;
import midend.Function;
import midend.Module;
import midend.instr.BrInstr;
import midend.instr.Instruction;
import midend.instr.PhiInstr;

import java.util.*;

public class CFGBuilder {
    private static Module module;
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> preMap;// 流图上一个
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> nextMap; //流图下一个
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> childMap; //支配树
    private static HashMap<BasicBlock, BasicBlock> parentMap;
    private static HashMap<BasicBlock, ArrayList<BasicBlock>> DFMap;

    public CFGBuilder(Module module) {
        this.module = module;
    }

    public static void run() {
        for (Function function : module.getFunctions()) {
            initFunc(function);
            getCFG(function);
            removeUseLessBlock(function);
            getDom(function);
            getImmDom(function);
            buildIdomTree(function.getBlockList().get(0), 0);
            getDF(function);
        }
    }

    public static void removeUseLessBlock(Function function) {
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        HashSet<BasicBlock> visited = new HashSet<>();

        Stack<BasicBlock> stack = new Stack<>();
        stack.push(basicBlocks.get(0));

        while (!stack.isEmpty()) {
            BasicBlock block = stack.pop();
            if (!visited.contains(block)) {
                visited.add(block);
                ArrayList<BasicBlock> nextBlocks = block.getNextBlock();
                for (BasicBlock nextBlock : nextBlocks) {
                    stack.push(nextBlock);
                }
            }
        }
        ArrayList<BasicBlock> delete = new ArrayList<>();
        for (int i = basicBlocks.size() - 1; i >= 0; i--) {
            if (!visited.contains(basicBlocks.get(i))) {
                delete.add(basicBlocks.get(i));
            }
        }
        for (BasicBlock del : delete) {
            for (BasicBlock block : function.getBlockList()) {
                if (!block.getInstrList().isEmpty() && block.getInstrList().getFirst() instanceof PhiInstr) {
                    Instruction instruction = block.getInstrList().getFirst();
                    int count = 0;
                    if (instruction instanceof PhiInstr) {
                        if (((PhiInstr) instruction).getPreBlockList().contains(del)) {
                            int index = ((PhiInstr) instruction).getPreBlockList().indexOf(del);
                            ((PhiInstr) instruction).getPreBlockList().remove(index);
                            while (instruction instanceof PhiInstr) {
                                int index0 = ((PhiInstr) instruction).getPreBlockList().indexOf(del);
                                if (index0 != -1) {
                                    ((PhiInstr) instruction).getPreBlockList().remove(index0);
                                }
                                ((PhiInstr) instruction).removeValue(index);
                                instruction = block.getInstrList().get(++count);
                            }
                        }
                    }
//                    while (instruction instanceof PhiInstr) {
//                        if (((PhiInstr) instruction).getPreBlockList().contains(del)) {
//                            ((PhiInstr) instruction).removeBlock(del);
//                        }
//                        instruction = block.getInstrList().get(++count);
//                    }
                }
            }
            for (BasicBlock next : del.getNextBlock()) {
                next.getPreBlock().remove(del);
            }
            for (int count = del.getInstrList().size() - 1; count >= 0; count--) {
                del.getInstrList().get(count).remove();
            }
            deleteBlock(del);
        }

        basicBlocks.removeAll(delete);
    }

    public static void deleteBlock(BasicBlock block) {
        preMap.remove(block);
        nextMap.remove(block);
        childMap.remove(block);
        parentMap.remove(block);
        DFMap.remove(block);
    }

    public static void initFunc(Function function) {
        preMap = new HashMap<>();
        nextMap = new HashMap<>();
        childMap = new HashMap<>();
        parentMap = new HashMap<>();
        DFMap = new HashMap<>();
        for (BasicBlock block : function.getBlockList()) {
            preMap.put(block, new ArrayList<>());
            nextMap.put(block, new ArrayList<>());
            childMap.put(block, new ArrayList<>());
            DFMap.put(block, new ArrayList<>());
            parentMap.put(block, null);
        }
    }

    private static void addEdge(BasicBlock fromBlock, BasicBlock toBlock) {
        nextMap.get(fromBlock).add(toBlock);
        preMap.get(toBlock).add(fromBlock);
    }


    public static void getCFG(Function function) {
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        for (BasicBlock block : basicBlocks) {
            LinkedList<Instruction> instructions = block.getInstrList();
            Instruction lastInstr = instructions.getLast();
            if (lastInstr instanceof BrInstr && !((BrInstr) lastInstr).getIsIf()) {
                BasicBlock destBlock = ((BrInstr) lastInstr).getDestBlock();
                addEdge(block, destBlock);
            }
            else if (lastInstr instanceof BrInstr && ((BrInstr) lastInstr).getIsIf()) {
                BasicBlock ifTrueBlock = ((BrInstr) lastInstr).getIfTrueBlock();
                BasicBlock ifFalseBlock = ((BrInstr) lastInstr).getIfFalseBlock();
                addEdge(block, ifTrueBlock);
                addEdge(block, ifFalseBlock);
            }
        }
        for (BasicBlock block : basicBlocks) {
            block.setPreBlock(preMap.get(block));
            block.setNextBlock(nextMap.get(block));
        }
        function.setPreMap(preMap);
        function.setNextMap(nextMap);
    }

    public static void getDom(Function function) {
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        BasicBlock entry = basicBlocks.getFirst();
        for (BasicBlock block : basicBlocks) {
            ArrayList<BasicBlock> dom = new ArrayList<>();
            HashSet<BasicBlock> flag = new HashSet<>();
            DFS(entry, block, flag);
            for (BasicBlock basicBlock : basicBlocks) {
                if (flag.contains(basicBlock)) {
                    continue;
                }
                else if (!flag.contains(basicBlock)) {
                    dom.add(basicBlock);
                }
            }
            block.setDomBlock(dom);
        }
    }

    public static void DFS(BasicBlock entry, BasicBlock block, HashSet<BasicBlock> flag) {
        if (entry.equals(block)) {
            return;
        }
        if (flag.contains(entry)) {
            return;
        }
        flag.add(entry);
        for (BasicBlock basicBlock : entry.getNextBlock()) {
            if (!flag.contains(basicBlock) && !basicBlock.equals(block)) {
                DFS(basicBlock, block, flag);
            }
        }
    }

    public static void getImmDom(Function function) {
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        for (BasicBlock block : basicBlocks) {
            for (BasicBlock dom : block.getDomBlock()) {
                if (isIDOM(block, dom)) {
                    parentMap.put(dom, block);
                    childMap.get(block).add(dom);
                }
            }
        }
        for (BasicBlock block : basicBlocks) {
            block.setChildBlock(childMap.get(block));
            block.setParentBlock(parentMap.get(block));
        }
        function.setParentMap(parentMap);
        function.setChildMap(childMap);
    }

    public static boolean isIDOM(BasicBlock block, BasicBlock dom) {
        if (!block.getDomBlock().contains(dom) || block.equals(dom)) {
            return false;
        }
        for (BasicBlock mid : block.getDomBlock()) {
            if (!mid.equals(block) && !mid.equals(dom) && mid.getDomBlock().contains(dom)) {
                return false;
            }
        }
        return true;
    }

    public static void getDF(Function function) {
        for (Map.Entry<BasicBlock, ArrayList<BasicBlock>> entry : nextMap.entrySet()) {
            BasicBlock a = entry.getKey();
            for (BasicBlock b : entry.getValue()) {
                BasicBlock temp = a;
                while (!(temp == null) && (!temp.getDomBlock().contains(b) || temp.equals(b))) {
                    DFMap.get(temp).add(b);
                    temp = temp.getParentBlock();
                }
            }
        }
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        for (BasicBlock block : basicBlocks) {
            block.setDF(DFMap.get(block));
        }
    }

    public static void buildIdomTree(BasicBlock block, int depth) {
        block.setDomDepth(depth);
        for (BasicBlock basicBlock : block.getChildBlock()) {
            buildIdomTree(basicBlock, depth + 1);
        }
    }

    public static ArrayList<BasicBlock> getDomPostOrder(Function function) {
        ArrayList<BasicBlock> postOrder = new ArrayList<>();
        Stack<BasicBlock> stack = new Stack<>();
        stack.add(function.getBlockList().get(0));
        HashSet<BasicBlock> visited = new HashSet<>();
        while (!stack.isEmpty()) {
            BasicBlock block = stack.peek();
            if (visited.contains(block)) {
                postOrder.add(stack.pop());
                continue;
            }
            visited.add(block);

            for (BasicBlock basicBlock : block.getChildBlock()) {
                stack.push(basicBlock);
            }
        }
        return postOrder;
    }

}
