package Pass.IR;

import Driver.Config;
import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.CloneHelper;
import Pass.IR.Utils.IRLoop;
import Pass.IR.Utils.LoopAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Stack;

import static Pass.IR.Utils.UtilFunc.*;

public class LoopUnroll implements Pass.IRPass {

    private final IRBuildFactory f = IRBuildFactory.getInstance();

    private BasicBlock head;
    private BasicBlock entering;
    private BasicBlock latch;
    private BasicBlock exit;
    private BasicBlock headNext;

    private boolean change;

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            LoopAnalysis.runLoopInfo(function);
            LoopAnalysis.runLoopIndvarInfo(function);
            change = false;
            ConstLoopUnroll(function);
            if(change){
                makeCFG(function);
            }
        }
    }

    //  init/step/end 均为常数
    private void ConstLoopUnroll(Function function){
        ArrayList<IRLoop> dfsOrderLoops = new ArrayList<>();
        for(IRLoop loop : function.getTopLoops()){
            dfsOrderLoops.addAll(getDFSLoops(loop));
        }

        for(IRLoop loop : dfsOrderLoops){
            ConstLoopUnrollForLoop(loop);
        }
    }

    private ArrayList<IRLoop> getDFSLoops(IRLoop loop){
        ArrayList<IRLoop> allLoops = new ArrayList<>();
        for(IRLoop subLoop : loop.getSubLoops()){
            allLoops.addAll(getDFSLoops(subLoop));
        }
        allLoops.add(loop);
        return allLoops;
    }

    private void ConstLoopUnrollForLoop(IRLoop loop){
        if (!loop.isSetIndVar()) {
            return;
        }

        Value itInit = loop.getItInit();
        Value itStep = loop.getItStep();
        Value itEnd = loop.getItEnd();
        BinaryInst itAlu = (BinaryInst) loop.getItAlu();

        if (!(itInit instanceof Const) || !(itStep instanceof Const) || !(itEnd instanceof Const)) {
            return;
        }

        CmpInst headBrCond = loop.getHeadBrCond();
        int init = ((ConstInteger) itInit).getValue();
        int step = ((ConstInteger) itStep).getValue();
        int end = ((ConstInteger) itEnd).getValue();
        if (step == 0) {
            return;
        }

        //  aluOP是i = i + 1中的add
        //  cmpOP是i < n中的<
        OP aluOP = itAlu.getOp();
        OP cmpOP = headBrCond.getOp();
        int times = getConstLoopTimes(aluOP, cmpOP, init, step, end);

        if (times < 0) {
            return;
        }
        loop.setItTimes(times);
        if(!initCheck(loop)){
            return;
        }
        //  初始化该循环的各种block
        if(!initAllBlock(loop)){
            return ;
        }

        //  能到这就肯定会被展开了
        change = true;
        DoConstLoopUnRoll(loop);
    }

    private int getConstLoopTimes(OP aluOP, OP cmpOP, int init, int step, int end){
        int times = -1;
        if(aluOP.equals(OP.Add)){
            //  while (i == n)
            if(cmpOP.equals(OP.Eq)){
                times = (init == end) ? 1 : 0;
            }
            //  while (i != n)
            else if(cmpOP.equals(OP.Ne)){
                times = ((end - init) % step == 0) ? (end - init) / step : -1;
            }
            //  while (i >=/<= n)
            else if(cmpOP.equals(OP.Ge) || cmpOP.equals(OP.Le)){
                times = (end - init) / step + 1;
            }
            //  while (i >/< n)
            else if(cmpOP.equals(OP.Gt) || cmpOP.equals(OP.Lt)){
                times = ((end - init) % step == 0)? (end - init) / step : (end - init) / step + 1;
            }
        }
        else if(aluOP.equals(OP.Sub)){
            if(cmpOP.equals(OP.Eq)){
                times = (init == end)? 1:0;
            }
            else if(cmpOP.equals(OP.Ne)){
                times = ((init - end) % step == 0)? (init - end) / step : -1;
            }
            else if(cmpOP.equals(OP.Ge) || cmpOP.equals(OP.Le)){
                times = (init - end) / step + 1;
            }
            else if(cmpOP.equals(OP.Gt) || cmpOP.equals(OP.Lt)){
                times = ((init - end) % step == 0)? (init - end) / step : (init - end) / step + 1;
            }
        }
        else if(aluOP.equals(OP.Mul)){
            double val = Math.log(end / init) / Math.log(step);
            boolean tag = init * Math.pow(step, val) == end;
            if(cmpOP.equals(OP.Eq)){
                times = (init == end)? 1:0;
            }
            else if(cmpOP.equals(OP.Ne)){
                times = tag ? (int) val : -1;
            }
            else if(cmpOP.equals(OP.Ge) || cmpOP.equals(OP.Le)){
                times = (int) val + 1;
            }
            else if(cmpOP.equals(OP.Gt) || cmpOP.equals(OP.Lt)){
                times = tag ? (int) val : (int) val + 1;
            }
        }
        return times;
    }

    private boolean initCheck(IRLoop loop){
        HashSet<BasicBlock> exits = new HashSet<>();
        HashSet<BasicBlock> allBB = new HashSet<>(loop.getBbs());

        for (IRLoop subLoop: loop.getSubLoops()) {
            checkDFS(subLoop, allBB, exits);
        }

        for (BasicBlock bb: exits) {
            if (!allBB.contains(bb)) {
                return false;
            }
        }

        int times = loop.getItTimes();
        int cnt = cntDFS(loop);
        return (long) cnt * times <= Config.loopUnrollMaxLines;
    }

    private boolean initAllBlock(IRLoop loop){
        head = loop.getHead();
        //  Latch & Entering
        for (BasicBlock bb: head.getPreBlocks()) {
            if (loop.getLatchBlocks().contains(bb)) {
                latch = bb;
            }
            else {
                entering = bb;
            }
        }

        //  Exit
        for (BasicBlock bb: loop.getExitBlocks()) {
            exit = bb;
        }
        //  HeadNext
        for (BasicBlock bb: head.getNxtBlocks()) {
            if (!bb.equals(exit)) {
                headNext = bb;
            }
        }

        assert headNext != null;
        if (headNext.getPreBlocks().size() != 1) {
            return false;
        }
        return true;
    }

    private void DoConstLoopUnRoll(IRLoop loop){
        //  开始unroll
        //  phiLatchMap: 存放从latch到达head时各个phi所取的值
        //  beginToEnd: 存放head中的value到最后映射成了什么值，用于修复exit中的LCSSA
        HashMap<Value, Value> phiMap = new HashMap<>();
        HashMap<Value, Value> beginToEnd = new HashMap<>();
        Function function = head.getParentFunc();
        ArrayList<Phi> headPhiInsts = new ArrayList<>();
        for (IList.INode<Instruction, BasicBlock> instNode : head.getInsts()) {
            Instruction inst = instNode.getValue();
            if (!(inst instanceof Phi)) {
                break;
            }
            headPhiInsts.add((Phi) inst);
        }

        //  删除掉headPhi和latchBlock的联系，新建phiInLatchNxt
        int latchIndex = head.getPreBlocks().indexOf(latch);
        for (Phi phi : headPhiInsts) {
            Value newValue = phi.getUseValues().get(latchIndex);
            phi.removeOperand(latchIndex);
            phiMap.put(phi, newValue);
            beginToEnd.put(phi, newValue);
        }
        //  删除headBlock和latchBlock/exitBlock的前驱后继关系
        head.removePreBlock(latch);
        latch.removeNxtBlock(head);
        head.removeNxtBlock(exit);
        exit.removePreBlock(head);
        loop.removeBb(head);
        head.setLoop(loop.getParentLoop());
        BrInst brInst = (BrInst) head.getLastInst();
        brInst.turnToJump(headNext);
        latch.removeLastInst();

        //  得到复制Bb的dfs序
        HashSet<BasicBlock> allBbInLoop = new HashSet<>();
        getAllBBInLoop(loop, allBbInLoop);
        allBbInLoop.remove(head);
        ArrayList<BasicBlock> dfsBbInLoop = getDFSBBInLoop(allBbInLoop, headNext);

        //  更新该loop与parentLoop的关系(如果有的话)
        IRLoop parentLoop = loop.getParentLoop();
        if(parentLoop != null) {
            parentLoop.getSubLoops().remove(loop);
            for (BasicBlock bb : loop.getBbs()) {
                bb.setLoop(parentLoop);
                if (!parentLoop.getBbs().contains(bb)) {
                    parentLoop.addBlock(bb);
                }
            }
            for (IRLoop subLoop : loop.getSubLoops()) {
                subLoop.setParentLoop(parentLoop);
            }
        }
        else{
            function.getTopLoops().remove(loop);
        }

        //  开始unroll
        BasicBlock oldHeadNext = headNext;
        BasicBlock oldLatch = latch;
        CloneHelper cloneHelper = new CloneHelper();
        for(Value key : phiMap.keySet()){
            cloneHelper.addValueMapping(key, phiMap.get(key));
        }
        for (int time = 0; time < loop.getItTimes() - 1; time++) {
            //  copy allBlockInLoop and update dfsBbInLoop
            for(BasicBlock bb : dfsBbInLoop){
                BasicBlock newBb = f.buildBasicBlock(function);
                cloneHelper.addValueMapping(bb, newBb);
            }
            for(BasicBlock bb : dfsBbInLoop){
                BasicBlock newBb = (BasicBlock) cloneHelper.findValue(bb);
                cloneHelper.copyBlockToBlock(bb, newBb);
                newBb.setLoop(bb.getLoop());
                bb.getLoop().addBlock(newBb);
            }
            //  tmpDfsBbInLoop记录clone前的所有loop中的bb
            ArrayList<BasicBlock> tmpDfsBbInLoop = new ArrayList<>(dfsBbInLoop);
            dfsBbInLoop.clear();
            for(BasicBlock bb : tmpDfsBbInLoop){
                dfsBbInLoop.add((BasicBlock) cloneHelper.findValue(bb));
            }

            //  update newHeadNext, newLatch
            BasicBlock newHeadNext = (BasicBlock) cloneHelper.findValue(oldHeadNext);
            BasicBlock newLatch = (BasicBlock) cloneHelper.findValue(oldLatch);
            //  init allBbInLoop's preNxt
            makeCFG(dfsBbInLoop);
            //  init specialBb's preNxt(oldLatch->nxtBb = newHeadNext)
            oldLatch.setNxtBlock(newHeadNext);
            newHeadNext.setPreBlock(oldLatch);
            f.buildBrInst(newHeadNext, oldLatch);

            //  update phi
            ArrayList<Phi> phis = getAllPhiInBbs(tmpDfsBbInLoop);
            for (Phi phi : phis) {
                for (int i = 0; i < phi.getOperands().size(); i++) {
                    BasicBlock preBB = phi.getParentbb().getPreBlocks().get(i);
                    BasicBlock nowBB = (BasicBlock) cloneHelper.findValue(preBB);
                    Phi copyPhi = (Phi) cloneHelper.findValue(phi);
                    int index = copyPhi.getParentbb().getPreBlocks().indexOf(nowBB);

                    Value value = phi.getOperand(i);
                    Value copyValue;
                    if(value instanceof ConstInteger){
                        int val = ((ConstInteger) value).getValue();
                        copyValue = new ConstInteger(val, IntegerType.I32);
                    }
                    else if(value instanceof ConstFloat){
                        float val = ((ConstFloat) value).getValue();
                        copyValue = new ConstFloat(val);
                    }
                    else copyValue = cloneHelper.findValue(value);
                    copyPhi.replaceOperand(index, copyValue);
                }
            }
            //  update beginToEnd
            for(Value key : beginToEnd.keySet()){
                Value value = beginToEnd.get(key);
                beginToEnd.put(key, cloneHelper.findValue(value));
            }

            //  update oldHeadNext, oldLatch
            oldHeadNext = newHeadNext;
            oldLatch = newLatch;
        }

        //  oldLatch->nextBlock := exit
        oldLatch.setNxtBlock(exit);
        exit.setPreBlock(oldLatch);
        f.buildBrInst(exit, oldLatch);

        //  修正exitBb中的LCSSA
        ArrayList<Phi> phiInExit = getPhiInBb(exit);
        for(Phi phi : phiInExit){
            for(Value value : phi.getUseValues()){
                if(value instanceof Instruction inst && inst.getParentbb().equals(head)){
                    phi.replaceOperand(value, beginToEnd.get(value));
                }
            }
        }

    }

    private ArrayList<BasicBlock> getDFSBBInLoop(HashSet<BasicBlock> allBBInLoop, BasicBlock dfsEntry){
        HashSet<BasicBlock> visitedMap = new HashSet<>();
        Stack<BasicBlock> dfsStack = new Stack<>();
        ArrayList<BasicBlock> dfsBBInLoop = new ArrayList<>();
        dfsStack.push(dfsEntry);
        visitedMap.add(dfsEntry);
        while (!dfsStack.isEmpty()) {
            BasicBlock loopBlock = dfsStack.pop();
            dfsBBInLoop.add(loopBlock);
            if(!loopBlock.getNxtBlocks().isEmpty()) {
                for (BasicBlock basicBlock : loopBlock.getNxtBlocks()) {
                    if (!visitedMap.contains(basicBlock)) {
                        visitedMap.add(basicBlock);
                        dfsStack.push(basicBlock);
                    }
                }
            }
        }
        return dfsBBInLoop;
    }

    private void getAllBBInLoop(IRLoop loop, HashSet<BasicBlock> bbs) {
        bbs.addAll(loop.getBbs());
        for (IRLoop subLoop : loop.getSubLoops()) {
            getAllBBInLoop(subLoop, bbs);
        }
    }

    //  DFS获取该loop下的所有bb和exitbb
    private void checkDFS(IRLoop loop, HashSet<BasicBlock> allBB, HashSet<BasicBlock> exits) {
        allBB.addAll(loop.getBbs());
        exits.addAll(loop.getExitBlocks());
        for (IRLoop subLoop: loop.getSubLoops()) {
            checkDFS(subLoop, allBB, exits);
        }
    }

    private int cntDFS(IRLoop loop){
        int cnt = 0;
        for (BasicBlock bb: loop.getBbs()) {
            cnt += bb.getInsts().getSize();
        }
        for (IRLoop subLoop: loop.getSubLoops()) {
            cnt += cntDFS(subLoop);
        }
        return cnt;
    }

    @Override
    public String getName() {
        return "LoopUnroll";
    }
}
