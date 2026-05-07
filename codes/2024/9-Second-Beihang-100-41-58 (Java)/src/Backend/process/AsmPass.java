package Backend.process;

import Backend.component.AsmBlock;
import Backend.component.AsmFunction;
import Backend.component.AsmInstr;
import Backend.component.AsmModule;
import Backend.instruction.*;
import Backend.operand.*;
import config.Config;

import java.util.*;

public class AsmPass {
    private final AsmModule asmModule;
    private final HashMap<AsmBlock, LinkedList<AsmInstr>> block2Instr = new HashMap<>();
    private final HashMap<AsmFunction, LinkedList<AsmBlock>> func2Block = new HashMap<>();
    private final HashMap<AsmBlock, AsmBlock> tarBlockMap = new HashMap<>();

    public AsmPass(AsmModule asmModule) {
        this.asmModule = asmModule;
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
                block2Instr.put(asmBlock, asmBlock.getInstrList());
            }
            func2Block.put(objFunction, objFunction.getObjBlocks());
        }
    }

    public void run() {
        deleteUselessLi();
        changeReg2Zero();
        deleteUselessAdd1();
//        if (Config.isO1){
//            deleteUselessAdd2();
//        }
        reOrderBlock();
        peepHole();
        deleteUnUsedBlock();
        LinearScanAllocator linearScanAllocator = new LinearScanAllocator(block2Instr, false);
        linearScanAllocator.run();
    }

    private void deleteUselessLi() {
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            HashMap<AsmBinary, Integer> needChange = new HashMap<>();
            ArrayList<AsmInstr> needDelete = new ArrayList<>();
            for (int i = 0; i < block2Instr.get(asmBlock).size(); i++) {
                if (i + 1 != block2Instr.get(asmBlock).size()) {
                    AsmInstr nowInstr = block2Instr.get(asmBlock).get(i);
                    AsmInstr nxtInstr = block2Instr.get(asmBlock).get(i + 1);
                    if (nowInstr instanceof AsmMove transNowInstr) {
                        if (transNowInstr.getSrc() instanceof AsmImm src) {
                            if (!(src.getImmediate() >= -2048 && src.getImmediate() <= 2047)) {
                                continue;
                            }
                            if (((PhysicalReg) transNowInstr.getDst()).isSReg()) {
                                continue;
                            }
                            if (nxtInstr instanceof AsmBinary transNxtInstr) {
                                if (transNxtInstr.getType().equals("add") || transNxtInstr.getType().equals("addw")) {
                                    if (transNxtInstr.getSrc2().equals(transNowInstr.getDst())) {
                                        needDelete.add(nowInstr);
                                        needChange.put((AsmBinary) nxtInstr, ((AsmImm) transNowInstr.getSrc()).getImmediate());
                                    } else if (transNxtInstr.getSrc1().equals(transNowInstr.getDst())) {
                                        AsmOperand src1 = transNxtInstr.getSrc1();
                                        AsmOperand src2 = transNxtInstr.getSrc2();
                                        transNxtInstr.setSrcLeft(src2);
                                        transNxtInstr.setSrcRight(src1);
                                        needDelete.add(nowInstr);
                                        needChange.put((AsmBinary) nxtInstr, ((AsmImm) transNowInstr.getSrc()).getImmediate());
                                    }
                                } else if (transNxtInstr.getType().equals("subw")) {
                                    if (transNxtInstr.getSrc2().equals(transNowInstr.getDst())) {
                                        needDelete.add(nowInstr);
                                        needChange.put((AsmBinary) nxtInstr, ((AsmImm) transNowInstr.getSrc()).getImmediate());
                                    }
                                }
                            }
                        }
                    }
                }
            }
            for (AsmInstr instr : needDelete) {
                asmBlock.deleteInstr(instr);
            }
            for (Map.Entry<AsmBinary, Integer> entry : needChange.entrySet()) {
                AsmBinary binary = entry.getKey();
                int value = entry.getValue();
                switch (binary.getType()) {
                    case "add" -> {
                        binary.setSrcRight(new AsmImm(value, true));
                        binary.setType("addi");
                    }
                    case "addw" -> {
                        binary.setSrcRight(new AsmImm(value, true));
                        binary.setType("addiw");
                    }
                    case "subw" -> {
                        binary.setSrcRight(new AsmImm(-value, true));
                        binary.setType("addiw");
                    }
                }
            }
        }
    }

    private void changeReg2Zero() {
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            ArrayList<AsmInstr> needDelete = new ArrayList<>();
            for (int i = 0; i < block2Instr.get(asmBlock).size(); i++) {
                if (i + 1 != block2Instr.get(asmBlock).size()) {
                    AsmInstr nowInstr = block2Instr.get(asmBlock).get(i);
                    AsmInstr nxtInstr = block2Instr.get(asmBlock).get(i + 1);
                    if (nowInstr instanceof AsmMove transNowInstr) {
                        if (transNowInstr.getSrc() instanceof AsmImm src) {
                            if (src.getImmediate() != 0) {
                                continue;
                            }
                            if (((PhysicalReg) transNowInstr.getDst()).isSReg()) {
                                continue;
                            }
                            if (nxtInstr instanceof AsmStore transNxtInstr) {
                                if (transNxtInstr.getSrc().equals(transNowInstr.getDst())) {
                                    needDelete.add(nowInstr);
                                    transNxtInstr.setSrc(AllPhysicalReg.ZERO);
                                }
                            }
                        } else {
                            if (nxtInstr instanceof AsmMove transNxtInstr) {
                                if (transNowInstr.getDst().equals(transNxtInstr.getDst()) && !transNxtInstr.getDst().equals(transNxtInstr.getSrc())) {
                                    needDelete.add(nowInstr);
                                }
                            }
                        }
                    }
                }
            }
            for (AsmInstr instr : needDelete) {
                asmBlock.deleteInstr(instr);
            }
        }
    }

    private void deleteUselessAdd1() {
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            ArrayList<AsmInstr> needDelete = new ArrayList<>();
            ArrayList<AsmBinary> needChange = new ArrayList<>();
            for (int i = 0; i < block2Instr.get(asmBlock).size(); i++) {
                if (block2Instr.get(asmBlock).get(i) instanceof AsmBinary binary) {
                    if (binary.getType().equals("addi") || binary.getType().equals("addiw")) {
                        if (binary.getSrc2() instanceof AsmImm imm && imm.getImmediate() == 0) {
                            needChange.add(binary);
                            continue;
                        }
                    }
                }
                if (i + 1 != block2Instr.get(asmBlock).size()) {
                    AsmInstr nowInstr = block2Instr.get(asmBlock).get(i);
                    AsmInstr nxtInstr = block2Instr.get(asmBlock).get(i + 1);
                    if (nowInstr instanceof AsmBinary binary) {
                        if (!binary.getType().equals("addi")) {
                            continue;
                        }
                        if (!binary.getSrc1().equals(AllPhysicalReg.SP)) {
                            continue;
                        }
                        if (nxtInstr instanceof AsmLoad load) {
                            if (binary.getDst().equals(load.getAddress())) {
                                if (load.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    load.setAddress(AllPhysicalReg.SP);
                                    load.setOffset(binary.getSrc2());
                                }
                            }
                        } else if (nxtInstr instanceof AsmStore store) {
                            if (binary.getDst().equals(store.getAddress())) {
                                if (store.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    store.setAddress(AllPhysicalReg.SP);
                                    store.setOffset(binary.getSrc2());
                                }
                            }
                        }
                    }
                }
            }
            for (AsmInstr instr : needDelete) {
                asmBlock.deleteInstr(instr);
            }
            for (AsmBinary binary : needChange) {
                AsmMove move = new AsmMove(Arrays.asList(binary.getDst(), binary.getSrc1()));
                asmBlock.insertAfter(binary, move);
                asmBlock.deleteInstr(binary);
            }
        }
    }

    private void deleteUselessAdd2() {
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            ArrayList<AsmInstr> needDelete = new ArrayList<>();
            ArrayList<AsmBinary> needChange = new ArrayList<>();
            for (int i = 0; i < block2Instr.get(asmBlock).size(); i++) {
                if (block2Instr.get(asmBlock).get(i) instanceof AsmBinary binary) {
                    if (binary.getType().equals("addi") || binary.getType().equals("addiw") || binary.getType().equals("add")) {
                        if (binary.getSrc2() instanceof AsmImm imm && imm.getImmediate() == 0) {
                            needChange.add(binary);
                            continue;
                        }
                    }
                }
                if (i + 1 < block2Instr.get(asmBlock).size()) {
                    AsmInstr nowInstr = block2Instr.get(asmBlock).get(i);
                    AsmInstr nxtInstr = block2Instr.get(asmBlock).get(i + 1);
                    if (nowInstr instanceof AsmBinary binary) {
                        if (!(binary.getType().equals("addi") || binary.getType().equals("add"))) {
                            continue;
                        }
                        if (binary.getType().equals("add")) {
                            if (!binary.isSrc2Imm12()) {
                                continue;
                            }
                        }
                        if (((PhysicalReg) binary.getDst()).isSReg() || binary.getDst().equals(AllPhysicalReg.SP)) {
                            continue;
                        }
                        if (binary.getDst() instanceof PhysicalReg) {
                            if (AllPhysicalReg.phyA.containsValue((PhysicalReg) binary.getDst())) {
                                continue;
                            }
                        }
                        if (binary.getDst() instanceof FPhysicalReg) {
                            if (AllPhysicalReg.fPhyA.containsValue((FPhysicalReg) binary.getDst())) {
                                continue;
                            }
                        }
                        if (nxtInstr instanceof AsmLoad load) {
                            if (binary.getDst().equals(load.getAddress())) {
                                if (load.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    load.setAddress(binary.getSrc1());
                                    load.setOffset(binary.getSrc2());
                                }
                            }
                        } else if (nxtInstr instanceof AsmStore store) {
                            if (binary.getDst().equals(store.getAddress())) {
                                if (store.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    store.setAddress(binary.getSrc1());
                                    store.setOffset(binary.getSrc2());
                                }
                            }
                        }
                    }
                }
                if (i + 2 < block2Instr.get(asmBlock).size()) {
                    AsmInstr nowInstr = block2Instr.get(asmBlock).get(i);
                    AsmInstr nxtInstr = block2Instr.get(asmBlock).get(i + 2);
                    if (nowInstr instanceof AsmBinary binary) {
                        if (!(binary.getType().equals("addi") || binary.getType().equals("add"))) {
                            continue;
                        }
                        if (binary.getType().equals("add")) {
                            if (!binary.isSrc2Imm12()) {
                                continue;
                            }
                        }
                        if (((PhysicalReg) binary.getDst()).isSReg() || binary.getDst().equals(AllPhysicalReg.SP)) {
                            continue;
                        }
                        if (binary.getDst() instanceof PhysicalReg) {
                            if (AllPhysicalReg.phyA.containsValue((PhysicalReg) binary.getDst())) {
                                continue;
                            }
                        }
                        if (binary.getDst() instanceof FPhysicalReg) {
                            if (AllPhysicalReg.fPhyA.containsValue((FPhysicalReg) binary.getDst())) {
                                continue;
                            }
                        }
                        if (nxtInstr instanceof AsmLoad load) {
                            if (binary.getDst().equals(load.getAddress())) {
                                if (load.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    load.setAddress(binary.getSrc1());
                                    load.setOffset(binary.getSrc2());
                                }
                            }
                        } else if (nxtInstr instanceof AsmStore store) {
                            if (binary.getDst().equals(store.getAddress())) {
                                if (store.getOffset() instanceof AsmImm offset && offset.getImmediate() == 0) {
                                    needDelete.add(nowInstr);
                                    store.setAddress(binary.getSrc1());
                                    store.setOffset(binary.getSrc2());
                                }
                            }
                        }
                    }
                }
            }
            for (AsmInstr instr : needDelete) {
                asmBlock.deleteInstr(instr);
            }
            for (AsmBinary binary : needChange) {
                AsmMove move = new AsmMove(Arrays.asList(binary.getDst(), binary.getSrc1()));
                asmBlock.insertAfter(binary, move);
                asmBlock.deleteInstr(binary);
            }
        }
    }

    private void deleteUnUsedBlock() {
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
                if (asmBlock.getInstrList().size() == 1 && asmBlock.getInstrList().getLast() instanceof AsmJ asmJ) {
                    tarBlockMap.put(asmBlock, asmJ.getTarget());
                }
            }
        }
        for (Map.Entry<AsmBlock, AsmBlock> entry : tarBlockMap.entrySet()) {
            AsmBlock block = entry.getKey();
            AsmBlock target = entry.getValue();
            while (tarBlockMap.containsKey(target)) {
                target = tarBlockMap.get(target);
            }
            tarBlockMap.put(block, target);
        }
        for (AsmFunction asmFunction : func2Block.keySet()) {
            ArrayList<AsmBlock> needDelete = new ArrayList<>();
            LinkedList<AsmBlock> blocks = func2Block.get(asmFunction);
            for (AsmBlock asmBlock : blocks) {
                if (tarBlockMap.containsKey(asmBlock)) {
                    needDelete.add(asmBlock);
                    continue;
                }
                for (AsmInstr instr : asmBlock.getInstrList()) {
                    if (instr instanceof AsmJ asmJ) {
                        if (tarBlockMap.containsKey(asmJ.getTarget())) {
                            asmJ.setTarget(tarBlockMap.get(asmJ.getTarget()));
                        }
                    } else if (instr instanceof AsmBeqz asmBeqz) {
                        if (tarBlockMap.containsKey(asmBeqz.getTarget())) {
                            asmBeqz.setTarget(tarBlockMap.get(asmBeqz.getTarget()));
                        }
                    }
                }
            }
            for (AsmBlock asmBlock : needDelete) {
                asmFunction.deleteBlock(asmBlock);
            }
        }
    }

    private void reOrderBlock() {
        for (AsmFunction asmFunction : func2Block.keySet()) {
            LinkedList<AsmBlock> reOrderBlocks = new LinkedList<>();
            LinkedList<AsmBlock> oldBlocks = func2Block.get(asmFunction);
            for (AsmBlock currentBlock : oldBlocks) {
                if (reOrderBlocks.contains(currentBlock)) {
                    continue;
                }
                reOrderBlocks.add(currentBlock);
                AsmInstr lastInstr = currentBlock.getInstrList().getLast();
                int size = currentBlock.getInstrList().size();
                AsmInstr last2Instr;
                if (size >= 2) {
                    last2Instr = currentBlock.getInstrList().get(size - 2);
                } else {
                    last2Instr = null;
                }
                if (lastInstr instanceof AsmJ j) {
                    if (last2Instr instanceof AsmBeqz beqZ) {
                        AsmBlock jTarget = j.getTarget();
                        AsmBlock beqZTarget = beqZ.getTarget();
                        if (beqZTarget.equals(currentBlock)) {
                            continue;
                        }
                        if (reOrderBlocks.contains(beqZTarget)) {
                            continue;
                        }
                        reOrderBlocks.add(beqZTarget);
                        if (jTarget.equals(currentBlock)) {
                            continue;
                        }
                        if (reOrderBlocks.contains(jTarget)) {
                            continue;
                        }
                        reOrderBlocks.add(jTarget);
                    } else {
                        AsmBlock target = j.getTarget();
                        if (target.equals(currentBlock)) {
                            continue;
                        }
                        if (reOrderBlocks.contains(target)) {
                            continue;
                        }
                        reOrderBlocks.add(target);
                    }
                }
            }
            asmFunction.setAsmBlocks(reOrderBlocks);
        }
    }

    private void peepHole() {
        for (AsmFunction asmFunction : asmModule.getFunctions()) {
            for (AsmBlock asmBlock : asmFunction.getObjBlocks()) {
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
                            }
                        }
                    }
                }
                int index = 0;
                for (AsmInstr instr : needDeleteLoad) {
                    asmBlock.insertAfter(instr, needAddMove.get(index));
                    index++;
                }
                for (AsmInstr instr : needDeleteLoad) {
                    asmBlock.deleteInstr(instr);
                }
            }
            for (AsmBlock asmBlock : asmFunction.getObjBlocks()) {
                asmBlock.getInstrList().removeIf(inst -> inst instanceof AsmMove && ((AsmMove) inst).getDst().equals(((AsmMove) inst).getSrc()));
            }
        }
    }
}
