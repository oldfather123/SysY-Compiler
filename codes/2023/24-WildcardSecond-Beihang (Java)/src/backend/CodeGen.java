package backend;

import ir.Value;
import ir.instr.*;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.LoopAnalyzer;
import ir.pass.analyze.RemoveUselessUser;
import ir.type.*;
import ir.value.*;
import backend.component.*;
import backend.instruction.*;
import backend.operand.*;
import ir.value.Module;
import ir.value.user.Function;
import tools.DoubleNode;
import tools.IrRegDispatcher;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedList;

import static backend.instruction.RiscvBranch.buildBranch;
import static ir.instr.InstType.CmpType.*;

//如何进一步减少寄存器的溢出？我们发现绝大多数测试点主要使用整数寄存器，而浮点寄存器有大量空闲，这种情况下，我们可以尝试
/**
 * 在Code生成中，将LLVM的IR中的Value映射到Mips的寄存器中。
 * 在中端的实现中，IR Value 实体有：
 * 1. Argument : 函数的参数（虚拟寄存器类型）
 * 2. BasicBlock : 基本块（标签地址类型）
 * 3. Function : 函数（标签地址类型）
 * 4. GlobalVariable : 全局变量（内存地址类型）
 * 5. Instruction : 指令（虚拟寄存器类型）
 * 6. ConstantInt : 常量整数（立即数类型）
 */
public class CodeGen {
    private int loopDepth;
    private final Module llvmModule;
    private final RiscvModule riscvModule;

    private final LinkedHashMap<Value, RiscvOperand> operandMap = new LinkedHashMap<>();
    /**
     * 用于处理BasicBlock、function 以及全局变量 讲这些映射到Label 浮点常量也映射到这
     */
    private final LinkedHashMap<Value, RiscvLabel> value2Label = new LinkedHashMap<>();
    private final LinkedHashMap<Value, Integer> getEleCount = new LinkedHashMap<>();
    /**
     * 用于将指令映射到最后结果的寄存器 比如 add rd,rs,rt 那么 就映射到 rd 寄存器
     */
    private final LinkedHashMap<Value, RiscvReg> value2Reg = new LinkedHashMap<>();
    private RiscvFunction curRVFunction;
    private RiscvBlock curBlock;
    private IrRegDispatcher irRegDispatcher;
    private LinkedHashMap<Double, RiscvFloatConst> floats = new LinkedHashMap<>();
    /*
    用来处理cmp以及Br指令
    将带条件Br指令绑定到 RiscvBranch 处
     */
    private LinkedHashMap<Br, ArrayList<RiscvBranch>> br2Branch = new LinkedHashMap<>();
    private int funcinitial;
    private LinkedHashMap<RiscvLabel, RiscvReg> labelAlreadyInReg = new LinkedHashMap<>();

    public CodeGen(Module llvmModule, RiscvModule riscvModule) {
        this.llvmModule = llvmModule;
        this.riscvModule = riscvModule;
    }

    public void run() {
        RemoveUselessUser.run(llvmModule);
        DomAnalyzer.calDomForModule(llvmModule);
        LoopAnalyzer.analyzeLoopWithTimes(llvmModule);
        for(var func : llvmModule.functions.values()){
            if(func.isEmpty())continue;
            var entryBlock = func.blocks.get(0);
            if(entryBlock.getLoopDepth()!=0){
                var newEntry = new BasicBlock("entry", func,true);
                func.blocks.add(0, newEntry);
                var newBr = new Br(new LinkedList<>(Arrays.asList(entryBlock)), newEntry);
                newEntry.addValue(newBr);
            }
        }
        DomAnalyzer.calDomForModule(llvmModule);
        LoopAnalyzer.analyzeLoop(llvmModule);
        //TODO 还没了解运行时库函数如何处理,貌似是链接了就不用处理
        riscvModule.setFloats(floats);
        riscvModule.setValue2Label(value2Label);
        for (var global : llvmModule.globals.values()) {
            parseGlobal(global);
        }
        for (var constGlobal : llvmModule.constGlobals.values()) {
            parseGlobal(constGlobal);
        }
        loopDepth = 1;
        for (var function : llvmModule.functions.values()) {
            RiscvFunction rvFunction = new RiscvFunction(function.name, new IrRegDispatcher());
            riscvModule.addFunction(function.name, rvFunction);
            value2Label.put(function, rvFunction);
            riscvModule.getFunction(function.name).genArgPos(function, value2Reg);
        }
        for (var function : llvmModule.functions.values()) {
            if (function.blocks.size() > 0) {
                curRVFunction = riscvModule.getFunction(function.name);
                irRegDispatcher = curRVFunction.getIrRegDispatcher();
                loopDepth = 1;
                funcinitial = 0;//用于记录是第一次 所以要把mv们挪进去
                parseFunction(function);
                for (int i = 0; i < curRVFunction.getBlocks().size(); i++) {
                    if (i != 0) {
                        RiscvBlock block1 = curRVFunction.getBlocks().get(i - 1);
                        RiscvBlock block2 = curRVFunction.getBlocks().get(i);
                        if (!(block1.getRiscvInstrs().getTail().getContent() instanceof RiscvJ ||
                                block1.getRiscvInstrs().getTail().getContent() instanceof RiscvJr)) {
                            block2.addPreds(block1);
                        }
                    }
                    if (i != curRVFunction.getBlocks().size() - 1) {
                        RiscvBlock block1 = curRVFunction.getBlocks().get(i);
                        RiscvBlock block2 = curRVFunction.getBlocks().get(i + 1);
                        if (!(block1.getRiscvInstrs().getTail().getContent() instanceof RiscvJ ||
                                block1.getRiscvInstrs().getTail().getContent() instanceof RiscvJr)) {
                            block1.addSuccs(block2);
                        }
                    }
                }
            }
        }
        for(RiscvFunction function:riscvModule.getFunctions().values()) {
            for(RiscvBlock block:function.getBlocks()) {
                for(DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                    insNode != null; insNode = insNode.getSucc()) {
                    RiscvInstr ins = insNode.getContent();
                    if(!(ins instanceof RiscvLw)) {
                        continue;
                    }
                    boolean flag = false;
                    for(DoubleNode<RiscvInstr> user:ins.getDefReg().getUser()) {
                        RiscvInstr insUser = user.getContent();
                        if((insUser instanceof RiscvBinary && ((RiscvBinary) insUser).is64Ins())
                                || insUser instanceof RiscvBranch) {
                            if(insUser.getDefReg() != null && !insUser.getDefReg().equals(ins.getDefReg())) {
                                flag = true;
                                break;
                            }
                        }
                    }
                    if(flag) {
                        insNode.getParentList().insertAfterNode(insNode,
                                new DoubleNode<>(new RiscvSext(ins.getDefReg(), ins.getDefReg())));
                    }
                }
            }
        }
    }

    public void parseGlobal(Variable global) {
        ArrayList<RiscvGlobalElement> ans = new ArrayList<>();
        ArrayList<RiscvGlobalElement> riscvGlobalElements = new ArrayList<>();
        /*TODO
            观察我的类就可以了
            例如只有一个变量,无论是int还是float,直接在riscvGlobalElements加入一个元素即可,
            如果是数组,未初始化的化直接进入else无影响
            已初始化的比如
            int a[4][2] = {{1,2},{},{},{}}
            我希望得到的应该是两个分别值为1和2的size为4 RiscvGlobalInt,一个size为 4*6=24 的RiscvGlobalZero
            这意味着你需要将连续的0合并,要不然生成的汇编会过长
        */
        if (global.isInit) {
            RiscvGlobalVariable label =
                    new RiscvGlobalVariable(global.name, global.isInit, global.getType().getSpace(),
                            ans);
            riscvModule.getDataGlobalVariables().add(label);
            if (((Pointer) (global.allocInst.getType())).pointTo instanceof FloatType) {
                Object obj = ((Global) global.allocInst).getInitval();
                if (obj == null) {
                    riscvGlobalElements.add(new RiscvGlobalFloat(0.0F));
                } else {
                    riscvGlobalElements.add(new RiscvGlobalFloat(((Double) obj).floatValue()));
                }
            } else if (((Pointer) (global.allocInst.getType())).pointTo instanceof IntType) {
                Object obj = ((Global) global.allocInst).getInitval();
                if (obj == null) {
                    riscvGlobalElements.add(new RiscvGlobalInt(0));
                } else {
                    riscvGlobalElements.add(new RiscvGlobalInt(((int) obj)));
                }
            } else {
                RiscvGlobalVariable var =
                        (RiscvGlobalVariable) genInit(((Global) (global.allocInst)).getInitval(),
                                ((Pointer) global.allocInst.getType()).pointTo);
                riscvGlobalElements.addAll(var.getRiscvGlobalElements());
            }
            for (RiscvGlobalElement element : riscvGlobalElements) {
                flatten(element, ans);
            }
            value2Label.put(global.allocInst, label);
        } else {
            RiscvGlobalVariable label =
                    new RiscvGlobalVariable(global.name, global.isInit, global.getType().getSpace(),
                            ans);
            riscvModule.getBssGlobalVariables().add(label);
            value2Label.put(global.allocInst, label);
        }
    }

    public void flatten(RiscvGlobalElement element, ArrayList<RiscvGlobalElement> ans) {
        if (element instanceof RiscvGlobalZero) {
            if (!ans.isEmpty()) {
                RiscvGlobalElement element2 = ans.get(ans.size() - 1);
                if (element2 instanceof RiscvGlobalZero) {
                    ((RiscvGlobalZero) element2).addSize(((RiscvGlobalZero) element).getSize());
                } else {
                    ans.add(element);
                }
            } else {
                ans.add(element);
            }
        } else if (element instanceof RiscvGlobalFloat) {
            ans.add(element);
        } else if (element instanceof RiscvGlobalInt) {
            if (((RiscvGlobalInt) element).getValue() == 0) {
                element = new RiscvGlobalZero(4);
            }
            if (!ans.isEmpty()) {
                RiscvGlobalElement element2 = ans.get(ans.size() - 1);
                if (element2 instanceof RiscvGlobalZero && element instanceof RiscvGlobalZero) {
                    ((RiscvGlobalZero) element2).addSize(((RiscvGlobalZero) element).getSize());
                } else {
                    ans.add(element);
                }
            } else {
                ans.add(element);
            }
        } else {
            for (RiscvGlobalElement element1 : ((RiscvGlobalVariable) element).getRiscvGlobalElements()) {
                flatten(element1, ans);
            }
        }
    }

    public RiscvGlobalElement genInit(Object obj, Type t) {
        if (t instanceof IntType) {
            if (obj == null) {
                return new RiscvGlobalZero(4);
            }
            return new RiscvGlobalInt((Integer) obj);
        } else if (t instanceof FloatType) {
            if (obj == null) {
                return new RiscvGlobalZero(4);
            }
            return new RiscvGlobalFloat(((Double) obj).floatValue());
        } else {
            int num = ((ArrayType) t).size;
            ArrayList<Object> list = (ArrayList<Object>) obj;
            ArrayList<RiscvGlobalElement> globalElements = new ArrayList<>();
            RiscvGlobalVariable variable = new RiscvGlobalVariable(
                    null, true, num, globalElements);
            for (int i = 0; i < num; i++) {
                if (obj == null || i >= ((ArrayList<?>) obj).size()) {
                    RiscvGlobalElement element = genInit(null, ((ArrayType) t).t);
                    if (!globalElements.isEmpty()) {
                        RiscvGlobalElement element2 = globalElements.get(globalElements.size() - 1);
                        if (element2 instanceof RiscvGlobalZero &&
                                element instanceof RiscvGlobalZero) {
                            ((RiscvGlobalZero) element2).addSize(
                                    ((RiscvGlobalZero) element).getSize());
                        } else {
                            globalElements.add(element);
                        }
                    } else {
                        globalElements.add(element);
                    }
                } else {
                    RiscvGlobalElement element = genInit(list.get(i), ((ArrayType) t).t);
                    if (!globalElements.isEmpty()) {
                        RiscvGlobalElement element2 = globalElements.get(globalElements.size() - 1);
                        if (element2 instanceof RiscvGlobalZero &&
                                element instanceof RiscvGlobalZero) {
                            ((RiscvGlobalZero) element2).addSize(
                                    ((RiscvGlobalZero) element).getSize());
                        } else {
                            globalElements.add(element);
                        }
                    } else {
                        globalElements.add(element);
                    }
                }
            }
            return variable;
        }
    }

    public void parseFunction(Function function) {
        curBlock = null;
        for (var basicBlock : function.blocks) {
            RiscvBlock block = new RiscvBlock("block" + irRegDispatcher.allocId("Block"),
                    basicBlock.loop);
            value2Label.put(basicBlock, block);
        }
        boolean flag = false;
        for (var basicBlock : function.blocks) {
            if (curBlock != null) {
                curRVFunction.addBlock(curBlock);
            }
            curBlock = (RiscvBlock) value2Label.get(basicBlock);
            if (funcinitial == 0) {
                funcinitial = 1;
                for (RiscvMv mv : curRVFunction.moves) {
                    addInstr2Block(mv);
                }
            }
            if (!flag) {
                flag = true;
                RiscvVirReg reg = new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction);
                curRVFunction.setRaReg(reg);
                addInstr2Block(new RiscvMv(RiscvCPUReg.getRiscvCPUReg(1), reg));
            }
            labelAlreadyInReg.clear();
            parseBasicBlock(basicBlock, function);
        }
        curRVFunction.addBlock(curBlock);
    }

    public void parseBasicBlock(BasicBlock basicBlock, Function function) {
        loopDepth = basicBlock.getLoopDepth();
        for (var instr : basicBlock.getInsts()) {
            parseInstr((Instr) instr, basicBlock, function);
        }
    }

    public void addInstr2Block(RiscvInstr riscvInstr) {
        curBlock.addInstr(new DoubleNode<>(riscvInstr), loopDepth);
    }

    public void addInstr2Block(RiscvInstr riscvInstr, RiscvBlock block) {
        curBlock.addInstr(new DoubleNode<>(riscvInstr), loopDepth);
        curRVFunction.addBlock(curBlock);
        curBlock = block;
        labelAlreadyInReg.clear();
    }

    public void parseInstr(Instr instr, BasicBlock basicBlock, Function function) {

        if (instr instanceof BinaryInstr) {
            parseBinary((BinaryInstr) instr, function);
        } else if (instr instanceof Bitcast) {
            parseBitcast((Bitcast) instr);
        } else if (instr instanceof Zext) {
            parseZext((Zext) instr);
        } else if (instr instanceof Br) {
            parseBr((Br) instr);
        } else if (instr instanceof Call) {
            parseCall((Call) instr);
        } else if (instr instanceof Fptosi) {
            parseFptosi((Fptosi) instr);
        } else if (instr instanceof Sitofp) {
            parseSitofp((Sitofp) instr);
        } else if (instr instanceof GetElementPtrInst) {
            parseGetElementPtr((GetElementPtrInst) instr);
        } else if (instr instanceof Alloca) {
            parseAlloca((Alloca) instr);
        } else if (instr instanceof Load) {
            parseLoad((Load) instr);
        } else if (instr instanceof Store) {
            parseStore((Store) instr);
        } else if (instr instanceof Ret) {
            parseRet((Ret) instr);
        } else if (instr instanceof Move) {
            parseMove((Move) instr, basicBlock, function);
        }

    }

    private void parseFcmp(BinaryInstr instr, Function function) {
        //TODO nextBlock != instr.getOP
        boolean beenBuild = false;
        for (User user : instr.getUsers()) {
            if (!beenBuild) {
                beenBuild = true;
                RiscvReg riscvReg = new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction);
                RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), null);
                RiscvOperand rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
                RiscvBinary.RiscvBinaryType type = RiscvBinary.RiscvBinaryType.feq;
                boolean reverse = false;
                switch (instr.cmpType) {
                    case oeq -> {
                        type = RiscvBinary.RiscvBinaryType.feq;
                    }
                    case one -> {
                        reverse = true;
                        type = RiscvBinary.RiscvBinaryType.feq;
                    }
                    case ogt -> {
                        type = RiscvBinary.RiscvBinaryType.flt;
                        RiscvOperand temp = rightOperand;
                        rightOperand = leftOperand;
                        leftOperand = temp;
                    }
                    case oge -> {
                        type = RiscvBinary.RiscvBinaryType.fle;
                        RiscvOperand temp = rightOperand;
                        rightOperand = leftOperand;
                        leftOperand = temp;
                    }
                    case olt -> {
                        type = RiscvBinary.RiscvBinaryType.flt;
                    }
                    case ole -> {
                        type = RiscvBinary.RiscvBinaryType.fle;
                    }
                }
                RiscvInstr riscvInstr1 = new RiscvBinary(new ArrayList<>(
                        Arrays.asList(leftOperand, rightOperand)), riscvReg, type);
                addInstr2Block(riscvInstr1);
                if (reverse) {
                    RiscvInstr riscvInstr2 = new RiscvBinary(
                            new ArrayList<>(Arrays.asList(riscvReg, new RiscvImm(1))), riscvReg,
                            RiscvBinary.RiscvBinaryType.xori);
                    addInstr2Block(riscvInstr2);
                }
                value2Reg.put(instr, riscvReg);
            }
            if (user instanceof Br) {
                BasicBlock block = ((Br) user).getParent();
                BasicBlock nextBlock = null;
                for (int i = 0; i < function.blocks.size(); i++) {
                    if (function.blocks.get(i).equals(block)) {
                        nextBlock = function.blocks.get(i + 1);
                        break;
                    }
                }
                if(user.getOperands().get(1) == nextBlock ||
                        user.getOperands().get(2) == nextBlock) {
                    boolean flag = user.getOperands().get(1) == nextBlock;
                    RiscvLabel jBlock = (user.getOperands().get(1) == nextBlock) ?
                            value2Label.get(user.getOperands().get(2)) :
                            value2Label.get(user.getOperands().get(1));
                    //flag == true 代表为1 跳往trueblock 为0去falseblock
                    //flag == true 那么就说明 ret == 0 时 才需要跳 所以是bne ret, 0 , jblock
                    RiscvBranch.RvCmpType cmpType =
                            (flag) ? RiscvBranch.RvCmpType.beq : RiscvBranch.RvCmpType.bne;
                    RiscvBranch riscvBranch = buildBranch(value2Reg.get(instr), jBlock, cmpType);
                    br2Branch.put((Br) user, new ArrayList<>(Collections.singleton(riscvBranch)));
                } else {
                    RiscvBranch riscvBranch = buildBranch(value2Reg.get(instr),
                            value2Label.get(user.getOperands().get(2)), RiscvBranch.RvCmpType.beq);
                    RiscvBranch riscvBranch1 = buildBranch(value2Reg.get(instr),
                            value2Label.get(user.getOperands().get(1)), RiscvBranch.RvCmpType.bne);
                    br2Branch.put((Br) user, new ArrayList<>(Arrays.asList(riscvBranch, riscvBranch1)));
                }
            }
        }
    }

    private void parseICmp(BinaryInstr instr, Function function) {
        boolean beenBuild = false;
        for (User user : instr.getUsers()) {
            if (!beenBuild && !(user instanceof Br)) {
                //TODO：舍弃了sltiu优化 只能在少数情况下节省每次1条指令
                beenBuild = true;
                RiscvReg newDefReg = new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction);
                RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), newDefReg);
                RiscvOperand rightOperand;
                if (!leftOperand.equals(newDefReg)) {
                    rightOperand = getOperandFromValueNoimm(instr.getOP(1), newDefReg);
                } else {
                    rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
                }
                if (instr.cmpType == lt) {
                    RiscvInstr ins1 = new RiscvBinary(
                            new ArrayList<>(Arrays.asList(leftOperand, rightOperand)), newDefReg,
                            RiscvBinary.RiscvBinaryType.slt);
                    addInstr2Block(ins1);
                } else if (instr.cmpType == gt) {
                    RiscvInstr ins1 = new RiscvBinary(
                            new ArrayList<>(Arrays.asList(rightOperand, leftOperand)), newDefReg,
                            RiscvBinary.RiscvBinaryType.slt);
                    addInstr2Block(ins1);
                } else {
                    //ge ne eq le;
                    if (instr.cmpType == ge) {
                        RiscvInstr ins1 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(rightOperand, leftOperand)), newDefReg,
                                RiscvBinary.RiscvBinaryType.subw);
                        addInstr2Block(ins1);
                        RiscvInstr ins2 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(newDefReg, new RiscvImm(1))), newDefReg,
                                RiscvBinary.RiscvBinaryType.slti);
                        addInstr2Block(ins2);
                    } else if (instr.cmpType == ne) {
                        RiscvInstr ins1 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(rightOperand, leftOperand)), newDefReg,
                                RiscvBinary.RiscvBinaryType.subw);
                        addInstr2Block(ins1);
                        RiscvInstr ins2 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(0)
                                        , newDefReg)), newDefReg, RiscvBinary.RiscvBinaryType.sltu);
                        addInstr2Block(ins2);
                    } else if (instr.cmpType == eq) {
                        RiscvInstr ins1 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(leftOperand, rightOperand)), newDefReg,
                                RiscvBinary.RiscvBinaryType.subw);
                        addInstr2Block(ins1);
                        RiscvInstr ins2 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(newDefReg, new RiscvImm(1))), newDefReg,
                                RiscvBinary.RiscvBinaryType.sltiu);
                        addInstr2Block(ins2);
                    } else if (instr.cmpType == le) {
                        RiscvInstr ins1 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(leftOperand, rightOperand)), newDefReg,
                                RiscvBinary.RiscvBinaryType.subw);
                        addInstr2Block(ins1);
                        RiscvInstr ins2 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(newDefReg, new RiscvImm(1))), newDefReg,
                                RiscvBinary.RiscvBinaryType.slti);
                        addInstr2Block(ins2);
                    }
                }
                value2Reg.put(instr, newDefReg);
            } else {
                //buildBranch(RiscvOperand left, RiscvOperand right, InstType.CmpType type, RiscvLabel jLabel)
                RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), null);
                RiscvOperand rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
                BasicBlock block = ((Br) user).getParent();
                BasicBlock nextBlock = null;
                for (int i = 0; i < function.blocks.size(); i++) {
                    if (function.blocks.get(i).equals(block)
                            && i + 1 < function.blocks.size()) {
                        nextBlock = function.blocks.get(i + 1);
                        break;
                    }
                }
                assert nextBlock != null;
                if(user.getOperands().get(1) == nextBlock ||
                        user.getOperands().get(2) == nextBlock) {
                    boolean flag = user.getOperands().get(1) == nextBlock;
                    RiscvLabel jBlock = (user.getOperands().get(1) == nextBlock) ?
                            value2Label.get(user.getOperands().get(2)) :
                            value2Label.get(user.getOperands().get(1));
                    RiscvInstr branch =
                            buildBranch(leftOperand, rightOperand, instr.cmpType, jBlock, flag);
                    br2Branch.put((Br) user, new ArrayList<>(Collections.singleton((RiscvBranch) branch)));
                } else {
                    boolean flag = true;
                    RiscvBranch branch1 =
                            buildBranch(leftOperand, rightOperand, instr.cmpType,
                                    value2Label.get(user.getOperands().get(2)), flag);
                    RiscvBranch branch2 =
                            buildBranch(leftOperand, rightOperand, instr.cmpType,
                                    value2Label.get(user.getOperands().get(1)), !flag);
                    br2Branch.put((Br) user, new ArrayList<>(Arrays.asList(branch1,branch2)));
                }
            }
        }
    }

    private void parseAdd(BinaryInstr instr) {
        RiscvVirReg reg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        Value left = instr.getOP(0);
        Value right = instr.getOP(1);
        if (instr.getOP(0) instanceof ConstNumber) {
            right = instr.getOP(0);
            left = instr.getOP(1);
        }
        RiscvOperand leftOperand = getOperandFromValueNoimm(left, reg);
        RiscvOperand rightOperand = getOperandFromValueMayimm(right, reg);
        if (rightOperand instanceof RiscvImm) {
            if(((RiscvImm) rightOperand).getValue() == 0) {
                RiscvInstr riscvInstr = new RiscvMv((RiscvReg) leftOperand, reg);
                addInstr2Block(riscvInstr);
            } else {
                RiscvInstr riscvInstr = new RiscvBinary(
                        new ArrayList<>(Arrays.asList(leftOperand, rightOperand)),
                        reg, RiscvBinary.RiscvBinaryType.addiw);
                addInstr2Block(riscvInstr);
            }
            value2Reg.put(instr, reg);
        } else {
            RiscvInstr riscvInstr = new RiscvBinary(
                    new ArrayList<>(Arrays.asList(leftOperand, rightOperand)),
                    reg, RiscvBinary.RiscvBinaryType.addw);
            value2Reg.put(instr, reg);
            addInstr2Block(riscvInstr);
        }
    }

    private void parseSub(BinaryInstr instr) {
        RiscvVirReg riscvReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), riscvReg);
        RiscvOperand rightOperand = getOperandFromValueMayimm(instr.getOP(1), riscvReg, false);
        if (rightOperand instanceof RiscvImm) {
            ((RiscvImm) rightOperand).opposite();
            if(((RiscvImm) rightOperand).getValue() == 0) {
                RiscvInstr riscvInstr = new RiscvMv((RiscvReg) leftOperand, riscvReg);
                addInstr2Block(riscvInstr);
            } else {
                RiscvInstr riscvInstr = new RiscvBinary(
                        new ArrayList<>(Arrays.asList(leftOperand, rightOperand)),
                        riscvReg, RiscvBinary.RiscvBinaryType.addiw);
                addInstr2Block(riscvInstr);
            }
            value2Reg.put(instr, riscvReg);
        } else {
            RiscvInstr riscvInstr1 =
                    new RiscvBinary(new ArrayList<>(Arrays.asList(leftOperand, rightOperand))
                            , riscvReg, RiscvBinary.RiscvBinaryType.subw);
            value2Reg.put(instr, riscvReg);
            addInstr2Block(riscvInstr1);
        }
    }

    private ArrayList<Integer> canOpt(int num) {
        ArrayList<Integer> ans = new ArrayList<>();
        int i = 1;
        while(i < num) {
            i *= 2;
        }
        if(i == num) {
            ans.add(i);
            return ans;
        }
        if(BigInteger.valueOf(Math.abs(num)).bitCount() == 2) {
            for(int j = 1;j < i;j*=2) {
                if(((num - j) & (num - j - 1)) == 0) {
                    ans.add(j);
                    ans.add(num - j);
                    break;
                }
            }
        } else if(BigInteger.valueOf(Math.abs(i - num)).bitCount() == 1) {
            ans.add(i);
            ans.add(num - i);
        }
        return ans;
    }

    public int getShift(int temp) {
        int shift = 0;
        while(temp >= 2) {
            shift++;
            temp /= 2;
        }
        return shift;
    }

    private void parseMul(BinaryInstr instr) {
        RiscvVirReg riscvReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        Value left = instr.getOP(0);
        Value right = instr.getOP(1);
        if (instr.getOP(0) instanceof ConstNumber) {
            left = instr.getOP(1);
            right = instr.getOP(0);
        }
        RiscvOperand leftOperand = getOperandFromValueNoimm(left, null);
        RiscvOperand rightOperand;
        if(right instanceof ConstNumber) {
            if((int)((ConstNumber) right).value == 1) {
                addInstr2Block(new RiscvMv((RiscvReg) leftOperand, riscvReg));
                value2Reg.put(instr, riscvReg);
                return;
            } else if((int)((ConstNumber) right).value == -1) {
                addInstr2Block(new RiscvBinary(new ArrayList<>(
                        Arrays.asList(RiscvCPUReg.getRiscvCPUReg(0), leftOperand)),
                        riscvReg, RiscvBinary.RiscvBinaryType.subw));
                value2Reg.put(instr, riscvReg);
                return;
            } else if((int)((ConstNumber) right).value == 0) {
                addInstr2Block(new RiscvLi(new RiscvImm(0), riscvReg));
                value2Reg.put(instr, riscvReg);
                return;
            } else {
                ArrayList<Integer> ans = canOpt(Math.abs((int)((ConstNumber) right).value));
                if(ans.size() > 0) {
                    if((int)((ConstNumber) right).value < 0) {
                        RiscvVirReg reg = new RiscvVirReg(irRegDispatcher,
                                RiscvVirReg.RegType.intType, curRVFunction);
                        addInstr2Block(new RiscvBinary(
                                new ArrayList<>(Arrays.asList(
                                        RiscvCPUReg.getRiscvCPUReg(0), leftOperand)),
                                reg, RiscvBinary.RiscvBinaryType.subw));
                        leftOperand = reg;
                    }
                    RiscvReg assistReg = new RiscvVirReg(irRegDispatcher,
                            RiscvVirReg.RegType.intType, curRVFunction);
                    if(ans.size() == 1) {
                        int shift = getShift(Math.abs(ans.get(0)));
                        addInstr2Block(new RiscvBinary(
                                new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(shift))),
                                riscvReg, RiscvBinary.RiscvBinaryType.slliw));
                        value2Reg.put(instr, riscvReg);
                        return ;
                    } else if(ans.size() == 2) {
                        assert ans.get(0) > 0;
                        int shift = getShift(Math.abs(ans.get(0)));
                        if(shift == 0) {
                            addInstr2Block(new RiscvMv((RiscvReg) leftOperand, riscvReg));
                        } else {
                            addInstr2Block(new RiscvBinary(
                                    new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(shift))),
                                    riscvReg, RiscvBinary.RiscvBinaryType.slliw));
                        }
                        boolean flag = ans.get(1) > 0;
                        shift = getShift(Math.abs(ans.get(1)));
                        if(flag) {
                            if(shift == 0) {
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(leftOperand, riscvReg)),
                                        riscvReg, RiscvBinary.RiscvBinaryType.addw));
                            } else {
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(shift))),
                                        assistReg, RiscvBinary.RiscvBinaryType.slliw));
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(assistReg, riscvReg)),
                                        riscvReg, RiscvBinary.RiscvBinaryType.addw));
                            }
                        } else {
                            if(shift == 0) {
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(riscvReg, leftOperand)),
                                        riscvReg, RiscvBinary.RiscvBinaryType.subw));
                            } else {
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(shift))),
                                        assistReg, RiscvBinary.RiscvBinaryType.slliw));
                                addInstr2Block(new RiscvBinary(
                                        new ArrayList<>(Arrays.asList(riscvReg, assistReg)),
                                        riscvReg, RiscvBinary.RiscvBinaryType.subw));
                            }
                        }
                        value2Reg.put(instr, riscvReg);
                        return ;
                    }
                }
            }
        }
        if (right instanceof ConstNumber) {
            RiscvInstr riscvInstr = new RiscvLi(new RiscvImm((ConstNumber) right), riscvReg);
            addInstr2Block(riscvInstr);
            rightOperand = riscvReg;
        } else {
            rightOperand = getRegByValue(right);
        }
        RiscvInstr riscvInstr1 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(leftOperand, rightOperand)), riscvReg, RiscvBinary.RiscvBinaryType.mulw);
        value2Reg.put(instr, riscvReg);
        addInstr2Block(riscvInstr1);
    }

    private void parseDiv(BinaryInstr instr) {
        RiscvVirReg riscvReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), riscvReg);
        RiscvOperand rightOperand = null;
        if (instr.getOP(1) instanceof ConstNumber) {
            int val = (int) ((ConstNumber) instr.getOP(1)).value;
            if (val == 1) {
                addInstr2Block(new RiscvMv((RiscvReg) leftOperand, riscvReg));
                value2Reg.put(instr, riscvReg);
                return ;
            } else if(val == -1) {
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(
                                RiscvCPUReg.getRiscvCPUReg(0), leftOperand)),
                        riscvReg, RiscvBinary.RiscvBinaryType.subw));
                value2Reg.put(instr, riscvReg);
                return ;
            } else if ((Math.abs(val) & (Math.abs(val) - 1)) == 0) {
                boolean flag = val < 0;
                int temp = Math.abs(val);
                int shift = 0;
                while(temp >= 2) {
                    shift++;
                    temp /= 2;
                }
                if(flag) {
                    RiscvVirReg reg1 = new RiscvVirReg(irRegDispatcher,
                            RiscvVirReg.RegType.intType, curRVFunction);
                    addInstr2Block(new RiscvBinary(
                            new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(0), leftOperand)),
                            reg1, RiscvBinary.RiscvBinaryType.subw));
                    leftOperand = reg1;
                }
                RiscvReg reg = new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction);
                addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(31))),
                        reg, RiscvBinary.RiscvBinaryType.sraiw));
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(reg, new RiscvImm(32 - shift))),
                        reg, RiscvBinary.RiscvBinaryType.srliw));
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(leftOperand, reg)),
                        reg, RiscvBinary.RiscvBinaryType.addw));
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(reg, new RiscvImm(shift))),
                        riscvReg, RiscvBinary.RiscvBinaryType.sraiw));
                value2Reg.put(instr, riscvReg);
                return;
            }
            RiscvInstr riscvInstr =
                    new RiscvLi(new RiscvImm((ConstNumber) instr.getOP(1)), riscvReg);
            addInstr2Block(riscvInstr);
            rightOperand = riscvReg;
        }
        if (rightOperand == null) {
            if (leftOperand != riscvReg) {
                rightOperand = getOperandFromValueNoimm(instr.getOP(1), riscvReg);
            } else {
                rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
            }
        }
        RiscvInstr riscvInstr1 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(leftOperand, rightOperand)), riscvReg, RiscvBinary.RiscvBinaryType.divw);
        value2Reg.put(instr, riscvReg);
        addInstr2Block(riscvInstr1);
    }

    public void parseSrem(BinaryInstr instr) {
        RiscvVirReg riscvReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), riscvReg);
        RiscvOperand rightOperand = null;
        if(instr.getOP(1) instanceof ConstNumber) {
            int val = (Integer) ((ConstNumber) instr.getOP(1)).value;
            int temp = Math.abs(val);
            if((temp & (temp- 1)) == 0) {
                int shift = 0;
                while(temp >= 2) {
                    shift++;
                    temp /= 2;
                }
                RiscvReg reg = new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction);
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(leftOperand, new RiscvImm(31))),
                        reg, RiscvBinary.RiscvBinaryType.sraiw));
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(reg, new RiscvImm(32 - shift))),
                        reg, RiscvBinary.RiscvBinaryType.srliw));
                addInstr2Block(new RiscvBinary(
                        new ArrayList<>(Arrays.asList(leftOperand, reg)),
                        reg, RiscvBinary.RiscvBinaryType.addw));
                addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(reg, new RiscvImm(shift))),
                        reg, RiscvBinary.RiscvBinaryType.srliw));
                addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(reg, new RiscvImm(shift))),
                        reg, RiscvBinary.RiscvBinaryType.slli));
                addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(leftOperand, reg)),
                        riscvReg, RiscvBinary.RiscvBinaryType.subw));
                value2Reg.put(instr, riscvReg);
                return ;
            }
        }
        if (rightOperand == null) {
            if (leftOperand != riscvReg) {
                rightOperand = getOperandFromValueNoimm(instr.getOP(1), riscvReg);
            } else {
                rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
            }
        }
        RiscvInstr riscvInstr1 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(leftOperand, rightOperand)), riscvReg, RiscvBinary.RiscvBinaryType.remw);
        value2Reg.put(instr, riscvReg);
        addInstr2Block(riscvInstr1);
    }

    private void parseFbin(BinaryInstr instr, RiscvBinary.RiscvBinaryType type) {
        RiscvVirReg riscvReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
        RiscvOperand leftOperand = getOperandFromValueNoimm(instr.getOP(0), null);
        RiscvOperand rightOperand = getOperandFromValueNoimm(instr.getOP(1), null);
        RiscvInstr riscvInstr1 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(leftOperand, rightOperand)), riscvReg, type);
        value2Reg.put(instr, riscvReg);
        addInstr2Block(riscvInstr1);
    }

    private void parseBinary(BinaryInstr instr, Function function) {
        if (instr.instType == InstType.BinaryType.icmp) {
            parseICmp(instr, function);
        } else if (instr.instType == InstType.BinaryType.fcmp) {
            parseFcmp(instr, function);
        } else {
            switch (instr.instType) {
                case add -> {
                    parseAdd(instr);
                }
                case sub -> {
                    parseSub(instr);
                }
                case mul -> {
                    parseMul(instr);
                }
                case sdiv -> {
                    parseDiv(instr);
                }
                case srem -> {
                    parseSrem(instr);
                }
                case fadd -> {
                    parseFbin(instr, RiscvBinary.RiscvBinaryType.fadd);
                }
                case fsub -> {
                    parseFbin(instr, RiscvBinary.RiscvBinaryType.fsub);
                }
                case fmul -> {
                    parseFbin(instr, RiscvBinary.RiscvBinaryType.fmul);
                }
                case fdiv -> {
                    parseFbin(instr, RiscvBinary.RiscvBinaryType.fdiv);
                }
            }
        }
    }

    //评价为BitCast和Zext其实无用,如果强行要做成SSA形式可能要空添一条mv,无效挪动一次寄存器
    //擦，实在不行就挪吧
    private void parseBitcast(Bitcast instr) {
//        for(Value value: instr.getOperands()) {
//            System.out.println(value);
//        }
        if(instr.getOP(0) instanceof Alloca) {
            RiscvVirReg reg =
                    new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
            int num = (-1 * curRVFunction.getOffset(instr.getOP(0)));
            if(num < 2028 && num >= -2028) {
                if(num == 0) {
                    RiscvInstr ins = new RiscvMv(RiscvCPUReg.getRiscvCPUReg(2), reg);
                    addInstr2Block(ins);
                } else {
                    RiscvInstr ins = new RiscvBinary(new ArrayList<>(
                            Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), new RiscvImm(num))),
                            reg, RiscvBinary.RiscvBinaryType.addi);
                    addInstr2Block(ins);
                }
            } else {
                RiscvInstr ins1 =
                        new RiscvLi(new RiscvImm(num), reg);
                RiscvInstr ins2 = new RiscvBinary(
                        new ArrayList<>(Arrays.asList(reg, RiscvCPUReg.getRiscvCPUReg(2))),
                        reg, RiscvBinary.RiscvBinaryType.add);
                addInstr2Block(ins1);
                addInstr2Block(ins2);
            }
            value2Reg.put(instr, reg);
        } else if(value2Label.containsKey(instr.getOP(0))) {
            //全局
            RiscvReg reg = getExactAddr(value2Label.get(instr.getOP(0)));
            value2Reg.put(instr, reg);
        } else {
            RiscvReg reg = getRegByValue(instr.getOP(0));
            RiscvVirReg newReg = new RiscvVirReg(
                    irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
            RiscvInstr ins = new RiscvMv(reg, newReg);
            addInstr2Block(ins);
            value2Reg.put(instr, newReg);
        }
    }

    private void parseZext(Zext instr) {
        assert true;
        if (instr.getOP(0) instanceof ConstNumber) {
            RiscvReg defReg =
                    new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
            value2Reg.put(instr, (RiscvReg) getOperandFromValueNoimm(instr.getOP(0), defReg));
        } else {
            value2Reg.put(instr, getRegByValue(instr.getOP(0)));
        }
    }

    private void parseBr(Br instr) {
        if (instr.getOperands().size() == 1) {
            assert value2Label.containsKey(instr.getOperands().get(0));
            addInstr2Block(new RiscvJ(value2Label.get(instr.getOperands().get(0)), curBlock));
        } else {
            assert instr.getOperands().size() > 1;
            assert br2Branch.containsKey(instr);
            if(br2Branch.get(instr).size() == 1) {
                RiscvBranch ins = br2Branch.get(instr).get(0);
                ins.setParent(curBlock);
                addInstr2Block(br2Branch.get(instr).get(0));
            } else {
                RiscvBranch ins = br2Branch.get(instr).get(0);
                ins.setParent(curBlock);
                addInstr2Block(br2Branch.get(instr).get(0));
                br2Branch.get(instr).get(1).setParent(curBlock);
                addInstr2Block(br2Branch.get(instr).get(1));
            }
        }
    }

    private void parseCall(Call instr) {
        RiscvLabel targetFunction = value2Label.get(instr.getFunction());
        RiscvCall call = new RiscvCall(targetFunction);
        int argc = instr.getOperands().size();
        assert targetFunction instanceof RiscvFunction;
        int floatArgNum = ((RiscvFunction) targetFunction).getFloatArgNum();
        int otherArgNum = ((RiscvFunction) targetFunction).getOtherArgNum();
        //大于8个都压栈z
        int floatStackNum = floatArgNum > 8 ? floatArgNum - 8 : 0;
        int otherStackNum = otherArgNum > 8 ? otherArgNum - 8 : 0;
        int totalNum = floatStackNum + otherStackNum;
        int stackNum = totalNum;//所有溢出默认使用64位，故每个参数占用8字节，stacknum只需要跟2n对齐即可
        int stackCur = 0;//表示调用此函数时jal时的栈顶参数栈位置
        int otherCur = 0, floatCur = 0;//表示当前参数保存的位置

        // TODO: 2023/8/8 待修改
//        RiscvReg regUp = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
//        RiscvInstr ins1 = new RiscvLi(new StackFixer(curRVFunction,
//            stackNum * 4), regUp);
//        addInstr2Block(ins1);
//        RiscvInstr ins2 = new RiscvBinary(new ArrayList<>(
//            Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), regUp)),
//            RiscvCPUReg.getRiscvCPUReg(2), RiscvBinary.RiscvBinaryType.sub);
//        addInstr2Block(ins2);
        // TODO: 2023/8/8 end
        for (int i = 0; i < argc; ++i) {
            Value arg = instr.getOperands().get(i);
            if (arg.getType() instanceof FloatType) {
                if (floatCur < 8) {
                    if (arg instanceof ConstNumber) {
                        //如果是浮点常量
                        RiscvFloatConst label;
                        if(floats.containsKey(((ConstNumber) arg).value)) {
                            label = floats.get(((ConstNumber) arg).value);
                        } else {
                            label = new RiscvFloatConst("float" + floats.size(),
                                    (Double) (((ConstNumber) arg).value));
                            floats.put((Double) (((ConstNumber) arg).value), label);
                        }
                        RiscvReg reg1 =
                                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                                        curRVFunction);
                        RiscvInstr lui = new RiscvLui(label.hi(), reg1);
                        addInstr2Block(lui);
                        RiscvReg reg2 = RiscvFPUReg.getRiscvFPUReg(10 + floatCur);
                        RiscvInstr flw = new RiscvFlw(reg2, label.lo(), reg1);
                        addInstr2Block(flw);
                        call.addUsedReg(reg2);
                    } else {
                        //如果已经被算出来了,直接move
                        RiscvReg reg = RiscvFPUReg.getRiscvFPUReg(10 + floatCur);
                        RiscvInstr fmv = new RiscvFmv(getRegByValue(arg), reg);
                        addInstr2Block(fmv);
                        call.addUsedReg(reg);
                    }
                } else {
                    int offset = stackCur * 8 + 8;
                    stackCur++;
                    if (arg instanceof ConstNumber) {
                        RiscvFloatConst label;
                        if(floats.containsKey(((ConstNumber) arg).value)) {
                            label = floats.get(((ConstNumber) arg).value);
                        } else {
                            label = new RiscvFloatConst("float" + floats.size(),
                                    (Double) (((ConstNumber) arg).value));
                            floats.put((Double) (((ConstNumber) arg).value), label);
                        }
                        RiscvReg reg1 =
                                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                                        curRVFunction);
                        RiscvInstr lui = new RiscvLui(label.hi(), reg1);
                        addInstr2Block(lui);
                        RiscvReg reg2 =
                                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType,
                                        curRVFunction);
                        RiscvInstr flw = new RiscvFlw(reg2, label.lo(), reg1);
                        addInstr2Block(flw);
                        RiscvReg reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                                curRVFunction);
                        RiscvInstr li = new RiscvLi(new StackFixer(curRVFunction, offset), reg);
                        addInstr2Block(li);
                        RiscvInstr sub = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), reg)), reg,
                                RiscvBinary.RiscvBinaryType.sub);
                        addInstr2Block(sub);
                        RiscvInstr sw = new RiscvFsd(reg2, new RiscvImm(0), reg);
                        addInstr2Block(sw);
                    } else {
                        RiscvReg reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                                curRVFunction);
                        RiscvInstr li = new RiscvLi(new StackFixer(curRVFunction, offset), reg);
                        addInstr2Block(li);
                        RiscvInstr sub = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), reg)), reg,
                                RiscvBinary.RiscvBinaryType.sub);
                        addInstr2Block(sub);
                        RiscvInstr sw = new RiscvFsd(getRegByValue(arg), new RiscvImm(0), reg);
                        addInstr2Block(sw);
                    }
                }
                floatCur++;
            } else {
                if (otherCur < 8) {
                    if (arg instanceof ConstNumber) {
                        RiscvReg reg = RiscvCPUReg.getRiscvCPUReg(10 + otherCur);
                        RiscvInstr li = new RiscvLi(new RiscvImm((ConstNumber) arg), reg);
                        addInstr2Block(li);
                        call.addUsedReg(reg);
                    } else {
                        RiscvReg reg = RiscvCPUReg.getRiscvCPUReg(10 + otherCur);
                        RiscvInstr mv = new RiscvMv(getRegByValue(arg), reg);
                        addInstr2Block(mv);
                        call.addUsedReg(reg);
                    }
                } else {
                    int offset = stackCur * 8 + 8;
                    stackCur++;
                    if (arg instanceof ConstNumber) {
                        RiscvReg reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                                curRVFunction);
                        RiscvInstr li = new RiscvLi(new RiscvImm((ConstNumber) arg), reg);
                        addInstr2Block(li);
                        RiscvVirReg newReg = new RiscvVirReg(irRegDispatcher,
                                RiscvVirReg.RegType.intType, curRVFunction);
                        RiscvInstr insLi =
                                new RiscvLi(new StackFixer(curRVFunction, offset), newReg);
                        addInstr2Block(insLi);
                        RiscvInstr sub = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), newReg)), newReg,
                                RiscvBinary.RiscvBinaryType.sub);
                        addInstr2Block(sub);
                        RiscvInstr sw = new RiscvSd(reg,
                                new RiscvImm(0), newReg);
                        addInstr2Block(sw);
                    } else {
                        RiscvVirReg newReg = new RiscvVirReg(irRegDispatcher,
                                RiscvVirReg.RegType.intType, curRVFunction);
                        RiscvInstr insLi =
                                new RiscvLi(new StackFixer(curRVFunction, offset), newReg);
                        addInstr2Block(insLi);
                        RiscvInstr sub = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), newReg)), newReg,
                                RiscvBinary.RiscvBinaryType.sub);
                        addInstr2Block(sub);
                        RiscvInstr sw = new RiscvSd(getRegByValue(arg),
                                new RiscvImm(0), newReg);
                        addInstr2Block(sw);
                    }
                }
                otherCur++;
            }
        }

        RiscvReg regUp =RiscvCPUReg.getRiscvCPUReg(9);
        RiscvInstr ins1 = new RiscvLi(new StackFixer(curRVFunction, 0), regUp);
        addInstr2Block(ins1);
        RiscvInstr ins2 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), regUp)),
                RiscvCPUReg.getRiscvCPUReg(2), RiscvBinary.RiscvBinaryType.sub);
        addInstr2Block(ins2);

        //栈生长与恢复
        addInstr2Block(call);
        RiscvInstr ins3 = new RiscvBinary(new ArrayList<>(
                Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2), regUp)),
                RiscvCPUReg.getRiscvCPUReg(2), RiscvBinary.RiscvBinaryType.add);
        addInstr2Block(ins3);
        //观察学长代码貌似还要特殊维护和物理寄存器的一些关系,具体call会影响哪些寄存器我还没完全搞清楚
        //不排除是arm本身架构比较特殊
        //最后处理返回值
        if (!(instr.getFunction().getRetType() instanceof VoidType)) {
            if (instr.getFunction().getRetType() instanceof FloatType) {
                RiscvReg reg1 =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
                RiscvInstr fmv = new RiscvFmv(RiscvFPUReg.getRiscvFPUReg(10), reg1);
                value2Reg.put(instr, reg1);
                addInstr2Block(fmv);
            } else {
                RiscvReg reg1 =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                RiscvInstr mv = new RiscvMv(RiscvCPUReg.getRiscvCPUReg(10), reg1);
                value2Reg.put(instr, reg1);
                addInstr2Block(mv);
            }
        }
    }

    //这里我认为不会出现fptosi或sitofp对立即数的操作,默认在中端已经优化掉了
    private void parseFptosi(Fptosi instr) {
        RiscvVirReg riscvReg0 =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvVirReg riscvReg1 =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
        RiscvOperand src = null;
        if (instr.getOP(0) instanceof ConstNumber) {
            getOperandFromValueNoimm(instr.getOP(0), riscvReg1);
            src = riscvReg1;
        } else {
            src = getRegByValue(instr.getOP(0));
        }
        RiscvInstr riscvInstr = new RiscvConvert(new ArrayList<>(
                Arrays.asList(src)), false, riscvReg0);
        addInstr2Block(riscvInstr);
        value2Reg.put(instr, riscvReg0);
    }

    private void parseSitofp(Sitofp instr) {
        RiscvVirReg riscvReg0 =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
        RiscvVirReg riscvReg1 =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvOperand src = null;
        if (instr.getOP(0) instanceof ConstNumber) {
            getOperandFromValueNoimm(instr.getOP(0), riscvReg1);
            src = riscvReg1;
        } else {
            src = getRegByValue(instr.getOP(0));
        }
        RiscvInstr riscvInstr = new RiscvConvert(new ArrayList<>(
                Arrays.asList(src)), true, riscvReg0);
        addInstr2Block(riscvInstr);
        value2Reg.put(instr, riscvReg0);
    }

    private void parseGetElementPtr(GetElementPtrInst instr) {
        // 为了减少指令数量，首先求出offset为ConstNumber的部分，最后依次加上offset为Reg的部分
        int offset_imm = 0;
        assert instr.getOP(0).type instanceof Pointer;
        Type typeori = ((Pointer) instr.getOP(0).type).pointTo;
        Type type = typeori;
        boolean flag = true;
        for (int i = 1; i < instr.getOperands().size(); i++) { // 仅处理offset为ConstNumber
            if (instr.getOP(i) instanceof ConstNumber) {
                offset_imm += type.getSpace() * (int) ((ConstNumber) instr.getOP(i)).value;
            } else {
                flag = false;
            }
            if (type instanceof ArrayType) {
                type = ((ArrayType) type).getT();
            }
        }
        RiscvReg defReg =
                new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        if (value2Label.containsKey(instr.getOP(0))) { //global
            if (offset_imm < 2048 && offset_imm >= -2048) {
                RiscvReg reg = getExactAddr(value2Label.get(instr.getOP(0)));
                if(offset_imm == 0) {
                    RiscvInstr riscvInstr1 = new RiscvMv(reg, defReg);
                    addInstr2Block(riscvInstr1);
                } else {
                    RiscvInstr riscvInstr1 = new RiscvBinary(
                            new ArrayList<>(Arrays.asList(reg, new RiscvImm(offset_imm)))
                            , defReg, RiscvBinary.RiscvBinaryType.addi);
                    addInstr2Block(riscvInstr1);
                }
            } else {
                RiscvInstr riscvInstr = new RiscvLi(new RiscvImm(offset_imm), defReg);
                addInstr2Block(riscvInstr);
                RiscvReg defReg1 = getExactAddr(value2Label.get(instr.getOP(0)));
                RiscvInstr riscvInstr1 =
                        new RiscvBinary(new ArrayList<>(Arrays.asList(defReg, defReg1))
                                , defReg, RiscvBinary.RiscvBinaryType.add);
                addInstr2Block(riscvInstr1);
            }
        } else if (instr.getOP(0) instanceof Arg || !value2Reg.containsKey(instr.getOP(0))) {  //局部
            assert instr.getOP(0) instanceof Arg || instr.getOP(0) instanceof Instr;
            int offset_all;
            if (instr.getOP(0) instanceof Instr) {
                offset_all = -1 * curRVFunction.getOffset((Instr) instr.getOP(0)) + offset_imm;
                if(flag) {
                    getEleCount.put(instr, offset_all);
                    return;
                }
                if(offset_all < 2048 && offset_all >=-2048){
                    if(offset_all == 0) {
                        addInstr2Block(new RiscvMv(RiscvCPUReg.getRiscvCPUReg(2), defReg));
                    } else {
                        addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                                new RiscvImm(offset_all))), defReg, RiscvBinary.RiscvBinaryType.addi));
                    }
                } else {
                    var li = new RiscvLi(new RiscvImm(offset_all), defReg);
                    addInstr2Block(li);
                    var add = new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                            defReg)), defReg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(add);
                }
            } else {
//                offset_all = -1 * curRVFunction.getOffset((Arg) instr.getOP(0)) + offset_imm;
                var argreg = getRegByValue(instr.getOP(0));//必须有，arg都存到了指定区域中，就算溢出也在栈里
                if (offset_imm < 2048 && offset_imm >= -2048) {
                    if(offset_imm == 0) {
                        RiscvInstr riscvInstr1 = new RiscvMv(argreg, defReg);
                        addInstr2Block(riscvInstr1);
                    } else {
                        RiscvInstr riscvInstr1 = new RiscvBinary(
                                new ArrayList<>(Arrays.asList(argreg, new RiscvImm(offset_imm)))
                                , defReg, RiscvBinary.RiscvBinaryType.addi);
                        addInstr2Block(riscvInstr1);
                    }
                } else {
                    RiscvInstr riscvInstr = new RiscvLi(new RiscvImm(offset_imm), defReg);
                    addInstr2Block(riscvInstr);
                    RiscvInstr riscvInstr1 =
                            new RiscvBinary(new ArrayList<>(Arrays.asList(defReg, argreg))
                                    , defReg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(riscvInstr1);
                }
            }
        } else {
            var basereg = getRegByValue(instr.getOP(0));
            if (offset_imm < 2048 && offset_imm >= -2048) {
                if(offset_imm == 0) {
                    RiscvInstr ins1 = new RiscvMv(basereg, defReg);
                    addInstr2Block(ins1);
                } else {
                    RiscvInstr ins1 = new RiscvBinary(
                            new ArrayList<>(Arrays.asList(basereg, new RiscvImm(offset_imm))),
                            defReg, RiscvBinary.RiscvBinaryType.addi);
                    addInstr2Block(ins1);
                }
            } else {
                RiscvInstr ins0 = new RiscvLi(new RiscvImm(offset_imm), defReg);
                addInstr2Block(ins0);
                RiscvInstr ins1 = new RiscvBinary(
                        new ArrayList<>(Arrays.asList(basereg, defReg)),
                        defReg, RiscvBinary.RiscvBinaryType.add);
                addInstr2Block(ins1);
            }
        }
        Type type1 = typeori;
        for (int i = 1; i < instr.getOperands().size(); i++) {
            if (!(instr.getOP(i) instanceof ConstNumber)) {
                RiscvVirReg defReg1 =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                if((type1.getSpace() & (type1.getSpace())) == 0) {
                    int shift = getShift(type1.getSpace());
                    RiscvInstr riscvInstr1 =
                            new RiscvBinary(
                                    new ArrayList<>(
                                            Arrays.asList(getRegByValue(instr.getOP(i)), new RiscvImm(shift)))
                                    , defReg1, RiscvBinary.RiscvBinaryType.slli);
                    addInstr2Block(riscvInstr1);
                    RiscvReg oldDefReg = defReg;
                    defReg = new RiscvVirReg(irRegDispatcher,
                            RiscvVirReg.RegType.intType, curRVFunction);
                    RiscvInstr riscvInstr2 =
                            new RiscvBinary(new ArrayList<>(Arrays.asList(defReg1, oldDefReg))
                                    , defReg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(riscvInstr2);
                } else {
                    if((type1.getSpace() & (type1.getSpace() - 1)) == 0) {
                        int shift = 0;
                        int temp = type1.getSpace();
                        while(temp > 1) {
                            temp /= 2;
                            shift++;
                        }
                        addInstr2Block(new RiscvBinary(new ArrayList<>(
                                Arrays.asList(getRegByValue(instr.getOP(i)), new RiscvImm(shift))),
                                defReg1, RiscvBinary.RiscvBinaryType.slli));
                    } else {
                        RiscvVirReg reg = new RiscvVirReg(irRegDispatcher,
                                RiscvVirReg.RegType.intType, curRVFunction);
                        addInstr2Block(new RiscvLi(new RiscvImm(type1.getSpace()), reg));
                        addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(reg
                                , getRegByValue(instr.getOP(i))))
                                , defReg1, RiscvBinary.RiscvBinaryType.mul));
                    }
                    RiscvReg oldDefReg = defReg;
                    defReg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                    RiscvInstr riscvInstr2 =
                            new RiscvBinary(new ArrayList<>(Arrays.asList(defReg1, oldDefReg))
                                    , defReg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(riscvInstr2);
                }
            }
            if(type1 instanceof ArrayType)
                type1 = ((ArrayType)type1).t;
        }
        value2Reg.put(instr, defReg);
    }

    /**
     * alloca 不生成新寄存器value2reg无法查到 直接去对应RiscvFunction中查找offset
     */
    private void parseAlloca(Alloca instr) {
        curRVFunction.alloc(instr);
    }

    private void parseLoad(Load instr) {
        RiscvReg srcReg, defReg;
        RiscvInstr riscvInstr;
        if (value2Label.containsKey(instr.getOP(0))) {
            srcReg = getExactAddr(value2Label.get(instr.getOP(0)));
        } else {
            srcReg = getRegByValue(instr.getOP(0));
        }
        if (instr.getType() instanceof FloatType) {
            // flw rd,imm(rs)
            defReg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
            riscvInstr = new RiscvFlw(defReg, new RiscvImm(0), srcReg);
        } else {
            // lw rd,imm(rs)
            defReg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
            riscvInstr = new RiscvLw(new RiscvImm(0), srcReg, defReg);
        }
        addInstr2Block(riscvInstr);
        value2Reg.put(instr, defReg);
    }

    private void parseStore(Store instr) {
        RiscvReg srcReg, defReg;
        if (value2Label.containsKey(instr.getDest())) {
            srcReg = getExactAddr(value2Label.get(instr.getDest()));
        } else {
            srcReg = getRegByValue(instr.getDest());
        }
        if (instr.getVal() instanceof ConstNumber) {
            if (instr.getVal().getType() instanceof FloatType) {
                defReg =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType, curRVFunction);
                defReg = (RiscvVirReg) getOperandFromValueNoimm(instr.getVal(), defReg);
                RiscvInstr riscvInstr = new RiscvFsw(defReg, new RiscvImm(0), srcReg);
                addInstr2Block(riscvInstr);
            } else {
                defReg =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                RiscvInstr riscvInstr =
                        new RiscvLi(new RiscvImm((ConstNumber) instr.getVal()), defReg);
                addInstr2Block(riscvInstr);
                RiscvInstr riscvInstr1 = new RiscvSw(defReg, new RiscvImm(0), srcReg);
                addInstr2Block(riscvInstr1);
            }
        } else {
            if (instr.getVal().getType() instanceof FloatType) {
                defReg = getRegByValue(instr.getVal());
                RiscvInstr riscvInstr = new RiscvFsw(defReg, new RiscvImm(0), srcReg);
                addInstr2Block(riscvInstr);
            } else {
                defReg = getRegByValue(instr.getVal());
                RiscvInstr riscvInstr = new RiscvSw(defReg, new RiscvImm(0), srcReg);
                addInstr2Block(riscvInstr);
            }
        }
    }

    private void parseRet(Ret instr) {
        if (instr.getOperands().size() != 0 && !(instr.getOP(0) instanceof VoidValue)) {
            if (instr.getOP(0) instanceof ConstNumber) {
                if (instr.getOP(0).getType() instanceof FloatType) {
                    // flw rd,rs,rt
                    RiscvFPUReg a0Reg = RiscvFPUReg.getRiscvFPUReg(10);
                    getOperandFromValueNoimm(instr.getOP(0), a0Reg);
                } else {
                    // li rd,rs,rt
                    RiscvCPUReg a0Reg = RiscvCPUReg.getRiscvCPUReg(10);
                    getOperandFromValueNoimm(instr.getOP(0), a0Reg);
                }
            } else {
                if (instr.getOP(0).getType() instanceof FloatType) {
                    // mv rd,rs,rt
                    RiscvFPUReg a0Reg = RiscvFPUReg.getRiscvFPUReg(10);
                    RiscvOperand srcReg = getRegByValue(instr.getOP(0));
                    RiscvInstr riscvInstr = new RiscvFmv((RiscvReg) srcReg, a0Reg);
                    addInstr2Block(riscvInstr);
                } else {
                    // mv rd,rs,rt
                    RiscvCPUReg a0Reg = RiscvCPUReg.getRiscvCPUReg(10);
                    RiscvOperand srcReg = getRegByValue(instr.getOP(0));
                    RiscvInstr riscvInstr = new RiscvMv((RiscvReg) srcReg, a0Reg);
                    addInstr2Block(riscvInstr);
                }
            }
        }
        // jr ra
        RiscvCPUReg raReg = RiscvCPUReg.getRiscvCPUReg(1);
        RiscvInstr mv = new RiscvMv(curRVFunction.getRaReg(), raReg);
        addInstr2Block(mv);
        RiscvInstr riscvInstr = new RiscvJr(raReg);
        addInstr2Block(riscvInstr);
    }

    private void parseMove(Move instr, BasicBlock basicBlock, Function function) {
        if (!value2Reg.containsKey(instr.getOP(0)) && !(instr.getOP(0) instanceof Arg)) {
            value2Reg.put(instr.getOP(0), new RiscvVirReg(irRegDispatcher,
                    instr.getOP(0).type instanceof FloatType ?
                            RiscvVirReg.RegType.floatType : RiscvVirReg.RegType.intType, curRVFunction));
        }
        if(instr.getOP(0).type instanceof FloatType){
            var target = getRegByValue(instr.getOP(0));
            var from = getOperandFromValueNoimm(instr.getOP(1), null);
            var fmv = new RiscvFmv((RiscvReg) from, target);
            addInstr2Block(fmv);
        }
        else{
            var target = getRegByValue(instr.getOP(0));
            var from = getOperandFromValueNoimm(instr.getOP(1), null);
            var mv = new RiscvMv((RiscvReg) from, target);
            addInstr2Block(mv);
        }
    }

    private RiscvReg getExactAddr(RiscvLabel label) {
        if(labelAlreadyInReg.containsKey(label)) {
            return labelAlreadyInReg.get(label);
        }
        RiscvReg reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvInstr ins1 = new RiscvLui(label.hi(), reg);
        addInstr2Block(ins1);
        RiscvReg reg1 = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
        RiscvInstr ins2 = new RiscvBinary(new ArrayList<>(Arrays.asList(reg, label.lo())),
                reg1, RiscvBinary.RiscvBinaryType.addi);
        addInstr2Block(ins2);
        labelAlreadyInReg.put(label, reg1);
        return reg1;
    }

    private RiscvOperand getOperandFromValueMayimm(Value value, RiscvReg reg) {
        return getOperandFromValueMayimm(value, reg, true);
    }

    private RiscvOperand getOperandFromValueMayimm(Value value, RiscvReg reg, boolean flag) {
        if(flag) {
            if (value instanceof ConstNumber constNumber) {
                if (value.type instanceof IntType) {
                    if ((int) (constNumber.value) <= 2047
                            && (int) (constNumber.value) >= -2048) {
                        return new RiscvImm(constNumber);
                    }
                }
            }
            return getOperandFromValueNoimm(value, reg);
        } else {
            if (value instanceof ConstNumber constNumber) {
                if (value.type instanceof IntType) {
                    if ((int) (constNumber.value) <= 2048
                            && (int) (constNumber.value) >= -2047) {
                        return new RiscvImm(constNumber);
                    }
                }
            }
            return getOperandFromValueNoimm(value, reg);
        }
    }

    private RiscvOperand getOperandFromValueNoimm(Value value, RiscvReg reg1) {
        if(value instanceof GetElementPtrInst) {
            assert value2Reg.containsKey(value) || getEleCount.containsKey(value);
            if(getEleCount.containsKey(value)) {
                RiscvReg reg = (reg1 == null) ? new RiscvVirReg(irRegDispatcher,
                        RiscvVirReg.RegType.intType, curRVFunction):reg1;
                int offset_all = getEleCount.get(value);
                if(offset_all < 2048 && offset_all >=-2048){
                    if(offset_all == 0) {
                        addInstr2Block(new RiscvMv(RiscvCPUReg.getRiscvCPUReg(2), reg));
                    } else {
                        addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                                new RiscvImm(offset_all))), reg, RiscvBinary.RiscvBinaryType.addi));
                    }
                }else{
                    var li = new RiscvLi(new RiscvImm(offset_all), reg);
                    addInstr2Block(li);
                    var add = new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                            reg)), reg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(add);
                }
                return reg;
            } else {
                if (!value2Reg.containsKey(value)) {
                    value2Reg.put(value, new RiscvVirReg(irRegDispatcher,
                            value.type instanceof FloatType ? RiscvVirReg.RegType.floatType
                                    : RiscvVirReg.RegType.intType, curRVFunction));
                }
                return getRegByValue(value);
            }
        } else if (value instanceof Instr) {
            if (!value2Reg.containsKey(value)) {
                value2Reg.put(value, new RiscvVirReg(irRegDispatcher,
                        value.type instanceof FloatType ? RiscvVirReg.RegType.floatType
                                : RiscvVirReg.RegType.intType, curRVFunction));
            }
            return getRegByValue(value);
        } else if (value instanceof ConstNumber constNumber) {
            if (value.type instanceof FloatType) {
                RiscvFloatConst label;
                if(floats.containsKey((Double) constNumber.value)) {
                    label = floats.get((Double) constNumber.value);
                } else {
                    label = new RiscvFloatConst("float" + floats.size(),
                            (Double) constNumber.value);
                    floats.put((Double) constNumber.value, label);
                }
                RiscvReg reg =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                RiscvInstr lui = new RiscvLui(label.hi(), reg);
                addInstr2Block(lui);
                RiscvReg reg2;
                if (reg1 != null) {
                    reg2 = reg1;
                    //assert reg1.regType == RiscvVirReg.RegType.floatType;
                } else {
                    reg2 = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType,
                            curRVFunction);
                }
                RiscvInstr flw = new RiscvFlw(reg2, label.lo(), reg);
                addInstr2Block(flw);
                return reg2;
            } else if (value.type instanceof IntType) {
                RiscvReg reg;
                if (reg1 != null) {
                    reg = reg1;
                } else {
                    reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType,
                            curRVFunction);
                }
                RiscvInstr ins = new RiscvLi(new RiscvImm((int) (constNumber.value)), reg);
                addInstr2Block(ins);
                return reg;
            }
        } else if (value instanceof Arg) {
            return getRegByValue(value);
        } else {
            assert false;
        }
        return null;
    }

    public RiscvReg getRegByValue(Value value) {
        if(value instanceof GetElementPtrInst) {
            if(getEleCount.containsKey(value)) {
                int offset_all = getEleCount.get(value);
                RiscvReg reg = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                if(offset_all < 2048 && offset_all >=-2048){
                    if(offset_all == 0) {
                        addInstr2Block(new RiscvMv(RiscvCPUReg.getRiscvCPUReg(2), reg));
                    } else {
                        addInstr2Block(new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                                new RiscvImm(offset_all))), reg, RiscvBinary.RiscvBinaryType.addi));
                    }
                }else{
                    var li = new RiscvLi(new RiscvImm(offset_all), reg);
                    addInstr2Block(li);
                    var add = new RiscvBinary(new ArrayList<>(Arrays.asList(RiscvCPUReg.getRiscvCPUReg(2),
                            reg)), reg, RiscvBinary.RiscvBinaryType.add);
                    addInstr2Block(add);
                }
                return reg;
            } else {
                assert value2Reg.containsKey(value);
                return value2Reg.get(value);
            }
        } else if (value instanceof Arg) {
            if (value2Reg.containsKey(value)) {
                return value2Reg.get(value);
            } else {
                int offset = -1 * curRVFunction.getOffset(value);
                RiscvReg reg =
                        new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.intType, curRVFunction);
                if(offset >= -2028 && offset < 2028) {
                    if(value.type instanceof FloatType) {
                        RiscvReg reg2 = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType,
                                curRVFunction);
                        RiscvInstr ins3 = new RiscvFld(
                                reg2, new RiscvImm(offset), RiscvCPUReg.getRiscvCPUReg(2));
                        addInstr2Block(ins3);
                        value2Reg.put(value, reg2);
                    } else {
                        RiscvInstr ins3 = new RiscvLd(new RiscvImm(offset), RiscvCPUReg.getRiscvCPUReg(2), reg);
                        addInstr2Block(ins3);
                        value2Reg.put(value, reg);
                    }
                    return value2Reg.get(value);
                } else {
                    addInstr2Block(new RiscvLi(new RiscvImm(offset), reg));
                    addInstr2Block(new RiscvBinary(
                            new ArrayList<>(Arrays.asList(reg, RiscvCPUReg.getRiscvCPUReg(2)))
                            , reg, RiscvBinary.RiscvBinaryType.add));
                }
                if (value.type instanceof FloatType) {
                    RiscvReg reg2 = new RiscvVirReg(irRegDispatcher, RiscvVirReg.RegType.floatType,
                            curRVFunction);
                    RiscvInstr ins3 = new RiscvFld(reg2, new RiscvImm(0), reg);
                    addInstr2Block(ins3);
                    value2Reg.put(value, reg2);
                } else {
                    RiscvInstr ins3 = new RiscvLd(new RiscvImm(0), reg, reg);
                    addInstr2Block(ins3);
                    value2Reg.put(value, reg);
                }
            }
        }
        return value2Reg.get(value);
    }
}
