package frontend.ir.constvalue;

import frontend.ir.DataType;

import java.util.Objects;

public class ConstFloat extends ConstValue {
    private final float value;
    public static final ConstFloat Zero = new ConstFloat(0.0f);
    public ConstFloat(float init) {
        value = init;
    }
    
    @Override
    public Float getNumber() {
        return value;
    }
    
    @Override
    public DataType getDataType() {
        return DataType.FLOAT;
    }
    
    @Override
    public String value2string() {
        return "0x" + Long.toHexString(Double.doubleToLongBits(value));
    }
    
    @Override
    public boolean equals(Object other) {
        if (!(other instanceof ConstFloat)) {
            return false;
        }
        
        return this.value == ((ConstFloat) other).value;
    }
    
    @Override
    public int hashCode() {
        return Objects.hash(value);
    }
}
