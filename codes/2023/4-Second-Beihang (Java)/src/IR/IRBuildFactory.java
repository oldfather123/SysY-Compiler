package IR;

import IR.Type.*;
import IR.Value.*;
import IR.Value.Instructions.*;
import java.util.ArrayList;

public class IRBuildFactory {
    private IRBuildFactory(){}

    private static final IRBuildFactory f = new IRBuildFactory();

    public static IRBuildFactory getInstance(){
        return f;
    }

    private int calculate(int a, int b, String op){
        return switch (op){
            case "+" -> a + b;
            case "-" -> a - b;
            case "*" -> a * b;
            case "/" -> a / b;
            case "%" -> a % b;
            case "<" -> a < b ? 1 : 0;
            case "<=" -> a <= b ? 1 : 0;
            case ">" -> a > b ? 1 : 0;
            case ">=" -> a >= b ? 1 : 0;
            case "==" -> a == b ? 1 : 0;
            case "!=" -> a != b ? 1 : 0;
            default -> 0;
        };
    }

    private float calculate(float a, float b, String op){
        return switch (op){
            case "+" -> a + b;
            case "-" -> a - b;
            case "*" -> a * b;
            case "/" -> a / b;
            case "%" -> a % b;
            case "<" -> a < b ? 1 : 0;
            case "<=" -> a <= b ? 1 : 0;
            case ">" -> a > b ? 1 : 0;
            case ">=" -> a >= b ? 1 : 0;
            case "==" -> a == b ? 1 : 0;
            case "!=" -> a != b ? 1 : 0;
            default -> 0;
        };
    }

    private ArrayType getArrayType(ArrayList<Integer> indexs, Type eleType){
        if(indexs.size() == 1){
            return new ArrayType(indexs.get(0), eleType);
        }

        ArrayList<Integer> newIndexs = new ArrayList<>();
        for(int i = 1; i < indexs.size(); i++){
            newIndexs.add(indexs.get(i));
        }
        Type type = getArrayType(newIndexs, eleType);
        return new ArrayType(indexs.get(0), type);
    }

    public GlobalVar getGlobalArray(String name, Type eleType, ArrayList<Integer> indexs, ArrayList<Value> values){
        ArrayType arrayType = getArrayType(indexs, eleType);
        return new GlobalVar("@" + name, new PointerType(arrayType), values);
    }

    public GlobalVar getGlobalArray(String name, Type arrType, ArrayList<Value> values){
        return new GlobalVar("@" + name, new PointerType(arrType), values);
    }

    //  传入一个指向目标数组的指针
    public GepInst buildGepInst(Value target, ArrayList<Value> indexs, BasicBlock bb){
        Type type = ((PointerType) target.getType()).getEleType();
        for(int i = 0; i < indexs.size() - 1; i++){
            type = ((ArrayType) type).getEleType();
        }

        GepInst gepInst = new GepInst(target, indexs, new PointerType(type));
        bb.addInst(gepInst);
        return gepInst;
    }

    public GepInst getGepInst(Value target, ArrayList<Value> indexs){
        Type type = ((PointerType) target.getType()).getEleType();
        for(int i = 0; i < indexs.size() - 1; i++){
            type = ((ArrayType) type).getEleType();
        }

        return new GepInst(target, indexs, new PointerType(type));
    }

    public AllocInst buildArray(Type eleType, ArrayList<Integer> indexs, BasicBlock bb){
        ArrayType arrayType = getArrayType(indexs, eleType);
        return buildAllocInst(arrayType, bb);
    }

    public Function getFunction(String name, Type type){
        return new Function(name, type);
    }

    public Argument getArgument(String name, Type type, Function parentFunc){
        return new Argument(name, type, parentFunc);
    }

    public BasicBlock getBasicBlock(Function parentFunc){
        return new BasicBlock(parentFunc);
    }

    public Instruction getAllocInst(Type type){
        Type pointerTy = new PointerType(type);
        return new AllocInst(pointerTy);
    }

    public Instruction getAllocInst(Type type, ArrayList<Value> initValues){
        Type pointerTy = new PointerType(type);
        AllocInst allocInst = new AllocInst(pointerTy);
        allocInst.setInitValues(initValues);
        return allocInst;
    }

    public LoadInst getLoadInst(Value pointer){
        Type type = ((PointerType) pointer.getType()).getEleType();
        return new LoadInst(pointer, type);
    }

    public StoreInst getStoreInst(Value value, Value pointer){
        return new StoreInst(value, pointer);
    }

    private void buildCallRelation(Function caller, Function callee){
        if(callee.isLibFunction()) return;
        if(!callee.getCallerList().contains(caller)) {
            callee.addCaller(caller);
        }
        if(!caller.getCalleeList().contains(callee)){
            caller.addCallee(callee);
        }
    }

    private Value turnType(Value value, Type targetTy, BasicBlock bb){
        Type type = value.getType();
        if(type == IntegerType.I1 && targetTy == IntegerType.I32){
            return buildConversionInst(value, OP.Zext, bb);
        }
        else if(type == IntegerType.I32 && targetTy == FloatType.F32){
            return buildConversionInst(value, OP.Itof, bb);
        }
        else if(type == FloatType.F32 && targetTy == IntegerType.I32){
            return buildConversionInst(value, OP.Ftoi, bb);
        }
        return null;
    }

    public Const buildI1Number(int val){
        return new ConstInteger(val, IntegerType.I1);
    }

    public Const buildNumber(int val){
        return new ConstInteger(val, IntegerType.I32);
    }
    public Const buildNumber(float val){
        return new ConstFloat(val);
    }

    public GlobalVar buildGlobalVar(String name, Type type, Value value){
        return new GlobalVar("@" + name, new PointerType(type), value);
    }

    public ConversionInst buildConversionInst(Value value, OP op, BasicBlock bb){
        Type type = null;
        if(op == OP.Ftoi || op == OP.Zext){
            type = IntegerType.I32;
        }
        else if(op == OP.Itof){
            type = FloatType.F32;
        }
        else if(op == OP.BitCast){
            type = new PointerType(IntegerType.I32);
        }

        ConversionInst conversionInst = new ConversionInst(value, type, op);
        bb.addInst(conversionInst);
        return conversionInst;
    }

    public ConversionInst getConversionInst(Value value, OP op){
        Type type = null;
        if(op == OP.Ftoi || op == OP.Zext){
            type = IntegerType.I32;
        }
        else if(op == OP.Itof){
            type = FloatType.F32;
        }
        else if(op == OP.BitCast){
            type = new PointerType(IntegerType.I32);
        }

        return new ConversionInst(value, type, op);
    }

    public BinaryInst getBinaryInst(Value left, Value right, OP op, Type type){
        BinaryInst binaryInst;
        if(op.isCmpOP()){
            binaryInst = new CmpInst(op, left, right);
        }
        else {
            binaryInst = new BinaryInst(op, left, right, type);
        }
        return binaryInst;
    }

    public BinaryInst buildBinaryInst(Value left, Value right, OP op, BasicBlock bb){
        Type type;
        Type leftType = left.getType();
        Type rightType = right.getType();
        if(leftType != rightType) {
            //  先将1位的全部转化为I32
            if(leftType == IntegerType.I1){
                left = turnType(left, IntegerType.I32, bb);
            }
            if(rightType == IntegerType.I1){
                right = turnType(right, IntegerType.I32, bb);
            }
            type = IntegerType.I32;

            assert left != null;
            leftType = left.getType();
            assert right != null;
            rightType = right.getType();

            if(leftType == FloatType.F32 && rightType == IntegerType.I32){
                right = turnType(right, FloatType.F32, bb);
                type = FloatType.F32;
            }
            if(rightType == FloatType.F32 && leftType == IntegerType.I32){
                left = turnType(left, FloatType.F32, bb);
                type = FloatType.F32;
            }
        }
        else type = leftType;

        assert type != null;
        if(type.isFloatTy()){
            op = OP.turnToFloat(op);
        }

        BinaryInst binaryInst;
        if(op.isCmpOP()){
            binaryInst = new CmpInst(op, left, right);
        }
        else {
            binaryInst = new BinaryInst(op, left, right, type);
        }
        bb.addInst(binaryInst);
        return binaryInst;
    }

    public CallInst buildCallInst(Function callFunc, ArrayList<Value> values, BasicBlock bb){
        CallInst callInst = new CallInst(callFunc, values);
        bb.addInst(callInst);

        buildCallRelation(bb.getParentFunc(), callFunc);
        return callInst;
    }

    public CallInst getCallInst(Function callFunc, ArrayList<Value> values){
        return new CallInst(callFunc, values);
    }

    public Argument buildArgument(String name, String typeStr, Function parentFunc){
        Argument argument = switch (typeStr) {
            case "int" -> new Argument(name, IntegerType.I32, parentFunc);
            case "float" -> new Argument(name, FloatType.F32, parentFunc);
            case "int*" -> new Argument(name, new PointerType(IntegerType.I32), parentFunc);
            case "float*" -> new Argument(name, new PointerType(FloatType.F32), parentFunc);
            default -> new Argument(name, VoidType.voidType, parentFunc);
        };
        parentFunc.addArg(argument);
        return argument;
    }

    public Argument buildArgument(String name, String typeStr, Function parentFunc, ArrayList<Integer> indexs){
        Type eleType;
        if(typeStr.equals("int")) eleType = IntegerType.I32;
        else eleType = FloatType.F32;
        Type type = new PointerType(getArrayType(indexs, eleType));
        Argument argument = new Argument(name, type, parentFunc);

        parentFunc.addArg(argument);
        return argument;
    }

    public RetInst buildRetInst(BasicBlock bb){
        Value voidValue = new Value("void", VoidType.voidType);
        RetInst retInst = new RetInst(voidValue);
        bb.addInst(retInst);
        return retInst;
    }

    public RetInst buildRetInst(Value value, BasicBlock bb){
        assert value != null;
        RetInst retInst = new RetInst(value);
        bb.addInst(retInst);
        return retInst;
    }

    public RetInst getRetInst(Value value){
        return new RetInst(value);
    }

    public RetInst getRetInst(){
        return new RetInst();
    }

    public BrInst buildBrInst(BasicBlock jumpBB, BasicBlock bb){
        BrInst brInst = new BrInst(jumpBB);
        bb.addInst(brInst);
        return brInst;
        //  前驱后继关系
//        bb.getNxtBlocks().clear();
//        bb.setNxtBlock(jumpBB);
//        jumpBB.setPreBlock(bb);
    }

    public BrInst getBrInst(BasicBlock jumpBB){
        return new BrInst(jumpBB);
    }

    public BrInst buildBrInst(Value judVal, BasicBlock trueBlock, BasicBlock falseBlock, BasicBlock bb){
        BrInst brInst = new BrInst(judVal, trueBlock, falseBlock);
        bb.addInst(brInst);
        return brInst;
        //  前驱后继关系
//        bb.getNxtBlocks().clear();
//        bb.setNxtBlock(trueBlock);
//        bb.setNxtBlock(falseBlock);
//        trueBlock.setPreBlock(bb);
//        falseBlock.setPreBlock(bb);
    }

    public BrInst getBrInst(Value judVal, BasicBlock trueBlock, BasicBlock falseBlock){
        return new BrInst(judVal, trueBlock, falseBlock);
    }

    public Phi buildPhi(BasicBlock bb, Type type, ArrayList<Value> values){
        Phi phi = new Phi(type, values);
        bb.addInstToHead(phi);
        return phi;
    }

    public Phi getPhi(Type type, ArrayList<Value> values){
        return new Phi(type, values);
    }

    public BasicBlock buildBasicBlock(Function parentFunc){
        BasicBlock bb = new BasicBlock(parentFunc);
        parentFunc.getBbs().add(bb.getNode());
        return bb;
    }

    public Function buildFunction(String name, String type, IRModule module){
        Function function;
        if(type.equals("int")){
            function = new Function(name, IntegerType.I32);
        }
        else if(type.equals("float")){
            function = new Function(name, FloatType.F32);
        }
        else {
            function = new Function(name, VoidType.voidType);
        }
        module.addFunction(function);
        return function;
    }

    public Const buildCalculateNumber(Const _left, Const _right, String op){
        if(_left instanceof ConstFloat left && _right instanceof ConstInteger right){
            return new ConstFloat(calculate(left.getValue(), (float) right.getValue(), op));
        }
        else if(_left instanceof ConstFloat left && _right instanceof ConstFloat right){
            return new ConstFloat(calculate(left.getValue(), right.getValue(), op));
        }
        else if(_left instanceof ConstInteger left && _right instanceof ConstInteger right){
            return new ConstInteger(calculate(left.getValue(), right.getValue(), op), IntegerType.I32);
        }
        else if(_left instanceof ConstInteger left && _right instanceof ConstFloat right){
            return new ConstFloat(calculate((float) left.getValue(), right.getValue(), op));
        }
        return null;
    }

    public AllocInst buildAllocInst(Type type, BasicBlock bb){
        Type pointerTy = new PointerType(type);
        AllocInst allocInst = new AllocInst(pointerTy);
        bb.addInst(allocInst);
        return allocInst;
    }

    public StoreInst buildStoreInst(Value value, Value pointer, BasicBlock bb){
        Type valueTy = value.getType();
        PointerType pointerTy = (PointerType) pointer.getType();
        Type eleTy = pointerTy.getEleType();
        if(valueTy == IntegerType.I32 && eleTy == FloatType.F32){
            if(value instanceof ConstInteger constInt){
                value = f.buildNumber((float) constInt.getValue());
            }
            else{
                value = f.buildConversionInst(value, OP.Itof, bb);
            }
        }
        else if(valueTy == FloatType.F32 && eleTy == IntegerType.I32){
            if(value instanceof ConstFloat constFloat){
                value = f.buildNumber((int) constFloat.getValue());
            }
            else{
                value = f.buildConversionInst(value, OP.Ftoi, bb);
            }
        }

        StoreInst storeInst = new StoreInst(value, pointer);
        bb.addInst(storeInst);
        return storeInst;
    }

    public LoadInst buildLoadInst(Value pointer, BasicBlock bb){
        Type type = ((PointerType) pointer.getType()).getEleType();
        LoadInst loadInst = new LoadInst(pointer, type);
        bb.addInst(loadInst);
        return loadInst;
    }
}
