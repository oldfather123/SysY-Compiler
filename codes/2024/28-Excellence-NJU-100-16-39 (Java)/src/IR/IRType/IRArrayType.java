package IR.IRType;

import IR.IRConst;

import java.util.ArrayList;
import java.util.List;

public class IRArrayType implements IRType{
    /*
    * 可以理解成Length * BaseType
    * BaseType: 数组元素的类型 e.g. i32或是其他数组
    * Length: 数组的长度
    * lengthList: 保存数组的维度信息
    * e.g. [3 x [2 x i32]]: 长度为3的数组，数组元素是长度为2的32位有符号整数数组
    * 相当于C/C++中的int[3][2]
    * 这时lengthList为[3, 2]，第一个元素表示第一维的长度，第二个元素表示第二维的长度
    * length存储的是第一维的长度，baseType存储的是第二维的类型
     */
    private IRType baseType;
    private int length;
    private List<Integer> lengthList;
    private boolean needInit = false;
    public IRArrayType(IRType baseType, int length){
        this.baseType = baseType;
        this.length = length;
        if(baseType instanceof IRArrayType){
            //如果数组元素是数组类型，那么将原来的维度列表加入到新的维度列表中
            lengthList = ((IRArrayType) baseType).getLengthList();
        }else {
            lengthList = new ArrayList<>();
        }
        //将当前的长度加入到维度列表的第一个位置
        lengthList.add(0, length);
        needInit = false;
    }

    public List<Integer> getLengthList() {
        return lengthList;
    }

    public int getLength() {
        return length;
    }

    public IRType getBaseType() {
        return baseType;
    }

    public boolean isNeedInit() {
        return needInit;
    }
    public void setNeedInit(boolean needInit) {
        this.needInit = needInit;
    }

    @Override
    public String getText() {
        //返回数组的字符串表示，e.g. int[3][2]就是[3 x [2 x i32]]
        return "[" + length + " x " + baseType.getText() + "]";
    }

    @Override
    public int getTypeKind() {
        return IRConst.IRConstantArrayValueKind;
    }
}
