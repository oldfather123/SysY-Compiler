package backend.ir.optimize;

import backend.ir.entity.Program;
import backend.ir.optimize.tool.MemToReg;

public class IROptimize {

    private final Program program;
    private boolean optimize = false;

    public IROptimize(Program program){
        this.program = program;
    }

    public boolean isOptimize() {
        return optimize;
    }

    public void setOptimize(boolean optimize) {
        this.optimize = optimize;
    }

    public void optimizeInit(){
        //首先删除完全不会到达的代码
        program.DCEBlock();
        //建立控制流图
        program.buildCfgGraph();
        //生成支配树
        program.generateDominantTree();
    }

    public void runOptimize(){
        if(!optimize){return;}
        optimizeInit();
        MemToReg.Mem2Reg(program);
        //活跃变量分析
        program.activeVarAnalysis();
        //program.GVNOptimize();
    }
}
