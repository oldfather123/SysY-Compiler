package ir.value;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.InstType;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.Type;
import tools.ErrOutput;

public class ConstNumber extends Value {
    public Object value;

    public ConstNumber(Type type, String name) {
        super(type, name);
    }

    public ConstNumber(Integer value){
        this(new IntType(), value.toString());
        this.value = value;
    }

    public static ConstNumber getzeroi1(){
        ConstNumber ret =  new ConstNumber(0);
        ret.type = new IntType(1);
        return ret;
    }

    public static ConstNumber getonei1(){
        ConstNumber ret = new ConstNumber(1);
        ret.type = new IntType(1);
        return ret;
    }


    public ConstNumber(Double value){
        this(new FloatType(), value.toString());
        this.value = value;
    }


    @Override
    public String getFullName() {
        if(type instanceof FloatType) {
            return String.format("%.30f",((Double)value).floatValue());
            //return "0x" + Integer.toUnsignedString(Float.floatToIntBits(((Double)value).floatValue()), 16);
            //return "0x" + Long.toUnsignedString(Double.doubleToLongBits((double) value), 16);
        }
        return value.toString();
    }

    @Override
    public String getNameWithType() {
        return super.getNameWithType();
    }

    public ConstNumber computeWithConstNumber(ConstNumber constNumber, InstType.BinaryType type, InstType.BinaryType thisType,
                                              int pos, int thispos, BinaryInstr instr){
        if(!(this.value instanceof Integer && constNumber.value instanceof Integer)){
            ErrOutput.outputErr("error: tryto Compute Float in ComputeWithConstNumber");
            return null;
        }
        ConstNumber result = new ConstNumber((Integer)this.value);
        if(type == InstType.BinaryType.add){
            if(thisType == InstType.BinaryType.add)
                result.value = (Integer)result.value + (Integer) constNumber.value;
            else if(thisType == InstType.BinaryType.sub)//(x+y)
                if(thispos == 0){
                    //a-(b+x) / a-(x+b)
                    result.value = (Integer)result.value - (Integer) constNumber.value;
                }else if(thispos == 1){
                    //(b+x)-a
                    //-->x-(a-b)
                    result.value =  (Integer)result.value - (Integer) constNumber.value;
                }
        }else if(type == InstType.BinaryType.sub){
            if(thisType == InstType.BinaryType.sub) {
                if (thispos == 0) {
                    //a-(b-x) / a-(x-b)
                    if (pos == 0) {
                        //a-(b-x)
                        //-->x-(b-a)
                        var tmp = instr.getOP(0);
                        instr.setOperand(0, instr.getOP(1));
                        instr.setOperand(1, tmp);
                        result.value = (Integer) constNumber.value - (Integer) result.value;
                    } else if (pos == 1) {
                        //a-(x-b)
                        //-->(a+b)-x
                        result.value = (Integer) constNumber.value + (Integer) result.value;
                    }
                } else if (thispos == 1) {
                    //(b-x)-a  /  (x-b)-a
                    if (pos == 0) {
                        //(b-x) - a
                        //-->(b-a)-x
                        var tmp = instr.getOP(0);
                        instr.setOperand(0, instr.getOP(1));
                        instr.setOperand(1, tmp);
                        result.value = (Integer) constNumber.value - (Integer) result.value;
                    } else if (pos == 1) {
                        //(x-b)-a
                        //-->x-(b+a)
                        result.value = (Integer) result.value + (Integer) constNumber.value;
                    }
                }
            }else if(thisType == InstType.BinaryType.add) {
                if (pos == 0) {
                    //a+(b-x) // (b-x)+a
                    result.value = (Integer) result.value + (Integer) constNumber.value;
                    instr.instType = InstType.BinaryType.sub;
                    if(thispos == 1){
                        var tmp = instr.getOP(0);
                        instr.setOperand(0, instr.getOP(1));
                        instr.setOperand(1, tmp);
                    }
                } else if (pos == 1) {
                    //a+(x-b)
                    result.value = (Integer) result.value - (Integer) constNumber.value;
                }
            }
        }else if(type == InstType.BinaryType.sdiv||type == InstType.BinaryType.mul){
            result.value = (Integer)result.value*(Integer)constNumber.value;
        }else{
            ErrOutput.outputErr("error: tryto compute constNumber without add/sub/mul/sdiv");
            return null;
        }
        return result;
    }
}
