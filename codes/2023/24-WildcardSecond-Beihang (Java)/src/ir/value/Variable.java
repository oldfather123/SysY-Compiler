package ir.value;

import ir.Value;
import ir.instr.Instr;
import ir.type.ArrayType;
import ir.type.FloatType;
import ir.type.Type;
import tools.ErrOutput;

import java.util.ArrayList;
import java.util.LinkedHashMap;

public class Variable extends Value {
    //由于非const全局变量仍然需要自动生成global声明指令，故用value是否经过初始化来判断是否生成带有初始化的全局变量，
    //并且将value和construct提升得到Variable类中
    public Object value;
    public boolean isInit;
    public Instr allocInst;
    //服务于computeNow情况下的计算，记录数组情况下当前初始化的位置，
    // 目前暂时放到variable下，之后如果想继续抽象出一个类也可以
    public ArrayList<Integer> currentInitPos;
    private int decLevel;//记录了明文表示的括号层级

    //如a[2][2]={1,2,2}的结果是{{1,2},{2,0}}
    //但是a[2][2]={{1,2,3}}出错
    // 对于第一个，decLevel==1，则可以通过next回到第一层，并设置通过preparePos进入第二个第二层。
    // 对于第二个，~中层级为2，则next后如果回到第一层则当前层级小于明文层级，出错。
    // 随着解析大括号的posIn和posOut维护这个层级。
    public Variable(User user, Type type, String name, boolean isInit) {
        super(user, type, name);
        this.isInit = isInit;
        value = null;
        decLevel = 0;
    }

    public void construct() {
        if (type instanceof ArrayType) {
            value = ((ArrayType) type).constructContainer();
        } else if (type instanceof FloatType) {
            //自动包装为Float
            value = (double) 0;
        } else {
            value = 0;
        }
    }

    public Variable(User user, Type type, String name) {
        this(user, type, name, false);
    }

    public void initCurrentInitPos() {
        decLevel = 1;
        if (!(type instanceof ArrayType)) {
            ErrOutput.outputErr("error: trying to init non-array variable with array");
        } else {
            currentInitPos = new ArrayList<>();
            currentInitPos.add(-1);
        }
    }

    public ArrayList<Object> getArrayListByPos() {
        ArrayList<Object> arrayList = (ArrayList<Object>) value;
        for (var i : currentInitPos.subList(0, currentInitPos.size() - 1)) {
            if (arrayList.get(i) instanceof ArrayList<?>) {
                arrayList = (ArrayList<Object>) arrayList.get(i);
            } else {
                ErrOutput.outputErr("error: access wrong dimension");
            }
        }
        return arrayList;
    }

    public ArrayList<Object> getArrayListByPos(ArrayList<Integer> pos) {
        this.currentInitPos = pos;
        ArrayList<Object> ret = getArrayListByPos();
        this.currentInitPos = null;
        return ret;
    }

    public void posIn() {
        if (decLevel == 0) {
            initCurrentInitPos();
        } else {
            currentInitPos.add(0);
            if(!((ArrayType) type).isValidPos(currentInitPos)){
                while(!((ArrayType) type).isValidPos(currentInitPos)){
                    currentInitPos.remove(currentInitPos.size() - 1);
                    if(currentInitPos.isEmpty()){
                        ErrOutput.outputErr("error: wrong currentpos");
                    }
                    currentInitPos.set(currentInitPos.size()-1,
                        currentInitPos.get(currentInitPos.size()-1)+1);
                }
                currentInitPos.add(0);
            }
            currentInitPos.remove(currentInitPos.size()-1);

            decLevel++;
            if (!(type instanceof ArrayType)) {
                ErrOutput.outputErr("error: can't posIn");
            } else {
                boolean valid = ((ArrayType) type).isValidPos(currentInitPos);
                if (!valid) {
                    ErrOutput.outputErr("error: can't posIn");
                }
                currentInitPos.add(-1);
            }
        }

    }

    public void posOut() {
        decLevel--;
        while (currentInitPos.size() > decLevel) {
            currentInitPos.remove(currentInitPos.size() - 1);
        }
        if (decLevel == 0) {
            currentInitPos = null;
        }
    }

    public int getDecLevel() {
        return decLevel;
    }

    public void posNext() {
        if (!(type instanceof ArrayType) || currentInitPos == null) {
            return;
        }
        currentInitPos.set(
            currentInitPos.size() - 1,
            currentInitPos.get(currentInitPos.size() - 1) + 1
        );

        boolean valid = ((ArrayType) type).isValidPos(currentInitPos);
        while (!valid) {
            currentInitPos.remove(currentInitPos.size() - 1);
            if (currentInitPos.size() < decLevel) {
                ErrOutput.outputErr("error: out of index");
            }
            currentInitPos.set(
                currentInitPos.size() - 1,
                currentInitPos.get(currentInitPos.size() - 1) + 1
            );
            valid = ((ArrayType) type).isValidPos(currentInitPos);
        }

    }

    public void preparePos() {
        if (!(type instanceof ArrayType)) {
            ErrOutput.outputErr("error: can't preparePos");
        } else {
            while (((ArrayType) type).isValidPos(currentInitPos)) {
                currentInitPos.add(0);
            }
            currentInitPos.remove(currentInitPos.size() - 1);
        }
    }

    public void saveValue(int v) {
        if (!(type instanceof ArrayType) && value instanceof Integer) {
            value = v;
        } else if (type instanceof ArrayType) {
            preparePos();
            ArrayList<Object> targetlist = getArrayListByPos();
            targetlist.add((Integer)v);
        } else {
            ErrOutput.outputErr("error: init saving int into un-Integer variable value");

        }
    }

    public void saveValue(double v) {
        if (!(type instanceof ArrayType) && value instanceof Double) {
            value = v;
        } else if (type instanceof ArrayType) {
            preparePos();
            ArrayList<Object> targetlist = getArrayListByPos();
            targetlist.add(v);
        } else {
            ErrOutput.outputErr("error: init saving double into un-Double variable value");
        }
    }

    public void saveValue(Object container) {
        if (container instanceof Double) {
            saveValue((double) container);
        } else if (container instanceof Integer) {
            saveValue((int) container);
        } else {
            ErrOutput.outputErr("error: saving container " +
                "which is not a double or a int into the variable value");
        }
    }

    public Object clone(User user) throws CloneNotSupportedException {
        Variable clone = new Variable(user, this.type, this.name);
        clone.isInit = this.isInit;
        return clone;
    }

    public void afterclone(LinkedHashMap<Value, Value> valueMap){
        this.allocInst = (Instr) valueMap.get(this.allocInst);
    }


}
