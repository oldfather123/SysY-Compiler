package backend.opt.riscv;

import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.rv.memory.RVLoad;
import backend.asm.instr.rv.memory.RVStore;
import backend.asm.instr.rv.others.RVLa;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMGlobalObject;
import backend.asm.structure.ASMModel;
import backend.opt.ASMGPool;
import util.CustomList;

import java.util.Iterator;

public class LoadFloatFromPool {
    public static final ASMGPool gPool = ASMGPool.getInstance();

    public static void execute(ASMModel asmModel) {
        asmModel.addGlobalObject(gPool);
        Iterator<ASMGlobalObject> it = asmModel.getGlobalObjectlist().iterator();
        while (it.hasNext()) {
            ASMGlobalObject obj = it.next();
            if (obj.isFloat()) {
                gPool.addGlobal(obj);
                it.remove();
            }
        }
        for (ASMFunction function : asmModel.getFunctionList()) {
            for (CustomList.Node blockNode : function.getBasicBlockList()) {
                ASMBasicBlock asmBasicBlock = (ASMBasicBlock) blockNode;
                replaceInBlock(asmBasicBlock);
            }
        }
    }

    public static void replaceInBlock(ASMBasicBlock block) {
        for (CustomList.Node instrNode : block.getInstructionList()) {
            ASMInstruction nowInstr = (ASMInstruction) instrNode;
            if (nowInstr.getNext() == null) break;
            ASMInstruction nextInstr = (ASMInstruction) nowInstr.getNext();
            if (nowInstr instanceof RVLa la && la.isLoadingFloat()) {
                int offset = gPool.getOffset(la.getGlobalObject());
                if (nextInstr instanceof RVLoad rvLoad) {
                    if (la.getRegister().equals(rvLoad.getBase())) {
                        la.setGlobalObject(gPool);
                        rvLoad.setOffset(new ASMImmediate(offset));
                    }
                } else if (nextInstr instanceof RVStore rvStore) {
                    if (la.getRegister().equals(rvStore.getBase())) {
                        la.setGlobalObject(gPool);
                        rvStore.setOffset(new ASMImmediate(offset));
                    }
                }
            }
        }
    }
}
