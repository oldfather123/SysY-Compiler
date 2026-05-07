package entities;

public class FOI {

    public static final FOI ZERO = new FOI(0), ONE = new FOI(1);
    private int a;
    private float b;
    private boolean isInteger;

    public FOI(int a) {
        this.a = a;
        isInteger = true;
    }

    public FOI(float b) {
        this.b = b;
        isInteger = false;
    }

    public FOI copy() {
        return isInteger ? new FOI(a) : new FOI(b);
    }

    public int getInt() {
        return isInteger ? a : (int) Math.floor(b);
    }

    public float getFloat() {
        return isInteger ? (float) a : b;
    }

    public static FOI add_(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return new FOI(a.getInt() + b.getInt());
        }
        return new FOI(a.getFloat() + b.getFloat());
    }

    public void add(FOI other) {
        if (isInteger && other.isInteger) {
            a += other.a;
        } else if (!isInteger && !other.isInteger) {
            b += other.b;
        } else {
            b = getFloat() + other.getFloat();
            isInteger = false;
        }
    }

    public static FOI sub_(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return new FOI(a.getInt() - b.getInt());
        }
        return new FOI(a.getFloat() - b.getFloat());
    }

    public void sub(FOI other) {
        if (isInteger && other.isInteger) {
            a -= other.a;
        } else if (!isInteger && !other.isInteger) {
            b -= other.b;
        } else {
            b = getFloat() - other.getFloat();
            isInteger = false;
        }
    }

    public static FOI mul_(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return new FOI(a.getInt() * b.getInt());
        }
        return new FOI(a.getFloat() * b.getFloat());
    }

    public void mul(FOI other) {
        if (isInteger && other.isInteger) {
            a *= other.a;
        } else if (!isInteger && !other.isInteger) {
            b *= other.b;
        } else {
            b = getFloat() * other.getFloat();
            isInteger = false;
        }
    }

    public static FOI div_(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return new FOI(a.getInt() / b.getInt());
        }
        return new FOI(a.getFloat() / b.getFloat());
    }

    public void div(FOI other) {
        if (isInteger && other.isInteger) {
            a /= other.a;
        } else if (!isInteger && !other.isInteger) {
            b /= other.b;
        } else {
            b = getFloat() / other.getFloat();
            isInteger = false;
        }
    }

    public static FOI mod_(FOI a, FOI b) {
        return new FOI(a.getInt() % b.getInt());
    }

    public void mod(FOI other) {
        if (isInteger && other.isInteger) {
            a %= other.a;
        } else {
            a = getInt() % other.getInt();
            isInteger = true;
        }
    }

    public FOI neg() {
        if (isInteger) {
            a = -a;
        } else {
            b = -b;
        }
        return this;
    }

    public static FOI lt(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() < b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() < b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public static FOI le(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() <= b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() <= b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public static FOI gt(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() > b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() > b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public static FOI ge(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() >= b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() >= b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public static FOI eq(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() == b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() == b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public static FOI ne(FOI a, FOI b) {
        if (a.isInteger && b.isInteger) {
            return a.getInt() != b.getInt() ? FOI.ONE : FOI.ZERO;
        }
        return a.getFloat() != b.getFloat() ? FOI.ONE : FOI.ZERO;
    }

    public FOI toFloat() {
        if (isInteger) {
            b = a;
            isInteger = false;
        }
        return this;
    }

    public FOI toInt() {
        if (!isInteger) {
            a = (int) Math.floor(b);
            isInteger = true;
        }
        return this;
    }

    public boolean isInteger() {
        return isInteger;
    }

    @Override
    public String toString() {
        if (isInteger) {
            return "I="+a;
        } else {
            return "F="+b;
        }
    }
}
