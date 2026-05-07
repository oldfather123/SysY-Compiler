package midend;

import frontend.ir.structure.Function;

import java.util.List;

public class DetectTailRecursive {
    public static void excute(List<Function> functions) {
        for (Function f : functions) {
            f.setIsTailRecursive();
        }
    }
}
