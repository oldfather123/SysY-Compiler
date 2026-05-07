package ir.instr;

//只是一个装enum的容器，没有实际意义，不需要实现
//分为二元指令、控制流指令、一元指令、访问地址计算指令
public interface InstType {
    //二元指令类型
    public enum BinaryType {
        //算术运算: add,fadd,sub,fsub,mul,fmul,sdiv,fdiv
        //逻辑运算: and,or
        //比较运算: eq,ne,gt,ge,lt,le
        //空间分配: alloca,load,store
        //函数调用: call
        //跳转相关: br,ret
        //比较跳转: icmp
        //扩展相关: zext(zero extension)
        //数组指针: getelementptr
        //浮点整数强转:fptosi,sitofp
        //phi instruction

        //计算
        add,
        fadd,
        sub,
        fsub,
        mul,
        fmul,
        sdiv,
        fdiv,
        srem,//就是mod

        //比较指令，具体比较类型交给CmpType定义
        icmp,
        fcmp,

        //逻辑运算
        and,
        or,

        //仍然认为是二元指令，因为用到值和目标指针
        store,
    }

    public enum SingleType{
        alloca,
        load,
        zext,
        sitofp,
        fptosi,
    }

    public enum AccessType{
        getelementptr,
    }
    //控制流特殊类型
    public enum FlowType{
        call,
        br,
        ret,
        phi,
    }

    public enum CmpType {
        eq,
        ne,
        gt,
        ge,
        lt,
        le,
        oeq,
        one,
        ogt,
        oge,
        olt,
        ole,
    }
}
