package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.value.BaseFunction;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.*;

public class CloneBase {
    private final Map<Value, Value> valueMap = new HashMap<>();

    protected void addBasicBlock(BasicBlock basicBlock) {
        valueMap.put(basicBlock, new BasicBlock());
    }

    protected void addSymbolFor(Value value) {
        valueMap.put(value, new Symbol(value.getType()));
    }

    protected Value getReplacedValue(Value value) {
        return valueMap.getOrDefault(value, value);
    }

    protected void setValueMapKv(Value key, Value value) {
        if (valueMap.containsKey(key)) {
            var oldValue = valueMap.get(key);
            if (!(oldValue instanceof Symbol)) {
                // 如果一个key已经被分配了value，且这个value不是占位符
                // 则替换可能导致value悬空，这通常不是调用者所期待的行为
                throw new RuntimeException("Cannot changed value for a key that is not a symbol.");
            }
            oldValue.replaceAllUsageTo(value);
        }
        valueMap.put(key, value);
    }

    protected Instruction cloneInstruction(Instruction instruction) {
        if (instruction instanceof AllocateInst allocateInst) {
            return new AllocateInst(allocateInst.getAllocatedType());
        } else if (instruction instanceof BitCastInst bitCastInst) {
            return new BitCastInst(
                    getReplacedValue(bitCastInst.getSourceOperand()),
                    bitCastInst.getTargetType()
            );
        } else if (instruction instanceof BranchInst branchInst) {
            return new BranchInst(
                    getReplacedValue(branchInst.getCondition()),
                    (BasicBlock) getReplacedValue(branchInst.getTrueExit()),
                    (BasicBlock) getReplacedValue(branchInst.getFalseExit())
            );
        } else if (instruction instanceof CallInst callInst) {
            var argumentList = new ArrayList<Value>();
            for (int i = 0; i < callInst.getArgumentSize(); i++) {
                argumentList.add(getReplacedValue(callInst.getArgumentAt(i)));
            }
            return new CallInst(
                    (BaseFunction) getReplacedValue(callInst.getCallee()),
                    argumentList
            );
        } else if (instruction instanceof FloatAddInst floatAddInst) {
            return new FloatAddInst(
                    floatAddInst.getType(),
                    getReplacedValue(floatAddInst.getOperand1()),
                    getReplacedValue(floatAddInst.getOperand2())
            );
        } else if (instruction instanceof FloatCompareInst floatCompareInst) {
            return new FloatCompareInst(
                    floatCompareInst.getComparedType(),
                    floatCompareInst.getCondition(),
                    getReplacedValue(floatCompareInst.getOperand1()),
                    getReplacedValue(floatCompareInst.getOperand2())
            );
        } else if (instruction instanceof FloatDivideInst floatDivideInst) {
            return new FloatDivideInst(
                    floatDivideInst.getType(),
                    getReplacedValue(floatDivideInst.getOperand1()),
                    getReplacedValue(floatDivideInst.getOperand2())
            );
        } else if (instruction instanceof FloatMultiplyInst floatMultiplyInst) {
            return new FloatMultiplyInst(
                    floatMultiplyInst.getType(),
                    getReplacedValue(floatMultiplyInst.getOperand1()),
                    getReplacedValue(floatMultiplyInst.getOperand2())
            );
        } else if (instruction instanceof FloatNegateInst floatNegateInst) {
            return new FloatNegateInst(
                    floatNegateInst.getType(),
                    getReplacedValue(floatNegateInst.getOperand1())
            );
        } else if (instruction instanceof FloatSubInst floatSubInst) {
            return new FloatSubInst(
                    floatSubInst.getType(),
                    getReplacedValue(floatSubInst.getOperand1()),
                    getReplacedValue(floatSubInst.getOperand2())
            );
        } else if (instruction instanceof FloatToSignedIntegerInst floatToSignedIntegerInst) {
            return new FloatToSignedIntegerInst(
                    getReplacedValue(floatToSignedIntegerInst.getSourceOperand()),
                    floatToSignedIntegerInst.getTargetType()
            );
        } else if (instruction instanceof GetElementPtrInst getElementPtrInst) {
            var indices = new ArrayList<Value>();
            for (Value indexOperand : getElementPtrInst.getIndexOperands()) {
                indices.add(getReplacedValue(indexOperand));
            }
            return new GetElementPtrInst(
                    getReplacedValue(getElementPtrInst.getRootOperand()),
                    indices
            );
        } else if (instruction instanceof IntegerAddInst integerAddInst) {
            return new IntegerAddInst(
                    integerAddInst.getType(),
                    getReplacedValue(integerAddInst.getOperand1()),
                    getReplacedValue(integerAddInst.getOperand2())
            );
        } else if (instruction instanceof IntegerCompareInst integerCompareInst) {
            return new IntegerCompareInst(
                    integerCompareInst.getComparedType(),
                    integerCompareInst.getCondition(),
                    getReplacedValue(integerCompareInst.getOperand1()),
                    getReplacedValue(integerCompareInst.getOperand2())
            );
        } else if (instruction instanceof IntegerMultiplyInst integerMultiplyInst) {
            return new IntegerMultiplyInst(
                    integerMultiplyInst.getType(),
                    getReplacedValue(integerMultiplyInst.getOperand1()),
                    getReplacedValue(integerMultiplyInst.getOperand2())
            );
        } else if (instruction instanceof IntegerSignedDivideInst integerSignedDivideInst) {
            return new IntegerSignedDivideInst(
                    integerSignedDivideInst.getType(),
                    getReplacedValue(integerSignedDivideInst.getOperand1()),
                    getReplacedValue(integerSignedDivideInst.getOperand2())
            );
        } else if (instruction instanceof IntegerSignedRemainderInst integerSignedRemainderInst) {
            return new IntegerSignedRemainderInst(
                    integerSignedRemainderInst.getType(),
                    getReplacedValue(integerSignedRemainderInst.getOperand1()),
                    getReplacedValue(integerSignedRemainderInst.getOperand2())
            );
        } else if (instruction instanceof IntegerSubInst integerSubInst) {
            return new IntegerSubInst(
                    integerSubInst.getType(),
                    getReplacedValue(integerSubInst.getOperand1()),
                    getReplacedValue(integerSubInst.getOperand2())
            );
        } else if (instruction instanceof SignedMinInst signedMinInst) {
            return new SignedMinInst(
                    signedMinInst.getType(),
                    getReplacedValue(signedMinInst.getOperand1()),
                    getReplacedValue(signedMinInst.getOperand2())
            );
        } else if (instruction instanceof SignedMaxInst signedMaxInst) {
            return new SignedMaxInst(
                    signedMaxInst.getType(),
                    getReplacedValue(signedMaxInst.getOperand1()),
                    getReplacedValue(signedMaxInst.getOperand2())
            );
        } else if (instruction instanceof JumpInst jumpInst) {
            return new JumpInst((BasicBlock) getReplacedValue(jumpInst.getExit()));
        } else if (instruction instanceof LoadInst loadInst) {
            return new LoadInst(getReplacedValue(loadInst.getAddressOperand()));
        } else if (instruction instanceof PhiInst phiInst) {
            var clonedPhiInst = new PhiInst(phiInst.getType());
            phiInst.forEach((entryBlock, value) -> clonedPhiInst.addEntry(
                    (BasicBlock) getReplacedValue(entryBlock),
                    getReplacedValue(value)
            ));
            return clonedPhiInst;
        } else if (instruction instanceof ReturnInst returnInst) {
            return new ReturnInst(getReplacedValue(returnInst.getReturnValue()));
        } else if (instruction instanceof SignedIntegerToFloatInst signedIntegerToFloatInst) {
            return new SignedIntegerToFloatInst(
                    getReplacedValue(signedIntegerToFloatInst.getSourceOperand()),
                    signedIntegerToFloatInst.getTargetType()
            );
        } else if (instruction instanceof StoreInst storeInst) {
            return new StoreInst(
                    getReplacedValue(storeInst.getAddressOperand()),
                    getReplacedValue(storeInst.getValueOperand())
            );
        } else if (instruction instanceof ZeroExtensionInst zeroExtensionInst) {
            return new ZeroExtensionInst(
                    getReplacedValue(zeroExtensionInst.getSourceOperand()),
                    zeroExtensionInst.getTargetType()
            );
        } else if (instruction instanceof SignedExtensionInst signedExtensionInst) {
            return new SignedExtensionInst(getReplacedValue(signedExtensionInst.getSourceOperand()), signedExtensionInst.getTargetType());
        } else if (instruction instanceof TruncInst truncInst) {
            return new TruncInst(getReplacedValue(truncInst.getSourceOperand()), truncInst.getTargetType());
        } else {
            throw new IllegalArgumentException("Cannot clone instruction of type " + instruction.getClass());
        }
    }

    protected static class Symbol extends Value {
        public Symbol(Type type) {
            super(type);
        }

        private String name;

        @Override
        public String getValueName() {
            if (name == null) {
                name = String.format("%s#%s", this.getClass(), UUID.randomUUID());
            }
            return name;
        }

        @Override
        public String getValueNameIR() {
            return '%' + getValueName();
        }

        @Override
        public void setValueName(String valueName) {
            this.name = valueName;
        }

    }

    protected static class BasicBlockSymbol extends BasicBlock {
        private String name;

        @Override
        public String getValueName() {
            if (name == null) {
                name = String.format("%s#%s", this.getClass(), UUID.randomUUID());
            }
            return name;
        }

        @Override
        public String getValueNameIR() {
            return '%' + getValueName();
        }

        @Override
        public void setValueName(String valueName) {
            this.name = valueName;
        }

        @Override
        public void emitIr(StringBuilder builder) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void addInstruction(Instruction instruction) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void setTerminateInstruction(TerminateInst terminateInstruction) {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<Instruction> getLeadingInstructions() {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<Instruction> getMainInstructions() {
            throw new UnsupportedOperationException();
        }

        @Override
        public TerminateInst getTerminateInstruction() {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<Instruction> getInstructions() {
            throw new UnsupportedOperationException();
        }

        @Override
        public Collection<BasicBlock> getExitBlocks() {
            throw new UnsupportedOperationException();
        }

        @Override
        public Set<BasicBlock> getEntryBlocks() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void removeEntryFromPhi(BasicBlock entry) {
            throw new UnsupportedOperationException();
        }

        @Override
        public Function getFunction() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void removeFromFunction() {
            throw new UnsupportedOperationException();
        }

        @Override
        public void __setFunction__(Function function, boolean shouldFixFunction) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void __clearFunction__() {
            throw new UnsupportedOperationException();
        }

    }

}
