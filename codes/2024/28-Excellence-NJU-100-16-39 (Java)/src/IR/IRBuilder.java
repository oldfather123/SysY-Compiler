package IR;


import IR.IRInstruction.*;
import IR.IRType.IRArrayType;
import IR.IRType.IRFunctionType;
import IR.IRType.IRPointerType;
import IR.IRType.IRType;
import IR.IRValueRef.*;

import java.util.ArrayList;
import java.util.List;

import static IR.IRType.IRFloatType.IRFloatType;
import static IR.IRType.IRInt1Type.IRInt1Type;
import static IR.IRType.IRInt32Type.IRInt32Type;
import static IR.IRType.IRVoidType.IRVoidType;

public class IRBuilder {
    /*
     * 该类是IRBuilder的主类，用于提供访问抽象语法树之后构建IR的接口
     */
    private IRBaseBlockRef currentBaseBlock = new IRBaseBlockRef("root");
    private static final IRType int32Type = IRInt32Type();
    private static final IRType floatType = IRFloatType();
    private static final IRType int1Type = IRInt1Type();
    private static final IRType voidType = IRVoidType();

    public IRBuilder() {
    }

    //提供给前端的接口方法
    public static void IRPositionBuilderAtEnd(IRBuilder builder, IRBaseBlockRef block) {
        builder.currentBaseBlock.addSuccBaseBlock(block);//添加控制流,将当前基本块的后继基本块设置为block
        block.addPredBaseBlock(builder.currentBaseBlock);//添加控制流,将block的前驱基本块设置为当前基本块
        builder.currentBaseBlock = block;
    }

    public static IRValueRef IRBuildRet(IRBuilder builder, IRValueRef valueRef) {
        builder.currentBaseBlock.appendInstr(new ReturnInstruction(wrapWithList(valueRef), builder.currentBaseBlock));
        builder.currentBaseBlock.getFunctionBlockRef().addRetBlock(builder.currentBaseBlock);
        if (valueRef == null) {
            builder.appendln("ret " + "void");
        } else {
            builder.appendln("ret " + valueRef.getType().getText() + " " + valueRef.getText());
        }
        return null;
    }

    public static IRValueRef IRBuildAlloca(IRBuilder builder, IRType type, String name) {
        //构建一个alloca指令，过程是将一个alloca指令加入到当前基本块的指令列表中，返回一个变量的地址
        IRValueRef resRegister;
        IRPointerType pointerType = new IRPointerType(type);
        resRegister = new IRVirtualRegRef(name, pointerType);
        builder.appendInstr(new AllocateInstruction(wrapWithList(resRegister), builder.currentBaseBlock));
        builder.appendln(resRegister.getText() + " = alloca " + type.getText(), 4);
        return resRegister;
    }

    public static IRValueRef IRBuildLoad(IRBuilder builder, IRValueRef pointer, String varName) {
        IRType baseType = ((IRPointerType) pointer.getType()).getBaseType();
        IRValueRef resRegister = new IRVirtualRegRef(varName, baseType);
        //构建一个load指令，过程是将一个load指令加入到当前基本块的指令列表中，返回一个变量的值
        builder.appendln(resRegister.getText() + " = load " + baseType.getText() + ", "
                + pointer.getType().getText() + " " + pointer.getText(), 4);
        List<IRValueRef> operands = new ArrayList<>();
        operands.add(resRegister);
        operands.add(pointer);
        builder.appendInstr(new LoadInstruction(operands, builder.currentBaseBlock));
        return resRegister;
    }
    public static IRValueRef IRBuildGEP(IRBuilder builder, IRValueRef valueRef, List<IRValueRef> index, int indexSize, String varName) {
        IRType baseType;
        if(valueRef.getType() instanceof IRArrayType){
            baseType = ((IRArrayType) valueRef.getType()).getBaseType();
        }else {
            baseType = ((IRPointerType) valueRef.getType()).getBaseType();
        }
        IRType resType;
        if (baseType instanceof IRArrayType && indexSize != 1) {
            resType = ((IRArrayType) baseType).getBaseType();
        } else {
            resType = baseType;
        }
        resType = new IRPointerType(resType);

        IRValueRef resRegister = new IRVirtualRegRef(varName, resType);

        StringBuilder indexStrBuilder = new StringBuilder();
        List<IRValueRef> operands = new ArrayList<>();
        operands.add(resRegister);
        operands.add(valueRef);

        for (IRValueRef i : index) {
            operands.add(i);
            indexStrBuilder.append(", ").append(int32Type.getText())
                    .append(" ").append(i.getText());
        }
        builder.appendInstr(new GetElementPointerInstruction(operands, builder.currentBaseBlock));
        builder.appendln(resRegister.getText() + " = getelementptr " + baseType.getText() + ", "
                + valueRef.getType().getText() + " " + valueRef.getText() + indexStrBuilder);
        return resRegister;
    }

    public static IRValueRef IRBuildStore(IRBuilder builder, IRValueRef valueRef, IRValueRef pointer) {
        IRValueRef resRegister;
        IRPointerType pointerType;
        if (valueRef.getType() == int32Type && ((IRPointerType) pointer.getType()).getBaseType() == floatType) {
            //int到float的隐式转换
            valueRef = typeTransfer(builder, valueRef, IRConst.IntToFloat);
            pointerType = new IRPointerType(valueRef.getType());
        } else if (valueRef.getType() == floatType && ((IRPointerType) pointer.getType()).getBaseType() == int32Type) {
            // valueRef = typeTrans(builder, valueRef, FpToSi);
            pointerType = new IRPointerType(valueRef.getType());
        } else {
            pointerType = new IRPointerType(valueRef.getType());
        }
        resRegister = new IRVirtualRegRef("temp", pointerType);
        //构建一个store指令，过程是将一个store指令加入到当前基本块的指令列表中，包括了存储的值和存储的地址
        builder.appendInstr(new StoreInstruction(wrapWithList(resRegister, valueRef, pointer), builder.currentBaseBlock));
        builder.appendln("store " + valueRef.getType().getText() + " " + valueRef.getText() +
                ", " + pointer.getType().getText() + " " + pointer.getText(), 4);
        return resRegister;
    }

    public static IRValueRef IRBuildAdd(IRBuilder builder, IRValueRef value1, IRValueRef value2, String name) {
        if (value1 instanceof IRConstIntRef && value2 instanceof IRConstIntRef) {//两个都是常数,不需要生成指令
            int res = ((IRConstIntRef) value1).getValue() + ((IRConstIntRef) value2).getValue();
            return new IRConstIntRef(res, int32Type);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstFloatRef) value1).getValue() + ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstIntRef) {
            float res = ((IRConstFloatRef)value1).getValue() + ((IRConstIntRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstIntRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstIntRef)value1).getValue() + ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        }else {
            IRValueRef resRegister;
            if (value1.getType() == int32Type && value2.getType() == floatType) {
                //int到float的隐式转换
                value1 = typeTransfer(builder, value1, IRConst.IntToFloat);
            } else if (value1.getType() == floatType && value2.getType() == int32Type) {
                value2 = typeTransfer(builder, value2, IRConst.IntToFloat);
            }
            resRegister = new IRVirtualRegRef(name, value1.getType());
            if(value1.getType() == int32Type && value2.getType() == int32Type) {
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "add"));
                builder.appendln(resRegister.getText() + " = add " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }else{
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "fadd"));
                builder.appendln(resRegister.getText() + " = fadd " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }
            return resRegister;
        }
    }

    public static IRValueRef IRBuildSub(IRBuilder builder, IRValueRef value1, IRValueRef value2, String name) {
        if (value1 instanceof IRConstIntRef && value2 instanceof IRConstIntRef) {//两个都是常数,不需要生成指令
            int res = ((IRConstIntRef) value1).getValue() - ((IRConstIntRef) value2).getValue();
            return new IRConstIntRef(res, int32Type);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstFloatRef) value1).getValue() - ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstIntRef) {
            float res = ((IRConstFloatRef)value1).getValue() - ((IRConstIntRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstIntRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstIntRef)value1).getValue() - ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        }else {
            IRValueRef resRegister;
            if (value1.getType() == int32Type && value2.getType() == floatType) {
                //int到float的隐式转换
                value1 = typeTransfer(builder, value1, IRConst.IntToFloat);
            } else if (value1.getType() == floatType && value2.getType() == int32Type) {
                value2 = typeTransfer(builder, value2, IRConst.IntToFloat);
            }
            resRegister = new IRVirtualRegRef(name, value1.getType());
            if(value1.getType() == int32Type && value2.getType() == int32Type) {
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "sub"));
                builder.appendln(resRegister.getText() + " = sub " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }else{
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "fsub"));
                builder.appendln(resRegister.getText() + " = fsub " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }
            return resRegister;
        }
    }

    public static IRValueRef IRBuildMul(IRBuilder builder, IRValueRef value1, IRValueRef value2, String name) {
        if (value1 instanceof IRConstIntRef && value2 instanceof IRConstIntRef) {//两个都是常数,不需要生成指令
            int res = ((IRConstIntRef) value1).getValue() * ((IRConstIntRef) value2).getValue();
            return new IRConstIntRef(res, int32Type);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstFloatRef) value1).getValue() * ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstIntRef) {
            float res = ((IRConstFloatRef)value1).getValue() * ((IRConstIntRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstIntRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstIntRef)value1).getValue() * ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        }else {
            IRValueRef resRegister;
            if (value1.getType() == int32Type && value2.getType() == floatType) {
                //int到float的隐式转换
                value1 = typeTransfer(builder, value1, IRConst.IntToFloat);
            } else if (value1.getType() == floatType && value2.getType() == int32Type) {
                value2 = typeTransfer(builder, value2, IRConst.IntToFloat);
            }
            resRegister = new IRVirtualRegRef(name, value1.getType());
            if (value1.getType() == int32Type && value2.getType() == int32Type) {
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "mul"));
                builder.appendln(resRegister.getText() + " = mul " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            } else {
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "fmul"));
                builder.appendln(resRegister.getText() + " = fmul " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }
            return resRegister;
        }
    }
    public static IRValueRef IRBuildDiv(IRBuilder builder, IRValueRef value1, IRValueRef value2, String name) {
        if (value1 instanceof IRConstIntRef && value2 instanceof IRConstIntRef) {//两个都是常数,不需要生成指令
            int res = ((IRConstIntRef) value1).getValue() / ((IRConstIntRef) value2).getValue();
            return new IRConstIntRef(res, int32Type);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstFloatRef) value1).getValue() / ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstFloatRef && value2 instanceof IRConstIntRef) {
            float res = ((IRConstFloatRef)value1).getValue() / ((IRConstIntRef) value2).getValue();
            return new IRConstFloatRef(res);
        } else if (value1 instanceof IRConstIntRef && value2 instanceof IRConstFloatRef) {
            float res = ((IRConstIntRef)value1).getValue() / ((IRConstFloatRef) value2).getValue();
            return new IRConstFloatRef(res);
        }else {
            IRValueRef resRegister;
            if (value1.getType() == int32Type && value2.getType() == floatType) {
                //int到float的隐式转换
                value1 = typeTransfer(builder, value1, IRConst.IntToFloat);
            } else if (value1.getType() == floatType && value2.getType() == int32Type) {
                value2 = typeTransfer(builder, value2, IRConst.IntToFloat);
            }
            resRegister = new IRVirtualRegRef(name, value1.getType());
            if(value1.getType() == int32Type && value2.getType() == int32Type) {
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "sdiv"));
                builder.appendln(resRegister.getText() + " = sdiv " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }else{
                builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "fdiv"));
                builder.appendln(resRegister.getText() + " = fdiv " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            }
            return resRegister;
        }
    }
    public static IRValueRef IRBuildRem(IRBuilder builder, IRValueRef value1, IRValueRef value2, String name) {
        if (value1 instanceof IRConstIntRef && value2 instanceof IRConstIntRef) {//两个都是常数,不需要生成指令
            int res = ((IRConstIntRef) value1).getValue() % ((IRConstIntRef) value2).getValue();
            return new IRConstIntRef(res, int32Type);
        } else {
            IRValueRef resRegister;
            resRegister = new IRVirtualRegRef(name, value1.getType());
            builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, value1, value2), builder.currentBaseBlock, "srem"));
            builder.appendln(resRegister.getText() + " = srem " + value1.getType().getText() + " " + value1.getText() + ", " + value2.getText());
            return resRegister;
        }
    }
    public static IRValueRef IRBuildICmp(IRBuilder builder, int icmpType, IRValueRef lhs, IRValueRef rhs, String text) {
        //先进行类型转换，如果类型不同，将int转为float
        if ((lhs.getType() == int32Type || lhs.getType() == int1Type) && rhs.getType() == floatType) {
            lhs = typeTransfer(builder, lhs, IRConst.IntToFloat);
        } else if (lhs.getType() == floatType && (rhs.getType() == int32Type || rhs.getType() == int1Type)) {
            rhs = typeTransfer(builder, rhs, IRConst.IntToFloat);
        }
        //两者都是int类型或者两者都是float类型
        IRValueRef resRegister = new IRVirtualRegRef(text, int1Type);
        builder.appendInstr(new CompareInstruction(wrapWithList(resRegister, lhs, rhs), builder.currentBaseBlock, icmpType));
        if(lhs.getType() == int32Type && rhs.getType() == int32Type) {
            builder.appendln(resRegister.getText() + " = icmp " + IRConst.compareTypes[icmpType] + " " + lhs.getType().getText() + " " + lhs.getText() + ", " + rhs.getText());
        }else {
            builder.appendln(resRegister.getText() + " = fcmp " + IRConst.floatCompareTypes[icmpType] + " " + lhs.getType().getText() + " " + lhs.getText() + ", " + rhs.getText());
        }
        return resRegister;
    }
    public static IRValueRef IRBuildXor(IRBuilder builder, IRValueRef lhs, IRValueRef rhs, String name) {
        if (lhs instanceof IRConstIntRef) {
            //如果lhs是常数，直接返回结果
            return new IRConstIntRef(Integer.valueOf(lhs.getText()) ^ 1,int1Type);
        }
        IRValueRef resRegister = new IRVirtualRegRef(name, rhs.getType());
        builder.appendInstr(new CalculateInstruction(wrapWithList(resRegister, lhs, rhs), builder.currentBaseBlock, "xor"));
        builder.appendln(resRegister.getText() + " = xor " + rhs.getType().getText() + " " + lhs.getText() + ", true");
        return resRegister;
    }
    public static IRValueRef IRBuildZExt(IRBuilder builder, IRValueRef valueRef, IRType type, String name) {
        if (valueRef.getType() == type) {
            return valueRef;
        }
        if (valueRef instanceof IRConstIntRef) {
            //如果valueRef是常数，直接返回结果
            return new IRConstIntRef(Integer.valueOf(valueRef.getText()), type);
        }
        IRValueRef resRegister = new IRVirtualRegRef(name, type);
        builder.appendInstr(new ZextInstruction(wrapWithList(resRegister, valueRef, new IRConstIntRef(0, type)), builder.currentBaseBlock));
        builder.appendln(resRegister.getText() + " = zext " + valueRef.getType().getText() + " " + valueRef.getText() + " to " + type.getText());
        return resRegister;
    }
    public static IRValueRef IRBuildNeg(IRBuilder builder, IRValueRef valueRef, String name) {
        // appendInstruction in sub
        if (valueRef instanceof IRConstIntRef) {
            return new IRConstIntRef(-Integer.parseInt(valueRef.getText()),valueRef.getType());
        }
        if (valueRef instanceof IRConstFloatRef) {
            return new IRConstFloatRef(-((IRConstFloatRef) valueRef).getValue());
        }
        if (valueRef instanceof IRVirtualRegRef) {
            if (valueRef.getType() == floatType) {
                return IRBuildSub(builder, new IRConstFloatRef(0), valueRef, name);
            } else {
                return IRBuildSub(builder, new IRConstIntRef(0,int32Type), valueRef, name);
            }
        }
        return null;
    }
    public static IRValueRef IRBuildCall(IRBuilder builder, IRFunctionBlockRef function, List<IRValueRef> args, int argc, String varName) {
        IRType retType = ((IRFunctionType) function.getType()).getReturnType();
        IRValueRef resRegister = new IRVirtualRegRef(varName + "_call", retType);
        StringBuilder stringBuilder = new StringBuilder();
        List<IRValueRef> operands = new ArrayList<>();
        operands.add(resRegister);
        operands.add(function);
        if (retType != voidType) {
            stringBuilder.append(resRegister.getText()).append(" = ");
        }
        stringBuilder.append("call").append(" ").append(function.getRetType().getText())
                .append(" ").append(function.getText()).append("(");
        if (argc > 0) {
            operands.add(args.get(0));
            stringBuilder.append(args.get(0).getType().getText()).append(" ").append(args.get(0).getText());
        }
        for (int i = 1; i < argc; i++) {
            operands.add(args.get(i));
            stringBuilder.append(", ")
                    .append(args.get(i).getType().getText())
                    .append(" ").append(args.get(i).getText());
        }
        builder.appendInstr(new CallInstruction(operands, builder.currentBaseBlock));
        //builder.currentBaseBlock.addSuccBaseBlock(function);
        stringBuilder.append(")");
        builder.appendln(stringBuilder.toString());
        return resRegister;
    }
    public static void IRBuildBr(IRBuilder builder, IRBaseBlockRef block) {
        builder.appendInstr(new BranchInstruction(wrapWithList(block), builder.currentBaseBlock));
        builder.appendln( "br label %" + block.getLabel());
    }

    public static void IRBuildCondBr(IRBuilder builder, IRValueRef condition, IRBaseBlockRef ifTrue, IRBaseBlockRef ifFalse) {
        builder.appendInstr(new BranchInstruction(wrapWithList(condition, ifTrue, ifFalse), builder.currentBaseBlock));
        builder.appendln( "br " + condition.getType().getText() + " " + condition.getText() + ", " +
                "label %" + ifTrue.getLabel() + ", label %" + ifFalse.getLabel());
    }


    //私有的辅助方法
    private static List<IRValueRef> wrapWithList(IRValueRef valueRef) {
        List<IRValueRef> list = new ArrayList<>();
        list.add(valueRef);
        return list;
    }

    private static List<IRValueRef> wrapWithList(IRValueRef valueRef1, IRValueRef valueRef2, IRValueRef valueRef3) {
        List<IRValueRef> list = new ArrayList<>();
        list.add(valueRef1);
        list.add(valueRef2);
        list.add(valueRef3);
        return list;
    }

    private static List<IRValueRef> wrapWithList(IRValueRef valueRef1, IRValueRef valueRef2) {
        List<IRValueRef> list = new ArrayList<>();
        list.add(valueRef1);
        list.add(valueRef2);
        return list;
    }

    public static IRValueRef typeTransfer(IRBuilder builder, IRValueRef valueRef, int type) {
        IRValueRef resRegister;
        if (type == IRConst.FloatToInt) {
            resRegister = new IRVirtualRegRef("temp", int32Type);
            builder.appendInstr(new TypeTransferInstruction(wrapWithList(resRegister, valueRef), builder.currentBaseBlock, IRConst.FloatToInt));
            builder.appendln(resRegister.getText() + " = fptosi " + valueRef.getType().getText() + " " + valueRef.getText() + " to " + int32Type.getText());
        } else {
            resRegister = new IRVirtualRegRef("temp", floatType);
            builder.appendInstr(new TypeTransferInstruction(wrapWithList(resRegister, valueRef), builder.currentBaseBlock, IRConst.IntToFloat));
            builder.appendln(resRegister.getText() + " = sitofp " + valueRef.getType().getText() + " " + valueRef.getText() + " to " + floatType.getText());
        }
        return resRegister;
    }

    private void appendln(String s) {
        currentBaseBlock.appendln(s);
    }

    private void appendln(String s, int align) {
        //align表示根据align对齐
        currentBaseBlock.appendln(s, align);
    }

    private void appendInstr(IRInstruction instr) {
        currentBaseBlock.appendInstr(instr);
    }

    //提供给后端访问的接口方法
    public static IRBaseBlockRef IRGetFirstBasicBlock(IRFunctionBlockRef functionBlock) {
        return functionBlock.getBaseBlocks().get(0);
    }
    public static IRBaseBlockRef IRGetNextBasicBlock(IRBaseBlockRef baseBlock) {
        return (IRBaseBlockRef) baseBlock.getSuccList().get(0);
    }
    public static IRBaseBlockRef IRGetPreviousBasicBlock(IRBaseBlockRef baseBlock) {
        return (IRBaseBlockRef) baseBlock.getPredList().get(0);
    }
    public static String IRGetBasicBlockName(IRBaseBlockRef baseBlock) {
        return baseBlock.getText();
    }
    public static List<IRInstruction> IRGetInstructions(IRBaseBlockRef baseBlock) {
        return baseBlock.getInstructionList();
    }
}
