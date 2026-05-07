package frontend.ir.instr.binop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class SRemInstr extends BinaryOperation {
    public SRemInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "srem", DataType.INT);
        assert op1.getDataType() == DataType.INT;
        assert op2.getDataType() == DataType.INT;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new SRemInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            return new ConstInt(((ConstInt) op1).getNumber() % ((ConstInt) op2).getNumber());
        }
        return null;
    }
    
    /**
     * 用于简化累加取模的表达式
     * 如果合并成功会返回一条加法指令，以后可能会加入累乘化简、加减化简等情形
     */
    public BinaryOperation sumSimplify(Procedure procedure) {
        if (op2 instanceof ConstInt) {
            int divisor = ((ConstInt) op2).getNumber();
            if (op1 instanceof AddInstr) {
                ((AddInstr) op1).trySwapOp();
                Value addOp1 = ((AddInstr) op1).getOp1();
                Value addOp2 = ((AddInstr) op1).getOp2();
                if (addOp1 instanceof SRemInstr && addOp2 instanceof ConstInt) {
                    Value addOp1Dividend = ((SRemInstr) addOp1).getOp1();
                    Value addOp1Divisor  = ((SRemInstr) addOp1).getOp2();
                    if (op2.equals(addOp1Divisor)) {
                        if (addOp1Dividend instanceof ConstInt) {
                            ConstInt newOp =new ConstInt((addOp1Dividend.getNumber().intValue() % divisor
                                    + addOp2.getNumber().intValue() % divisor) % divisor);
                            modifyUse(this.op1, newOp);
                            return null;
                        } else if (addOp1Dividend instanceof AddInstr) {
                            Value mightBeConst = ((AddInstr) addOp1Dividend).getOp2();
                            if (mightBeConst instanceof ConstInt) {
                                ConstInt newOp = new ConstInt((mightBeConst.getNumber().intValue() % divisor
                                        + addOp2.getNumber().intValue() % divisor) % divisor);
                                AddInstr newAdd = new AddInstr(procedure.getAndAddRegIndex(),
                                        ((AddInstr) addOp1Dividend).getOp1(), newOp);
                                modifyUse(this.op1, newAdd);
                                return newAdd;
                            }
                        }
                    }
                }
            }
        }
        
        return null;
    }
}
