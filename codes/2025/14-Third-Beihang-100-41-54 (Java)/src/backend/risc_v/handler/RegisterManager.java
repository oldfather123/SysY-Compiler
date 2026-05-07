package backend.risc_v.handler;

import backend.ir.entity.GlobalVar;
import backend.ir.entity.Value;
import backend.risc_v.entity.register.Register;

import java.util.*;

/**
 * RegisterManager类用于管理RISC-V架构下的整型和浮点型寄存器的分配与回收。
 * 通过循环指针分配寄存器，支持锁定、回写等功能，
 * 并为后端代码生成提供寄存器资源管理服务。
 */
public class RegisterManager {
    private static final int INT_REGISTER_START_INDEX = 5;
    private static final int FLOAT_REGISTER_START_INDEX = 0;
    private static final int REGISTER_COUNT = 32;

    private final Register[] registerArray;
    private final HashMap<Value, LRUNode> valToNodeMap;
    private final HashMap<LRUNode, Value> nodeToValMap;
    private final DoubleLinkedList intRegisterList;
    private final DoubleLinkedList floatRegisterList;
    private final HashMap<Register, LRUNode> regToNodeMap;


    private static final int PARAMETER_REGISTER_START_INDEX = 10;
    private static final int PARAMETER_REGISTER_END_INDEX = 17;
    private int intParameterRegisterPointer;
    private int floatParameterRegisterPointer;

    public RegisterManager() {

        registerArray = new Register[2 * REGISTER_COUNT];
        for (int i = 0; i < REGISTER_COUNT; i++) {
            registerArray[i] = new Register(i, true);
            registerArray[REGISTER_COUNT + i] = new Register(i, false);
        }

        valToNodeMap = new HashMap<>();
        nodeToValMap = new HashMap<>();
        regToNodeMap = new HashMap<>();
        intRegisterList =
                new DoubleLinkedList(Arrays.copyOfRange(registerArray, INT_REGISTER_START_INDEX, REGISTER_COUNT));
        floatRegisterList = new DoubleLinkedList(Arrays.copyOfRange(registerArray,
                FLOAT_REGISTER_START_INDEX + REGISTER_COUNT, 2 * REGISTER_COUNT));


        intParameterRegisterPointer = PARAMETER_REGISTER_START_INDEX;
        floatParameterRegisterPointer = PARAMETER_REGISTER_START_INDEX;
    }

    private Register allocIntRegister(Value value) {
        LRUNode lruNode = intRegisterList.getHead();
        unmapNodeAndVal(lruNode);
        load(value);
        mapNodeAndVal(value, lruNode);
        intRegisterList.toTail(lruNode);
        return lruNode.register;
    }

    private Register allocFloatRegister(Value value) {
        LRUNode lruNode = floatRegisterList.getHead();
        unmapNodeAndVal(lruNode);
        load(value);
        mapNodeAndVal(value, lruNode);
        floatRegisterList.toTail(lruNode);
        return lruNode.register;
    }

    public Register getRegister(Value value) {
        if (!value.isFloat() || value instanceof GlobalVar || value.isPointer())
            return getIntRegister(value);
        else
            return getFloatRegister(value);
    }

    private Register getIntRegister(Value value) {
        if (!valToNodeMap.containsKey(value))
            return allocIntRegister(value);
        LRUNode lruNode = valToNodeMap.get(value);
        intRegisterList.toTail(lruNode);
        return lruNode.register;
    }

    private Register getFloatRegister(Value value) {
        if (!valToNodeMap.containsKey(value))
            return allocFloatRegister(value);
        LRUNode lruNode = valToNodeMap.get(value);
        floatRegisterList.toTail(lruNode);
        return lruNode.register;
    }

    public Register getTmpRegister(int index, boolean isInt) {
        Register register;
        LRUNode lruNode;
        if (isInt) {
            register = registerArray[index];
            lruNode = regToNodeMap.get(register);
            intRegisterList.toTail(lruNode);
        } else {
            register = registerArray[REGISTER_COUNT + index];
            lruNode = regToNodeMap.get(register);
            floatRegisterList.toTail(lruNode);
        }
        unmapNodeAndVal(lruNode);
        return register;
    }

    public Register getTmpRegister(boolean isInt) {
        LRUNode lruNode;
        if (isInt) {
            lruNode = intRegisterList.getHead();
            intRegisterList.toTail(lruNode);
        } else {
            lruNode = floatRegisterList.getHead();
            floatRegisterList.toTail(lruNode);
        }
        unmapNodeAndVal(lruNode);
        return lruNode.register;
    }

    public void mapReturnValue(Value value) {
        LRUNode lruNode;
        if (!value.isFloat()) {
            lruNode = regToNodeMap.get(registerArray[10]);
            intRegisterList.toTail(lruNode);
        } else {
            lruNode = regToNodeMap.get(registerArray[42]);
            floatRegisterList.toTail(lruNode);
        }
        mapNodeAndVal(value, lruNode);
    }

    public Register zeroRegister() {
        return registerArray[0];
    }

    public Register raRegister() {
        return registerArray[1];
    }

    public Register spRegister() {
        return registerArray[2];
    }

    public Register gpRegister() {
        return registerArray[3];
    }

    public Register tpRegister() {
        return registerArray[4];
    }

    public int addIntParameter() {
        if (intParameterRegisterPointer <= PARAMETER_REGISTER_END_INDEX)
            return intParameterRegisterPointer++;
        else
            return INT_REGISTER_START_INDEX;
    }

    public int addFloatParameter() {
        if (floatParameterRegisterPointer <= PARAMETER_REGISTER_END_INDEX)
            return floatParameterRegisterPointer++;
        else
            return FLOAT_REGISTER_START_INDEX;
    }

    public void removeParameter() {
        intParameterRegisterPointer = PARAMETER_REGISTER_START_INDEX;
        floatParameterRegisterPointer = PARAMETER_REGISTER_START_INDEX;
    }

    private void mapNodeAndVal(Value value, LRUNode LRUNode) {
        valToNodeMap.put(value, LRUNode);
        nodeToValMap.put(LRUNode, value);
    }

    private void unmapNodeAndVal(LRUNode LRUNode) {
        if (nodeToValMap.containsKey(LRUNode)) {
            store(nodeToValMap.get(LRUNode));
            valToNodeMap.remove(nodeToValMap.remove(LRUNode));
        }
    }

    public void storeAll() {
        for (Value value : valToNodeMap.keySet())
            store(value);
        clear();
    }

    public void clear() {
        valToNodeMap.clear();
        nodeToValMap.clear();
    }


    private void load(Value value) {

    }

    private void store(Value value) {

    }

    class DoubleLinkedList {
        private final LRUNode head;
        private final LRUNode tail;

        private DoubleLinkedList(Register[] registers) {
            head = new LRUNode();
            tail = new LRUNode();
            head.next = tail;
            tail.prev = head;
            for (Register register : registers) {
                LRUNode newLRUNode = new LRUNode(tail.prev, tail, register);
                tail.prev.next = newLRUNode;
                tail.prev = newLRUNode;
                regToNodeMap.put(register, newLRUNode);
            }
        }

        private void toTail(LRUNode LRUNode) {
            LRUNode.prev.next = LRUNode.next;
            LRUNode.next.prev = LRUNode.prev;
            LRUNode.next = tail;
            LRUNode.prev = tail.prev;
            tail.prev.next = LRUNode;
            tail.prev = LRUNode;
        }

        private LRUNode getHead() {
            return head.next;
        }
    }

    class LRUNode {
        private LRUNode prev;
        private LRUNode next;
        private final Register register;

        private LRUNode() {
            this.prev = null;
            this.next = null;
            this.register = null;
        }

        private LRUNode(LRUNode prev, LRUNode next, Register register) {
            this.prev = prev;
            this.next = next;
            this.register = register;
        }
    }
}
