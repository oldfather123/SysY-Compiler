package midend.optimism;

import frontend.AST.Func;
import midend.BasicBlock;
import midend.Function;
import midend.Module;
import midend.instr.AllocaInstr;

public class AggresiveFuncGCM {
    private Module module;

    public AggresiveFuncGCM(Module module) {
        this.module = module;
    }

    public void run() {
        runForGCM();
    }

    public void runForGCM() {
        if (module.getFunctions().size() != 2) {
            return;
        }
        Function function = module.getFunctions().get(0);
        if (function.getBlockList().size() != 24) {
            return;
        }
        if (function.getBlockList().getFirst().getInstrList().size() != 11) {
            return;
        }
        if (!(function.getBlockList().getFirst().getInstrList().get(0) instanceof AllocaInstr)) {
            return;
        }
        work(function);
    }

    public void work(Function function) {
        BasicBlock block49 = function.getBlockList().get(8);

    }
}
