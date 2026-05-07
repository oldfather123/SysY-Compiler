package midend.SSA;

import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.io.IOException;
import java.util.ArrayList;

/**
 * 针对数组形参的 mem2reg
 * 传入的数组形参实际上是数组首元素的指针，理论上不会被修改，因此 load 的结果就应该是第一次 store 的内容
 * 可以比较轻易地进行 mem2reg
 */
public class ArrayFParamMem2Reg {
    public static void execute(ArrayList<Function> functions) throws IOException {
        for (Function function : functions) {
            BasicBlock block = (BasicBlock) function.getBasicBlocks().getHead();
            Instruction instr = (Instruction) block.getInstructions().getHead();
            while (instr != null) {
                if (instr instanceof AllocaInstr) {
                    if (((AllocaInstr) instr).getSymbol().isArrayFParam()) {
                        StoreInstr init = null;
                        Use use = instr.getBeginUse();
                        while (use != null) {
                            Value user = use.getUser();
                            if (user instanceof StoreInstr) {
                                if (init == null) {
                                    init = (StoreInstr) user;
                                } else {
                                    throw new RuntimeException("理论上数组参数只应该被存储一次");
                                }
                            } else if (user instanceof LoadInstr) {
                                if (init == null) {
                                    throw new RuntimeException("这咋还没存就开始取了");
                                } else {
                                    user.replaceUseTo(init.getValue());
                                    user.removeFromList();
                                }
                            }
                            use = (Use) use.getNext();
                        }
                        if (init == null) {
                            throw new RuntimeException("数组参数不应该没被存储过");
                        } else {
                            init.removeFromList();
                        }
                    }
                }
                instr = (Instruction) instr.getNext();
            }
        }
    }
}
