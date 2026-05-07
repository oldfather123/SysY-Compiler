package frontend.ir.constvalue;

import frontend.ir.DataType;

import java.util.Objects;

public class ConstBool extends ConstValue {
    private final boolean value;
    
    public ConstBool(int init) {
        value = init != 0;
    }
    
    public ConstBool(boolean init) {
        value = init;
    }
    
    @Override
    public Integer getNumber() {
        return value ? 1 : 0;
    }
    
    @Override
    public DataType getDataType() {
        return DataType.BOOL;
    }
    
    @Override
    public String value2string() {
        return Integer.toString(value ? 1 : 0);
    }
    
    @Override
    public boolean equals(Object other) {
        if (!(other instanceof ConstBool)) {
            return false;
        }
        
        return this.value == ((ConstBool) other).value;
    }
    
    @Override
    public int hashCode() {
        return Objects.hash(value);
    }
}
