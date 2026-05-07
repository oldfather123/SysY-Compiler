package backend.risc_v.entity.data;


import backend.risc_v.entity.type.RiscVBasicType;

public abstract class RiscVData {
    protected final String name;
    protected final RiscVBasicType type;

    public RiscVData(String name, RiscVBasicType type) {
        this.name = name;
        this.type = type;
    }

    @Override
    public String toString() {
        return name + ": " + type + " ";
    }
}
