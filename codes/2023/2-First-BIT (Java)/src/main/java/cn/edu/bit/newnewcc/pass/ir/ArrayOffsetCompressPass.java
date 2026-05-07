package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.BitCastInst;
import cn.edu.bit.newnewcc.ir.value.instruction.GetElementPtrInst;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * 数组下标压缩 <br>
 * 对于完全由常量确定的下标，将其扁平化 <br>
 */
// 有助于优化局部数组的初始化语句
public class ArrayOffsetCompressPass {

    private final Function function;

    private ArrayOffsetCompressPass(Function function) {
        this.function = function;
    }

    private record ArrayAddress(Value rootAddress, int offset, Type type) {
    }

    private final Map<Value, ArrayAddress> addressMap = new HashMap<>();

    private boolean canCompressAddress(Value address) {
        var compressedGep = getCompressedAddress(address);
        int gepCount = 0;
        int bcCount = 0;
        while (address != compressedGep.rootAddress) {
            if (address instanceof GetElementPtrInst getElementPtrInst) {
                address = getElementPtrInst.getRootOperand();
                gepCount++;
            } else if (address instanceof BitCastInst bitCastInst) {
                address = bitCastInst.getSourceOperand();
                bcCount++;
            } else {
                throw new CompilationProcessCheckFailedException("Cannot trace calculation path of address.");
            }
            if (gepCount > 1 || bcCount > 1) return true;
        }
        return false;
    }

    private ArrayAddress getCompressedGep(GetElementPtrInst getElementPtrInst) {
        boolean isConstGep = true;
        for (Value indexOperand : getElementPtrInst.getIndexOperands()) {
            if (!(indexOperand instanceof ConstInt)) {
                isConstGep = false;
                break;
            }
        }
        if (isConstGep) {
            var rootArrayAddress = getCompressedAddress(getElementPtrInst.getRootOperand());
            var offset = rootArrayAddress.offset + ((ConstInt) getElementPtrInst.getIndexAt(0)).getValue();
            var type = ((PointerType) rootArrayAddress.type).getBaseType();
            for (int i = 1; i < getElementPtrInst.getIndicesSize(); i++) {
                var arrayType = (ArrayType) type;
                offset = offset * arrayType.getLength() + ((ConstInt) getElementPtrInst.getIndexAt(i)).getValue();
                type = arrayType.getBaseType();
            }
            type = PointerType.getInstance(type);
            return new ArrayAddress(rootArrayAddress.rootAddress, offset, type);
        } else {
            return new ArrayAddress(getElementPtrInst, 0, getElementPtrInst.getType());
        }
    }

    private ArrayAddress getCompressedBitCast(BitCastInst bitCastInst) {
        var currentType = bitCastInst.getSourceType();
        if (!(currentType instanceof PointerType)) {
            return new ArrayAddress(bitCastInst, 0, bitCastInst.getType());
        }
        currentType = ((PointerType) currentType).getBaseType();
        var targetType = bitCastInst.getTargetType();
        if (!(targetType instanceof PointerType)) {
            return new ArrayAddress(bitCastInst, 0, bitCastInst.getType());
        }
        targetType = ((PointerType) targetType).getBaseType();
        int rate = (int) (currentType.getSize() / targetType.getSize());
        var sourceAddress = getCompressedAddress(bitCastInst.getSourceOperand());
        return new ArrayAddress(sourceAddress.rootAddress, sourceAddress.offset * rate, bitCastInst.getTargetType());
    }

    private ArrayAddress getCompressedAddress(Value address) {
        if (!addressMap.containsKey(address)) {
            if (address instanceof GetElementPtrInst getElementPtrInst) {
                addressMap.put(address, getCompressedGep(getElementPtrInst));
            } else if (address instanceof BitCastInst bitCastInst) {
                addressMap.put(address, getCompressedBitCast(bitCastInst));
            } else {
                addressMap.put(address, new ArrayAddress(address, 0, address.getType()));
            }
        }
        return addressMap.get(address);
    }

    private boolean runOnFunction() {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                if (instruction instanceof GetElementPtrInst) continue;
                for (Operand operand : instruction.getOperandList()) {
                    if (operand.getValue() instanceof GetElementPtrInst getElementPtrInst && canCompressAddress(getElementPtrInst)) {
                        var arrayAddress = getCompressedAddress(getElementPtrInst);
                        var bitcast = new BitCastInst(arrayAddress.rootAddress, arrayAddress.type);
                        var indices = new ArrayList<Value>();
                        indices.add(ConstInt.getInstance(arrayAddress.offset));
                        var gep = new GetElementPtrInst(bitcast, indices);
                        bitcast.insertBefore(instruction);
                        gep.insertBefore(instruction);
                        operand.setValue(gep);
                        changed = true;
                    }
                }
            }
        }
        return changed;
    }

    public static boolean runOnFunction(Function function) {
        return new ArrayOffsetCompressPass(function).runOnFunction();
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed |= runOnFunction(function);
        }
        return changed;
    }
}
