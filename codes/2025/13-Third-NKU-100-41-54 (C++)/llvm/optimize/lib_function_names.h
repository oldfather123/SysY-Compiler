#ifndef LIB_FUNCTION_NAMES_H
#define LIB_FUNCTION_NAMES_H
#include <unordered_set>
#include <string>

inline const std::unordered_set<std::string> lib_function_names = {
    "getint", "getch", "getfloat", "getarray", "getfarray",
    "putint", "putch", "putfloat", "putarray", "putfarray",
    "_sysy_starttime", "_sysy_stoptime",
    "llvm.memset.p0.i32", "llvm.umax.i32", "llvm.umin.i32",
    "llvm.smax.i32", "llvm.smin.i32", "___parallel_loop_constant_100"
};

#endif 