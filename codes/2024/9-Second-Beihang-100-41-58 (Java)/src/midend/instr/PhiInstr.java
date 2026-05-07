package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.IntegerType;
import midend.LLVMType.Type;
import midend.Use;
import midend.Value;

import java.util.ArrayList;
import java.util.stream.Collectors;

public class PhiInstr extends Instruction {
    private ArrayList<BasicBlock> preBlockList;
    public PhiInstr(Type type, ArrayList<BasicBlock> preBlockList) {
        super(type, "%reg" + (Value.num++), InstrType.PHI);
//        if (this.getName().equals("%reg527")) {
//            System.out.println("");
//        }
        this.preBlockList = preBlockList;
        for (int i = 0; i < preBlockList.size(); i++) {
            addValue(null);
        }
    }

    public PhiInstr(Type type) {
        super(type, "%reg" + (Value.num++), InstrType.PHI);
        this.preBlockList = new ArrayList<>();
    }

    public Value getValueFrom(BasicBlock block) {
        for (int i = 0; i < preBlockList.size(); i++) {
            if (preBlockList.get(i) == block) {
                return getValue(i);
            }
        }
        return null;
    }

    public void addOp(Value value, BasicBlock preBlock) {
        preBlockList.add(preBlock);
        addValue(value);
    }

    public void addOption(Value value, BasicBlock preBlock) {
        int index = preBlockList.indexOf(preBlock);
//        Value before = getValue(index);
//        if (before != null) {
//            before.removeUserFromUse(this);
//        }
        if (index == -1) {
            System.out.println("");
        }
        setValue(index, value);
        //value.addUse(new Use(value, this));
    }

    public ArrayList<Value> getOptions() {
        return getValueList();
    }//感觉不是很需要，直接调用getValueList应该一样的

    public boolean canUse() {
        return true;
    }

    public String getInstr() {
        ArrayList<Value> options = getOptions();
        //  %4 = phi i32 [ 1, %2 ], [ %6, %5 ]
//        if (this.getName().equals("%reg4379")) {
//            System.out.println("");
//        }
        return getName() + " = phi " + getType() + " "
                + preBlockList.stream()
                .map(bb -> "[ " + options.get(preBlockList.indexOf(bb)).getName() + ", %" + bb.getName() + " ]")
                .collect(Collectors.joining(", ")) + "\n";
    }

    public BasicBlock getPreBlock(int count) {
        return preBlockList.get(count);
    }

    public BasicBlock getPreBlock(Value value) {
        int index = this.getOptions().indexOf(value);
        for (int count = 0; count < preBlockList.size(); count++) {
            if (getValue(count).equals(value)) {
                index = count;
            }
        }
        return preBlockList.get(index);
    }

    public ArrayList<BasicBlock> getPreBlockList() {
        return this.preBlockList;
    }

    public void removeValue(int index) {
//        if (this.getName().equals("%reg256")) {
//            System.out.println("");
//        }
//        if (this.getName().equals("%reg4379")) {
//            System.out.println("");
//        }
//        if (this.getName().equals("%reg493")) {
//            System.out.println("");
//        }
        Value value = getValue(index);
        this.getValueList().remove(index);
        value.removeUserFromUse(this);
    }

    @Override
    public PhiInstr clone(BasicBlock block) {
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        for (BasicBlock basicBlock : preBlockList) {
            BasicBlock block1 = basicBlock;
            if (Function.cloneMap.containsKey(block1)) {
                block1 = (BasicBlock) Function.cloneMap.get(block1);
            }
            basicBlocks.add(block1);
        }
        PhiInstr instr = new PhiInstr(this.getType(), basicBlocks);
        instr.setBasicBlock(block);
        for (int count = 0; count < instr.preBlockList.size(); count++) {
            Value copy = this.getValue(count);
            if (Function.cloneMap.containsKey(copy)) {
                copy = Function.cloneMap.get(copy);
            }
            instr.setValue(count, copy);
        }
        return instr;
    }

    public void modifyBlock(BasicBlock before, BasicBlock after) {
//        if (before.getName().equals("block20")) {
//            System.out.println("");
//        }
        for (int count = 0; count < this.preBlockList.size(); count++) {
            if (this.preBlockList.get(count).equals(before)) {
                this.preBlockList.set(count, after);
                break;
            }
        }
    }

    public boolean canBeUsed() {
        return true;
    }

    public void setPreBlockList(ArrayList<BasicBlock> basicBlocks) {
        this.preBlockList = basicBlocks;
    }

}

