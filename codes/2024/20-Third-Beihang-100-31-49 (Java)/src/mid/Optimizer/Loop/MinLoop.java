package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.HashSet;

public class MinLoop extends Loop {
    /*
        不包含break和continue，且能求出循环次数的循环
        这表明它只有一个exiting和exit，其中所有归纳变量可以被正确地外提
        exiting一定是header或latch其一
     */
    private BasicBlock exiting;
    private final BasicBlock exit;


    public MinLoop(Loop loop) {
        super(loop.getHeader());
        setLatch(loop.getLatch());
        setPreheader(loop.getPreheader());
        exiting = (BasicBlock) loop.getExitings().toArray()[0];
        exit = (BasicBlock) loop.getExits().toArray()[0];
        inds = loop.getInds();
        indWithPhi = loop.getIndWithPhi();
    }

    public BasicBlock getExiting() {
        return exiting;
    }

    public BasicBlock getExit() {
        return exit;
    }

    public void setRepeatNumber(Value repeatNumber) {
        if (operandList.size() > 0) {
            operandList.remove(0);
        }
        use(repeatNumber);
    }

    public Value getRepeatNumber() {
        return operandList.get(0);
    }

    public void setExiting(BasicBlock exiting) {
        this.exiting = exiting;
    }


    public HashSet<BasicBlock> getExitings() {
        HashSet<BasicBlock> ret = new HashSet<>();
        ret.add(exiting);
        return ret;
    }

    public HashSet<BasicBlock> getLatchs() {
        HashSet<BasicBlock> ret = new HashSet<>();
        ret.add(getLatch());
        return ret;
    }

    public HashSet<BasicBlock> getExits() {
        HashSet<BasicBlock> ret = new HashSet<>();
        ret.add(exit);
        return ret;
    }
}
