package backend;

import Utils.CustomList;
import backend.asmInstr.asmBinary.AsmSlt;
import backend.asmInstr.asmBr.AsmBnez;
import backend.asmInstr.asmLS.AsmL;
import backend.asmInstr.asmLS.AsmMove;
import backend.asmInstr.asmLS.AsmS;
import backend.asmInstr.asmTermin.AsmCall;
import backend.itemStructure.*;
import backend.regs.RegGeter;

import java.util.ArrayList;

public class PeepHole {
    public static void run(AsmModule module) {
        passOne(module);
    }

    public static void passOne(AsmModule module) {
        for (AsmFunction asmFunction : module.getFunctions()) {
            for (CustomList.Node node : asmFunction.getBlocks()) {
                AsmBlock block = (AsmBlock) node;
                for (CustomList.Node instrNode : block.getInstrs()) {
                    if (instrNode instanceof AsmS asmS) {
                        if (instrNode.getNext() != null && instrNode.getNext() instanceof AsmL asmL) {
                            if (asmL.getOffset() == null) {
                                //考虑到有La,LLa,Move的情况
                                continue;
                            }
                            if (asmS.getSrc() == asmL.getDst() && ((AsmImm12) (asmS.getOffset())).getValue() == ((AsmImm12) (asmL.getOffset())).getValue() && asmS.getAddr() == asmL.getSrc()) {
                                //asmS.removeFromList();
                                asmL.removeFromList();
                            }
                        }
                    }
                    if (instrNode instanceof AsmMove asmMove) {
                        if (asmMove.getSrc() instanceof AsmImm12 asmImm12 && asmImm12.getValue() == 0
                                || asmMove.getSrc() instanceof AsmImm32 asmImm32 && asmImm32.getValue() == 0) {
                            if (asmMove.getNext() instanceof AsmSlt asmSlt && asmSlt.getSrc1() == asmMove.getDst()) {
                                asmMove.removeFromList();
                                asmSlt.changeSrc1(RegGeter.ZERO);
                            }
                        }
                    }
                }
            }
        }
    }
}
