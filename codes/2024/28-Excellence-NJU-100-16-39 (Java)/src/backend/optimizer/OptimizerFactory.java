package backend.optimizer;

import backend.RISCVCode.RISCVFunction;

import java.util.ArrayList;
import java.util.List;

public class OptimizerFactory {
    private final List<OptForRISCV> optForRISCV;

    public OptimizerFactory() {
        this.optForRISCV = new ArrayList<>();
        // 添加所有优化器
        optForRISCV.add(new RemoveUnreachableBlock());
        optForRISCV.add(new LSOpt());
        optForRISCV.add(new Peephole());
    }

    public void optimize(RISCVFunction riscvFunction) {
        for (OptForRISCV optForRISCV : optForRISCV) {
            optForRISCV.Optimize(riscvFunction);
        }
    }
}
