package backend.component;

import com.sun.source.tree.Tree;
import ir.type.IrType;

import java.util.ArrayList;
import java.util.TreeMap;

public class ObjGlobalVar {
    private String name;
    private String typeName;
    private IrType type;
    private int size;
    private int value;
    private TreeMap<Integer, Integer> elements;//已初始化的数值变量或数组
    private int arrayLength;
    private String string;//初始化的字符串
    public static int floatIndex = 0;
    private static int intIndex = 0;
    private static int arrayIndex = 0;
    private boolean isInit = false;
    private boolean isConst = false;//是否是常量，用于浮点立即数

    public ObjGlobalVar(String name, int size, int value, IrType type) {
        this.name = name;
        this.size = size;
        this.value = value;
        this.type = type;
    }

//    public ObjGlobalVar(String name, IrType type, ArrayList<Integer> elements) {
//        this.name = name;
//        this.size = 4 * elements.size();
//        this.elements = elements;
//    }

    public ObjGlobalVar(String name, int size, IrType type) {
        this.name = name;
        this.size = size;
        this.type = type;
    }

    public ObjGlobalVar(int value, boolean isInit) {
        name = "INT" + intIndex;
        intIndex++;
        this.value = value;
        size = 4;
        typeName = "int";
        this.isInit = isInit;
    }

    public ObjGlobalVar(float floatValue, boolean isInit) {
        //创建浮点立即数时，会调用此方法
        value = Float.floatToRawIntBits(floatValue);
        size = 8;
        name = "FLOAT" + floatIndex;
        floatIndex++;
        this.isInit = isInit;
        typeName = "float";
    }

    public ObjGlobalVar(TreeMap<Integer, Integer> elements, boolean isInit, int size, int arrayLength) {
        //数组已经展开成一维
        name = "array" + arrayIndex;
        arrayIndex++;
        this.elements = elements;
        this.isInit = isInit;
        //this.isInit=true;//数组默认初始化
        this.size = size;
        typeName = "array";
        this.arrayLength = arrayLength;
    }

    public void setConst(boolean isConst) {
        this.isConst = isConst;
    }

    public void setSize(int size) {
        this.size = size;
    }

//    public void init(ArrayList<Integer> elements) {
//        this.elements = elements;
//    }

    public void init(String string) {
        this.string = string;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        //SysY中没有字符串？
        String string = "";
        if (isConst) {
            //浮点立即数
            string += "\t.section .rodata\n\t.align 3\n";
        } else {
            string += "\t.global\t" + name + "\n";
            if (isInit) {
                string += ".data\n";
            } else {
                string += ".bss\n";
            }
            string += "\t.type\t" + name + ",\t" + "@object\n";
            string += "\t.size\t" + name + ",\t" + size + "\n";
        }
        string += name + ":\n";

        if (!isInit) {
            string += "\t.zero\t" + size + "\n";
        } else {
            if (typeName.equals("float") || typeName.equals("int")) {
                string += "\t.word\t" + value + "\n";
            } else {
                int pos = 0;
                for (Integer i : elements.keySet()) {
                    if (i != pos) {
                        string += "\t.zero\t" + (i - pos) * 4 + "\n";
                        pos = i + 1;
                    } else {
                        pos++;
                    }
                    string += "\t.word\t" + elements.get(i) + "\n";
                }
                if (pos != arrayLength) {
                    string += "\t.zero\t" + (arrayLength - pos) * 4 + "\n";
                }
            }
        }
        return string;
    }
}
