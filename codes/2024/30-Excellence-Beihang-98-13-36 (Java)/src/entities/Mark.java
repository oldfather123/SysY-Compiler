package entities;

public class Mark extends Assembler {

    public final Type type;
    private final int value;

    public Mark(Type type, int value) {
        this.type = type;
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public enum Type {
        LOAD, SAVE;

        @Override
        public String toString() {
            return this.name();
        }
    }

    @Override
    public String toString() {
        return "Mark{" + type + '}';
    }
}
