package tools;

import ir.Value;
import ir.value.Variable;

import java.util.ArrayList;
import java.util.LinkedHashMap;

public class ValueStack {
    public ArrayList<LinkedHashMap<String, Value>> stack = new ArrayList<>();

    public LinkedHashMap<String, Value> pop() {
        return stack.remove(stack.size() - 1);
    }

    public void push(LinkedHashMap<String, Value> values) {
        stack.add(values);
    }

    //往栈的最顶层加入Value 也就是当前模块加Value
    public void addValue(String name, Value value) {
        stack.get(stack.size() - 1).put(name, value);
    }

    public boolean isEmpty() {
        return stack.size() == 0;
    }

    //拥有局部变量
    public boolean containsLocalValue(String str) {
        return stack.get(stack.size() - 1).containsKey(str);
    }

    //拥有
    public boolean containsKey(String str) {
        boolean flag = false;
        int i = stack.size() - 1;
        while (i >= 0 && !flag) {
            flag = stack.get(i).containsKey(str);
            i--;
        }
        return flag;
    }

    //可以通过使用String 从上到下 也就是从局部到全局寻找变量
    public Value get(String str) {
        boolean flag = false;
        int i = stack.size() - 1;
        while (i >= 0 && !flag) {
            if(stack.get(i).containsKey(str)) {
                return stack.get(i).get(str);
            }
            i--;
        }
        return null;
    }

    public int getKeyPriority(String str){ //越高优先级越高
        int ret = -1;
        for(int i = 0;i<stack.size();i++){
            if(stack.get(i).containsKey(str)){
                ret = i;
            }
        }
        return ret;
    }
}
