package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Fptosi;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Shift;
import mid.IntermediatePresentation.Instruction.Sitofp;
import mid.IntermediatePresentation.Instruction.ZextTo;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.HyperParams;
import utils.Config;

import java.util.ArrayList;
import java.util.HashSet;

public class ConstFold {
    private boolean pinned = false;

    private boolean finalOpt = false;

    public ConstFold() {

    }

    public ConstFold(boolean pinned) {
        this.pinned = pinned;
    }

    public boolean finalOptimize(boolean pinned) {
        finalOpt = true;
        this.pinned = pinned;
        return optimize();
    }

    public boolean optimize() {
        boolean flag = false;
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            boolean hasChanged = true;
            while (hasChanged) {
                hasChanged = false;
                for (BasicBlock bb : f.getBlocks()) {
                    ArrayList<Instruction> instructions = new ArrayList<>(bb.getInstructionList());
                    for (Instruction instruction : instructions) {
                        if (!bb.getInstructionList().contains(instruction)) {
                            continue;
                        }
                        if (instruction instanceof GetElementPtr gep) {
                            if (gep.canGetConstNumber()) {
                                Number n = gep.getStorageVal();
                                for (User user : gep.getUserList()) {
                                    if (user instanceof Load load) {
                                        load.beReplacedBy((new ConstNumber(n)).withType(!load.isFloat()));
                                        load.destroy();
                                        load.getBlock().removeInstruction(load);
                                        hasChanged = true;
                                    }
                                }
                                //还可能作为参数，不能删
                                if (hasChanged) {
                                    continue;
                                }
                            }
                        }

                        if (instruction instanceof ALU alu) {
                            if (alu.isConst()) {
                                alu.beReplacedBy(alu.toConstNumber());
                                alu.getBlock().removeInstruction(alu);
                                alu.destroy();
                                hasChanged = true;
                                continue;
                            }

                            hasChanged |= switch (alu.getAluType()) {
                                case "mul" -> multOptimize(alu);
                                case "add" -> addOptimize(alu);
                                case "sub" -> subOptimize(alu);
                                case "sdiv" -> divOptimize(alu);
                                case "srem" -> remOptimize(alu);
                                default -> false;
                            };
                        } else if (instruction instanceof Cmp cmp) {
                            if (cmp.isConst()) {
                                cmp.beReplacedBy(cmp.toConstNumber());
                                cmp.getBlock().removeInstruction(cmp);
                                cmp.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof Br br && !pinned) {
                            if (br.getCond() != null && br.getCond() instanceof ConstNumber n) {
                                int index = br.getBlock().getInstructionList().indexOf(br);
                                Br newBr;
                                if (n.getVal().intValue() == 0) {
                                    newBr = new Br(br.getIfFalse());
                                } else {
                                    newBr = new Br(br.getIfTrue());
                                }
                                br.beReplacedBy(newBr);
                                br.getBlock().removeInstruction(br);
                                br.destroy();
                                br.getBlock().addInstructionAt(index, newBr);
                                hasChanged = true;
                            }
                        } else if (instruction instanceof Call call) {
                            Number retVal = call.toConst();
                            if (retVal != null) {
                                call.beReplacedBy((new ConstNumber(retVal)).withType(!call.isFloat()));
                                call.getBlock().removeInstruction(call);
                                call.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof ZextTo zextTo) {
                            if (zextTo.getOperandList().get(0) instanceof ConstNumber n) {
                                zextTo.beReplacedBy(n);
                                zextTo.getBlock().removeInstruction(zextTo);
                                zextTo.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof Sitofp sitofp) {
                            if (sitofp.getOperandList().get(0) instanceof ConstNumber n) {
                                sitofp.beReplacedBy(n.withType(false));
                                sitofp.getBlock().removeInstruction(sitofp);
                                sitofp.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof Fptosi fptosi) {
                            if (fptosi.getOperandList().get(0) instanceof ConstNumber n) {
                                fptosi.beReplacedBy(n.withType(true));
                                fptosi.getBlock().removeInstruction(fptosi);
                                fptosi.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof Phi phi) {
                            if (phi.isConst()) {
                                Value v = phi.getOperandList().get(0);
                                if (v instanceof ConstNumber || !pinned) {
                                    phi.beReplacedBy(phi.getOperandList().get(0));
                                    phi.getBlock().removeInstruction(phi);
                                    phi.destroy();
                                    hasChanged = true;
                                }
                            }
                        } else if (instruction instanceof Shift shift) {
                            if (shift.getOperandList().get(0) instanceof ConstNumber n1 &&
                                    shift.getOperandList().get(1) instanceof ConstNumber n2) {
                                int leftOperand = n1.getVal().intValue();
                                int rightOperand = n2.getVal().intValue();
                                int finalRes;
                                if (shift.isLogicalShiftRight()) {
                                    finalRes = leftOperand >>> rightOperand;
                                } else if (shift.isShiftRight()) {
                                    finalRes = leftOperand >> rightOperand;
                                } else {
                                    finalRes = leftOperand << rightOperand;
                                }
                                Value replaceConst = new ConstNumber(finalRes);
                                shift.beReplacedBy(replaceConst);
                                shift.getBlock().removeInstruction(shift);
                                shift.destroy();
                                hasChanged = true;
                            }
                        } else if (instruction instanceof GetElementPtr gep) {
                            hasChanged |= gepOptimize(gep);
                        }
                    }
                    flag |= hasChanged;
                }
            }
        }
        return flag;
    }

    private boolean gepOptimize(GetElementPtr gep) {
        Value addr = gep.getPtr();
        Instruction finalOffset;
        if (addr instanceof GetElementPtr gepAddr) {
            gepOptimize(gepAddr);
            Value originOffset = gepAddr.getElemIndex();
            finalOffset = new ALU(originOffset, "+", gep.getElemIndex(), true);
            gep.getBlock().addInstructionAt(gep.getBlock().getInstructionList().indexOf(gep), finalOffset);
            gep.replaceOperand(gep.getElemIndex(), finalOffset);
            gep.replaceOperand(gepAddr, gepAddr.getPtr());
            return true;
        }
        return false;
    }

    private boolean addOptimize(ALU add) {
        Value lOperand = add.getOperand1();
        Value rOperand = add.getOperand2();
        boolean isInt = !add.isFloat();
        if (lOperand instanceof ConstNumber && rOperand instanceof ConstNumber) {
            return false;
        }

        if (lOperand instanceof ConstNumber || rOperand instanceof ConstNumber) {
            Value operand;
            float imm;
            if (lOperand instanceof ConstNumber) {
                imm = ((ConstNumber) lOperand).getVal().floatValue();
                operand = rOperand;
            } else {
                imm = ((ConstNumber) rOperand).getVal().floatValue();
                operand = lOperand;
            }

            //a+0=0+a=a
            if (imm == 0) {
                add.beReplacedBy(operand);
                add.getBlock().removeInstruction(add);
                add.destroy();
                return true;
            }

            //addi可以和使用它的addi或subi合并，它们的类型一定是相同的
            for (User user : add.getUserList()) {
                if (user instanceof ALU aluUser) {
                    if (aluUser.getAluType().equals("add")) {
                        //add des1,op,imm ; add des2,des1,n -> add des2,op,imm1+n
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt)
                                    , "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt),
                                    "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        }
                    } else if (aluUser.getAluType().equals("sub")) {
                        //add des1,op,imm ; sub des2,des1,n -> add des2,op,imm-n
                        //add des1,op,imm ; sub des2,n,des1 -> sub des2,n-imm,op
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedSub = new ALU(
                                    new ConstNumber(-imm + n.getVal().floatValue()).withType(isInt)
                                    , "-", operand, isInt);
                            replaceInstrWith(aluUser, mergedSub);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(imm - n.getVal().floatValue()).withType(isInt),
                                    "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        }
                    }
                }
            }
        }

        if (lOperand instanceof ALU lalu && lalu.getOperator().equals("-") &&
                lalu.getOperand2().equals(rOperand)) {
            add.beReplacedBy(lalu.getOperand1());
            add.getBlock().removeInstruction(add);
            add.destroy();
            return true;
        }
        if (rOperand instanceof ALU ralu && ralu.getOperator().equals("-") &&
                ralu.getOperand2().equals(lOperand)) {
            add.beReplacedBy(ralu.getOperand1());
            add.getBlock().removeInstruction(add);
            add.destroy();
            return true;
        }

        if (lOperand instanceof ALU lalu && rOperand instanceof ALU ralu) {
            /*
                a+b的折叠，将两者分为正负两侧，若有两侧相同的，则可以消去
             */
            HashSet<Value> valSet = new HashSet<>();
            valSet.add(lalu.getOperand1());
            valSet.add(ralu.getOperand1());
            valSet.add(lalu.getOperand2());
            valSet.add(ralu.getOperand2());
            ArrayList<Value> valList = new ArrayList<>();
            valList.add(lalu.getOperand1());
            valList.add(ralu.getOperand1());
            valList.add(lalu.getOperand2());
            valList.add(ralu.getOperand2());
            if (valSet.size() <= 3) {
                HashSet<Value> lvals = new HashSet<>();
                HashSet<Value> rvals = new HashSet<>();
                ALU combined = null;
                if (lalu.getOperator().equals("+")) {
                    lvals.add(lalu.getOperand1());
                    lvals.add(lalu.getOperand2());
                } else if (lalu.getOperator().equals("-")) {
                    lvals.add(lalu.getOperand1());
                    rvals.add(lalu.getOperand2());
                }
                if (ralu.getOperator().equals("+")) {
                    if (rvals.contains(ralu.getOperand1())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "+", ralu.getOperand2(), isInt);
                        } else {
                            combined = new ALU(ralu.getOperand2(), "-", valList.get(0), isInt);
                        }
                    } else if (rvals.contains(ralu.getOperand2())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "+", ralu.getOperand1(), isInt);
                        } else {
                            combined = new ALU(ralu.getOperand1(), "-", valList.get(0), isInt);
                        }
                    }
                } else if (ralu.getOperator().equals("-")) {
                    if (rvals.contains(ralu.getOperand1())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "-", ralu.getOperand2(), isInt);
                        }
                    } else if (lvals.contains(ralu.getOperand2())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "+", ralu.getOperand1(), isInt);
                        } else {
                            combined = new ALU(ralu.getOperand1(), "-", valList.get(0), isInt);
                        }
                    }
                }
                if (combined != null) {
                    replaceInstrWith(add, combined);
                    return true;
                }
            }
        }
        return false;
    }

    private boolean subOptimize(ALU sub) {
        Value lOperand = sub.getOperand1();
        Value rOperand = sub.getOperand2();
        boolean isInt = !sub.isFloat();
        if (lOperand.equals(rOperand)) {
            sub.beReplacedBy((new ConstNumber(0)).withType(isInt));
            sub.getBlock().removeInstruction(sub);
            sub.destroy();
        }
        if (lOperand instanceof ConstNumber && rOperand instanceof ConstNumber) {
            return false;
        }
        //a-0=a
        if (rOperand instanceof ConstNumber) {
            Value operand;
            float imm;
            imm = ((ConstNumber) rOperand).getVal().floatValue();
            operand = lOperand;

            if (imm == 0) {
                sub.beReplacedBy(operand);
                sub.getBlock().removeInstruction(sub);
                sub.destroy();
                return true;
            }

            //subi可以和使用它的addi或subi合并
            for (User user : sub.getUserList()) {
                if (user instanceof ALU aluUser) {
                    if (aluUser.getAluType().equals("add")) {
                        //sub des1,op,imm ; add des2,des1,n -> add des2,op,-imm1+n
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(-imm + n.getVal().floatValue()).withType(isInt),
                                    "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(-imm + n.getVal().floatValue()).withType(isInt),
                                    "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        }
                    } else if (aluUser.getAluType().equals("sub")) {
                        //sub des1,op,imm ; sub des2,des1,n -> sub des2,op,imm+n
                        //sub des1,op,imm ; sub des2,n,des1 -> sub des2,n+imm,op
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedSub = new ALU(
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt),
                                    "-", operand, isInt);
                            replaceInstrWith(aluUser, mergedSub);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedSub = new ALU(operand, "-",
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt), isInt);
                            replaceInstrWith(aluUser, mergedSub);
                            return true;
                        }
                    }
                }
            }
        }

        if (lOperand instanceof ConstNumber) {
            Value operand;
            float imm;
            imm = ((ConstNumber) lOperand).getVal().floatValue();
            operand = rOperand;

            //subi可以和使用它的addi或subi合并
            for (User user : sub.getUserList()) {
                if (user instanceof ALU aluUser) {
                    if (aluUser.getAluType().equals("add")) {
                        //sub des1,imm,op ; add des2,des1,n -> sub des2,imm+n,op
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt),
                                    "-", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedAdd = new ALU(
                                    new ConstNumber(imm + n.getVal().floatValue()).withType(isInt)
                                    , "-", operand, isInt);
                            replaceInstrWith(aluUser, mergedAdd);
                            return true;
                        }
                    } else if (aluUser.getAluType().equals("sub")) {
                        //sub des1,imm,op ; sub des2,des1,n -> sub des2,imm-n,op
                        //sub des1,imm,op ; sub des2,n,des1 -> add des2,n-imm,op
                        if (aluUser.getOperand1() instanceof ConstNumber n) {
                            ALU mergedSub = new ALU(
                                    new ConstNumber(-imm + n.getVal().floatValue()).withType(isInt)
                                    , "+", operand, isInt);
                            replaceInstrWith(aluUser, mergedSub);
                            return true;
                        } else if (aluUser.getOperand2() instanceof ConstNumber n) {
                            ALU mergedSub = new ALU(
                                    new ConstNumber(imm - n.getVal().floatValue()).withType(isInt),
                                    "-", operand, isInt);
                            replaceInstrWith(aluUser, mergedSub);
                            return true;
                        }
                    }
                }
            }
        }

        if (lOperand instanceof ALU lalu) {
            if (lalu.getOperator().equals("+")) {
                if (lalu.getOperand1().equals(rOperand)) {
                    sub.beReplacedBy(lalu.getOperand2());
                    sub.getBlock().removeInstruction(sub);
                    sub.destroy();
                    return true;
                }
                if (lalu.getOperand2().equals(rOperand)) {
                    sub.beReplacedBy(lalu.getOperand1());
                    sub.getBlock().removeInstruction(sub);
                    sub.destroy();
                    return true;
                }
            } else if (lalu.getOperator().equals("-") && lalu.getOperand1().equals(rOperand)) {
                replaceInstrWith(sub, new ALU((new ConstNumber(0)).withType(isInt),
                        "-", lalu.getOperand2(), isInt));
                return true;
            }
        }

        if (rOperand instanceof ALU ralu) {
            if (ralu.getOperator().equals("+")) {
                if (ralu.getOperand1().equals(lOperand)) {
                    replaceInstrWith(sub, new ALU((new ConstNumber(0)).withType(isInt), "-",
                            ralu.getOperand2(), isInt));
                    return true;
                }
                if (ralu.getOperand2().equals(lOperand)) {
                    replaceInstrWith(sub, new ALU((new ConstNumber(0)).withType(isInt), "-",
                            ralu.getOperand1(), isInt));
                    return true;
                }
            } else if (ralu.getOperator().equals("-") && ralu.getOperand1().equals(lOperand)) {
                sub.beReplacedBy(ralu.getOperand2());
                sub.getBlock().removeInstruction(sub);
                sub.destroy();
                return true;
            }
        }

        if (lOperand instanceof ALU lalu && rOperand instanceof ALU ralu) {
            /*
                a-b的折叠，将两者分为正负两侧，若有两侧相同的，则可以消去
             */
            HashSet<Value> vals = new HashSet<>();
            vals.add(lalu.getOperand1());
            vals.add(ralu.getOperand1());
            vals.add(lalu.getOperand2());
            vals.add(ralu.getOperand2());
            ArrayList<Value> valList = new ArrayList<>();
            valList.add(lalu.getOperand1());
            valList.add(ralu.getOperand1());
            valList.add(lalu.getOperand2());
            valList.add(ralu.getOperand2());
            if (vals.size() == 3) {
                HashSet<Value> lvals = new HashSet<>();
                HashSet<Value> rvals = new HashSet<>();
                ALU combined = null;
                if (lalu.getOperator().equals("+")) {
                    lvals.add(lalu.getOperand1());
                    lvals.add(lalu.getOperand2());
                } else if (lalu.getOperator().equals("-")) {
                    lvals.add(lalu.getOperand1());
                    rvals.add(lalu.getOperand2());
                }
                if (ralu.getOperator().equals("+")) {
                    if (lvals.contains(ralu.getOperand1())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "-", ralu.getOperand2(), isInt);
                        }
                    } else if (lvals.contains(ralu.getOperand2())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "-", ralu.getOperand1(), isInt);
                        }
                    }
                } else if (ralu.getOperator().equals("-")) {
                    if (lvals.contains(ralu.getOperand1())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "+", ralu.getOperand2(), isInt);
                        } else {
                            combined = new ALU(ralu.getOperand2(), "-", valList.get(0), isInt);
                        }
                    } else if (rvals.contains(ralu.getOperand2())) {
                        valList.remove(ralu.getOperand1());
                        valList.remove(ralu.getOperand2());
                        valList.remove(ralu.getOperand2());
                        if (lvals.contains(valList.get(0))) {
                            combined = new ALU(valList.get(0), "-", ralu.getOperand1(), isInt);
                        }
                    }
                }
                if (combined != null) {
                    replaceInstrWith(sub, combined);
                    return true;
                }
            }
        }
        return false;
    }

    public void replaceInstrWith(Instruction origin, Value newValue) {
        int index = origin.getBlock().getInstructionList().indexOf(origin);
        origin.beReplacedBy(newValue);
        origin.getBlock().removeInstruction(origin);
        origin.destroy();
        if (newValue instanceof Instruction newInstr) {
            origin.getBlock().addInstructionAt(index, newInstr);
        }
    }

    private boolean multOptimize(ALU mult) {
        Value lOperand = mult.getOperand1();
        Value rOperand = mult.getOperand2();
        boolean isInt = !mult.isFloat();
        if (!isInt) {
            // TODO: 如果是 a*2.0 这种呢？
            return false;
        }
        if (lOperand instanceof ConstNumber && rOperand instanceof ConstNumber) {
            return false;
        }
        //尝试用shift代替
        if (lOperand instanceof ConstNumber || rOperand instanceof ConstNumber) {
            Value operand;
            int imm;
            if (lOperand instanceof ConstNumber) {
                imm = ((ConstNumber) lOperand).getVal().intValue();
                operand = rOperand;
            } else {
                imm = ((ConstNumber) rOperand).getVal().intValue();
                operand = lOperand;
            }

            boolean negative = false;
            if (imm < 0) {
                negative = true;
                imm = -imm;
            }

            //构建shift指令
            String binString = Integer.toBinaryString(imm);
            //统计1的数目，如果多于2个就不能优化
            ArrayList<Integer> shiftLengths = new ArrayList<>();
            char[] charArray = binString.toCharArray();
            for (int i = 0; i < charArray.length; i++) {
                char c = binString.charAt(i);
                if (c == '1') {
                    shiftLengths.add(charArray.length - i - 1);
                    if (shiftLengths.size() > HyperParams.MULT_SPLIT_NUM) {
                        return false;
                    }
                }
            }

            ArrayList<Instruction> shifts = new ArrayList<>();
            Instruction finalValue = null;
            if (shiftLengths.size() == 0) {
                //是乘以0,就直接给一个0
                finalValue = new ALU(new ConstNumber(0), "-", new ConstNumber(0), true);
                shifts.add(finalValue);
            } else if (shiftLengths.size() == 1 && shiftLengths.get(0) == 0) {
                finalValue = new ALU((new ConstNumber(0)), "+", operand, true);
                shifts.add(finalValue);
            } else if (finalOpt) {
                for (Integer shiftLength : shiftLengths) {
                    Value shift = operand;
                    if (shiftLength != 0) {
                        shift = new Shift(false, operand,
                                (ConstNumber) new ConstNumber(shiftLength).withType(true));
                    }
                    shifts.add((Instruction) shift);
                }

                finalValue = shifts.get(0);
                for (int i = 0; i < shiftLengths.size() - 1; i++) {
                    finalValue = new ALU(shifts.get(i), "+", shifts.get(i + 1), true);
                    shifts.add(finalValue);
                }
            }

            if (finalValue == null) {
                return false;
            }

            //用这些指令替换
            ArrayList<Instruction> instrList = mult.getBlock().getInstructionList();
            if (negative) {
                ALU neg = new ALU(new ConstNumber(0).withType(true),
                        "-", finalValue, true);
                finalValue = neg;
                shifts.add(neg);
            }

            int index = instrList.indexOf(mult);
            mult.beReplacedBy(finalValue);
            mult.getBlock().addInstructionAt(index, finalValue);
            mult.getBlock().removeInstruction(mult);
            mult.destroy();
            shifts.remove(operand);
            for (Instruction instr : shifts) {
                //因为有GCM，所以这里顺序其实无妨
                if (!instr.equals(finalValue)) {
                    mult.getBlock().addInstructionAt(index, instr);
                }
            }
            return true;
        }
        return false;
    }

    private boolean divOptimize(ALU div) {
        Value lOperand = div.getOperand1();
        Value rOperand = div.getOperand2();
        boolean isInt = !(lOperand.isFloat() || rOperand.isFloat());
        if (!isInt) {
            return false;
        }
        if (lOperand instanceof ConstNumber && rOperand instanceof ConstNumber) {
            return false;
        }
        //0/a=0 ; a/1=a
        if (lOperand instanceof ConstNumber || rOperand instanceof ConstNumber) {
            Value operand;
            int imm;
            if (lOperand instanceof ConstNumber) {
                imm = ((ConstNumber) lOperand).getVal().intValue();
                operand = rOperand;
            } else {
                imm = ((ConstNumber) rOperand).getVal().intValue();
                operand = lOperand;
            }

            if (imm == 0 && lOperand instanceof ConstNumber) {
                div.beReplacedBy((new ConstNumber(0)).withType(true));
                div.getBlock().removeInstruction(div);
                div.destroy();
                return true;
            }
            if (imm == 1 && rOperand instanceof ConstNumber) {
                div.beReplacedBy(operand);
                div.getBlock().removeInstruction(div);
                div.destroy();
                return true;
            }
            if (imm == -1 && rOperand instanceof ConstNumber) {
                ALU neg = new ALU((new ConstNumber(0)).withType(true), "-", operand, true);
                int index = div.getBlock().getInstructionList().indexOf(div);
                div.beReplacedBy(neg);
                div.getBlock().removeInstruction(div);
                div.destroy();
                div.getBlock().addInstructionAt(index, neg);
                return true;
            }

            //取反
            boolean negative = imm < 0;
            if (imm < 0) {
                imm = -imm;
            }
            if (rOperand instanceof ConstNumber && finalOpt) {
                // 如果是2的幂次
                if (Integer.bitCount(imm) == 1) {
                    int size = Integer.toString(imm, 2).length();

                    Shift shift1 = new Shift(true, operand,
                            (ConstNumber) new ConstNumber(31).withType(true));
                    Shift shift2 = new Shift(true, shift1,
                            (ConstNumber) new ConstNumber(33 - size).withType(true));
                    shift2.setLogicalShiftRight(true);
                    ALU addu = new ALU(operand, "+", shift2, true);
                    Shift shift = new Shift(true, addu,
                            (ConstNumber) (new ConstNumber(size - 1)).withType(true));

                    Value finalInstr = shift;
                    int index = div.getBlock().getInstructionList().indexOf(div);

                    if (negative) {
                        ALU neg = new ALU(new ConstNumber(0).withType(true),
                                "-", finalInstr, true);
                        div.getBlock().addInstructionAt(index, neg);
                        finalInstr = neg;
                    }

                    div.beReplacedBy(finalInstr);
                    div.getBlock().removeInstruction(div);
                    div.destroy();
                    div.getBlock().addInstructionAt(index, shift);
                    div.getBlock().addInstructionAt(index, addu);
                    div.getBlock().addInstructionAt(index, shift2);
                    div.getBlock().addInstructionAt(index, shift1);
                    return true;
                }
                //乘法+移位优化

                if (!Config.support_mulh) {
                    return false;
                }

                long nc = ((long) 1 << 31) - (((long) 1 << 31) % imm) - 1;
                int l = 32;
                while (((long) 1 << l) <= nc * (imm - ((long) 1 << l) % imm)) {
                    l++;
                }
                long m = ((((long) 1 << l) + (long) imm - ((long) 1 << l) % imm) / (long) imm);
                l = l - 32;

                ArrayList<Instruction> newInstructions = new ArrayList<>();
                //计算xsign
                Shift signShift = new Shift(true, operand,
                        (ConstNumber) (new ConstNumber(31)).withType(true));
                signShift.setLogicalShiftRight(true);
                newInstructions.add(signShift);
                Instruction finalInstr;

                if (m < Integer.MAX_VALUE) {
                    //quotient = (signMulHi(dividend, multiplier) >> shift) - xsign(dividend);
                    ALU mult = new ALU(operand, "*",
                            (new ConstNumber((int) m)).withType(true), true);
                    mult.setResFromHi(true);
                    newInstructions.add(mult);
                    Shift srl = new Shift(true, mult, (ConstNumber) new ConstNumber(l).withType(true));
                    newInstructions.add(srl);
                    ALU fin = new ALU(srl, "+", signShift, true);
                    newInstructions.add(fin);
                    finalInstr = fin;
                } else {
                    //quotient = ((dividend + signMulHi(dividend, (sword) (multiplier - 0x80000000))) >> shift)
                    // - xsign(dividend);
                    long mSub32 = m - (1L << 32);
                    ALU mult = new ALU(operand, "*", new ConstNumber((int) mSub32).withType(true),
                            true);
                    mult.setResFromHi(true);
                    newInstructions.add(mult);

                    ALU add = new ALU(operand, "+", mult, true);
                    newInstructions.add(add);
                    Shift srl = new Shift(true, add, (ConstNumber) new ConstNumber(l).withType(true));
                    newInstructions.add(srl);
                    finalInstr = new ALU(srl, "+", signShift, true);
                    newInstructions.add(finalInstr);
                }
                //补符号位，因为C中负数是向上取整的，但移位不是
                if (negative) {
                    ALU neg = new ALU((new ConstNumber(0)).withType(true),
                            "-", finalInstr, true);
                    newInstructions.add(neg);
                    finalInstr = neg;
                }

                int index = div.getBlock().getInstructionList().indexOf(div);
                div.beReplacedBy(finalInstr);
                div.getBlock().removeInstruction(div);
                div.destroy();
                for (int i = newInstructions.size() - 1; i >= 0; i--) {
                    div.getBlock().addInstructionAt(index, newInstructions.get(i));
                }
                return true;
            }
        }
        return false;
    }

    private boolean remOptimize(ALU rem) {
        Value lOperand = rem.getOperand1();
        Value rOperand = rem.getOperand2();
        if (lOperand.isFloat() || rOperand.isFloat()) {
            return false;
        }
        if (lOperand instanceof ConstNumber && rOperand instanceof ConstNumber) {
            return false;
        }
        //0%a=0 ; a%+-1 = 0
        if (lOperand instanceof ConstNumber || rOperand instanceof ConstNumber) {
            int imm;
            if (lOperand instanceof ConstNumber) {
                imm = ((ConstNumber) lOperand).getVal().intValue();
            } else {
                imm = ((ConstNumber) rOperand).getVal().intValue();
            }

            if (imm == 0 && lOperand instanceof ConstNumber) {
                rem.beReplacedBy((new ConstNumber(0)).withType(true));
                rem.getBlock().removeInstruction(rem);
                rem.destroy();
                return true;
            }
            if ((imm == 1 || imm == -1) && rOperand instanceof ConstNumber) {
                rem.beReplacedBy((new ConstNumber(0)).withType(true));
                rem.getBlock().removeInstruction(rem);
                rem.destroy();
                return true;
            }
        }

        if (rOperand instanceof ConstNumber && finalOpt) {
            //其他模常数的情况，转化为除法，即a%b = a-b*(a/b)，之后的通过再次遍历转为移位
            ALU div = new ALU(rem.getOperand1(), "/", rem.getOperand2(), true);
            ALU mult = new ALU(div, "*", rem.getOperand2(), true);
            ALU sub = new ALU(rem.getOperand1(), "-", mult, true);
            int index = rem.getBlock().getInstructionList().indexOf(rem);
            rem.beReplacedBy(sub);
            rem.getBlock().removeInstruction(rem);
            rem.destroy();
            rem.getBlock().addInstructionAt(index, sub);
            rem.getBlock().addInstructionAt(index, mult);
            rem.getBlock().addInstructionAt(index, div);
            return true;
        }
        return false;
    }
}
