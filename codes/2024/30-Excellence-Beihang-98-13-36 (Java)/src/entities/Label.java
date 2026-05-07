package entities;

public class Label extends Assembler {

    private final String label;

    public Label(String label) {
        this.label = label;
    }

    @Override
    public String toString() {
        return label + ":";
    }

}
