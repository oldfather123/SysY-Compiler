package backend.component;

public class RiscvGlobalZero implements RiscvGlobalElement{
    private int size;
    public RiscvGlobalZero(int size) {
        this.size = size;
    }

    public int getSize() {
        return size;
    }

    public void addSize(int size) {
        this.size += size;
    }

    @Override
    public String toString() {
        return "\t.zero\t" + size + "\n";
    }
}
