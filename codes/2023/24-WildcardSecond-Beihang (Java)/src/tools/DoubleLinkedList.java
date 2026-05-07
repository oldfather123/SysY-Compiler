package tools;

import backend.component.RiscvInstr;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import ir.instr.Instr;
import ir.pass.analyze.Loop;

public class DoubleLinkedList<T> {
    private DoubleNode<T> head;
    private DoubleNode<T> tail;
    private int numOfNode;
    public int loopdepth;
    public Loop loop;

    public DoubleLinkedList() {
        head = null;
        tail = null;
        numOfNode = 0;
    }

    public DoubleNode<T> getHead() {
        return head;
    }

    public void setHead(DoubleNode<T> head) {
        this.head = head;
    }

    public DoubleNode<T> getTail() {
        return tail;
    }

    public void setTail(DoubleNode<T> tail) {
        this.tail = tail;
    }

    private void updateBeUsed(DoubleNode<RiscvInstr> instrnode) {
        for (RiscvOperand operand : instrnode.getContent().getOperands()) {
            operand.beUsed(instrnode);
            if (operand instanceof RiscvReg) {
                ((RiscvReg) operand).addDefOrUse(loopdepth, loop);
            }
        }
        if (instrnode.getContent().getDefReg() != null) {
            instrnode.getContent().getDefReg().beUsed(instrnode);
            var operand = instrnode.getContent().getDefReg();
            if (operand != null) {
                operand.addDefOrUse(loopdepth, loop);
            }
        }
    }

    public void insert(DoubleNode<T> target) {
        if (head == null) {
            head = target;
            target.setPred(null);
        }
        target.setPred(tail);
        target.setSucc(null);
        if (tail == null) {
            tail = target;
        } else {
            tail.setSucc(target);
            tail = target;
        }
        numOfNode++;
        target.setParentList(this);
        updateBeUsed((DoubleNode<RiscvInstr>) target);
    }

    public void insertAfterNode(DoubleNode<T> target, DoubleNode<T> src) {
        numOfNode++;
        src.setSucc(target.getSucc());
        if (target.getSucc() != null) {
            target.getSucc().setPred(src);
        }
        target.setSucc(src);
        src.setPred(target);
        src.setParentList(this);
        updateBeUsed((DoubleNode<RiscvInstr>) src);
    }

    public void insertBeforeNode(DoubleNode<T> target, DoubleNode<T> src) {
        numOfNode++;
        src.setPred(target.getPred());
        src.setSucc(target);
        if (target.getPred() != null) {
            target.getPred().setSucc(src);
        } else {
            this.head = src;
        }
        target.setPred(src);
        src.setParentList(this);
        updateBeUsed((DoubleNode<RiscvInstr>) src);
    }

    public void removeNode(DoubleNode<T> target) {
        numOfNode--;
        if (head == target) {
            head = target.getSucc();
        }
        if (tail == target) {
            tail = target.getPred();
        }
        if (target.getPred() != null) {
            target.getPred().setSucc(target.getSucc());
        }
        if (target.getSucc() != null) {
            target.getSucc().setPred(target.getPred());
        }
    }

    public boolean isEmpty() {
        return numOfNode == 0;
    }

    public void addListBefore(DoubleLinkedList<T> suclist) {
        if (this.tail == null) {
            assert isEmpty();
            this.head = suclist.getHead();
        } else {
            this.tail.setSucc(suclist.getHead());
        }
        numOfNode += suclist.numOfNode;
        suclist.getHead().setPred(this.tail);
        for (DoubleNode<T> sucnode = suclist.getHead(); sucnode != null; sucnode = sucnode.getSucc()) {
            sucnode.setParentList(this);
        }
        this.tail = suclist.tail;
    }
}