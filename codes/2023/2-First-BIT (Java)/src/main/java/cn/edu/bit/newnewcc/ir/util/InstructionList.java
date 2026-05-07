package cn.edu.bit.newnewcc.ir.util;

import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.exception.UsageRelationshipCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.TerminateInst;

import java.util.Iterator;

/// 基本块与指令的关系
public class InstructionList implements Iterable<Instruction> {

    public static class Node {
        private Node prev, next;
        private final Instruction instruction;

        private InstructionList list;

        private Node() {
            this.instruction = null;
        }

        public Node(Instruction instruction) {
            this.instruction = instruction;
        }

        public BasicBlock getBasicBlock() {
            if (list == null) return null;
            return list.getBasicBlock();
        }

    }

    public static class InstructionIterator implements Iterator<Instruction> {

        private Node node;
        private final Node ending;

        private InstructionIterator(Node node, Node ending) {
            this.node = node;
            this.ending = ending;
            if (!isValidNode()) moveToNextValidNode();
        }

        private boolean isValidNode() {
            return node == null || node.instruction != null;
        }

        private void moveToNextValidNode() {
            do {
                node = node.next;
                if (node == ending) node = null;
            } while (!isValidNode());
        }

        @Override
        public boolean hasNext() {
            return node != null;
        }

        @Override
        public Instruction next() {
            var result = node.instruction;
            moveToNextValidNode();
            return result;
        }
    }

    @Override
    public Iterator<Instruction> iterator() {
        return new InstructionIterator(head, tail);
    }

    private final BasicBlock basicBlock;

    /**
     * 链表中用于定位的节点
     */
    private final Node head, leadingEnd, mainEnd, tail;
    private Node terminateInstructionNode;

    public InstructionList(BasicBlock basicBlock) {
        this.basicBlock = basicBlock;
        this.head = new Node();
        this.leadingEnd = new Node();
        this.mainEnd = new Node();
        this.terminateInstructionNode = new Node();
        this.tail = new Node();
        this.head.list = this;
        insertAlphaAfterBeta(leadingEnd, head);
        insertAlphaAfterBeta(mainEnd, leadingEnd);
        insertAlphaAfterBeta(terminateInstructionNode, mainEnd);
        insertAlphaAfterBeta(tail, terminateInstructionNode);
    }

    public BasicBlock getBasicBlock() {
        return basicBlock;
    }

    public void addLeadingInst(Instruction instruction) {
        insertAlphaBeforeBeta(instruction.__getInstructionListNode__(), leadingEnd);
    }

    public Iterator<Instruction> getLeadingInstructions() {
        return new InstructionIterator(head, leadingEnd);
    }

    public void appendMainInstruction(Instruction instruction) {
        insertAlphaBeforeBeta(instruction.__getInstructionListNode__(), mainEnd);
    }

    public void appendMainInstructionAtBeginning(Instruction instruction) {
        insertAlphaAfterBeta(instruction.__getInstructionListNode__(), leadingEnd);
    }

    public Iterator<Instruction> getMainInstructions() {
        return new InstructionIterator(leadingEnd, mainEnd);
    }

    public void setTerminateInstruction(TerminateInst instruction) {
        removeNodeFromList(this.terminateInstructionNode);
        this.terminateInstructionNode = (instruction == null ? new Node() : instruction.__getInstructionListNode__());
        insertAlphaAfterBeta(terminateInstructionNode, mainEnd);
    }

    public TerminateInst getTerminateInstruction() {
        if (terminateInstructionNode.instruction == null) {
            return null;
        } else {
            return (TerminateInst) terminateInstructionNode.instruction;
        }
    }

    /**
     * 将节点甲插入到节点乙之前
     *
     * @param alpha 节点甲
     * @param beta  节点乙
     */
    // 关于alpha和beta：
    // 本来想叫A和B的
    // 但是由于A和B在小驼峰中不美观，就改成了alpha和beta
    public static void insertAlphaBeforeBeta(Node alpha, Node beta) {
        assertNodeFree(alpha);
        alpha.prev = beta.prev;
        if (beta.prev != null) // 事实上为null的情况不会发生，因为设置了哨兵节点
            beta.prev.next = alpha;
        alpha.next = beta;
        beta.prev = alpha;
        alpha.list = beta.list;
    }

    /**
     * 将节点甲插入到节点乙之后
     *
     * @param alpha 节点甲
     * @param beta  节点乙
     */
    public static void insertAlphaAfterBeta(Node alpha, Node beta) {
        assertNodeFree(alpha);
        alpha.next = beta.next;
        if (beta.next != null)
            beta.next.prev = alpha;
        alpha.prev = beta;
        beta.next = alpha;
        alpha.list = beta.list;
    }

    public static void removeNodeFromList(Node node) {
        if (node.list == null) {
            throw new IllegalArgumentException("Node is not in any list");
        }
        var prev = node.prev;
        var next = node.next;
        if (prev != null) prev.next = next;
        if (next != null) next.prev = prev;
        node.prev = null;
        node.next = null;
        node.list = null;
    }

    private static void assertNodeFree(Node node) {
        if (!(node.prev == null && node.next == null && node.list == null)) {
            throw new UsageRelationshipCheckFailedException("This node has been inserted to some list, you must remove it before inserting to a new one");
        }
    }

}
