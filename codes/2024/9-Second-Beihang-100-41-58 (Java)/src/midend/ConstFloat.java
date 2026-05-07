package midend;

import midend.LLVMType.Type;

public class ConstFloat extends Constant {
    private  boolean isDafult = false;
    private float value;
    public ConstFloat(Type type, float value) {
        super(type, String.format("0x%x", Double.doubleToRawLongBits(value)));
        this.value = value;
    }

    public float getValue() {
        return this.value;
    }

    public void setDafult() {
        this.isDafult = true;
    }

    public boolean getIsDafult() {
        return this.isDafult;
    }
}
