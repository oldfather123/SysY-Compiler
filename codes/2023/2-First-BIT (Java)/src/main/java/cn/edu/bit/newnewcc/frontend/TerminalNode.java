package cn.edu.bit.newnewcc.frontend;

public class TerminalNode extends Node {
    private Object value;

    public TerminalNode(Object value) {
        this.value = value;
    }

    public Object getValue() {
        return value;
    }

    public void setValue(Object value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return "TerminalNode(" + value + ")";
    }
}
