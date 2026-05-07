package midend.optimism;

import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class GVN {
    private Module module;
    private HashMap<String, Instruction> GVNMap;
    private IRBuilder builder = new IRBuilder();

    public GVN(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            BasicBlock entry = function.getBlockList().getFirst();
            GVNMap = new HashMap<>();
            GVNvisit(entry);
        }
    }

    public void GVNvisit(BasicBlock block) {
        AluOptimize(block);
        HashSet<Instruction> inserted = new HashSet<>();
        LinkedList<Instruction> instructions = block.getInstrList();
        LinkedList<Instruction> temp = new LinkedList<>(instructions);
        for (Instruction instruction : temp) {
            if (canBeGVN(instruction)) {
                if (repeated(instruction)) {
                    inserted.add(instruction);
                }
            }
        }

        for (BasicBlock basicBlock : block.getChildBlock()) {
            GVNvisit(basicBlock);
        }

        for (Instruction instruction : inserted) {
            if (instruction instanceof ALUInstr && isSwap(instruction)) {
                GVNMap.remove(getSwapHash((ALUInstr) instruction));
            }
            GVNMap.remove(getHash(instruction));
        }
    }

    public void AluOptimize(BasicBlock block) {
        ArrayList<Instruction> toBeRemoved = new ArrayList<>();
        for (Instruction instruction : block.getInstrList()) {
            if (!(instruction instanceof ALUInstr)) {
                continue;
            }
//            if (instruction instanceof CmpInstr) {
//                continue;
//            }
            ALUInstr aluInstr = (ALUInstr) instruction;
            Value left = aluInstr.getLeft();
            Value right =  aluInstr.getRight();
            int count = 0;
            if (left instanceof Constant) {
                count++;
            }
            if (right instanceof Constant) {
                count++;
            }
            if (count == 2) {
                Value constant = builder.buildNumber((Constant) left, (Constant) right, aluInstr.getOpStr());
                aluInstr.replaceUse(constant);
                toBeRemoved.add(aluInstr);
            } else if (count == 1) {
                Value value = calcuOneConst(aluInstr, left, right, aluInstr.getOpStr());
                if (value != null) {
                    aluInstr.replaceUse(value);
                    toBeRemoved.add(aluInstr);
                }
            } else {
                Value value = calcuZeroConst(left, right, aluInstr.getOpStr(), aluInstr);
                if (value != null) {
                    aluInstr.replaceUse(value);
                    toBeRemoved.add(aluInstr);
                }
            }
        }
        for (Instruction instruction : toBeRemoved) {
            instruction.remove();
        }
    }

    public Value calcuZeroConst(Value left, Value right, String op, Instruction instruction) {
        boolean isFloat = false;
        if (left.getType() == FloatType.f32 || right.getType() == FloatType.f32) {
            isFloat = true;
        }
        switch (op) {
//            case "+":
//                if (left.equals(right)) {
//                    ALUInstr aluInstr = (ALUInstr) instruction;
//                    Value baseValue = ((ALUInstr) instruction).getLeft();
//                    int count = 2;
//                    boolean slyw = true;
//                    while (slyw) {
//                        slyw = false;
//                        for (User user : aluInstr.getUserList()) {
//                            if (!(user instanceof ALUInstr)) {
//                                continue;
//                            }
//                            if (!((ALUInstr) user).getLeft().equals(baseValue) && !((ALUInstr) user).getRight().equals(baseValue)) {
//                                continue;
//                            }
//                            ALUInstr aluInstr1 = (ALUInstr) user;
//                            aluInstr1.setInstrType(InstrType.MUL);
//                            count++;
//                            if (aluInstr1.getLeft().equals(left)) {
//                                aluInstr1.setValue(0, new ConstInt(IntegerType.i32, count + 1));
//                            } else if (aluInstr1.getRight().equals(left)) {
//                                aluInstr1.setValue(1, new ConstInt(IntegerType.i32, count + 1));
//                            }
//                            slyw = true;
//                            aluInstr = (ALUInstr) user;
//                        }
//                    }
//                }
//                break;
            case "-":
                if (left.equals(right)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0.0F);
                    } else {
                        return new ConstInt(IntegerType.i32, 0);
                    }
                }
                break;
            case "/":
                if (left.equals(right)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 1F);
                    } else {
                        return new ConstInt(IntegerType.i32, 1);
                    }
                }
                break;
            case "%":
                if (left.equals(right)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0.0F);
                    } else {
                        return new ConstInt(IntegerType.i32, 0);
                    }
                }
                break;
            case "==", ">=", "<=":
                if (left.equals(right)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 1F);
                    } else {
                        return new ConstFloat(IntegerType.i32, 1);
                    }
                }
                break;
            case "!=", "<", ">":
                if (left.equals(right)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0F);
                    } else {
                        return new ConstFloat(IntegerType.i32, 0);
                    }
                }
                break;
        }
        return null;
    }

    public Value calcuOneConst(Instruction instruction, Value left, Value right, String op) {
        boolean isFloat = false;
        if (left instanceof ConstFloat || right instanceof ConstFloat) {
            isFloat = true;
        }
        switch (op) {
            case "+":
                if (left instanceof ConstInt && ((ConstInt) left).getValue() == 0) {
                    return right;
                } else if (right instanceof ConstInt && ((ConstInt) right).getValue() == 0) {
                    return left;
                } else if (left instanceof ConstFloat && ((ConstFloat) left).getValue() == 0.0F) {
                    return right;
                } else if (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 0.0F) {
                    return left;
                } else if (instruction.getUseList().size() == 1) {
                    if (instruction.getUseList().get(0).getUser() instanceof ALUInstr) {
                        ALUInstr aluInstr = (ALUInstr) instruction.getUseList().get(0).getUser();
                        Value left0 = aluInstr.getLeft();
                        Value right0 = aluInstr.getRight();
                        if (left0 instanceof Constant || right0 instanceof Constant) {
                            if (aluInstr.getOpStr().equals("+")) {
                                Constant const0 = (Constant) ((left instanceof Constant) ? left : right);
                                Constant const1 = (Constant) ((left0 instanceof Constant) ? left0 : right0);
                                Value after;
                                after = builder.buildNumber(const0, const1, "+");
                                aluInstr.modifyValue(const1, after);
                                return (left instanceof Constant) ? right : left;
                            }  else if (aluInstr.getOpStr().equals("-")) {
                                Constant const0 = (Constant) ((left instanceof Constant)? left : right);
                                Value after;
                                if (left0 instanceof Constant) {
                                    after = builder.buildNumber((Constant) left0, const0, "-");
                                    aluInstr.modifyValue(left0, after);
                                } else if (right0 instanceof Constant) {
                                    after = builder.buildNumber((Constant) right0, const0, "-");
                                    aluInstr.modifyValue(right0, after);
                                }
                                return (left instanceof Constant) ? right : left;
                            }
                        }
                    }
                }
                break;
            case "-":
                if ((right instanceof ConstInt && ((ConstInt) right).getValue() == 0) || (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 0.0F)) {
                    return left;
                } else if (instruction.getUseList().size() == 1) {
                    if (instruction.getUseList().get(0).getUser() instanceof ALUInstr) {
                        ALUInstr aluInstr = (ALUInstr) instruction.getUseList().get(0).getUser();
                        Value left0 = aluInstr.getLeft();
                        Value right0 = aluInstr.getRight();
                        if (left0 instanceof Constant || right0 instanceof Constant) {
                            if (aluInstr.getOpStr().equals("+")) {
                                Constant const0 = (Constant) ((left0 instanceof Constant)? left0 : right0);
                                Value after;
                                if (left instanceof Constant) {
                                    after = builder.buildNumber(const0, (Constant) left, "+");
                                    aluInstr.modifyInstrType(InstrType.SUB);
                                    aluInstr.modifyValue(left0, after);
                                    aluInstr.modifyValue(right0, instruction);
                                } else if (right instanceof Constant) {
                                    after = builder.buildNumber(const0, (Constant) right, "-");
                                    aluInstr.modifyValue(const0, after);
                                }
                                return (left instanceof Constant)? right : left;
                            } else if (aluInstr.getOpStr().equals("-")) {
                                Value after;
                                if (left instanceof Constant) {
                                    if (left0 instanceof Constant) { // c1 - reg; c2 - (c1 - reg) -> reg - (c1 - c2)
                                        after = builder.buildNumber((Constant) left, (Constant) left0, "-");
                                        aluInstr.modifyValue(left0, instruction);
                                        aluInstr.modifyValue(right0, after);
                                    } else if (right0 instanceof Constant) { // c1 - reg; (c1 - reg) - c2 -> (c1 - c2) - reg
                                        after = builder.buildNumber((Constant) left, (Constant) right0, "-");
                                        aluInstr.modifyValue(left0, after);
                                        aluInstr.modifyValue(right0, instruction);
                                    }
                                } else if (right instanceof Constant) {
                                    if (left0 instanceof Constant) { // reg - c1; c2 - (reg - c1) -> (c1 + c2) - reg
                                        after = builder.buildNumber((Constant) right, (Constant) left0, "+");
                                        aluInstr.modifyValue(left0, after);
                                    } else if (right0 instanceof Constant) { // reg - c1; (reg - c1) - c2 -> reg - (c2 - c1)
                                        after = builder.buildNumber((Constant) right0, (Constant) right, "-");
                                        aluInstr.modifyValue(left0, instruction);
                                        aluInstr.modifyValue(right0, after);
                                    }
                                }
                                return (left instanceof Constant)? right : left;
                            }
                        }
                    }
                }
                break;
            case "*":
                //有一个是0
                if ((left instanceof ConstInt && ((ConstInt) left).getValue() == 0) || (right instanceof ConstInt && ((ConstInt) right).getValue() == 0) ||
                        (left instanceof ConstFloat && ((ConstFloat) left).getValue() == 0.0F) || (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 0.0F)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0.0F);
                    } else {
                        return new ConstInt(IntegerType.i32, 0);
                    }
                } else if ((left instanceof ConstInt && ((ConstInt) left).getValue() == 1) || (left instanceof ConstFloat && ((ConstFloat) left).getValue() == 1F)) {
                    return right;
                } else if ((right instanceof ConstInt && ((ConstInt) right).getValue() == 1) || (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 1F)) {
                    return left;
                } else if (instruction.getUseList().size() == 1) {
                    if (instruction.getUseList().get(0).getUser() instanceof ALUInstr) {
                        ALUInstr aluInstr = (ALUInstr) instruction.getUseList().get(0).getUser();
                        Value left0 = aluInstr.getLeft();
                        Value right0 = aluInstr.getRight();
                        if (left0 instanceof Constant || right0 instanceof Constant) {
                            if (aluInstr.getOpStr().equals("*")) {
                                Constant const0 = (Constant) ((left instanceof Constant)? left : right);
                                Constant const1 = (Constant) ((left0 instanceof Constant)? left0 : right0);
                                Value after;
                                after = builder.buildNumber(const0, const1, "*");
                                aluInstr.modifyValue(const1, after);
                                return (left instanceof Constant)? right : left;
                            }
                        }
                    }
                }
                break;
            case "/":
                if ((left instanceof ConstInt && ((ConstInt) left).getValue() == 0) || (left instanceof ConstFloat && ((ConstFloat) left).getValue() == 0.0F)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0.0F);
                    } else {
                        return new ConstInt(IntegerType.i32, 0);
                    }
                } else if ((right instanceof ConstInt && ((ConstInt) right).getValue() == 1) || (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 1F)) {
                    return left;
                } else if (instruction.getUseList().size() == 1) {
                    if (instruction.getUseList().get(0).getUser() instanceof ALUInstr) {
                        ALUInstr aluInstr = (ALUInstr) instruction.getUseList().get(0).getUser();
                        Value left0 = aluInstr.getLeft();
                        Value right0 = aluInstr.getRight();
                        Value after;
                        if (left0 instanceof Constant || right0 instanceof Constant) {
                            Constant const0 = (Constant) ((left instanceof Constant)? left : right);
                            Constant const1 = (Constant) ((left0 instanceof Constant)? left0 : right0);
                            if (aluInstr.getOpStr().equals("/")) {
                                after = builder.buildNumber(const0, const1, "*");
                                aluInstr.modifyValue(const1, after);
                                return (left instanceof Constant)? right : left;
                            }
                        }
                    }
                }
                break;
            case "%":
                if ((right instanceof ConstInt && ((ConstInt) right).getValue() == 1) || (right instanceof ConstFloat && ((ConstFloat) right).getValue() == 1F)) {
                    if (isFloat) {
                        return new ConstFloat(FloatType.f32, 0.0F);
                    } else {
                        return new ConstInt(IntegerType.i32, 0);
                    }
                }
                break;
            //case "<", "<=", "==", ">", ">=", "!=":
        }
        return null;
    }

    public boolean repeated(Instruction instruction) {
        if (instruction instanceof CallInstr || instruction instanceof GetElementPtrInstr || instruction instanceof ConversionInstr) {
            String hash = getHash(instruction);
            if (GVNMap.containsKey(hash)) {
                instruction.replaceUse(GVNMap.get(hash));
                instruction.remove();
                return false;
            }
            GVNMap.put(hash, instruction);
            return true;
        } else if (instruction instanceof ALUInstr) {
            String hash = getHash(instruction);
            if (GVNMap.containsKey(hash)) {
                instruction.replaceUse(GVNMap.get(hash));
                instruction.remove();
                return false;
            }
            if (isSwap(instruction)) {
                String swapHash = getSwapHash((ALUInstr) instruction);
                if (GVNMap.containsKey(swapHash)) {
                    instruction.replaceUse(GVNMap.get(hash));
                    instruction.remove();
                    return false;
                }
                GVNMap.put(swapHash, instruction);
            }
            GVNMap.put(hash, instruction);
            return true;
        }
        throw new RuntimeException("wrong instruction type");
    }

    public String getSwapHash(ALUInstr instr) {
        String op = instr.getInstrType().toString();
        if (instr instanceof CmpInstr) {
            op = ((CmpInstr) instr).getOp().toString();
        }
        return instr.getRight().getName() + op + instr.getLeft().getName();
    }

    public String getHash(Instruction instruction) {
        if (instruction instanceof GetElementPtrInstr) {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.append(((GetElementPtrInstr) instruction).getTarget().getName());
            ArrayList<Value> values = instruction.getValueList();
            for (int count = 1; count < values.size(); count++) {
                stringBuilder.append("[").append(values.get(count).getName()).append("]");
            }
            return stringBuilder.toString();
        } else if (instruction instanceof CallInstr) {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.append(((CallInstr) instruction).getFunction().getName()).append("(");
            ArrayList<Value> params = ((CallInstr) instruction).getValues();
            for (int count = 0; count < params.size(); count++) {
                stringBuilder.append(params.get(count).getName());
                if (count != params.size() - 1) {
                    stringBuilder.append(", ");
                }
            }
            stringBuilder.append(")");
            return stringBuilder.toString();
        } else if (instruction instanceof ConversionInstr) {
            return ((ConversionInstr) instruction).getInstr();
        } else if (instruction instanceof ALUInstr) {
            String op = instruction.getInstrType().toString();
            if (instruction instanceof CmpInstr) {
                op = ((CmpInstr) instruction).getOp().toString();
            }
            return ((ALUInstr) instruction).getLeft().getName() + op + ((ALUInstr) instruction).getRight().getName();
        }
        throw new RuntimeException("wrong Instruction type");
    }

    public boolean canBeGVN(Instruction instruction) {
        if (instruction instanceof ALUInstr || instruction instanceof ConversionInstr || instruction instanceof GetElementPtrInstr) {
            return true;
        }
        if (instruction instanceof CallInstr) {
            Function function = ((CallInstr) instruction).getFunction();
            return !function.isLib() && function.canBeGVN();
        }
        return false;
    }

    public boolean isSwap(Instruction instruction) {
        InstrType instrType = instruction.getInstrType();
        if (instruction instanceof CmpInstr) {
            OP op = ((CmpInstr) instruction).getOp();
            return op == OP.EQ || op == OP.NE || op == OP.OEQ || op == OP.ONE;
        }
        return instrType == InstrType.ADD || instrType == InstrType.FADD ||
               // instrType == Instruction.InstrType.SUB || instrType == Instruction.InstrType.FSUB ||
                instrType == InstrType.MUL || instrType == InstrType.FMUL;

    }
}
