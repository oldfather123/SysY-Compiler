package midend.optimism;

import Utils.LibFunction;
import midend.*;
import midend.LLVMType.IntegerType;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;

public class Parallel {
    private Module module;
    private HashSet<BasicBlock> know = new HashSet<>();
    private int parallel = 4;

    public Parallel(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            LoopInfo.getExitingBlocks();
            LoopInfo.iteratorAnalysis(function);
            know.clear();
            for (Loop loop : function.getLoops()) {
                if (!know.contains(loop.getHeader())) {
                    markLoop(loop, function);
                }
            }
        }
    }

    public boolean isPureLoop(Loop loop) {
        if (loop.getSubLoops().size() == 0) {
            return true;
        }
        if (loop.getSubLoops().size() != 1) {
            return false;
        }
        if (!(loop.getHeader().getPreBlock().size() == 2 && LoopInfo.getLoopLatch(loop) != null && loop.getExitBlock().size() == 1)) {
            //TODO:Exitingblock = 1
            return false;
        }
        if (!loop.isIter()) {
            return false;
        }
        return isPureLoop(loop.getSubLoops().get(0));
    }

    public void markLoop(Loop loop, Function function) {
        if (!isPureLoop(loop)) {
            return;
        }
        HashSet<BasicBlock> basicBlocks = new HashSet<>();
        HashSet<Value> itVars = new HashSet<>();
        HashSet<Loop> loops = new HashSet<>();
        DFS(basicBlocks, itVars, loops, loop);

        for (Loop loop1 : loops) {
            for (Instruction instruction : loop1.getHeader().getInstrList()) {
                if (instruction instanceof PhiInstr && !instruction.equals(loop1.getItVar())) {
                    return;
                }
            }
        }

        for (BasicBlock block : basicBlocks) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof CallInstr) {
                    return;
                }
                //UseOutLoop
                for (User user : instruction.getUserList()) {
                    if (user instanceof Instruction && !basicBlocks.contains(((Instruction) user).getBasicBlock())) {
                        return;
                    }
                }
                if (instruction instanceof GetElementPtrInstr) {
                    if (!(((GetElementPtrInstr) instruction).getTarget() instanceof GlobalVar)) {
                        return;
                    }
                    for (Value value : ((GetElementPtrInstr) instruction).getIndexes()) {
                        if (value instanceof ConstInt && ((ConstInt) value).getValue() == 0) {
                            continue;
                        }
                        if (!itVars.contains(value)) {
                            return;
                        }
                    }
                }
            }
        }

        Value itEnd = loop.getItEnd();
        if (itEnd instanceof ConstInt) { //??????????
            return;
        }
        int count = 0;
        BasicBlock entering = null;
        for (BasicBlock pre : loop.getHeader().getPreBlock()) {
            if (!loop.getBasicBlocks().contains(pre)) {
                count++;
                entering = pre;
            }
            if (count > 1) {
                return;
            }
        }
        if (entering == null) {
            return;
        }
        //构建
        String op = loop.getOp();
        Value itVar = loop.getItVar();
        Value itCmp = loop.getItCmp();
        BasicBlock latch = LoopInfo.getLoopLatch(loop);
        BasicBlock exitBlock = loop.getExitBlock().get(0);
        BasicBlock exitingBlock = null;
        int index = 1 - loop.getHeader().getPreBlock().indexOf(latch);
        for (BasicBlock pre : exitBlock.getPreBlock()) {
            if (loop.getBasicBlocks().contains(pre)) {
                if (exitingBlock != null) {
                    throw new RuntimeException("exiting more than one\n");
                }
                exitingBlock = pre;
            }
        }

        BasicBlock parallelStart = new BasicBlock(UndefinedType.undefined, function);
        function.getBlockList().add(parallelStart);
        loop.getBasicBlocks().add(parallelStart);
        BasicBlock parallelEnd = new BasicBlock(UndefinedType.undefined, function);
        function.getBlockList().add(parallelEnd);
        loop.getBasicBlocks().add(parallelEnd);

        if (!((CmpInstr) itCmp).getOpStr().equals("<")) {
            return;
        }

        CallInstr startCall = new CallInstr(LibFunction.findFunc("parallel_start").getRetType(), LibFunction.findFunc("parallel_start"), new ArrayList<>());
        startCall.setBasicBlock(parallelStart);
        ALUInstr mul1 = new ALUInstr(IntegerType.i32, Arrays.asList(startCall, itEnd), InstrType.MUL, parallelStart);
        ALUInstr div1 = new ALUInstr(IntegerType.i32, Arrays.asList(mul1, new ConstInt(IntegerType.i32, parallel)), InstrType.SDIV, parallelStart);
        ((PhiInstr) itVar).setValue(index, div1);
        ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(startCall, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, parallelStart);
        ALUInstr mul2 = new ALUInstr(IntegerType.i32, Arrays.asList(add1, itEnd), InstrType.SUB, parallelStart);
        ALUInstr div2 = new ALUInstr(IntegerType.i32, Arrays.asList(mul2, new ConstInt(IntegerType.i32, parallel)), InstrType.SDIV, parallelStart);
        ((CmpInstr) itCmp).setValue(1, div2);
        BrInstr brInstr = new BrInstr(loop.getHeader());
        brInstr.setBasicBlock(loop.getHeader());
        brInstr.setBasicBlock(parallelStart);
        parallelStart.addInstr(startCall);
        parallelStart.addInstr(mul1);
        parallelStart.addInstr(div1);
        parallelStart.addInstr(add1);
        parallelStart.addInstr(mul2);
        parallelStart.addInstr(div2);
        parallelStart.addInstr(brInstr);

        ((BrInstr) entering.getInstrList().getLast()).modifyBlock(loop.getHeader(), parallelStart);

        ArrayList<Value> params = new ArrayList<>();
        params.add(startCall);
        CallInstr endCall = new CallInstr(LibFunction.findFunc("parallel_end").getRetType(), LibFunction.findFunc("parallel_end"), params);
        endCall.setBasicBlock(parallelEnd);
        BrInstr brInstr1 = new BrInstr(exitBlock);
        brInstr1.setBasicBlock(parallelEnd);
        parallelEnd.addInstr(endCall);
        parallelEnd.addInstr(brInstr1);

        ((BrInstr) exitingBlock.getInstrList().getLast()).modifyBlock(exitBlock, parallelEnd);

        know.addAll(basicBlocks);
    }

    public void DFS(HashSet<BasicBlock> basicBlocks, HashSet<Value> itVars, HashSet<Loop> loops, Loop loop) {
        loops.add(loop);
        basicBlocks.addAll(loop.getBasicBlocks());
        itVars.add(loop.getItVar());
        for (Loop loop1 : loop.getSubLoops()) {
            DFS(basicBlocks, itVars, loops, loop1);
        }
    }
}
