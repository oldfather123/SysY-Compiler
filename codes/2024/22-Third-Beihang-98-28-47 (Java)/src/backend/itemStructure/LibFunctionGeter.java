package backend.itemStructure;

import java.util.HashMap;

public class LibFunctionGeter {
    HashMap<String,AsmFunction> libFunctions = new HashMap<String,AsmFunction>();
    public LibFunctionGeter(){
        libFunctions.put("putint", new AsmFunction("putint"));
        libFunctions.put("getint", new AsmFunction("getint"));
        libFunctions.put("putch", new AsmFunction("putch"));
        libFunctions.put("getch", new AsmFunction("getch"));
        libFunctions.put("putarray", new AsmFunction("putarray"));
        libFunctions.put("getarray", new AsmFunction("getarray"));
        libFunctions.put("putfloat", new AsmFunction("putfloat"));
        libFunctions.put("getfloat", new AsmFunction("getfloat"));
        libFunctions.put("putfarray", new AsmFunction("putfarray"));
        libFunctions.put("getfarray", new AsmFunction("getfarray"));
        libFunctions.put("memset", new AsmFunction("memset"));
        libFunctions.put("_sysy_starttime", new AsmFunction("_sysy_starttime"));
        libFunctions.put("_sysy_stoptime", new AsmFunction("_sysy_stoptime"));
    }
}
