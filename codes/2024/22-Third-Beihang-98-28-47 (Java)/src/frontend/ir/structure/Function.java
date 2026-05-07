package frontend.ir.structure;

import Utils.CustomList;
import frontend.ir.DataType;
import frontend.ir.FuncDef;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.lib.LibFunc;
import frontend.ir.symbols.SymTab;
import frontend.lexer.TokenType;
import frontend.syntax.Ast;
import midend.loop.Loop;

import java.io.IOException;
import java.io.Writer;
import java.util.*;

public class Function extends Value implements FuncDef {
    private static final HashMap<String, Function> FUNCTION_MAP = new HashMap<>();
    private static final int whatIsLong = 100;  // 一个函数多少指令算是“多”，不能内联
    private final String name;
    private final DataType returnType;
    private final Procedure procedure;
    private final List<Ast.FuncFParam> astFParams;
    private final List<FParam> fParams;
    private final SymTab funcSymTab;
    private final HashSet<Function> myImmediateCallee = new HashSet<>(); // 被 this 直接调用的自定义函数集合
    private final HashSet<Function> allCallee = new HashSet<>(); // 本函数执行过程中会调用（包括间接调用）的所有函数，可能包括自己
    private final HashSet<LibFunc> allLibCallee = new HashSet<>(); // 本函数执行过程中会调用（包括间接调用）的所有库函数
    private ArrayList<Loop> allLoop = new ArrayList<>();
    private HashMap<BasicBlock, Loop> header2loop = new HashMap<>();
    private ArrayList<Loop> outerLoop = new ArrayList<>();
    private int calledCnt = 0;
    private boolean isTailRecursive = false;
    private final boolean main;
    private boolean neverUsed = false;
    private int recursiveCallCnt = 0;
    private boolean memorized = false;
    
    public Function(Ast.FuncDef funcDef, SymTab globalSymTab) {
        if (funcDef == null) {
            throw new NullPointerException();
        }
        name = funcDef.getIdent().getContent();
        main = name.equals("main");
        this.funcSymTab = new SymTab(globalSymTab);
        switch (funcDef.getType().getType()) {
            case INT:
                returnType = DataType.INT;
                break;
            case FLOAT:
                returnType = DataType.FLOAT;
                break;
            case VOID:
                returnType = DataType.VOID;
                break;
            default:
                throw new RuntimeException("未定义的返回值类型");
        }
        astFParams = funcDef.getFParams();
        fParams = new ArrayList<>();
        FUNCTION_MAP.put(name, this);
        ArrayList<FuncDef> immediateCalleeList = new ArrayList<>();
        procedure = new Procedure(returnType, astFParams, funcDef.getBody(), funcSymTab, immediateCalleeList, this, fParams);
        initAllCallee(immediateCalleeList);
    }
    
    public static Function getFunction(String name) {
        return FUNCTION_MAP.get(name);
    }
    
    public void printIR(Writer writer) throws IOException {
        if (writer == null) {
            throw new NullPointerException();
        }
        writer.append("define ");
        switch (this.returnType) {
            case VOID:  writer.append("void ");  break;
            case FLOAT: writer.append("float "); break;
            case INT:   writer.append("i32 ");   break;
            default: throw new RuntimeException("输出时出现了未曾设想的函数类型");
        }
        writer.append("@").append(this.name);
        writer.append("(");
        printFParams(writer);
        writer.append(") ");
        writer.append("{\n");
        this.procedure.printIR(writer);
        writer.append("}\n");
    }

    private void printFParams(Writer writer) throws IOException {
        if (writer == null) {
            throw new NullPointerException();
        }
        int len = fParams.size();
        for (int i = 0; i < len; i++) {
            FParam fParam = fParams.get(i);
            writer.append(fParam.type2string());
            writer.append(" ").append(fParam.value2string());
            if (i < len - 1) {
                writer.append(", ");
            }
        }
    }
    
    public List<FParam> getFParamValueList() {
        return this.procedure.getFParamValueList();
    }

    public CustomList getBasicBlocks() {
        return procedure.getBasicBlocks();
    }

    public CallInstr makeCall(int result, List<Value> rParams) {
        if (rParams == null) {
            throw new NullPointerException();
        }
        if (!this.checkParams(rParams)) {
            throw new RuntimeException("形参实参不匹配");
        }
        DataType type = this.getDataType();
        if (type == DataType.VOID) {
            return new CallInstr(null, type, this, rParams);
        } else {
            return new CallInstr(result, type, this, rParams);
        }
    }

    private boolean checkParams(List<Value> rParams) {
        if (rParams == null) {
            throw new NullPointerException();
        }
        if (rParams.size() != astFParams.size()) {
            throw new RuntimeException();
        }
        for (int i = 0; i < astFParams.size(); i++) {
            if (rParams.get(i).getDataType() == DataType.INT &&
                    astFParams.get(i).getType().getType() != TokenType.INT) {
                throw new RuntimeException();
            }
            if (rParams.get(i).getDataType() == DataType.FLOAT &&
                    astFParams.get(i).getType().getType() != TokenType.FLOAT) {
                throw new RuntimeException();
            }
        }
        return true;
    }
    
    public List<Ast.FuncFParam> getAstFParams() {
        return astFParams;
    }
    
    public List<FParam> getFParams() {
        return fParams;
    }
    
    @Override
    public Number getNumber() {
        throw new RuntimeException("函数暂时没有值");
    }
    
    @Override
    public DataType getDataType() {
        return returnType;
    }
    
    @Override
    public String value2string() {
        throw new RuntimeException("函数暂时没有值");
    }
    
    @Override
    public String getName() {
        return name;
    }
    
    @Override
    public String toString() {
        return this.name;
    }

    public LinkedList<Instruction> getAllInstr() {
        LinkedList<Instruction> list = new LinkedList<>();
        BasicBlock block = (BasicBlock) this.getBasicBlocks().getHead();
        while (block != null) {
            Instruction instr = (Instruction) block.getInstructions().getHead();
            while (instr != null) {
                list.add(instr);
                instr = (Instruction) instr.getNext();
            }
            block = (BasicBlock) block.getNext();
        }
        return list;
    }
    
    public int getAndAddRegIndex() {
        return this.procedure.getAndAddRegIndex();
    }
    public int getAndAddPhiIndex() {
        return this.procedure.getAndAddPhiIndex();
    }

    public int getAndAddBlkIndex() {
        return this.procedure.getAndAddBlkIndex();
    }
    
    public boolean isRecursive() {
        return this.recursiveCallCnt > 0;
    }
    
    private void initAllCallee(List<FuncDef> immediateCalleeList) {
        for (FuncDef callee : immediateCalleeList) {
            if (callee instanceof Function) {
                myImmediateCallee.add((Function) callee);
                if (callee == this) {
                    this.recursiveCallCnt++;
                }
            } else if (callee instanceof LibFunc) {
                allLibCallee.add((LibFunc) callee);
            }
        }
        for (Function callee : myImmediateCallee) {
            if (callee != this) {
                this.allCallee.addAll(callee.allCallee);
                this.allLibCallee.addAll(callee.allLibCallee);
            }
            this.allCallee.add(callee);
        }
    }
    
    public ArrayList<BasicBlock> func2blocks(int curDepth, List<Value> rParams, Function caller) {
        ArrayList<BasicBlock> bbs = new ArrayList<>();
        HashMap<Value, Value> old2new = new HashMap<>();
        
        // 准备替换参数
        List<FParam> fParamValueList = this.procedure.getFParamValueList();
        for (int i = 0; i < fParamValueList.size(); i++) {
            old2new.put(fParamValueList.get(i), rParams.get(i));
        }
        
        BasicBlock curBB = (BasicBlock) this.procedure.getBasicBlocks().getHead();
        while (curBB != null) {
            BasicBlock newBB = new BasicBlock(curDepth + curBB.getLoopDepth(), caller.getAndAddBlkIndex());
            old2new.put(curBB, newBB);
            bbs.add(newBB);
            
            Instruction curIns = (Instruction) curBB.getInstructions().getHead();
            while (curIns != null) {
                Instruction newIns = curIns.cloneShell(caller);
                newBB.addInstruction(newIns);
                old2new.put(curIns, newIns);
                curIns = (Instruction) curIns.getNext();
            }
            
            curBB = (BasicBlock) curBB.getNext();
        }
        
        for (BasicBlock newBB : bbs) {
            Instruction newIns = (Instruction) newBB.getInstructions().getHead();
            while (newIns != null) {
                if (newIns instanceof PhiInstr) {
                    ((PhiInstr) newIns).renewBlocks(old2new);
                }
                
                ArrayList<Value> usedValues = new ArrayList<>(newIns.getUseValueList());
                for (Value toReplace : usedValues) {
                    if (!old2new.containsKey(toReplace)) {
                        if (newIns instanceof ReturnInstr && toReplace == newBB) {
                            continue;
                        }
                        if (!(toReplace instanceof ConstValue) && !(toReplace instanceof GlobalObject)) {
                            throw new RuntimeException(newIns.print() + "使用了未曾设想的 value " + toReplace);
                        }
                    } else {
                        newIns.modifyUse(toReplace, old2new.get(toReplace));
                    }
                }
                newIns = (Instruction) newIns.getNext();
            }
        }
        return bbs;
    }
    
    /**
     * 用于将所有的 alloca 都集中到函数最开始以避免反复申请内存；
     */
    public void allocaRearrangement() {
        this.procedure.allocaRearrangement();
    }
    
    public static void blkLabelReorder() {
        for (Function function : FUNCTION_MAP.values()) {
            int label = 0;
            BasicBlock curBB = (BasicBlock) function.procedure.getBasicBlocks().getHead();
            while (curBB != null) {
                curBB.setLabelCnt(label++);
                curBB = (BasicBlock) curBB.getNext();
            }
        }
    }
    
    public void addCall() {
        this.calledCnt++;
    }
    
    public void minusCall() {
        this.calledCnt--;
    }
    
    public int getCalledCnt() {
        return calledCnt;
    }
    
    public void updateUse() {
        if (this.main) {
            this.neverUsed = false;
            return;
        }
        this.neverUsed = this.calledCnt <= 0;
    }
    
    public boolean noUse() {
        return this.neverUsed;
    }

    public Procedure getProcedure() {
        return procedure;
    }
    
    public boolean checkInsTooMany() {
        int cnt = 0;
        BasicBlock basicBlock = (BasicBlock) this.procedure.getBasicBlocks().getHead();
        while (basicBlock != null) {
            Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
            while (instruction != null) {
                if (++cnt > whatIsLong) {
                    return true;
                }
                instruction = (Instruction) instruction.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
        return false;
    }

    public void setAllLoop(ArrayList<Loop> allLoop) {
        this.allLoop = allLoop;
    }

    public void setHeader2loop(HashMap<BasicBlock, Loop> header2loop) {
        this.header2loop = header2loop;
    }

    public void setOuterLoop(ArrayList<Loop> outerLoop) {
        this.outerLoop = outerLoop;
    }

    public ArrayList<Loop> getAllLoop() {
        return allLoop;
    }

    public ArrayList<Loop> getOuterLoop() {
        return outerLoop;
    }

    public HashMap<BasicBlock, Loop> getHeader2loop() {
        return header2loop;
    }

    public int getPhiIndex() {
        return procedure.getPhiIndex();
    }

    public void setCurPhiIndex(int curPhiIndex) {
        procedure.setCurPhiIndex(curPhiIndex);
    }

    public HashSet<CallInstr> getSelfCallingInstrSet() {
        return this.procedure.getSelfCallingInstrSet();
    }

    public boolean isMain() {
        return main;
    }

    public void setTailRecursive(boolean tailRecursive) {
        isTailRecursive = tailRecursive;
    }

    public boolean isTailRecursive() {
        return isTailRecursive;
    }
    
    public boolean canBeMemorized() {
        // 如果没有返回值不能记忆化
        if (this.returnType == DataType.VOID) {
            return false;
        }
        
        // 没有参数的函数不能记忆化
        if (this.fParams.isEmpty()) {
            return false;
        }
        
        // 如果传了指针参数则不可以记忆化
        for (FParam param : this.fParams) {
            if (param.getPointerLevel() > 0) {
                return false;
            }
        }
        
        // 只有调用自身多次（考虑新增：或者被外界重复调用）做记忆化才有意义
        if (this.recursiveCallCnt <= 1/* && this.calledCnt - this.recursiveCallCnt <= 2*/) {
            return false;
        }
        
        if (!this.allLibCallee.isEmpty()) {
            // 不能调用库函数，库函数有 I/O
            return false;
        }
        
        for (Function callee : this.allCallee) {
            // 函数执行过程中不能有副作用，包括 I/O 和修改内存
            if (!callee.checkNoSideEffect()) {
                return false;
            }
            // 不能有针对数组或者全局对象的 load
            if (callee.loadingNonRegElement()) {
                if (this.calledCnt - this.recursiveCallCnt > 1) {
                    // todo: 现在认为如果一个函数只被外界调用过一次可以忽略 load 的限制
                    return false;
                }
            }
        }
        // todo: 检查一下以上条件
        return true;
    }
    
    private boolean loadingNonRegElement() {
        BasicBlock basicBlock = (BasicBlock) this.procedure.getBasicBlocks().getHead();
        while (basicBlock != null) {
            Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
            while (instruction != null) {
                if (instruction instanceof LoadInstr) {
                    Value ptr = ((LoadInstr) instruction).getPtr();
                    if (ptr instanceof GEPInstr || ptr instanceof GlobalObject) {
                        return true;
                    }
                }
                instruction = (Instruction) instruction.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
        return false;
    }
    
    /**
     * 目前检查函数无副作用的标准就是没有 I/O 且没有修改内存（这里特指针对数组元素的修改，因为其它都会被 mem2reg 干掉）
     */
    public boolean checkNoSideEffect() {
        if (!this.allLibCallee.isEmpty()) {
            return false;
        }
        
        BasicBlock basicBlock = (BasicBlock) this.procedure.getBasicBlocks().getHead();
        while (basicBlock != null) {
            Instruction ins = (Instruction) basicBlock.getInstructions().getHead();
            while (ins != null) {
                if (ins instanceof StoreInstr) {
                    Value ptr = ((StoreInstr) ins).getPtr();
                    if (ptr instanceof GEPInstr || ptr instanceof GlobalObject) {
                        return false;
                    }
                    
                }
                ins = (Instruction) ins.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }

        for (Function function : myImmediateCallee) {
            if (function == this) {
                return true;
            } else {
                return function.checkNoSideEffect();
            }
        }

        return true;
    }
    
    public SymTab getFuncSymTab() {
        return funcSymTab;
    }
    
    public boolean isMemorized() {
        return memorized;
    }
    
    public void setMemorized() {
        this.memorized = true;
    }
}
