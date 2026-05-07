package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.ExternalFunction;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.BitCastInst;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;
import cn.edu.bit.newnewcc.ir.value.instruction.GetElementPtrInst;
import cn.edu.bit.newnewcc.ir.value.instruction.StoreInst;

import java.util.ArrayList;
import java.util.List;

/**
 * 初始化局部数组 <br>
 * 前端产生的局部数组初值是用常量数组赋值的，需要改用 memset+store 语句进行初始化 <br>
 */
public class LocalArrayInitializePass {

    private static void generateInitializationStore(List<Instruction> generatedInstructions, Value address, int offset, ConstArray constArray) {
        for (int i = 0; i < constArray.getInitializedLength(); i++) {
            var value = constArray.getValueAt(i);
            if (value instanceof ConstInt constInt) {
                if (constInt.isFilledWithZero()) continue;
                var indices = new ArrayList<Value>();
                indices.add(ConstInt.getInstance(0));
                indices.add(ConstInt.getInstance(offset + i));
                var gep = new GetElementPtrInst(address, indices);
                generatedInstructions.add(gep);
                var storeInst = new StoreInst(gep, constInt);
                generatedInstructions.add(storeInst);
            } else if (value instanceof ConstArray subConstArray) {
                generateInitializationStore(generatedInstructions, address, (offset + i) * subConstArray.getLength(), subConstArray);
            } else {
                throw new CompilationProcessCheckFailedException("Unable to generate initialize sequence for initial value of class " + value.getClass());
            }
        }
    }

    private static void runOnInstruction(StoreInst storeInst, ExternalFunction memsetFunction) {
        // 检查是否是在存储常量数组（外面已经检查过了，再检查一遍以防外部修改导致此处出错）
        if (!(storeInst.getValueOperand() instanceof ConstArray initialConstArray)) return;
        var initializeInstructions = new ArrayList<Instruction>();
        // 构造 memset 参数1 (i32*)address
        // 选择 i32* 是因为 clang 不接受 void* ，而 i8* 需要构建新的类
        var address = storeInst.getAddressOperand();
        var bitCastInst = new BitCastInst(address, PointerType.getInstance(IntegerType.getI32()));
        initializeInstructions.add(bitCastInst);
        // 构造 memset 参数3
        long arraySize = initialConstArray.getType().getSize();
        // 构造 call memset
        List<Value> arguments = new ArrayList<>();
        arguments.add(bitCastInst);
        arguments.add(ConstInt.getInstance(0));
        arguments.add(ConstInt.getInstance((int) arraySize));
        var memsetInst = new CallInst(memsetFunction, arguments);
        initializeInstructions.add(memsetInst);
        // 构造初始值
        var flattenedArray = new BitCastInst(address, PointerType.getInstance(((ArrayType) ((PointerType) address.getType()).getBaseType()).getFlattenedType()));
        initializeInstructions.add(flattenedArray);
        generateInitializationStore(initializeInstructions, flattenedArray, 0, initialConstArray);
        // 替换语句
        for (Instruction initializeInstruction : initializeInstructions) {
            initializeInstruction.insertBefore(storeInst);
        }
        storeInst.waste();
    }

    public static void runOnModule(Module module) {
        ExternalFunction memsetFunction = null;
        for (ExternalFunction externalFunction : module.getExternalFunctions()) {
            if (externalFunction.getFunctionName().equals("memset")) {
                memsetFunction = externalFunction;
                break;
            }
        }
        if (memsetFunction == null) {
            // 外部函数列表里没有memset，寄！
            throw new CompilationProcessCheckFailedException("Unable to locate function memset.");
        }
        for (Function function : module.getFunctions()) {
            for (BasicBlock basicBlock : function.getBasicBlocks()) {
                for (Instruction instruction : basicBlock.getMainInstructions()) {
                    if (instruction instanceof StoreInst storeInst && storeInst.getValueOperand() instanceof ConstArray) {
                        runOnInstruction(storeInst, memsetFunction);
                    }
                }
            }
        }
    }
}
