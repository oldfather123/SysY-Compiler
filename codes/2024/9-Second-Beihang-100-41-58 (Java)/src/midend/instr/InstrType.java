package midend.instr;

public enum InstrType {
    ADD, //加法 <result> = add <ty> <op1>, <op2>
    FADD,
    SUB, //减法 类ADD
    FSUB,
    MUL, //乘法 类ADD
    FMUL,
    SDIV, //有符号除法 类ADD
    FDIV,
    SHL, //逻辑左移
    LSHR, //逻辑右移
    ASHR, //算数右移
    ICMP, //比较指令 <result> = icmp <cond> <ty> <op1>, <op2>
    FCMP,
    AND, //与 类ADD
    MOD,
    FMOD,
    OR, //或 类ADD
    CALL, //函数调用 <result> = call [ret attrs] <ty> <fnptrval>(<function args>)
    ALLOCA, //分配内存 <result> = alloca <type>
    LOAD, //读取内存 <result> = load <ty>, <ty>* <pointer>
    STORE, //写内存 store <ty> <value>, <ty>* <pointer>
    GETELEMENTPTR, //目标元素位置
    PHI, //<result> = phi [fast-math-flags] <ty> [<val0>, <label0>]
    ZEXT, //<result> = zext <ty> <value> to <ty2> 将ty的value的type转为ty2
    TRUNC, //缩减为type
    FPTOSI, //浮点数转整型
    SITOFP, //整型转浮点数
    BR, //br i1 <cond>, label <iftrue>, label <iffalse> | br label <dest> 跳转
    RET, //ret <type> <value> | ret void
    BITCAST,
    PCOPY,
    MOVE;

    public InstrType toFloat() {
        if (this == ADD || this == FADD) {
            return FADD;
        } else if (this == SUB || this == FSUB) {
            return FSUB;
        } else if (this == MUL || this == FMUL) {
            return FMUL;
        } else if (this == SDIV || this == FDIV) {
            return FDIV;
        } else if (this == AND) {
            return null;
        } else if (this == MOD || this == FMOD) {
            return FMOD;
        } else if (this == ICMP || this == FCMP) {
            return FCMP;
        } else if (this == OR) {
            return null;
        } else if (this == CALL) {
            return null;
        } else if (this == ALLOCA) {
            return null;
        } else if (this == LOAD) {
            return null;
        } else if (this == STORE) {
            return null;
        } else if (this == GETELEMENTPTR) {
            return null;
        } else if (this == PHI) {
            return null;
        } else if (this == ZEXT) {
            return null;
        } else if (this == TRUNC) {
            return null;
        } else if (this == FPTOSI) {
            return null;
        } else if (this == SITOFP) {
            return null;
        } else if (this == BR) {
            return null;
        } else if (this == RET) {
            return null;
        } else if (this == BITCAST) {
            return null;
        } else if (this == PCOPY) {
            return null;
        } else if (this == MOVE) {
            return null;
        }
        return null; // 默认返回null
    }


    public static InstrType strToInstr(String str) {
        if (str.equals("+")) {
            return ADD;
        } else if (str.equals("-")) {
            return SUB;
        } else if (str.equals("*")) {
            return MUL;
        } else if (str.equals("/")) {
            return SDIV;
        } else if (str.equals("%")) {
            return MOD;
        } else if (str.equals("<") || str.equals("<=") || str.equals("==") ||
                str.equals("!=") || str.equals(">") || str.equals(">=")) {
            return ICMP;
        } else {
            throw new IllegalStateException("Unexpected value: " + str);
        }
    }


    public String toString() {
        if (this == ADD) {
            return "add";
        } else if (this == FADD) {
            return "fadd";
        } else if (this == SUB) {
            return "sub";
        } else if (this == FSUB) {
            return "fsub";
        } else if (this == MUL) {
            return "mul";
        } else if (this == FMUL) {
            return "fmul";
        } else if (this == SDIV) {
            return "sdiv";
        } else if (this == FDIV) {
            return "fdiv";
        } else if (this == ICMP) {
            return "icmp";
        } else if (this == FCMP) {
            return "fcmp";
        } else if (this == AND) {
            return "and";
        } else if (this == MOD) {
            return "srem";
        } else if (this == FMOD) {
            return "frem";
        } else if (this == OR) {
            return "or";
        } else if (this == CALL) {
            return "call";
        } else if (this == ALLOCA) {
            return "alloca";
        } else if (this == LOAD) {
            return "load";
        } else if (this == STORE) {
            return "store";
        } else if (this == GETELEMENTPTR) {
            return "getelementptr";
        } else if (this == PHI) {
            return "phi";
        } else if (this == ZEXT) {
            return "zext";
        } else if (this == TRUNC) {
            return "trunc";
        } else if (this == FPTOSI) {
            return "fptosi";
        } else if (this == SITOFP) {
            return "sitofp";
        } else if (this == BR) {
            return "br";
        } else if (this == RET) {
            return "ret";
        } else if (this == BITCAST) {
            return "bitcast";
        } else if (this == PCOPY) {
            return "pcopy";
        } else if (this == MOVE) {
            return "move";
        } else if (this == SHL) {
            return "shl";
        } else if (this == LSHR) {
            return "lshr";
        } else if (this == ASHR) {
            return "ashr";
        }
        return null; // 默认返回null，虽然在此情况下不会被触发
    }

}
