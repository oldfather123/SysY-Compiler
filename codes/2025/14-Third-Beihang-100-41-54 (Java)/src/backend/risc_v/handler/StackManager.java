package backend.risc_v.handler;


import backend.ir.entity.Value;

import java.util.HashMap;

public class StackManager {
    private final HashMap<Value, Integer> memoryMap;
    private int stackSize;
    private int argumentSize;
    private int parameterSize;

    public StackManager() {
        memoryMap = new HashMap<>();
        stackSize = 0;
        argumentSize = 0;
        parameterSize = 0;
    }

    public void allocate(Value value) {
        int size = value.getTotalBytes();
        if (memoryMap.containsKey(value))
            throw new RuntimeException("Variable " + value.getName() + " already allocated.");

        stackSize += size;
        memoryMap.put(value, -stackSize);
    }

    public int getAddress(Value value) {
        if (!memoryMap.containsKey(value)) {
            allocate(value);
        }

        return memoryMap.get(value);
    }

    public void clear() {
        memoryMap.clear();
        stackSize = 0;
        argumentSize = 0;
        parameterSize = 0;
    }

    public void addArgument(Value value) {
        memoryMap.put(value, argumentSize);
        argumentSize += value.getTotalBytes();
    }

    public void addParameter(Value param) {
        parameterSize += param.getTotalBytes();
    }

    public int getStackTop() {
        return stackSize + parameterSize;
    }

    public void removeParameter() {
        parameterSize = 0;
    }
}
