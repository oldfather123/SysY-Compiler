package frontend.ir.cloner;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.EmptyInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;

import java.util.*;

public class UnrollLoopEntire extends LoopCloner{
    private static UnrollLoopEntire instance;
    protected static BasicBlock exitTarget = null;
    protected static BasicBlock latchNow = null;
    protected static BasicBlock latchLast = null;
    protected static final HashMap<Value, Value> latchValueMap = new HashMap<>();

    protected UnrollLoopEntire() {
        super();
    }

    public static UnrollLoopEntire getInstance() {
        if (instance == null) {
            instance = new UnrollLoopEntire();
        }
        return instance;
    }



    public void makeBBgraphForLoop(LoopInfo loopInfo, List<BasicBlock> loopBBs) {
        BasicBlock insertPoint = latchNow;
        Function function = loopInfo.getFunction();
        BasicBlock newHeader = null;
        BasicBlock newLatch = null;
        for (BasicBlock bb : loopBBs) {
            BasicBlock newBB = new BasicBlock(function.getAndAddBlkIdx());
            if (loopInfo.getParentLoop() != null) {
                loopInfo.getParentLoop().addBodyBlock(newBB);
                newBB.setLoopBelonged(loopInfo.getParentLoop());
            }
            newBB.insertAfter(insertPoint);
            insertPoint = newBB;
            valueProjection.put(bb, newBB);
            if (loopInfo.getLatchBlocks().contains(bb)) {
                newLatch = newBB;
            } else if (loopInfo.getLoopHeader() == bb) {
                newHeader = newBB;
            }
        }
        latchNow.getLastInstr().forceRemoveFromList();
        new JumpInstr(newHeader, latchNow);
        latchLast = latchNow;
        latchNow = newLatch;
    }

    public void makePhiForLoop(Function targetFunc, LoopInfo loop) {

        for (Map.Entry<EmptyInstr, PhiInstr> entry : needPhi.entrySet()) {
            EmptyInstr empty = entry.getKey();
            PhiInstr phi = entry.getValue();

            if (phi.getParentBB() == loop.getLoopHeader()) {
                Value fromLatch = latchValueMap.get(phi.getOperandMap().get(loop.getLatchBlocks().iterator().next()));
                empty.replaceUseWith(fromLatch);
                empty.removeFromList();
                valueProjection.put(phi, fromLatch);
                continue;
            }

//            PhiInstr newPhi = new PhiInstr(phi.getType(), targetFunc.getAndAddRegIdx(), empty.getParentBB(), phi.isLCSSA());
//            for (Map.Entry<BasicBlock, Value> opBB : phi.getOperandMap().entrySet()) {
//                Value bb = newValueGetter(opBB.getKey());
//                Value op = newValueGetter(opBB.getValue());
//                newPhi.addOperand((BasicBlock) bb, op);
//            }
//            empty.replaceUseWith(newPhi);
//            newPhi.insertAfter(empty);
//            empty.removeFromList();
            PhiInstr newPhi = phiInstrCreator(phi, empty, targetFunc);
            valueProjection.put(phi, newPhi);
        }
    }

    public void processExitTarget(LoopInfo loopInfo, BasicBlock from) {
        for (Instruction instr : exitTarget.getInstrList()) {
            if (instr instanceof PhiInstr phi && phi.isLCSSA()) {
                phi.modifyUse(loopInfo.getLoopHeader(), from);
                from.getUserList().add(phi);
                loopInfo.getLoopHeader().getUserList().remove(phi);
                Value onlyVal = phi.getOperandMap().get(from);
                Value newValue = newValueGetter(onlyVal);
                phi.modifyUse(onlyVal, newValue);
                newValue.getUserList().add(phi);
                onlyVal.getUserList().remove(phi);
            }
        }
    }

    public void postProcess(LoopInfo loopInfo) {
        BasicBlock lastHeader = new BasicBlock(loopInfo.getFunction().getAndAddBlkIdx());

        if (loopInfo.getParentLoop() != null) {
            loopInfo.getParentLoop().addBodyBlock(lastHeader);
            lastHeader.setLoopBelonged(loopInfo.getParentLoop());
        }

        lastHeader.insertAfter(latchNow);
        valueProjection.clear();
        needPhi.clear();

        latchNow.getLastInstr().forceRemoveFromList();
        new JumpInstr(lastHeader, latchNow);
        latchLast = latchNow;
        latchNow = lastHeader;
        valueProjection.put(loopInfo.getLatchBlocks().iterator().next(), latchLast);

        for (Instruction instr : loopInfo.getLoopHeader().getInstrList()) {
            if (!(instr instanceof Terminator)) {
                cloneInstr(instr, lastHeader, loopInfo.getFunction());
            }
        }
        makePhiForLoop(loopInfo.getFunction(), loopInfo);

        latchNow.getLastInstr().forceRemoveFromList();
        new JumpInstr(exitTarget, lastHeader);
        processExitTarget(loopInfo, lastHeader);
    }

    public ArrayList<BasicBlock> preProcess(LoopInfo loopInfo) {
        // 读取所有块
        HashSet<BasicBlock> loopBBs = new HashSet<>();
        loopInfo.getAllBlocks(loopBBs);

        // 头先不跳出去
        BranchInstr br = (BranchInstr) loopInfo.getLoopHeader().getLastInstr();

        BasicBlock bbInLoop = br.getThenBlk().isContainsInLoop(loopInfo) ? br.getThenBlk() : br.getElseBlk();
        br.forceRemoveFromList();
        loopInfo.getLoopHeader().open();
        new JumpInstr(bbInLoop, loopInfo.getLoopHeader());

        // 排好序，便于查看
        ArrayList<BasicBlock> sortedBBs = loopInfo.getAllBlocksSorted();

        // 把回边来指记录一下
        for (Instruction instruction: loopInfo.getLoopHeader().getInstrList()) {
            if (instruction instanceof PhiInstr phi) {
                for (Map.Entry<BasicBlock, Value> opBB : phi.getOperandMap().entrySet()) {
                    if (opBB.getKey() == loopInfo.getLatchBlocks().iterator().next()) {
                        latchValueMap.put(opBB.getValue(), opBB.getValue());
                    }
                }
            }
        }
        return sortedBBs;
    }

    public void unrollOnce(LoopInfo loopInfo, ArrayList<BasicBlock> sortedBBs) {
        valueProjection.clear();
        needPhi.clear();
        makeBBgraphForLoop(loopInfo, sortedBBs);
        cloneBlockDfs(loopInfo.getLoopHeader(),
                (BasicBlock) valueProjection.get(loopInfo.getLoopHeader()), loopInfo.getFunction(), loopInfo);
        makePhiForLoop(loopInfo.getFunction(), loopInfo);

        latchValueMap.replaceAll((key, _) -> newValueGetter(key));
    }


    public void unrollLoopEntire(LoopInfo loopInfo, int times) {
        valueProjection.clear();
        needPhi.clear();
        exitTarget = loopInfo.getExitTargets().iterator().next();
        latchNow = loopInfo.getLatchBlocks().iterator().next();

        ArrayList<BasicBlock> sortedBBs = preProcess(loopInfo);

        for (int i = 0; i < times; i++) {
            unrollOnce(loopInfo, sortedBBs);
        }

        postProcess(loopInfo);

        // 最后再解除回边指向
        for (Instruction instruction : loopInfo.getLoopHeader().getInstrList()) {
            if (instruction instanceof PhiInstr phi) {
                phi.selfFix();
            }
        }
    }
}
