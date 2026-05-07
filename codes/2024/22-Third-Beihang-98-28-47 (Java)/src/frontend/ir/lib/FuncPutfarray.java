package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.List;

public class FuncPutfarray extends LibFunc {
    public FuncPutfarray() {
        fParams.add(Ast.FuncFParam.INT_PARAM);
        fParams.add(Ast.FuncFParam.FLOAT_PARAM);
    }
    
    @Override
    public String getName() {
        return "putfarray";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @putfarray(i32, float*)";
    }
    
    @Override
    protected DataType getType() {
        return DataType.VOID;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        // 运行时库说明中强调该函数不检查参数合法性
        return true;
    }
}
