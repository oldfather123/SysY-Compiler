package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.AllocaInstr;
import ir.instr.BinaInstr;
import ir.instr.LoadInstr;
import ir.instr.StoreInstr;
import ir.type.PointerType;
import ir.value.*;
import pass.Pass;
import utils.AliasContainer;
import utils.Pair;
import utils.Panic;

import java.util.*;

public class AliasAnalysis implements Pass.IrPass {
    @Override
    public String getName() {
        return "alias-analysis";
    }

    public class IndirectPointer {
        public String name;
        public Value value;
        public String basicPointer;
        public int offset;
        public IndirectPointer(String name, Value value, String basicPointer, int offset) {
            this.name = name;
            this.value = value;
            this.basicPointer = basicPointer;
            this.offset = offset;
        }

        @Override
        public String toString() {
            return basicPointer + " + " + offset;
        }

        @Override
        public int hashCode() {
            return toString().hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof IndirectPointer) {
                return this.toString().equals(obj.toString());
            }
            return false;
        }
    }

    public HashSet<String> basicPointers = new HashSet<>();
    public HashMap<String,HashSet<IndirectPointer>> indirectPointers = new HashMap<>();
    public HashMap<String,Value> basicPointerMap = new HashMap<>();
    public HashMap<IndirectPointer,Value> indirectPointerMap = new HashMap<>();
    public HashMap<IndirectPointer,String> indirectToBasic = new HashMap<>();
    public HashMap<Pair<IndirectPointer,String>,Pair<IndirectPointer,String>> aliasMap = new HashMap<>();
    public HashMap<String,IndirectPointer> nameToIndirect = new HashMap<>();
    public HashSet<String> globalBasicPointers = new HashSet<>();
    public HashMap<String,HashSet<String>> functionChangedGlobal = new HashMap<>();

    HashMap<String,Boolean> flag = new HashMap<>();
    HashMap<String,LinkedList<IrInstr>> loadInstrs = new HashMap<>();
    HashMap<String,LinkedList<IrInstr>> storeInstrs = new HashMap<>();
    HashMap<String,HashSet<Integer>> loadInstrsSet = new HashMap<>();
    HashMap<String,HashSet<Integer>> storeInstrsSet = new HashMap<>();
    HashSet<Integer> removeInstrs = new HashSet<>();
    HashMap<String,HashMap<Value,Value>> uselessToUseful = new HashMap<>();

    @Override
    public void run(IrModule module) {
        for (Variable variable : module.globalVariables) {
            basicPointers.add(variable.getMangle());
            basicPointerMap.put(variable.getMangle(), variable);
            globalBasicPointers.add(variable.getMangle());
        }
        for (Constant variable : module.globalConstants) {
            basicPointers.add(variable.getMangle());
            basicPointerMap.put(variable.getMangle(), variable);
            globalBasicPointers.add(variable.getMangle());
        }
        passStepOne(module);
        passStepTwo(module);
//        passStepThree(module);
        AliasContainer.getInstance().initialize(basicPointers, indirectPointers, basicPointerMap,
                indirectPointerMap, indirectToBasic, aliasMap, nameToIndirect,globalBasicPointers, functionChangedGlobal);
    }

    // 这一步中进行别名分析
    private void passStepOne(IrModule module) {
        for (Function function : module.functions) {
            for (Value argument : function.getArguments()) {
                if (argument.getType() instanceof PointerType) {
                    basicPointers.add(argument.getName());
                    globalBasicPointers.add(argument.getName());
                    basicPointerMap.put(argument.getName(), argument);
                }
            }
            for (BasicBlock block : function.getBasicBlocks()) {
                stepOneTackleBlock(block, function.getName(), function);
            }
        }
    }

    private void stepOneTackleBlock(BasicBlock block, String function, Function func) {
        for (IrInstr instr : block.getInstrs()) {
            if (instr instanceof AllocaInstr) {
                basicPointers.add(((AllocaInstr) instr).pointer.getName());
                basicPointerMap.put(((AllocaInstr) instr).pointer.getName(), ((AllocaInstr) instr).pointer);
            }
            else if (instr instanceof BinaInstr) {
                stepOneTackleBinaInstr(instr,function, block);
            }
            else if (instr instanceof LoadInstr) {
                if (((LoadInstr) instr).resultType instanceof PointerType) {
                    String trueName = ((LoadInstr)instr).pointer.getName();
                    // 去掉末尾五个字符
                    trueName = trueName.substring(0, trueName.length() - 5);
                    Value pointer = basicPointerMap.get(trueName);
                    for (IrInstr instr2 : block.getInstrs()) {
                        instr2.resetIntermediate(((LoadInstr) instr).result, pointer);
                    }
                }
            }
        }
    }

    private void stepOneTackleBinaInstr(IrInstr instr, String function, BasicBlock block) {
        if (instr.getOpCode().equals(IrInstr.OpCode.ADD)) {
            Value op1 = instr.getOperands()[1];
            Value op2 = instr.getOperands()[2];
            Value result = instr.getInitialValue();
            if (result.getType() instanceof PointerType) {
                Value fatherPointer = (op1.getType() instanceof PointerType) ? op1 : op2;
                Value offset = (op1.getType() instanceof PointerType) ? op2 : op1;
                int offsetValue = 0;
                if (offset instanceof Literal) {
                    offsetValue = ((Literal) offset).asInt();
                }
                else if (offset instanceof Constant) {
                    offsetValue = (int)(((Constant) offset).getValue());
                }
                else {
//                    basicPointers.add(((BinaInstr) instr).result.getName());
//                    basicPointerMap.put(((BinaInstr) instr).result.getName(), ((BinaInstr) instr).result);
//                    return;
                    offsetValue = -1;
                }
                Value basicPointer;
                if (!basicPointers.contains(fatherPointer.getName())) {
                    IndirectPointer indirectPointer = nameToIndirect.get(fatherPointer.getName());
                    basicPointer = basicPointerMap.get(indirectToBasic.get(indirectPointer));
                    offsetValue += indirectPointer.offset;
                }
                else {
                    basicPointer = fatherPointer;
                    if (offsetValue == 0) {
                        removeInstrs.add(instr.getGlobalInstrId());
                        for (IrInstr instr2 : block.getInstrs()) {
                            if (instr2.getGlobalInstrId() == instr.getGlobalInstrId()) {
                                continue;
                            }
                            instr2.replaceOperand(((BinaInstr) instr).result, fatherPointer);
                        }
                        return;
                    }
                }
                IndirectPointer indirectPointer = new IndirectPointer(result.getName(), result, basicPointer.getName(), offsetValue);
                if (!indirectPointers.containsKey(function)) {
                    indirectPointers.put(function, new HashSet<>());
                }
                nameToIndirect.put(result.getName(), indirectPointer);
                if (!indirectPointers.get(function).contains(indirectPointer)) {
                    indirectPointers.get(function).add(indirectPointer);
                    indirectPointerMap.put(indirectPointer, result);
                    indirectToBasic.put(indirectPointer, basicPointer.getName());
                }
                else {
                    for (IndirectPointer indirectPointer1 : indirectPointers.get(function)) {
                        if (indirectPointer1.equals(indirectPointer)) {
                            aliasMap.put(new Pair<>(indirectPointer, indirectPointer.name),
                                    new Pair<>(indirectPointer1, indirectPointer1.name));
                            break;
                        }
                    }
                }
            }
        }
    }

    private void passStepTwo(IrModule module) {
        for (Function function : module.functions) {
            for (BasicBlock block : function.getBasicBlocks()) {
                stepTwoTackleBlock(block, function.getName());
            }
        }
    }

    private void stepTwoTackleBlock(BasicBlock block, String function) {
        flag.clear();
        loadInstrs.clear();
        storeInstrs.clear();
        for (IrInstr instr : block.getInstrs()) {
            if (instr instanceof StoreInstr) {
                Value value = ((StoreInstr) instr).pointer;
                if (globalBasicPointers.contains(value.getName())) {
                    if (!functionChangedGlobal.containsKey(function)) {
                        functionChangedGlobal.put(function, new HashSet<>());
                    }
                    functionChangedGlobal.get(function).add(value.getName());
                }
                String key = pointerValueToString(((StoreInstr) instr).pointer);
                if (!flag.containsKey(key)) {
                    flag.put(key, false);
                }
                if (!flag.get(key)) {
                    flag.put(key, true);
                    if (!storeInstrs.containsKey(key)) {
                        storeInstrs.put(key, new LinkedList<>());
                    }
                }
                else {
                    IrInstr lastInstr = storeInstrs.get(key).getLast();
                    storeInstrsSet.get(key).remove(lastInstr.getGlobalInstrId());
                    storeInstrs.get(key).removeLast();
                }
                storeInstrs.get(key).add(instr);
                if (!storeInstrsSet.containsKey(key)) {
                    storeInstrsSet.put(key, new HashSet<>());
                }
                storeInstrsSet.get(key).add(instr.getGlobalInstrId());
            }
            else if (instr instanceof LoadInstr) {
                if (((LoadInstr) instr).resultType instanceof PointerType) {
                    continue;
                }
                String key = pointerValueToString(((LoadInstr) instr).pointer);
                if (!flag.containsKey(key)) {
                    flag.put(key, false);
                }
                if (flag.get(key)) {
                    flag.put(key, false);
                    if (!loadInstrs.containsKey(key)) {
                        loadInstrs.put(key, new LinkedList<>());
                    }
                    loadInstrs.get(key).add(instr);
                    if (!loadInstrsSet.containsKey(key)) {
                        loadInstrsSet.put(key, new HashSet<>());
                    }
                    loadInstrsSet.get(key).add(instr.getGlobalInstrId());
                }
                else {
                    if (!loadInstrs.containsKey(key)) {
                        loadInstrs.put(key, new LinkedList<>());
                        loadInstrs.get(key).add(instr);
                        if (!loadInstrsSet.containsKey(key)) {
                            loadInstrsSet.put(key, new HashSet<>());
                        }
                        loadInstrsSet.get(key).add(instr.getGlobalInstrId());
                    }
                    else {
                        Value useful = loadInstrs.get(key).getLast().getInitialValue();
                        Value useless = ((LoadInstr) instr).result;
                        if (!uselessToUseful.containsKey(block.getName())) {
                            uselessToUseful.put(block.getName(), new HashMap<>());
                        }
                        uselessToUseful.get(block.getName()).put(useless, useful);
                    }
                }
            }
        }
    }

    private String pointerValueToString(Value v) {
        if (basicPointers.contains(v.getName())) {
            return v.getName();
        }
        else if (nameToIndirect.containsKey(v.getName())) {
            return nameToIndirect.get(v.getName()).toString();
        }
        else {
            Panic.panicIfFalse(true);
            return null;
        }
    }

    private void passStepThree(IrModule module) {
        for (Function function : module.functions) {
            for (BasicBlock block : function.getBasicBlocks()) {
                stepThreeTackleBlock(block);
            }
        }
    }

    private void stepThreeTackleBlock(BasicBlock block) {
        ArrayList<IrInstr> result = new ArrayList<>();
        for (IrInstr instr : block.getInstrs()) {
            if (removeInstrs.contains(instr.getGlobalInstrId())) {
                continue;
            }
            if (uselessToUseful.containsKey(block.getName())) {
                for (Map.Entry<Value, Value> entry : uselessToUseful.get(block.getName()).entrySet()) {
                    instr.replaceOperand(entry.getKey(), entry.getValue());
                }
            }
            if (!(instr instanceof LoadInstr) && !(instr instanceof StoreInstr)) {
                result.add(instr);
            }
            if (instr instanceof LoadInstr) {
                String key = pointerValueToString(((LoadInstr) instr).pointer);
                if (loadInstrsSet.get(key).contains(instr.getGlobalInstrId())) {
                    result.add(instr);
                    loadInstrsSet.get(key).remove(instr.getGlobalInstrId());
                }
            }
            else if (instr instanceof StoreInstr) {
                String key = pointerValueToString(((StoreInstr) instr).pointer);
                if (storeInstrsSet.get(key).contains(instr.getGlobalInstrId()) &&
                        loadInstrsSet.containsKey(key) &&
                        loadInstrsSet.get(key).size() >= storeInstrsSet.get(key).size()) {
                    result.add(instr);
                }
                storeInstrsSet.get(key).remove(instr.getGlobalInstrId());
            }
        }
        block.resetInstrs(result);
    }
}
