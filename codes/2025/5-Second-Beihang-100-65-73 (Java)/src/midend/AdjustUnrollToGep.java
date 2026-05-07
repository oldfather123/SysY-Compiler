package midend;

import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.List;

public class AdjustUnrollToGep {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            for (BasicBlock block : function.getBasicBlockList()) {
                List<AddInstr> addsList = new ArrayList<>();
                for (Instruction instr : block.getInstrList()) {
                    if (instr instanceof AddInstr addInstr) {
                        addsList.add(addInstr);
                    }
                }
                int addInstrCnt = addsList.size();
                for (int i = 0; i < addInstrCnt - 3; i++) {
                    AddInstr add1 = addsList.get(i);
                    AddInstr add2 = addsList.get(i + 1);
                    AddInstr add3 = addsList.get(i + 2);
                    AddInstr add4 = addsList.get(i + 3);
                    boolean flag1 = add1.getOperand2() instanceof IRConst const1
                            && const1.getConstVal().intValue() == 1;
                    boolean flag2 = add1.equals(add2.getOperand1())
                            && add2.getOperand2() instanceof IRConst const2
                            && const2.getConstVal().intValue() == 1;
                    boolean flag3 = add2.equals(add3.getOperand1())
                            && add3.getOperand2() instanceof IRConst const3
                            && const3.getConstVal().intValue() == 1;
                    boolean flag4 = add3.equals(add4.getOperand1())
                            && add4.getOperand2() instanceof IRConst const4
                            && const4.getConstVal().intValue() == 1;
                    if (flag1 && flag2 && flag3 && flag4) {
                        add2.modifyUse(add2.getOperand1(), add1.getOperand1());
                        add2.modifyUse(add2.getOperand2(), new IntConst(2));
                        add3.modifyUse(add3.getOperand1(), add1.getOperand1());
                        add3.modifyUse(add3.getOperand2(), new IntConst(3));
                        add4.modifyUse(add4.getOperand1(), add1.getOperand1());
                        add4.modifyUse(add4.getOperand2(), new IntConst(4));
                    }
                }
            }
        }
    }
}
