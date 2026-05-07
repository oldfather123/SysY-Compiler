package frontend.ir.structure;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.HashMap;

public class GlobalObject extends Value {
    private final String name;
    private final DataType dataType;
    private static final HashMap<String, GlobalObject> globalObjects = new HashMap<>();
    
    public GlobalObject(String name, DataType dataType) {
        this.name = name;
        this.dataType = dataType;
        globalObjects.put(name, this);
        this.pointerLevel = 1;
    }
    
    public static GlobalObject getGlbObj(String name) {
        return globalObjects.get(name);
    }
    
    @Override
    public Number getNumber() {
        throw new RuntimeException("全局对象不好说 value 是什么");
    }
    
    @Override
    public DataType getDataType() {
        return dataType;
    }
    
    @Override
    public String value2string() {
        return "@" + name;
    }
}
