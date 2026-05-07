package backend.risc_v.entity.data;

import backend.risc_v.entity.type.RiscVBasicType;

public class RiscVStrData extends RiscVData {
    private final String value;

    public RiscVStrData(String name, String value) {
        super(name, RiscVBasicType.STR);
        this.value = value;
    }

    @Override
    public String toString() {
        return super.toString() + value;
    }
}
