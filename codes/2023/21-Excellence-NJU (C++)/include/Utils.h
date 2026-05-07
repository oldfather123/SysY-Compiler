//
// Created by q1w2e3r4 on 23-7-4.
//

#ifndef COMPILER_UTILS_H
#define COMPILER_UTILS_H
#include "antlr4-runtime.h"

class Utils {
public:

    static int tempCounter;

    static int labelCounter;

    static void setTempCounter(int n){
        tempCounter = n;
    }

    static int getTempCounter(){
        return tempCounter;
    }

    static std::string getNewTamp(){
        return "%" + std::to_string(tempCounter++);
    }

    static std::string getNewLabel(const std::string& label){
        return label + std::to_string(labelCounter++);
    }

};


#endif //COMPILER_UTILS_H
