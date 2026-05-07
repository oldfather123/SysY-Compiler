package utils;

import frontend.lexer.TokenType;

public class Calculator {
    public static Object Add(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a + (int) b;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a + (float) b;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a + (float) b;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a + (int) b;
        } else {
            return null;
        }
    }

    public static Object Sub(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a - (int) b;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a - (float) b;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a - (float) b;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a - (int) b;
        } else {
            return null;
        }
    }

    public static Object Mul(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a * (int) b;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a * (float) b;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a * (float) b;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a * (int) b;
        } else {
            return null;
        }
    }

    public static Object Div(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a / (int) b;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a / (float) b;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a / (float) b;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a / (int) b;
        } else {
            return null;
        }
    }

    public static Object Mod(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a % (int) b;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a % (float) b;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a % (float) b;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a % (int) b;
        } else {
            return null;
        }
    }

    public static Object Lt(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a < (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a < (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a < (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a < (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Gt(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a > (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a > (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a > (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a > (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Le(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a <= (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a <= (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a <= (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a <= (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Ge(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a >= (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a >= (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a >= (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a >= (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Eq(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a == (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a == (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a == (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a == (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Neq(Object a, Object b) {
        if (a instanceof Integer && b instanceof Integer) {
            return (int) a != (int) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Float) {
            return (float) a != (float) b ? 1 : 0;
        } else if (a instanceof Integer && b instanceof Float) {
            return (int) a != (float) b ? 1 : 0;
        } else if (a instanceof Float && b instanceof Integer) {
            return (float) a != (int) b ? 1 : 0;
        } else {
            return null;
        }
    }

    public static Object Neg(Object a) {
        if (a instanceof Integer) {
            return -(int) a;
        } else if (a instanceof Float) {
            return -(float) a;
        } else {
            return null;
        }
    }

    public static Object Not(Object a) {
        if (a instanceof Integer) {
            return (int) a == 0 ? 1 : 0;
        } else if (a instanceof Float) {
            return (float) a == 0 ? 1 : 0;
        } else {
            return null;
        }
    }
}
