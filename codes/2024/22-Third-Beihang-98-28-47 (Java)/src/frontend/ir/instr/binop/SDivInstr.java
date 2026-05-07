package frontend.ir.instr.binop;

import frontend.ir.constvalue.ConstInt;
import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

public class SDivInstr extends BinaryOperation {
    public boolean is64 = false;
    public SDivInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "sdiv", DataType.INT);
        assert op1.getDataType() == DataType.INT;
        assert op2.getDataType() == DataType.INT;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new SDivInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            return new ConstInt(((ConstInt) op1).getNumber() / ((ConstInt) op2).getNumber());
        } else if (op1 instanceof ConstInt && op1.getNumber().intValue() == 0) {
            return op1;
        } else if (op2 instanceof ConstInt && op2.getNumber().intValue() == 1) {
            return op1;
        }
        return null;
    }
    
    public ArrayList<Instruction> strengthReduction(Function function) {
        ArrayList<Instruction> res = new ArrayList<>();
        
        if (op2 instanceof ConstInt) {
            int intValue = op2.getNumber().intValue();
            int absValue = Math.abs(intValue);
            
            if (absValue > 0 && (absValue & (absValue - 1)) == 0) {
                int exp = 0;
                while (absValue > 1) {
                    absValue >>= 1;
                    exp++;
                }
                AShrInstr newAshr = new AShrInstr(function.getAndAddRegIndex(), op1, new ConstInt(exp));
                res.add(newAshr);
                
                if (intValue < 0) {
                    ConstInt zero = ConstInt.Zero;
                    SubInstr sub = new SubInstr(function.getAndAddRegIndex(), zero, newAshr);
                    res.add(sub);
                }
            }
        }
        
        return res;
    }
}
