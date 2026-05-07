package tools;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.instr.Sitofp;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.Type;
import ir.type.TypeName;
import ir.value.BasicBlock;

import java.util.LinkedList;

public class BinaryInstGenerator {
    //生成二元计算指令，处理类型隐式转换
    //binaryType只需要传入整数，由此函数处理浮点数的binaryType
    public static Instr genBinaryComputeInst(
        LinkedList<Value> uses, IrRegDispatcher dispatcher, InstType.BinaryType binaryType,
        BasicBlock basicBlock
    ) {
        boolean need32 = binaryType == InstType.BinaryType.add||
            binaryType == InstType.BinaryType.sub ||
            binaryType == InstType.BinaryType.mul ||
            binaryType == InstType.BinaryType.sdiv;
        //获取到正确的目标类型对象
        Type targetType = TypeCaster.castTypeAndSet(uses, dispatcher, basicBlock,need32);
        if(binaryType == InstType.BinaryType.or || binaryType == InstType.BinaryType.and){
            targetType = new IntType(1);
        }
        //如果目标类型是float，意味着此时两个运算数都是浮点类型，故要修改binaryInstr的类型为对应的浮点类型。
        //此处不优雅，可考虑修改。
        return new BinaryInstr(uses, targetType, dispatcher.allocId(),
            targetType.getType() == TypeName.F ?
                binaryType == InstType.BinaryType.add ?
                    InstType.BinaryType.fadd :
                    binaryType == InstType.BinaryType.sub ?
                        InstType.BinaryType.fsub :
                        binaryType == InstType.BinaryType.mul ?
                            InstType.BinaryType.fmul :
                            binaryType == InstType.BinaryType.sdiv ?
                                InstType.BinaryType.fdiv :
                                binaryType
                : binaryType, basicBlock
        );
    }

    public static Instr genBinaryCmpInst(LinkedList<Value> uses, IrRegDispatcher dispatcher, InstType.CmpType cmpType,
                                         BasicBlock basicBlock){
        //获取到正确的目标类型对象
        Type targetType = TypeCaster.castTypeAndSet(uses, dispatcher, basicBlock, false);
        //如果目标类型是float，意味着此时两个运算数都是浮点类型，故要修改binaryInstr的类型为对应的浮点类型。
        //此处不优雅，可考虑修改。
        BinaryInstr instr =  new BinaryInstr(uses, new IntType(1), dispatcher.allocId(),
            targetType instanceof FloatType? InstType.BinaryType.fcmp : InstType.BinaryType.icmp, basicBlock
        );
        if(targetType instanceof FloatType) {
            if(cmpType == InstType.CmpType.lt) {
                cmpType = InstType.CmpType.olt;
            } else if(cmpType == InstType.CmpType.le) {
                cmpType = InstType.CmpType.ole;
            } else if(cmpType == InstType.CmpType.gt) {
                cmpType = InstType.CmpType.ogt;
            } else if(cmpType == InstType.CmpType.ge) {
                cmpType = InstType.CmpType.oge;
            } else if(cmpType == InstType.CmpType.ne) {
                cmpType = InstType.CmpType.one;
            } else if(cmpType == InstType.CmpType.eq) {
                cmpType = InstType.CmpType.oeq;
            }
        }
        instr.cmpType = cmpType;
        return instr;
    }
}
