package ir;

import ir.pass.opt.*;
import ir.pass.opt.Pass;
import ir.value.Module;
import java.util.ArrayList;

public class PassRunner{
    private final ArrayList<Pass> passes = new ArrayList<>();

    public void run(Module module) {
        passes.add(new MemToReg());
        passes.add(new DeadCodeEmit());
        //passes.add(new InlineFunction());
        //TODO: waiting for more passes 2023/7/18
        for (Pass pass : passes) {
            pass.run(module);
        }
    }
}
