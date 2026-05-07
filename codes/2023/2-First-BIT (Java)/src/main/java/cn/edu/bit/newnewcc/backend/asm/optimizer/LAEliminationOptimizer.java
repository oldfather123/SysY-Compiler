package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmBlockEnd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.*;

public class LAEliminationOptimizer implements Optimizer {
    private static class Record {
        private final Register register;
        private final Label globalVar;

        public Record(Register register, Label globalVar) {
            this.register = register;
            this.globalVar = globalVar;
        }

        public Register getRegister() {
            return register;
        }

        public Label getGlobalVar() {
            return globalVar;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Record record = (Record) o;

            if (!register.equals(record.register)) return false;
            return globalVar.equals(record.globalVar);
        }

        @Override
        public int hashCode() {
            int result = register.hashCode();
            result = 31 * result + globalVar.hashCode();
            return result;
        }

        @Override
        public String toString() {
            return "Record{" +
                "register=" + register +
                ", globalVar=" + globalVar +
                '}';
        }
    }

    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        Set<Record> records = new HashSet<>();

        int count = 0;

        Iterator<AsmInstruction> iterator = instrList.iterator();
        while (iterator.hasNext()) {
            AsmInstruction instr = iterator.next();

            if (instr instanceof AsmBlockEnd) continue;

            if (instr instanceof AsmLabel) {
                records.clear();
            } else if (instr instanceof AsmLoad loadInstr && loadInstr.getOpcode() == AsmLoad.Opcode.LA) {
                Record newRecord = new Record((Register) loadInstr.getOperand(1), (Label) loadInstr.getOperand(2));
                if (records.contains(newRecord)) {
                    iterator.remove();
                    ++count;
                } else {
                    records.removeIf(record -> record.getRegister().equals(newRecord.getRegister()));
                    records.add(newRecord);
                }
            } else {
                Set<Register> def = instr.getDef();
                records.removeIf(record -> def.contains(record.getRegister()));
            }
        }

        return count > 0;
    }
}
