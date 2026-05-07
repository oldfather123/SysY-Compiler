package IR.optimizer;

import IR.IRInstruction.BranchInstruction;
import IR.IRInstruction.CompareInstruction;
import IR.IRInstruction.IRInstruction;
import IR.IRInstruction.ZextInstruction;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRFunctionBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class BrCmpOpt implements OptForIR {
    //针对跳转的if语句简化
    @Override
    public void Optimize(IRFunctionBlockRef irFunctionBlockRef) {
        for (IRBaseBlockRef irBaseBlockRef : irFunctionBlockRef.getBaseBlocks()) {
            List<IRInstruction> irInstructions = irBaseBlockRef.getInstructionList();
            Iterator<IRInstruction> it = irInstructions.iterator();
            List<Integer> toRemove = new ArrayList<>();

            while (it.hasNext()) {
                IRInstruction instruction = it.next();
                int currentIndex = irInstructions.indexOf(instruction);

                if (instruction instanceof CompareInstruction) {
                    if (currentIndex + 1 < irInstructions.size() && irInstructions.get(currentIndex + 1) instanceof ZextInstruction) {
                        if (currentIndex + 2 < irInstructions.size() && irInstructions.get(currentIndex + 2) instanceof CompareInstruction) {
                            if ((irInstructions.get(currentIndex + 2).getOperands().get(1) != null
                                    && irInstructions.get(currentIndex + 2).getOperands().get(1).getText().equals("0"))
                            || (irInstructions.get(currentIndex + 2).getOperands().get(2) != null
                                    && irInstructions.get(currentIndex + 2).getOperands().get(2).getText().equals("0"))) {
                                if (currentIndex + 3 < irInstructions.size() && irInstructions.get(currentIndex + 3) instanceof BranchInstruction) {
                                    IRValueRef irValueRef = instruction.getOperands().get(0);
                                    IRInstruction branchInstruction = irInstructions.get(currentIndex + 3);
                                    branchInstruction.IRSetOperand(0, irValueRef);
                                    toRemove.add(currentIndex + 1);
                                    toRemove.add(currentIndex + 2);
                                }
                            }
                        }
                    }
                }
            }
            for (int i = toRemove.size() - 1; i >= 0; i--) {
                irInstructions.remove((int) toRemove.get(i));
            }
        }
    }
}