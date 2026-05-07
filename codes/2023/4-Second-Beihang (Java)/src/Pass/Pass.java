package Pass;

import Backend.component.ObjModule;
import IR.IRModule;

public interface Pass {
    String getName();

    interface IRPass extends Pass{
        void run(IRModule module);
    }

    interface ObjPass extends Pass{
        void run(ObjModule objModule);
    }
}
