package ir.pass.analyze;


import java.util.LinkedHashMap;

import ir.Value;
import ir.value.*;
import ir.value.user.Function;
import ir.value.Module;
import ir.instr.*;

public class SideEffectAnalyzer
{
    /**
     * 这个表示需要被处理的 func
     */
    private static final LinkedHashMap<Function, Boolean> processed = new LinkedHashMap<>();
    private static final LinkedHashMap<Function, Boolean> visited = new LinkedHashMap<>();

    public static void calSideEffect(Module module,boolean opt)
    {
        processed.clear();
        visited.clear();
        initCallGraph(module,opt);
        Function main = module.getFunction("main");
        process(main);
        main.hasSideEffect=true;
        for(var function:module.functions.values()){
            if(function.blocks.size()>0){
                for(var callee:function.getCallees()){
                    callee.getCallers().add(function);

                }
            }
        }
    }

    private static void initCallGraph(Module module,boolean opt)

    {
        for (Function function : module.functions.values())
        {
            function.getCallees().clear();
            function.getCallers().clear();
            function.hasSideEffect = false;
            visited.put(function, false);
            if (function.blocks.size()==0) {
                function.hasSideEffect = true;
                processed.put(function, true);
                continue;
            }
            processed.put(function, false);

            for (BasicBlock block : function.blocks)
            {
                for (Value instr : block.getInsts())
                {
                    if (instr instanceof Call)
                    {
                        Function calledFunction = ((Call) instr).getFunction();
                        // 如果不是运行时库函数，加入callee
                        if (calledFunction.blocks.size()!=0)
                        {
                            function.getCallees().add(calledFunction);
                        }
                        // 认为调用运行时库函数是有影响的
                        else
                        {
                            function.hasSideEffect = true;
                            processed.put(function, true);
                        }
                    }

                    // 如果是 store 那么就是有影响的
                    if (instr instanceof Store && (((Store) instr).checkGlobalStore()||((Store) instr).checkArgPointerStore()))
                    {
                            function.hasSideEffect = true;
                            processed.put(function, true);
                    }
                }
            }
        }
    }

    private static boolean process(Function function)
    {
        boolean hasSideEffect = false;
        visited.put(function, true);
        if (processed.get(function))
        {
            hasSideEffect = function.hasSideEffect;
            for (Function callee : function.getCallees())
            {
                if (!processed.get(callee) && !visited.get(callee))
                {
                    process(callee);
                }
            }
        }
        else
        {
            for (Function callee : function.getCallees())
            {
                if (callee.hasSideEffect ||
                        (!processed.get(callee) && !visited.get(callee) && process(callee)))
                {
                    hasSideEffect = true;
                }
            }
        }

        function.hasSideEffect=hasSideEffect;
        processed.put(function, true);
        return hasSideEffect;
    }
}
