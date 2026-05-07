package pass;

import backend.component.ObjModule;
import ir.IrModule;

public interface Pass {
    String getName();

    interface IrPass extends Pass {
        void run(IrModule module);
    }

    interface ObjPass extends Pass {
        void run(ObjModule module);
    }
}
