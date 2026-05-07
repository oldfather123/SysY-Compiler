package Backend.process;

import Backend.component.AsmBlock;
import Backend.component.AsmFunction;
import Backend.component.AsmInstr;
import Backend.component.AsmModule;
import Backend.instruction.*;
import Backend.operand.AsmImm;
import Backend.operand.FVirtualReg;
import Backend.operand.AsmOperand;
import Backend.operand.VirtualReg;
import config.Config;

import java.util.*;

public class RemoveNoUseReg {
    private final AsmModule asmModule;
    private final HashSet<AsmOperand> ifUsed = new HashSet<>();

    public RemoveNoUseReg(AsmModule asmModule) {
        this.asmModule = asmModule;
    }

    public void run() {
        removeNoUseReg();
//        if (Config.isO1) {
//            peepHole();
//        }
    }

    private void removeNoUseReg() {
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
                asmBlock.getInstrList().removeIf(inst -> inst instanceof AsmMove && ((AsmMove) inst).getDst().equals(((AsmMove) inst).getSrc()));
                for (AsmInstr instr : asmBlock.getInstrList()) {
                    ifUsed.addAll(instr.getRegUse());
                }
            }
        }
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
                Iterator<AsmInstr> iterator = asmBlock.getInstrList().iterator();
                while (iterator.hasNext()) {
                    AsmInstr instr = iterator.next();
                    if (!(instr.getRegDef() == null) && !ifUsed.contains(instr.getRegDef())) {
                        if (instr.getRegDef() instanceof VirtualReg || instr.getRegDef() instanceof FVirtualReg) {
                            iterator.remove();
                        }
                    }
                }
            }
        }
    }

    private void peepHole() {
        for (AsmFunction asmFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : asmFunction.getObjBlocks()) {
                ArrayList<AsmInstr> needDeleteStore = new ArrayList<>();
                ArrayList<AsmInstr> needDeleteLoad = new ArrayList<>();
                ArrayList<AsmInstr> needAddMove = new ArrayList<>();
                for (int i = 0; i < asmBlock.getInstrList().size(); i++) {
                    if (i + 1 != asmBlock.getInstrList().size()) {
                        AsmInstr nowInstr = asmBlock.getInstrList().get(i);
                        AsmInstr nextInstr = asmBlock.getInstrList().get(i + 1);
                        if (nowInstr instanceof AsmStore store && nextInstr instanceof AsmLoad load) {
                            if (load.getAddress() == null) {
                                continue;
                            }
                            if (store.getAddress().equals(load.getAddress()) && ((AsmImm) store.getOffset()).getImmediate() == ((AsmImm) load.getOffset()).getImmediate()) {
                                AsmMove move = new AsmMove(Arrays.asList(load.getDst(), store.getSrc()));
                                needAddMove.add(move);
                                needDeleteLoad.add(load);
                                needDeleteStore.add(store);
                            }
                        }
                    }
                }
                int index = 0;
                for (AsmInstr instr : needDeleteLoad) {
                    asmBlock.insertAfter(instr, needAddMove.get(index));
                    index++;
                }
                for (AsmInstr instr : needDeleteStore) {
                    asmBlock.deleteInstr(instr);
                }
                for (AsmInstr instr : needDeleteLoad) {
                    asmBlock.deleteInstr(instr);
                }
            }
        }
    }
}
