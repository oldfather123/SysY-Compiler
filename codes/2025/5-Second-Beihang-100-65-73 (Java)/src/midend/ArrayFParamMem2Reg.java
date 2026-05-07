package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.List;

/**
 * todo: 这个方法的实现可能随着对类型系统的重整出现一些问题，如果需要建议重写
 */
public class ArrayFParamMem2Reg {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            BasicBlock headBlk = function.getFirstBlk();
            Instruction ins = headBlk.getFirstInstr();
            while (ins instanceof AllocaInstr alloca) {
                if (alloca.getReferencedType() instanceof TPointer) {
                    Value initVal = null;
                    List<Value> users = new ArrayList<>(alloca.getUserList());
                    for (Value user : users) {
                        if (user instanceof StoreInstr store && initVal == null) {
                            initVal = store.getValueToStore();
                            store.removeFromList();
                        } else if (user instanceof LoadInstr load) {
                            load.replaceUseWith(initVal);
                            load.removeFromList();
                        }
                    }
                    alloca.removeFromList();
                }
                ins = (Instruction) ins.getNext();
            }
        }
    }
}
