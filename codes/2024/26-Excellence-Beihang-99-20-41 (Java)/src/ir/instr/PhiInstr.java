package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.value.BasicBlock;
import ir.value.Value;
import utils.Panic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class PhiInstr extends IrInstr {
    public Value result;
    public IrType resultType;
    public ArrayList<Value> fromBlocks;
    public ArrayList<Value> fromValues;
    public OpCode op = OpCode.PHI;

    @Override
    public IrInstr copy() {
        return new PhiInstr(result, resultType, new ArrayList<>(fromBlocks), new ArrayList<>(fromValues));
    }

    public PhiInstr(Value result, IrType resultType, ArrayList<Value> fromBlocks, ArrayList<Value> fromValues) {
        this.result = result;
        this.resultType = resultType;
        this.fromBlocks = fromBlocks;
        this.fromValues = fromValues;
    }

    @Override
    public Value[] getOperands() {
        // return fromBlocks and fromValues
        Value[] res = new Value[1 + fromValues.size()];
        res[0] = result;
        for (int i = 0; i < fromValues.size(); i++) {
            res[i + 1] = fromValues.get(i);
        }
        return res;
    }

    @Override
    public OpCode getOpCode() {
        return op;
    }

    @Override
    public void replaceOperand(Value oldValue, Value newValue) {
        if (IrInstr.checkReplace(result, oldValue)) {
            result = newValue;
        }
        for (int i = 0; i < fromValues.size(); i++) {
            if (IrInstr.checkReplace(fromValues.get(i), oldValue)) {
                fromValues.set(i, newValue);
            }
        }
    }

    public void replaceBasicBlock(String oldBlock, Value newBlock) {
        for (int i = 0; i < fromBlocks.size(); i++) {
            if (fromBlocks.get(i).getName().equals(oldBlock)) {
                fromBlocks.set(i, newBlock);
            }
        }
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<>(){
            {
                addAll(fromBlocks);
                addAll(fromValues);
            }
        };
    }

    public Map<BasicBlock, Value> getIncoming() {
        Map<BasicBlock, Value> res = new HashMap<>();
        for (int i = 0; i < fromBlocks.size(); i++) {
            res.put((BasicBlock) fromBlocks.get(i), fromValues.get(i));
        }
        return res;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(result.getName()).append(" = phi ").append(resultType).append(" ");
        for (int i = 0; i < fromBlocks.size(); i++) {
            if (i != 0) {
                sb.append(", ");
            }
            sb.append("[ ").append(fromValues.get(i).getName()).append(", %").append(fromBlocks.get(i).getName()).append(" ]");
        }
        return sb.toString();
    }

    public void addIncoming(Value block, Value value) {
        Panic.panicIfTrue(block == null || value == null, "phi: block or value is null");
        fromBlocks.add(block);
        fromValues.add(value);
    }
}
