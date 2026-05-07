package IR.Value.Instructions;

public enum OP {
    Add,
    Fadd,
    Sub,
    Fsub,
    Mul,
    Fmul,
    Div,
    Fdiv,
    Mod,
    Fmod,
    Shl,
    Shr,
    And,
    Or,
    Xor,
    Lt,
    FLt,
    Le,
    FLe,
    Ge,
    FGe,
    Gt,
    FGt,
    Eq,
    FEq,
    Ne,
    FNe,
    //conversion op
    Zext,
    Ftoi,
    Itof,
    //Mem
    Alloca,
    Load,
    Store,
    GEP, //get element ptr
    Phi,
    //terminator op
    Br,
    Call,
    Ret,
    //not op
    Not,
    Move,
    BitCast;

    public boolean isCmpOP(){
        String name = name();
        return switch (name) {
            case "Ne", "FNe", "Eq", "FEq", "Lt", "Le", "FLt", "Gt", "FLe", "FGt", "Ge", "FGe" -> true;
            default -> false;
        };
    }

    public boolean isFloat(){
        String name = name();
        return switch (name) {
            case "Fadd", "FNe", "Fsub", "FEq", "Fmul", "Fdiv", "FLt", "Fmod", "FLe", "FGt", "FGe" -> true;
            default -> false;
        };
    }

    public static OP turnToFloat(OP op){
        return switch (op){
            case Add, Fadd -> OP.Fadd;
            case Sub, Fsub -> OP.Fsub;
            case Mul, Fmul -> OP.Fmul;
            case Div, Fdiv -> OP.Fdiv;
            case Mod, Fmod -> OP.Fmod;
            case Lt, FLt -> OP.FLt;
            case Le, FLe -> OP.FLe;
            case Ge, FGe -> OP.FGe;
            case Gt, FGt -> OP.FGt;
            case Eq, FEq -> OP.FEq;
            case Ne, FNe -> OP.FNe;
            default -> op;
        };
    }

    public static OP str2op(String str){
        return switch (str){
            case "+" -> OP.Add;
            case "+f" -> OP.Fadd;
            case "-" -> OP.Sub;
            case "-f" -> OP.Fsub;
            case "*" -> OP.Mul;
            case "*f" -> OP.Fmul;
            case "/" -> OP.Div;
            case "/f" -> OP.Fdiv;
            case "%" -> OP.Mod;
            case "%f" -> OP.Fmod;
            case "<=" -> OP.Le;
            case "<=f" -> OP.FLe;
            case "<" -> OP.Lt;
            case "<f" -> OP.FLt;
            case ">=" -> OP.Ge;
            case ">=f" -> OP.FGe;
            case ">" -> OP.Gt;
            case ">f" -> OP.FGt;
            case "==" -> OP.Eq;
            case "==f" -> OP.FEq;
            case "!=" -> OP.Ne;
            case "!=f" -> OP.FNe;
            case "&&" -> OP.And;
            case "||" -> OP.Or;
            case "ftoi" -> OP.Ftoi;
            case "itof" -> OP.Itof;
            case "zext" -> OP.Zext;
            case "bitcast" -> OP.BitCast;
            default -> null;
        };
    }

    @Override
    public String toString(){
        String name = name();
        return switch (name) {
            case "Add" -> "add";
            case "Fadd" -> "fadd";
            case "Sub" -> "sub";
            case "Fsub" -> "fsub";
            case "Mul" -> "mul";
            case "Fmul" -> "fmul";
            case "Div" -> "sdiv";
            case "Fdiv" -> "fdiv";
            case "Mod" -> "srem";
            case "Fmod" -> "frem";
            case "Ne" -> "icmp ne";
            case "FNe" -> "fcmp one";
            case "Eq" -> "icmp eq";
            case "FEq" -> "fcmp oeq";
            case "Lt" -> "icmp slt";
            case "FLt" -> "fcmp olt";
            case "Le" -> "icmp sle";
            case "FLe" -> "fcmp ole";
            case "Gt" -> "icmp sgt";
            case "FGt" -> "fcmp ogt";
            case "Ge" -> "icmp sge";
            case "FGe" -> "fcmp oge";

            case "Ftoi" -> "fptosi";
            case "Itof" -> "sitofp";
            case "Zext" -> "zext";
            case "BitCast" -> "bitcast";
            case "Move" -> "move";
            default -> null;
        };
    }
}

