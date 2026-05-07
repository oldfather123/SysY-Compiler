package backend.risc_v.entity;

public class RiscVExtern {
    private final String name;

    public RiscVExtern(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return ".extern " + name;
    }
}
