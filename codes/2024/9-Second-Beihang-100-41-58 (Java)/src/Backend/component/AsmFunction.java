package Backend.component;

import Backend.instruction.*;
import Backend.operand.*;

import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedList;

import static Backend.operand.AllPhysicalReg.SP;
import static Backend.operand.AllPhysicalReg.phyAllRegs;

public class AsmFunction extends AsmComponent {
    private static final int WORD_SIZE = 8; // 定义常量，避免魔法数字
    private static final int WORD_ALIGNMENT = 4; // 定义对齐大小常量
    private final String name;
    private LinkedList<AsmBlock> asmBlocks;
    private int allocSize;
    private int argsSize;
    private AsmBlock Exit = null;
    private final boolean ifHasParam;

    public AsmFunction(String fullName, boolean ifHasParam) {
        this.name = removePrefix(fullName);
        asmBlocks = new LinkedList<>();
        this.allocSize = 0;
        this.argsSize = 0;
        this.ifHasParam = ifHasParam;
    }

    private String removePrefix(String fullName) {
        return fullName.startsWith("@") ? fullName.substring(1) : fullName;
    }

    public boolean isThis(String fullName) {
        return this.name.equals(removePrefix(fullName));
    }

    public String getName() {
        return name;
    }

    public void addBlocks(AsmBlock asmBlock) {
        asmBlocks.add(asmBlock);
    }

    public LinkedList<AsmBlock> getObjBlocks() {
        return asmBlocks;
    }

    public void setArgsSize(int size) {
        argsSize = size;
    }

    public int getArgsSize() {
        return argsSize;
    }

    public int getAllocSize() {
        return allocSize;
    }

    public void addAllocSize(int size) {
        allocSize += size;
    }

    public int getStackSize() {
        int size = WORD_SIZE + argsSize + allocSize;
        if (size % WORD_SIZE != 0)
            size += WORD_ALIGNMENT;
        return size;
    }

    public AsmBlock getFirstBlock() {
        return asmBlocks.getFirst();
    }

    public void setExitBlock(AsmBlock block) {
        this.Exit = block;
    }

    public AsmBlock getExitBlock() {
        return this.Exit;
    }

    public void addInstBefore(AsmInstr pos_instr, AsmInstr inst) {
        for (AsmBlock block : asmBlocks) {
            if (block.getInstrList().contains(pos_instr)) {
                block.insertBefore(pos_instr, inst);
                break;
            }
        }
    }

    public void addInstAfter(AsmInstr pos_instr, AsmInstr inst) {
        for (AsmBlock block : asmBlocks) {
            if (block.getInstrList().contains(pos_instr)) {
                block.insertAfter(pos_instr, inst);
                break;
            }
        }
    }

    public void setAsmBlocks(LinkedList<AsmBlock> reOrderBlocks) {
        this.asmBlocks = reOrderBlocks;
    }

    public boolean isIfHasParam() {
        return ifHasParam;
    }

    public void instAlignStack(int oldStackSize) {
        if (ifHasParam) {
            return;
        }
        for (AsmBlock asmBlock : asmBlocks) {
            for (AsmInstr instr : asmBlock.getInstrList()) {
                if (instr instanceof AsmMove move && move.getNeedChange()) {
                    int rightSize = getStackSize() + ((AsmImm) (move.getSrc())).getImmediate() - oldStackSize;
                    move.setSrc(new AsmImm(rightSize, false));
                }
            }
        }
    }

    public void reserveSRegs(boolean isFloat) {
        if (ifHasParam) {
            return;
        }
        HashSet<AsmReg> needChange = new HashSet<>();
        for (AsmBlock asmBlock : asmBlocks) {
            for (AsmInstr instr : asmBlock.getInstrList()) {
                if (instr instanceof AsmBinary) {
                    if (((AsmBinary) instr).getDst() instanceof PhysicalReg dst && !isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    } else if (((AsmBinary) instr).getDst() instanceof FPhysicalReg dst && isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    }
                } else if (instr instanceof AsmLoad) {
                    if (((AsmLoad) instr).getDst() instanceof PhysicalReg dst && !isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    } else if (((AsmLoad) instr).getDst() instanceof FPhysicalReg dst && isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    }
                } else if (instr instanceof AsmMove) {
                    if (((AsmMove) instr).getDst() instanceof PhysicalReg dst && !isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    } else if (((AsmMove) instr).getDst() instanceof FPhysicalReg dst && isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    }
                } else if (instr instanceof AsmConversion) {
                    if (((AsmConversion) instr).getDst() instanceof PhysicalReg dst && !isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    } else if (((AsmConversion) instr).getDst() instanceof FPhysicalReg dst && isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    }
                } else if (instr instanceof AsmLa) {
                    if (((AsmLa) instr).getDst() instanceof PhysicalReg dst && !isFloat && dst.isSReg()) {
                        needChange.add(dst);
                    }
                }
            }
        }
        helpReserve(needChange);
    }

    private void insertStoreInstr(AsmReg reg, boolean isFloat) {
        String type = isFloat ? "fsd" : "sd";
        if (getStackSize() >= -2048 && getStackSize() <= 2047) {
            AsmStore store = new AsmStore(Arrays.asList(reg, SP, new AsmImm(getStackSize(), true)), type);
            addInstAfter(getFirstBlock().getInstrList().getFirst(), store);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(phyAllRegs.get(5), new AsmImm(getStackSize(), false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(phyAllRegs.get(5), SP, phyAllRegs.get(5)));
            AsmStore store = new AsmStore(Arrays.asList(reg, phyAllRegs.get(5), new AsmImm(0, true)), type);
            addInstAfter(getFirstBlock().getInstrList().getFirst(), move);
            addInstAfter(move, add);
            addInstAfter(add, store);
        }
    }

    private void insertLoadInstr(AsmReg reg, boolean isFloat) {
        String type = isFloat ? "fld" : "ld";
        if (getStackSize() >= -2048 && getStackSize() <= 2047) {
            AsmLoad load = new AsmLoad(Arrays.asList(reg, SP, new AsmImm(getStackSize(), true)), type);
            addInstBefore(getPreInstr(getExitBlock().getInstrList().getLast(), getExitBlock().getInstrList()), load);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(phyAllRegs.get(5), new AsmImm(getStackSize(), false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(phyAllRegs.get(5), SP, phyAllRegs.get(5)));
            AsmLoad load = new AsmLoad(Arrays.asList(reg, phyAllRegs.get(5), new AsmImm(0, true)), type);
            addInstBefore(getPreInstr(getExitBlock().getInstrList().getLast(), getExitBlock().getInstrList()), load);
            addInstBefore(load, add);
            addInstBefore(add, move);
        }
    }

    private void helpReserve(HashSet<AsmReg> needChange) {
        for (AsmReg reg : needChange) {
            if (reg instanceof PhysicalReg) {
                reg.setSpillPlace(getStackSize());
                insertStoreInstr(reg, false);
                insertLoadInstr(reg, false);
                allocSize += 8;
            } else if (reg instanceof FPhysicalReg) {
                reg.setSpillPlace(getStackSize());
                insertStoreInstr(reg, true);
                insertLoadInstr(reg, true);
                allocSize += 8;
            }
        }
    }

    public void deleteBlock(AsmBlock asmBlock) {
        this.asmBlocks.remove(asmBlock);
    }

    private AsmInstr getPreInstr(AsmInstr instr, LinkedList<AsmInstr> list) {
        int index = list.indexOf(instr);
        if (index == 0) {
            return null;
        } else {
            return list.get(index - 1);
        }
    }
}
