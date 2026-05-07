package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.List;

public class FuncPutint extends LibFunc {
    public FuncPutint() {
        fParams.add(Ast.FuncFParam.INT_PARAM);
    }
    
    @Override
    public String getName() {
        return "putint";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @putint(i32)";
    }
    
    @Override
    public DataType getType() {
        return DataType.VOID;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.size() == 1 && rParams.get(0).getDataType() == DataType.INT;
    }
}
