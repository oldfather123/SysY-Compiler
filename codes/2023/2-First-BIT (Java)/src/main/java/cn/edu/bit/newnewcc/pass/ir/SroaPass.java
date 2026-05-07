package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInteger;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * 将数组展开为多个寄存器
 */
public class SroaPass {

    private static class Formatter {

        private Formatter() {
        }

        private final List<AllocateInst> registers = new ArrayList<>();

        private void dfsUseTree(Instruction instruction, int offset) {
            if (instruction instanceof GetElementPtrInst getElementPtrInst) {
                Type currentType = ((PointerType) getElementPtrInst.getRootOperand().getType()).getBaseType();
                var indexOperands = getElementPtrInst.getIndexOperands();
                for (int i = 0; i < indexOperands.size(); i++) {
                    if (i != 0) {
                        currentType = ((ArrayType) currentType).getBaseType();
                    }
                    var index = (int) ConstInteger.valueOf((ConstInteger) indexOperands.get(i));
                    int offsetUnit;
                    if (currentType instanceof ArrayType arrayType) {
                        offsetUnit = arrayType.getFlattenedType().getLength();
                    } else {
                        offsetUnit = 1;
                    }
                    offset += index * offsetUnit;
                }
                for (Operand usage : instruction.getUsages()) {
                    dfsUseTree(usage.getInstruction(), offset);
                }
            } else if (instruction instanceof BitCastInst bitCastInst) {
                for (Operand usage : bitCastInst.getUsages()) {
                    dfsUseTree(usage.getInstruction(), offset);
                }
            } else if (instruction instanceof LoadInst loadInst) {
                loadInst.setAddressOperand(registers.get(offset));
            } else if (instruction instanceof StoreInst storeInst) {
                storeInst.setAddressOperand(registers.get(offset));
            } else if (instruction instanceof CallInst callInst) {
                assert callInst.getCallee().getValueName().equals("memset");
                var baseType = ((PointerType) callInst.getArgumentAt(0).getType()).getBaseType();
                while (baseType instanceof ArrayType arrayType) baseType = arrayType.getBaseType();
                var length = ConstInteger.valueOf((ConstInteger) callInst.getArgumentAt(2)) / baseType.getSize();
                for (int i = 0; i < length; i++) {
                    var storeInst = new StoreInst(registers.get(offset + i), baseType.getZeroInitialization());
                    storeInst.insertBefore(callInst);
                }
                callInst.waste();
            } else {
                throw new CompilationProcessCheckFailedException("No way to process when instruction is: " + instruction.getClass());
            }
        }

        private void optimize_(AllocateInst allocateInst) {
            var arrayType = (ArrayType) allocateInst.getAllocatedType();
            var baseType = arrayType.getFlattenedType().getBaseType();
            var size = arrayType.getFlattenedType().getLength();
            for (var i = 0; i < size; i++) {
                registers.add(new AllocateInst(baseType));
            }
            for (Operand usage : allocateInst.getUsages()) {
                dfsUseTree(usage.getInstruction(), 0);
            }
        }

        public List<AllocateInst> getAllocatedRegisters() {
            return Collections.unmodifiableList(registers);
        }

        public static Formatter optimize(AllocateInst allocateInst) {
            var formatter = new Formatter();
            formatter.optimize_(allocateInst);
            return formatter;
        }

    }

    private static final int EXTRACT_THRESHOLD = 128;

    private static boolean isUsedOnlyByConstIndex(Instruction address) {
        for (Operand usage : address.getUsages()) {
            var instruction = usage.getInstruction();
            if (instruction instanceof GetElementPtrInst getElementPtrInst) {
                for (Value indexOperand : getElementPtrInst.getIndexOperands()) {
                    if (!(indexOperand instanceof Constant)) return false;
                    if (!isUsedOnlyByConstIndex(getElementPtrInst)) {
                        return false;
                    }
                }
            } else if (instruction instanceof BitCastInst bitCastInst) {
                if (!isUsedOnlyByConstIndex(bitCastInst)) return false;
            } else if (instruction instanceof LoadInst || instruction instanceof StoreInst) {
                // it's ok
            } else if (instruction instanceof CallInst callInst) {
                if (!callInst.getCallee().getValueName().equals("memset")) return false;
            } else if (instruction instanceof PhiInst phiInst) {
                return false;
            } else {
                throw new CompilationProcessCheckFailedException("Cannot judge when address is: " + address.getClass());
            }
        }
        return true;
    }

    private static boolean canOptimize(AllocateInst allocateInst) {
        if (!(allocateInst.getAllocatedType() instanceof ArrayType arrayType)) return false;
        if (arrayType.getFlattenedType().getLength() > EXTRACT_THRESHOLD) return false;
        return isUsedOnlyByConstIndex(allocateInst);
    }

    public static void runOnFunction(Function function) {
        for (Instruction leadingInstruction : function.getEntryBasicBlock().getLeadingInstructions()) {
            if (leadingInstruction instanceof AllocateInst allocateInst && canOptimize(allocateInst)) {
                var formatter = Formatter.optimize(allocateInst);
                for (AllocateInst allocatedRegister : formatter.getAllocatedRegisters()) {
                    function.getEntryBasicBlock().addInstruction(allocatedRegister);
                }
            }
        }
    }

    public static void runOnModule(Module module) {
        for (Function function : module.getFunctions()) {
            runOnFunction(function);
        }
        MemoryToRegisterPass.runOnModule(module);
    }
}
