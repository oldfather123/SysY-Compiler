package frontend.ir.constant;

import frontend.ir.objecttype.arithmetic.TChar;

public class CharConst extends IRConst {
    private final int ascii;
    public static final CharConst Zero = new CharConst(0);
    
    public CharConst(int ascii) {
        super(new TChar());
        this.ascii = ascii & 0xff;
    }
    
    public int getAscii() {
        return ascii;
    }
    
    @Override
    public String value2string() {
        return String.valueOf(ascii);
    }
    
    @Override
    public String toString() {
        return "i8 " + ascii;
    }
    
    @Override
    public Number getConstVal() {
        return ascii;
    }

    @Override
    public CharConst clone() {
        return new CharConst(ascii);
    }
}
