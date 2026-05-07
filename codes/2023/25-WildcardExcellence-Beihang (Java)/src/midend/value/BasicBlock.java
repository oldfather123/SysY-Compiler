package midend.value;

import midend.Type;
import midend.value.instrs.Branch;
import midend.value.instrs.Instr;
import midend.value.instrs.Jump;
import midend.value.instrs.Return;
import midpass.TSValue;
import util.nodelist.NodeList;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public class BasicBlock extends Value {
    private NodeList<Instr> instrList;
    private Function parent;
    private ArrayList<BasicBlock> sucBB;
    private ArrayList<BasicBlock> prevBB;
    private HashSet<BasicBlock> domBB;
    private HashSet<BasicBlock> idomBB;
    private HashSet<BasicBlock> dfBB;
    private int LayerInDomTree;
    private BasicBlock fatherDominator;
    private TSValue tsValue;

    // 活跃变量分析
    public Set<Value> useRegs, defRegs, inRegs, outRegs;

    public void removeSucBlock(BasicBlock block) {
        this.sucBB.remove(block);
    }

    public void changeBlockAtoB(BasicBlock A, BasicBlock B) {
        Instr instr = this.getLastInstr();
        if (instr instanceof Branch) {
            int i = instr.getOperandList().indexOf(A);
            instr.changeUseIn(B, i);
        } else {
            instr.changeUseIn(B, 0);
        }
    }

    public TSValue getTsValue() {
        return tsValue;
    }
    
    public void setTsValue(TSValue tsValue) {
        this.tsValue = tsValue;
    }
    public BasicBlock getFatherDominator() {
        return fatherDominator;
    }

    public void setFatherDominator(BasicBlock fatherDominator) {
        this.fatherDominator = fatherDominator;
    }

    public HashSet<BasicBlock> getDfBB() {
        return dfBB;
    }

    public void setDfBB(HashSet<BasicBlock> dfBB) {
        this.dfBB = dfBB;
    }

    public HashSet<BasicBlock> getIdomBB() {
        return idomBB;
    }

    public void setLayerInDomTree(int layerInDomTree) {
        LayerInDomTree = layerInDomTree;
    }

    public void setIdomBB(HashSet<BasicBlock> idomBB) {
        this.idomBB = idomBB;
    }

    public HashSet<BasicBlock> getDomBB() {
        return domBB;
    }

    public void setDomBB(HashSet<BasicBlock> domBB) {
        this.domBB = domBB;
    }

    public ArrayList<BasicBlock> getSucBB() {
        return sucBB;
    }

    public void setSucBB(ArrayList<BasicBlock> sucBB) {
        this.sucBB = sucBB;
    }

    public ArrayList<BasicBlock> getPrevBB() {
        return prevBB;
    }

    public void setPrevBB(ArrayList<BasicBlock> prevBB) {
        this.prevBB = prevBB;
    }

    public NodeList<Instr> getInstrList() {
        return instrList;
    }

    public void setInstrList(NodeList<Instr> instrList) {
        this.instrList = instrList;
    }

    private boolean isTerminate;

    public Function getParent() {
        return parent;
    }

    public void setParent(Function parent) {
        this.parent = parent;
    }

    public BasicBlock(Type type, String name, Function parent) {
        super(type, name);
        this.instrList = new NodeList<>();
        this.parent = parent;
        this.isTerminate = false;
        this.parent.addBlock(this);
    }
    
    public BasicBlock(Function parent) {
        super(null, null);
        this.instrList = new NodeList<>();
        this.parent = parent;
        this.prevBB = new ArrayList<>();
        this.sucBB = new ArrayList<>();
        this.isTerminate = false;
        this.parent.addBlock(this);
        setName("Block" + BLOCK_NUM++);
    }

    public void addInstr(Instr instr) {
        if (isTerminate) {
            return;
        }
        this.instrList.addLast(instr);
        if (instr instanceof Branch || instr instanceof Jump || instr instanceof Return) {
            isTerminate = true;
        }
    }

    public Instr getLastInstr() {
        return this.instrList.lastElement();
    }

    public void remove() {
        this.parent.getBasicBlocks().removeIf(it -> it == this);
        removeAllInstr();
    }

    public void removeAllInstr() {
        for (var instr : instrList) {
            instr.remove();
            instr.get().removeUse();
        }
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        boolean changeLine = false;
        if (!isTerminate) {
            addInstr(new Return(null, this));
        }
        sb.append(getName()).append(":\n");
        for (var instr : instrList) {
            if (changeLine) {
                sb.append("\n");
            }
            sb.append("\t");
            sb.append(instr.get());
            changeLine = true;
        }
        sb.append("\n");
        return sb.toString();
    }

    public int getLayerInDomTree() {
        return LayerInDomTree;
    }
}
