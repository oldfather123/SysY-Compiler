package frontend.ir.cloner;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;

import java.util.HashMap;

public class LoopCloner extends Cloner {
    private static LoopCloner instance = new LoopCloner();
    protected LoopCloner() {}

    public static LoopCloner getInstance() {
        if (instance == null) {
            instance = new LoopCloner();
        }
        return instance;
    }

    /**
     * 填写构建好的bbGraph，只有后继在循环内时才进行对后代进行填写
     * @param srcBB 源基本块
     * @param targetBB bbGraph内基本块
     * @param targetFunc 目标函数
     * @param loop 循环信息
     */
    protected void cloneBlockDfs(BasicBlock srcBB, BasicBlock targetBB, Function targetFunc, LoopInfo loop) {
        if (!targetBB.isEmpty()) {
            return;
        }
        for (Instruction instr : srcBB.getInstrList()) {
            cloneInstr(instr, targetBB, targetFunc);
        }
        for (BasicBlock sucBB : srcBB.getSucs()) {
            if (sucBB.isContainsInLoop(loop)) {
                cloneBlockDfs(sucBB, (BasicBlock) valueProjection.get(sucBB), targetFunc, loop);
            }
        }
    }

    /**
     * 克隆循环到函数中
     * 注意，循环外的值在newValueGetter时自动取原值，如需修改这一特性请修改newValueGetter
     * 因为子类要用原版
     * @param loopInfo 循环信息
     */
    public void cloneLoopIntoFunc(LoopInfo loopInfo, Function targetFunc) {
        valueProjection.clear();
        needPhi.clear();

        BasicBlock preHeader = loopInfo.getPreHeader();
        BasicBlock newPreHeader = new BasicBlock(targetFunc.getAndAddBlkIdx());
        targetFunc.appendBasicBlock(newPreHeader);
        valueProjection.put(preHeader, newPreHeader);

        BasicBlock insertPoint = newPreHeader;
        for (BasicBlock srcBB : loopInfo.getAllBlocksSorted()) {
            BasicBlock newBB = new BasicBlock(targetFunc.getAndAddBlkIdx());
            newBB.insertAfter(insertPoint);
            insertPoint = newBB;
            valueProjection.put(srcBB, newBB);
        }

        cloneBlockDfs(preHeader, newPreHeader, targetFunc, loopInfo);
        makePhi(targetFunc);

        HashMap<Value, Value> cloneInfo = new HashMap<>(valueProjection);
        targetFunc.setCloneInfo(cloneInfo);
    }
}
