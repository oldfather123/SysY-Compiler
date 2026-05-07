package util;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class Strings {

    public static final String VOID_ = "void", INT_ = "int";
    public static final String IF_TRUE = "_IfTrue", IF_FALSE = "_IfFalse", IF_END = "_IfEnd";
    public static final String WHILE_COND = "_WhileCond", WHILE_BEGIN = "_WhileBegin", WHILE_END = "_WhileEnd";
    public static final String ADD_VAL = "AddVal", MUL_VAL = "MulVal", UNARY_VAL = "UnaryVal";
    public static final String GET_FLOAT_ARRAY = "getfarray", GET_INT_ARRAY = "getarray", GET_INT = "getint",
            GET_FLOAT = "getfloat", GET_CHAR = "getch", PUT_INT = "putint", PUT_CHAR = "putch", PUT_FLOAT = "putfloat",
            PUT_INT_ARRAY = "putarray", PUT_FLOAT_ARRAY = "putfarray", START_TIME = "starttime", STOP_TIME = "stoptime";
    public static final Set<String> PRE_FUNC_NAMES = new HashSet<>(List.of(GET_FLOAT_ARRAY, GET_INT_ARRAY, GET_INT, GET_FLOAT, GET_CHAR,
            PUT_INT, PUT_CHAR, PUT_FLOAT, PUT_INT_ARRAY, PUT_FLOAT_ARRAY, START_TIME, STOP_TIME));
    public static final String GET_FLOAT_ARRAY_ = "getfarray_______________", GET_INT_ARRAY_ = "getarray_______________",
    PUT_INT_ARRAY_ = "putarray_______________", PUT_FLOAT_ARRAY_ = "putfarray_______________";
    public static final String SYSY_START_TIME = "_sysy_starttime", SYSY_STOP_TIME = "_sysy_stoptime";
    public static final Set<String> POST_FUNC_NAMES = new HashSet<>(List.of(GET_FLOAT_ARRAY, GET_INT_ARRAY,
            GET_FLOAT_ARRAY_, GET_INT_ARRAY_, GET_INT, GET_FLOAT, GET_CHAR, PUT_INT, PUT_CHAR, PUT_FLOAT,
            PUT_INT_ARRAY, PUT_FLOAT_ARRAY, PUT_INT_ARRAY_, PUT_FLOAT_ARRAY_,
            START_TIME, STOP_TIME, SYSY_START_TIME, SYSY_STOP_TIME));
    public static final String RETURN_VALUE = "ReturnValue", SHORT_LABEL = "_ShortLabel";
    public static final String EQ_VAL = "EqVal", REL_VAL = "RelVal";
    public static final String _DATA = ".data", _BSS = ".bss", _TEXT_STARTUP = ".text.startup", MAIN = "main";
    public static final String _PRE = "......pre", GLOBAL = "......global";
    public static final String FCSR = "fcsr", RTZ = "rtz", _LC = ".LC", NEW_JUMP = "NewJump";
    public static final String LEFT_ARROW = "<-", RIGHT_ARROW = "->", DOUBLE_ARROW = "<>";
    public static final String XX = "qa4zp1lmw9sxo3kni7je5dc0ub8rfv6hy2gt";

}
