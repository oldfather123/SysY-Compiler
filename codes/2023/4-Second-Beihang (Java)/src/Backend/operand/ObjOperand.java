package Backend.operand;

public abstract class ObjOperand {
    boolean needsColor = true;
    public int color=-1;
    public int spillPlace=-1;
    public boolean isPrecolored() { return !needsColor; }
    public boolean needsColor() {
        return needsColor;
    }
    public void notNeedsColor() {
        needsColor = false;
    }
    public boolean isAllocated() { return false; }
}
