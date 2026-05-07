package frontend.semantic.symbol;

import backend.ir.entity.Declare;
import backend.ir.entity.Function;
import backend.ir.entity.Value;

public class FuncSymbol extends Symbol {
    public FuncSymbol(String name, IRBasicType type, Value value) {
        super(name, type, true, false);
        assert value instanceof Function || value instanceof Declare;
        this.value =value;
    }
}
