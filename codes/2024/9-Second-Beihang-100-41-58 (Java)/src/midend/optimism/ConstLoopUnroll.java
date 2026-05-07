package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.CFGBuilder;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.*;

public class ConstLoopUnroll {
    private Module module;
//    private HashMap<Function, ArrayList<Loop>> loops = new HashMap<>();
//    private HashMap<Function, ArrayList<Loop>> loopOrdered = new HashMap<>();
    private BasicBlock latch;
    private BasicBlock head;
    private BasicBlock entering;
    private BasicBlock next;
    private BasicBlock exit;

    public ConstLoopUnroll(Module module) {
        this.module = module;
    }

    public void run() {
//        for (Function function : module.getFunctions()) {
//            loops.put(function, function.getLoops());
//            loopOrdered.put(function, new ArrayList<>());
//        }
//        for (Function function : module.getFunctions()) {
//            Loop first = function.
//        }
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            LoopInfo.getExitingBlocks();
            LoopInfo.iteratorAnalysis(function);
            ArrayList<Loop> order = new ArrayList<>();
            for (Loop loop : function.getTopLevelLoop()) {
                order.addAll(DFSLoop(loop));
            }
            for (Loop loop : order) {
                LoopUnroll(loop);
            }
        }
    }

    public void LoopUnroll(Loop loop) {
        if (!loop.isIter()) {
            return;
        }

        if (!(loop.getItBegin() instanceof ConstInt) || !(loop.getItChange() instanceof ConstInt) || !(loop.getItEnd() instanceof ConstInt)) {
            return;
        }

        int init = ((ConstInt) loop.getItBegin()).getValue();
        int change = ((ConstInt) loop.getItChange()).getValue();
        int end = ((ConstInt) loop.getItEnd()).getValue();
        if (change == 0) {
            return;
        }
        int numbers = getNumbers(((ALUInstr) loop.getItAlu()).getOpStr(), ((CmpInstr) loop.getItCmp()).getOpStr(), change, end, init);
        if (numbers == -1 || numbers < 0) {
            return;
        }
        HashSet<BasicBlock> exits = new HashSet<>();
        HashSet<BasicBlock> basicBlocks = new HashSet<>(loop.getBasicBlocks());
        for (Loop loop1 : loop.getSubLoops()) {
            DFSForCheck(loop1, exits, basicBlocks);
        }
        for (BasicBlock exit : exits) {
            if (!basicBlocks.contains(exit)) {
                return;
            }
        }
        int count = 0;
        for (BasicBlock block : loop.getBasicBlocks()) {
            count += block.getInstrList().size();
        }
//        for (Loop subLoop: loop.getSubLoops()) {
//            count += cntDFS(subLoop);
//        }
        if (count * numbers > 8000) {
            return;
        }

        head = loop.getHeader();
        for (BasicBlock block : head.getPreBlock()) {
            if (loop.getBasicBlocks().contains(block)) {
                latch = block;
            } else {
                entering = block;
            }
        }
        if (loop.getExitBlock().size() > 1) {
            return;
        }
        exit = loop.getExitBlock().get(0);
        for (BasicBlock block : head.getNextBlock()) {
            if (!block.equals(exit)) {
                next = block;
            }
        }
        if (next.getPreBlock().size() != 1) {
            return;
        }

        DoLoopUnroll(loop, numbers);
    }

    public void DoLoopUnroll(Loop loop, int number) {
        HashMap<Value, Value> phiMap = new HashMap<>();
        HashMap<Value, Value> begin = new HashMap<>();
        ArrayList<PhiInstr> phiInstrs = new ArrayList<>();
        for (Instruction instruction : head.getInstrList()) {
            if (!(instruction instanceof PhiInstr)) {
                break;
            }
            phiInstrs.add((PhiInstr) instruction);
        }

        int index = head.getPreBlock().indexOf(latch);
        for (PhiInstr phiInstr : phiInstrs) {
            Value newVal = phiInstr.getValue(index);
//            phiInstr.removeValue(index);
            phiMap.put(phiInstr, newVal);
            begin.put(phiInstr, newVal);
        }

        BrInstr brInstr = (BrInstr) head.getInstrList().getLast();
        brInstr.remove();
        BrInstr newBr = new BrInstr(next);
        newBr.setBasicBlock(head);
        head.addInstr(newBr);
        latch.getInstrList().removeLast();

        HashSet<BasicBlock> allBlock = new HashSet<>();
        getAllBlock(loop, allBlock);
        allBlock.remove(head);

//        int blockIndex = loop.getHeader().getParentFunc().getBlockList().indexOf(head);
//        loop.getHeader().getParentFunc().getBlockList().removeAll(allBlock);

        ArrayList<BasicBlock> dfs = DFSBlock(next);

        BasicBlock oldNext = next;
        BasicBlock oldLatch = latch;
        HashMap<Value, Value> valueMap = new HashMap<>();
        Function.cloneMap = valueMap;
        for (Value value : phiMap.keySet()) {
            valueMap.put(value, phiMap.get(value));
        }
        for (int count = 0; count < number - 1; count++) {
            for (BasicBlock block : dfs) {
                BasicBlock newBlock = new BasicBlock(UndefinedType.undefined, loop.getHeader().getParentFunc());
//                loop.getHeader().getParentFunc().getBlockList().add(blockIndex + 1, newBlock);
                loop.getHeader().getParentFunc().addBB(newBlock);
                valueMap.put(block, newBlock);
            }
            for (BasicBlock block : dfs) {
                BasicBlock copyBlock = (BasicBlock) Function.cloneMap.getOrDefault(block, block);
                for (Instruction instruction : block.getInstrList()) {
                    Instruction copy = instruction.clone(copyBlock);
                    copyBlock.addInstr(copy);
                    valueMap.put(instruction, copy);
                }
            }
            //TODO:CFG
            ArrayList<BasicBlock> tmpDfs = new ArrayList<>(dfs);
            dfs.clear();
            for (BasicBlock block : tmpDfs) {
                dfs.add((BasicBlock) valueMap.get(block));
            }
            Function function = loop.getHeader().getParentFunc();

            ((BrInstr) head.getInstrList().getLast()).modifyBlock(next, (BasicBlock) valueMap.getOrDefault(next, next));

            BasicBlock newHeadNext = (BasicBlock) valueMap.get(oldNext);
            BasicBlock newLatch = (BasicBlock) valueMap.get(oldLatch);
            BrInstr brInstr1 = new BrInstr(newHeadNext);
            brInstr1.setBasicBlock(oldLatch);
            oldLatch.addInstr(brInstr1);
            CFGBuilder cfgBuilder = new CFGBuilder(null);
            cfgBuilder.initFunc(function);
            cfgBuilder.getCFG(function);
            oldLatch.setNextBlock(newHeadNext);
            newHeadNext.setPreBlock(oldLatch);

            ArrayList<PhiInstr> phi = new ArrayList<>();
            for (BasicBlock block : tmpDfs) {
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof PhiInstr) {
                        phi.add((PhiInstr) instruction);
                    } else {
                        break;
                    }
                }
            }
            for (PhiInstr phiInstr : phi) {
                for (int count0 = 0; count0 < phiInstr.getValueList().size(); count0++) {
                    BasicBlock pre = phiInstr.getPreBlock(count0);
                    BasicBlock newBlock = (BasicBlock) valueMap.getOrDefault(pre, pre);
                    PhiInstr copyPhi = (PhiInstr) valueMap.getOrDefault(phiInstr, phiInstr);
                    int index0 = copyPhi.getPreBlockList().indexOf(newBlock);

                    Value beforeValue = phiInstr.getValue(count0);
                    Value nowValue;
                    if (beforeValue instanceof ConstInt) {
                        nowValue = new ConstInt(IntegerType.i32, ((ConstInt) beforeValue).getValue());
                    } else if (beforeValue instanceof ConstFloat) {
                        nowValue = new ConstFloat(FloatType.f32, ((ConstFloat) beforeValue).getValue());
                    } else if (beforeValue instanceof UndefinedValue) {
                        nowValue = new UndefinedValue(beforeValue.getType());
                    } else {
                        nowValue = valueMap.get(beforeValue);
                    }
                    copyPhi.addOption(nowValue, newBlock);

                }
            }

            for (Value key : begin.keySet()) {
                begin.put(key, valueMap.getOrDefault(begin.get(key), begin.get(key)));
            }


            oldNext = newHeadNext;
            oldLatch = newLatch;
        }

        oldLatch.setNextBlock(exit);
        exit.setPreBlock(oldLatch);
        BrInstr brInstr1 = new BrInstr(exit);
        brInstr1.setBasicBlock(oldLatch);
        oldLatch.addInstr(brInstr1);

        ArrayList<PhiInstr> phiInstrs1 = new ArrayList<>();
        for (Instruction instruction : exit.getInstrList()) {
            if (instruction instanceof PhiInstr) {
                phiInstrs1.add((PhiInstr) instruction);
            } else {
                break;
            }
        }
        for (PhiInstr phiInstr : phiInstrs1) {
            for (Value value : phiInstr.getValueList()) {
                if (value instanceof Instruction && ((Instruction) value).getBasicBlock().equals(head)) {
                    phiInstr.addOption(begin.get(value), ((Instruction) value).getBasicBlock());
                }
            }
        }


    }

    public ArrayList<BasicBlock> DFSBlock(BasicBlock block) {
        HashSet<BasicBlock> visited = new HashSet<>();
        Stack<BasicBlock> stack = new Stack<>();
        ArrayList<BasicBlock> dfs = new ArrayList<>();
        stack.push(block);
        visited.add(block);
        while (!stack.isEmpty()) {
            BasicBlock basicBlock = stack.pop();
            dfs.add(basicBlock);
            if (!basicBlock.getNextBlock().isEmpty()) {
                for (BasicBlock block1 : basicBlock.getNextBlock()) {
                    if (!visited.contains(block1)) {
                        visited.add(block1);
                        stack.push(block1);
                    }
                }
            }
        }
        return dfs;
    }

    public void getAllBlock(Loop loop, HashSet<BasicBlock> allBlock) {
        allBlock.addAll(loop.getBasicBlocks());
        for (Loop loop1 : loop.getSubLoops()) {
            getAllBlock(loop1, allBlock);
        }
    }

    public void DFSForCheck(Loop loop, HashSet<BasicBlock> exits, HashSet<BasicBlock> basicBlocks) {
        exits.addAll(loop.getExitBlock());
        basicBlocks.addAll(loop.getBasicBlocks());
        for (Loop loop1 : loop.getSubLoops()) {
            DFSForCheck(loop1, exits, basicBlocks);
        }
    }

    public int getNumbers(String aluOp, String cmpOp, int change, int end, int init) {
        int number = -1;
        if (aluOp.equals("-")) {
            if (cmpOp.equals("!=")) {
                number = ((init - end) % change == 0)? (init - end) / change : -1;
            } else if (cmpOp.equals("==")) {
                number = (init == end)? 1 : 0;
            } else if (cmpOp.equals("<") || cmpOp.equals(">")) {
                number = ((init - end) % change == 0)? (init - end) / change : (init - end) / change + 1;
            } else if (cmpOp.equals("<=") || cmpOp.equals(">=")) {
                number = (init - end) / change + 1;
            }
        } else if (aluOp.equals("+")) {
            if (cmpOp.equals("!=")) {
                number = ((end - init) % change == 0)? (end - init) / change : -1;
            } else if (cmpOp.equals("==")) {
                number = (init == end)? 1 : 0;
            } else if (cmpOp.equals("<") || cmpOp.equals(">")) {
                number = ((end - init) % change == 0)? (end - init) / change : (end - init) / change + 1;
            } else if (cmpOp.equals("<=") || cmpOp.equals(">=")) {
                number = (end - init) / change + 1;
            }
        } else if (aluOp.equals("*")) {
            double val = Math.log(end / init) / Math.log(change);
            boolean tag = init * Math.pow(change, val) == end;
            if (cmpOp.equals("==")) {
                number = (init == end)? 1 : 0;
            } else if (cmpOp.equals("!=")) {
                number = tag? (int) val : -1;
            } else if (cmpOp.equals("<") || cmpOp.equals(">")) {
                number = tag? (int) val : (int) val + 1;
            } else if (cmpOp.equals("<=") || cmpOp.equals(">=")) {
                number = (int) val + 1;
            }
        }
        return number;
    }

    public ArrayList<Loop> DFSLoop(Loop loop) {
        ArrayList<Loop> loops1 = new ArrayList<>();
        for (Loop loop1 : loop.getSubLoops()) {
            loops1.addAll(DFSLoop(loop1));
        }
        loops1.add(loop);
        return loops1;
    }
}
