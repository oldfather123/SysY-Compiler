package main;

import entities.*;
import handler.*;
import file.Output;
import file.ReadSys;
import opt.AssOptimize;
import opt.QuaOptimize;
import util.Strings;
import util.Util;

import java.util.ArrayList;
import java.util.List;

public class Main {

    public static void doCompile(String path, String outPath) {
        List<Word> words = ReadSys.read(path);
        TreeNode root = SyntaxHandler.buildSyntaxTree(words);
        QuaHandler.QuaResult quaResult = QuaHandler.handle(root);
        if (OptHandler.shouldOpt) QuaOptimize.opt(quaResult);
        AssemblerHandler.AssemblerResult assemblers = AssemblerHandler.handle(quaResult);
        if (OptHandler.shouldOpt) {
            RegisterHandler.handle(assemblers.assemblers(), false);
            assemblers = AssemblerHandler.handle(quaResult);
        }
        RegisterHandler.handle(assemblers.assemblers(), true);
        if (OptHandler.shouldOpt) {
            for (String funcName : assemblers.assemblers().keySet()) {
                AssOptimize.opt(assemblers.assemblers().get(funcName));
            }
        }
        List<DoubleList<Assembler>> result = new ArrayList<>();
        result.add(assemblers.assemblers().get(Strings._PRE));
        if (Util.hasGetIntArray) result.add(Util.getIntArray);
        if (Util.hasGetFloatArray) result.add(Util.getFloatArray);
        if (Util.hasPutIntArray) result.add(Util.putIntArray);
        if (Util.hasPutFloatArray) result.add(Util.putFloatArray);
        DoubleList<Pair<String, Integer>>.Iterator iter = quaResult.funcOrder().iterator();
        while (iter.hasNext()) {
            result.add(assemblers.assemblers().get(iter.next().first));
        }
        result.add(assemblers.global());
        Output.output(outPath, result);
    }

}
