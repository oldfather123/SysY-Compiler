package cn.edu.bit.newnewcc.pass.ir.util;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.BaseFunction;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;

public class UtilFunctions {

    public static boolean isGlobalValue(Value value) {
        return value instanceof GlobalVariable || value instanceof BaseFunction || value instanceof Constant;
    }

}
