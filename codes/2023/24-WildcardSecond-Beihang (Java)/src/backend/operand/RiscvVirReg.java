package backend.operand;

import backend.component.RiscvFunction;
import tools.IrRegDispatcher;

public class RiscvVirReg extends RiscvReg {

    public final String name;
    public final RegType regType;

    public enum RegType {
        floatType,
        intType
    }

    public RiscvVirReg(IrRegDispatcher irRegDispatcher, RegType regType, RiscvFunction function) {
        super();
        if(regType == RegType.floatType) {
            name = "%float" + irRegDispatcher.allocId();
        } else {
            name = "%int" + irRegDispatcher.allocId();
        }
        function.addVirReg(this);
        this.regType = regType;
    }

    @Override
    public String toString() {
        return name;
    }
}
