package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FuncGetch extends LibFunc{
    public FuncGetch() {
    }
    
    @Override
    public String printDeclaration() {
        return "declare i32 @getch()";
    }
    
    @Override
    public DataType getType() {
        return DataType.INT;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.isEmpty();
    }
    
    @Override
    public String getName() {
        return "getch";
    }
}
