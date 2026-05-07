package ir.value;

import ir.Value;
import ir.type.Type;

public class Arg extends Value {
    public Arg(String name, Type type){
        super(type, name);
    }
}
