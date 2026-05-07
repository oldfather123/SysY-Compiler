package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

/**
 * &#064;Classname LiteralConst
 * &#064;Description  TODO
 * &#064;Date 2025/7/13 15:39
 * &#064;Created MuJue
 */
public class LiteralConst extends Value{
    private final Number number;
    private final String stringValue;
    private boolean defined = true;//表示这个常数是testfile中本身就有的，还是phi指令生成出来的
    public LiteralConst(Number number, IRBasicType IRBasicType){
        super(number.toString(), IRBasicType, true, false, null);
        this.number = number;
        stringValue = "";
    }
    public LiteralConst(String str){
        super(str, IRBasicType.I8, true, true, new ArrayList<>());
        stringValue = str;
        number = null;
    }

    public LiteralConst(Number number, IRBasicType IRBasicType, boolean defined){
        super(number.toString(), IRBasicType, true, false, null);
        this.number = number;
        stringValue = "";
        this.defined = defined;
    }

    public Integer getIntValue() {
        return number.intValue();
    }

    public Float getFloatValue() {
        assert isFloat();
        return number.floatValue();
    }

    public Number getNumber() {
        return number;
    }

    public String getStringValue() {
        return stringValue;
    }

    /**
     * 将 double 转换成 LLVM IR 接受的 64 位十六进制常量格式字符串，
     * 例如输入 Math.PI 输出 "0x400921fb54442d18"
     */
    public static String doubleConstToLLVMHex(double value) {
        long bits = Double.doubleToRawLongBits(value);
        return String.format("0x%016x", bits);
    }
    public static String formatDouble(double value) {
        // %e 代表科学计数法，小数点后6位，格式示例：2.000000e+00
        return String.format("%1.6e", value);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        String typeStr = getTypeStr();
        if(isInt() || isBoolean() || isChar() && !isString()){
            fout.write(typeStr + " " +  number.intValue());
        }else if(isFloat()){
            fout.write(typeStr + " " + doubleConstToLLVMHex(number.floatValue()));
        }
        else if(isString()){
            int len = stringValue.length() - 4;
            fout.write("[" + len + " x i8] c" + stringValue);
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        if(isInt() || isBoolean() || isChar()){
            fout.write(Integer.toString(number.intValue()));
        }else if(isFloat()){
            fout.write(doubleConstToLLVMHex(number.floatValue()));
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    public boolean isDefined() {
        return defined;
    }

    public void setDefined(boolean defined) {
        this.defined = defined;
    }
}
