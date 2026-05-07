package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.AsmBasicBlock;
import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmJump;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.ComparablePair;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;

import java.util.*;

/**
 * 生命周期控制器，
 */
public class LifeTimeController {
    //虚拟寄存器生命周期的设置过程
    private final AsmFunction function;
    private final Map<Register, ComparablePair<LifeTimeIndex, LifeTimeIndex>> lifeTimeRange = new HashMap<>();
    private final Map<Register, List<LifeTimeInterval>> lifeTimeIntervals = new HashMap<>();
    private final Map<Register, List<LifeTimePoint>> lifeTimePoints = new HashMap<>();
    private final Map<AsmInstruction, Integer> instIDMap = new HashMap<>();
    private List<AsmInstruction> idSourceInst;

    public Register getVReg(int index) {
        return function.getRegisterAllocator().get(index);
    }

    public LifeTimeController(AsmFunction function) {
        this.function = function;
    }

    private void init() {
        lifeTimeRange.clear();
        lifeTimeIntervals.clear();
        lifeTimePoints.clear();
        blocks.clear();
        blockMap.clear();
        instIDMap.clear();
    }

    public void buildInstID(List<AsmInstruction> instructionList) {
        idSourceInst = instructionList;
        instIDMap.clear();
        for (int i = 0; i < instructionList.size(); i++) {
            instIDMap.put(instructionList.get(i), i);
        }
    }

    public int getInstID(AsmInstruction inst) {
        return instIDMap.get(inst);
    }

    public boolean containsInst(AsmInstruction inst) {
        return instIDMap.containsKey(inst);
    }

    public void replaceInst(AsmInstruction replacedInst, AsmInstruction valueInst) {
        var id = instIDMap.get(replacedInst);
        instIDMap.remove(replacedInst);
        instIDMap.put(valueInst, id);
    }

    public Set<Integer> getVRegKeySet() {
        Set<Integer> res = new HashSet<>();
        for (var x : lifeTimePoints.keySet()) {
            if (x.isVirtual()) {
                res.add(x.getAbsoluteIndex());
            }
        }
        return res;
    }

    public Set<Register> getRegKeySet() {
        return lifeTimePoints.keySet();
    }

    public ComparablePair<LifeTimeIndex, LifeTimeIndex> getLifeTimeRange(Integer index) {
        return lifeTimeRange.get(getVReg(index));
    }

    public ComparablePair<LifeTimeIndex, LifeTimeIndex> getLifeTimeRange(Register reg) {
        return lifeTimeRange.get(reg);
    }

    private static class Block {
        String blockName;
        int l, r, startBranch;
        Set<Register> in = new HashSet<>();
        Set<Register> out = new HashSet<>();
        Set<Register> def = new HashSet<>();
        Set<String> nextBlockName = new HashSet<>();
        Block(AsmLabel label) {
            blockName = label.getLabel().getLabelName();
            startBranch = -1;
        }
    }

    private Set<Register> difference(Set<Register> a, Set<Register> b) {
        Set<Register> result = new HashSet<>();
        for (var x : a) {
            if (!b.contains(x)) {
                result.add(x);
            }
        }
        return result;
    }

    public void insertLifeTimePoint(Register reg, LifeTimePoint p) {
        if (!lifeTimePoints.containsKey(reg)) {
            lifeTimePoints.put(reg, new ArrayList<>());
        }
        lifeTimePoints.get(reg).add(p);
    }

    public void removeLifeTimePoint(Register reg, LifeTimePoint p) {
        lifeTimePoints.get(reg).remove(p);
    }

    public void removeReg(Register reg) {
        lifeTimePoints.remove(reg);
        lifeTimeRange.remove(reg);
        lifeTimeIntervals.remove(reg);
    }

    /**
     * 把y的引用点集合合并入x
     * 这里进行的是直接合并，生命周期还需要使用constructInterval函数进行重构
     * @param x 第一个虚拟寄存器的下标
     * @param y 第二个虚拟寄存器的下标
     */
    public void mergePoints(Register x, Register y) {
        if (x == y || !lifeTimePoints.containsKey(y)) {
            return;
        }
        if (lifeTimePoints.containsKey(x)) {
            lifeTimePoints.get(x).addAll(lifeTimePoints.get(y));
            lifeTimePoints.get(x).sort(Comparator.comparing(LifeTimePoint::getIndex));
        } else {
            lifeTimePoints.put(x, lifeTimePoints.get(y));
        }
        removeReg(y);
    }

    public void mergePoints(int x, int y) {
        mergePoints(getVReg(x), getVReg(y));
    }

    public boolean constructInterval(Register x) {
        if (!lifeTimePoints.containsKey(x)) {
            throw new IllegalArgumentException();
        }
        lifeTimePoints.get(x).removeIf((point) -> {
            if (!instIDMap.containsKey(point.getIndex().getSourceInst())) {
                return true;
            }
            var instr = point.getIndex().getSourceInst();
            if (instr instanceof AsmLabel || instr instanceof AsmJump) {
                return false;
            }
            return !AsmInstructions.instContainsReg(instr, x);
        });
        if (lifeTimePoints.get(x).isEmpty()) {
            return false;
        }
        lifeTimePoints.get(x).sort(Comparator.comparing(LifeTimePoint::getIndex));
        var points = lifeTimePoints.get(x);
        lifeTimeRange.put(x, new ComparablePair<>(points.get(0).getIndex(), points.get(points.size() - 1).getIndex()));
        List<LifeTimeInterval> intervals = new ArrayList<>();
        for (var p : points) {
            if (p.isDef()) {
                intervals.add(new LifeTimeInterval(x, new ComparablePair<>(p.getIndex(), null)));
            }
            if (intervals.size() == 0) {
                throw new RuntimeException("F!");
            }
            intervals.get(intervals.size() - 1).range.b = p.getIndex();
        }
        lifeTimeIntervals.put(x, intervals);
        return true;
    }

    public List<LifeTimeInterval> getInterval(int x) {
        return lifeTimeIntervals.get(getVReg(x));
    }

    public List<LifeTimeInterval> getInterval(Register reg) {
        return lifeTimeIntervals.get(reg);
    }

    public List<LifeTimePoint> getPoints(int x) {
        return lifeTimePoints.get(getVReg(x));
    }

    public List<LifeTimePoint> getPoints(Register reg) {
        return lifeTimePoints.get(reg);
    }

    private final List<Block> blocks = new ArrayList<>();
    private final Map<String, Block> blockMap = new HashMap<>();
    void buildBlocks(List<AsmInstruction> instructionList) {
        Block now = null;
        for (int i = 0; i < instructionList.size(); i++) {
            var inst = instructionList.get(i);
            if (inst instanceof AsmLabel label) {
                now = new Block(label);
                now.l = i;
                blocks.add(now);
                blockMap.put(now.blockName, now);
            }
            if (now == null) {
                continue;
            }
            if (inst instanceof AsmJump) {
                if (now.startBranch == -1) {
                    now.startBranch = i;
                }
                for (int j = 1; j <= 3; j++) {
                    if (inst.getOperand(j) instanceof Label label) {
                        now.nextBlockName.add(label.getLabelName());
                    }
                }
            }
            for (var reg : AsmInstructions.getReadRegSet(inst)) {
                if (!now.def.contains(reg)) {
                    now.in.add(reg);
                }
            }
            now.def.addAll(AsmInstructions.getWriteRegSet(inst));
            blocks.get(blocks.size() - 1).r = i;
        }
        if (function.getReturnRegister() != null) {
            blocks.get(blocks.size() - 1).in.add(function.getReturnRegister());
        }
    }
    private void iterateActiveReg() {
        while (true) {
            boolean changeLabel = false;
            for (int i = 0; i < blocks.size() - 1; i++) {
                var b = blocks.get(i);
                for (var nextName : b.nextBlockName) {
                    var next = blockMap.get(nextName);
                    if (next != null) {
                        changeLabel |= b.out.addAll(next.in);
                    }
                }
                changeLabel |= b.in.addAll(difference(b.out, b.def));
            }
            if (!changeLabel) {
                break;
            }
        }
    }
    private void buildLifeTimePoints(List<AsmInstruction> instructionList) {
        LifeTimeIndex functionStartIndex = LifeTimeIndex.getInstOut(this, instructionList.get(0));
        for (var reg : function.getParamRegs()) {
            insertLifeTimePoint(reg, LifeTimePoint.getDef(functionStartIndex));
        }
        for (int c = 0; c < blocks.size() - 1; c++) {
            var b = blocks.get(c);
            for (var x : b.in) {
                LifeTimeIndex index = LifeTimeIndex.getInstOut(this, instructionList.get(b.l));
                insertLifeTimePoint(x, LifeTimePoint.getDef(index));
            }
            for (int i = b.l; i <= b.r; i++) {
                var inst = instructionList.get(i);
                for (var x : AsmInstructions.getReadRegSet(inst)) {
                    LifeTimeIndex index = LifeTimeIndex.getInstIn(this, instructionList.get(i));
                    insertLifeTimePoint(x, LifeTimePoint.getUse(index));
                }
                for (var x : AsmInstructions.getWriteRegSet(inst)) {
                    LifeTimeIndex index = LifeTimeIndex.getInstOut(this, instructionList.get(i));
                    insertLifeTimePoint(x, LifeTimePoint.getDef(index));
                }
            }
            for (var x : b.out) {
                LifeTimeIndex index = LifeTimeIndex.getInstIn(this, instructionList.get(b.startBranch));
                insertLifeTimePoint(x, LifeTimePoint.getUse(index));
            }
        }
        for (var constantReg : Registers.CONSTANT_REGISTERS) {
            removeReg(constantReg);
        }
    }
    private void buildLifeTimeMessage(List<AsmInstruction> instructionList) {
        buildLifeTimePoints(instructionList);
        List<Register> removeList = new ArrayList<>();
        for (var x : getRegKeySet()) {
            if (!constructInterval(x)) {
                removeList.add(x);
            }
        }
        for (var x : removeList) {
            removeReg(x);
        }
    }

    public void getAllRegLifeTime(List<AsmInstruction> instructionList) {
        init();
        buildInstID(instructionList);
        buildBlocks(instructionList);
        iterateActiveReg();
        buildLifeTimeMessage(instructionList);
    }

    /**
     * 在寄存器引用点信息保持维护的情况下重新构建每个寄存器的生存区间集合，用于指令删除后重新构建
     * 也可在维护好添加的指令中变量引用点的情况下进行维护
     * @param instructionList 新指令列表
     */
    public void rebuildLifeTimeInterval(List<AsmInstruction> instructionList) {
        buildInstID(instructionList);

        List<Register> removeList = new ArrayList<>();
        for (var x : getRegKeySet()) {
            if (!constructInterval(x)) {
                removeList.add(x);
            }
        }
        for (var x : removeList) {
            removeReg(x);
        }
    }
}
