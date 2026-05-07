package Backend.operand;

public class FPhysicalReg extends AsmReg {
    private int index;
    private final String name;

    public FPhysicalReg(String name) {
        this.name = name;
        switch (name) {
            case "ft0" -> this.index = 0;
            case "ft1" -> this.index = 1;
            case "ft2" -> this.index = 2;
            case "ft3" -> this.index = 3;
            case "ft4" -> this.index = 4;
            case "ft5" -> this.index = 5;
            case "ft6" -> this.index = 6;
            case "ft7" -> this.index = 7;
            case "fs0" -> this.index = 8;
            case "fs1" -> this.index = 9;
            case "fa0" -> this.index = 10;
            case "fa1" -> this.index = 11;
            case "fa2" -> this.index = 12;
            case "fa3" -> this.index = 13;
            case "fa4" -> this.index = 14;
            case "fa5" -> this.index = 15;
            case "fa6" -> this.index = 16;
            case "fa7" -> this.index = 17;
            case "fs2" -> this.index = 18;
            case "fs3" -> this.index = 19;
            case "fs4" -> this.index = 20;
            case "fs5" -> this.index = 21;
            case "fs6" -> this.index = 22;
            case "fs7" -> this.index = 23;
            case "fs8" -> this.index = 24;
            case "fs9" -> this.index = 25;
            case "fs10" -> this.index = 26;
            case "fs11" -> this.index = 27;
            case "ft8" -> this.index = 28;
            case "ft9" -> this.index = 29;
            case "ft10" -> this.index = 30;
            case "ft11" -> this.index = 31;
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
