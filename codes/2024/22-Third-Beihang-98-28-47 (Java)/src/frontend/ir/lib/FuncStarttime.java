package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FuncStarttime extends LibFunc{
    public FuncStarttime() {
    }
    
    @Override
    public String getName() {
        return "_sysy_starttime";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @_sysy_starttime(i32)";
    }
    
    @Override
    protected DataType getType() {
        return DataType.VOID;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.size() == 1 && rParams.get(0).getDataType() == DataType.INT;
    }
}
