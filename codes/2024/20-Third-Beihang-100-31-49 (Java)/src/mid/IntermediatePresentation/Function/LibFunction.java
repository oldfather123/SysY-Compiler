package mid.IntermediatePresentation.Function;

import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.SymbolTable.SymbolTableManager;

public class LibFunction extends Function {
    public LibFunction(String ident, Param param, ValueType retType) {
        super(ident, param, retType);
        SymbolTableManager.getInstance().funcDecl(retType, ident);
        SymbolTableManager.getInstance().enterBlock();
        SymbolTableManager.getInstance().setIRValue(ident, this);
        SymbolTableManager.getInstance().funcDeclEnd();
    }

    public String toString() {
        return "declare " + vType.toString() + " " + reg + "(" + param + ")\n";
    }

    public static class Getint extends LibFunction {
        public Getint() {
            super("@getint", new Param(), ValueType.I32);
        }
    }

    public static class Getch extends LibFunction {
        public Getch() {
            super("@getch", new Param(), ValueType.I32);
        }
    }

    public static class Getfloat extends LibFunction {
        public Getfloat() {
            super("@getfloat", new Param(), ValueType.FLT);
        }
    }

    public static class Getarray extends LibFunction {
        public Getarray() {
            super("@getarray",
                    new Param(new Value("%param_0", ValueType.PI32)), ValueType.I32);
        }
    }

    public static class Getfarray extends LibFunction {
        public Getfarray() {
            super("@getfarray",
                    new Param(new Value("%param_0", ValueType.PFLT)), ValueType.I32);
        }
    }

    public static class Putint extends LibFunction {
        public Putint() {
            super("@putint",
                    new Param(new Value("%param_0", ValueType.I32)), ValueType.NULL);
        }
    }

    public static class Putch extends LibFunction {
        public Putch() {
            super("@putch",
                    new Param(new Value("%param_0", ValueType.I32)), ValueType.NULL);
        }
    }

    public static class Putfloat extends LibFunction {
        public Putfloat() {
            super("@putfloat",
                    new Param(new Value("%param_0", ValueType.FLT)), ValueType.NULL);
        }
    }

    public static class Putarray extends LibFunction {
        public Putarray() {
            super("@putarray",
                    new Param(new Value("%param_0", ValueType.I32),
                            new Value("%param_1", ValueType.PI32)), ValueType.NULL);
        }
    }

    public static class Putfarray extends LibFunction {
        public Putfarray() {
            super("@putfarray",
                    new Param(new Value("%param_0", ValueType.I32),
                            new Value("%param_1", ValueType.PFLT)), ValueType.NULL);
        }
    }

    public static class Starttime extends LibFunction {
        public Starttime() {
            super("@_sysy_starttime", new Param(), ValueType.NULL);
        }
    }

    public static class Stoptime extends LibFunction {
        public Stoptime() {
            super("@_sysy_stoptime", new Param(), ValueType.NULL);
        }
    }

    public static class Memset extends LibFunction {
        // memset(ptr,val,size)
        public Memset() {
            super("@memset",
                    new Param(new Value("%param_0", ValueType.PI32),
                            new Value("%param_1", ValueType.I32),
                            new Value("%param_2", ValueType.I32)), ValueType.NULL);
        }
    }
}
