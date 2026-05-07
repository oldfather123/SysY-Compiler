package cn.edu.bit.newnewcc.backend.asm;

import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.util.Utility;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;

import java.util.ArrayList;
import java.util.List;

/**
 * 用于存储汇编格式的全局（静态）变量，以变量名作为唯一标识符作为区分
 */
public class AsmGlobalVariable {
    private final String globalVariableName;
    private final boolean isConstant, isInitialized, isSmallSection;
    private final long size;
    //存储了该全局变量的全部数据
    private final List<ValueDirective> valueList;
    private final int align;

    public AsmGlobalVariable(GlobalVariable globalVariable) {
        this.globalVariableName = "globalVariable_" + globalVariable.getValueName();
        this.isConstant = globalVariable.isConstant();

        Constant initialValue = globalVariable.getInitialValue();
        this.isInitialized = (!initialValue.isFilledWithZero());
        this.size = initialValue.getType().getSize();
        this.valueList = new ArrayList<>();
        if (initialValue instanceof ConstInt intValue) {
            this.align = 2;
            this.valueList.add(new ValueDirective(intValue.getValue()));
            this.isSmallSection = true;
        } else if (initialValue instanceof ConstFloat floatValue) {
            this.align = 2;
            this.valueList.add(new ValueDirective(floatValue.getValue()));
            this.isSmallSection = true;
        } else if (initialValue instanceof ConstArray arrayValue) {
            this.align = 3;
            this.isSmallSection = false;
            if (this.isInitialized) {
                this.getArrayValues(arrayValue);
            } else {
                this.valueList.add(ValueDirective.getZeroValue(this.size));
            }
        } else {
            this.align = 2;
            this.isSmallSection = true;
        }
    }

    private void getArrayValues(ConstArray arrayValue) {
        Utility.workOnArray(arrayValue, 0, (Long offset, Constant item) -> {
            if (item instanceof ConstInt arrayItem) {
                this.valueList.add(new ValueDirective(arrayItem.getValue()));
            } else if (item instanceof ConstFloat arrayItem) {
                this.valueList.add(new ValueDirective(arrayItem.getValue()));
            }
        }, (Long offset, Long length) -> this.valueList.add(ValueDirective.getZeroValue(length)));
    }

    public Label emitNoSegmentLabel() {
        return new Label(globalVariableName, true);
    }

    private String getSectionStr() {
        if (this.isConstant) {
            if (this.isSmallSection) {
                return ".section .srodata,\"a\"";
            } else {
                return ".section .rodata";
            }
        } else if (this.isInitialized) {
            if (this.isSmallSection) {
                return ".section .sdata,\"aw\"";
            } else {
                return ".data";
            }
        } else {
            if (this.isSmallSection) {
                return ".section .sbss,\"aw\",@nobits";
            } else {
                return ".bss";
            }
        }
    }

    /**
     * 输出汇编格式的全局变量数据
     */
    public String emit() {
        StringBuilder output = new StringBuilder();
        output.append(String.format(".globl %s\n", this.globalVariableName));
        output.append(this.getSectionStr()).append('\n');
        output.append(String.format(".align %d\n", this.align));
        output.append(String.format(".type %s, @object\n", this.globalVariableName));
        output.append(String.format(".size %s, %d\n", this.globalVariableName, this.size));
        output.append(String.format("%s:\n", this.globalVariableName));
        for (var i : valueList) {
            output.append(i.emit());
        }
        return output.toString();
    }

}
