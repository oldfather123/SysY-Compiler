package IR.IRType;

import IR.IRConst;

public class IRInt32Type implements IRType{
    private static final String name = "i32";
    private static IRInt32Type int32Type = null;/*将初始化延迟到第一次使用*/

    public IRInt32Type() {}

    public static IRType IRInt32Type() {/*静态方法，直接通过类名调用*/
        if (int32Type == null) {
            int32Type = new IRInt32Type();//如果int32Type为空，创建一个新的Int32Type对象，从而保证全局只有一个Int32Type对象
        }
        return int32Type;
    }

    @Override
    public String getText() {
        return name;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantInt32ValueKind;
    }
}
