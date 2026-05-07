#!/usr/bin/bash
java -jar ./3rd_party/@bin/antlr4.jar \
    -Werror \
    -no-listener \
    -visitor \
    -Dlanguage=Cpp \
    ./src/antlr4/SysY.g4
