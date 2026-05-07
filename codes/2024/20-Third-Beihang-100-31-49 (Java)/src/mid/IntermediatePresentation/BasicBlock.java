package mid.IntermediatePresentation;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Instruction.*;
import mid.Optimizer.Utils.Range;

import java.util.ArrayList;
import java.util.HashMap;

public class BasicBlock extends Value {
    private ArrayList<Instruction> instructionList = new ArrayList<>();

    private int loopDepth;

    private final HashMap<Value, Range> ranges = new HashMap<>();

    public BasicBlock() {
        super(IRManager.getInstance().declareBlock(), ValueType.NULL);
        IRManager.getInstance().setCurBlock(this);
        IRManager.getInstance().addBBToCurFunction(this);
        loopDepth = 0;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(reg).append(":\n");
        for (Instruction instruction : instructionList) {
            sb.append("\t").append(instruction);
        }
        return sb.toString();
    }

    public void addInstruction(Instruction instruction) {
        int i;
        if (instruction instanceof Alloca) {
            instructionList.add(0, instruction);
        } else if (instruction instanceof Phi) {
            for (i = 0; i < instructionList.size(); i++) {
                if (!(instructionList.get(i) instanceof Alloca)) {
                    break;
                }
            }
            instructionList.add(i, instruction);
        } else {
            instructionList.add(instruction);
        }
        instruction.setBlock(this);
    }

    public void addInstrAtEntry(Instruction instruction) {
        int i;
        if (instruction instanceof Alloca) {
            instructionList.add(0, instruction);
        } else if (instruction instanceof Phi) {
            for (i = 0; i < instructionList.size(); i++) {
                if (!(instructionList.get(i) instanceof Alloca)) {
                    break;
                }
            }
            instructionList.add(i, instruction);
        } else {
            for (i = 0; i < instructionList.size(); i++) {
                if (!(instructionList.get(i) instanceof Phi)) {
                    break;
                }
            }
            instructionList.add(i, instruction);
        }
        instruction.setBlock(this);
    }

    public void addInstructionBeforeBranch(Instruction instruction) {
        //最后一个语句一定是跳转指令
        instructionList.add(instructionList.size() - 1, instruction);
        instruction.setBlock(this);
    }

    public void addInstructionAt(int index, Instruction instruction) {
        if (instructionList.contains(instruction)) {
            return;
        }
        instructionList.add(index, instruction);
        instruction.setBlock(this);
    }

    public ArrayList<Instruction> getInstructionList() {
        return instructionList;
    }

    public void deleteLastInstruction() {
        instructionList.remove(instructionList.size() - 1);
    }

    public void removeInstruction(Instruction instruction) {
        instructionList.remove(instruction);
    }

    public void updataInstructionList(ArrayList<Instruction> instructions) {
        instructionList = instructions;
    }

    public Instruction getLastInstruction() {
        return instructionList.get(instructionList.size() - 1);
    }

    public Function getFunction() {
        for (Value v : userList) {
            if (v instanceof Function f) {
                return f;
            }
        }
        return null;
    }

    public void destroy() {
        for (Instruction instruction : instructionList) {
            instruction.destroy();
        }

        for (User user : new ArrayList<>(userList)) {
            // 不包含function，因为可能产生循环中修改错误
            if (user instanceof Instruction) {
                user.removeOperand(this);
            }
        }
    }

    public void redirectTo(BasicBlock originBlock, BasicBlock nextBlock) {
        //修改末尾的跳转方向为新的nextBlock
        Instruction instruction = getLastInstruction();
        if (instruction instanceof Br br) {
            br.redirectTo(originBlock, nextBlock);
        }
        //修改原本的后继块中phi指令的来源
        for (Instruction instr : originBlock.instructionList) {
            if (instr instanceof Phi phi && phi.operandList.contains(this)) {
                phi.redirectFrom(this, nextBlock);
            }
        }
    }

    public int getLoopDepth() {
        return loopDepth;
    }

    public void setLoopDepth(int loopDepth) {
        this.loopDepth = loopDepth;
    }

    public void beReplacedBy(Value v) {
        super.beReplacedBy(v);
        ArrayList<Br> brsToReSet = new ArrayList<>();
        for (User user : userList) {
            // 将这个块代替之后，是否有br成为了单目标
            if (user instanceof Br br && br.operandList.size() > 1 && br.getIfTrue().equals(br.getIfFalse())) {
                brsToReSet.add(br);
            }
        }
        for (Br br : brsToReSet) {
            Br newBr = new Br((BasicBlock) v);
            int index = br.getBlock().getInstructionList().indexOf(br);
            br.beReplacedBy(newBr);
            br.getBlock().removeInstruction(br);
            br.destroy();
            br.getBlock().addInstructionAt(index, newBr);
        }
    }

    public HashMap<Value, Range> getRanges() {
        return ranges;
    }

    public void updateVarRange1(Value var, String op, Value val) {
        Range range = ranges.get(var);
        try {
            int i = Integer.parseInt(val.getReg()); //todo:假设为int
            if (range == null) {         //todo:这块可以删掉,由父亲更新儿子的所有变量
                range = new Range();
            }
            range.addRange(op, i);
            ranges.put(var, range);
        } catch (NumberFormatException e) {
            return;
        }
    }

    public void updateVarRange2(Value var, String op, Value val1, Value val2) {
        Range range1 = ranges.get(val1);
        if (range1 == null) return;
        try {
            int i = Integer.parseInt(val2.getReg()); //todo:假设为int
            Range range = range1.deepCopy();
            range.addRange(op, i);
            ranges.put(var, range);
        } catch (NumberFormatException e) {
            return;
        }
    }

    public void updateVarRange(Cmp cmp, boolean flag) {
        Value var = cmp.getOperandList().get(0);
        String op = cmp.getOperator();
        if (!flag)
            op = Cmp.getOppositeOperator2(op);
        Value val = cmp.getOperandList().get(1);
        updateVarRange1(var, op, val);
    }

    public void updateVarRange(ALU alu) {
        String op = alu.getOperator();
        Value val1 = alu.getOperand1();
        Value val2 = alu.getOperand2();
        updateVarRange2(alu, op, val1, val2);
    }

    public Range getVarRange(Value var) {
        Range range = ranges.get(var);
        if (range == null) {
            range = new Range();
        }
        return range;
    }

    public void printRange() {
        for (Value value : ranges.keySet()) {
            System.out.print(value.getReg() + ": ");
            System.out.println(ranges.get(value));
        }
    }

    public void copyRanges(BasicBlock block) {
        HashMap<Value, Range> srcRange = block.getRanges();
        for (Value value : srcRange.keySet())
            this.ranges.put(value, srcRange.get(value).deepCopy());
//        this.ranges = block.getRanges();
    }

    public void addInstructionAtMoveEntry(Instruction instruction) {
        int idx = -1;
        for (int i = 0; i < instructionList.size(); i++) {
            if (instructionList.get(i) instanceof MoveIR) {
                idx = i;
                break;
            }
        }
        if (idx == -1) {
            addInstructionBeforeBranch(instruction);
        } else {
            instructionList.add(idx, instruction);
        }
        instruction.setBlock(this);
    }

    public void addInstructionAtMoveTail(Instruction instruction) {
        int idx = -1;
        for (int i = instructionList.size() - 1; i >= 0; i--) {
            if (instructionList.get(i) instanceof MoveIR) {
                idx = i;
                break;
            }
        }
        if (idx == -1) {
            addInstructionBeforeBranch(instruction);
        } else {
            instructionList.add(idx + 1, instruction);
        }
        instruction.setBlock(this);
    }
}
