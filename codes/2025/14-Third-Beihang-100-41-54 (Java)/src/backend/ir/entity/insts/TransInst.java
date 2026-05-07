package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;
import frontend.semantic.symbol.VarInfo;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

/**
 * &#064;Classname TransInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public class TransInst extends Inst{
    private final VarInfo toVarInfo;
    public TransInst(String name, IRBasicType toType, List<Integer> toDimList, BasicBlock basicBlock) {
        super(name, toType, false, toDimList != null, toDimList, basicBlock);
        this.toVarInfo = new VarInfo(toType, false, toDimList != null, toDimList);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.isEmpty()) return;
        Value operand = usees.get(0);
        if(operand.getVarInfo().equals(toVarInfo)){
            return;
        }
        String fromStr = operand.getTypeStr();
        String toStr = toVarInfo.getTypeStr();
        
        String castOp = getCastOperator();
        fout.write(name + " = " + castOp + " " + fromStr + " ");
        operand.printName(fout);
        fout.write(" to " + toStr);
    }
    
    private String getCastOperator() {
        IRBasicType fromType = usees.get(0).getBasicType();
        IRBasicType toType = toVarInfo.getBasicType();
        // 根据源类型和目标类型确定转换操作符
        if (fromType == IRBasicType.I32 && toType == IRBasicType.FLOAT) {
            return "sitofp"; // 有符号整数转浮点数
        } else if (fromType == IRBasicType.FLOAT && toType == IRBasicType.I32) {
            return "fptosi"; // 浮点数转有符号整数
        } else if (fromType == IRBasicType.I32 && toType == IRBasicType.I1) {
            return "trunc"; // 整数截断
        } else if (fromType == IRBasicType.I1 && toType == IRBasicType.I32) {
            return "zext"; // 零扩展
        } else if (fromType == IRBasicType.I32 && toType == IRBasicType.I8) {
            return "bitcast"; // 相同类型，使用bitcast
        }  else if(fromType == IRBasicType.I1 && toType == IRBasicType.FLOAT) {
            return "uitofp";
        }
        return "type trans error";
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write(name);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        if (!usees.isEmpty()) {
            try {
                usees.get(0).printName(fout);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
