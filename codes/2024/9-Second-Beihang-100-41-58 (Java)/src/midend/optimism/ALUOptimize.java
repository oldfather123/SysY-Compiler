package midend.optimism;

import frontend.AST.Assign;
import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.Module;
import midend.instr.ALUInstr;
import midend.instr.InstrType;
import midend.instr.Instruction;

import java.util.ArrayList;
import java.util.Arrays;

public class ALUOptimize {
    private Module module;
    private boolean simplifyMod;

    public ALUOptimize(Module module, boolean simplifyMod) {
        this.module = module;
        this.simplifyMod = simplifyMod;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            if (simplifyMod) {
                removeMod(function);
            }
            simplifyDiv(function);
            simplifyMul(function);
        }
    }

    public static ArrayList<Instruction> simplifyMul(ALUInstr instruction, BasicBlock block) {
        ArrayList<Instruction> simplify = new ArrayList<>();
        Value left = ((ALUInstr) instruction).getLeft();
        Value right = ((ALUInstr) instruction).getRight();
        if (left instanceof ConstInt && !(right instanceof ConstInt)) {
            //-1
            if (((ConstInt) left).getValue() == -1) {
                ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), right), InstrType.SUB, block);
                int index = block.getInstrList().indexOf(instruction);
                simplify.add(newAlu);
                //block.getInstrList().add(index, newAlu);
                //instruction.replaceUse(newAlu);
                //instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) left).getValue())) { //2的整数次幂
                ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue()))), InstrType.SHL, block);
                int index = block.getInstrList().indexOf(instruction);
                simplify.add(newAlu);
//                block.getInstrList().add(index, newAlu);
//                instruction.replaceUse(newAlu);
//                instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) left).getValue() + 1)) { //2的整数次幂-1
                ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue() + 1))), InstrType.SHL, block);
                ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(shl, right), InstrType.SUB, block);
                simplify.add(shl);
                simplify.add(sub);
//                int index = block.getInstrList().indexOf(instruction);
//                block.getInstrList().add(index, sub);
//                block.getInstrList().add(index, shl);
//                instruction.replaceUse(sub);
//                instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) left).getValue() - 1)) { //2的整数次幂+1
                ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue() - 1))), InstrType.SHL, block);
                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl, right), InstrType.ADD, block);
                simplify.add(shl);
                simplify.add(add);
//                int index = block.getInstrList().indexOf(instruction);
//                block.getInstrList().add(index, add);
//                block.getInstrList().add(index, shl);
//                instruction.replaceUse(add);
//                instruction.remove();
            } else if (((ConstInt) left).getValue() > 0) {
                ArrayList<Integer> numbers;
                if ((numbers = countOnesInBinary(((ConstInt) left).getValue())).size() == 2) {
                    ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, numbers.get(0))), InstrType.SHL, block);
                    ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, numbers.get(1))), InstrType.SHL, block);
                    ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl1, shl2), InstrType.ADD, block);
                    simplify.add(shl1);
                    simplify.add(shl2);
                    simplify.add(add);
//                    int index = block.getInstrList().indexOf(instruction);
//                    block.getInstrList().add(index, add);
//                    block.getInstrList().add(index, shl2);
//                    block.getInstrList().add(index, shl1);
//                    instruction.replaceUse(add);
//                    instruction.remove();
                }
            }
        } else if (right instanceof ConstInt && !(left instanceof ConstInt)) {
            //与上面同理
            if (((ConstInt) right).getValue() == -1) {
                ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), left), InstrType.SUB, block);
                int index = block.getInstrList().indexOf(instruction);
                simplify.add(newAlu);
//                block.getInstrList().add(index, newAlu);
//                instruction.replaceUse(newAlu);
//                instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) right).getValue())) {
                ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue()))), InstrType.SHL, block);
                simplify.add(newAlu);
                int index = block.getInstrList().indexOf(instruction);
//                block.getInstrList().add(index, newAlu);
//                instruction.replaceUse(newAlu);
//                instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) right).getValue() + 1)) {
                ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue() + 1))), InstrType.SHL, block);
                ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(shl, left), InstrType.SUB, block);
                simplify.add(shl);
                simplify.add(sub);
//                int index = block.getInstrList().indexOf(instruction);
//                block.getInstrList().add(index, sub);
//                block.getInstrList().add(index, shl);
//                instruction.replaceUse(sub);
//                instruction.remove();
            } else if (isPowerOfTwo(((ConstInt) right).getValue() - 1)) {
                ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue() - 1))), InstrType.SHL, block);
                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl, left), InstrType.ADD, block);
                simplify.add(shl);
                simplify.add(add);
//                int index = block.getInstrList().indexOf(instruction);
//                block.getInstrList().add(index, add);
//                block.getInstrList().add(index, shl);
//                instruction.replaceUse(add);
//                instruction.remove();
            } else if (((ConstInt) right).getValue() > 0) {
                ArrayList<Integer> numbers;
                if ((numbers = countOnesInBinary(((ConstInt) right).getValue())).size() == 2) {
                    ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, numbers.get(0))), InstrType.SHL, block);
                    ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, numbers.get(1))), InstrType.SHL, block);
                    ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl1, shl2), InstrType.ADD, block);
                    simplify.add(shl1);
                    simplify.add(shl2);
                    simplify.add(add);
//                    int index = block.getInstrList().indexOf(instruction);
//                    block.getInstrList().add(index, add);
//                    block.getInstrList().add(index, shl2);
//                    block.getInstrList().add(index, shl1);
//                    instruction.replaceUse(add);
//                    instruction.remove();
                }
            }
        }
        if (simplify.size() == 0) {
            simplify.add(instruction);
        }
        return simplify;
    }

    public static ArrayList<Instruction> simplifyDiv(ALUInstr instruction, BasicBlock block) {
        Value left = instruction.getLeft();
        Value right = instruction.getRight();
        ArrayList<Instruction> instructions = new ArrayList<>();
        if (instruction.getInstrType() == InstrType.SDIV && right instanceof ConstInt) {
            int num = Math.abs(((ConstInt) right).getValue());
            long largeNum = ((long) 1 << 31) - (((long) 1 << 31) % num) - 1;
            long p = 32;
            while ((1L << p) <= largeNum * (num - (1L << p) % num)) {
                p++;
            }

            long m = (((1L << p) + (long) num - ((long) 1 << p) % num) / (long) num);
            int n = (int) ((m << 32) >>> 32);
            int shift = (int) (p - 32);

            ALUInstr mul = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, n)), InstrType.MUL, block);
            mul.setIs64();
            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(mul, new ConstInt(IntegerType.i32, 32)), InstrType.ASHR, block);
            ALUInstr add = null;
            if (m >= 0x80000000L) {
                add = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, left), InstrType.ADD, block);
            }
            ALUInstr ashr2 = new ALUInstr(IntegerType.i32, Arrays.asList(((add == null)? ashr: add), new ConstInt(IntegerType.i32, shift)), InstrType.ASHR, block);
            ALUInstr ashr3 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr2, new ConstInt(IntegerType.i32, 31)), InstrType.ASHR, block);
            ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(ashr2, ashr3), InstrType.SUB, block);
            ALUInstr sub2 = null;
            if (((ConstInt) right).getValue() < 0) {
                sub2 = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), sub), InstrType.SUB, block);
            }
            ALUInstr result = (sub2 == null) ? sub : sub2;
            int index = block.getInstrList().indexOf(instruction);
            instructions.add(mul);
            instructions.add(ashr);
            if (add != null) {
                instructions.add(add);
            }
            instructions.add(ashr2);
            instructions.add(ashr3);
            instructions.add(sub);
            if (sub2 != null) {
                instructions.add(sub2);
//                block.getInstrList().add(index, sub2);
            }
//            block.getInstrList().add(index, sub);
//            block.getInstrList().add(index, ashr3);
//            block.getInstrList().add(index, ashr2);
//            block.getInstrList().add(index, ashr);
//            block.getInstrList().add(index, mul);
//            instruction.replaceUse(result);
//            instruction.remove();
        }
        return instructions;
    }

    public static void simplifyMul(Function function) {
        for (BasicBlock block : function.getBlockList()) {
            for (int count = 0; count < block.getInstrList().size(); count++) {
                Instruction instruction = block.getInstrList().get(count);
                if (instruction instanceof ALUInstr && instruction.getInstrType() == InstrType.MUL) {
                    Value left = ((ALUInstr) instruction).getLeft();
                    Value right = ((ALUInstr) instruction).getRight();
                    if (left instanceof ConstInt && !(right instanceof ConstInt)) {
                        //-1
                        if (((ConstInt) left).getValue() == -1) {
                            ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), right), InstrType.SUB, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, newAlu);
                            instruction.replaceUse(newAlu);
                            instruction.remove();
                        } else if (isPowerOfTwo(((ConstInt) left).getValue())) { //2的整数次幂
                            ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue()))), InstrType.SHL, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, newAlu);
                            instruction.replaceUse(newAlu);
                            instruction.remove();
                        } else if (isPowerOfTwo(((ConstInt) left).getValue() + 1)) { //2的整数次幂-1
                            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue() + 1))), InstrType.SHL, block);
                            ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(shl, right), InstrType.SUB, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, sub);
                            block.getInstrList().add(index, shl);
                            instruction.replaceUse(sub);
                            instruction.remove();
                            continue;
                        } else if (isPowerOfTwo(((ConstInt) left).getValue() - 1)) { //2的整数次幂+1
                            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) left).getValue() - 1))), InstrType.SHL, block);
                            ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl, right), InstrType.ADD, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, add);
                            block.getInstrList().add(index, shl);
                            instruction.replaceUse(add);
                            instruction.remove();
                            continue;
                        } else if (((ConstInt) left).getValue() > 0) {
                            ArrayList<Integer> numbers;
                            if ((numbers = countOnesInBinary(((ConstInt) left).getValue())).size() == 2) {
                                ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, numbers.get(0))), InstrType.SHL, block);
                                ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(right, new ConstInt(IntegerType.i32, numbers.get(1))), InstrType.SHL, block);
                                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl1, shl2), InstrType.ADD, block);
                                int index = block.getInstrList().indexOf(instruction);
                                block.getInstrList().add(index, add);
                                block.getInstrList().add(index, shl2);
                                block.getInstrList().add(index, shl1);
                                instruction.replaceUse(add);
                                instruction.remove();
                            }
                        }
                    } else if (right instanceof ConstInt && !(left instanceof ConstInt)) {
                        //与上面同理
                        if (((ConstInt) right).getValue() == -1) {
                            ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), left), InstrType.SUB, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, newAlu);
                            instruction.replaceUse(newAlu);
                            instruction.remove();
                        } else if (isPowerOfTwo(((ConstInt) right).getValue())) {
                            ALUInstr newAlu = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue()))), InstrType.SHL, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, newAlu);
                            instruction.replaceUse(newAlu);
                            instruction.remove();
                        } else if (isPowerOfTwo(((ConstInt) right).getValue() + 1)) {
                            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue() + 1))), InstrType.SHL, block);
                            ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(shl, left), InstrType.SUB, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, sub);
                            block.getInstrList().add(index, shl);
                            instruction.replaceUse(sub);
                            instruction.remove();
                            continue;
                        } else if (isPowerOfTwo(((ConstInt) right).getValue() - 1)) {
                            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue() - 1))), InstrType.SHL, block);
                            ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl, left), InstrType.ADD, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, add);
                            block.getInstrList().add(index, shl);
                            instruction.replaceUse(add);
                            instruction.remove();
                            continue;
                        } else if (((ConstInt) right).getValue() > 0) {
                            ArrayList<Integer> numbers;
                            if ((numbers = countOnesInBinary(((ConstInt) right).getValue())).size() == 2) {
                                ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, numbers.get(0))), InstrType.SHL, block);
                                ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, numbers.get(1))), InstrType.SHL, block);
                                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(shl1, shl2), InstrType.ADD, block);
                                int index = block.getInstrList().indexOf(instruction);
                                block.getInstrList().add(index, add);
                                block.getInstrList().add(index, shl2);
                                block.getInstrList().add(index, shl1);
                                instruction.replaceUse(add);
                                instruction.remove();
                            }
                        }
                    }
                }
            }
        }
    }

    public static void simplifyDiv(Function function) {
        for (BasicBlock block : function.getBlockList()) {
            for (int count = 0; count < block.getInstrList().size(); count++) {
                Instruction instruction = block.getInstrList().get(count);
                if (instruction instanceof ALUInstr && instruction.getInstrType() == InstrType.SDIV) {
                    Value left = ((ALUInstr) instruction).getLeft();
                    Value right = ((ALUInstr) instruction).getRight();
                    if (right instanceof ConstInt) {
                        if (((ConstInt) right).getValue() == -1) {
                            ALUInstr sub = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), left), InstrType.SUB, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, sub);
                            instruction.replaceUse(sub);
                            instruction.remove();
                        } else if (isPowerOfTwo(((ConstInt) right).getValue())) {
                            ALUInstr lshl = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, powerOfTwo(((ConstInt) right).getValue()))), InstrType.ASHR, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, lshl);
                            instruction.replaceUse(lshl);
                            instruction.remove();
                        } else if ((Math.abs(((ConstInt) right).getValue()) & Math.abs((((ConstInt) right).getValue() - 1))) == 0) { //绝对值
                            int number = countOnesInBinary(((ConstInt) right).getValue()).get(0);
                            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(left, new ConstInt(IntegerType.i32, 31)), InstrType.ASHR, block);
                            ALUInstr lshr = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, new ConstInt(IntegerType.i32, 32 - number)), InstrType.LSHR, block);
                            ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(lshr, left), InstrType.ADD, block);
                            ALUInstr ashr2 = new ALUInstr(IntegerType.i32, Arrays.asList(add, new ConstInt(IntegerType.i32, number)), InstrType.ASHR, block);
                            int index = block.getInstrList().indexOf(instruction);
                            block.getInstrList().add(index, ashr2);
                            block.getInstrList().add(index, add);
                            block.getInstrList().add(index, lshr);
                            block.getInstrList().add(index, ashr);
                            instruction.replaceUse(ashr2);
                            instruction.remove();
                        } //else { //一般立即数
//                            int num = Math.abs(((ConstInt) right).getValue());
//                            long largeNum = ((long) 1 << 31) - (((long) 1 << 31) % num) - 1;
//                            long p = 32;
//                            while ((1L << p) <= largeNum * (num - (1L << p) % num)) {
//                                p++;
//                            }
//
//                            long m = (((1L << p) + (long) num - ((long) 1 << p) % num) / (long) num);
//                            int n = (int) ((m << 32) >>> 32);
//                            int shift = (int) (p - 32);
//                            ALUInstr mul = new ALUInstr(IntegerType.i32, left, new ConstInt(IntegerType.i32, n), InstrType.MUL, block);
//                            mul.setIs64();
//                            ALUInstr ashr = new ALUInstr(IntegerType.i32, mul, new ConstInt(IntegerType.i32, 32), InstrType.ASHR, block);
//                            ALUInstr ashr2 = new ALUInstr(IntegerType.i32, ashr, new ConstInt(IntegerType.i32, shift), InstrType.ASHR, block);
//                            ALUInstr ashr3 = new ALUInstr(IntegerType.i32, ashr2, new ConstInt(IntegerType.i32, 31), InstrType.ASHR, block);
//                            ALUInstr sub = new ALUInstr(IntegerType.i32, ashr2, ashr3, InstrType.SUB, block);
//                            ALUInstr sub2 = null;
//                            if (((ConstInt) right).getValue() < 0) {
//                                sub2 = new ALUInstr(IntegerType.i32, new ConstInt(IntegerType.i32, 0), sub, InstrType.SUB, block);
//                            }
//                            ALUInstr result = (sub2 == null) ? sub : sub2;
//                            int index = block.getInstrList().indexOf(instruction);
//                            if (sub2 != null) {
//                                block.getInstrList().add(index, sub2);
//                            }
//                            block.getInstrList().add(index, sub);
//                            block.getInstrList().add(index, ashr3);
//                            block.getInstrList().add(index, ashr2);
//                            block.getInstrList().add(index, ashr);
//                            block.getInstrList().add(index, mul);
//                            instruction.replaceUse(result);
//                            instruction.remove();
//                        }
                    }
                }
            }
        }
    }

    public static void removeMod(Function function) {
        //x mod m = x - (x / m) * m
        for (BasicBlock block : function.getBlockList()) {
            for (int count = 0; count < block.getInstrList().size(); count++) {
                Instruction instruction = block.getInstrList().get(count);
                if (instruction instanceof ALUInstr) {
                    if (((ALUInstr) instruction).getOpStr().equals("%")) {
                        Value left = ((ALUInstr) instruction).getLeft();
                        Value right = ((ALUInstr) instruction).getRight();
                        ALUInstr div = new ALUInstr(instruction.getType(), Arrays.asList(left, right), instruction.getType() == FloatType.f32 ? InstrType.FDIV : InstrType.SDIV, block);
                        ALUInstr mul = new ALUInstr(instruction.getType(), Arrays.asList(div, right), instruction.getType() == FloatType.f32 ? InstrType.FMUL : InstrType.MUL, block);
                        ALUInstr sub = new ALUInstr(instruction.getType(), Arrays.asList(left, mul), instruction.getType() == FloatType.f32 ? InstrType.FSUB : InstrType.SUB, block);
                        block.getInstrList().add(count, sub);
                        block.getInstrList().add(count, mul);
                        block.getInstrList().add(count, div);
                        instruction.replaceUse(sub);
                        instruction.remove();
                    }
                }
            }
        }
    }

    public static boolean isPowerOfTwo(int number) {
        if (number < 1) {
            return false;
        }
        return (number & (number - 1)) == 0;
    }

    public static int powerOfTwo(int number) {
        if (number < 1) {
            throw new IllegalArgumentException("Number must be greater than 0");
        }

        int count = 0;
        while (number > 1) {
            number >>>= 1; // 使用无符号右移，确保不会引入符号位
            count++;
        }
        return count;
    }

    public static ArrayList<Integer> countOnesInBinary(int number) {
        ArrayList<Integer> positions = new ArrayList<>();
        int position = 0;

        while (number != 0) {
            if ((number & 1) == 1) {
                positions.add(position);
            }
            position++;
            number = number >>> 1; // 无符号右移，检查下一位
        }
        return positions;
    }
}
