package midend.value;

import midend.Type;

public class Argument extends Value {
    private final String originName; //符号表中存储的name

    public Argument(Type type, String name) {
        super(type, name);
        this.originName = name;
        setName("%f" + FR_NUM++);
    }

    public String getOriginName() {
        return this.originName;
    }

    public String toString() {
        return getType() + " " + getName();
    }
}
