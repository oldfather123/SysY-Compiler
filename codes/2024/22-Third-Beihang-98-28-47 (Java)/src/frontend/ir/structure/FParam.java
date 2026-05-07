package frontend.ir.structure;

import frontend.ir.DataType;
import frontend.ir.Value;

import java.util.List;

public class FParam extends Value {
    private final int virtualReg;   // 表示存放这个参数值的虚拟寄存器号
    private final DataType dataType;
    private final List<Integer> limitList;
    
    public FParam(int virtualReg, DataType dataType, List<Integer> limitList) {
        this.virtualReg = virtualReg;
        this.dataType = dataType;
        this.pointerLevel = limitList.size();
        this.limitList = limitList;
    }
    
    @Override
    public String type2string() {
        if (this.pointerLevel > 0) {
            StringBuilder stringBuilder = new StringBuilder();
            int lim = limitList.size();
            for (int j = 1; j < lim; j++) {
                stringBuilder.append("[").append(limitList.get(j)).append(" x ");
            }
            stringBuilder.append(this.getDataType());
            for (int j = 1; j < lim; j++) {
                stringBuilder.append("]");
            }
            return stringBuilder.append("*").toString();
        } else {
            return this.getDataType().toString();
        }
    }
    
    @Override
    public Number getNumber() {
        return virtualReg;
    }
    
    @Override
    public DataType getDataType() {
        return dataType;
    }
    
    @Override
    public String value2string() {
        return "%reg_" + virtualReg;
    }
}
