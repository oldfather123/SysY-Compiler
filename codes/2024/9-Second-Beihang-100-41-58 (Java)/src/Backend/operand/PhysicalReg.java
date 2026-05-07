package Backend.operand;

public class PhysicalReg extends AsmReg {
    private int index;
    private final String name;

    public PhysicalReg(String name) {
        this.name = name;
        switch (name) {
            case "zero" -> this.index = 0;
            case "ra" -> this.index = 1;
            case "sp" -> this.index = 2;
            case "gp" -> this.index = 3;
            case "tp" -> this.index = 4;
            case "t0" -> this.index = 5;
            case "t1" -> this.index = 6;
            case "t2" -> this.index = 7;
            case "s0" -> this.index = 8;
            case "s1" -> this.index = 9;
            case "a0" -> this.index = 10;
            case "a1" -> this.index = 11;
            case "a2" -> this.index = 12;
            case "a3" -> this.index = 13;
            case "a4" -> this.index = 14;
            case "a5" -> this.index = 15;
            case "a6" -> this.index = 16;
            case "a7" -> this.index = 17;
            case "s2" -> this.index = 18;
            case "s3" -> this.index = 19;
            case "s4" -> this.index = 20;
            case "s5" -> this.index = 21;
            case "s6" -> this.index = 22;
            case "s7" -> this.index = 23;
            case "s8" -> this.index = 24;
            case "s9" -> this.index = 25;
            case "s10" -> this.index = 26;
            case "s11" -> this.index = 27;
            case "t3" -> this.index = 28;
            case "t4" -> this.index = 29;
            case "t5" -> this.index = 30;
            case "t6" -> this.index = 31;
        }
        setColor(this.index);
    }

    public int getIndex() {
        return index;
    }

    public boolean isSReg() {
        return index == 8 || index == 9 || (index >= 18 && index <= 27);
    }

    @Override
    public String toString() {
        return name;
    }
}
