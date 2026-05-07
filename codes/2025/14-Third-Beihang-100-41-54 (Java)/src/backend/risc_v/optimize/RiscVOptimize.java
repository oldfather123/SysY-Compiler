package backend.risc_v.optimize;

import backend.ir.entity.Program;
import backend.risc_v.optimize.tool.DeletePhi;

/**
 * risc_v优化器
 */
public class RiscVOptimize {
    private final Program program;
    private boolean optimize = false;

    public RiscVOptimize(Program program){
        this.program = program;
    }

    public boolean isOptimize() {
        return optimize;
    }

    public void setOptimize(boolean optimize) {
        this.optimize = optimize;
    }

    public void runOptimize(){
        //消除phi指令
        DeletePhi.deletePhi(program);
    }

}
