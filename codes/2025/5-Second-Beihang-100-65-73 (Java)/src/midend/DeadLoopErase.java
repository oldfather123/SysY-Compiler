package midend;

import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;

import java.util.HashSet;
import java.util.List;

public class DeadLoopErase {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            eraseDeadLoopForFunc(function);
        }
    }

    public static void eraseDeadLoopForFunc(Function function) {
        for (LoopInfo loopInfo : new HashSet<>(function.getAncientLoopInfo())) {
            eraseDeadLoopForLoop(loopInfo);
        }
        //todo? concurrent
    }

    public static void eraseDeadLoopForLoop(LoopInfo loopInfo) {
        for (LoopInfo childLoop : new HashSet<>(loopInfo.getChildLoop())) {
            eraseDeadLoopForLoop(childLoop);
        }
        if (loopInfo.loopContents != null && loopInfo.loopContents.isEmpty()) {
            loopInfo.deleteCurrentLoop();
        }
    }
}
