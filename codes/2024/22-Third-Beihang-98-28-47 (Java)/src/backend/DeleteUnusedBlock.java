package backend;

import Utils.CustomList;
import backend.asmInstr.asmBr.AsmBnez;
import backend.asmInstr.asmBr.AsmJ;
import backend.itemStructure.AsmBlock;
import backend.itemStructure.AsmFunction;
import backend.itemStructure.AsmModule;

import java.util.HashMap;
import java.util.Map;

public class DeleteUnusedBlock {
    public static void run(AsmModule module) {
        HashMap<AsmBlock, AsmBlock> blocksMap = new HashMap<>();
        for (AsmFunction asmFunction : module.getFunctions()) {
            for (CustomList.Node node : asmFunction.getBlocks()) {
                AsmBlock block = (AsmBlock) node;
                if (block.getInstrs().getSize() == 1 && block.getInstrTail() instanceof AsmJ asmJ) {
                    blocksMap.put(block, asmJ.getTarget());
                }
            }
        }
        for (Map.Entry<AsmBlock, AsmBlock> entry : blocksMap.entrySet()) {
            AsmBlock block = entry.getKey();
            AsmBlock target = entry.getValue();
            while (blocksMap.containsKey(target)) {
                target = blocksMap.get(target);
            }
            blocksMap.put(block, target);
        }
        for (AsmFunction asmFunction : module.getFunctions()) {
            for (CustomList.Node node : asmFunction.getBlocks()) {
                AsmBlock block = (AsmBlock) node;
                if (blocksMap.containsKey(block)) {
                    block.removeFromList();
                    continue;
                }
                for (CustomList.Node instrNode : block.getInstrs()) {
                    if (instrNode instanceof AsmJ asmJ) {
                        if (blocksMap.containsKey(asmJ.getTarget()))
                            asmJ.setTarget(blocksMap.get(asmJ.getTarget()));
                    } else if (instrNode instanceof AsmBnez asmBnez) {
                        if (blocksMap.containsKey(asmBnez.getTarget()))
                            asmBnez.setTarget(blocksMap.get(asmBnez.getTarget()));
                    }
                }
            }
        }
    }
}
