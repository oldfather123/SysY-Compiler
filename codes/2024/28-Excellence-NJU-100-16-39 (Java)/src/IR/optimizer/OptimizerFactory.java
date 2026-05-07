package IR.optimizer;

import IR.IRValueRef.IRFunctionBlockRef;

import java.util.ArrayList;
import java.util.List;

public class OptimizerFactory {
    private final List<OptForIR> optForIR;

    public OptimizerFactory() {
        this.optForIR = new ArrayList<>();
//         添加所有优化器
        optForIR.add(new Mem2Reg());
        optForIR.add(new ConstantPro());
        optForIR.add(new BrCmpOpt());
        optForIR.add(new TailRecursionEli());
        optForIR.add(new CommonSubEli());
    }

    public void optimize(IRFunctionBlockRef irFunctionBlockRef) {
        for (OptForIR optForRISCV : optForIR) {
            optForRISCV.Optimize(irFunctionBlockRef);
        }
    }
}


