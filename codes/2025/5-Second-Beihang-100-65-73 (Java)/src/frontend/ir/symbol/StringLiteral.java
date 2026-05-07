package frontend.ir.symbol;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;

public class StringLiteral extends Value {
    private final String content;
    
    public StringLiteral(Type type, String content) {
        super(type);
        this.content = content;
    }
    
    @Override
    public String toString() {
        return this.value2string();
    }
    
    @Override
    public String value2string() {
        return "private unnamed_addr constant " + this.getType().printIRType() + " c\"" + content + "\"";
    }
    
    public String getContent() {
        return content;
    }
}
