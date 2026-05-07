package utils.tools;

import front.lexer.Token;
import utils.type.ErrorType;
import utils.type.SyntaxType;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.*;

public class Printer {

    private static boolean ON = true;
    public static boolean PERM = false;
    private static final HashMap<Integer, ErrorType> errorLog = new HashMap<>();
    private static FileOutputStream outStream;
    private static FileOutputStream errorStream;
    private static FileOutputStream oriLLVMStream;
    private static FileOutputStream mipsStream;

//    static {
//        try {
//            outStream = new FileOutputStream("output.txt");
//            errorStream = new FileOutputStream("error.txt");
//            oriLLVMStream = new FileOutputStream("llvm_ir.txt");
//            mipsStream = new FileOutputStream("mips.txt");
//        } catch (FileNotFoundException e) {
//            throw new RuntimeException(e);
//        }
//    }

    public static void switchOn() {
        ON = true;
    }

    public static void switchOff() {
        ON = false;
    }

    public static boolean errored() {
        return !errorLog.isEmpty();
    }

    public static void printSType(SyntaxType SType) {
        String str = "<" + SType.toString() + ">\n";
        if (ON & PERM) {
            try {
                outStream.write(str.getBytes());
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public static void printTType(Token token) {
        String str = token.toString() + "\n";
        if (ON & PERM) {
            try {
                outStream.write(str.getBytes());
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public static void recordError(int line, ErrorType EType) {
        errorLog.put(line, EType);
    }

    public static void printError() {
        if (ON) {
            Object[] lines = errorLog.keySet().toArray();
            Arrays.sort(lines);
            for (Object line : lines) {
                String info = line + " " + errorLog.get((Integer) line) + "\n";
                try {
                    errorStream.write((info).getBytes());
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        }
    }

    public static void printLLVM(Module module) {
        if (ON) {
            try {
                oriLLVMStream.write(module.toString().getBytes());
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }


}
