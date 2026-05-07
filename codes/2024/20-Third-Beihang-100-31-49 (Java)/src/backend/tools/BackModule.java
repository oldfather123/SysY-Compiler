package backend.tools;

import backend.Pass.InstructionMerge;
import backend.Pass.Straighten;
import backend.instructions.BackBinary;
import backend.instructions.BackInstruction;
import backend.instructions.BackLoad;
import backend.instructions.BackMov;
import backend.instructions.BackStore;
import backend.operand.BackIReg;
import backend.operand.BackImm;
import backend.operand.BackImm12;
import backend.operand.BackRegTable;
import mid.IntermediatePresentation.BasicBlock;
import utils.tools.Timer;

import java.util.ArrayList;
import java.util.HashMap;

public class BackModule {
    private ArrayList<BackFunction> backFunctions;
    private ArrayList<BackGlobalDecl> backGlobalDecls;
    public HashMap<BasicBlock, BackBlock> midBlockToBack = new HashMap<>();

    public BackModule() {
        backFunctions = new ArrayList<>();
        backGlobalDecls = new ArrayList<>();
    }

    public void addFunction(BackFunction backFunction) {
        backFunctions.add(backFunction);
    }

    public void addGlobalDecl(BackGlobalDecl backGlobalDecl) {
        backGlobalDecls.add(backGlobalDecl);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (BackGlobalDecl backGlobalDecl : backGlobalDecls) {
            sb.append(backGlobalDecl.toString());
        }
        for (BackFunction backFunction : backFunctions) {
            sb.append(backFunction.toString());
        }
        return sb.toString();
    }

    public ArrayList<BackFunction> getFunctions() {
        return backFunctions;
    }

    public void optimize() {
        boolean flag;
        (new Straighten(this)).optimize();
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        InstructionMerge instructionMerge = new InstructionMerge(this);
        do {
            flag = instructionMerge.optimize();
        } while (flag);

        BackIReg t6 = BackRegTable.getBackReg("t6");
        for (BackFunction f : backFunctions) {
            if (f.getStackSize() < 2048) {
                continue;
            }
            for (BackBlock block : f.getBackBlocks()) {
                for (BackInstruction i : new ArrayList<>(block.getBackInstructions())) {
                    if (i instanceof BackBinary binary && binary.getName().equals("addi") &&
                            binary.getSrc2() instanceof BackImm imm12 && imm12.getImm() >= 2048) {
                        int index = block.getBackInstructions().indexOf(i);
                        block.getBackInstructions().remove(i);
                        block.getBackInstructions().add(index, new BackMov(t6, imm12));
                        block.getBackInstructions().add(index + 1, new BackBinary("add", binary.getDst(), binary.getSrc1(), t6));
                    } else if (i instanceof BackLoad load && load.getOffset() != null && load.getOffset() >= 2048) {
                        int index = block.getBackInstructions().indexOf(i);
                        block.getBackInstructions().remove(i);
                        block.getBackInstructions().add(index, new BackMov(t6, new BackImm(load.getOffset())));
                        block.getBackInstructions().add(index + 1, new BackBinary("add", t6, t6, load.getBase()));
                        block.getBackInstructions().add(index + 2, new BackLoad(load.getName(),
                                load.getDst(), new BackImm12(0), t6));
                    } else if (i instanceof BackStore store && store.getOffset() != null && store.getOffset() >= 2048) {
                        int index = block.getBackInstructions().indexOf(i);
                        block.getBackInstructions().remove(i);
                        block.getBackInstructions().add(index, new BackMov(t6, new BackImm(store.getOffset())));
                        block.getBackInstructions().add(index + 1, new BackBinary("add", t6, t6, store.getBase()));
                        block.getBackInstructions().add(index + 2, new BackLoad(store.getName(),
                                store.getSrc(), new BackImm12(0), t6));
                    }
                }
            }
        }
    }
}
