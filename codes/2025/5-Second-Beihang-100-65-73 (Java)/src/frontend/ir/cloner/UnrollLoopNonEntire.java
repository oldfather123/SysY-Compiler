package frontend.ir.cloner;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.SRemInstr;
import frontend.ir.instr.binop.SubInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.EmptyInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

public class UnrollLoopNonEntire extends UnrollLoopEntire{
    private static UnrollLoopNonEntire instance;
    private static final HashSet<BasicBlock> newLoopBody = new HashSet<>();
    private UnrollLoopNonEntire(){
        super();
    }

    public static UnrollLoopNonEntire getInstance(){
        if(instance == null){
            instance = new UnrollLoopNonEntire();
        }
        return instance;
    }

    @Override
    public void makeBBgraphForLoop(LoopInfo loopInfo, List<BasicBlock> loopBBs) {
        BasicBlock insertPoint = latchNow;
        Function function = loopInfo.getFunction();
        BasicBlock newHeader = null;
        BasicBlock newLatch = null;
        for (BasicBlock bb : loopBBs) {
            BasicBlock newBB = new BasicBlock(function.getAndAddBlkIdx());
            newLoopBody.add(newBB);
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

    public void makeBBgraphForRemaining(LoopInfo loopInfo, List<BasicBlock> loopBBs) {
        BasicBlock insertPoint = latchNow;
        Function function = loopInfo.getFunction();
        BasicBlock newLatch = null;
        for (BasicBlock bb : loopBBs) {
            BasicBlock newBB = new BasicBlock(function.getAndAddBlkIdx());

            newLoopBody.add(newBB);

            newBB.insertAfter(insertPoint);
            insertPoint = newBB;
            valueProjection.put(bb, newBB);
            if (loopInfo.getLatchBlocks().contains(bb)) {
                newLatch = newBB;
            }
        }
        latchLast = latchNow;
        latchNow = newLatch;
    }

    public void makePhiForRemaining(Function targetFunc, LoopInfo loop) {
        for (Map.Entry<EmptyInstr, PhiInstr> entry : needPhi.entrySet()) {
            EmptyInstr empty = entry.getKey();
            PhiInstr phi = entry.getValue();

            if (phi.getParentBB() == loop.getLoopHeader()) {
                Value fromMainLoop = phi.getOperandMap().get(latchLast);
                Value fromNewLatch = null;
                for (Map.Entry<Value, Value> entryInner : latchValueMap.entrySet()) {
                    if ((fromMainLoop == null && entryInner.getValue() == null) ||
                            (fromMainLoop != null && fromMainLoop.equals(entryInner.getValue()))) {
                        fromNewLatch = newValueGetter(entryInner.getKey());
                        break;
                    }
                }

                if (fromNewLatch == null || fromMainLoop == null) {
                    throw new RuntimeException("fromNewLatch/fromMainLoop is null");
                }

                PhiInstr phiForNewLoop = new PhiInstr(fromNewLatch.getType(), targetFunc.getAndAddRegIdx(),
                        empty.getParentBB());
                phiForNewLoop.addOperand(loop.getLoopHeader(), phi);
                phiForNewLoop.addOperand((BasicBlock) valueProjection.get(loop.getLatchBlocks().iterator().next()), fromNewLatch);
                phiForNewLoop.insertAfter(empty);
                empty.replaceUseWith(phiForNewLoop);
                empty.removeFromList();
                valueProjection.put(phi, phiForNewLoop);
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

    public void postProcessForNonEntire(LoopInfo loopInfo) {
        latchNow.getLastInstr().forceRemoveFromList();
        new JumpInstr(loopInfo.getLoopHeader(), latchNow);

        for (Instruction instruction : loopInfo.getLoopHeader().getInstrList()) {
            if (instruction instanceof PhiInstr phi) {
                BasicBlock originalLatch = loopInfo.getLatchBlocks().iterator().next();
                Value fromLatch = latchValueMap.get(phi.getOperandMap().get(originalLatch));
                phi.delOpPair(originalLatch);
                phi.addOperand((BasicBlock) valueProjection.get(originalLatch), fromLatch);
            } else {
                break;
            }
        }
    }

    public void postProcessForRemaining(LoopInfo loopInfo, int unrollFactor) {
        Value newIcmp = valueProjection.get(loopInfo.loopCondition.icmp);
        BasicBlock bbInMainLoop = ((JumpInstr)loopInfo.getLoopHeader().getLastInstr()).getTarget();
        BasicBlock newHeader = (BasicBlock) valueProjection.get(loopInfo.getLoopHeader());
        loopInfo.getLoopHeader().getLastInstr().forceRemoveFromList();

        new BranchInstr(loopInfo.loopCondition.icmp, bbInMainLoop, newHeader, loopInfo.getLoopHeader());

        BasicBlock bbInRemaining = ((JumpInstr)newHeader.getLastInstr()).getTarget();
        newHeader.getLastInstr().forceRemoveFromList();
        new BranchInstr(newIcmp, bbInRemaining, exitTarget, newHeader);

        latchNow.getLastInstr().forceRemoveFromList();
        new JumpInstr(newHeader, latchNow);
        processExitTarget(loopInfo, newHeader);

        if (loopInfo.loopCondition.cond == CmpInstr.CmpCond.IGT || loopInfo.loopCondition.cond == CmpInstr.CmpCond.ILT
        || loopInfo.loopCondition.cond == CmpInstr.CmpCond.IGE || loopInfo.loopCondition.cond == CmpInstr.CmpCond.ILE) {
            IntConst unrollFactorConst = new IntConst(unrollFactor);
            SRemInstr remainingCount = new SRemInstr(loopInfo.getFunction().getAndAddRegIdx(),
                    loopInfo.loopCondition.tripCountExpr.getValueProjected(), unrollFactorConst, loopInfo.getPreHeader());
            remainingCount.setUse(loopInfo.loopCondition.tripCountExpr.getValueProjected());
            remainingCount.setUse(unrollFactorConst);

            Value originalBound = loopInfo.loopCondition.bound;
            remainingCount.insertBefore(loopInfo.getPreHeader().getLastInstr());
            Instruction newBound;
            if (loopInfo.loopCondition.cond == CmpInstr.CmpCond.IGT || loopInfo.loopCondition.cond == CmpInstr.CmpCond.IGE) {
                newBound = new AddInstr(loopInfo.getFunction().getAndAddRegIdx(),
                        originalBound, remainingCount, loopInfo.getPreHeader());
            } else {
                newBound = new SubInstr(loopInfo.getFunction().getAndAddRegIdx(),
                        originalBound, remainingCount, loopInfo.getPreHeader());
            }
            newBound.setUse(loopInfo.loopCondition.bound);
            newBound.setUse(remainingCount);
            newBound.insertBefore(loopInfo.getPreHeader().getLastInstr());

            loopInfo.loopCondition.icmp.modifyUse(originalBound, newBound);
            originalBound.getUserList().remove(loopInfo.loopCondition.icmp);
            newBound.getUserList().add(loopInfo.loopCondition.icmp);
        } else {
            throw new RuntimeException("unexpected cond");
        }
    }


    public void unrollLoopNonEntire(LoopInfo loopInfo, int unrollFactor) {
        valueProjection.clear();
        needPhi.clear();
        newLoopBody.clear();
        exitTarget = loopInfo.getExitTargets().iterator().next();
        latchNow = loopInfo.getLatchBlocks().iterator().next();

        ArrayList<BasicBlock> sortedBBs = preProcess(loopInfo);

        for (int i = 0; i < unrollFactor - 1; i++) {
            unrollOnce(loopInfo, sortedBBs);
        }
        postProcessForNonEntire(loopInfo);

        valueProjection.clear();
        needPhi.clear();
        makeBBgraphForRemaining(loopInfo, sortedBBs);

        BasicBlock tailLoopHeader = (BasicBlock) valueProjection.get(loopInfo.getLoopHeader());
        loopInfo.getLoopHeader().sisterLoopHeader = tailLoopHeader;
        loopInfo.getLoopHeader().isElder = true;
        tailLoopHeader.sisterLoopHeader = loopInfo.getLoopHeader();


        cloneBlockDfs(loopInfo.getLoopHeader(),
                (BasicBlock) valueProjection.get(loopInfo.getLoopHeader()), loopInfo.getFunction(), loopInfo);
        makePhiForRemaining(loopInfo.getFunction(), loopInfo);
        postProcessForRemaining(loopInfo, unrollFactor);

        for (BasicBlock basicBlock : newLoopBody) {
            basicBlock.setLoopBelonged(loopInfo);
            loopInfo.addBodyBlock(basicBlock);
        }
    }
}
