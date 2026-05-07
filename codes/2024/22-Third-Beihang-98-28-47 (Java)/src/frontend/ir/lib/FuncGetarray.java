package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.ArrayList;
import java.util.List;

public class FuncGetarray extends LibFunc {
    public FuncGetarray() {
        fParams.add(Ast.FuncFParam.INT_PARAM);
    }
    
    @Override
    public String getName() {
        return "getarray";
    }
    
    @Override
    public String printDeclaration() {
        return "declare i32 @getarray(i32*)";
    }
    
    @Override
    protected DataType getType() {
        return DataType.INT;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        return  rParams != null &&
                rParams.size() == 1 &&
                rParams.get(0).getDataType() == DataType.INT;
    }
}
