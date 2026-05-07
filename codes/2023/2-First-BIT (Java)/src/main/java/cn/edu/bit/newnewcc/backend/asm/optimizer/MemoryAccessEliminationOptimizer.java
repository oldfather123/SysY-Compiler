package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.AsmOperand;
import cn.edu.bit.newnewcc.backend.asm.operand.MemoryAddress;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.operand.StackVar;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

public class MemoryAccessEliminationOptimizer implements Optimizer {
    private static class Record {
        private final Register register;
        private final MemoryAddress memoryAddress;
        private final int bitLength;

        public Record(Register register, MemoryAddress memoryAddress, int bitLength) {
            this.register = register;
            this.memoryAddress = memoryAddress;
            this.bitLength = bitLength;
        }

        public Register getRegister() {
            return register;
        }

        public MemoryAddress getMemoryAddress() {
            return memoryAddress;
        }

        public int getBitLength() {
            return bitLength;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Record record = (Record) o;

            if (bitLength != record.bitLength) return false;
            if (!register.equals(record.register)) return false;
            return memoryAddress.equals(record.memoryAddress);
        }

        @Override
        public int hashCode() {
            int result = register.hashCode();
            result = 31 * result + memoryAddress.hashCode();
            result = 31 * result + bitLength;
            return result;
        }

        @Override
        public String toString() {
            return "Record{" +
                "register=" + register +
                ", memoryAddress=" + memoryAddress +
                ", bitLength=" + bitLength +
                '}';
        }
    }

    private static MemoryAddress getMemoryAddress(AsmInstruction instr) {
        if (instr instanceof AsmLoad || instr instanceof AsmStore) {
            AsmOperand operand = instr.getOperand(2);
            if (operand instanceof MemoryAddress memoryAddress) {
                return memoryAddress;
            } else if (operand instanceof StackVar stackVar) {
                return stackVar.getAddress();
            } else {
                throw new IllegalArgumentException();
            }
        } else {
            throw new IllegalArgumentException();
        }
    }

    private static int getBitLength(AsmInstruction instr) {
        if (instr instanceof AsmLoad || instr instanceof AsmStore) {
            AsmOperand operand = instr.getOperand(2);
            if (operand instanceof MemoryAddress) {
                if (instr instanceof AsmLoad) {
                    return switch (((AsmLoad) instr).getOpcode()) {
                        case LD, FLD -> 64;
                        case LW, FLW -> 32;
                        default -> throw new IllegalArgumentException();
                    };
                } else {
                    return switch (((AsmStore) instr).getOpcode()) {
                        case SD, FSD -> 64;
                        case SW, FSW -> 32;
                    };
                }
            } else if (operand instanceof StackVar stackVar) {
                return stackVar.getSize() * 8;
            } else {
                throw new IllegalArgumentException();
            }
        } else {
            throw new IllegalArgumentException();
        }
    }

    private static Record getRecord(AsmInstruction instr) {
        if (instr instanceof AsmLoad || instr instanceof AsmStore) {
            return new Record((Register) instr.getOperand(1), getMemoryAddress(instr), getBitLength(instr));
        } else {
            throw new IllegalArgumentException();
        }
    }

    private static boolean overlap(long offset1, int size1, long offset2, int size2) {
        return !(offset1 + size1 <= offset2 || offset2 + size2 <= size1);
    }

    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        Set<Record> records = new HashSet<>();

        int count = 0;

        Iterator<AsmInstruction> iterator = instrList.iterator();
        while (iterator.hasNext()) {
            AsmInstruction instr = iterator.next();

            if (!(instr instanceof AsmLabel) && !(instr instanceof AsmBlockEnd)) {
                Set<Register> def = instr.getDef();
                records.removeIf(record -> def.contains(record.getRegister()) || def.contains(record.getMemoryAddress().getBaseAddress()));
            }

            if (instr instanceof AsmLabel) {
                records.clear();
            } else if (instr instanceof AsmLoad && (instr.getOperand(2) instanceof MemoryAddress || instr.getOperand(2) instanceof StackVar)) {
                Record newRecord = getRecord(instr);
                records.add(newRecord);
            } else if (instr instanceof AsmStore) {
                Record newRecord = getRecord(instr);

                if (records.contains(newRecord)) {
                    iterator.remove();
                    ++count;
                } else {
                    records.removeIf(record ->
                        !record.getMemoryAddress().getBaseAddress().equals(newRecord.getMemoryAddress().getBaseAddress())
                            || overlap(record.getMemoryAddress().getOffset(), record.getBitLength() / 8, newRecord.getMemoryAddress().getOffset(), newRecord.getBitLength() / 8)
                    );
                }
            }
        }

        return count > 0;
    }
}
