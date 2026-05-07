package ir.instr;

import ir.Value;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import tools.ErrOutput;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class BinaryInstr extends Instr {
    public InstType.BinaryType instType;
    public InstType.CmpType cmpType = InstType.CmpType.eq;
    public int combineAddcount = 1;
    public Value combineAddori = null;
    public Value combineOther = null;
    public Value combineChildren = null;

    /**
     * @param uses
     * @param type
     * @param name
     * @param instType uses中第一个对象为左操作数，第二个对象为右操作数，一定是value，否则会引起错误。
     */
    public BinaryInstr(LinkedList<Value> uses, Type type,
                       String name, InstType.BinaryType instType, BasicBlock parent) {
        super(uses, type, name, parent);
        this.instType = instType;
    }

    public String mapCmpToString() {
        switch (cmpType) {
            case eq -> {
                return "eq";
            }
            case ge -> {
                return "sge";
            }
            case gt -> {
                return "sgt";
            }
            case le -> {
                return "sle";
            }
            case lt -> {
                return "slt";
            }
            case ne -> {
                return "ne";
            }
            case oeq -> {
                return "oeq";
            }
            case oge -> {
                return "oge";
            }
            case ogt -> {
                return "ogt";
            }
            case ole -> {
                return "ole";
            }
            case olt -> {
                return "olt";
            }
            case one -> {
                return "one";
            }
        }
        return null;
    }

    public String mapInstTypeToString() {
        switch (instType) {
            case or -> {
                return "or";
            }
            case add -> {
                return "add";
            }
            case and -> {
                return "and";
            }
            case mul -> {
                return "mul";
            }
            case sub -> {
                return "sub";
            }
            case fadd -> {
                return "fadd";
            }
            case fcmp -> {
                return "fcmp " + mapCmpToString();
            }
            case fdiv -> {
                return "fdiv";
            }
            case fmul -> {
                return "fmul";
            }
            case fsub -> {
                return "fsub";
            }
            case icmp -> {
                return "icmp " + mapCmpToString();
            }
            case sdiv -> {
                return "sdiv";
            }
            case srem -> {
                return "srem";
            }
            case store -> {
                return "store";
            }
        }
        return null;
    }

    public ConstNumber computeConstResult() {
        return this.computeConstResult((ConstNumber) getOP(0), (ConstNumber) getOP(1));
    }

    public ConstNumber computeConstResult(ConstNumber value1, ConstNumber value2) {
        //        add,
        if (this.instType == InstType.BinaryType.add) {
            return new ConstNumber((Integer) value1.value + (Integer) value2.value);
        }
        //        fadd,
        else if (this.instType == InstType.BinaryType.fadd) {
            return new ConstNumber((Double) value1.value + (Double) value2.value);
        }
        //        sub,
        else if (this.instType == InstType.BinaryType.sub) {
            return new ConstNumber((Integer) value1.value - (Integer) value2.value);
        }
        //        fsub,
        else if (this.instType == InstType.BinaryType.fsub) {
            return new ConstNumber((Double) value1.value - (Double) value2.value);
        }
        //        mul,
        else if (this.instType == InstType.BinaryType.mul) {
            return new ConstNumber((Integer) value1.value * (Integer) value2.value);
        }
        //        fmul,
        else if (this.instType == InstType.BinaryType.fmul) {
            return new ConstNumber((Double) value1.value * (Double) value2.value);
        }
        //        sdiv,
        else if (this.instType == InstType.BinaryType.sdiv) {
            return new ConstNumber((Integer) value1.value / (Integer) value2.value);
        }
        //        fdiv,
        else if (this.instType == InstType.BinaryType.fdiv) {
            return new ConstNumber((Double) value1.value / (Double) value2.value);
        }
        //        srem,//就是mod
        else if (this.instType == InstType.BinaryType.srem) {
            return new ConstNumber((Integer) value1.value % (Integer) value2.value);
        }        //        //逻辑运算
        //        and,
        //        or,
        else if (this.instType == InstType.BinaryType.and ||
            this.instType == InstType.BinaryType.or) {
            ErrOutput.outputErr("and IR appear!while try to compute");
        }
        //
        //        //比较指令，具体比较类型交给CmpType定义
        //        icmp,
        else if (this.instType == InstType.BinaryType.icmp) {
            //        eq,
            if (this.cmpType == InstType.CmpType.eq) {
                return ((Integer) value1.value).equals((Integer) value2.value) ?
                    ConstNumber.getonei1() : ConstNumber.getzeroi1();
            }
            //        ne,
            else if (this.cmpType == InstType.CmpType.ne) {
                return ((Integer) value1.value).equals((Integer) value2.value) ?
                    ConstNumber.getzeroi1() : ConstNumber.getonei1();
            }
            //        gt,
            else if (this.cmpType == InstType.CmpType.gt) {
                return ((Integer) value1.value) > ((Integer) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        ge,
            else if (this.cmpType == InstType.CmpType.ge) {
                return ((Integer) value1.value) >= ((Integer) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        lt,
            else if (this.cmpType == InstType.CmpType.lt) {
                return ((Integer) value1.value) < ((Integer) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        le,
            else if (this.cmpType == InstType.CmpType.le) {
                return ((Integer) value1.value) <= ((Integer) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
        }
        //        fcmp,
        else if (this.instType == InstType.BinaryType.fcmp) {
            if (this.cmpType == InstType.CmpType.oeq) {
                return ((Double) value1.value).equals((Double) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        one,
            else if (this.cmpType == InstType.CmpType.one) {
                return ((Double) value1.value).equals((Double) value2.value) ? ConstNumber.getzeroi1() :
                    ConstNumber.getonei1();
            }
            //        ogt,
            else if (this.cmpType == InstType.CmpType.ogt) {
                return ((Double) value1.value) > ((Double) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        oge,
            else if (this.cmpType == InstType.CmpType.oge) {
                return ((Double) value1.value) >= ((Double) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        olt,
            else if (this.cmpType == InstType.CmpType.olt) {
                return ((Double) value1.value) < ((Double) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
            //        ole,
            else if (this.cmpType == InstType.CmpType.ole) {
                return ((Double) value1.value) <= ((Double) value2.value) ? ConstNumber.getonei1() :
                    ConstNumber.getzeroi1();
            }
        }
        ErrOutput.outputErr("can't match any compute branch");
        return null;
    }


    private String getRightType() {
        if (getOP(0) instanceof ConstNumber) {
            return getOP(1).getType().toString();
        }
        return getOP(0).getType().toString();
    }

    @Override
    public String toString() {
        return String.format("%s = %s %s %s, %s", getFullName(), mapInstTypeToString(),
            getRightType(), getOP(0).getFullName(), getOP(1).getFullName());
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for (int i = 0; i < this.getOperands().size(); i++) {
            Value temp = this.getOperands().get(i);
            if (temp instanceof ConstNumber) {
                uses.add(temp);
            } else {
                assert valueHashMap.containsKey(temp);
                uses.add(valueHashMap.get(temp));
            }
        }
        BinaryInstr binaryInstr = new BinaryInstr(uses, type, name, instType, basicBlock);
        binaryInstr.cmpType = cmpType;
        valueHashMap.put(this, binaryInstr);
        return binaryInstr;
    }
}
