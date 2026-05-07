package frontend.ir.instr.otherop.cmp;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstBool;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class ICmpInstr extends Cmp {
    public ICmpInstr(int result, CmpCond cond, Value op1, Value op2) {
        super(result, cond, op1, op2);
        if (op1.getDataType() != DataType.INT || op2.getDataType() != DataType.INT) {
            throw new RuntimeException("整数比较必须是两个三十二位整数之间");
        }
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new ICmpInstr(procedure.getAndAddRegIndex(), cond, op1, op2);
    }
    
    @Override
    public String print() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("%reg_").append(result).append(" = icmp ");
        switch (cond) {
            case EQ: stringBuilder.append("eq"); break;
            case NE: stringBuilder.append("ne"); break;
            case GT: stringBuilder.append("sgt"); break;
            case GE: stringBuilder.append("sge"); break;
            case LT: stringBuilder.append("slt"); break;
            case LE: stringBuilder.append("sle"); break;
            default: throw new RuntimeException("还有高手？");
        }
        stringBuilder.append(" i32 ").append(op1.value2string()).append(", ").append(op2.value2string());
        return stringBuilder.toString();
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            int const1 = op1.getNumber().intValue();
            int const2 = op2.getNumber().intValue();
            switch (cond) {
                case EQ: return new ConstBool(const1 == const2);
                case NE: return new ConstBool(const1 != const2);
                case GE: return new ConstBool(const1 >= const2);
                case GT: return new ConstBool(const1 >  const2);
                case LE: return new ConstBool(const1 <= const2);
                case LT: return new ConstBool(const1 <  const2);
                default: throw new RuntimeException("意料之外的运算符");
            }
        }
        
        return null;
    }
}
