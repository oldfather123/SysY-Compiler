package frontend.ir.instr.binop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

public class AddInstr extends BinaryOperation implements Swappable {
    public AddInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "add", DataType.INT);
        assert op1.getDataType() == DataType.INT;
        assert op2.getDataType() == DataType.INT;
        trySwapOp();
    }
    
    public void trySwapOp() {
        if (op1 instanceof ConstInt && !(op2 instanceof ConstInt)) {
            Value tmp = op1;
            op1 = op2;
            op2 = tmp;
        }
    }
    
    @Override
    public void modifyValue(Value from, Value to) {
        super.modifyValue(from, to);
        trySwapOp();
    }
    
    @Override
    public Value operationSimplify() {
        Value res = null;
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            return new ConstInt(((ConstInt) op1).getNumber() + ((ConstInt) op2).getNumber());
        } else if (op1 instanceof ConstInt) {
            res = mergeConst((ConstInt) op1, op2);
        } else if (op2 instanceof ConstInt) {
            res = mergeConst((ConstInt) op2, op1);
        }
        return res;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new AddInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
    }
    
    private Value mergeConst(ConstInt constInt, Value nonConst) {
        if (constInt == null || nonConst == null) {
            throw new NullPointerException();
        }
        if (constInt.getNumber() == 0) {
            return nonConst;
        }
        if (nonConst instanceof AddInstr) {
            ArrayList<Value> userSet2list = new ArrayList<>(nonConst.getUserSet());
            if (userSet2list.size() == 1 && userSet2list.get(0) == this) {
                Value upperOp1 = ((AddInstr) nonConst).getOp1();
                Value upperOp2 = ((AddInstr) nonConst).getOp2();
                if (upperOp1 instanceof ConstInt) {
                    int res = upperOp1.getNumber().intValue() + constInt.getNumber();
                    ((AddInstr) nonConst).setOp1(new ConstInt(res));
                    return nonConst;
                }
                if (upperOp2 instanceof ConstInt) {
                    int res = upperOp2.getNumber().intValue() + constInt.getNumber();
                    ((AddInstr) nonConst).setOp2(new ConstInt(res));
                    return nonConst;
                }
            }
        } else if (nonConst instanceof SubInstr) {
            ArrayList<Value> userSet2list = new ArrayList<>(nonConst.getUserSet());
            if (userSet2list.size() == 1 && userSet2list.get(0) == this) {
                Value upperOp1 = ((SubInstr) nonConst).getOp1();
                Value upperOp2 = ((SubInstr) nonConst).getOp2();
                if (upperOp1 instanceof ConstInt) {
                    int res = upperOp1.getNumber().intValue() + constInt.getNumber();
                    ((SubInstr) nonConst).setOp1(new ConstInt(res));
                    return nonConst;
                }
                if (upperOp2 instanceof ConstInt) {
                    int res = upperOp2.getNumber().intValue() - constInt.getNumber();
                    ((SubInstr) nonConst).setOp2(new ConstInt(res));
                    return nonConst;
                }
            }
        }
        
        return null;
    }
}
