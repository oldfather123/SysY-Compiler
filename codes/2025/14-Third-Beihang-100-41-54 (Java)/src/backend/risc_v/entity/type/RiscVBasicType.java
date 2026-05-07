package backend.risc_v.entity.type;

public enum RiscVBasicType {
    INT(".word"),
    FLOAT(".float"),
    STR(".asciz");

    private final String typeName;

    RiscVBasicType(String typeName) {
        this.typeName = typeName;
    }

    @Override
    public String toString() {
        return typeName;
    }
}
