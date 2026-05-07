package Backend.process.RegAllocator;

import Backend.component.AsmFunction;
import Backend.component.AsmModule;

import java.util.HashMap;

public class Monitor {
    private final AsmModule asmModule;
    private final Allocator allocator;
    private final AfterCare afterCare;
    private final HashMap<AsmFunction, Integer> funcStack = new HashMap<>();

    public Monitor(AsmModule asmModule) {
        this.asmModule = asmModule;
        this.allocator = new Allocator(asmModule);
        this.afterCare = new AfterCare(asmModule, funcStack);
    }

    public void run() {
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            getNeedChangeDes(objFunction);
        }
        allocator.run();
        afterCare.run();
    }

    private void getNeedChangeDes(AsmFunction objFunction) {
        int stackSize = objFunction.getStackSize();
        funcStack.put(objFunction, stackSize);
    }
}
