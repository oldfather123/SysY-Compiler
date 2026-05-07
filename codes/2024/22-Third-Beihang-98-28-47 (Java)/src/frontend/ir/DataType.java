package frontend.ir;

public enum DataType {
    INT("i32"),
    FLOAT("float"),
    VOID("void"),
    VOID_("i8*"),  // void*, 通用的指针
    BOOL("i1");   // 1 位整数
    
    private final String name;
    
    DataType(String name) {
        this.name = name;
    }
    
    @Override
    public String toString() {
        return this.name;
    }
}


