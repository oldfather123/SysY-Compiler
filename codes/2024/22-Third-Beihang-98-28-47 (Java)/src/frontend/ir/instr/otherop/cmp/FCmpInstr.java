package frontend.ir.instr.otherop.cmp;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstBool;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class FCmpInstr extends Cmp {
    
    public FCmpInstr(int result, CmpCond cond, Value op1, Value op2) {
        super(result, cond, op1, op2);
        if (op1.getDataType() != DataType.FLOAT || op2.getDataType() != DataType.FLOAT) {
            throw new RuntimeException("浮点数比较必须是两个浮点数之间");
        }
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new FCmpInstr(procedure.getAndAddRegIndex(), cond, op1, op2);
    }
    
    @Override
    public String print() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("%reg_").append(result).append(" = fcmp ");
        switch (cond) {
            case EQ: stringBuilder.append("oeq"); break;
            case NE: stringBuilder.append("one"); break;
            case GT: stringBuilder.append("ogt"); break;
            case GE: stringBuilder.append("oge"); break;
            case LT: stringBuilder.append("olt"); break;
            case LE: stringBuilder.append("ole"); break;
            default: throw new RuntimeException("还有高手？");
        }
        stringBuilder.append(" float ").append(op1.value2string()).append(", ").append(op2.value2string());
        return stringBuilder.toString();
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstFloat && op2 instanceof ConstFloat) {
            float const1 = op1.getNumber().floatValue();
            float const2 = op2.getNumber().floatValue();
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
