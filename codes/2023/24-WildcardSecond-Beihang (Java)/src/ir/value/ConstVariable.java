package ir.value;

import ir.type.Type;


public class ConstVariable extends Variable{
    public boolean isInit;

    public ConstVariable(User user, Type type, String name, boolean isInit) {
        super(user, type, name, isInit);
        this.isInit = isInit;
    }
}
