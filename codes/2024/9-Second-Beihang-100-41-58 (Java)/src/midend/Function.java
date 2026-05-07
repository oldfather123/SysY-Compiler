package midend;

import Utils.LibFunction;
import midend.LLVMType.*;
import midend.analysis.Loop;
import midend.instr.BrInstr;
import midend.instr.CallInstr;
import midend.instr.Instruction;
import midend.instr.PhiInstr;
import midend.analysis.CFGBuilder;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;

public class Function extends User {
    private ArrayList<Param> params;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> preMap;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> nextMap;
    private HashMap<BasicBlock, BasicBlock> parentMap;
    private HashMap<BasicBlock, ArrayList<BasicBlock>> childMap;
    private Type retType;
    private LinkedList<BasicBlock> blockList;
    private boolean isLib = false;
    private int canGVN = -1;
    private ArrayList<Function> beCalledList = new ArrayList<>(); //调用该函数的函数
    private ArrayList<Function> calledList = new ArrayList<>(); //该函数调用的函数
    public static HashMap<Value, Value> cloneMap;
    private BasicBlock exitBlock;
    private HashMap<BasicBlock, Loop> loopInfo;
    private boolean sideEffect = false;
    private ArrayList<Loop> topLevelLoop = new ArrayList<>();
    private boolean isTail = false;
    private boolean isMem = false;

    public Function(Type type, String name, Type retType) {
        super(type, name);
        Value.num++;
        this.retType = retType;
        this.params = new ArrayList<>();
        this.blockList = new LinkedList<>();
    }

    public void setMem() {
        this.isMem = true;
    }

    public boolean isMem() {
        return this.isMem;
    }

    public void setSideEffect() {
        this.sideEffect = true;
    }

    public boolean isSideEffect() {
        return this.sideEffect;
    }

    public void setLoopInfo(HashMap<BasicBlock, Loop> loopInfo) {
        this.loopInfo = loopInfo;
    }

    public HashMap<BasicBlock, Loop> getLoopInfo() {
        return this.loopInfo;
    }

    public int getLoopDepth(BasicBlock block) {
        Loop loop = loopInfo.get(block);
        if (loop == null) {
            return 0;
        }
        return loop.getLoopDepth();
    }

    public void setTopLevelLoop(ArrayList<Loop> loops) {
        this.topLevelLoop = loops;
    }

    public ArrayList<Loop> getTopLevelLoop() {
        return this.topLevelLoop;
    }

    public void setExitBlock(BasicBlock block) {
        this.exitBlock = block;
    }

    public BasicBlock getExitBlock() {
        return this.exitBlock;
    }

    public void setPreMap(HashMap<BasicBlock, ArrayList<BasicBlock>> preMap) {
        this.preMap = this.preMap;
    }

    public void setNextMap(HashMap<BasicBlock, ArrayList<BasicBlock>> nextMap) {
        this.nextMap = nextMap;
    }

    public void addParam(Param param) {
        this.params.add(param);
    }

    public void setParams(ArrayList<Param> params) {
        this.params = params;
    }

    public void addBB(BasicBlock block) {
        this.blockList.add(block);
    }

    public Type getRetType() {
        return this.retType;
    }

    public ArrayList<Param> getParams() {
        return this.params;
    }

    public LinkedList<BasicBlock> getBlockList() {
        return this.blockList;
    }

    public boolean isLib() {
        return LibFunction.libFunc.contains(this);
    }

    public void setParentMap(HashMap<BasicBlock, BasicBlock> preMap) {
        this.parentMap = preMap;
    }

    public void setChildMap(HashMap<BasicBlock, ArrayList<BasicBlock>> childMap) {
        this.childMap = childMap;
    }

    public boolean canBeGVN() {
        if (canGVN != -1) {
            return canGVN == 1;
        }
        for (Param param : params) {
            if (param.getType().isPointer()) {
                canGVN = 0;
                return false;
            }
        }
        for (BasicBlock block : blockList) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof CallInstr) {
                    canGVN = 0;
                    return false;
                }
                for (Value value : instruction.getValueList()) {
                    if (value instanceof GlobalVar) {
                        canGVN = 0;
                        return false;
                    }
                }
            }
        }
        canGVN = 1;
        return true;
    }

    public ArrayList<Function> getBeCalledList() {
        return this.beCalledList;
    }

    public ArrayList<Function> getCalledList() {
        return this.calledList;
    }

    public boolean isInLineable() {
        if (getName().equals("@main")) {
            return false;
        }
        return !this.isLib && this.calledList.isEmpty();
    }

    public Function clone() {
        this.cloneMap = new HashMap<>();
        Function function = new Function(this.getType(), this.getName(), this.retType);
        for (Param param : params) {
            Param param1 = new Param(param.getType());
            function.addParam(param1);
            cloneMap.put(param, param1);
        }
        for (BasicBlock block : blockList) {
            BasicBlock copyBlock = new BasicBlock(UndefinedType.undefined, function);
            if (copyBlock.getName().equals("block141")) {
                System.out.println("");
            }
            function.addBB(copyBlock);
            cloneMap.put(block, copyBlock);
        }
        //TODO:是否按顺序复制
        for (BasicBlock block : blockList) {
            BasicBlock copyBlock = (BasicBlock) cloneMap.get(block);
            for (Instruction instruction : block.getInstrList()) {
                Instruction copyInstr = instruction.clone(copyBlock);
                copyBlock.addInstr(copyInstr);
                cloneMap.put(instruction, copyInstr);
            }
        }
        CFGBuilder cfgBuilder = new CFGBuilder(null);
        cfgBuilder.initFunc(function);
        cfgBuilder.getCFG(function);
        //TODO:phi指令的前驱基本块
        ArrayList<PhiInstr> phiInstrs = new ArrayList<>();
        for (BasicBlock basicBlock : blockList) {
            for (Instruction instruction : basicBlock.getInstrList()) {
                if (instruction instanceof PhiInstr) {
                    phiInstrs.add((PhiInstr) instruction);
                }
            }
        }

        for (PhiInstr phiInstr : phiInstrs) {
            for (int count = 0; count < phiInstr.getOptions().size(); count++) {
                BasicBlock beforeBlock = phiInstr.getBasicBlock().getPreBlock().get(count);
                BasicBlock nowBlock = (BasicBlock) cloneMap.get(beforeBlock);
                PhiInstr copyInstr = (PhiInstr) cloneMap.get(phiInstr);
                Value beforeValue = phiInstr.getValue(count);
                Value nowValue;
                if (beforeValue instanceof ConstInt) {
                    nowValue = new ConstInt(IntegerType.i32, ((ConstInt) beforeValue).getValue());
                } else if (beforeValue instanceof ConstFloat) {
                    nowValue = new ConstFloat(FloatType.f32, ((ConstFloat) beforeValue).getValue());
                } else if (beforeValue instanceof UndefinedValue) {
                    nowValue = new UndefinedValue(beforeValue.getType());
                } else {
                    nowValue = cloneMap.get(beforeValue);
                }
//                if (copyInstr.getName().equals("%reg527")) {
//                    System.out.println("");
//                }
                copyInstr.addOption(nowValue, nowBlock);
            }

        }
        return function;
    }

    public boolean isRecursive() {
        return this.calledList.contains(this);
    }

    public boolean isTailRecursive() {
        return this.isTail;
    }

    public void setTail() {
        this.isTail = true;
    }

    public ArrayList<Param> getFParams() {
        ArrayList<Param> fParams = new ArrayList<>();
        for (Param param : params) {
            if (param.getType().isFloat()) {
                fParams.add(param);
            }
        }
        return fParams;
    }

    public ArrayList<Param> getIParams() {
        ArrayList<Param> IParams = new ArrayList<>();
        for (Param param : params) {
            if (!param.getType().isFloat()) {
                IParams.add(param);
            }
        }
        return IParams;
    }
    public boolean ifHasParam(){
        return params.isEmpty();
    }

    public ArrayList<Loop> getLoops() {
        ArrayList<Loop> loops = new ArrayList<>();
        for (Loop loop : loopInfo.values()) {
            if (!loops.contains(loop)) {
                loops.add(loop);
            }
        }
        return loops;
    }

//    public Loop getFirstLoop() {
//        for (BasicBlock block : blockList) {
//
//        }
//    }
}
