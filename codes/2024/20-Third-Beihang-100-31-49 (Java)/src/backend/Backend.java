package backend;

import backend.instructions.*;
import backend.operand.*;
import backend.tools.BackBlock;
import backend.tools.BackFunction;
import backend.tools.BackGlobalDecl;
import backend.tools.BackModule;
import mid.IntermediatePresentation.*;
import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.*;
import mid.IntermediatePresentation.Module;
import utils.Config;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

import static utils.Runner.output;

public class Backend {
    private Module module;
    private BackModule backModule;
    private BackFunction backFunction;
    private BackBlock backBlock;
    //private HashMap<Value,BackOperand> operandMap;
    private HashMap<String, BackFunction> libFunction;
    private HashMap<String, Integer> stackTable;
    private HashMap<Integer, BackOperand> lc;
    private HashMap<GlobalDecl, BackGlobalDecl> globalMap;
    private HashMap<String, Function> functionMap;

    public Backend(Module module) {
        this.module = module;
        backModule = new BackModule();
        //operandMap = new HashMap<>();
        libFunction = new HashMap<>();
        stackTable = new HashMap<>();
        lc = new HashMap<>(); //全局
        globalMap = new HashMap<>(); //全局
        functionMap = new HashMap<>();
        buildLibFunction();
    }

    private void buildLibFunction() {
        for (Function f : IRManager.getModule().getFunctions()) {
            if (f instanceof LibFunction libf) {
                libFunction.put(libf.getName(), new BackFunction(f));
            }
        }
    }

    public BackModule run() {
        RegisterAlloc.INSTANCE.alloc();
        try {
            output("back.ll", module);
        } catch (Exception ignored) {
        }
        parseGlobalVars();
        parseFunctions();
        return backModule;
    }

    public void parseGlobalVars() {
        for (GlobalDecl globalDecl : module.getGlobalDecls()) {
            BackGlobalDecl backGlobalDecl = parseGlobalVar(globalDecl);
            backModule.addGlobalDecl(backGlobalDecl);
            globalMap.put(globalDecl, backGlobalDecl);
        }
    }

    public BackGlobalDecl parseGlobalVar(GlobalDecl globalDecl) {
        ArrayList<Value> operandList = globalDecl.getOperandList();
        ArrayList<Integer> numbers = new ArrayList<>();
        if (operandList.get(0) instanceof ArrayInitializer aInit) {
            if (aInit.getISZeroInit()) {
                return new BackGlobalDecl(globalDecl.getReg(), 4 * aInit.getLen());
            } else {
                int size = globalDecl.getType().getLength();
                for (int i = 0; i < size; i++) {
                    Value value = aInit.getVals().getOrDefault(i,
                            new ConstNumber(0).withType(globalDecl.getRefType() == ValueType.I32));
                    if (value.getType() == ValueType.FLT) {
                        float floatValue = ((ConstNumber) value).getVal().floatValue();
                        int intValue = Float.floatToRawIntBits(floatValue);
                        numbers.add(intValue);
                    } else {
                        numbers.add(Integer.parseInt(value.getReg()));
                    }
                }
                return new BackGlobalDecl(globalDecl.getReg(), numbers);
            }
        } else {
            if (operandList.get(0).getType() == ValueType.I32) {
                numbers.add(Integer.parseInt(globalDecl.getInit().getReg()));
                return new BackGlobalDecl(globalDecl.getReg(), numbers);
            } else if (operandList.get(0).getType() == ValueType.FLT) {
                float floatValue = ((ConstNumber) globalDecl.getInit()).getVal().floatValue();
                int intValue = Float.floatToRawIntBits(floatValue);
                numbers.add(intValue);
                return new BackGlobalDecl(globalDecl.getReg(), numbers);
            }
        }
        return null;
    }

    public void parseFunctions() {
        for (Function function : module.getDecledFunctions()) {
            for (BasicBlock b : function.getBlocks()) {
                backModule.midBlockToBack.put(b, new BackBlock(b));
            }
            parseFunction(function);
        }
    }

    public void parseFunction(Function function) {
        backFunction = new BackFunction(function);
        stackTable = new HashMap<>();
        if (!libFunction.containsKey("@" + function.getName())) {
            functionMap.put(function.getName(), function);
        }
        //先计算栈空间，只有alloca和call需要
        RegisterValueMap.instance().setCurFunction(function);
        HashSet<BackIReg> regs = new HashSet<>(RegisterValueMap.instance().getAllRegs().values());
        int totalT = 0;
        int totalA = 0;
        int maxParamStack = -8;
        for (BasicBlock block : function.getBlocks()) {
            for (Instruction instruction : block.getInstructionList()) {
                if (instruction instanceof Alloca) {
                    if (instruction.getType().equals(ValueType.ARRAY)) {
                        backFunction.setS(instruction.getRefType().getByteLen() * ((Alloca) instruction).getLen());
                    } else {
                        backFunction.setS(instruction.getRefType().getByteLen());
                    }
                } else if (instruction instanceof Call call) {
                    backFunction.setR(); //记录当前函数中的call数
                    Function f = call.getCallingFunction();
                    ArrayList<Value> operandList = f.getParam().getParams();
                    ArrayList<Value> intParams = new ArrayList<>();
                    ArrayList<Value> floatParams = new ArrayList<>();
                    HashSet<BackIReg> regsToDestroy;
                    regsToDestroy = new HashSet<>(BackRegTable.getAllTempRegs());
                    regsToDestroy.addAll(BackFRegTable.getAllTempRegs());
                    int t = 0;
                    for (BackIReg backIReg : regs) {
                        if (!backIReg.isGlobalRegs()) {
                            if (regsToDestroy.contains(backIReg)) {
                                t = t + 1;
                            }
                        }
                    }
                    if (t > totalT) {
                        totalT = t;
                    }

                    if (operandList.size() != 1) { //有参数
                        for (Value value : operandList) {
                            if (!(value instanceof Function)) {
                                if (value.getType() == ValueType.I32 || value.isPointer()) {
                                    intParams.add(value);
                                } else {
                                    floatParams.add(value);
                                }
                            }
                        }
                        int total_intParams = intParams.size();
                        int total_floatParams = floatParams.size();
                        int functionA = Math.max(total_intParams, total_floatParams);
                        backFunction.setA(functionA); //记录最大参数个数
                        totalA = Math.max(totalA, functionA);
                    }
                } else if (instruction instanceof Push push) {
                    maxParamStack = Math.max(maxParamStack, push.getOffset());
                }
            }
        }
        backFunction.setCurStack(maxParamStack + 8);
        /*RegisterValueMap.instance().setCurFunction(function);
        HashSet<BackIReg> regs = new HashSet<>(RegisterValueMap.instance().getActiveRegs().values());*/
        for (BackIReg backIReg : regs) { //需要保存的寄存器
            if (backIReg.getName().charAt(0) == 's') {
                backFunction.setS(8);
            } else if (backIReg.getName().charAt(1) == 's') {
                backFunction.setS(4);
            }
        }
        backFunction.setS(8 * totalT); //最多的t
        backFunction.setS(8 * totalA);

        // 计算结束，进入函数
        for (BasicBlock b : function.getBlocks()) {
            parseBlock(b);
        }

        HashMap<BackIReg, Integer> saveRecord = new HashMap<>();
        // ra占8B，自己占8，从-16开始
        int n = backFunction.getStackSize() - 8;

       /* for (int i = size - 1;i >=0 ;i--) {
            Value value = function.getParam().getParams().get(i);
            int off = n;
            n = n - 8;
            BackOperand backOperand = parseOperand(value,0,null);
            BackStore backStore = new BackStore("sw",backOperand,new BackImm(off),BackRegTable.getBackReg("sp"));
            backFunction.addFirstInstruction(backStore);
            saveRecord.put(new BackIReg(backOperand.toString()),off);
        }*/
        // prologue：保存寄存器
        for (BackIReg backIReg : regs) { //需要保存的寄存器
            if (backIReg.getName().charAt(0) == 's') {
                n = n - 8;
                int off = n;
                BackStore backStore = new BackStore("sd", BackRegTable.getBackReg(backIReg.getName()), new BackImm(off), BackRegTable.getBackReg("sp"));
                backFunction.addFirstInstruction(backStore);
                saveRecord.put(backIReg, off);
            } else if (backIReg.getName().charAt(1) == 's') {
                n = n - 4;
                int off = n;
                BackStore backStore = new BackStore("fsw", BackFRegTable.getBackReg(backIReg.getName()), new BackImm(off), BackRegTable.getBackReg("sp"));
                backFunction.addFirstInstruction(backStore);
                saveRecord.put(backIReg, off);
            }
        }

        // epilogue：恢复寄存器
        for (BackIReg backIReg : saveRecord.keySet()) {
            if (backIReg.getName().charAt(0) == 's' || (backIReg.getName().charAt(0) == 'a' && !backIReg.getName().equals("a0"))) {
                BackLoad backLoad = new BackLoad("ld", BackRegTable.getBackReg(backIReg.getName()), new BackImm(saveRecord.get(backIReg)), BackRegTable.getBackReg("sp"));
                backFunction.addBefore(backLoad);
            } else if (backIReg.getName().charAt(1) == 's' || (backIReg.getName().charAt(1) == 'a' && !backIReg.getName().equals("fa0"))) {
                BackLoad backLoad = new BackLoad("flw", BackFRegTable.getBackReg(backIReg.getName()), new BackImm(saveRecord.get(backIReg)), BackRegTable.getBackReg("sp"));
                backFunction.addBefore(backLoad);
            }
        }


        if (backFunction.getStackSize() != 0) {
            BackOperand stackOffset = parseConstInt(-backFunction.getStackSize(), 12);
            if (backFunction.hasCall()) {
                BackOperand stackSize = new BackImm(backFunction.getStackSize() - 8);
                BackStore backStore = new BackStore("sd", BackRegTable.getBackReg("ra"), stackSize, BackRegTable.getBackReg("sp"));
                backFunction.addFirstInstruction(backStore);
            }
            BackInstruction changeStack;
            if (stackOffset instanceof BackImm) {
                changeStack = new BackBinary("addi", BackRegTable.getBackReg("sp"), BackRegTable.getBackReg("sp"), stackOffset);
            } else {
                changeStack = new BackBinary("add", BackRegTable.getBackReg("sp"), BackRegTable.getBackReg("sp"), stackOffset);
            }
            backFunction.addFirstInstruction(changeStack);
            if (backFunction.hasCall()) {
                BackOperand stackSize = new BackImm(backFunction.getStackSize() - 8);
                BackLoad backLoad = new BackLoad("ld", BackRegTable.getBackReg("ra"), stackSize, BackRegTable.getBackReg("sp"));
                backFunction.addBefore(backLoad);
            }
            BackOperand stackOffsetBack = parseConstInt(backFunction.getStackSize(), 12);
            if (stackOffsetBack instanceof BackImm) {
                BackInstruction recoverStack = new BackBinary("addi", BackRegTable.getBackReg("sp"), BackRegTable.getBackReg("sp"), stackOffsetBack);
                backFunction.addBefore(recoverStack);
            } else {
                BackInstruction recoverStack = new BackBinary("add", BackRegTable.getBackReg("sp"), BackRegTable.getBackReg("sp"), stackOffsetBack);
                backFunction.addBefore(recoverStack);
            }
        }
        backModule.addFunction(backFunction);
    }

    public void parseBlock(BasicBlock b) {
        backBlock = backModule.midBlockToBack.get(b);
        for (Instruction instruction : b.getInstructionList()) {
            parseInstruction(instruction);
            if (instruction instanceof Ret) {
                break;
            }
        }
        backFunction.addBackBlock(backBlock);
    }

    public void parseInstruction(Instruction instruction) {
        if (Config.debug) {
            backBlock.addInstruction(new BackAnnotation(instruction.toString()));
        }
        if (instruction instanceof Ret) {
            parseRetInstr(instruction);
            backFunction.addRetBlocks(backBlock);
        } else if (instruction instanceof Call) {
            parseCallInstr(instruction);
        } else if (instruction instanceof ALU) {
            switch (((ALU) instruction).getAluType()) {
                case "add" -> parseAddInstr(instruction);
                case "fadd" -> parseFAddInstr(instruction);
                case "sub" -> parseSubInstr(instruction);
                case "fsub" -> parseFSubInstr(instruction);
                case "mul" -> parseMulInstr(instruction);
                case "fmul" -> parseFMulInstr(instruction);
                case "sdiv" -> parseswivInstr(instruction);
                case "fdiv" -> parseFDivInstr(instruction);
                case "srem" -> parseSremInstr(instruction);
                case "frem" -> parseFremInstr(instruction); //这个有问题,float % float ?
            }
        } else if (instruction instanceof Alloca) {
            parseAllocaInstr(instruction);
        } else if (instruction instanceof Store) {
            parseStoreInstr(instruction);
        } else if (instruction instanceof Load) {
            parseLoadInstr(instruction);
        } else if (instruction instanceof Shift) {
            if (((Shift) instruction).isShiftRight()) {
                parseAshrInstr(instruction);
            } else {
                parseShlInstr(instruction);
            }
        } else if (instruction instanceof Cmp) {
            parseCmpInstr(instruction);
        } else if (instruction instanceof Br) {
            parseBrInstr(instruction);
        } else if (instruction instanceof Fptosi) {
            parseFptosiInstr(instruction);
        } else if (instruction instanceof Sitofp) {
            parseSitofpInstr(instruction);
        } else if (instruction instanceof ZextTo) {
            parseZextToInstr(instruction);
        } else if (instruction instanceof GetElementPtr) {
            parseGetElementPtr(instruction);
        } else if (instruction instanceof MoveIR) {
            parseMove(instruction);
        } else if (instruction instanceof Push push) {
            parsePush(push);
        } else if (instruction instanceof GetParam getParam) {
            parseGetParam(getParam);
        } else {
            System.out.println(instruction);
        }
    }

    public void parseAllocaInstr(Instruction instruction) {
        stackTable.put(instruction.getReg(), backFunction.getCurStack());
        if (instruction.getType().equals(ValueType.ARRAY)) {
            backFunction.setCurStack(instruction.getRefType().getByteLen() * ((Alloca) instruction).getLen());
        } else {
            backFunction.setCurStack(instruction.getRefType().getByteLen());
        }
    }

    public void parseLoadInstr(Instruction instruction) {
        if (instruction.getOperandList().get(0) instanceof GlobalDecl) {
            BackOperand tmp = BackRegTable.getBackReg("t6");
            BackOperand dst = getDstOperand(instruction);
            BackLoad backLoad = new BackLoad("la", tmp, parseOperand(instruction.getOperandList().get(0), 0, null));
            backBlock.addInstruction(backLoad);
            if (dst.toString().charAt(0) == 'f') {
                BackLoad backLoad1 = new BackLoad("flw", dst, new BackImm12(0), tmp);
                backBlock.addInstruction(backLoad1);
            } else {
                BackLoad backLoad1 = new BackLoad("lw", dst, new BackImm12(0), tmp);
                backBlock.addInstruction(backLoad1);
            }
//            if (stackTable.containsKey(instruction.getReg())) {
//                saveInStack(dst, instruction);
//            }
        } else if (stackTable.containsKey(instruction.getOperandList().get(0).getReg())) {
            BackOperand dst = getDstOperand(instruction);
            if (instruction.isPointer()) {
                BackLoad backLoad = new BackLoad("ld", dst, new BackImm12(stackTable.get(instruction.getOperandList().get(0).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
            } else if (instruction.getOperandList().get(0).getRefType() == ValueType.I32) {
                BackLoad backLoad = new BackLoad("lw", dst, new BackImm12(stackTable.get(instruction.getOperandList().get(0).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
            } else {
                BackLoad backLoad = new BackLoad("flw", dst, new BackImm12(stackTable.get(instruction.getOperandList().get(0).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
            }
//            if (stackTable.containsKey(instruction.getReg())) {
//                saveInStack(dst, instruction);
//            }
        } else {
            BackOperand dst = getDstOperand(instruction);
            if (instruction.isPointer()) {
                BackLoad backLoad = new BackLoad("ld", dst, new BackImm(0), parseOperand(instruction.getOperandList().get(0), 0, instruction));
                backBlock.addInstruction(backLoad);
            } else if (instruction.getOperandList().get(0).getRefType() == ValueType.FLT) {
                BackLoad backLoad = new BackLoad("flw", dst, new BackImm(0), parseOperand(instruction.getOperandList().get(0), 0, instruction));
                backBlock.addInstruction(backLoad);
            } else {
                BackLoad backLoad = new BackLoad("lw", dst, new BackImm(0), parseOperand(instruction.getOperandList().get(0), 0, instruction));
                backBlock.addInstruction(backLoad);
            }
//            if (stackTable.containsKey(instruction.getReg())) {
//                saveInStack(dst, instruction);
//            }
        }
    }

    public void parseStoreInstr(Instruction instruction) { //参数后开始存
        if (instruction.getOperandList().get(1) instanceof GlobalDecl) {
            BackOperand tmp = BackRegTable.getBackReg("t6");
            BackOperand dst = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackLoad backLoad = new BackLoad("la", tmp, parseOperand(instruction.getOperandList().get(1), 0, null));
            backBlock.addInstruction(backLoad);
            if (dst.toString().charAt(0) == 'f') {
                BackLoad backLoad1 = new BackLoad("fsw", dst, new BackImm12(0), tmp);
                backBlock.addInstruction(backLoad1);
            } else {
                BackStore backStore = new BackStore("sw", dst, new BackImm12(0), tmp);
                backBlock.addInstruction(backStore);
            }
        } else if (stackTable.containsKey(instruction.getOperandList().get(1).getReg())) {
            if (instruction.getOperandList().get(0).isPointer()) {
                BackStore backStore = new BackStore("sd", parseOperand(instruction.getOperandList().get(0), 0, instruction),
                        new BackImm12(stackTable.get(instruction.getOperandList().get(1).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            } else if (instruction.getOperandList().get(1).getRefType() == ValueType.FLT) {
                BackStore backStore = new BackStore("fsw", parseOperand(instruction.getOperandList().get(0), 0, instruction),
                        new BackImm12(stackTable.get(instruction.getOperandList().get(1).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            } else {
                BackStore backStore = new BackStore("sw", parseOperand(instruction.getOperandList().get(0), 0, instruction),
                        new BackImm12(stackTable.get(instruction.getOperandList().get(1).getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            }
        } else {
            if (instruction.getOperandList().get(0).isPointer()) {
                BackStore backStore = new BackStore("sd", parseOperand(instruction.getOperandList().get(0), 0, instruction), new BackImm(0), parseOperand(instruction.getOperandList().get(1), 0, instruction));
                backBlock.addInstruction(backStore);
            } else if (instruction.getOperandList().get(1).getRefType() == ValueType.FLT) {
                BackStore backStore = new BackStore("fsw", parseOperand(instruction.getOperandList().get(0), 0, instruction), new BackImm(0), parseOperand(instruction.getOperandList().get(1), 0, instruction));
                backBlock.addInstruction(backStore);
            } else {
                BackStore backStore = new BackStore("sw", parseOperand(instruction.getOperandList().get(0), 0, instruction), new BackImm(0), parseOperand(instruction.getOperandList().get(1), 0, instruction));
                backBlock.addInstruction(backStore);
            }

        }
    }

    public void parseCallInstr(Instruction instruction) {
        ArrayList<Value> operandList = instruction.getOperandList();
        //backFunction.setR(); //记录当前函数中的call数

        //临时寄存器保存
        HashMap<BackIReg, Integer> saveRecord = new HashMap<>();
        HashSet<BackIReg> regs = new HashSet<>(RegisterValueMap.instance().getActiveRegs((Call) instruction));

        HashSet<BackIReg> regsToDestroy;
        regsToDestroy = new HashSet<>(BackRegTable.getAllTempRegs());
        regsToDestroy.addAll(BackFRegTable.getAllTempRegs());

        for (BackIReg backIReg : regs) {
            if (!backIReg.getName().startsWith("f") && !backIReg.isGlobalRegs()) {
                if (regsToDestroy.contains(backIReg)) {
                    int off = backFunction.getCurStack();
                    backFunction.setCurStack(8);
                    BackStore backStore = new BackStore("sd", BackRegTable.getBackReg(backIReg.getName()), new BackImm(off), BackRegTable.getBackReg("sp"));
                    backBlock.addInstruction(backStore);
                    saveRecord.put(backIReg, off);
                }
            } else if (backIReg.getName().startsWith("f") && !backIReg.isGlobalRegs()) {
                if (regsToDestroy.contains(backIReg)) {
                    int off = backFunction.getCurStack();
                    backFunction.setCurStack(4);
                    BackStore backStore = new BackStore("fsw", BackFRegTable.getBackReg(backIReg.getName()), new BackImm(off), BackRegTable.getBackReg("sp"));
                    backBlock.addInstruction(backStore);
                    saveRecord.put(backIReg, off);
                }
            }
        }

        // 传参
        ArrayList<Value> intParams = new ArrayList<>();
        ArrayList<Value> floatParams = new ArrayList<>();
        if (operandList.size() != 1) { //有参数
            for (Value value : operandList) {
                if (!(value instanceof Function)) {
                    if (value.getType() == ValueType.FLT) {
                        floatParams.add(value);
                    } else {
                        intParams.add(value);
                    }
                }
            }
            int total_intParams = intParams.size();
            int total_floatParams = floatParams.size();
            /*if (total_intParams > total_floatParams) {
                  backFunction.setA(total_intParams); //记录最大参数个数
               } else {
                   backFunction.setA(total_floatParams);
            }*/
            if (total_intParams > 8) { //大于8存入堆栈
                total_intParams = 8;
            }
            if (total_floatParams > 8) {
                total_floatParams = 8;
            }
            // 开始设置参数
            for (int i = 0; i < total_intParams; i++) {
                BackOperand backOperand = parseOperand(intParams.get(i), 32, null);
                if (!stackTable.containsKey(intParams.get(i).getReg())) {
                    if (backOperand.toString().equals("a" + i)) {
                        continue;
                    } else if (backOperand.toString().charAt(0) == 'a' &&
                            Integer.parseInt(backOperand.toString().substring(1)) < i
                            && saveRecord.containsKey(backOperand)) {
                        // 如果要传递的参数寄存器已经被破坏了，则只能向栈中去取
                        BackLoad backLoad = new BackLoad("ld", BackRegTable.getBackReg("a" + i),
                                new BackImm(saveRecord.get(backOperand)), BackRegTable.getBackReg("sp"));
                        backBlock.addInstruction(backLoad);
                    } else {
                        BackMov backMov = new BackMov(BackRegTable.getBackReg("a" + i), backOperand);
                        backBlock.addInstruction(backMov);
                    }
                } else {
                    BackMov backMov = new BackMov(BackRegTable.getBackReg("a" + i), backOperand);
                    backBlock.addInstruction(backMov);
                }
            }

            for (int i = 0; i < total_floatParams; i++) {
                BackOperand backOperand = parseOperand(floatParams.get(i), 32, null);
                if (!stackTable.containsKey(floatParams.get(i).getReg())) {
                    if (backOperand.toString().equals("fa" + i)) {
                        continue;
                    } else if (backOperand.toString().charAt(1) == 'a' &&
                            Integer.parseInt(backOperand.toString().substring(2)) < i
                            && saveRecord.containsKey(backOperand)) {
                        BackLoad backLoad = new BackLoad("flw", BackFRegTable.getBackReg("fa" + i),
                                new BackImm(saveRecord.get(backOperand)), BackRegTable.getBackReg("sp"));
                        backBlock.addInstruction(backLoad);
                    } else {
                        BackMov backMov = new BackMov(BackFRegTable.getBackReg("fa" + i), backOperand);
                        backBlock.addInstruction(backMov);
                    }
                } else {
                    BackMov backMov = new BackMov(BackFRegTable.getBackReg("fa" + i), backOperand);
                    backBlock.addInstruction(backMov);
                }
            }
        }
        // call
        BackCall backCall = new BackCall(operandList.get(0).getName());
        backBlock.addInstruction(backCall);
        // 保存返回值
        if (instruction.getType() != ValueType.NULL) {
            BackOperand dst = parseOperand(instruction, 0, instruction);
            if (instruction.getType() == ValueType.I32) {
                BackMov backMov = new BackMov(dst, BackRegTable.getBackReg("a0"));
                backBlock.addInstruction(backMov);
            } else if (instruction.getType() == ValueType.FLT) {
                BackMov backMov = new BackMov(dst, BackFRegTable.getBackReg("fa0"));
                backBlock.addInstruction(backMov);
            }
        }
        for (BackIReg backIReg : saveRecord.keySet()) {
            if (instruction.getType() != ValueType.NULL) {
                if (backIReg.getName().equals(parseOperand(instruction, 0, instruction).toString())) {
                    backFunction.setCurStack(-backIReg.getRegMaxByteWidth());
                    continue;
                }
            }
            if (!backIReg.getName().startsWith("f") && !backIReg.isGlobalRegs()) {
                BackLoad backLoad = new BackLoad("ld", BackRegTable.getBackReg(backIReg.getName()), new BackImm(saveRecord.get(backIReg)), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                backFunction.setCurStack(-8);
            } else if (backIReg.getName().startsWith("f") && !backIReg.isGlobalRegs()) {
                BackLoad backLoad = new BackLoad("flw", BackFRegTable.getBackReg(backIReg.getName()), new BackImm(saveRecord.get(backIReg)), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                backFunction.setCurStack(-4);
            }
        }
    }

    public void parseRetInstr(Instruction instruction) {
        if (instruction.getType() == ValueType.NULL) {
            backBlock.addInstruction(new BackRet());
        } else {
            if (instruction.getOperandList().get(0).getType() == ValueType.FLT) {
                BackOperand operand = parseOperand(instruction.getOperandList().get(0), 32, null);
                BackMov backMov = new BackMov(BackFRegTable.getBackReg("fa0"), operand); //暂时使用t
                backBlock.addInstruction(backMov);
                backBlock.addInstruction(new BackRet());
            } else {
                BackOperand operand = parseOperand(instruction.getOperandList().get(0), 32, null);
                BackMov backMov = new BackMov(BackRegTable.getBackReg("a0"), operand); //暂时使用t
                backBlock.addInstruction(backMov);
                backBlock.addInstruction(new BackRet());
            }
        }
    }

    public void parseAddInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        if (left instanceof ConstNumber) {
            if (right instanceof ConstNumber) {
                int sum = ((int) ((ConstNumber) left).getVal()) + ((int) ((ConstNumber) right).getVal());
                BackMov backMov = new BackMov(dst, new BackImm(sum));
                backBlock.addInstruction(backMov);
            } else {
                BackBinary backBinary = new BackBinary("addw", dst, parseOperand(right, 0, null), parseOperand(left, 12, instruction));
                backBlock.addInstruction(backBinary);
            }
        } else {
            if (right instanceof ConstNumber) {
                BackBinary backBinary = new BackBinary("addw", dst, parseOperand(left, 32, null), parseOperand(right, 12, instruction));
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("addw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
                backBlock.addInstruction(backBinary);
            }
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseFAddInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        BackBinary backBinary = new BackBinary("fadd.s", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseSubInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        if (left instanceof ConstNumber && right instanceof ConstNumber) {
            int sub = ((int) ((ConstNumber) left).getVal()) - ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(sub));
            backBlock.addInstruction(backMov);
        } else {
            if (right instanceof ConstNumber) {
                int rightNumber = -((int) ((ConstNumber) right).getVal());
                BackOperand src = parseOperand(right, 0, instruction);
                if (src instanceof BackImm12) {
                    BackBinary backBinary = new BackBinary("addw", dst, parseOperand(left, 0, null), new BackImm12(rightNumber));
                    backBlock.addInstruction(backBinary);
                } else {
                    BackBinary backBinary = new BackBinary("subw", dst, parseOperand(left, 0, instruction), src);
                    backBlock.addInstruction(backBinary);
                }

            } else {
                BackBinary backBinary = new BackBinary("subw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
                backBlock.addInstruction(backBinary);
            }
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseFSubInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        BackBinary backBinary = new BackBinary("fsub.s", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseMulInstr(Instruction instruction) { //现有乘法只支持32位乘法
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        if (left instanceof ConstNumber && right instanceof ConstNumber) {
            int mul = ((int) ((ConstNumber) left).getVal()) * ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(mul));
            backBlock.addInstruction(backMov);
            if (stackTable.containsKey(instruction.getReg())) {
                saveInStack(dst, instruction);
            }
            return;
        }

        if (((ALU) instruction).getResFromHi()) {
            BackBinary backBinary = new BackBinary("mul", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
            backBlock.addInstruction(backBinary);
            backBinary = new BackBinary("srli", dst, dst, new BackImm(32));
            backBlock.addInstruction(backBinary);
        } else {
            BackBinary backBinary = new BackBinary("mulw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
            backBlock.addInstruction(backBinary);
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseFMulInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        BackBinary backBinary = new BackBinary("fmul.s", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseswivInstr(Instruction instruction) {//现有除法只支持32位除法
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        if (left instanceof ConstNumber && right instanceof ConstNumber) {
            int div = ((int) ((ConstNumber) left).getVal()) / ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(div));
            backBlock.addInstruction(backMov);
            if (stackTable.containsKey(instruction.getReg())) {
                saveInStack(dst, instruction);
            }
            return;
        }
        BackBinary backBinary = new BackBinary("divw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseFDivInstr(Instruction instruction) {
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        BackBinary backBinary = new BackBinary("fdiv.s", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseSremInstr(Instruction instruction) {//现有模只支持32位模
        Value left = ((ALU) instruction).getOperand1();
        Value right = ((ALU) instruction).getOperand2();
        BackOperand dst = getDstOperand(instruction);
        if (left instanceof ConstNumber && right instanceof ConstNumber) {
            int srem = ((int) ((ConstNumber) left).getVal()) % ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(srem));
            backBlock.addInstruction(backMov);
            if (stackTable.containsKey(instruction.getReg())) {
                saveInStack(dst, instruction);
            }
            return;
        }
        BackBinary backBinary = new BackBinary("remw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
        backBlock.addInstruction(backBinary);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseFremInstr(Instruction instruction) {
        //todo mod of float
    }

    public void parseShlInstr(Instruction instruction) {//现有模只支持32位模
        Value left = instruction.getOperandList().get(0);
        Value right = instruction.getOperandList().get(1);
        BackOperand dst = getDstOperand(instruction);
        if ((left instanceof ConstNumber) && (right instanceof ConstNumber)) {
            int ans = ((int) ((ConstNumber) left).getVal()) << ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(ans));
            backBlock.addInstruction(backMov);
        } else {
            if (right instanceof ConstNumber) {
                BackBinary backBinary = new BackBinary("slliw", dst, parseOperand(left, 0, instruction), parseOperand(right, 32, null));
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("sllw", dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
                backBlock.addInstruction(backBinary);
            }
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseAshrInstr(Instruction instruction) {//现有模只支持32位模
        Value left = instruction.getOperandList().get(0);
        Value right = instruction.getOperandList().get(1);
        BackOperand dst = getDstOperand(instruction);
        if ((left instanceof ConstNumber) && (right instanceof ConstNumber)) {
            int ans = ((int) ((ConstNumber) left).getVal()) >> ((int) ((ConstNumber) right).getVal());
            BackMov backMov = new BackMov(dst, new BackImm(ans));
            backBlock.addInstruction(backMov);
        } else {
            if (right instanceof ConstNumber) {
                String name;
                if (((Shift) instruction).getLogicalShiftRight()) {
                    name = "srliw";
                } else {
                    name = "sraiw";
                }
                BackBinary backBinary = new BackBinary(name, dst, parseOperand(left, 0, instruction), parseOperand(right, 32, null));
                backBlock.addInstruction(backBinary);
            } else {
                String name;
                if (((Shift) instruction).getLogicalShiftRight()) {
                    name = "srlw";
                } else {
                    name = "sraw";
                }
                BackBinary backBinary = new BackBinary(name, dst, parseOperand(left, 0, instruction), parseOperand(right, 0, instruction));
                backBlock.addInstruction(backBinary);
            }
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseCmpInstr(Instruction instruction) {
        String cond = ((Cmp) instruction).getCond();
        BackOperand dst = getDstOperand(instruction);
        if (cond.equals("slt")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 12, null);
            if (right instanceof BackImm12) {
                BackBinary backBinary = new BackBinary("slti", dst, left, right);
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("slt", dst, left, right);
                backBlock.addInstruction(backBinary);
            }
        } else if (cond.equals("olt")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("flt.s", dst, left, right);
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("sle")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 12, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            if (left instanceof BackImm12) {
                BackBinary backBinary = new BackBinary("slti", dst, right, left);
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("slt", dst, right, left);
                backBlock.addInstruction(backBinary);
            }
            BackBinary backBinary = new BackBinary("xori", dst, dst, new BackImm12(1));
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("ole")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("fle.s", dst, left, right);
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("sgt")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 12, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            if (left instanceof BackImm12) {
                BackBinary backBinary = new BackBinary("slti", dst, right, left);
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("slt", dst, right, left);
                backBlock.addInstruction(backBinary);
            }
        } else if (cond.equals("ogt")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("fgt.s", dst, left, right);
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("sge")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 12, null);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            if (right instanceof BackImm12) {
                BackBinary backBinary = new BackBinary("slti", dst, left, right);
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("slt", dst, left, right);
                backBlock.addInstruction(backBinary);
            }
            BackBinary backBinary = new BackBinary("xori", dst, dst, new BackImm12(1));
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("oge")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("fge.s", dst, left, right);
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("ne") || cond.equals("eq")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 12, null);
            if (right instanceof BackImm12) {
                BackBinary backBinary = new BackBinary("xori", dst, left, right);
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("xor", dst, left, right);
                backBlock.addInstruction(backBinary);
            }
            if (cond.equals("eq")) {
                BackBinary backBinary = new BackBinary("sltu", dst, BackRegTable.getBackReg("zero"), dst); //0 < xor
                backBlock.addInstruction(backBinary);
                backBinary = new BackBinary("xori", dst, dst, new BackImm12(1));
                backBlock.addInstruction(backBinary);
            } else {
                BackBinary backBinary = new BackBinary("sltu", dst, BackRegTable.getBackReg("zero"), dst); //0 < xor
                backBlock.addInstruction(backBinary);
            }
        } else if (cond.equals("one")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("feq.s", dst, left, right);
            backBlock.addInstruction(backBinary);
            backBinary = new BackBinary("xori", dst, dst, new BackImm12(1));
            backBlock.addInstruction(backBinary);
        } else if (cond.equals("oeq")) {
            BackOperand left = parseOperand(instruction.getOperandList().get(0), 0, instruction);
            BackOperand right = parseOperand(instruction.getOperandList().get(1), 0, instruction);
            BackBinary backBinary = new BackBinary("feq.s", dst, left, right);
            backBlock.addInstruction(backBinary);
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseBrInstr(Instruction instruction) {
        if (instruction.getOperandList().size() == 3) {
            Value cond = instruction.getOperandList().get(0);
            Value trueBlock = instruction.getOperandList().get(1);
            Value falseBlock = instruction.getOperandList().get(2);
            if (cond instanceof ConstNumber) {
                int number = (Integer) ((ConstNumber) cond).getVal();
                if (number == 0) {
                    BackBranch backBranch = new BackBranch(backModule.midBlockToBack.get(falseBlock));
                    backBlock.addInstruction(backBranch);
                } else {
                    BackBranch backBranch = new BackBranch(backModule.midBlockToBack.get(trueBlock));
                    backBlock.addInstruction(backBranch);
                }
            } else {
                BackOperand c = parseOperand(cond, 0, instruction);
                BackBranch backBranch = new BackBranch("bnez", c, backModule.midBlockToBack.get(trueBlock));
                backBlock.addInstruction(backBranch);
                backBranch = new BackBranch(backModule.midBlockToBack.get(falseBlock));
                backBlock.addInstruction(backBranch);
            }
        } else {
            BackBranch backBranch = new BackBranch(backModule.midBlockToBack.get(((Br) instruction).getDest()));
            backBlock.addInstruction(backBranch);
        }
    }

    public void parseFptosiInstr(Instruction instruction) {
        Value src = instruction.getOperandList().get(0);
        BackOperand dst = getDstOperand(instruction);
        BackConversion backConversion = new BackConversion(dst, parseOperand(src, 0, instruction), 1);
        backBlock.addInstruction(backConversion);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseSitofpInstr(Instruction instruction) {
        Value src = instruction.getOperandList().get(0);
        BackOperand dst = getDstOperand(instruction);
        BackConversion backConversion = new BackConversion(dst, parseOperand(src, 0, instruction), 0);
        backBlock.addInstruction(backConversion);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseZextToInstr(Instruction instruction) {
        Value src = instruction.getOperandList().get(0);
        BackOperand dst = getDstOperand(instruction);
        BackMov backMov = new BackMov(dst, parseOperand(src, 0, instruction));
        backBlock.addInstruction(backMov);
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, instruction);
        }
    }

    public void parseMove(Instruction instruction) {
        Value src = instruction.getOperandList().get(0);
        BackOperand dst = getDstOperand(((MoveIR) instruction).getOriginPhi());
        if (src instanceof ConstNumber) {
            if (src.getType() == ValueType.FLT) {
                float floatValue = ((ConstNumber) src).getVal().floatValue();
                int intValue = Float.floatToRawIntBits(floatValue);
                BackOperand tmp = BackRegTable.getBackReg("t6");
                BackOperand initValue = parseConstFloat(intValue);
                BackLoad l1 = new BackLoad("lla", tmp, initValue);
                backBlock.addInstruction(l1);
                BackLoad backLoad = new BackLoad("flw", dst, new BackImm12(0), tmp);
                backBlock.addInstruction(backLoad);
            } else {
                BackMov backMov = new BackMov(dst, new BackImm(Integer.parseInt(src.getReg())));
                backBlock.addInstruction(backMov);
            }
        } else {
            BackMov backMov = new BackMov(dst, parseOperand(src, 32, instruction));
            backBlock.addInstruction(backMov);
        }
        if (stackTable.containsKey(instruction.getReg())) {
            saveInStack(dst, ((MoveIR) instruction).getOriginPhi());
        }
    }

    public BackOperand parseOperand(Value value, int length, Instruction instruction) {
        if (value instanceof GlobalDecl) {
            return parseGlobalDecl((GlobalDecl) value);
        }

        if (RegisterValueMap.instance().getRegOf(value) != null) {
            return RegisterValueMap.instance().getRegOf(value);
        }
        /*if (operandMap.containsKey(value)) { //寄存器
            BackOperand backOperand = operandMap.get(value);
            return backOperand;
        }*/
        if (stackTable.containsKey(value.getReg())) {
            if (value.getType() == ValueType.FLT) {
                BackLoad backLoad = new BackLoad("flw", BackFRegTable.getBackReg("ft11"), new BackImm(stackTable.get(value.getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                return BackFRegTable.getBackReg("ft11");
            } else {
                BackLoad backLoad = new BackLoad("lw", BackRegTable.getBackReg("t6"), new BackImm(stackTable.get(value.getReg())), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                return BackRegTable.getBackReg("t6");
            }
        }

        if (value instanceof ConstNumber) {
            if (value.getType() == ValueType.FLT) {
                float floatValue = ((ConstNumber) value).getVal().floatValue();
                int intValue = Float.floatToRawIntBits(floatValue);
                BackOperand tmp = BackRegTable.getBackReg("t6");
                BackOperand initValue = parseConstFloat(intValue);
                BackLoad l1 = new BackLoad("lla", tmp, initValue);
                backBlock.addInstruction(l1);
                BackOperand src = getDstOperand(value);
                BackLoad backLoad = new BackLoad("flw", src, new BackImm12(0), tmp);
                backBlock.addInstruction(backLoad);
                return src;
            } else {
                return parseConstInt((Integer) ((ConstNumber) value).getVal(), length);
            }
        }
        return getDstOperand(value);
    }

    public void parseGetElementPtr(Instruction instruction) {
        BackOperand tmp = BackRegTable.getBackReg("t6");
        BackOperand dst = getDstOperand(instruction);
        BackOperand base;
        if (instruction.getOperandList().get(0) instanceof GlobalDecl) {
            base = parseGlobalDecl((GlobalDecl) instruction.getOperandList().get(0));
            BackMov backMov = new BackMov(tmp, base);
            base = tmp;
            backBlock.addInstruction(backMov);
            BackOperand off = parseOperand(instruction.getOperandList().get(1), 32, null);
            if (!(instruction.getOperandList().get(1) instanceof ConstNumber)) {
                if (off instanceof BackImm) {
                    BackMov backMov1 = new BackMov(off, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg())));
                    backBlock.addInstruction(backMov1);
                }
                BackBinary backBinary = new BackBinary("slli", dst, off, new BackImm(2));
                backBlock.addInstruction(backBinary);
                BackBinary backBinary1 = new BackBinary("add", dst, dst, base);
                backBlock.addInstruction(backBinary1);
            } else {
                BackMov backMov1 = new BackMov(dst, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg()) * 4));
                backBlock.addInstruction(backMov1);
                BackBinary backBinary1 = new BackBinary("add", dst, dst, base);
                backBlock.addInstruction(backBinary1);
            }
            if (stackTable.containsKey(instruction.getReg())) {
                saveInStack(dst, instruction);
            }
        } else if (stackTable.containsKey(instruction.getOperandList().get(0).getReg())) {
            // 在stack中，只有两种情况：1. alloca 2. 栈中的param
            BackOperand off = parseOperand(instruction.getOperandList().get(1), 32, null);
            base = new BackImm(stackTable.get(instruction.getOperandList().get(0).getReg()));
            if (instruction.getOperandList().get(0) instanceof Alloca) {
                if (!(instruction.getOperandList().get(1) instanceof ConstNumber)) {
                    if (off instanceof BackImm) {
                        BackMov backMov = new BackMov(off, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg())));
                        backBlock.addInstruction(backMov);
                    }
                    BackBinary backBinary = new BackBinary("slli", dst, off, new BackImm(2));
                    backBlock.addInstruction(backBinary);
                    BackBinary backBinary1 = new BackBinary("addi", dst, dst, base);
                    backBlock.addInstruction(backBinary1);
                } else {
                    BackMov backMov = new BackMov(tmp, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg()) * 4));
                    backBlock.addInstruction(backMov);
                    BackBinary backBinary1 = new BackBinary("addi", dst, tmp, base);
                    backBlock.addInstruction(backBinary1);
                }
                BackBinary backBinary1 = new BackBinary("add", dst, dst, BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backBinary1);
                if (stackTable.containsKey(instruction.getReg())) {
                    saveInStack(dst, instruction);
                }
            } else {
                // 如果是栈中的param，那它从栈中取出来就是指针

                if (!(instruction.getOperandList().get(1) instanceof ConstNumber)) {
                    if (off instanceof BackImm) {
                        BackMov backMov = new BackMov(off, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg())));
                        backBlock.addInstruction(backMov);
                    }
                    BackBinary backBinary = new BackBinary("slli", dst, off, new BackImm(2));
                    backBlock.addInstruction(backBinary);
                    BackLoad load = new BackLoad("ld", BackRegTable.getBackReg("t6"), base, BackRegTable.getBackReg("sp"));
                    backBlock.addInstruction(load);
                    BackBinary backBinary1 = new BackBinary("add", dst, dst, BackRegTable.getBackReg("t6"));
                    backBlock.addInstruction(backBinary1);
                } else {
                    BackMov backMov = new BackMov(tmp, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg()) * 4));
                    backBlock.addInstruction(backMov);
                    BackLoad load = new BackLoad("ld", BackRegTable.getBackReg("t6"), base, BackRegTable.getBackReg("sp"));
                    backBlock.addInstruction(load);
                    BackBinary backBinary1 = new BackBinary("add", dst, tmp, BackRegTable.getBackReg("t6"));
                    backBlock.addInstruction(backBinary1);
                }
                if (stackTable.containsKey(instruction.getReg())) {
                    saveInStack(dst, instruction);
                }
            }
        } else {
            base = RegisterValueMap.instance().getRegOf(instruction.getOperandList().get(0));
            BackOperand off = parseOperand(instruction.getOperandList().get(1), 32, null);
            if (!(instruction.getOperandList().get(1) instanceof ConstNumber)) {
                if (off instanceof BackImm) {
                    BackMov backMov = new BackMov(off, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg())));
                    backBlock.addInstruction(backMov);
                }
                BackBinary backBinary = new BackBinary("slli", tmp, off, new BackImm(2));
                backBlock.addInstruction(backBinary);
                BackBinary backBinary1 = new BackBinary("add", dst, tmp, base);
                backBlock.addInstruction(backBinary1);
            } else {
                BackMov backMov = new BackMov(tmp, new BackImm(Integer.parseInt(instruction.getOperandList().get(1).getReg()) * 4));
                backBlock.addInstruction(backMov);
                BackBinary backBinary1 = new BackBinary("add", dst, tmp, base);
                backBlock.addInstruction(backBinary1);
            }
            if (stackTable.containsKey(instruction.getReg())) {
                saveInStack(dst, instruction);
            }
        }
    }

    public BackOperand parseConstInt(Integer imm, int length) {//length == 32: 可以操作数可以直接是整数；length == 0:操作数要存到寄存器
        if (length == 32) {
            return new BackImm(imm);
        } else if (length == 12 && (imm >= -2048 && imm <= 2047)) {
            return new BackImm12(imm);
        } else {
            BackOperand backOperand;
           /* if (instruction != null) {
                backOperand = parseOperand(instruction, 0, null);
                System.out.println(backOperand);
            } else {
                backOperand = BackRegTable.getBackReg("s11");
            }*/
            backOperand = BackRegTable.getBackReg("t6");
            BackMov backMov = new BackMov(backOperand, new BackImm(imm));
            backBlock.addInstruction(backMov);
            return backOperand;
        }
    }

    public BackOperand parseConstFloat(Integer imm) {
        if (lc.containsKey(imm)) {
            return lc.get(imm);
        } else {
            BackGlobalDecl backGlobalDecl = new BackGlobalDecl(imm);
            backModule.addGlobalDecl(backGlobalDecl);
            BackLabel backLabel = new BackLabel(backGlobalDecl.getName());
            lc.put(imm, backLabel);
            return backLabel;
        }
    }

    public BackOperand parseGlobalDecl(GlobalDecl globalDecl) {
        BackGlobalDecl backGlobalDecl = globalMap.get(globalDecl);
        BackLabel backLabel = new BackLabel(backGlobalDecl.getName());
        return backLabel;
    }

    private BackOperand getDstOperand(Value value) {
        if (RegisterValueMap.instance().getRegOf(value) != null) {
            return RegisterValueMap.instance().getRegOf(value);
        }
        // 不可能没有分配寄存器
        if (value.getType() == ValueType.FLT) {
            return BackFRegTable.getBackReg("ft11");
        } else {
            return BackRegTable.getBackReg("t6");
        }
    }

    private void parsePush(Push push) {
        Value v = push.getValue();
        BackOperand backOperand = parseOperand(v, 32, null);
        int offset = push.getOffset();
        if (backOperand instanceof BackImm) {
            BackOperand tempReg;
            if (v.getType() != ValueType.FLT) {
                tempReg = BackRegTable.getBackReg("t6");
            } else {
                tempReg = BackFRegTable.getBackReg("f11");
            }
            BackMov backMov = new BackMov(tempReg, backOperand);
            backOperand = tempReg;
            backBlock.addInstruction(backMov);
        }
        if (v.getType() != ValueType.FLT) {
            if (!stackTable.containsKey(v.getReg())) {
                BackStore backStore = new BackStore("sd", backOperand, new BackImm(offset), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            } else {
                BackLoad backLoad = new BackLoad("ld", BackRegTable.getBackReg("t6"), backOperand, BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                BackStore backStore = new BackStore("sd", BackRegTable.getBackReg("t6"), new BackImm(offset), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            }
        } else {
            if (!stackTable.containsKey(v.getReg())) {
                BackStore backStore = new BackStore("fsw", backOperand, new BackImm(offset), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            } else {
                BackLoad backLoad = new BackLoad("flw", BackFRegTable.getBackReg("ft11"), backOperand, BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backLoad);
                BackStore backStore = new BackStore("fsw", BackFRegTable.getBackReg("ft11"), new BackImm(offset), BackRegTable.getBackReg("sp"));
                backBlock.addInstruction(backStore);
            }
        }
    }

    private void parseGetParam(GetParam getParam) {
        int offset = getParam.getOffset();
        BackLoad backLoad;
        if (getParam.getType() == ValueType.FLT) {
            backLoad = new BackLoad("flw", RegisterValueMap.instance().getRegOf(getParam), new BackImm(offset + backFunction.getStackSize()),
                    BackRegTable.getBackReg("sp"));
        } else {
            backLoad = new BackLoad("ld", RegisterValueMap.instance().getRegOf(getParam), new BackImm(offset + backFunction.getStackSize()),
                    BackRegTable.getBackReg("sp"));
        }
        backBlock.addInstruction(backLoad);
    }

    private void saveInStack(BackOperand dst, Value value) {
        if (dst.toString().charAt(0) == 'f') {
            BackStore backStore = new BackStore("fsw", dst, new BackImm(stackTable.get(value.getReg())), BackRegTable.getBackReg("sp"));
            backBlock.addInstruction(backStore);
        } else {
            BackStore backStore = new BackStore("sw", dst, new BackImm(stackTable.get(value.getReg())), BackRegTable.getBackReg("sp"));
            backBlock.addInstruction(backStore);
        }
    }
}
