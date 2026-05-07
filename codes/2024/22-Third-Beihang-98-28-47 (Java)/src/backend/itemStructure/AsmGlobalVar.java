package backend.itemStructure;

import frontend.ir.Value;
import frontend.ir.symbols.ArrayInitVal;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;
import java.util.List;

import static frontend.ir.DataType.INT;

public class AsmGlobalVar {
    public String name = null;
    private boolean isArray = false;
    private boolean isInit = false;
    private boolean isFloat = false;
    private int value = 0;
    private int size = 4;
    private Symbol arraySymbol = null;
    private StringBuilder sb = new StringBuilder();
    private static int lcCnt = 0;

    //已赋值数组
    public AsmGlobalVar(Symbol symbol) {
        this.name = symbol.getName() + "_val";
        this.isArray = true;
        this.isInit = true;
        this.arraySymbol = symbol;
        this.size = 4 * ((ArrayInitVal) symbol.getInitVal()).getLimSize();
    }

    //已赋值变量
    public AsmGlobalVar(String name, Number number) {
        this.name = name + "_val";
        this.isInit = true;
        this.value = number.intValue();
    }

    //未赋值变量
    public AsmGlobalVar(String name) {
        this.name = name + "_val";
    }

    //未赋值数组
    public AsmGlobalVar(String name, int size) {
        this.name = name + "_val";
        this.size = size;
    }

    //中途申请的浮点数
    public AsmGlobalVar(int floatvar) {
        this.name = ".LC" + lcCnt;
        lcCnt++;
        this.isInit = true;
        this.isFloat = true;
        this.value = floatvar;
    }

    public String toString() {
        if (!isFloat) {
            sb.append("\t.globl\t");
            sb.append(name);
            sb.append("\n");
        }

        if (isFloat) {
            sb.append("\t.section .rodata\n");
            sb.append("\t.align 3\n");
        } else if (isInit) {
            sb.append("\t.data\n");
        } else {
            sb.append("\t.bss\n");
        }

        if(!isFloat){
            sb.append("\t.type\t");
            sb.append(name);
            sb.append(",\t@object\n");
            sb.append("\t.size\t");
            sb.append(name);
            sb.append(",\t");
            sb.append(size);
            sb.append("\n");
        }
        sb.append(name);
        sb.append(":\n");
        if (isInit) {
            if (isArray) {
                Value initVal = arraySymbol.getInitVal();
                printArrayInit(initVal, ((ArrayInitVal) initVal).getLimSize());
            } else {
                sb.append("\t.word\t");
                sb.append(value);
                sb.append("\n");
            }
        } else {
            sb.append("\t.zero\t");
            sb.append(size);
            sb.append("\n");
        }
        return sb.toString();
    }

    public void printArrayInit(Value initVal, int limSize) {
        if (initVal instanceof ArrayInitVal) {
            ArrayInitVal arrayInitVal = (ArrayInitVal) initVal;
            List<Value> initList = arrayInitVal.getInitList();
            for (Value value : initList) {
                int limSizeNext = limSize / arrayInitVal.getDimSize(0);
                printArrayInit(value, limSizeNext);
            }
            int limSizeNext = limSize / arrayInitVal.getDimSize(0);
            if (arrayInitVal.getDimSize(0) > initList.size()) {
                int zeroSize = ((arrayInitVal.getDimSize(0) - initList.size()) * 4) * limSizeNext;
                sb.append("\t.zero\t");
                sb.append(zeroSize);
                sb.append("\n");
            }
        } else {
            Number number = initVal.getNumber();
            if (number != null) {
                if (initVal.getDataType() == INT) {
                    sb.append("\t.word\t");
                    sb.append(number.intValue());
                    sb.append("\n");
                } else {
                    sb.append("\t.word\t");
                    float f = number.floatValue();
                    int i = Float.floatToRawIntBits(f);
                    sb.append(i);
                    sb.append("\n");
                }
            }
        }
    }
}


