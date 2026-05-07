package midend;

import midend.LLVMType.Type;

public class ConstInt extends Constant {
    private boolean isDafult = false;
    private int value;
    public ConstInt(Type type, int value) {
        super(type, String.valueOf(value));
        this.value = value;
    }

    public int getValue() {
        return this.value;
    }

    public void setDafult() {
        this.isDafult = true;
    }

    public boolean getIsDafult() {
        return this.isDafult;
    }
}
