package backend.process;

import backend.component.*;
import backend.instruction.*;
import backend.operand.*;
import ir.IrInstr;
import ir.IrModule;
import ir.instr.*;
import ir.type.*;
import ir.value.*;
import utils.Pair;

import java.util.*;

public class ParseIr {
    private IrModule irModule;
    private ObjModule objModule = new ObjModule();
    private HashMap<String, ObjGlobalVar> gMap = new HashMap<>();
    private HashMap<Float, ObjGlobalVar> fMap = new HashMap<>();
    private HashMap<String, ObjFunction> funcMap = new HashMap<>();
    private HashMap<String, ObjBlock> bMap = new HashMap<>();

    public enum LoadStore {
        LOAD, STORE;

        public ObjInstr spawn(String type, ObjOperand dst, ObjOperand addr, ObjOperand offset) {
            return switch (this) {
                case LOAD -> new ObjLoad(type, dst, addr, offset);
                case STORE -> new ObjStore(addr, dst, offset, type);
            };
        }
    }

    public static List<ObjInstr> spawnLoadStore(LoadStore ls, String type, ObjOperand dst, ObjOperand addr, ObjOperand offset) {
        if (!(offset instanceof ObjImm)) {
            throw new RuntimeException("Offset must be immediate");
        }
        List<ObjInstr> results = new ArrayList<>(4);
        if (ls == LoadStore.STORE && dst instanceof ObjImm) {
            // TODO: Will we have float imm?
            ObjVirReg tmp = new ObjVirReg();
            results.add(new ObjMove(dst, tmp));
            dst = tmp;
        }
        int offsetValue = ((ObjImm) offset).getImmediate();
        if (-2048 <= offsetValue && offsetValue <= 2047) {
            results.add(ls.spawn(type, dst, addr, offset));
        } else if (-4096 <= offsetValue && offsetValue <= 4094) {
            // Make it can use addi
            int halfOffset = offsetValue / 2;
            int otherOffset = offsetValue - halfOffset;
            ObjVirReg tmp = new ObjVirReg();
            results.addAll(List.of(
                    new ObjBinary("add", "i32", tmp, addr, new ObjImm(halfOffset)),
                    ls.spawn(type, dst, tmp, new ObjImm(otherOffset))
            ));
        } else {
            ObjVirReg tmpOffset = new ObjVirReg();
            ObjVirReg tmpAddr = new ObjVirReg();
            results.addAll(List.of(
                    new ObjMove(offset, tmpOffset),
                    new ObjBinary("add", "i32", tmpAddr, addr, tmpOffset),
                    ls.spawn(type, dst, tmpAddr, new ObjImm(0))
            ));
        }
        return results;
    }

    public ParseIr(IrModule irModule) {
        this.irModule = irModule;
        ObjPhyReg.SP.notNeedColor();
        ObjPhyReg.RA.notNeedColor();
        ObjPhyReg.ZERO.notNeedColor();
    }

    public ObjModule parse() {
        parseGlobalVariables();
        parseFunctions();
        //待补充
        objModule.setGlobalVars(new ArrayList<>() {{
            addAll(gMap.values());
            addAll(fMap.values());
        }});
        return objModule;
    }

    public void parseGlobalVariables() {
        for (Variable var : irModule.getGlobalVariables()) {
            parseGlobal(var);
        }
        for (Constant constant : irModule.getGlobalConstants()) {
            parseGlobal(constant);
        }
    }

    public void parseGlobal(Constant var) {
        ObjGlobalVar globalVar = null;
        Object initValue;
        if (var instanceof Variable variable) {
            initValue = variable.getInitialValue();
        } else {
            initValue = var.initialValue;
        }
        if (var.getType() instanceof IntegerType) {
            if (initValue != null) {
                globalVar = new ObjGlobalVar(((Integer) initValue), true);
            } else {
                globalVar = new ObjGlobalVar(0, false);
            }
        } else if (var.getType() instanceof FloatType) {
            if (initValue != null) {
                globalVar = new ObjGlobalVar(((Float) initValue), true);
            } else {
                globalVar = new ObjGlobalVar(Float.valueOf(0), false);
            }
        } else if (var.getType() instanceof ArrayType) {
            IrType type = var.getType();
            int len = 1;
            int baseSize = 0;
            while (type instanceof ArrayType arrayType) {
                type = arrayType.getBase();
                len *= arrayType.getLength();
            }
            if (type instanceof IntegerType) {
                baseSize = 4;
            } else {
                baseSize = 8;
            }
            baseSize *= len;
            if (initValue != null) {
                TreeMap<Integer, Object> tmp = (TreeMap) initValue;
                TreeMap<Integer, Integer> arr = new TreeMap<>();
                if (type instanceof FloatType) {
                    for (Integer i : tmp.keySet()) {
                        arr.put(i, Float.floatToRawIntBits((Float) tmp.get(i)));
                    }
                } else {
                    for (Integer i : tmp.keySet()) {
                        arr.put(i, (Integer) tmp.get(i));
                    }
                }
                globalVar = new ObjGlobalVar(arr, true, baseSize, len);
            } else {
                globalVar = new ObjGlobalVar(null, false, baseSize, len);
            }
        }
        //ObjGlobalVar objVar = parseGlobalVar(var);
        //objModule.addGlobalVar(globalVar);
        gMap.put(var.getName(), globalVar);
    }


    private void iMap() {
        for (Function f : irModule.getFunctions())
            for (BasicBlock block : f.getBasicBlocks()) {
                ObjBlock objBlock = bMap.get(f.getName() + block.getName());
//                objBlock.depth = block.getLoopDepth(); TODO
            }
    }

    public void parseFunctions() {
        iMap();
        for (Function function : irModule.getFunctions()) {
            //确定alloca、ra、args
            ObjFunction objFunction = new ObjFunction(function.getName().substring(1), (LinkedList<Argument>) function.getArguments(), new LinkedList<>());
            HashMap<String, ObjBlock> blockMap = objFunction.getBlockMap();
            for (BasicBlock block : function.getBasicBlocks()) {
                ObjBlock objBlock = new ObjBlock(objFunction);
                objFunction.addObjBlock(objBlock);
                blockMap.put(block.getName(), objBlock);
                bMap.put(objFunction.getName() + block.getName(), objBlock);
                for (IrInstr instr : block.getInstrs()) {
                    if (instr instanceof AllocaInstr allocaInstr) {
                        objFunction.addAllocaSize(allocaInstr.size);//alloca
                    } else if (instr instanceof TermInstr termInstr) {
                        if (termInstr.op == IrInstr.OpCode.CALL) {
                            objFunction.setRaSize();//ra
                            LinkedList<Value> arguments = new LinkedList<>(termInstr.getArguments());
                            int size = 0;
                            ArrayList<Value> intArgs = new ArrayList<>();
                            ArrayList<Value> floatArgs = new ArrayList<>();
                            for (Value value : arguments) {
                                if (value.getType() instanceof FloatType) {
                                    floatArgs.add(value);
                                } else {
                                    intArgs.add(value);
                                }
                            }
                            for (int i = 8; i < intArgs.size(); i++) {
                                if (intArgs.get(i).getType() instanceof IntegerType) {
                                    size += 4;
                                } else {
                                    size += 8;
                                }
                            }
                            if (floatArgs.size() > 8) {
                                size += (floatArgs.size() - 8) * 8;
                            }
                            objFunction.setArgsSize(size);//args
                        }
                    }
                }
            }
            funcMap.put(function.getName(), objFunction);
        }
        for (Function function : irModule.getFunctions()) {
            ObjFunction objFunction = funcMap.get(function.getName());
            List<BasicBlock> blocks = function.getBasicBlocks();
            HashMap<String, ObjReg> opMap = new HashMap<>();
            HashMap<String, ObjBlock> blockMap = objFunction.getBlockMap();//new HashMap<>();//BasicBlock.getName(),ObjBlock
            LinkedList<Pair<TermInstr, ObjBlock>> brs = new LinkedList<>();//等所有基本块建立完毕后，再处理br指令
            //parseArgs(function, objFunction, opMap, blockMap);
            for (BasicBlock block : blocks) {
                ObjBlock objBlock = blockMap.get(block.getName());//new ObjBlock(objFunction);
                blockMap.put(block.getName(), objBlock);
                for (IrInstr instr : block.getInstrs()) {
                    if (instr instanceof AllocaInstr allocaInstr) {
                        //objBlock.addInstr(new ObjAlloca(allocaInstr));
                        parseAlloca(objBlock, opMap, allocaInstr, objFunction);
                    } else if (instr instanceof BinaInstr binaInstr) {
                        parseBina(objBlock, opMap, binaInstr);
                    } else if (instr instanceof CommentInstr commentInstr) {
                        //objBlock.addInstr(parseComment(commentInstr));
                    } else if (instr instanceof ExtInstr extInstr) {
                        parseExt(objBlock, opMap, extInstr);
                    } else if (instr instanceof F2iInstr || instr instanceof I2fInstr) {
                        parseConversion(objBlock, opMap, instr);
                    } else if (instr instanceof ICmpInstr iCmpInstr) {
                        parseICmp(objBlock, opMap, iCmpInstr);
                    } else if (instr instanceof FCmpInstr fCmpInstr) {
                        parseFCmp(objBlock, opMap, fCmpInstr);
                    } else if (instr instanceof LoadInstr loadInstr) {
                        parseLoad(objBlock, opMap, loadInstr);
                    } else if (instr instanceof MoveInstr moveInstr) {
                        parseMove(objBlock, opMap, moveInstr);
                    } else if (instr instanceof NotInstr notInstr) {
                        parseNot(objBlock, opMap, notInstr);
                    } else if (instr instanceof PhiInstr phiInstr) {
                        parsePhi(objBlock, opMap, phiInstr, blockMap);
                    } else if (instr instanceof StoreInstr storeInstr) {
                        parseStore(objBlock, opMap, storeInstr);
                    } else if (instr instanceof TermInstr termInstr) {
//                        if (termInstr.getOp() == IrInstr.OpCode.BR) {
//                            brs.addLast(new Pair<>(termInstr, objBlock));
//                        }
                        parseTerm(objBlock, opMap, termInstr, objFunction, blockMap);
                    } else if (instr instanceof TruncInstr truncInstr) {
                        parseTrunc(objBlock, opMap, truncInstr);
                    }
                }
                // objFunction.addObjBlock(objBlock);
            }
//            for (Pair<TermInstr, ObjBlock> pair : brs) {
//                parseBr(pair.getSecond(), opMap, pair.getFirst(), blockMap);
//            }
            parseArgs(function, objFunction, opMap, blockMap);
            HashSet<ObjReg> objOperands = new HashSet<>(opMap.values());
            objFunction.setOperands(objOperands);//提供函数所使用的虚拟寄存器集合
//            resetIndex();
            objModule.addFunction(objFunction);
        }
    }

    public void setExit() {
        for (Function function : irModule.getFunctions()) {
            ObjFunction objFunction = funcMap.get(function.getName());
            if (function.getBbExit() == null) {
                objFunction.setExits(null);
                continue;
            }
            ObjBlock tmpb = bMap.get(function.getName() + function.getBbExit().getName());
            objFunction.setExits(tmpb);
        }
    }

    void resetIndex() {
        ObjVirReg.virRegIndex = 0;
        ObjFVirReg.fVirRegIndex = 0;
        ObjOperand.operandIndex = 0;
        ObjBlock.blockIndex = 0;
    }

    private List<ObjInstr> bfSpawnLoad(String type, ObjOperand dst, ObjOperand addr, int offset) {
        ObjOperand offsetOperand = new ObjImm(offset);
        ObjOperand tmpOffset = new ObjVirReg();
        ObjOperand finalAddr = new ObjVirReg();
        return List.of(
                new ObjMove("li.stack", offsetOperand, tmpOffset),
                new ObjBinary("add", "i32", finalAddr, addr, tmpOffset),
                new ObjLoad(type, dst, finalAddr, new ObjImm(0))
        );

    }

    void parseArgs(Function function, ObjFunction objFunction, HashMap<String, ObjReg> opMap, HashMap<String, ObjBlock> blockMap) {
        //先不处理递归函数有关的优化
        int num_f = 0;
        ArrayList<Argument> fArgs = new ArrayList<>();
        int num_i = 0;
        ArrayList<Argument> iArgs = new ArrayList<>();
        // int offset = objFunction.getStackSize();
        for (Argument argument : function.getArguments()) {
            if (argument.getType() instanceof FloatType) {
                num_f++;
                fArgs.add(argument);
            } else {
                num_i++;
                iArgs.add(argument);
            }
        }
        ObjBlock firstBlock = new ObjBlock(objFunction);
        for (int i = 0; i < num_f && i < 8; i++) {
            ObjOperand objOperand = parseFloatOperand(fArgs.get(i), opMap, firstBlock);
            firstBlock.addInstr(new ObjMove(ObjFPhyReg.regs.get(10 + i), objOperand));
        }
        for (int i = 0; i < num_i && i < 8; i++) {
            ObjOperand objOperand = parseIntOperand(iArgs.get(i), opMap);
            firstBlock.addInstr(new ObjMove(ObjPhyReg.regs.get(10 + i), objOperand));
        }
        int offset = 0;
        for (int i = 8; i < num_f; i++) {
            ObjOperand objOperand = parseFloatOperand(fArgs.get(i), opMap, firstBlock);
            var instrs = bfSpawnLoad("flw", objOperand, ObjPhyReg.SP, offset);
            instrs.forEach(firstBlock::addInstr);
            offset += 4;//为什么是4呢？
        }
        for (int i = 8; i < num_i; i++) {
            ObjOperand objOperand = parseIntOperand(iArgs.get(i), opMap);
            if (iArgs.get(i).getType() instanceof PointerType) {
                var instrs = bfSpawnLoad("ld", objOperand, ObjPhyReg.SP, offset);
                instrs.forEach(firstBlock::addInstr);
                offset += 8;
            } else {
                var instrs = bfSpawnLoad("lw", objOperand, ObjPhyReg.SP, offset);
                instrs.forEach(firstBlock::addInstr);
                offset += 4;
            }
        }
        //压栈
        int stackSize = objFunction.getStackSize();
        int raSize = objFunction.getRaSize();
        if (raSize > 0) {
            var lst = spawnLoadStore(LoadStore.STORE, "sd", ObjPhyReg.RA, ObjPhyReg.SP, new ObjImm(stackSize - 8));
            for (int i = lst.size() - 1; i >= 0; i--) {
                firstBlock.addFirstInstr(lst.get(i));
            }
            ((ObjStore) lst.get(lst.size() - 1)).setCanBeAlter(true);
        }
        if (stackSize >= 0) {  // 是这样的，等于 0 我们也先加一下，万一后面要改呢
            ObjBinary objBinary = new ObjBinary("add", "i32", ObjPhyReg.SP, ObjPhyReg.SP, new ObjImm(-stackSize));
            objFunction.setAllocStack(objBinary);
            objBinary.setCanBeAlter(true);
            firstBlock.addFirstInstr(objBinary);
        }

        objFunction.addFirstBlock(firstBlock);
        //出栈
//        Iterator<ObjInstr> it = objFunction.getBlocks().getLast().getInstrs().descendingIterator();
//        int tmp = objFunction.getBlocks().getLast().getInstrs().size();
//        while (it.hasNext()) {
//            ObjInstr instr = it.next();
//            if (!(instr instanceof ObjRet)) {
//                //通过在ret指令附加addi sp sp offset来实现
//                break;
//            }
//            tmp--;
//        }
//        objFunction.getBlocks().getLast().getInstrs().add(tmp, new ObjBinary("add", "i32", ObjPhyReg.SP, ObjPhyReg.SP, new ObjImm(offset)));
    }

    public void parseAlloca(ObjBlock block, HashMap<String, ObjReg> opMap, AllocaInstr allocaInstr, ObjFunction objFunction) {
        ObjOperand dst = parseIntOperand(allocaInstr.getOperands()[0], opMap);
        int allocaStart = objFunction.getAllocaStart();
        objFunction.addAllocaStart(allocaInstr.size);
        if (allocaStart <= 2047) {
            block.addInstr(new ObjBinary("add", "i32", dst, ObjPhyReg.SP, new ObjImm(allocaStart)));
        } else {
            ObjOperand tmp = new ObjVirReg();
            block.addInstr(new ObjMove("li", new ObjImm(allocaStart), tmp));
            block.addInstr(new ObjBinary("add", "i32", dst, ObjPhyReg.SP, tmp));
        }
    }

    private ObjOperand handleUnacceptableImm(ObjBlock block, ObjOperand operand) {
        if (operand instanceof ObjImm imm) {
            if (imm.getImmediate() == 0) {
                return ObjPhyReg.ZERO;
            }
            ObjVirReg virReg = new ObjVirReg();
            block.addInstr(new ObjMove(imm, virReg));
            return virReg;
        }
        return operand;
    }

    private ObjInstr handleBothImm(ObjBinary instr) {
        if (!(instr.getSrc1() instanceof ObjImm) || !(instr.getSrc2() instanceof ObjImm)) {
            return instr;
        }
        int imm1 = ((ObjImm) instr.getSrc1()).getImmediate();
        int imm2 = ((ObjImm) instr.getSrc2()).getImmediate();
        return switch (instr.getType()) {
            case "addw", "add" -> new ObjMove(new ObjImm(imm1 + imm2), instr.getDst());
            case "sub" -> new ObjMove(new ObjImm(imm1 - imm2), instr.getDst());
            case "mul" -> new ObjMove(new ObjImm(imm1 * imm2), instr.getDst());
            case "div" -> new ObjMove(new ObjImm(imm1 / imm2), instr.getDst());
            case "rem" -> new ObjMove(new ObjImm(imm1 % imm2), instr.getDst());
            case "and" -> new ObjMove(new ObjImm(imm1 & imm2), instr.getDst());
            case "or" -> new ObjMove(new ObjImm(imm1 | imm2), instr.getDst());
            case "sll" -> new ObjMove(new ObjImm(imm1 << imm2), instr.getDst());
            case "slt" -> new ObjMove(new ObjImm(imm1 < imm2 ? 1 : 0), instr.getDst());
            case "srl" -> new ObjMove(new ObjImm(imm1 >>> imm2), instr.getDst());
            case "sra" -> new ObjMove(new ObjImm(imm1 >> imm2), instr.getDst());
            case "xor" -> new ObjMove(new ObjImm(imm1 ^ imm2), instr.getDst());
            default -> instr;
        };
    }

    public void parseBina(ObjBlock block, HashMap<String, ObjReg> opMap, BinaInstr binaInstr) {
        if (binaInstr.pointerOffset) {
//            la x10, @v8         # 将全局变量@v8的地址加载到寄存器x10中
//            slli x11, x11, 2    # 将%3乘以4（左移2位），计算字节偏移量
//            add x12, x10, x11   # 将基地址和字节偏移量相加，结果保存在x12中
            ObjOperand src1, src2, dst;
            if (IrValue.from(binaInstr.val1).isGlobal) {
                src1 = new ObjVirReg();
                block.addInstr(new ObjLoad("la", src1, new ObjLabel(gMap.get(binaInstr.val1.getName()).getName()), new ObjImm(0)));
            } else {
                src1 = parseIntOperand(binaInstr.val1, opMap);
            }
            if (IrValue.from(binaInstr.val2).getType() instanceof IntegerType) {
                src2 = new ObjVirReg();//需要新建还是直接原样？
                block.addInstr(handleBothImm(new ObjBinary("sll", "i32", src2, parseOperand(binaInstr.val2, opMap, block), new ObjImm(2))));
            } else {
                src2 = new ObjVirReg();//需要新建还是直接原样？
                block.addInstr(handleBothImm(new ObjBinary("sll", "i32", src2, parseOperand(binaInstr.val2, opMap, block), new ObjImm(3))));
            }
            dst = parseIntOperand(binaInstr.result, opMap);
            ObjBinary objBinary = new ObjBinary("add", "i32", dst, src1, src2);
            block.addInstr(objBinary);
            return;
        }
        // 疑问：需要优化算出两个立即数相加吗？  -- 需要的吧，因为前端在不优化的情况下，会干出这种事情！而 addi reg, 1, 1 是不合法的
        ObjOperand dst, src1, src2;
        if (binaInstr.op == IrInstr.OpCode.ADD) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("addw", "i32", dst, src1, src2)));  // Handled in Bina
        } else if (binaInstr.op == IrInstr.OpCode.FADD) {
            dst = parseFloatOperand(binaInstr.result, opMap, block);
            src1 = parseFloatOperand(binaInstr.val1, opMap, block);
            src2 = parseFloatOperand(binaInstr.val2, opMap, block);
            // TODO: 浮点数立即数怎么加？
            block.addInstr(new ObjBinary("fadd.s", "float", dst, src1, src2));
        } else if (binaInstr.op == IrInstr.OpCode.SUB) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            if (src1 instanceof ObjImm imm && !(src2 instanceof ObjImm)) {
                if (imm.getImmediate() == 0) {
                    block.addInstr(new ObjBinary("sub", "i32", dst, ObjPhyReg.ZERO, src2));
                } else {
                    ObjOperand tmp = new ObjVirReg();
                    block.addInstr(new ObjMove(new ObjImm(imm.getImmediate()), tmp));
                    block.addInstr(new ObjBinary("sub", "i32", dst, tmp, src2));
                }
            } else {
                block.addInstr(handleBothImm(new ObjBinary("sub", "i32", dst, src1, src2)));  // Handled in Bina
            }
        } else if (binaInstr.op == IrInstr.OpCode.FSUB) {
            dst = parseFloatOperand(binaInstr.result, opMap, block);
            src1 = parseFloatOperand(binaInstr.val1, opMap, block);
            src2 = parseFloatOperand(binaInstr.val2, opMap, block);
            // TODO: 浮点数立即数怎么减？
            block.addInstr(new ObjBinary("fsub.s", "float", dst, src1, src2));
        } else if (binaInstr.op == IrInstr.OpCode.MUL) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            if (src1 instanceof ObjImm imm1 && src2 instanceof ObjImm imm2) {
                int result = imm1.getImmediate() * imm2.getImmediate();
                block.addInstr(new ObjMove(new ObjImm(result), dst));
            } else {
                block.addInstr(new ObjBinary("mul", "i32", dst,
                        handleUnacceptableImm(block, src1), handleUnacceptableImm(block, src2)));
            }
        } else if (binaInstr.op == IrInstr.OpCode.FMUL) {
            dst = parseFloatOperand(binaInstr.result, opMap, block);
            src1 = parseFloatOperand(binaInstr.val1, opMap, block);
            src2 = parseFloatOperand(binaInstr.val2, opMap, block);
            block.addInstr(new ObjBinary("fmul.s", "float", dst, src1, src2));
        } else if (binaInstr.op == IrInstr.OpCode.SDIV) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            if (src1 instanceof ObjImm imm1 && src2 instanceof ObjImm imm2) {
                int result = imm1.getImmediate() / imm2.getImmediate();
                block.addInstr(new ObjMove(new ObjImm(result), dst));
            } else {
                block.addInstr(new ObjBinary("div", "i32", dst,
                        handleUnacceptableImm(block, src1), handleUnacceptableImm(block, src2)));
            }
        } else if (binaInstr.op == IrInstr.OpCode.FDIV) {
            dst = parseFloatOperand(binaInstr.result, opMap, block);
            src1 = parseFloatOperand(binaInstr.val1, opMap, block);
            src2 = parseFloatOperand(binaInstr.val2, opMap, block);
            block.addInstr(new ObjBinary("fdiv.s", "float", dst, src1, src2));
        } else if (binaInstr.op == IrInstr.OpCode.SREM) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            if (src1 instanceof ObjImm imm1 && src2 instanceof ObjImm imm2) {
                int result = imm1.getImmediate() % imm2.getImmediate();
                block.addInstr(new ObjMove(new ObjImm(result), dst));
            } else {
                block.addInstr(new ObjBinary("rem", "i32", dst,
                        handleUnacceptableImm(block, src1), handleUnacceptableImm(block, src2)));
            }
        } else if (binaInstr.op == IrInstr.OpCode.FREM) {
            //TODO:浮点数取余
        } else if (binaInstr.op == IrInstr.OpCode.SHL) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("sll", "i32", dst, src1, src2)));
        } else if (binaInstr.op == IrInstr.OpCode.LSHR) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("srl", "i32", dst, src1, src2)));
        } else if (binaInstr.op == IrInstr.OpCode.ASHR) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("sra", "i32", dst, src1, src2)));
        } else if (binaInstr.op == IrInstr.OpCode.AND) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("and", "i32", dst, src1, src2)));
        } else if (binaInstr.op == IrInstr.OpCode.OR) {
            dst = parseIntOperand(binaInstr.result, opMap);
            src1 = parseIntOperand(binaInstr.val1, opMap, block);
            src2 = parseIntOperand(binaInstr.val2, opMap, block);
            block.addInstr(handleBothImm(new ObjBinary("or", "i32", dst, src1, src2)));
        }
    }

    public ObjOperand parseIntOperand(Value value, HashMap<String, ObjReg> opMap) {
        if (IrValue.from(value).isLiteral) {
            //立即数
            return new ObjImm(IrValue.from(value).intLiteral);
        }
        if (!opMap.containsKey(value.getName())) {
            opMap.put(value.getName(), new ObjVirReg());
        }
        return opMap.get(value.getName());
    }

    public ObjOperand parseIntOperand(Value value, HashMap<String, ObjReg> opMap, ObjBlock block) {
        ObjOperand operand = parseIntOperand(value, opMap);
        if (operand instanceof ObjImm imm) {
            if (imm.getImmediate() < -2048 || imm.getImmediate() > 2047) {
                ObjVirReg virReg = new ObjVirReg();
                block.addInstr(new ObjMove(imm, virReg));
                return virReg;
            }
        }
        return operand;
    }

    public ObjOperand parseFloatOperand(Value value, HashMap<String, ObjReg> opMap, ObjBlock block) {
        if (IrValue.from(value).isLiteral) {
            //浮点立即数，建立全局变量来解决
            //FLOAT0 .section 123456
            //lla a FLOAT0
            //flw a 0(a)
            //存疑：用浮点数作key可行吗？
            ObjGlobalVar globalVar = null;
            if (fMap.containsKey(IrValue.from(value).floatLiteral)) {
                globalVar = fMap.get(IrValue.from(value).floatLiteral);
            } else {
                globalVar = new ObjGlobalVar(IrValue.from(value).floatLiteral, true);
                fMap.put(value.into().floatLiteral, globalVar);
            }
            globalVar.setConst(true);
            //lla
            ObjLabel label = new ObjLabel(globalVar.getName());
            ObjVirReg loadDst = new ObjVirReg();
            ObjImm offset = new ObjImm(0);
            spawnLoadStore(LoadStore.LOAD, "lla", loadDst, label, offset).forEach(block::addInstr);
            //flw
            ObjFVirReg fVirReg = new ObjFVirReg();
            spawnLoadStore(LoadStore.LOAD, "flw", fVirReg, loadDst, offset).forEach(block::addInstr);
            return fVirReg;
        } else {
            if (!opMap.containsKey(value.getName())) {
                opMap.put(value.getName(), new ObjFVirReg());
            }
            return opMap.get(value.getName());
        }
    }

    public ObjOperand parseOperand(Value value, HashMap<String, ObjReg> opMap, ObjBlock block) {
        if (gMap.containsKey(value.getName())) {
            //la
            ObjVirReg addr = new ObjVirReg();
            block.addInstr(new ObjLoad("la", addr, new ObjLabel((gMap.get(value.getName())).getName()), new ObjImm(0)));
            //lw or flw.s
            ObjOperand dst;
            if (value.getType() instanceof FloatType) {
                dst = new ObjFVirReg();
                block.addInstr(new ObjLoad("flw.s", dst, addr, new ObjImm(0)));
            } else if (value.getType() instanceof IntegerType) {
                dst = new ObjVirReg();
                block.addInstr(new ObjLoad("lw", dst, addr, new ObjImm(0)));
            } else {
                dst = addr;
            }
            return dst;
        }
        if (value.getType().is("INT") || value.getType().is("int1") || value.getType().is("int32") || value.getType().is("ptr")) {
            return parseIntOperand(value, opMap);
        } else {
            return parseFloatOperand(value, opMap, block);
        }
    }

    public ObjOperand parseOperandButForPhiBranch(Value value, HashMap<String, ObjReg> opMap, ObjBlock block) {
        ObjInstr termInstr;
        if (block.getInstrs().isEmpty()) {
            termInstr = null;
        } else {
            termInstr = block.getInstrs().removeLast();
        }
        if (gMap.containsKey(value.getName())) {
            //la
            ObjVirReg addr = new ObjVirReg();
            block.addInstr(new ObjLoad("la", addr, new ObjLabel((gMap.get(value.getName())).getName()), new ObjImm(0)));
            //lw or flw.s
            ObjOperand dst;
            if (value.getType() instanceof FloatType) {
                dst = new ObjFVirReg();
                block.addInstr(new ObjLoad("flw.s", dst, addr, new ObjImm(0)));
            } else if (value.getType() instanceof IntegerType) {
                dst = new ObjVirReg();
                block.addInstr(new ObjLoad("lw", dst, addr, new ObjImm(0)));
            } else {
                dst = addr;
            }
            if (termInstr != null) {
                block.addInstr(termInstr);
            }
            return dst;
        }
        if (value.getType().is("INT") || value.getType().is("int1") || value.getType().is("int32") || value.getType().is("ptr")) {
            if (termInstr != null) {
                block.addInstr(termInstr);
            }
            return parseIntOperand(value, opMap);
        } else {
            ObjOperand flt = parseFloatOperand(value, opMap, block);
            if (termInstr != null) {
                block.addInstr(termInstr);
            }
            return flt;
        }
    }

    public ObjComment parseComment(CommentInstr instr) {
        return new ObjComment(instr);
    }

    public void parseExt(ObjBlock block, HashMap<String, ObjReg> opMap, ExtInstr extInstr) {
        if (extInstr.isSigned) {
            //先左移，再算数右移
            //slli
            ObjVirReg dst = new ObjVirReg();
            opMap.put(dst.getName(), dst);
            ObjOperand src1 = parseIntOperand(extInstr.getOperands()[1], opMap);
            ObjOperand src2 = new ObjImm(extInstr.toType.sizeof());
            block.addInstr(handleBothImm(new ObjBinary("sll", "i32", dst, src1, src2)));
            //srai
            ObjOperand dst2 = parseIntOperand(extInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("sra", "i32", dst2, dst, src2)));
        } else {
            //sext.w
            //andi 0xfff
            ObjOperand dst = parseIntOperand(extInstr.getOperands()[0], opMap);
            ObjOperand src1 = parseIntOperand(extInstr.getOperands()[1], opMap);
            ObjImm src2 = new ObjImm((int) (((long) 1 << (extInstr.toType.sizeof() * 8 + 1)) - 1));
            block.addInstr(handleBothImm(new ObjBinary("and", "i32", dst, src1, src2)));
        }
    }

    public void parseConversion(ObjBlock block, HashMap<String, ObjReg> opMap, IrInstr instr) {
        if (instr instanceof F2iInstr) {
            F2iInstr f2iInstr = (F2iInstr) instr;
            ObjOperand dst = parseIntOperand(f2iInstr.getOperands()[0], opMap);
            ObjOperand src = parseFloatOperand(f2iInstr.getOperands()[1], opMap, block);
            block.addInstr(new ObjConversion("fcvt.w.s", dst, src));
        } else {
            I2fInstr i2fInstr = (I2fInstr) instr;
            ObjOperand dst = parseFloatOperand(i2fInstr.getOperands()[0], opMap, block);
            ObjOperand src = handleUnacceptableImm(block, parseIntOperand(i2fInstr.getOperands()[1], opMap));
            block.addInstr(new ObjConversion("fcvt.s.w", dst, src));
        }
    }

    public void parseICmp(ObjBlock block, HashMap<String, ObjReg> opMap, ICmpInstr iCmpInstr) {
        ObjOperand a = parseIntOperand(iCmpInstr.getOperands()[1], opMap);
        ObjOperand b = parseIntOperand(iCmpInstr.getOperands()[2], opMap);
        if (!(a instanceof ObjImm) || !(b instanceof ObjImm)) {
            a = handleUnacceptableImm(block, a);
            b = handleUnacceptableImm(block, b);
        }
        if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.eq) {
            //xor tmp a b
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(handleBothImm(new ObjBinary("xor", "i32", tmp, a, b)));
            //sltu tmp2 Zero tmp
            ObjVirReg tmp2 = new ObjVirReg();
            block.addInstr(new ObjBinary("sltu", "i32", tmp2, ObjPhyReg.ZERO, tmp));
            //xor dst tmp2 1
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("xor", "i32", dst, tmp2, new ObjImm(1))));
        } else if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.ne) {
            //xor tmp a b
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(handleBothImm(new ObjBinary("xor", "i32", tmp, a, b)));
            //sltu dst Zero tmp
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(new ObjBinary("sltu", "i32", dst, ObjPhyReg.ZERO, tmp));
        } else if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.sgt) {
            //slt dst b a
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("slt", "i32", dst, b, a)));
        } else if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.sge) {
            //slt tmp a b
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(handleBothImm(new ObjBinary("slt", "i32", tmp, a, b)));
            //xor dst tmp 1
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("xor", "i32", dst, tmp, new ObjImm(1))));
        } else if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.slt) {
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("slt", "i32", dst, a, b)));
        } else if (iCmpInstr.getPredicate() == ICmpInstr.Predicate.sle) {
            //slt tmp b a
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(handleBothImm(new ObjBinary("slt", "i32", tmp, b, a)));
            //xor dst tmp 1
            ObjOperand dst = parseIntOperand(iCmpInstr.getOperands()[0], opMap);
            block.addInstr(handleBothImm(new ObjBinary("xor", "i32", dst, tmp, new ObjImm(1))));
        }
    }

    public void parseFCmp(ObjBlock block, HashMap<String, ObjReg> opMap, FCmpInstr fCmpInstr) {
        //问题:报不报错？o:二者都不是QNAN且符合条件a:二者有一个是QNAN或者符合条件
        if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.oeq || fCmpInstr.getPredicate() == FCmpInstr.Predicate.ueq) {
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            block.addInstr(new ObjBinary("feq.s", "float", dst, a, b));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.one || fCmpInstr.getPredicate() == FCmpInstr.Predicate.une) {
            //feq.s tmp a b
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(new ObjBinary("feq.s", "float", tmp, a, b));
            //xor dst tmp 1
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            block.addInstr(new ObjBinary("xor", "i32", dst, tmp, new ObjImm(1)));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.ogt || fCmpInstr.getPredicate() == FCmpInstr.Predicate.ugt) {
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            block.addInstr(new ObjBinary("fgt.s", "float", dst, a, b));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.oge || fCmpInstr.getPredicate() == FCmpInstr.Predicate.uge) {
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            block.addInstr(new ObjBinary("fge.s", "float", dst, a, b));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.olt || fCmpInstr.getPredicate() == FCmpInstr.Predicate.ult) {
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            block.addInstr(new ObjBinary("flt.s", "float", dst, a, b));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.ole || fCmpInstr.getPredicate() == FCmpInstr.Predicate.ule) {
            ObjOperand dst = parseIntOperand(fCmpInstr.getOperands()[0], opMap);
            ObjOperand a = parseFloatOperand(fCmpInstr.getOperands()[1], opMap, block);
            ObjOperand b = parseFloatOperand(fCmpInstr.getOperands()[2], opMap, block);
            block.addInstr(new ObjBinary("fle.s", "float", dst, a, b));
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.ord) {
            //fclass.s tmp1 a
            //fclass.s tmp2 b
            //and tmp3 tmp1 tmp2
            //and tmp4 tmp3 1<<9
            //slt dst ZERO tmp4
        } else if (fCmpInstr.getPredicate() == FCmpInstr.Predicate.uno) {
            //fclass.s tmp1 a
            //fclass.s tmp2 b
            //or tmp3 tmp1 tmp2
            //and tmp4 tmp3 1<<9
            //slt dst ZERO tmp4
        }
    }

    public void parseLoad(ObjBlock block, HashMap<String, ObjReg> opMap, LoadInstr loadInstr) {
        //问题:如何判断操作数类型，选择恰当指令？
        Value op2 = loadInstr.getOperands()[1];
        Value op1 = loadInstr.getOperands()[0];
        ObjOperand addr;
        if (IrValue.from(op2).isGlobal) {
            addr = new ObjVirReg();
            block.addInstr(new ObjLoad("la", addr, new ObjLabel(gMap.get(op2.getName()).getName()), new ObjImm(0)));
            opMap.put(addr.getName(), (ObjVirReg) addr);
        } else {
            addr = parseIntOperand(op2, opMap);
        }
        if (IrValue.from(op1).isGlobal) {
            //la tmp 0(addr)
            ObjVirReg tmp = new ObjVirReg();
            block.addInstr(new ObjLoad("la", tmp, addr, new ObjImm(0)));
            //lw dst 0(tmp)
            ObjOperand dst = parseIntOperand(op1, opMap);
            block.addInstr(new ObjLoad("lw", dst, tmp, new ObjImm(0)));
        } else if (loadInstr.resultType.is("POINTER")) {
            ObjOperand dst = parseIntOperand(op1, opMap);
            block.addInstr(new ObjLoad("ld", dst, addr, new ObjImm(0)));
        } else {
            if (loadInstr.resultType.is("FLOAT")) {
                ObjOperand dst = parseFloatOperand(op1, opMap, block);
                block.addInstr(new ObjLoad("flw", dst, addr, new ObjImm(0)));
            } else {
                ObjOperand dst = parseIntOperand(op1, opMap);
                block.addInstr(new ObjLoad("lw", dst, addr, new ObjImm(0)));
            }
        }
    }

    public void parseMove(ObjBlock block, HashMap<String, ObjReg> opMap, MoveInstr instr) {
        // move is actually add/fadd instr
        Value[] values = instr.getOperands();
        ObjOperand src = parseOperand(values[1], opMap, block);
        ObjOperand dst = parseOperand(values[0], opMap, block);
        ObjMove objMove = new ObjMove(src, dst);
        block.addInstr(objMove);
    }

    public void parseNot(ObjBlock block, HashMap<String, ObjReg> opMap, NotInstr instr) {
        Value[] values = instr.getOperands();
        ObjOperand val = parseOperand(values[1], opMap, block);
        ObjOperand result = parseOperand(values[0], opMap, block);
        ObjNot objNot = new ObjNot(val, result);
        block.addInstr(objNot);
    }

    public void parsePhi(ObjBlock block, HashMap<String, ObjReg> opMap, PhiInstr phiInstr, HashMap<String, ObjBlock> blockMap) {
        ObjOperand dst = parseOperand(phiInstr.result, opMap, block);
        LinkedList<Pair<ObjOperand, ObjBlock>> operands = new LinkedList<>();
        for (int i = 0; i < phiInstr.fromValues.size(); i++) {
            ObjBlock objBlock = blockMap.get(phiInstr.fromBlocks.get(i).getName());
            ObjOperand operand = parseOperandButForPhiBranch(phiInstr.fromValues.get(i), opMap, objBlock);
            operands.addLast(new Pair<>(operand, objBlock));
        }
        block.addInstr(new ObjPhi(dst, new ArrayList<>(operands)));
    }

    public void parseStore(ObjBlock block, HashMap<String, ObjReg> opMap, StoreInstr instr) {
        Value[] values = instr.getOperands();
        ObjOperand pointer;
        if (IrValue.from(values[0]).isGlobal) {
            pointer = new ObjVirReg();
            block.addInstr(new ObjLoad("la", pointer, new ObjLabel(gMap.get(values[0].getName()).getName()), new ObjImm(0)));
            opMap.put(pointer.getName(), (ObjVirReg) pointer);
        } else {
            pointer = parseIntOperand(values[0], opMap);
        }
        if (IrValue.from(values[0]).isGlobal) {
            //TODO:改成标签
            ObjOperand value = parseOperand(values[1], opMap, block);
            //ObjVirReg tmp = new ObjVirReg();
            //block.addInstr(new ObjLoad("la", tmp, pointer, new ObjImm(0)));
            String type = "sw";
            if (value instanceof ObjFVirReg) {
                type = "fsw";
            }
            if (value instanceof ObjImm) {
                ObjVirReg tmp = new ObjVirReg();
                block.addInstr(new ObjMove(value, tmp));
                block.addInstr(new ObjStore(pointer, tmp, new ObjImm(0), type));
            } else {
                block.addInstr(new ObjStore(pointer, value, new ObjImm(0), type));
            }
        } else if (instr.getValueType().is("POINTER")) {
            ObjOperand value = parseOperand(values[1], opMap, block);
            block.addInstr(new ObjStore(pointer, value, new ObjImm(0), "sd"));
        } else {
            ObjOperand value = parseOperand(values[1], opMap, block);
            if (value instanceof ObjImm) {
                ObjReg tmp = new ObjVirReg();
                opMap.put(tmp.getName(), tmp);
                block.addInstr(new ObjMove("li", value, tmp));
                value = tmp;
            }
            if (instr.getValueType().is("INT")) {
                block.addInstr(new ObjStore(pointer, value, new ObjImm(0), "sw"));
            } else {
                block.addInstr(new ObjStore(pointer, value, new ObjImm(0), "fsw"));
            }
        }
    }

    public void parseTerm(ObjBlock block, HashMap<String, ObjReg> opMap, TermInstr instr, ObjFunction objFunction, HashMap<String, ObjBlock> blockMap) {
        if (instr.getOp() == IrInstr.OpCode.RET) {
            parseRet(block, opMap, instr, objFunction);
        } else if (instr.getOp() == IrInstr.OpCode.CALL) {
            parseCall(block, opMap, instr);
        } else if (instr.getOp() == IrInstr.OpCode.BR) {
            parseBr(block, opMap, instr, blockMap);
        }
    }

    public void parseRet(ObjBlock block, HashMap<String, ObjReg> opMap, TermInstr instr, ObjFunction objFunction) {
        Value[] values = instr.getOperands();
        int stackSize = objFunction.getStackSize();
        if (objFunction.getRaSize() > 0) {
            //恢复ra
            spawnLoadStore(LoadStore.LOAD, "ld", ObjPhyReg.RA, ObjPhyReg.SP, new ObjImm(stackSize - 8)).forEach(block::addInstr);
        }
        //销毁栈
        ObjBinary freeStack = new ObjBinary("add", "i32", ObjPhyReg.SP, ObjPhyReg.SP, new ObjImm(stackSize));
        objFunction.addFreeStack(freeStack, block);

        if (values[1] == null) {
            //block.addInstr(new ObjMove(ObjPhyReg.regs.get(0), null));
            block.addInstr(freeStack);
            block.addInstr(new ObjRet());
        } else {
            ObjOperand value = parseOperand(values[1], opMap, block);
            ObjMove move;
            if (values[1].getType().is("INT")) {
                move = new ObjMove(value, ObjPhyReg.regs.get(10));
            } else {
                move = new ObjMove(value, ObjFPhyReg.regs.get(10));
            }
            ObjRet objRet = new ObjRet(value);
            objRet.setObjFunction(objFunction);
            block.addInstr(move);
            block.addInstr(freeStack);  // 这个位置很微妙
            block.addInstr(objRet);
        }
        objFunction.setExits(block);
    }

    public void parseCall(ObjBlock block, HashMap<String, ObjReg> opMap, TermInstr instr) {
        Value[] values = instr.getOperands();
        ObjOperand returnValue = null;
        if (values[0] != null) {
            returnValue = parseOperand(values[0], opMap, block); // return Type
        }
        //ObjOperand value = parseOperand(values[1], opMap, block); // function
        String funcName = values[1].getName();
        ObjFunction function = funcMap.get(funcName);

        LinkedList<Value> args = new LinkedList<>(instr.getArguments());
        int int_num = 0;
        int float_num = 0;
        int stack_size = 0;
        for (Value arg : args) {
            if (arg.getType() instanceof FloatType) {
                ObjOperand operand = parseOperand(arg, opMap, block);
                if (float_num < 8) {
                    ObjMove move = new ObjMove(operand, ObjFPhyReg.regs.get(10 + float_num));
                    block.addInstr(move);
                } else {
                    spawnLoadStore(LoadStore.STORE, "fsw", operand, ObjPhyReg.SP, new ObjImm(stack_size)).forEach(block::addInstr);
                    stack_size += 4;//为啥是4
                }
                float_num++;
            }
        }
        for (Value arg : args) {
            if (!(arg.getType() instanceof FloatType)) {
                ObjOperand operand = parseOperand(arg, opMap, block);
                if (int_num < 8) {
                    ObjMove move = new ObjMove(operand, ObjPhyReg.regs.get(10 + int_num));
                    block.addInstr(move);
                } else {
                    if (arg.getType() instanceof PointerType) {
                        spawnLoadStore(LoadStore.STORE, "sd", operand, ObjPhyReg.SP, new ObjImm(stack_size)).forEach(block::addInstr);
                        stack_size += 8;
                    } else {
                        spawnLoadStore(LoadStore.STORE, "sw", operand, ObjPhyReg.SP, new ObjImm(stack_size)).forEach(block::addInstr);
                        stack_size += 4;
                    }
                }
                int_num++;
            }
        }

        block.addInstr(new ObjCall(funcName));

        if (returnValue != null) {
            if (values[0].getType().is("INT")) {
                ObjMove move = new ObjMove("move", ObjPhyReg.regs.get(10), returnValue);//将a0(mips中的v0)中的值移动到指定的result中
                block.addInstr(move);
            } else {
                ObjMove move = new ObjMove("fmv.s", ObjFPhyReg.regs.get(10), returnValue);
                block.addInstr(move);
            }
        }
//        if (i_size > 0) {
//            block.addInstr(new ObjBinary("add", "i32", ObjPhyReg.SP, ObjPhyReg.SP, new ObjImm(i_size)));
//        }
//        if(f_size>0){
//            block.addInstr(new ObjBinary("add","i32",ObjFPhyReg.SP,ObjPhyReg.SP,new ObjImm(i_size)));
//        }
    }

    public void parseBr(ObjBlock block, HashMap<String, ObjReg> opMap, TermInstr instr, HashMap<String, ObjBlock> blockMap) {
        boolean isCond = instr.checkIfCond();
        if (isCond) {
            Value[] values = instr.getOperands();
            ObjOperand cond = parseOperand(values[2], opMap, block);
            ObjBlock thenDest = blockMap.get(values[1].getName());//所有块都已经被定义过
            ObjBlock elseDest = blockMap.get(values[0].getName());
            if (cond instanceof ObjImm) {
                if (cond.equals(new ObjImm(1))) {
                    block.addInstr(new ObjBr(thenDest));
                } else {
                    block.addInstr(new ObjBr(elseDest));
                }
            } else {
                ObjBr objBr = new ObjBr(cond, thenDest, elseDest);
                block.addInstr(objBr);
            }
            block.addNextBlock(thenDest);
            block.addNextBlock(elseDest);
            thenDest.addPreBlock(block);
            elseDest.addPreBlock(block);
        } else {
            Value[] values = instr.getOperands();
            ObjBlock thenDest;
            if (blockMap.containsKey(values[1].getName())) {
                thenDest = blockMap.get(values[1].getName());
            } else {
                thenDest = new ObjBlock(values[1].getName());
                blockMap.put(values[1].getName(), thenDest);
            }
            ObjBr objBr = new ObjBr(thenDest);
            block.addInstr(objBr);
            block.addNextBlock(thenDest);
            thenDest.addPreBlock(block);
        }
    }

    public void parseTrunc(ObjBlock block, HashMap<String, ObjReg> opMap, TruncInstr instr) {
        Value[] values = instr.getOperands();
        ObjOperand result = parseOperand(values[0], opMap, block);
        ObjOperand value = parseOperand(values[1], opMap, block);
        ObjTrunc objTrunc = new ObjTrunc(result, value);
        block.addInstr(objTrunc);
    }

}
/*
alloca从栈中分配空间
传入的多余的args从栈中取
调用函数前存ra，保存当前的全局变量s0-s8；调用后恢复ra和全局变量
//先扫描一遍，调用需要多少空间(取最大值)、alloca需要多少空间，共size
//函数开头先创建栈帧add sp sp size
//随后获取参数:s0=sp+size,lw 8(s0),lw 16(s0)...
//alloca调用ObjFunction中动态调整的参数,addi vr0 sp x
//call前存储全局变量和ra,sd ra size-8(sp),sd s0
//如何确定要保留哪些变量？

test1函数调用test2函数的栈帧:
----高地址
(test1)
ra
s0
...
s7
....
alloca的变量
...
arg2
arg1
----低地址
(test2)
*/
