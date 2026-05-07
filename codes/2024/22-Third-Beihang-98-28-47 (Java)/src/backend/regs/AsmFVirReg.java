package backend.regs;

public class AsmFVirReg extends AsmReg {
    private static int wholeIndex = 0;
    private int personalIndex = 0;
    public int color = -1;

    public AsmFVirReg() {
        personalIndex = wholeIndex;
        wholeIndex++;
        color = -1;
    }
    public  int getPersonalIndex() {
        return personalIndex;
    }
    public String toString() {
        return "fvir" + personalIndex+ " color: "+ color;
    }

}
