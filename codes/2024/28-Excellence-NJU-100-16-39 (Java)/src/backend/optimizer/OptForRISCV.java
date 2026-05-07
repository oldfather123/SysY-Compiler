package backend.optimizer;

import backend.RISCVCode.RISCVFunction;

public interface OptForRISCV {
    void Optimize(RISCVFunction riscvFunction);
}
