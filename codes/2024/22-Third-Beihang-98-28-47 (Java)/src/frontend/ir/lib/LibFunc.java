package frontend.ir.lib;

import frontend.ir.DataType;
import frontend.ir.FuncDef;
import frontend.ir.Value;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.syntax.Ast;

import java.util.ArrayList;
import java.util.List;

public abstract class LibFunc implements FuncDef {
    protected final ArrayList<Ast.FuncFParam> fParams = new ArrayList<>();
    
    public abstract String printDeclaration();
    
    protected abstract DataType getType();
    
    protected abstract boolean checkParams(List<Value> rParams);
    
    public List<Ast.FuncFParam> getFParams() {
        return fParams;
    }
    
    public CallInstr makeCall(int result, List<Value> rParams) {
        if (!checkParams(rParams)) {
            throw new RuntimeException(this.getName() + "形参实参不匹配");
        }
        DataType type = getType();
        if (type == DataType.VOID) {
            return new CallInstr(null, type, this, rParams);
        } else {
            return new CallInstr(result, type, this, rParams);
        }
    }
}
