package midend;

import midend.LLVMType.*;
import midend.instr.*;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;

public class IRBuilder {
    public IRBuilder() {

    }

    public void buildRetInstr(Type type, String name, Value value, BasicBlock block) {
        RetInstr retInstr = new RetInstr(type, name, value, block);
        retInstr.setBasicBlock(block);
        block.addInstr(retInstr);
    }

    public ALUInstr buildALUInstr(Value left, Value right, String op, BasicBlock block) {
        Type type = null;
        if (left.getType() != right.getType()) {
            if (left.getType() == IntegerType.i1) {
                left = buildCvsInstr(InstrType.ZEXT, left, block);
            }
            if (right.getType() == IntegerType.i1){
                right = buildCvsInstr(InstrType.ZEXT, right, block);
            }
            type = IntegerType.i32;
            if (left.getType() == IntegerType.i32 && right.getType() == FloatType.f32) {
                left = buildCvsInstr(InstrType.SITOFP, left, block);
                type = FloatType.f32;
            } else if (left.getType() == FloatType.f32 && right.getType() == IntegerType.i32) {
                right = buildCvsInstr(InstrType.SITOFP, right, block);
                type = FloatType.f32;
            }
        }
        type = left.getType();

        InstrType instrType1 = InstrType.strToInstr(op);
        if (type.isFloat()) {
            instrType1 = instrType1.toFloat();
        }
        ALUInstr instr;
        if (instrType1 == InstrType.ICMP || instrType1 == InstrType.FCMP) {
            instr = new CmpInstr(op, IntegerType.i1, Arrays.asList(left, right), instrType1, block);
        } else {
            instr = new ALUInstr(type, Arrays.asList(left, right), instrType1, block);
        }
        block.addInstr(instr);
        return instr;
    }

    public ConversionInstr buildCvsInstr(InstrType instrType, Value value, BasicBlock block) {
        Type type = null;
        if (instrType == InstrType.ZEXT) {
            type = IntegerType.i32;
        } else if (instrType == InstrType.FPTOSI) {
            type = IntegerType.i32;
        } else if (instrType == InstrType.SITOFP) {
            type = FloatType.f32;
        } else if (instrType == InstrType.BITCAST){
            type = new PointerType(IntegerType.i32);
        } else {
            return null;
        }
        ConversionInstr instr = new ConversionInstr(type, instrType, value);
        block.addInstr(instr);
        instr.setBasicBlock(block);
        return instr;
    }

    public AllocaInstr buildAllocaInstr(Type type, BasicBlock basicBlock) {
        PointerType pointerType = new PointerType(type);
        return new AllocaInstr(pointerType, basicBlock);
    }

    public StoreInstr buildStoreInstr(Value value, Value pointer, BasicBlock block) {
        PointerType pointerType = (PointerType) pointer.getType();
        Type eleType = pointerType.getElementType();
        if (value.getType() == IntegerType.i32 && eleType == FloatType.f32) {
            if (value instanceof ConstInt) {
                value = new ConstFloat(FloatType.f32, (float) ((ConstInt) value).getValue());
            } else {
                value = buildCvsInstr(InstrType.SITOFP, value, block);
            }
        } else if (value.getType() == FloatType.f32 && eleType == IntegerType.i32) {
            if (value instanceof ConstFloat) {
                value = new ConstInt(IntegerType.i32, (int) ((ConstFloat) value).getValue());
            } else {
                value = buildCvsInstr(InstrType.FPTOSI, value, block);
            }
        }
        StoreInstr storeInstr = new StoreInstr(value.getType(), value, pointer, block);
        block.addInstr(storeInstr);
        return storeInstr;
    }

    public LoadInstr buildLoadInstr(Value pointer, BasicBlock block) {
        PointerType pointerType = (PointerType) pointer.getType();
        LoadInstr loadInstr = new LoadInstr(pointerType.getElementType(), pointer, block);
        if (block != null){
            block.addInstr(loadInstr);
            loadInstr.setBasicBlock(block);
        }
        return loadInstr;
    }

    public CallInstr buildCallInstr(Function function, ArrayList<Value> values, BasicBlock block) {
        CallInstr callInstr = new CallInstr(function.getRetType(), function, values);
        block.addInstr(callInstr);
        callInstr.setBasicBlock(block);
        //TODO:buildCallRelation();
        return callInstr;

    }

    public Constant buildNumber(Constant left, Constant right, String op) {
        if (left instanceof ConstInt && right instanceof ConstInt) {
            return new ConstInt(IntegerType.i32, calcu(((ConstInt) left).getValue(), ((ConstInt) right).getValue(), op));
        } else if (left instanceof ConstFloat && right instanceof ConstInt) {
            return new ConstFloat(FloatType.f32, calcu(((ConstFloat) left).getValue(), (float) ((ConstInt) right).getValue(), op));
        } else if (left instanceof ConstInt && right instanceof ConstFloat) {
            return new ConstFloat(FloatType.f32, calcu((float) ((ConstInt) left).getValue(), ((ConstFloat) right).getValue(), op));
        } else if (left instanceof ConstFloat && right instanceof ConstFloat){
            return new ConstFloat(FloatType.f32, calcu(((ConstFloat) left).getValue(), ((ConstFloat) right).getValue(), op));
        }
        return null;
    }

    public float calcu(float left, float right, String op) {
        return switch (op) {
            case "+" -> left + right;
            case "-" -> left - right;
            case "*" -> left * right;
            case "/" -> left / right;
            case "%" -> left % right;
            case "<" -> (left < right)? 1 : 0;
            case "<=" -> (left <= right)? 1 : 0;
            case ">" -> (left > right)? 1 : 0;
            case ">=" -> (left >= right)? 1 : 0;
            case "==" -> (left == right)? 1 : 0;
            case "!=" -> (left != right)? 1 : 0;
            default -> 0;
        };
    }

    public int calcu(int left, int right, String op) {
        return switch (op) {
            case "+" -> left + right;
            case "-" -> left - right;
            case "*" -> left * right;
            case "/" -> left / right;
            case "%" -> left % right;
            //case "&" -> left & right;
            case "<" -> (left < right)? 1 : 0;
            case "<=" -> (left <= right)? 1 : 0;
            case ">" -> (left > right)? 1 : 0;
            case ">=" -> (left >= right)? 1 : 0;
            case "==" -> (left == right)? 1 : 0;
            case "!=" -> (left != right)? 1 : 0;
            case "<<" -> left << right;
            case ">>" -> left >> right;
            case ">>>" -> left >>> right;
            default -> 0;
        };
    }

    public BrInstr buildBrInstr(Value cond, BasicBlock trueBlock, BasicBlock flaseBlock, BasicBlock block) {
        BrInstr brInstr = new BrInstr(cond, Arrays.asList(trueBlock, flaseBlock));
        block.addInstr(brInstr);
        brInstr.setBasicBlock(block);
        return brInstr;
    }

    public BrInstr buildBrInstr(BasicBlock destBlock, BasicBlock block) {
        BrInstr brInstr = new BrInstr(destBlock);
        block.addInstr(brInstr);
        brInstr.setBasicBlock(block);
        return brInstr;
    }

    public GetElementPtrInstr buildGepInstr(Value ptrval, ArrayList<Value> indexs, BasicBlock block) {
        Type type = ((PointerType) ptrval.getType()).getElementType();
        for (int count = 0; count < indexs.size() - 1; count++) {
            type = ((ArrayType) type).getElementType();
        }
        type = new PointerType(type);
        GetElementPtrInstr getElementPtrInstr = new GetElementPtrInstr(type, ptrval, indexs);
        if (block != null) {
            getElementPtrInstr.setBasicBlock(block);
            block.addInstr(getElementPtrInstr);
        }
        return getElementPtrInstr;
    }
}
