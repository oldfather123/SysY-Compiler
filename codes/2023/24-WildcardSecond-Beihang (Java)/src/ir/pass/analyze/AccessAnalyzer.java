package ir.pass.analyze;

import ir.value.BasicBlock;
import ir.value.Module;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class AccessAnalyzer {
    static boolean changed = true;

    private static void setChanged() {
        AccessAnalyzer.changed = true;
    }

    public static void run(Module module) {
        DomAnalyzer.calDomForModule(module);
        module.functions.values().forEach(
            function -> function.blocks.forEach(block -> {
                block.accessible = new LinkedHashSet<>();
                block.accessible.addAll(block.getDominate());
                block.accessible.remove(block);
                block.accessible.addAll(block.getSuccs());
            }));


        module.functions.values().forEach(func -> {
            changed = true;
            LinkedHashMap<BasicBlock, LinkedHashSet<BasicBlock>> added = new LinkedHashMap<>();
            func.blocks.forEach(block ->added.put(block, new LinkedHashSet<>(
                Collections.singletonList(block))));

            while (changed) {
                changed = false;
                func.blocks.forEach(block ->
                {
                    LinkedHashSet<BasicBlock> tmp = new LinkedHashSet<>(block.accessible);
                    tmp.forEach(accblcok ->{
                            if(!added.get(block).contains(accblcok)) {
                                block.accessible.addAll(accblcok.accessible);
                                added.get(block).add(accblcok);
                            }
                        });
                    if (!block.accessible.equals(tmp)) {
                        setChanged();
                    }
                });
            }
        });
    }
}
