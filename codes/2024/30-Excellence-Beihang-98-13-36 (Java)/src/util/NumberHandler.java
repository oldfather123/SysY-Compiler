package util;

public class NumberHandler {

    public static int getInteger(String intStr) {
        if (intStr.length() >= 2 && intStr.charAt(0) == '0' && (intStr.charAt(1) == 'x' || intStr.charAt(1) == 'X')) {
            return Integer.parseInt(intStr.substring(2), 16);
        } else if (intStr.length() >= 2 && intStr.charAt(0) == '0') {
            return Integer.parseInt(intStr.substring(1), 8);
        }
        return Integer.parseInt(intStr);
    }

    public static float getFloat(String floatStr) {
        return Float.parseFloat(floatStr);
    }

}
