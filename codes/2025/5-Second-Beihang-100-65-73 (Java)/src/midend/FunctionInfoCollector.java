package midend;

import frontend.ir.structure.Function;

import java.util.HashMap;
import java.util.Map;

public class FunctionInfoCollector {
    private static final Map<Function, FunctionInfo> funcInfoMap = new HashMap<>();

    public static FunctionInfo getFuncInfo(Function function) {
        if(!funcInfoMap.containsKey(function)) {
            funcInfoMap.put(function, new FunctionInfo(function));
        }
        return funcInfoMap.getOrDefault(function, null);
    }

    public static void printFunctionInfo() {
        for(FunctionInfo info: funcInfoMap.values()) {
            System.out.println(info.toString());
        }
    }
}
