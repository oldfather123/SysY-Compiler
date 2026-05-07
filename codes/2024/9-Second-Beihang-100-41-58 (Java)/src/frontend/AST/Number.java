package frontend.AST;


import static frontend.Lexer.*;

//Number -> IntConst | FloatConst
public class Number implements PrimaryExp {
    private boolean isIntConst = false;
    private boolean isFloatConst = false;
    private int intConst = 0;
    private float floatConst = 0.0F;

    public Number(String number, int type) {
        if (type == DECFLOATCON || type == HEXFLOATCON) {
            isFloatConst = true;
            floatConst = Float.parseFloat(number);
            intConst = (int) floatConst;
        } else if (type == HEXCON || type == OCTCON || type == DECCON) {
            isIntConst = true;
            switch (type){
                case HEXCON -> intConst = Integer.parseInt(number.substring(2), 16);
                case OCTCON -> intConst = Integer.parseInt(number.substring(0), 8);
                case DECCON -> intConst = Integer.parseInt(number);
                default -> throw new AssertionError("number is wrong");
            }
            floatConst = (float) intConst;
        }
    }

    public int getIntConst() {
        return this.intConst;
    }

    public float getFloatConst() {
        return this.floatConst;
    }

    public boolean getIsInt() {
        return this.isIntConst;
    }

    public boolean getIsFloat() {
        return this.isFloatConst;
    }
}
