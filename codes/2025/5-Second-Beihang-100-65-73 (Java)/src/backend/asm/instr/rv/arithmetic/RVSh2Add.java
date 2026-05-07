package backend.asm.instr.rv.arithmetic;

import backend.asm.ASMValue;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class RVSh2Add extends RVArithmetic {
    public RVSh2Add(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(operand1, operand2);
    }

    @Override
    protected String printIns() {
        //注意：这里先打印operand2，再打印operand1
        return "sh2add " + this.printAsOperand() + ", " + operand2.printAsOperand() + ", " + operand1.printAsOperand();
    }

}
