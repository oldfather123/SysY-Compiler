package Backend.operand;

public class AsmOperand {
    private boolean needsColor = true;
    private int color = -1;
    private int spillPlace = -1;

    public boolean isPreColored() {
        return needsColor;
    }

    public int getColor() {
        return color;
    }

    public int getSpillPlace() {
        return spillPlace;
    }

    public void setColor(int color) {
        this.color = color;
    }

    public void setSpillPlace(int spillPlace) {
        this.spillPlace = spillPlace;
    }

    public void notNeedsColor() {
        needsColor = false;
    }
}