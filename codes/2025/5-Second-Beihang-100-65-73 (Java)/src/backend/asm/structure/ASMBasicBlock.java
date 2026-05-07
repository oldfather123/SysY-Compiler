package backend.asm.structure;

import backend.asm.ASMValue;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.tags.BranchIns;
import backend.asm.instr.tags.JumpIns;
import backend.asm.register.vir.VirReg;
import util.CustomList;

import java.util.LinkedHashSet;
import java.util.Set;

public class ASMBasicBlock extends CustomList.Node {
    private final ASMFunction parentFunction;
    private final CustomList instructionList;
    private final String name;
    private final int loopDepth;
    private final Set<VirReg> defSet;  // 为了寄存器分配进行分析，因此只考虑待分配的虚拟寄存器
    private final Set<VirReg> useSet;
    private final Set<ASMBasicBlock> preSet;
    private final Set<ASMBasicBlock> sucSet;
    private final Set<VirReg> liveInSet;
    private final Set<VirReg> liveOutSet;
    
    public ASMBasicBlock(ASMFunction parentFunction, int loopDepth, String name) {
        this.parentFunction = parentFunction;
        this.loopDepth = loopDepth;
        this.instructionList = new CustomList(this);
        this.name = name;
        this.defSet = new LinkedHashSet<>();
        this.useSet = new LinkedHashSet<>();
        this.preSet = new LinkedHashSet<>();
        this.sucSet = new LinkedHashSet<>();
        this.liveInSet = new LinkedHashSet<>();
        this.liveOutSet = new LinkedHashSet<>();
        
        parentFunction.insertBlock2Tail(this);
    }
    
    public ASMFunction getParentFunction() {
        return parentFunction;
    }
    
    public void insertInstr2Tail(ASMInstruction instr) {
        this.instructionList.addToTail(instr);
    }
    
    public CustomList getInstructionList() {
        return instructionList;
    }
    
    /**
     * 为图着色做准备，做数据流分析，这里只考虑待分配寄存器的指令
     */
    public void initDefUse() {
        this.defSet.clear();
        this.useSet.clear();
        for (CustomList.Node node : this.instructionList) {
            ASMInstruction ins = (ASMInstruction) node;
            if (ins.getRegister() instanceof VirReg virReg && !this.useSet.contains(virReg)) {
                this.defSet.add(virReg);
            }
            for (ASMValue value : ins.getUsedValList()) {
                if (value instanceof VirReg virReg && !this.defSet.contains(virReg)) {
                    this.useSet.add(virReg);
                }
            }
        }
    }
    
    /**
     * 为图着色做准备，做控制流分析，建立基本块之间的前驱后继关系
     */
    public void initPreSuc() {
        this.preSet.clear();
        this.sucSet.clear();
        for (CustomList.Node insNode : instructionList) {
            if (insNode instanceof JumpIns jIns) {
                ASMBasicBlock tar = jIns.getTarget();
                tar.preSet.add(this);
                this.sucSet.add(tar);
            }
            if (insNode instanceof BranchIns brIns) {
                ASMBasicBlock tar = brIns.getTarget();
                tar.preSet.add(this);
                this.sucSet.add(tar);
            }
        }
    }
    
    public void initLiveIO() {
        this.liveInSet.clear();
        this.liveOutSet.clear();
        this.liveInSet.addAll(this.useSet);
    }
    
    /**
     * 更新 LiveIn 和 LiveOut
     * @return 如果有修改返回 true 否则返回 false
     */
    public boolean renewLiveIO() {
        boolean modifiedLiveOut = false;
        for (ASMBasicBlock suc : this.sucSet) {
            if (!this.liveOutSet.containsAll(suc.liveInSet)) {
                modifiedLiveOut = true;
                this.liveOutSet.addAll(suc.liveInSet);
            }
        }
        if (modifiedLiveOut) {
            for (VirReg ins : this.liveOutSet) {
                if (!this.defSet.contains(ins)) {
                    this.liveInSet.add(ins);
                }
            }
            return true;
        }
        return false;
    }
    
    public Set<VirReg> getLiveOutSet() {
        return liveOutSet;
    }
    
    @Override
    public String toString() {
        return this.name;
    }
    
    public String printAsOperand() {
        return this.toString();
    }
    
    public int getLoopDepth() {
        return loopDepth;
    }
}
