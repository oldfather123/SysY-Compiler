package midend.analysis;

import midend.*;
import midend.LLVMType.IntegerType;
import midend.instr.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Stack;

public class LoopInfo {
    public static HashMap<BasicBlock, Loop> loopInfo = new HashMap<>();
    public static ArrayList<Loop> topLevelLoop = new ArrayList<>();

    public static void loopAnalysis(Function function) {
        loopInfo = new HashMap<>();
        topLevelLoop = new ArrayList<>();
        CFGBuilder.initFunc(function);
        CFGBuilder.getDom(function);
        ArrayList<BasicBlock> postOrder = CFGBuilder.getDomPostOrder(function);
        //后序遍历domTree(从而找到最内层循环)
        Stack<BasicBlock> backEdge;
        for (BasicBlock block : postOrder) {
            backEdge = new Stack<>();
            for (BasicBlock basicBlock : block.getPreBlock()) {
                if (block.getDomBlock().contains(basicBlock)) { //一个节点支配其前继
                    backEdge.push(basicBlock);
                }
            }

            if (!backEdge.isEmpty()) {
                Loop L = new Loop(block);
                discoverAndMapSubLoop(L, backEdge);
            }
        }

        //完善循环嵌套关系
        for (BasicBlock block : postOrder) {
            populateLoops(block);
        }

        function.setLoopInfo(loopInfo);
        function.setTopLevelLoop(topLevelLoop);
    }

    public static void discoverAndMapSubLoop(Loop L, Stack<BasicBlock> backEdge) {
        List<BasicBlock> reverseCFG = backEdge;
        while (!reverseCFG.isEmpty()) {
            BasicBlock preBlock = reverseCFG.remove(reverseCFG.size() - 1);
            Loop subloop = loopInfo.get(preBlock);
            if (subloop == null) {
                //TODO:if (!domTree.isReachableFromEntry(predBB)) {
                //                    continue;
                //                }
                loopInfo.put(preBlock, L);
                if (preBlock.equals(L.getHeader())) {
                    continue;
                }
                //将所有前继加入到list中
                reverseCFG.addAll(preBlock.getPreBlock());
            } else {
                while (subloop.getParentLoop() != null) {
                    subloop = subloop.getParentLoop();
                }
                if (subloop.equals(L)) {
                    continue;
                }

                subloop.setParentLoop(L);
                preBlock = subloop.getHeader();
                for (BasicBlock block : preBlock.getPreBlock()) {
                    if (loopInfo.get(block) == null || !loopInfo.get(block).equals(subloop)) {
                        reverseCFG.add(block);
                    }
                }
            }
        }
    }

    public static void populateLoops(BasicBlock block) {
        Loop subloop = loopInfo.get(block);
        if (subloop != null && block.equals(subloop.getHeader())) {
            if (subloop.getParentLoop() != null) {
                subloop.getParentLoop().addSubLoop(subloop);
            } else {
                topLevelLoop.add(subloop);
            }
            subloop.reverseBlock();
            subloop.reverseSubLoop();
            subloop = subloop.getParentLoop();
        }
        while (true) {
            if (subloop == null) {
                break;
            }
            subloop.addBlock(block);
            subloop = subloop.getParentLoop();
        }
    }

    /*template <class BlockT, class LoopT>
	void LoopBase<BlockT, LoopT>::getExitBlocks(
		SmallVectorImpl<BlockT*> & ExitBlocks) const {
		assert(!isInvalid() && "Loop not in a valid state!");
		for (const auto BB : blocks())
			for (const auto& Succ : children<BlockT*>(BB))
				if (!contains(Succ))
					// Not in current loop? It must be an exit block.
					ExitBlocks.push_back(Succ);
	}*/

    //循环内即将退出的block
    public static void getExitingBlocks() {
        for (Loop loop : loopInfo.values()) {
            ArrayList<BasicBlock> basicBlocks = loop.getBasicBlocks();
            for (BasicBlock block : basicBlocks) {
                if (block.getChildBlock().isEmpty()) {
                    continue;
                }
                for (BasicBlock child : block.getChildBlock()) {
                    if (!basicBlocks.contains(child)) {
                        loop.addExitBlock(child);
                    }
                }
            }
        }
    }

    //循环头的前继，并且其只有循环一个后继
    //判断循环头的唯一前继是否以循环头为唯一后继，也判断该前继是否为唯一前继，同时判断前继是否能将代码提升到这个前继中
//    public static BasicBlock getLoopPreheader(Loop loop) {
//        BasicBlock block = getLoopPredecessor(loop);
//        if (block == null || !block.isLegalToHoistInto()) {
//            return null;
//        }
//        //是否为唯一后继
//        ArrayList<BasicBlock> childs = block.getChildBlock();
//        if (childs.size() != 1 || childs.get(0).equals(loop.getHeader())) {
//            return null;
//        }
//        return block;
//    }

    //获取循环头的唯一前继
//    public static BasicBlock getLoopPredecessor(Loop loop) {
//        BasicBlock out = null;
//        BasicBlock header = loop.getHeader();
//        for (BasicBlock pred : header.getPreBlock()) {
//            if (!loop.getBasicBlocks().contains(pred)) {
//                if (out != null && out != pred) {
//                    return null;
//                }
//                out = pred;
//            }
//        }
//        return out;
//    }

    //循环内唯一跳转到循环头的结点，不可以有break或continue
    public static BasicBlock getLoopLatch(Loop loop) {
        BasicBlock head = loop.getHeader();
        BasicBlock latch = null;
        for (BasicBlock pred : head.getPreBlock()) {
            if (loop.getBasicBlocks().contains(pred)) {
                if (latch != null) {
                    return null;
                }
                latch = pred;
            }
        }
        return latch;
    }

    public static void iteratorAnalysis(Function function) {
        for (Loop loop : function.getLoops()) {
            Value itBegin = null; //循环开始值
            Value itVar = null; //循环变量
            Value itEnd = null; //循环结束值
            String op = null; //迭代变量的运算
            Value itAlu = null; //迭代alu
            Value itChange = null;
            Value itCmp = null;

            BasicBlock header = loop.getHeader();
            BrInstr brInstr = (BrInstr) header.getInstrList().getLast();
            if (brInstr.getValueList().size() == 0) {
                loop.setLoopIterator(itBegin, itCmp, itVar, itEnd, op, itAlu, itChange, true);
                return;
            }
            Value brValue = brInstr.getValue();
            itCmp = brValue;
            if (brValue instanceof ConstInt && ((ConstInt) brValue).getValue() == 0) {
                loop.setLoopIterator(itBegin, itCmp, itVar, itEnd, op, itAlu, itChange, true);
            }  else if (brValue instanceof ConstInt && ((ConstInt) brValue).getValue() == 1) {
                itBegin = new ConstInt(IntegerType.i32, 1);
                itEnd = new ConstInt(IntegerType.i32, 1);
                itChange = new ConstInt(IntegerType.i32, 0);
                loop.setLoopIterator(itBegin, itCmp, itVar, itEnd, op, itAlu, itChange, false);
            } else {
                if (!(brValue instanceof CmpInstr)) {
                    throw new RuntimeException("brValue is not cmp\n");
                }
                if (((CmpInstr) brValue).getLeft() instanceof PhiInstr) {
                    itVar = ((CmpInstr) brValue).getLeft();
                    itEnd = ((CmpInstr) brValue).getRight();
                } else if (((CmpInstr) brValue).getRight() instanceof PhiInstr) {
                    itVar = ((CmpInstr) brValue).getRight();
                    itEnd = ((CmpInstr) brValue).getLeft();
                } else {
                    continue;
//                    throw new RuntimeException("something bad happened\n");
                }

                BasicBlock latchBlock = getLoopLatch(loop);
                if (latchBlock == null) {
                    continue;
                } else {
                    int index = ((PhiInstr) itVar).getPreBlockList().indexOf(latchBlock);
                    itAlu = ((PhiInstr) itVar).getValueFrom(latchBlock);
                    if (!(itAlu instanceof ALUInstr)) {
                        continue;
                    }
                    if (index == 0) {
                        itBegin = ((PhiInstr) itVar).getValue(1);
                    } else if (index == 1) {
                        itBegin = ((PhiInstr) itVar).getValue(0);
                    } else {
                        throw new RuntimeException("index problem\n");
                    }
                }

                if (((ALUInstr) itAlu).getLeft().equals(itVar)) {
                    itChange = ((ALUInstr) itAlu).getRight();
                } else if (((ALUInstr) itAlu).getRight().equals(itVar)) {
                    itChange = ((ALUInstr) itAlu).getLeft();
                } else {
                    continue;
                }
                op = ((ALUInstr) itAlu).getOpStr();
                if (op.equals("+") || op.equals("-") || op.equals("*")) {
                    loop.setLoopIterator(itBegin, itCmp, itVar, itEnd, op, itAlu, itChange, false);
                }
            }
        }
    }

    public static void analysisUseLess(Loop loop) {
        Value itVar = loop.getItVar();
        Value itAlu = loop.getItAlu();
        if (loop.isUseLess()) {
            return;
        }
        if (loop.getHeader().getPreBlock().size() != 2) {
            return;
        }
        if (loop.getExitBlock().size() != 1) {
            return;
        }
        if (loop.getSubLoops().size() != 0) {
            return;
        }
        BasicBlock latch = getLoopLatch(loop);
        if (latch == null) {
            return;
        }
        for (BasicBlock block : loop.getBasicBlocks()) {
            for (Instruction instruction : block.getInstrList()) {
                if (!(instruction.equals(itAlu) || instruction.equals(loop.getHeader().getInstrList().getLast()) ||
                        instruction.equals(itVar) || instruction.equals(latch.getInstrList().getLast()) || instruction.equals(((BrInstr) loop.getHeader().getInstrList().getLast()).getValue()))) {
                    return;
                }
            }
        }

        for (User user : itVar.getUserList()) {
            Instruction instruction = (Instruction) user;
            if (!(instruction.equals(itAlu) || instruction.equals(loop.getHeader().getInstrList().getLast()) ||
                    instruction.equals(itVar) || instruction.equals(latch.getInstrList().getLast()) || instruction.equals(((BrInstr) loop.getHeader().getInstrList().getLast()).getValue()))) {
                return;
            }
        }

        loop.setUseLess();
    }

    //所有exitBlock的前继只能在循环内
//    public static void hasDedicateExits() {
//
//    }
}
