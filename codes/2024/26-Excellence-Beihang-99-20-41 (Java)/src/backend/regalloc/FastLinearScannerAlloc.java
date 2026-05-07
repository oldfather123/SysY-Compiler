package backend.regalloc;

import backend.component.ObjBlock;
import backend.component.ObjFunction;
import backend.component.ObjInstr;
import backend.component.ObjModule;
import backend.instruction.ObjBinary;
import backend.instruction.ObjLoad;
import backend.instruction.ObjMove;
import backend.instruction.ObjStore;
import backend.operand.*;

import java.util.*;
import java.util.stream.Collectors;

public class FastLinearScannerAlloc {
    private final ObjModule module;
    private int nowIndex;
    private ArrayList<ObjOperand> virRegList;
    private ArrayList<ObjOperand> activeIntegerList;
    private ArrayList<ObjOperand> activeFloatList;
    private int[] integerReg = new int[32];
    private int[] floatReg = new int[32];
    private HashMap<ObjOperand, Integer> regMap = new HashMap<>();
    private HashMap<ObjOperand, Integer> regMapInStack = new HashMap<>();
    private Stack<Integer> sRegStack = new Stack<>();
    private Stack<Integer> tRegStack = new Stack<>();
    private Stack<Integer> fSRegStack = new Stack<>();
    private Stack<Integer> fTRegStack = new Stack<>();
    private HashSet<ObjBlock> visited = new HashSet<>();
    private int maxS = 12;
    private int maxT = 9;
    private int maxF = 32;
    private int currentStackSize;

    public FastLinearScannerAlloc(ObjModule module) {
        this.module = module;
        this.nowIndex = 0;
    }

    public void reset(ObjFunction function) {
        this.nowIndex = 0;
        activeIntegerList = new ArrayList<>();
        activeFloatList = new ArrayList<>();
        for (int i = 0; i < 32; i++) {
            integerReg[i] = 0;
            floatReg[i] = 0;
        }
        regMap.clear();
        regMapInStack.clear();
        sRegStack.clear();
        tRegStack.clear();
        fSRegStack.clear();
        fTRegStack.clear();
        for (int i = 0; i < 32; i++) {
            if (isS(i)) {
                sRegStack.push(i);
            } else if (isT(i)) {
                tRegStack.push(i);
            }
            if (isFs(i)) {
                fSRegStack.push(i);
            } else if (isFt(i)) {
                fTRegStack.push(i);
            }
        }
        currentStackSize = function.getStackSize();
    }

    private void allocRegInStack(ObjOperand virReg) {
        if (virReg instanceof ObjVirReg virtual) {
            regMapInStack.put(virtual, currentStackSize);
            currentStackSize += 8;
        } else if (virReg instanceof ObjFVirReg fVirtual) {
            regMapInStack.put(fVirtual, currentStackSize);
            currentStackSize += 8;
        } else {
            throw new RuntimeException("Unexpected operand type");
        }
    }

    enum SLType {
        FStore, FLoad, Store, Load
    }

    private List<ObjInstr> spawnLoadStoreSp(SLType type, int offset, ObjOperand value) {
        if (-2048 <= offset && offset <= 2047) {
            // Operate immediately
            return List.of(switch (type) {
                case Load -> new ObjLoad("ld", value, ObjPhyReg.SP, new ObjImm(offset));
                case Store -> new ObjStore(ObjPhyReg.SP, value, new ObjImm(offset), "sd");
                case FLoad -> new ObjLoad("fld", value, ObjPhyReg.SP, new ObjImm(offset));
                case FStore -> new ObjStore(ObjPhyReg.SP, value, new ObjImm(offset), "fsd");
            });
        } else if (-4096 <= offset && offset <= 4094) {
            int halfOffset = offset / 2;
            int otherOffset = offset - halfOffset;
            int tmpRegIndex = tRegStack.pop();
            integerReg[tmpRegIndex] = 1;
            tRegStack.push(tmpRegIndex);
            return List.of(
                    new ObjBinary("add", "i32", ObjPhyReg.regs.get(tmpRegIndex), ObjPhyReg.SP, new ObjImm(halfOffset)),
                    switch (type) {
                        case Load -> new ObjLoad("ld", value, ObjPhyReg.regs.get(tmpRegIndex), new ObjImm(otherOffset));
                        case Store -> new ObjStore(ObjPhyReg.regs.get(tmpRegIndex), value, new ObjImm(otherOffset), "sd");
                        case FLoad -> new ObjLoad("fld", value,ObjPhyReg.regs.get(tmpRegIndex), new ObjImm(otherOffset));
                        case FStore -> new ObjStore(ObjPhyReg.regs.get(tmpRegIndex), value, new ObjImm(otherOffset), "fsd");
                    }
            );
        } else {
            int tmpRegIndex = tRegStack.pop();
            integerReg[tmpRegIndex] = 1;
            tRegStack.push(tmpRegIndex);
            return List.of(
                    new ObjMove(new ObjImm(offset), ObjPhyReg.regs.get(tmpRegIndex)),
                    new ObjBinary("add", "i32", ObjPhyReg.regs.get(tmpRegIndex), ObjPhyReg.regs.get(tmpRegIndex), ObjPhyReg.SP),
                    switch (type) {
                        case Load -> new ObjLoad("ld", value, ObjPhyReg.regs.get(tmpRegIndex), new ObjImm(0));
                        case Store -> new ObjStore(ObjPhyReg.regs.get(tmpRegIndex), value, new ObjImm(0), "sd");
                        case FLoad -> new ObjLoad("fld", value,ObjPhyReg.regs.get(tmpRegIndex), new ObjImm(0));
                        case FStore -> new ObjStore(ObjPhyReg.regs.get(tmpRegIndex), value, new ObjImm(0), "fsd");
                    }
            );
        }
    }

    private List<ObjInstr> spawnPhysicalInstr(ObjInstr instr) {
        List<ObjInstr> toInsert = new LinkedList<>();
        List<Integer> usedIntegerRegIndex = new LinkedList<>();
        List<Integer> usedFloatRegIndex = new LinkedList<>();
        for (var operand : new ArrayList<ObjOperand>() {{
            addAll(instr.regUse);
        }}) {
            if (operand instanceof ObjFPhyReg || operand instanceof ObjPhyReg) {
                continue;
            }
            if (regMap.containsKey(operand)) {
                // Use regs directly
                int regIndex = regMap.get(operand);
                if (operand instanceof ObjVirReg) {
                    instr.replaceReg(operand, ObjPhyReg.regs.get(regIndex));
                    integerReg[regIndex] = 1;
                } else if (operand instanceof ObjFVirReg) {
                    instr.replaceReg(operand, ObjFPhyReg.regs.get(regIndex));
                    floatReg[regIndex] = 1;
                }
            } else {
                // Load from the stack
                int stackOffset = regMapInStack.get(operand);
                if (operand instanceof ObjVirReg) {
                    int dstRegIndex = tRegStack.pop();
                    integerReg[dstRegIndex] = 1;
                    usedIntegerRegIndex.add(dstRegIndex);
                    toInsert.addAll(spawnLoadStoreSp(SLType.Load, stackOffset, ObjPhyReg.regs.get(dstRegIndex)));
                    instr.replaceReg(operand, ObjPhyReg.regs.get(dstRegIndex));
                } else if (operand instanceof ObjFVirReg) {
                    int dstRegIndex = fTRegStack.pop();
                    floatReg[dstRegIndex] = 1;
                    usedFloatRegIndex.add(dstRegIndex);
                    toInsert.addAll(spawnLoadStoreSp(SLType.FLoad, stackOffset, ObjFPhyReg.regs.get(dstRegIndex)));
                    instr.replaceReg(operand, ObjFPhyReg.regs.get(dstRegIndex));
                }
            }
        }
        toInsert.add(instr);
        for (var operand : new ArrayList<ObjOperand>() {{
            addAll(instr.regDef);
        }}) {
            if (operand instanceof ObjFPhyReg || operand instanceof ObjPhyReg) {
                continue;
            }
            if (regMap.containsKey(operand)) {
                // Use regs directly
                int regIndex = regMap.get(operand);
                if (operand instanceof ObjVirReg) {
                    instr.replaceReg(operand, ObjPhyReg.regs.get(regIndex));
                    integerReg[regIndex] = 1;
                } else if (operand instanceof ObjFVirReg) {
                    instr.replaceReg(operand, ObjFPhyReg.regs.get(regIndex));
                    floatReg[regIndex] = 1;
                }
            } else {
                // Load from the stack
                int stackOffset = regMapInStack.get(operand);
                if (operand instanceof ObjVirReg) {
                    int dstRegIndex = tRegStack.pop();
                    integerReg[dstRegIndex] = 1;
                    usedIntegerRegIndex.add(dstRegIndex);
                    toInsert.addAll(spawnLoadStoreSp(SLType.Store, stackOffset, ObjPhyReg.regs.get(dstRegIndex)));
                    instr.replaceReg(operand, ObjPhyReg.regs.get(dstRegIndex));
                } else if (operand instanceof ObjFVirReg) {
                    int dstRegIndex = fTRegStack.pop();
                    floatReg[dstRegIndex] = 1;
                    usedFloatRegIndex.add(dstRegIndex);
                    toInsert.addAll(spawnLoadStoreSp(SLType.FStore, stackOffset, ObjFPhyReg.regs.get(dstRegIndex)));
                    instr.replaceReg(operand, ObjFPhyReg.regs.get(dstRegIndex));
                }
            }
        }
        for (int i : usedIntegerRegIndex) {
            tRegStack.push(i);
        }
        for (int i : usedFloatRegIndex) {
            fTRegStack.push(i);
        }
        return toInsert;
    }

    public void run() {
        for (ObjFunction function: module.getFunctions()) {
            reset(function);
            getInstrIndex(function);
            HashSet<ObjOperand> regSet = livingAnalysis(function);
            getSortedVirReg(regSet);
            distributeReg();
            convertInstruction(function);
            int baseStackSize = currentStackSize;
            contextSave(function);
            contextRestore(function,baseStackSize);
        }
    }

    public void convertInstruction(ObjFunction function) {
        for (var block: function.getBlocks()) {
            LinkedList<ObjInstr> newInstrs = new LinkedList<>();
            for (var instr: block.getInstrs()) {
                LinkedList<ObjInstr> physicalInstrs = (LinkedList<ObjInstr>) spawnPhysicalInstr(instr);
                newInstrs.addAll(physicalInstrs);
            }
            block.resetInstrs(newInstrs);
        }
    }

    private void getInstrIndex(ObjFunction function) {
        this.nowIndex = 0;
        distributeInstrIndex(function.getFirstBlock());
        distributeInstrIndex(function.getBlocks().get(1));
    }

    private void distributeInstrIndex(ObjBlock block) {
        if (visited.contains(block)) {
            return;
        }
        visited.add(block);
        // 深度优先搜索，给块中的每个指令分配一个序号
        for (ObjInstr instr: block.getInstrs()) {
            instr.index = this.nowIndex++;
        }
        for (ObjBlock nextBlock: block.getNextBlocks()) {
            distributeInstrIndex(nextBlock);
        }
    }

    private HashSet<ObjOperand> livingAnalysis(ObjFunction function) {
        HashSet<ObjOperand> regSet = new HashSet<>();
        BLOCK: for (var block : function.getBlocks()) {
            for (var instr : block.getInstrs()) {
                if (instr.index == -1) {
                    continue BLOCK;
                }
                for (var operand : new LinkedList<ObjOperand>() {{
                    addAll(instr.regDef);
                    addAll(instr.regUse);
                }}) {
                    operand.updateBirth(instr.index);
                    operand.updateDeath(instr.index);
                    if (operand instanceof ObjVirReg || operand instanceof ObjFVirReg) {
                        regSet.add(operand);
                    }
                }
            }
        }
        for (var block : function.getBlocks()) {
            int start = block.getInstrs().getFirst().index;
            int end = block.getInstrs().getLast().index;
            for (var instr : block.getInstrs()) {
                for (var operand : new LinkedList<ObjOperand>() {{
                    addAll(instr.regDef);
                    addAll(instr.regUse);
                }}) {
                    operand.checkLivingRange(start, end);
                }
            }
        }
        return regSet;
    }

    private void getSortedVirReg(HashSet<ObjOperand> set) {
        // 对虚拟寄存器按照出现顺序排序，结束顺序做第二关键词
        this.virRegList = set.stream().sorted(
                Comparator.comparingInt(ObjOperand::getBirth).thenComparingInt(ObjOperand::getDeath)
        ).collect(Collectors.toCollection(ArrayList::new));
    }

    private void distributeReg() {
        for (var operand: virRegList) {
            allocRegInStack(operand);
//            if (operand instanceof ObjVirReg) {
//                if (sRegStack.empty()) {
//                    int flag = 0;
//                    for (var active : activeIntegerList) {
//                        if (active.getDeath() < operand.getBirth()) {
//                            int index = regMap.get(active);
//                            regMap.put(operand, index);
//                            activeIntegerList.remove(active);
//                            flag = 1;
//                            break;
//                        }
//                    }
//                    if (flag == 0) {
//                        allocRegInStack(operand);
//                    }
//                }
//                else {
//                    int index = sRegStack.pop();
//                    regMap.put(operand, index);
//                    integerReg[index] = 1;
//                    activeIntegerList.add(operand);
//                }
//            } else if (operand instanceof ObjFVirReg) {
//                if (fSRegStack.empty()) {
//                    int flag = 0;
//                    for (var active : activeFloatList) {
//                        if (active.getDeath() < operand.getBirth()) {
//                            int index = regMap.get(active);
//                            regMap.put(operand, index);
//                            activeFloatList.remove(active);
//                            flag = 1;
//                            break;
//                        }
//                    }
//                    if (flag == 0) {
//                        allocRegInStack(operand);
//                    }
//                }
//                else {
//                    int index = fSRegStack.pop();
//                    regMap.put(operand, index);
//                    floatReg[index] = 1;
//                    activeFloatList.add(operand);
//                }
//            }
        }
    }

    private void contextSave(ObjFunction function) {
        // save s register which is used by this function
        LinkedList<ObjInstr> instrs = new LinkedList<>();
        for (int i = 0; i < 32; i++) {
            if (isS(i) && integerReg[i] == 1) {
                instrs.addAll(spawnLoadStoreSp(SLType.Store, currentStackSize, ObjPhyReg.regs.get(i)));
                currentStackSize += 8;
            }
            if (isFs(i) && floatReg[i] == 1) {
                instrs.addAll(spawnLoadStoreSp(SLType.FStore, currentStackSize, ObjFPhyReg.regs.get(i)));
                currentStackSize += 8;
            }
        }
        LinkedList<ObjInstr> tempInstrs = new LinkedList<>();
        for (var instr: function.getFirstBlock().getInstrs()) {
            tempInstrs.add(instr);
            if (instr.equals(function.getAllocStack())) {
                tempInstrs.addAll(instrs);
            }
        }
        function.getFirstBlock().resetInstrs(tempInstrs);
    }

    private void contextRestore(ObjFunction function,int baseStackSize) {
        // restore s register which is used by this function
        LinkedList<ObjInstr> instrs = new LinkedList<>();
        for (int i = 0; i < 32; i++) {
            if (isS(i) && integerReg[i] == 1) {
                instrs.addAll(spawnLoadStoreSp(SLType.Load, baseStackSize, ObjPhyReg.regs.get(i)));
                baseStackSize += 8;
            }
            if (isFs(i) && floatReg[i] == 1) {
                instrs.addAll(spawnLoadStoreSp(SLType.FLoad, baseStackSize, ObjFPhyReg.regs.get(i)));
                baseStackSize += 8;
            }
        }
        function.resetArgsLoadTarget(currentStackSize);
        if (currentStackSize<=2047)
            function.resetStackSize(new ObjImm(currentStackSize),new ObjImm(-currentStackSize));
        else {
            int tmpRegIndex = tRegStack.pop();
            integerReg[tmpRegIndex] = 1;
            instrs.add(new ObjMove(new ObjImm(currentStackSize), ObjPhyReg.regs.get(tmpRegIndex)));
            // 插入头一个指令
            int tmpRegIndex2 = tRegStack.pop();
            function.getFirstBlock().getInstrs().addFirst(new ObjMove(new ObjImm(-currentStackSize), ObjPhyReg.regs.get(tmpRegIndex2)));
            integerReg[tmpRegIndex2] = 1;
            function.resetStackSize(ObjPhyReg.regs.get(tmpRegIndex),ObjPhyReg.regs.get(tmpRegIndex2));
            tRegStack.push(tmpRegIndex);
            tRegStack.push(tmpRegIndex2);
        }
        for (var block: function.getExits()) {
            ObjInstr spRestoreInstr = function.getFreeStack(block);
            LinkedList<ObjInstr> restoreInstrs = new LinkedList<>();
            for (var instr: block.getInstrs()) {
                if (instr.equals(spRestoreInstr)) {
                    restoreInstrs.addAll(instrs);
                }
                restoreInstrs.add(instr);
            }
            block.resetInstrs(restoreInstrs);
        }
    }

    private boolean isS(int i) {
        return i == 8 || i == 9 || i >= 18 && i <= 27;
    }

    private boolean isT(int i) {
        return i >= 5 && i <= 7 || i >= 28 && i <= 31;
    }

    private boolean isFt(int i) {
        return i >= 0 && i <= 9 || i == 18;
    }

    private boolean isFs(int i) {
        return i >= 18 && i <= 31;
    }

}
