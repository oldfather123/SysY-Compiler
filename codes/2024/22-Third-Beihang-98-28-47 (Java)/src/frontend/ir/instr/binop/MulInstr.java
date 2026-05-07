package frontend.ir.instr.binop;

import frontend.ir.constvalue.ConstInt;
import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

public class MulInstr extends BinaryOperation implements Swappable {
    public boolean is64 = false;
    public MulInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "mul", DataType.INT);
        assert op1.getDataType() == DataType.INT;
        assert op2.getDataType() == DataType.INT;
        trySwapOp();
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new MulInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
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
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            return new ConstInt(((ConstInt) op1).getNumber() * ((ConstInt) op2).getNumber());
        } else if (op1 instanceof ConstInt) {
            return mergeConst((ConstInt) op1, op2);
        } else if (op2 instanceof ConstInt) {
            return mergeConst((ConstInt) op2, op1);
        }
        return null;
    }
    
    private Value mergeConst(ConstInt constInt, Value nonConst) {
        if (constInt == null || nonConst == null) {
            throw new NullPointerException();
        }
        if (constInt.getNumber() == 0) {
            return constInt;
        }
        if (nonConst instanceof MulInstr) {
            ArrayList<Value> userSet2list = new ArrayList<>(nonConst.getUserSet());
            if (userSet2list.size() == 1 && userSet2list.get(0) == this) {
                Value upperOp1 = ((MulInstr) nonConst).getOp1();
                Value upperOp2 = ((MulInstr) nonConst).getOp2();
                if (upperOp1 instanceof ConstInt) {
                    int res = upperOp1.getNumber().intValue() * constInt.getNumber();
                    ((MulInstr) nonConst).setOp1(new ConstInt(res));
                    return nonConst;
                }
                if (upperOp2 instanceof ConstInt) {
                    int res = upperOp2.getNumber().intValue() * constInt.getNumber();
                    ((MulInstr) nonConst).setOp2(new ConstInt(res));
                    return nonConst;
                }
            }
        } else if (nonConst instanceof SDivInstr) {
            ArrayList<Value> userSet2list = new ArrayList<>(nonConst.getUserSet());
            if (userSet2list.size() == 1 && userSet2list.get(0) == this) {
                Value upperOp1 = ((SDivInstr) nonConst).getOp1();
                if (upperOp1 instanceof ConstInt) {
                    int res = upperOp1.getNumber().intValue() * constInt.getNumber();
                    ((SDivInstr) nonConst).setOp1(new ConstInt(res));
                    return nonConst;
                }
            }
        }
        
        return null;
    }
    
    public ArrayList<Instruction> strengthReduction(Function function) {
        ArrayList<Instruction> res = new ArrayList<>();
        
        if (op2 instanceof ConstInt) {
            int intValue = op2.getNumber().intValue();
            int absValue = Math.abs(intValue);
            
            Value abs = null;
            int exp = 0;
            while (absValue > 0) {
                if ((absValue & 1) > 0) {
                    if (exp == 0) {
                        abs = op1;
                    } else {
                        ShlInstr newShl = new ShlInstr(function.getAndAddRegIndex(), op1, new ConstInt(exp));
                        res.add(newShl);
                        if (abs == null) {
                            abs = newShl;
                        } else {
                            abs = new AddInstr(function.getAndAddRegIndex(), abs, newShl);
                            res.add((Instruction) abs);
                        }
                    }
                }
                exp += 1;
                absValue >>= 1;
            }
            
            if (intValue < 0) {
                ConstInt zero = ConstInt.Zero;
                SubInstr sub = new SubInstr(function.getAndAddRegIndex(), zero, abs);
                res.add(sub);
            }
        }
        
        return res;
    }
}
