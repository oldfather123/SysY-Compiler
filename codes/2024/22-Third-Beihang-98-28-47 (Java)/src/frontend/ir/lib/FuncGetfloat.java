package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FuncGetfloat extends LibFunc{
    public FuncGetfloat() {
    }
    
    @Override
    public String getName() {
        return "getfloat";
    }
    
    @Override
    public String printDeclaration() {
        return "declare float @getfloat()";
    }
    
    @Override
    public DataType getType() {
        return DataType.FLOAT;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.isEmpty();
    }
}
