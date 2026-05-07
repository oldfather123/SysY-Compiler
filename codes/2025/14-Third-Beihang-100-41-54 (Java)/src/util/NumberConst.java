package util;

import java.nio.ByteBuffer;

public class NumberConst {
    public static int parseInt(String str) {
        if (str == null || str.isEmpty())
            throw new NumberFormatException("Empty string");
        str = str.trim();
        if (str.startsWith("0x") || str.startsWith("0X"))
            return Integer.parseInt(str.substring(2), 16);
        else if (str.startsWith("0") && str.length() > 1)
            return Integer.parseInt(str.substring(1), 8);
        else
            return Integer.parseInt(str, 10);
    }

    public static int decimalToHexBinary(String str) {
        if (str == null || str.isEmpty())
            throw new NumberFormatException("Empty string");
        str = str.trim();
        try {
            float decimalValue = Float.parseFloat(str);
            return Float.floatToRawIntBits(decimalValue);
        } catch (NumberFormatException e) {
            throw new NumberFormatException("Invalid decimal string: " + str);
        }
    }
}
