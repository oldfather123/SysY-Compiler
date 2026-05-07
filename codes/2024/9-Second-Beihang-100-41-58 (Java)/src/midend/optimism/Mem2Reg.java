package midend.optimism;

import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.PointerType;
import midend.Module;
import midend.instr.*;

import java.util.*;

public class Mem2Reg {
    private Module module;
    private Instruction curInstr;
    private HashSet<BasicBlock> visited = new HashSet<>();
    private Stack<Value> stack;
    private ArrayList<BasicBlock> defBlockList;
    private ArrayList<BasicBlock> useBlockList;
    private HashMap<Instruction, Instruction> defInstrList;
    private ArrayList<Instruction> useInstrList;

    public Mem2Reg(Module module){
        this.module = module;
        curInstr = null;
        stack = null;
        this.useBlockList = null;
        this.defBlockList = null;
        this.useInstrList = null;
        this.defInstrList = null;
    }

    public void initAttr(Instruction instruction) {
        curInstr = instruction;
        stack = new Stack<>();
        this.useBlockList = new ArrayList<>();
        this.defBlockList = new ArrayList<>();
        this.useInstrList = new ArrayList<>();
        this.defInstrList = new HashMap<>();

        for (Use use: instruction.getUseList()) {
            assert use.getUser() instanceof Instruction;
            Instruction user = (Instruction) use.getUser();
            if (user instanceof StoreInstr) {//TODO isdelete()
                defInstrList.put(user, instruction);
                if (!defBlockList.contains(user.getBasicBlock())) {
                    defBlockList.add(user.getBasicBlock());
                }
            }
            else if (user instanceof LoadInstr) {//TODO isdelete()
                useInstrList.add(user);
                if (!defBlockList.contains(user.getBasicBlock()))
                    useBlockList.add(user.getBasicBlock());
            }
        }
    }

    public void insert(BasicBlock block, AllocaInstr instr) {
        LinkedList<Instruction> instructionList = block.getInstrList();
        //ArrayList<BasicBlock> preBlocks = new ArrayList<>(block.getPreBlock());
        Instruction phi = new PhiInstr(((PointerType) instr.getType()).getElementType(), block.getPreBlock());
        instructionList.addFirst(phi);
        phi.setBasicBlock(block);
        useInstrList.add(phi);
        defInstrList.put(phi, instr);
    }
    public void insertPhi(Instruction instruction){
        HashSet<BasicBlock> needPhi = new HashSet<>();//需要phi的block
        Stack<BasicBlock> defValBlock = new Stack<>();//变量定义块集合
        for (BasicBlock block: defBlockList) {
            defValBlock.push(block);
        }
        while (!defValBlock.isEmpty()) {
            BasicBlock X = defValBlock.pop();
            for (BasicBlock Y : X.getDF()) {
                if (!needPhi.contains(Y)) {
                    insert(Y, (AllocaInstr) instruction);
                    needPhi.add(Y);
                    if (! defBlockList.contains(Y)) {
                        defValBlock.push(Y);
                    }
                }
            }
        }
    }

    public void rename(BasicBlock entryBlock) {
        visited.add(entryBlock);
        int count = 0;
        //Iterator<Instruction>iterator = entryBlock.getInstrList().iterator();
        //LinkedList<Instruction> instructions = new LinkedList<>(entryBlock.getInstrList());
        ArrayList<Instruction> instructions = new ArrayList<>();
        for (Instruction instruction : entryBlock.getInstrList()) {
            if (useInstrList.contains(instruction) && instruction instanceof LoadInstr) {
                Value newValue;
                if (stack.empty()) {
                    newValue = new UndefinedValue(instruction.getType());
                }
                else {
                    newValue = stack.peek();
                }
//                if (instruction.getName().equals("%reg52")) {
//                    System.out.println("");
//                }
                instruction.replaceUse(newValue);//TODO modifyAllUseThisToNewValue
                instructions.add(instruction);
            }
            else if (defInstrList.containsKey(instruction) && instruction instanceof StoreInstr) {
                Value value = ((StoreInstr) instruction).getValue();
                stack.push(value);
                count++;
                instructions.add(instruction);
            }
            else if (defInstrList.containsKey(instruction) && instruction instanceof PhiInstr) {
                stack.push(instruction);
                count++;
            }
            else if (instruction.equals(curInstr)) {
                instructions.add(instruction);
            }
        }

        for (BasicBlock nextBlock: entryBlock.getNextBlock()) {
            Instruction firstInstruction = nextBlock.getInstrList().getFirst();
            if (useInstrList.contains(firstInstruction) && firstInstruction instanceof PhiInstr) {
                PhiInstr phi = (PhiInstr) firstInstruction;
                Value option = stack.empty() ? new UndefinedValue(phi.getType()) : stack.peek();
//                if (option instanceof UndefinedValue) {
//                    continue;
//                }
                phi.addOption(option, entryBlock);
            }
        }

        for (Instruction instruction : instructions) {
            instruction.remove();
        }

        for (BasicBlock block : entryBlock.getChildBlock()) {
            if (block.equals(entryBlock)) {
                continue;
            }
//            if (visited.contains(block)) {
//                continue;
//            }
            rename(block);
        }
        for (int i = 0; i < count; i++) {
            stack.pop();
        }
    }

    public void run() {
        for (Function func: module.getFunctions()) {
            for (BasicBlock block: func.getBlockList()) {
                ArrayList<Instruction> instructions = new ArrayList<>(block.getInstrList());
                for (Instruction instruction: instructions) {
                    if (instruction instanceof AllocaInstr && (((PointerType) instruction.getType()).getElementType() == IntegerType.i32 || ((PointerType) instruction.getType()).getElementType() == FloatType.f32)) {
                        visited = new HashSet<>();
                        initAttr(instruction);//初始化和该alloca指令相关的数据结构
                        insertPhi(instruction);//找出需要添加phi指令的基本块并添加phi
                        rename(func.getBlockList().get(0));//通过DFS进行重命名，同时将相关的alloca, store,load指令删除
                    }
                }
            }
        }
    }
}
