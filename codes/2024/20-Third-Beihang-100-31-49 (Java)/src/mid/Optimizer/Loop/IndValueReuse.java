package mid.Optimizer.Loop;

import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.HashMap;
import java.util.HashSet;

public class IndValueReuse {
    /*
        一个循环内部的若干归纳变量可以相互替换
        由于SSA，只要其scev一致，则在整个循环内部其所有被使用的地方一定值相等
        更进一步，也可以对scev中evovle相同的归纳变量进行替换
     */

    public void optimize() {
        Optimizer.instance().getLoopAnalyze().getLoops().forEach(this::optimizeFor);
    }

    private void optimizeFor(Loop loop) {
        HashMap<Value, ScEvValue> bivs = loop.getInds();
        HashMap<Value, Value> indValueWithPhi = loop.getIndWithPhi();

        HashMap<Value, Value> evovleToInd = new HashMap<>();
        for (Value v : new HashSet<>(bivs.keySet())) {
            if (!(v instanceof Phi phi)) {
                continue;
            }

            Value evovle = bivs.get(v).getStepVal();
            if (!evovleToInd.containsKey(evovle)) {
                evovleToInd.put(evovle, v);
            } else if (!indValueWithPhi.containsKey(v) ||
                    !indValueWithPhi.get(v).equals(evovleToInd.get(evovle))) {
                Value baseInd = evovleToInd.get(evovle);
                ScEvValue vScev = bivs.get(v);
                ScEvValue baseScev = bivs.get(baseInd);

                ALU initGap = new ALU(vScev.getInitVal(), "-", baseScev.getInitVal(),
                        !evovle.isFloat());
                loop.getPreheader().addInstructionBeforeBranch(initGap);

                ALU newValue = new ALU(baseInd, "+", initGap, !evovle.isFloat());
                phi.getBlock().addInstrAtEntry(newValue);
                v.beReplacedBy(newValue);
                phi.getBlock().removeInstruction(phi);
                loop.removeInd(phi);
                phi.destroy();
            }
        }
    }
}
