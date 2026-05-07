package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FuncStoptime extends LibFunc {
    public FuncStoptime() {
    }
    
    @Override
    public String getName() {
        return "_sysy_stoptime";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @_sysy_stoptime(i32)";
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
