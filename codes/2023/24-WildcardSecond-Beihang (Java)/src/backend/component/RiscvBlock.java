package backend.component;

import backend.operand.RiscvLabel;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import ir.instr.Instr;
import ir.pass.analyze.Loop;
import ir.value.BasicBlock;
import tools.DoubleLinkedList;
import tools.DoubleNode;

import java.awt.*;
import java.util.ArrayList;

public class RiscvBlock extends RiscvLabel {
    private final DoubleLinkedList<RiscvInstr> riscvInstrs = new DoubleLinkedList<>();

    private final ArrayList<RiscvBlock> preds = new ArrayList<>();
    private final ArrayList<RiscvBlock> succs = new ArrayList<>();

    public RiscvBlock(String name, Loop loop) {
        super(name);
        riscvInstrs.loop = loop;
    }

    public void addInstr(DoubleNode<RiscvInstr> riscvInsNode, int loopdepth) {
        //TODO: 后续可能需要针对加入指令情况 补充preds succs关系
        for(RiscvOperand operand: riscvInsNode.getContent().getOperands()) {
            operand.beUsed(riscvInsNode);
        }
        if(riscvInsNode.getContent().getDefReg()!=null){
            riscvInsNode.getContent().getDefReg().beUsed(riscvInsNode);
        }
        riscvInstrs.insert(riscvInsNode);
    }

    public String toPrint() {
        StringBuilder sb = new StringBuilder();
        sb.append(super.toPrint());
        for(DoubleNode<RiscvInstr> node = riscvInstrs.getHead();
            node != null; node = node.getSucc()) {
            RiscvInstr ins = node.getContent();
            sb.append("\t" + ins.toString() + "\n");
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        return name;
    }

    public void addPreds(RiscvBlock block) {
        preds.add(block);
    }

    public void addSuccs(RiscvBlock block) {
        succs.add(block);
    }

    public ArrayList<RiscvBlock> getPreds() {
        return preds;
    }

    public ArrayList<RiscvBlock> getSuccs() {
        return succs;
    }

    public DoubleLinkedList<RiscvInstr> getRiscvInstrs() {
        return riscvInstrs;
    }
}
