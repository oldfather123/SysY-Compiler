package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.List;

public class FuncMemset extends LibFunc {
    public FuncMemset() {
        fParams.add(null);  // todo: 这个是用来做参数隐式转换的时候用的，但是这个库函数不应该被用户调用，也就不涉及类型转换
        fParams.add(Ast.FuncFParam.INT_PARAM);
        fParams.add(Ast.FuncFParam.INT_PARAM);
    }
    
    @Override
    public String getName() {
        return "memset";
    }
    
    @Override
    public String printDeclaration() {
        return "declare void @memset(i8*, i32, i32)";
    }
    
    @Override
    protected DataType getType() {
        return DataType.VOID;
    }
    
    @Override
    protected boolean checkParams(List<Value> rParams) {
        if (rParams == null) {
            throw new NullPointerException();
        }
        if (rParams.size() != 3) {
            return false;
        }
        DataType type0 = rParams.get(0).getDataType();
        DataType type1 = rParams.get(1).getDataType();
        DataType type2 = rParams.get(2).getDataType();
        if (rParams.get(0).getPointerLevel() != 1) {
            return false;
        }
        return type0 == DataType.VOID_ && type1 == DataType.INT && type2 == DataType.INT;
    }
}
