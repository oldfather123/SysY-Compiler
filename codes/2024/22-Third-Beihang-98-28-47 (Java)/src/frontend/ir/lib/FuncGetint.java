package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FuncGetint extends LibFunc {
    public FuncGetint() {
    }
    
    @Override
    public String printDeclaration() {
        return "declare i32 @getint()";
    }
    
    @Override
    public DataType getType() {
        return DataType.INT;
    }
    
    @Override
    public String getName() {
        return "getint";
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.isEmpty();
    }
}
