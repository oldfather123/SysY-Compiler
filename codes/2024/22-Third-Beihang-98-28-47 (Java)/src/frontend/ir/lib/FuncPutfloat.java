package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.List;

public class FuncPutfloat extends LibFunc {
    public FuncPutfloat() {
        fParams.add(Ast.FuncFParam.FLOAT_PARAM);
    }
    
    @Override
    public String getName() {
        return "putfloat";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @putfloat(float)";
    }
    
    @Override
    public DataType getType() {
        return DataType.VOID;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return rParams != null && rParams.size() == 1 && rParams.get(0).getDataType() == DataType.FLOAT;
    }
}
